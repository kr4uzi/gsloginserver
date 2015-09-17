#pragma once
#include <string>
#include <map>
#include <vector>

namespace gamespy {
	std::string gamespy_xor(const std::string& string);
	std::string gamespy_passencode(const std::string& password);
	std::string gamespy_passdecode(std::string password);

	enum class Error {
		TEST1 = 500, TEST2 = 501, TEST3 = 502,
		TEST4 = 504, TEST5 = 505, TEST6 = 506, TEST7 = 507,
		TEST8 = 508, TEST9 = 509, TEST10 = 510,
		TEST11 = 511, TEST12 = 512, TEST13 = 513, TEST14 = 514, TEST15 = 515,
		NICKNAME_IN_USE = 516,
		INVALID_CREDENTIALS = 260,
		FATAL = 265,
		//The account is invalid. Either the name is incorrect or email address is incorrect or already in use. Please try again. (retrieve account with email and no connction/conn abort)
	};

	enum class Game {
		Battlefield2,
		SoldierOfAnarchy,
		Halo
	};

	class Packet : public std::map<std::string, std::string> {
	private:
		std::string type;

	public:
		std::string get_type() const				{ return type; }
		void set_type(const std::string& type)		{ this->type = type; }
		bool has_key(const std::string& key) const	{ return find(key) != cend(); }

		bool has_password_key() const				{ return has_key("passenc") || has_key("pass"); }
		std::string get_password_value() const;

		bool has_nick_key() const					{ return has_key("uniquenick") || has_key("nick"); }
		std::string get_nick_value() const;
	};
	
	class Protocol {
	private:
		const std::string packet_end = R"(\final\)";
		std::string challenge;
	
	private:
		std::string generate_random_string(const std::string& seed, std::string::size_type length);
		std::uint16_t generate_session_id(const std::string& nick);
		std::string generate_response(const std::string& nickname, const std::string& hashedPassword, const std::string& localChallenge, const std::string& remoteChallenge);

	public:
		std::string::size_type EXTRACT_SIZE(const std::string& data);
		Packet EXTRACT_PACKET(const std::string& data, bool& isValid);

		std::string MD5(const std::string& data);
	
		std::string SERVER_SEND_CHALLENGE();
		std::string SERVER_SEND_NICKS(std::vector<std::string> names);
		std::string SERVER_SEND_CHECK(std::uint32_t profile_id);
		std::string SERVER_SEND_NEW_PROFILE(std::uint32_t user_id, std::uint32_t profile_id);
		std::string SERVER_SEND_ERROR(const Error& error, std::string error_message);

		bool SERVER_VALIDATE_RESPONSE(const std::string& unique_nick, const std::string& hashedPassword, const std::string& clientChallenge, const std::string& clientResponse);
		std::string SERVER_SEND_PROOF(const std::string& unique_nick, const std::string& hashedPassword, std::uint32_t userId, std::uint32_t profileId, std::string& clientChallenge);
		std::string SERVER_SEND_PROFILE(const std::string& unique_nick, const std::string& nickname, const std::string& email, std::uint32_t profile_id, std::uint32_t user_id, const std::string& country, bool retrieve);
		std::string SERVER_SEND_HEARTBEAT();
		std::string SERVER_SEND_KEEP_ALIVE();
	};
}