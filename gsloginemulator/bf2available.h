#pragma once

#include <boost/asio.hpp>
#include <array>

namespace gamespy {
	namespace available {
		namespace battlefield2 {
			class Server {
			public:
				static constexpr unsigned PORT = 27900;

			private:
				boost::asio::io_service& service;
				boost::asio::ip::udp::socket socket;
				boost::asio::ip::udp::endpoint remote_endpoint;
				std::array<char, 1024> recv_buffer;

			public:
				Server(boost::asio::io_service& service);

			private:
				void doAccept();
				void doSend();
			};
		}
	}
}