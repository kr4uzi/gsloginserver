#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <functional>
#include <chrono>
#include "database.h"

namespace gamespy {
	class Packet;
	class Protocol;

	namespace gpcm {
		class Session : public std::enable_shared_from_this<Session> {
		private:
			boost::asio::io_service& service;
			boost::asio::ip::tcp::socket socket;
			boost::asio::deadline_timer heartbeat;
			std::string incoming;
			std::chrono::system_clock::time_point last_sent;
			std::string outgoing;
			
			gamespy::database::Database& database;
			gamespy::Protocol * protocol;
			gamespy::database::GamespyUser user;

		private:
			void doSend();
			void doReceive(std::function<void(gamespy::Packet&)> onReceive);

		public:
			Session(boost::asio::io_service& service, boost::asio::ip::tcp::socket socket, gamespy::database::Database& database);
			~Session();

			void start();
			void close();

		private:
			void listen(gamespy::Packet& packet);
			void login(gamespy::Packet& packet);
			void newUser(gamespy::Packet& packet);
			void getProfile(gamespy::Packet& packet);
			void retrieveProfile(gamespy::Packet& packet);
			void updateProfile(gamespy::Packet& packet);
			void logout(gamespy::Packet& packet);
			void startHeartbeat();
		};

		class Server {
		public:
			static constexpr unsigned PORT = 29900;

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
