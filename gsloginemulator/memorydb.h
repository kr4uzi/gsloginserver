#pragma once
#include "database.h"
#include <list>

namespace gamespy {
	namespace database {
		class MemoryDB : public Database {
		private:
			std::list<GamespyUser> users;
			std::list<GamespyProfile> profiles;

		public:
			GamespyUser get_user_from_email_and_password(const std::string& email, const std::string& hashedPassword) override
			{
				for (auto& user : users) {
					if (user->get_email() == email && user->get_hashed_password() == hashedPassword)
						return user;
				}

				return GamespyUser();
			}

			GamespyUser get_user_from_uniquenick(const std::string& uniquenick) override
			{
				for (auto& user : users) {
					if (user->get_uniquenick() == uniquenick)
						return user;
				}

				return GamespyUser();
			}

			std::vector<GamespyProfile> get_users_from_nick(const std::string& nick) override
			{
				std::vector<GamespyProfile> users;
				for (auto& profile : profiles) {
					if (profile->get_nick() == nick)
						users.push_back(profile);
				}

				return users;
			}

			GamespyUser insert_new_user(const std::string& uniquenick, const std::string& hashedPassword, const std::string& email, const std::string& country) override
			{
				auto user = std::make_shared<User>(10000 + users.size(), uniquenick, hashedPassword, email, country);
				auto profile = std::make_shared<Profile>(2000 + profiles.size(), uniquenick);
				user->add_profile(profile);
				users.push_back(user);
				profiles.push_back(profile);
				return user;
			}

			void update_user(GamespyUser user) override
			{
				
			}
		};
	}
}
