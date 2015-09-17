#pragma once

#include <cstdint>
#include <string>
#include <tuple>
#include <map>
#include <vector>
#include <memory>

namespace gamespy {
	namespace database {
		class Profile {
		private:
			std::uint32_t profile_id;
			std::string nick;

		public:
			Profile(std::uint32_t profile_id, const std::string& nick) : profile_id(profile_id), nick(nick) { }

			std::string get_nick() { return nick; }
			std::uint32_t get_profile_id() { return profile_id; }
		};

		typedef std::shared_ptr<Profile> GamespyProfile;

		class User {
		private:
			std::string uniquenick;
			std::string password;
			std::string email;
			std::uint32_t user_id;
			std::string country;
			std::vector<GamespyProfile> profiles;

		public:
			User(std::uint32_t user_id, const std::string& uniquenick, const std::string& password, const std::string& email, const std::string& country)
				: uniquenick(uniquenick), password(password), email(email), user_id(user_id), country(country)
			{

			}

			std::string get_uniquenick() { return uniquenick; }
			std::string get_hashed_password() { return password; }
			std::string get_email() { return email; }
			std::uint32_t get_user_id() { return user_id; }
			std::string get_country() { return country; }
			const std::vector<GamespyProfile>& get_profiles() { return profiles; }

			void set_country(const std::string& country) { this->country = country; }
			void add_profile(GamespyProfile profile) { profiles.push_back(profile); }
		};

		typedef std::shared_ptr<User> GamespyUser;

		//interface
		class Database {
		public:
			virtual GamespyUser get_user_from_email_and_password(const std::string& email, const std::string& hashedPassword) = 0;
			virtual GamespyUser get_user_from_uniquenick(const std::string& uniquenick) = 0;
			virtual std::vector<GamespyProfile> get_users_from_nick(const std::string& nick) = 0;
			virtual GamespyUser insert_new_user(const std::string& uniquenick, const std::string& hashedPassword, const std::string& email, const std::string& country) = 0;
			virtual void update_user(GamespyUser user) = 0;
		};
	}
}