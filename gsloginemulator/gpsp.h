#pragma once
#include <boost/asio.hpp>
#include <memory> //shared_from_this
#include <string>
#include <map>
#include <functional>

namespace gamespy {
	class Packet;
	class Protocol;

	namespace database {
		class Database;
	}

	namespace gpsp {
		class Session : public std::enable_shared_from_this<Session> {
		private:
			boost::asio::io_service& service;
			boost::asio::ip::tcp::socket socket;
			std::string incoming;
			std::string outgoing;

			gamespy::Protocol * protocol;
			gamespy::database::Database& database;

		private:
			void doSend();
			void doReceive(std::function<void(gamespy::Packet&)> onReceive);

		public:
			Session(boost::asio::io_service& service, boost::asio::ip::tcp::socket socket, gamespy::database::Database& database);
			~Session();

			void start();
			void close();

			void doListen(gamespy::Packet& packet);
		};

		class Server {
		public:
			static constexpr unsigned PORT = 29901;

		private:
			boost::asio::io_service& service;
			boost::asio::ip::tcp::acceptor acceptor;
			boost::asio::ip::tcp::socket socket;
			gamespy::database::Database& database;

		public:
			Server(boost::asio::io_service& service, gamespy::database::Database& database);

			void start();

		private:
			void doAccept();
		};
	}
}
