Some words first: This project was thought as a remake of Luigi Auriemma's Gamespy Login Emulator. It is usefull especially for Battlefield 2 Mod teams and for people who have a BF2 statistic server running but have no possibility to connect the the internet (so no unlocks without an login emulator). My main goal was to add database support to Luigi's emulator which then will allow any login password (Lugigi's only allowed "pass" ;)) and give mod teams a option to extend Battlefield 2's extensibility.

Feature list:
  * Database
    1. MySQL
    1. SQLite3

  * One Executable that runs:
    1. gpcm.gamespy.com
    1. gpsp.gamespy.com
    1. battlefield2.available.gamespy.com

  * Full Battlefield 2 (since v. 1.1a also BF2 1.5) Interface support:
    1. Login with any valid password
    1. Create new Account
    1. Recieve Account by e-mail or name

Planned features:
  * PHP Admin Interface
    1. kick users (kicks user back to the login menu)
    1. delete users
  * Create Account via website with a little PHP script
  * Python Plugin support
  * Makefile to let the program run on Linux and OS X