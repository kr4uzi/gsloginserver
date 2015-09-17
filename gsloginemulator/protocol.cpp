#include "protocol.h"
#include <openssl/md5.h>
#include <cstdio>
#include <vector>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/algorithm/string.hpp>
#include <regex>
#include <algorithm>
#include <sstream>
#include <random>
#include <boost/crc.hpp>
using namespace gamespy;

namespace {
	//http://www.firstpr.com.au/dsp/rand31/
	long unsigned int rand31_next(long seed) {
		long unsigned int hi, lo;

		lo = 0x41a7 * (seed & 0xFFFF);
		hi = 0x41a7 * (seed >> 16);

		lo += (hi & 0x7FFF) << 16;
		lo += hi >> 15;
		lo = (lo & 0x7FFFFFFF) + (lo >> 31);

		return (seed = (long)lo);
	}

	//http://aluigi.altervista.org/papers/gspassenc.zip
	std::string gspassenc(const std::string& password) {
		int seed = 0x79707367; // "gspy" little endian
		std::string encoded(password);

		for (std::string::size_type i = 0; i < password.length(); i++) {
			seed = rand31_next(seed);
			encoded[i] ^= (seed % 0xFF);
		}

		return encoded;
	}

	//http://stackoverflow.com/a/28471421
	std::string base64_decode(const std::string& data) {
		using namespace boost::archive::iterators;
		using It = transform_width<binary_from_base64<std::string::const_iterator>, 8, 6>;
		return boost::algorithm::trim_right_copy_if(std::string(It(std::begin(data)), It(std::end(data))), [](char c) {
			return c == '\0';
		});
	}

	std::string base64_encode(const std::string& data) {
		using namespace boost::archive::iterators;
		using It = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
		auto tmp = std::string(It(std::begin(data)), It(std::end(data)));
		return tmp.append((3 - data.size() % 3) % 3, '=');
	}
}

namespace gamespy {
	std::string gamespy_xor(const std::string& string) {
		static const char gamespy[] = "gamespy";
		std::string encoded = string;

		for (std::string::size_type i = 0; i < string.size(); i++) {
			encoded[i] ^= gamespy[i % sizeof(gamespy)];
		}

		return encoded;
	}

	std::string gamespy_passencode(const std::string& password) {
		auto base64 = base64_encode(gspassenc(password));
		boost::replace_all(base64, "=", "_");
		boost::replace_all(base64, "+", "[");
		boost::replace_all(base64, "/", "]");

		return base64;
	}

	std::string gamespy_passdecode(std::string password) {
		boost::replace_all(password, "_", "=");
		boost::replace_all(password, "[", "+");
		boost::replace_all(password, "]", "/");

		return gspassenc(base64_decode(password));
	}
}

std::string Packet::get_password_value() const
{
	const_iterator iter;
	if ((iter = find("passenc")) != cend())
		return gamespy_passdecode(iter->second);
	else if ((iter = find("pass")) != cend())
		return iter->second;

	//get_password_value is only allowed after a successfull has_password_key
}

std::string Packet::get_nick_value() const
{
	const_iterator iter;
	if ((iter = find("uniquenick")) != cend())
		return iter->second;
	else if ((iter = find("nick")) != cend())
		return iter->second;

	//get_nick_value is only allowed after a successfull has_nick_key
}

std::string::size_type Protocol::EXTRACT_SIZE(const std::string& data) {
	auto iter = data.find(packet_end);

	if (iter != std::string::npos)
		iter += packet_end.length();

	return iter;
}

std::string Protocol::MD5(const std::string& data)
{
	unsigned char digest[16];
	::MD5(reinterpret_cast<const unsigned char *>(data.c_str()), data.length(), digest);
	char md5string[33];
	for (int i = 0; i < 16; ++i)
		std::sprintf(&md5string[i * 2], "%02x", digest[i]);
	return std::string(md5string);
}

Packet Protocol::EXTRACT_PACKET(const std::string& data, bool& isValid) {
	Packet packet;
	isValid = false;

	std::regex packet_pattern(R"PACKET(^\\(\w+)\\(.*)\\final\\)PACKET");
	std::smatch packet_match;
	if (std::regex_match(data, packet_match, packet_pattern)) {
		packet.set_type(packet_match[1]);

		if (packet_match.size() == 3) {
			std::string data = packet_match[2];
			std::regex data_pattern(R"DATA(\\([^\\]*)\\([^\\]*))DATA");
			std::smatch data_match;
			while (std::regex_search(data, data_match, data_pattern)) {
				//lowercase the key
				std::string key = data_match[1];
				boost::algorithm::to_lower(key);
				packet[key] = data_match[2];
				data = data_match.suffix().str();
			}

			isValid = true;
		}
	}

	return packet;
}

std::string Protocol::generate_random_string(const std::string& seed, std::string::size_type length)
{
	std::default_random_engine engine;
	std::string randomString(length, ' ');

	while (length--)
		randomString[length] = seed[engine() % seed.length()];

	return randomString;
}

std::uint16_t Protocol::generate_session_id(const std::string& nick)
{
	boost::crc_16_type session;
	session.process_bytes(nick.c_str(), nick.length());
	return session.checksum();
}

