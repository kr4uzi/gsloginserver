#include "bf2available.h"
#include <cstring>
using namespace gamespy::available::battlefield2;
using boost::asio::ip::udp;

Server::Server(boost::asio::io_service& service)
	: service(service), socket(service, udp::endpoint(udp::v4(), PORT))
{
	doAccept();
}

void Server::doAccept() {
	static const char expected[] = {
		0x09, 0x00, 0x00, 0x00, 0x00, 0x62, 0x61, 0x74, 0x74,
		0x6c, 0x65, 0x66, 0x69, 0x65, 0x6c, 0x64, 0x32, 0x00
	};

	socket.async_receive_from(boost::asio::buffer(recv_buffer), remote_endpoint, [=](auto error, auto bytes_received) {
		if (!error) {
			if (bytes_received == sizeof(expected) && std::memcmp(recv_buffer.data(), expected, sizeof(expected)) == 0) {
				return doSend();
			}
		}

		doAccept();
	});
}

void Server::doSend() {
	static const unsigned char send[] = {
		0xfe, 0xfd, 0x09, 0x00, 0x00, 0x00, 0x00
	};

	socket.async_send_to(boost::asio::buffer(send, sizeof(send)), remote_endpoint, [=](auto error, auto bytes_transfered) {
		doAccept();
	});
}