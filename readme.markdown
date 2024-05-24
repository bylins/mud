# BRus MUD Engine readme.

Для сборки под Ubuntu 20.04 или WSL (ubuntu под WIN10 установка https://docs.microsoft.com/ru-ru/windows/wsl/install) требуется ввести:

sudo apt update && sudo apt upgrade

sudo apt install build-essential make libssl-dev libghc-zlib-dev libcurl4-gnutls-dev libexpat1-dev gettext unzip cmake

git clone --recurse-submodules https://github.com/bylins/mud

cd mud

cp -n -r lib.template/* lib

mkdir build

cd build

cmake -DBUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Test ..

make -j2 (2 это количество ядер в компьютере)

cd ..

build/circle 4000

подключение из клиента #conn localhost 4000

