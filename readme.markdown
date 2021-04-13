# BRus MUD Engine readme.
Для сборки под Ubuntu 20.04 или WSL (ubuntu под WIN10) требуется ввести:

sudo apt update && sudo apt upgrade

sudo apt install build-essential make libssl-dev libghc-zlib-dev libcurl4-gnutls-dev libexpat1-dev gettext unzip libboost-all-dev cmake

git clone https://github.com/bylins/mud

cd mud

mkdir build

cd build

cmake -DSCRIPTING=NO -DCMAKE_BUILD_TYPE=Test -DBUILD_TESTS=NO ..

make

если используете clion следуйте этой статье: 
https://blog.jetbrains.com/clion/2018/01/clion-and-linux-toolchain-on-windows-are-now-friends/

