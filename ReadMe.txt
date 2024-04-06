06.04.2024: Fully rewritten using c++23 and boost::asio, with all gamespy "encryption" done with publicly available ciphers (RC4 + Sapphire II):
https://github.com/kr4uzi/gamespy
-> Only the new /gamespy repository will be maintained from this day on

17.09.2015: initial v2 release
-dependencies: openssl (md5), boost (asio)
-only featuring a "memory" database for testing, mysql and sqlite3 dbs are currently beegin implemented
-linux Makefile probably not working right now as i dont have time to test it
