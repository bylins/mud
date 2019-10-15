# BRus MUD Engine readme.
Для сборки под UNIX или WSL  (ubuntu под WIN10) требуется:

sudo apt update && sudo apt upgrade

sudo apt install build-essential make libssl-dev libghc-zlib-dev libcurl4-gnutls-dev libexpat1-dev gettext unzip libboost-all-dev cmake

wget https://github.com/git/git/archive/master.zip

unzip master.zip

cd git-master/

make prefix=/usr/local all

sudo make prefix=/usr/local install

cd ..

git clone https://github.com/bylins/mud

cd mud

mkdir build

cd build

cmake -DSCRIPTING=NO -DCMAKE_BUILD_TYPE=Test -DBUILD_TESTS=NO ..

make

если используете clion следуйте этой статье: 
https://blog.jetbrains.com/clion/2018/01/clion-and-linux-toolchain-on-windows-are-now-friends/

