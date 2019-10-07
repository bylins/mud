# BRus MUD Engine readme.
Для сборки под UNIX или WSL  (ubuntu под WIN10) требуется:

gcc

boost

cmake

свежий git из сорцов !!! обязательно

zlib

собирать: cmake -DSCRIPTING=NO -DCMAKE_BUILD_TYPE=Test -DBUILD_TESTS=NO

под cygwin все тоже самое

для Clion рекомендую WSL

обязательно обновите:

sudo apt update

sudo apt upgrade

и следуйте этой статье:

https://blog.jetbrains.com/clion/2018/01/clion-and-linux-toolchain-on-windows-are-now-friends/
