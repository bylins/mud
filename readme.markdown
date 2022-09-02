# BRus MUD Engine readme.
Для сборки под Ubuntu 20.04 или WSL (ubuntu под WIN10 установка https://docs.microsoft.com/ru-ru/windows/wsl/install) требуется ввести:

sudo apt update && sudo apt upgrade

sudo apt install build-essential make libssl-dev libghc-zlib-dev libcurl4-gnutls-dev libexpat1-dev gettext unzip libboost-all-dev cmake

git clone https://github.com/bylins/mud

cd mud

mv -n lib.template/* lib

mkdir build

cd build

cmake -DSCRIPTING=NO -DCMAKE_BUILD_TYPE=Test -DBUILD_TESTS=NO ..

make -j2 (2 это количество ядер в компьютере)

cd ..

build/circle 4000

подключение из клиента #conn localhost 4000

если используете clion следуйте этой статье: 
https://blog.jetbrains.com/clion/2018/01/clion-and-linux-toolchain-on-windows-are-now-friends/

