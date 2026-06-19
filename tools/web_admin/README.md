# Bylins MUD Web Admin Interface

Веб-интерфейс для администрирования мира Былин через Admin API.

## Возможности

- 🗺️ **Зоны**: просмотр, редактирование
- 👹 **Мобы**: список, просмотр, создание, редактирование
- ⚔️ **Предметы**: список, просмотр, создание, редактирование
- 🏛️ **Комнаты**: список, просмотр, создание, редактирование
- ⚡ **Триггеры**: список, просмотр, создание, редактирование

## Установка

```bash
cd tools/web_admin
pip install -r requirements.txt
```

## Запуск

**ВАЖНО:** Перед запуском Flask необходимо задать путь к сокету через переменную окружения `MUD_SOCKET`.

### Шаг 1: Запустите MUD сервер

```bash
cd build_debug
./circle -d ~/repos/world.yaml 4000
```

Убедитесь, что Admin API включен в `configuration.xml`:
```xml
<admin_api>
    <enabled>true</enabled>
    <socket_path>admin_api.sock</socket_path>
    <require_auth>true</require_auth>
</admin_api>
```

### Шаг 2: Запустите Flask с указанием пути к сокету

```bash
cd tools/web_admin
MUD_SOCKET=~/repos/world.yaml/admin_api.sock python3 app.py
```

### Шаг 3: Откройте браузер

http://127.0.0.1:5000

## Запуск в Docker

Образ собирается на `python:3.12-alpine`, web-сервер крутится под gunicorn,
логи (HTTP-запросы + успехи/ошибки аутентификации) пишутся в stdout.

### Сокет в выделенном каталоге

Монтировать в контейнер нужно **каталог** с сокетом, а не сам файл сокета.
Причина: при рестарте циркуль делает `unlink()`+`bind()` и пересоздаёт сокет с
новым inode; bind-mount одного файла зафиксирует старый inode, и после рестарта
в контейнере останется «протухший» сокет. Монтирование каталога показывает
контейнеру актуальный файл сразу.

Поэтому держите сокет в отдельном каталоге. В `configuration.xml`:
```xml
<admin_api>
    <enabled>true</enabled>
    <socket_path>run/admin_api.sock</socket_path>
    <require_auth>true</require_auth>
</admin_api>
```
Путь относителен каталогу мира. Каталог `run/` циркуль создаёт автоматически
при старте (см. `init_unix_socket` в `src/engine/core/comm.cpp`).

### Права доступа

Сокет создаётся с правами `0600` (owner-only). Контейнер обязан подключаться от
того же пользователя, под которым работает циркуль, иначе будет «Connection
refused». Поэтому запускаем контейнер с `--user UID:GID` владельца сокета.

### docker run

```bash
docker build -t mud-web-admin tools/web_admin

docker run -d --name mud-web-admin \
  -p 127.0.0.1:12001:5000 \
  -v /opt/mud/lib/run:/mud/socket:ro \
  --user "$(id -u):$(id -g)" \
  mud-web-admin
# морда на http://127.0.0.1:12001

docker logs -f mud-web-admin    # запросы и ошибки аутентификации
```

### docker compose

```bash
MUD_SOCKET_DIR=/opt/mud/lib/run \
WEB_ADMIN_UID=$(id -u) WEB_ADMIN_GID=$(id -g) \
  docker compose -f tools/web_admin/docker-compose.yml up -d --build
```

### Переменные окружения контейнера

| Переменная | Назначение | По умолчанию |
|---|---|---|
| `MUD_SOCKET` | путь к сокету внутри контейнера | `/mud/socket/admin_api.sock` |
| `LOG_LEVEL` | уровень логирования (`INFO`/`DEBUG`/…) | `INFO` |
| `FLASK_SECRET_KEY` | постоянный секрет сессий (необязательно) | случайный на запуск |

## Конфигурация

### Переменная окружения MUD_SOCKET (обязательна)

Указывает путь к Unix socket Admin API:

```bash
# Для YAML мира
export MUD_SOCKET=~/repos/world.yaml/admin_api.sock

# Для тестового мира small/
export MUD_SOCKET=~/repos/mud/build_debug/small/admin_api.sock

# Запуск Flask
python3 app.py
```

**Примечание:** Переменная `MUD_SOCKET` обязательна. Flask не запустится без неё.

## Требования

- Python 3.8+
- Flask 3.0.0
- Запущенный MUD сервер с включенным Admin API

## Структура

```
web_admin/
├── app.py              # Flask приложение
├── mud_client.py       # Клиент Admin API
├── requirements.txt    # Python зависимости
├── templates/          # HTML шаблоны
│   ├── base.html
│   ├── index.html
│   ├── zones_list.html
│   └── error.html
└── static/            # Статические файлы
    └── css/
        └── style.css  # Стили (ретро 2000-х)
```

## Стиль

Интерфейс выполнен в ретро-стиле ранних 2000-х годов (как оригинальный сайт Былин):
- HTML таблицы для вёрстки
- Цветовая схема: коричневый, бежевый, золотой
- Классические шрифты (Arial, Georgia)
- Минимум JavaScript

## API

Все операции выполняются через Admin API по Unix socket:
- Подключение к `admin_api.sock`
- Протокол: JSON over newline-delimited stream
- Кодировка: UTF-8 (автоматическая конвертация в/из KOI8-R)

## Безопасность

**ВАЖНО:** Этот интерфейс предназначен только для локального использования!

- Работает только на 127.0.0.1
- Требует доступа к Unix socket (локальная файловая система)
- Не имеет защиты от CSRF (только локальное использование)
- Аутентификация отключена для разработки (`require_auth=false`)

Для продакшена:
1. Включите аутентификацию (`require_auth=true`)
2. Добавьте SSL/TLS
3. Настройте правильные права доступа к socket файлу
4. Используйте reverse proxy (nginx)