std::string Protocol::SERVER_SEND_CHALLENGE() {
	// the protocol is meant to be used one side only
	// (one instance for each, client and server)
	challenge = generate_random_string("ABCDEFGHIJKLMNOPQRSTUVWXYZ", 10);

	std::string packet(R"(\lc\1\challenge\)");
	packet += challenge;
	packet += R"(\id\1)";
	packet += packet_end;

	return packet;
}

std::string Protocol::SERVER_SEND_NICKS(std::vector<std::string> users) {
	std::stringstream packet;
	packet << R"(\nr\)" << users.size();

	for (auto& user : users) {
		packet << R"(\nick\)" << user << R"(\uniquenick\)" << user;
	}
	packet << R"(\ndone\)";

	packet << packet_end;
	return packet.str();
}

std::string Protocol::SERVER_SEND_CHECK(std::uint32_t profile_id) {
	std::stringstream packet;
	packet << R"(\cur\)" << 0;
	packet << R"(\pid\)" << profile_id;
	packet << packet_end;

	return packet.str();
}

std::string Protocol::SERVER_SEND_ERROR(const Error& error, std::string error_message) {
	boost::replace_all(error_message, R"(\)", R"(/)");

	static unsigned int err = 500;

	std::stringstream packet;
	packet << R"(\error\)";
	packet << R"(\err\)" << err++/*static_cast<std::underlying_type<Error>::type>(error)*/ << R"(\fatal\)";

	switch (error) {
	case Error::FATAL:
	case Error::NICKNAME_IN_USE:
	case Error::INVALID_CREDENTIALS:
		packet << R"(\errormsg\)" << error_message;
		break;
	}

	packet << R"(\id\)" << 1;
	packet << packet_end;
	return packet.str();
}

std::string Protocol::generate_response(const std::string& nickname, const std::string& hashedPassword, const std::string& localChallenge, const std::string& remoteChallenge)
{
	std::string response;
	response += hashedPassword;
	response += std::string(48, ' ');
	response += nickname;
	response += localChallenge;
	response += remoteChallenge;
	response += hashedPassword;

	return MD5(response);
}

std::string Protocol::SERVER_SEND_PROOF(const std::string& unique_nick, const std::string& hashedPassword, std::uint32_t userId, std::uint32_t profileId, std::string& clientChallenge)
{
	std::stringstream proof;
	proof << R"(\lc\)" << 2;
	proof << R"(\sesskey\)" << generate_session_id(unique_nick);
	proof << R"(\proof\)" << generate_response(unique_nick, hashedPassword, challenge, clientChallenge);
	proof << R"(\userid\)" << userId;
	proof << R"(\profileid\)" << profileId;
	proof << R"(\uniquenick\)" << unique_nick;
	proof << R"(\lt\)" << generate_random_string("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ][", 22) << "__";
	proof << R"(\id\)" << 1;
	proof << packet_end;

	return proof.str();
}

bool Protocol::SERVER_VALIDATE_RESPONSE(const std::string& unique_nick, const std::string& hashedPassword, const std::string& clientChallenge, const std::string& clientResponse)
{
	return clientResponse == generate_response(unique_nick, hashedPassword, clientChallenge, challenge);
}

std::string Protocol::SERVER_SEND_NEW_PROFILE(std::uint32_t user_id, std::uint32_t profile_id)
{
	std::stringstream packet;
	packet << R"(\nur\)";
	packet << R"(\userid\)" << user_id;
	packet << R"(\profileid\)" << profile_id;
	packet << R"(\id\)" << 1;
	packet << packet_end;

	return packet.str();
}

std::string Protocol::SERVER_SEND_PROFILE(const std::string& unique_nick, const std::string& nickname, const std::string& email, std::uint32_t profile_id, std::uint32_t user_id, const std::string& country, bool retrieve)
{
	std::stringstream packet;
	packet << R"(\pi\)";
	packet << R"(\profileid\)" << profile_id;
	packet << R"(\nick\)" << unique_nick;
	packet << R"(\userid\)" << user_id;
	packet << R"(\email\)" << email;
	packet << R"(\sig\)" << generate_random_string("0123456789abcdef", 32);
	packet << R"(\uniquenick\)" << nickname;
	packet << R"(\pid\)" << 0;
	packet << R"(\firstname\)";
	packet << R"(\lastname\)";
	//packet << R"(\homepage\)";
	//packet << R"(\zipcode\)";
	packet << R"(\countrycode\)" << country;
	//packet << R"(\st\)" << "  ";
	packet << R"(\birthday\)" << 16844722;
	//packet << R"(\pmask\)" << 64;
	//packet << R"(\conn\)" << 0;
	//packet << R"(\i1\)" << 0;
	//packet << R"(\o1\)" << 0;
	//packet << R"(\mp\)" << 0;
	packet << R"(\lon\)" << "0.000000";
	packet << R"(\lat\)" << "0.000000";
	packet << R"(\loc\)";
	packet << R"(\id\)" << (retrieve ? 5 : 2);
	packet << packet_end;

	return packet.str();
}

std::string Protocol::SERVER_SEND_HEARTBEAT()
{
	std::string packet;
	packet += R"(\lt\)";
	packet += generate_random_string("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ][", 22);
	packet += "__";
	packet += packet_end;

	return packet;
}

std::string Protocol::SERVER_SEND_KEEP_ALIVE()
{
	std::string packet;
	packet += R"(\ka\)";
	packet += packet_end;

	return packet;
}

