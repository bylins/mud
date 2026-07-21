# BRus MUD Engine

[![Multi-Platform Build](https://github.com/bylins/mud/actions/workflows/build.yml/badge.svg)](https://github.com/bylins/mud/actions/workflows/build.yml)
[![Quick Check](https://github.com/bylins/mud/actions/workflows/quick-check.yml/badge.svg)](https://github.com/bylins/mud/actions/workflows/quick-check.yml)
[![codecov](https://codecov.io/gh/bylins/mud/branch/master/graph/badge.svg)](https://codecov.io/gh/bylins/mud)

Проект собирается на Linux (Ubuntu/Debian), Windows (MSVC, Clang, MinGW, Cygwin, WSL) и macOS с помощью [Meson](https://mesonbuild.com/).

---

## Содержание

 * [Подготовка](#подготовка)
 * [Сборка](#сборка)
  * [Linux / WSL](#linux--wsl)
  * [macOS](#macos)
  * [Windows (MSVC / Clang)](#windows-msvc--clang)
  * [Windows (MinGW / MSYS2)](#windows-mingw--msys2)
  * [Cross-компиляция под Windows с Linux (MinGW)](#cross-компиляция-под-windows-с-linux-mingw)
  * [LuaJIT и форматирование Lua](#luajit-и-форматирование-lua)
 * [Unity-сборка](#unity-сборка)
 * [Пересборка](#пересборка)
 * [Тесты](#тесты)
  * [Покрытие кода](#покрытие-кода)
 * [Docker](#docker)
 * [Опции сборки](#опции-сборки)
  * [Профиль сборки](#профиль-сборки)
  * [Функциональность](#функциональность)
  * [Зависимости](#зависимости)

---

## Подготовка

Клонируйте репозиторий:

```bash
git clone https://github.com/bylins/mud
cd mud
```

Скопируйте шаблон данных мира:

```bash
cp --update=none -r lib.template/* lib
```

---

## Сборка

### Linux / WSL

Установите зависимости (Ubuntu 24.04):

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential make \
  libssl-dev libcurl4-gnutls-dev libexpat1-dev \
  gettext unzip gdb \
  libgtest-dev zlib1g-dev libcrypt-dev \
  ninja-build meson pkg-config
```

Сборка:

```bash
meson setup build -Dbuild_profile=dev
meson compile -C build
build/circle 4000
```

WSL (Ubuntu под Windows) настраивается по [официальной инструкции Microsoft](https://docs.microsoft.com/ru-ru/windows/wsl/install), после чего шаги идентичны Linux.

### macOS

```bash
brew install openssl curl expat googletest meson ninja pkgconf
meson setup build -Dbuild_profile=dev
meson compile -C build
build/circle 4000
```

Сборка для Mac на процессорах Intel:

```bash
meson setup build --native-file=toolchains/macos-x86_64-native.txt -Dbuild_profile=dev
meson compile -C build
```

Создание universal binary для двух архитектур одновременно:

```bash
meson setup build --native-file=toolchains/macos-universal-native.txt -Dbuild_profile=dev
meson compile -C build
```

### Windows (MSVC / Clang)

Установите зависимости через vcpkg:

```powershell
vcpkg install openssl:x64-windows curl:x64-windows expat:x64-windows gtest:x64-windows
pip install meson ninja
```

Откройте Developer Command Prompt (или используйте `ilammy/msvc-dev-cmd` в CI), затем:

```powershell
$env:PKG_CONFIG_PATH = "C:\vcpkg\installed\x64-windows\lib\pkgconfig"
meson setup build -Dbuild_profile=dev
meson compile -C build
```

Для Clang передайте переменные окружения перед `meson setup`:

```powershell
$env:CC = "clang-cl"; $env:CXX = "clang-cl"
```

### Windows (MinGW / MSYS2)

В терминале MSYS2 (MINGW64):

```bash
pacman -S git mingw-w64-x86_64-gcc mingw-w64-x86_64-meson mingw-w64-x86_64-ninja \
  mingw-w64-x86_64-pkg-config mingw-w64-x86_64-openssl \
  mingw-w64-x86_64-curl mingw-w64-x86_64-expat mingw-w64-x86_64-gtest

meson setup build -Dbuild_profile=dev
meson compile -C build
```

### Cross-компиляция под Windows с Linux (MinGW)

В репозитории есть готовый toolchain-файл:

```bash
meson setup build --cross-file toolchains/windows-mingw64-cross.txt -Dbuild_profile=dev
meson compile -C build
```

### LuaJIT и форматирование Lua

Для сборки движка с Lua-триггерами и встроенным форматтером EmmyLuaCodeStyle используйте встроенные Meson-зависимости:

```bash
meson setup build_lua \
  -Dbuild_profile=release \
  -Dbuild_tests=false \
  -Dluajit=builtin \
  -Dlua_formatter=true
meson compile -C build_lua
```

`-Dluajit=builtin` включает LuaJIT и интеграцию через sol2. При включенном LuaJIT опция `lua_formatter` по умолчанию равна `true`; она указана в примере явно, чтобы конфигурация была очевидна. Meson использует subprojects из репозитория или загружает недостающие исходники через wrap-файлы.

Если LuaJIT установлен в системе и доступен через `pkg-config`, вместо встроенной версии можно использовать:

```bash
meson setup build_lua \
  -Dbuild_profile=release \
  -Dbuild_tests=false \
  -Dluajit=system \
  -Dlua_formatter=true
```

Чтобы оставить Lua-триггеры, но отключить форматтер:

```bash
meson configure build_lua -Dlua_formatter=false
meson compile -C build_lua
```

В итоговой сводке конфигурации Meson строки `LuaJIT` и `Lua formatter` показывают фактически выбранные режимы. Форматтер доступен в OLC только в сборке с LuaJIT.

---

## Unity-сборка

Meson поддерживает unity-сборку (объединение нескольких `.cpp` в один compilation unit), что существенно ускоряет компиляцию за счёт параллельной обработки:

```bash
# Ускорение примерно в 2x по сравнению с обычной сборкой
meson setup build --unity=on

# по умолчанию эта опция включена, поэтому допустимо
meson setup build

# Ускорение в 4-5x — подберите unity_size под число ядер вашего процессора
meson setup build -Dunity_size=45
```

Рекомендуемые значения `unity_size`: 30–50. Чем больше ядер — тем большее значение имеет смысл пробовать. Флаг можно передать при первом `meson setup` или сбросить конфигурацию через `meson setup build --wipe`.

---

## Пересборка
Если не требуется менять конфигурацию сборки, пересборка осуществляется так:

```bash
meson compile -C build

# также допустимо, если бэкендом выбран ninja
ninja -C build
```

Пересборка с изменением параметров осуществляется так:

```bash
# первая сборка
meson setup build -Dyaml=system -Dunity_size=45

# теперь yaml соберется из кода, unity_size по-прежнему 45
meson setup --reconfigure build -Dyaml=builtin

# сброс всех ранее примененных настроек
meson setup --wipe build
```

---

## Тесты

```bash
meson setup build -Dbuild_profile=dev
meson compile -C build tests
meson test -C build
```

Запуск конкретного теста напрямую:

```bash
build/tests/tests --gtest_filter="TriggersList_F.*"
```

### Покрытие кода

```bash
# -Db_coverage=true обязательно (встроенная опция meson для отчётов о покрытии)
meson setup build -Dbuild_profile=debug -Db_coverage=true
meson compile -C build tests
meson test -C build

# HTML-отчёт (требует lcov/gcovr)
# для MacOS работает только gcovr
ninja -C build coverage-html

# Или другие форматы:
ninja -C build coverage       # отчёты во всех доступных форматах + сводка в терминале
ninja -C build coverage-xml   # для CI / Codecov
```

---

## Docker

```bash
docker build -t mud-server --build-arg BUILD_TYPE=Test .
docker run -d -p 4000:4000 -e MUD_PORT=4000 -v ./lib:/mud/lib --name mud mud-server
```

Остановка:

```bash
docker stop mud
```

Подключение из клиента: `localhost:4000`

---

## Опции сборки

Опции передаются при вызове `meson setup` в виде `-Dимя=значение`.

### Профиль сборки

| Опция | Значения | По умолчанию | Описание |
|---|---|---|---|
| `build_profile` | `release`, `debug`, `dev`, `fasttest`, `custom` | `release` | `release`/`debug` — ggdb3 O0; `dev` — O3 + TEST\_BUILD; `fasttest` — Ofast + TEST\_BUILD; `custom` — без специфических флагов |

### Функциональность

| Опция | Тип | По умолчанию | Описание |
|---|---|---|---|
| `build_tests` | boolean | `true` | Собирать тесты |
| `scripting` | boolean | `false` | Python-скриптинг через Boost.Python |
| `admin_api` | boolean | `false` | Admin API и JSON-обработка |
| `has_epoll` | boolean | `true` | Использовать epoll (Linux/FreeBSD) |
| `nocrypt` | boolean | `false` | Отключить использование `crypt()` |
| `with_asan` | boolean | `false` | Address Sanitizer |
| `use_pch` | boolean | `true` | Предкомпилированные заголовки |
| `lua_formatter` | boolean | `true` | Встроенный EmmyLuaCodeStyle; используется только при включенном LuaJIT |
| `linker` | string | `` (системный) | Линковщик: `gold`, `mold`, `lld`, `bfd` |
| `full_world_path` | string | `` | Абсолютный путь к данным мира для создания симлинков |
| `sol2_root` | string | `` | Необязательный путь к заголовкам sol2; пустое значение использует Meson dependency/fallback |

### Зависимости

Для некоторых возможностей движка требуются внешние зависимости. Большинство из них возможно как собирать из исходного кода, так и использовать установленные в системе. Синтаксис такой:

```bash
meson setup build -Ddependency1=builtin -Ddependency2=system -Ddependency3=disabled -Ddependency4=auto
```

Для каждой зависимости доступны значения: `auto` (сначала искать системную, при отсутствии собрать встроенную), `system` (использовать зависимость из системы или ошибка), `builtin` (сразу собрать встроенную), `disabled`.
Если зависимость собирается из исходного кода впервые, Meson автоматически скачает её.

| Опция | По умолчанию | Описание |
|---|---|---|
| `zlib` | `auto` | Поддержка MCCP через ZLib |
| `iconv` | `disabled` | Поддержка iconv |
| `telegram` | `disabled` | Telegram-интеграция (требует CURL + OpenSSL) |
| `luajit` | `disabled` | Lua-триггеры через LuaJIT и sol2; `builtin` собирает встроенные зависимости |
| `boost` | `disabled` | Boost (нужен для scripting) |
| `sqlite` | `disabled` | SQLite как источник данных мира |
| `yaml` | `disabled` | YAML как источник данных мира |
| `otel` | `disabled` | OpenTelemetry (трассировка и метрики) |
| `fmt` | `builtin` | Библиотека fmt (обязательная зависимость) |
| `gtest` | `builtin` | Google Test (используется при `build_tests=true`) |

Стандартные опции Meson также доступны, например `-Db_coverage=true` для покрытия кода.
