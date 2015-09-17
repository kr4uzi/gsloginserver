#include "gpcm.h"
#include "protocol.h"
#include "database.h"
#include <vector>
#include <iostream>
#include <algorithm>
using namespace std;
using namespace gamespy::gpcm;
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
	: service(service), socket(std::move(socket)), protocol(new gamespy::Protocol()), database(database), heartbeat(service)
{
	cout << "gpcm session created" << endl;
}

Session::~Session()
{
	cout << "gpcm session deleted" << endl;
	close();
	delete protocol;
}

void Session::doSend() {
	auto self = shared_from_this();
	socket.async_write_some(boost::asio::buffer(outgoing), [this, self](auto error, auto bytes_transfered) {
		if (!error) {
			cout << "gpcm -> " << std::string(outgoing.begin(), outgoing.begin() + bytes_transfered) << endl;
			outgoing.erase(outgoing.begin(), outgoing.begin() + bytes_transfered);
			last_sent = std::chrono::system_clock::now();

			if (!outgoing.empty())
				doSend();
		}
		else {
			cout << "[GPCM] read error, closing" << endl;
			close();
		}
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
				cout << "gpcm <- " << std::string(incoming.begin(), incoming.begin() + length) << endl;
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
	outgoing += protocol->SERVER_SEND_CHALLENGE();
	doSend();

	doReceive(std::bind(&Session::listen, shared_from_this(), std::placeholders::_1));
}

void Session::listen(gamespy::Packet& packet)
{
	if (!user) { // !user == !logged_in
		if (packet.get_type() == "login") // general login and retrieve account by name
			login(packet);
		else if (packet.get_type() == "newuser")
			newUser(packet);
		//else log unexpected message
	}
	else {
		if (packet.get_type() == "getprofile")
			getProfile(packet); //custom onReceive here because 2 getprofile's in a row means retrieve account
		else if (packet.get_type() == "updatepro")
			updateProfile(packet);
		else if (packet.get_type() == "logout")
			logout(packet);
		//else log unexpected message
	}
}

void Session::close() 
{
	heartbeat.cancel();
	socket.close();
}

void Session::login(gamespy::Packet& packet)
{
	auto self = shared_from_this();
	if (packet.has_key("uniquenick") && packet.has_key("challenge") && packet.has_key("response")) {
		auto user = database.get_user_from_uniquenick(packet["uniquenick"]);
		if (user) {
			//can we generate the same response with the password stored in our database?
			if (protocol->SERVER_VALIDATE_RESPONSE(user->get_uniquenick(), user->get_hashed_password(), packet["challenge"], packet["response"])) {
				outgoing += protocol->SERVER_SEND_PROOF(user->get_uniquenick(), user->get_hashed_password(), user->get_user_id(), user->get_profiles()[0]->get_profile_id(), packet["challenge"]);

				//not sure if it's necessary to start heartbeat earlier
				startHeartbeat();
				this->user = user;
			}
			else
				outgoing += protocol->SERVER_SEND_ERROR(gamespy::Error::INVALID_CREDENTIALS, "The password provided is incorrect.");
		}
		else {
			outgoing += protocol->SERVER_SEND_ERROR(gamespy::Error::INVALID_CREDENTIALS, "Username does not exist.");
		}
	}
	//else log unexpected message
	
	doSend();
	doReceive(std::bind(&Session::listen, self, std::placeholders::_1));
}

void Session::newUser(gamespy::Packet& packet)
{
	if (packet.has_key("nick") && packet.has_key("email") && packet.has_key("passwordenc")) {
		auto password = protocol->MD5(gamespy::gamespy_passdecode(packet["passwordenc"]));
		auto user = database.get_user_from_email_and_password(packet["email"], password);
		auto& profiles = user->get_profiles();
		if (user && std::find_if(profiles.begin(), profiles.end(), [&packet](auto profile) { return profile->get_nick() == packet["nick"]; }) != profiles.end())
			outgoing += protocol->SERVER_SEND_ERROR(gamespy::Error::NICKNAME_IN_USE, "This account name is already in use!");
		else {
			auto user = database.insert_new_user(packet["nick"], password, packet["email"], "??");
			outgoing += protocol->SERVER_SEND_NEW_PROFILE(user->get_user_id(), user->get_profiles()[0]->get_profile_id());
		}

		doSend();
	}
	//else log unexpected message
	
	auto self = shared_from_this();
	doReceive(std::bind(&Session::listen, self, std::placeholders::_1));
}

void Session::getProfile(gamespy::Packet& packet)
{	
	outgoing += protocol->SERVER_SEND_PROFILE(user->get_uniquenick(), user->get_profiles()[0]->get_nick(), user->get_email(), user->get_profiles()[0]->get_profile_id(), user->get_user_id(), user->get_country(), false);
	doSend();

	auto self = shared_from_this();
	doReceive([this, self](auto packet) {
		if (packet.get_type() == "getprofile")
			retrieveProfile(packet);
		else
			listen(packet);
	});
}

void Session::retrieveProfile(gamespy::Packet& packet)
{
	outgoing += protocol->SERVER_SEND_PROFILE(user->get_uniquenick(), user->get_profiles()[0]->get_nick(), user->get_email(), user->get_profiles()[0]->get_profile_id(), user->get_user_id(), user->get_country(), false);
	doSend();

	auto self = shared_from_this();
	doReceive(std::bind(&Session::listen, self, std::placeholders::_1));
}

void Session::updateProfile(gamespy::Packet& packet)
{	
	if (packet.has_key("country")) {
		user->set_country(packet["country"]);
		database.update_user(user);
	}
	//else log unexpected message

	auto self = shared_from_this();
	doReceive(std::bind(&Session::listen, self, std::placeholders::_1));
}

void Session::logout(gamespy::Packet& packet)
{
	close();
}

void Session::startHeartbeat()
{
	auto self = shared_from_this();
	heartbeat.expires_from_now(boost::posix_time::minutes(1));
	heartbeat.async_wait([this, self](const boost::system::error_code& error) {
		if (!error) {
			auto diff = std::chrono::system_clock::now() - last_sent;
			if (diff >= std::chrono::minutes(1)) {
				outgoing += protocol->SERVER_SEND_HEARTBEAT();
				outgoing += protocol->SERVER_SEND_KEEP_ALIVE();
				doSend();
			}

			startHeartbeat();
		}
	});
}