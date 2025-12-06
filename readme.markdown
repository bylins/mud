Prool comments see in file readme.prool.
10 Aug 2025 - bug found!

----

# BRus MUD Engine readme.

Для сборки под Ubuntu 24.04 или WSL (ubuntu под WIN10 установка https://docs.microsoft.com/ru-ru/windows/wsl/install) требуется ввести:

## Подготовка
```bash
sudo apt update && sudo apt upgrade
sudo apt install build-essential make libssl-dev libcurl4-gnutls-dev libexpat1-dev gettext unzip cmake gdb libgtest-dev

git clone --recurse-submodules https://github.com/bylins/mud
cd mud
cp --update=none -r lib.template/* lib
```

## Нативная сборка
```bash
mkdir build
cd build
cmake -DBUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Test ..
make -j2
#2 это количество ядер в компьютере

cd ..
build/circle 4000
```

## Сборка с тестами
```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Test ..
make tests -j2

# Запуск тестов
./tests/tests
```

## Запуск в docker
``` bash
docker build -t mud-server --build-arg BUILD_TYPE=Test .
docker run -d -p 4000:4000 -e MUD_PORT=4000 -v ./lib:/mud/lib --name mud mud-server
```
Для остановки сервера:
```bash
docker stop mud
```


подключение из клиента #conn localhost 4000
