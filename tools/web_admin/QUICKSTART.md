# Быстрый старт Web Admin API

## Подготовка

### 1. Сборка Circle с Admin API

```bash
cd /home/kvirund/repos/mud

# Создать build директорию с Admin API
mkdir -p build_admin
cd build_admin

# Конфигурация с Admin API
cmake -DCMAKE_BUILD_TYPE=Test -DHAVE_YAML=ON -DENABLE_ADMIN_API=ON ..

# Компиляция
make -j$(nproc)
```

### 2. Подготовка world данных

```bash
cd build_admin

# Скопировать world данные (если еще не сделано)
mkdir -p small
cp -r ../lib small/
cp -r ../lib.template/* small/lib/

# Убедиться что admin_api включен в конфиге
grep -A 3 "<admin_api>" small/lib/misc/configuration.xml
# Должно быть: <enabled>true</enabled>
```

## Запуск

### 1. Запустить Circle MUD

```bash
cd build_admin
./circle -W -d small 4000
```

**Проверить создание сокета:**
```bash
ls -la small/lib/admin_api.sock
# Должен показать: srw------- (права 0600)
```

**Проверить логи:**
```bash
tail -f small/lib/syslog | grep "Admin API"
# Должно быть: "Admin API listening on Unix socket: small/lib/admin_api.sock"
```

### 2. Тестировать API напрямую (netcat)

```bash
# Терминал 2 (из корня репозитория)
cd build_admin

# Тест ping (не требует аутентификации)
echo '{"command":"ping"}' | nc -U small/lib/admin_api.sock
# Ответ: {"status":"pong"}

# Тест аутентификации + list_mobs
(
  echo '{"command":"auth","username":"Хмель","password":"kvirund12"}'
  sleep 0.1
  echo '{"command":"list_mobs","zone":"1"}'
) | nc -U small/lib/admin_api.sock
```

### 3. Запустить Web интерфейс

```bash
# Терминал 3
cd tools/web_admin

# Установить зависимости (первый раз)
pip3 install -r requirements.txt

# Запустить Flask
python3 app.py
```

**Открыть браузер:**
```
http://127.0.0.1:5000
```

**Вход:**
- Username: Хмель
- Password: kvirund12

## Структура

```
build_admin/
├── circle                    # Исполняемый файл MUD
└── small/
    └── lib/
        ├── admin_api.sock    # Unix socket (создается при запуске)
        └── misc/
            └── configuration.xml  # <admin_api><enabled>true</enabled>

tools/web_admin/
├── app.py                    # Flask сервер
├── mud_client.py             # Unix socket клиент
├── requirements.txt          # Python зависимости
├── templates/                # HTML шаблоны (Bylins стиль)
└── static/
    ├── css/style.css         # Стилизация (ранние 2000-е)
    └── js/app.js
```

## Доступные API команды

### Без аутентификации
- `ping` - Тест соединения

### С аутентификацией (имм/билдер)
- `auth` - Аутентификация
- `list_mobs` - Список мобов в зоне
- `get_mob` - Детали моба
- `update_mob` - Обновить моба
- `list_objects` - Список объектов в зоне
- `get_object` - Детали объекта
- `update_object` - Обновить объект

### Пример полного CRUD теста

```bash
cd /home/kvirund/repos/mud
python3 tests/test_admin_api.py full_crud_test
```

## Отладка

### Socket не создан
1. Проверить сборку: `ldd build_admin/circle | grep ENABLE_ADMIN_API`
2. Проверить логи: `grep "Admin API" build_admin/small/lib/syslog`
3. Проверить конфиг: `grep -A 2 admin_api build_admin/small/lib/misc/configuration.xml`

### Web сервер не подключается
1. Проверить путь в `app.py`: `SOCKET_PATH = 'build_admin/small/lib/admin_api.sock'`
2. Проверить права: `ls -la build_admin/small/lib/admin_api.sock`
3. Проверить что Circle запущен: `ps aux | grep circle`

### Ошибка аутентификации
1. Проверить что пользователь существует в player files
2. Проверить уровень: должен быть >= Builder (kLvlBuilder)
3. Проверить пароль

## Безопасность

- ✅ Unix socket (только локальный доступ)
- ✅ Права 0600 (только владелец)
- ✅ Максимум 1 админское подключение
- ✅ Аутентификация через player files
- ✅ Проверка уровня >= Builder
- ✅ Compile-time флаг (не компилируется по умолчанию)
- ✅ Runtime флаг в конфиге

## Производительность

- Нулевое влияние если `ENABLE_ADMIN_API=OFF` (compile-time удаление)
- < 0.1% влияние если включено но нет подключений
- Минимальное влияние при использовании (1 соединение, non-blocking I/O)
