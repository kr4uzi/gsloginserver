#include "gpsp.h"
#include "protocol.h"
#include "database.h"
#include <string>
#include <cassert>
#include <iostream>
using namespace std;
using namespace gamespy::gpsp;
using boost::asio::ip::tcp;

Server::Server(boost::asio::io_service& service, gamespy::database::Database& database)
	: service(service), acceptor(service, tcp::endpoint(tcp::v4(), PORT)), socket(service), database(database)
{

}

void Server::start()
{
	doAccept();
}


void Server::doAccept()
{
	acceptor.async_accept(socket, [this](auto error) {
		if (!error)
			std::make_shared<Session>(service, std::move(socket), database)->start();

		doAccept();
	});
}

Session::Session(boost::asio::io_service& service, tcp::socket socket, gamespy::database::Database& database)
	: service(service), socket(std::move(socket)), protocol(new gamespy::Protocol()), database(database)
{
	cout << "gpsp session created" << endl;
}

Session::~Session() 
{
	close();
	delete protocol;
	cout << "gpsp session deleted" << endl;
}

void Session::doSend() 
{
	auto self = shared_from_this();
	socket.async_write_some(boost::asio::buffer(outgoing), [this, self](auto error, auto bytes_transfered) {
		if (!error) {
			cout << "gpsp -> " << std::string(outgoing.begin(), outgoing.begin() + bytes_transfered) << endl;
			outgoing.erase(outgoing.begin(), outgoing.begin() + bytes_transfered);

			if (!outgoing.empty()) 
				doSend();
		}
		else 
			close();
	});
}

void Session::doReceive(std::function<void(gamespy::Packet&)> onReceive)
{
	auto self = shared_from_this();
	auto readBuffer = std::make_shared<std::array<char, 1024>>();

	socket.async_read_some(boost::asio::buffer(*readBuffer), [=](auto error, auto bytes_transfered) {
		if (!error) {
			incoming.append(readBuffer->data(), bytes_transfered);

			auto length = protocol->EXTRACT_SIZE(incoming);
			if (length != std::string::npos) {
				bool isValid;
				auto packet = protocol->EXTRACT_PACKET(std::string(incoming.begin(), incoming.begin() + length), isValid);
				cout << "gpsp <- " << std::string(incoming.begin(), incoming.begin() + length) << endl;
				if (isValid) {
					onReceive(packet);
					incoming.erase(incoming.begin(), incoming.begin() + length);
				}
			}
			else
				doReceive(onReceive);
		}
	});
}

void Session::start() 
{
	auto self = shared_from_this();
	doReceive(std::bind(&Session::doListen, self, std::placeholders::_1));
}

void Session::close() 
{
	socket.close();
}

void Session::doListen(gamespy::Packet& packet)
{
	auto self = shared_from_this();
	if (packet.get_type() == "nicks" && packet.has_key("email") && packet.has_key("pass")) {
		auto user = database.get_user_from_email_and_password(packet["email"], protocol->MD5(packet["pass"]));
		std::vector<std::string> names;
		if (user) {
			for (auto& profile : user->get_profiles())
				names.push_back(profile->get_nick());
		}

		outgoing.append(protocol->SERVER_SEND_NICKS(names));
		doSend();

		doReceive(std::bind(&Session::doListen, self, std::placeholders::_1));
	}
	else if (packet.get_type() == "check" &&
		// either nick or uniquenick
		packet.has_key("nick")) {
		
		auto users = database.get_users_from_nick(packet["nick"]);
		if (users.size() >= 1)
			outgoing += protocol->SERVER_SEND_CHECK(users[0]->get_profile_id());
		
		doSend();
		doReceive(std::bind(&Session::doListen, self, std::placeholders::_1));
	}
	// log unexpected error
}