# Документация Admin API

## Обзор

Admin API предоставляет JSON-интерфейс через Unix Domain Socket для онлайн-редактирования данных мира (зоны, мобы, объекты, комнаты, триггеры). Служит программным фронтендом к системе OLC (OnLine Creation).

**Ключевые возможности:**
- Коммуникация через Unix Domain Socket (только локальный доступ)
- Протокол запрос/ответ на основе JSON
- Аутентификация через учетные данные игрового аккаунта (иммортале/строители)
- Автоматическая конвертация UTF-8 ↔ KOI8-R
- Ограничение на одно одновременное подключение
- Прямая интеграция с внутренними методами OLC

**Архитектура:**
```
Клиент (Python/curl/netcat) → Unix Socket → Admin API → Методы OLC → Данные мира
                              JSON/UTF-8              medit_save_internally()
                                                      oedit_save_internally()
                                                      redit_save_internally()
```

## Подключение

**Путь к сокету:** Настраивается в `lib/misc/configuration.xml`:
```xml
<admin_api>
    <socket_path>admin_api.sock</socket_path>
    <enabled>true</enabled>
</admin_api>
```

Расположение по умолчанию: `admin_api.sock` (относительно игровой директории)

**Протокол:**
- JSON с разделением строками (каждый запрос/ответ заканчивается `\n`)
- Кодировка UTF-8 (автоматическая конвертация в/из KOI8-R)
- Одно постоянное соединение (рекомендуется переиспользование)

**Ограничение подключений:** Максимум 1 одновременное admin-подключение

## Аутентификация

**Все операции API (кроме `ping`) требуют аутентификации.**

### Команда: `auth`

Аутентификация с использованием учетных данных игрового аккаунта (только иммортали и строители).

**Запрос:**
```json
{
  "command": "auth",
  "username": "ВашеИмя",
  "password": "ваш_пароль"
}
```

**Ответ (успех):**
```json
{
  "status": "ok",
  "message": "Authentication successful"
}
```

**Ответ (ошибка):**
```json
{
  "status": "error",
  "error": "Authentication failed"
}
```

**Требования к доступу:**
- Аккаунт должен существовать в игровых файлах
- Минимальный уровень: Строитель (kLvlBuilder)
- И иммортали, и строители имеют полный доступ к API

## Зоны

### Команда: `list_zones`

Получить список всех зон.

**Запрос:**
```json
{
  "command": "list_zones"
}
```

**Ответ:**
```json
{
  "status": "ok",
  "zones": [
    {
      "vnum": 1,
      "name": "Тестовая Зона",
      "level": 1,
      "age": 15,
      "lifespan": 20,
      "reset_mode": 2
    }
  ]
}
```

**Поля:**
- `vnum` (int) - Виртуальный номер зоны
- `name` (string) - Название зоны
- `level` (int) - Рекомендуемый уровень
- `age` (int) - Текущий возраст в пульсах
- `lifespan` (int) - Интервал сброса в пульсах
- `reset_mode` (int) - Поведение сброса (0-3)

### Команда: `get_zone`

Получить детальную информацию о зоне.

**Запрос:**
```json
{
  "command": "get_zone",
  "vnum": 1
}
```

**Ответ:**
```json
{
  "status": "ok",
  "zone": {
    "vnum": 1,
    "name": "Тестовая Зона",
    "level": 1,
    "type": 0,
    "locked": false,
    "reset_idle": false,
    "age": 15,
    "lifespan": 20,
    "top": 199,
    "reset_mode": 2,
    "group": 1
  }
}
```

**Дополнительные поля:**
- `type` (int) - Тип зоны (0=обычная, и т.д.)
- `locked` (bool) - Зона заблокирована для редактирования
- `reset_idle` (bool) - Сброс только когда нет игроков
- `top` (int) - Наивысший vnum в зоне
- `group` (int) - ID группы зоны

### Команда: `update_zone`

Обновить свойства зоны.

**Запрос:**
```json
{
  "command": "update_zone",
  "vnum": 1,
  "data": {
    "name": "Обновленное Имя Зоны",
    "level": 5,
    "lifespan": 30,
    "reset_mode": 1
  }
}
```

**Ответ:**
```json
{
  "status": "ok",
  "message": "Zone updated successfully"
}
```

**Обновляемые поля:**
- `name` (string) - Название
- `level` (int) - Уровень
- `type` (int) - Тип
- `locked` (bool) - Заблокирована
- `reset_idle` (bool) - Сброс в idle
- `lifespan` (int) - Время жизни
- `reset_mode` (int) - Режим сброса
- `group` (int) - Группа

## Мобы

### Команда: `list_mobs`

Список всех мобов в зоне.

**Запрос:**
```json
{
  "command": "list_mobs",
  "zone": "1"
}
```

**Ответ:**
```json
{
  "status": "ok",
  "mobs": [
    {
      "vnum": 100,
      "name": "тестовый моб",
      "short_desc": "Тестовый моб стоит здесь.",
      "level": 1
    }
  ]
}
```

### Команда: `get_mob`

Получить детальные данные моба.

**Запрос:**
```json
{
  "command": "get_mob",
  "vnum": 100
}
```

**Ответ:**
```json
{
  "status": "ok",
  "mob": {
    "vnum": 100,
    "names": {
      "aliases": "тестовый моб",
      "nominative": "тестовый моб",
      "genitive": "тестового моба",
      "dative": "тестовому мобу",
      "accusative": "тестового моба",
      "instrumental": "тестовым мобом",
      "prepositional": "тестовом мобе"
    },
    "descriptions": {
      "short_desc": "Тестовый моб стоит здесь.",
      "long_desc": "Это длинное описание моба.\n"
    },
    "stats": {
      "level": 1,
      "sex": 0,
      "race": 0,
      "alignment": 0,
      "hitroll_penalty": 0,
      "armor": 100,
      "hp": {
        "dice_count": 1,
        "dice_size": 10,
        "bonus": 0
      },
      "damage": {
        "dice_count": 1,
        "dice_size": 4,
        "bonus": 0
      }
    },
    "abilities": {
      "strength": 11,
      "dexterity": 11,
      "constitution": 11,
      "intelligence": 11,
      "wisdom": 11,
      "charisma": 11
    },
    "resistances": {
      "fire": 0,
      "air": 0,
      "water": 0,
      "earth": 0,
      "vitality": 0,
      "mind": 0,
      "immunity": 0
    },
    "savings": {
      "will": 0,
      "stability": 0,
      "reflex": 0
    },
    "position": {
      "default_position": 8,
      "load_position": 8
    },
    "behavior": {
      "class": 0,
      "special": 0,
      "attack_type": 0,
      "exp_bonus": 0,
      "gold": {
        "dice_count": 0,
        "dice_size": 0,
        "bonus": 0
      },
      "helpers": []
    },
    "flags": {
      "mob_flags": ["!SLEEP", "!CHARM"],
      "affect_flags": [],
      "npc_flags": []
    },
    "skills": {},
    "features": {},
    "spells": {},
    "triggers": [100, 101]
  }
}
```

### Команда: `update_mob`

Обновить поля моба.

**Запрос:**
```json
{
  "command": "update_mob",
  "vnum": 100,
  "data": {
    "names": {
      "aliases": "измененный моб"
    },
    "stats": {
      "level": 5
    }
  }
}
```

**Ответ:**
```json
{
  "status": "ok",
  "message": "Mob updated successfully"
}
```

**Обновляемые группы полей:**

**names:** Все падежные формы русского языка
- `aliases` (string) - Ключевые слова для таргетинга
- `nominative`, `genitive`, `dative`, `accusative`, `instrumental`, `prepositional` (string)

**descriptions:**
- `short_desc` (string) - Описание в комнате
- `long_desc` (string) - Описание при осмотре

**stats:**
- `level` (int) - Уровень моба
- `sex` (int) - 0=средний, 1=мужской, 2=женский, 3=множественное
- `race` (int) - ID расы
- `alignment` (int) - От -1000 до 1000
- `hitroll_penalty` (int) - Модификатор атаки
- `armor` (int) - Значение AC
- `hp` (object) - `{dice_count, dice_size, bonus}`
- `damage` (object) - `{dice_count, dice_size, bonus}`

**abilities:** Атрибуты (3-50)
- `strength`, `dexterity`, `constitution`, `intelligence`, `wisdom`, `charisma` (int)

**resistances:** Сопротивления стихиям (-200 до 200)
- `fire`, `air`, `water`, `earth`, `vitality`, `mind`, `immunity` (int)

**savings:** Модификаторы спасбросков
- `will`, `stability`, `reflex` (int)

**position:**
- `default_position` (int) - Стойка по умолчанию (0-8)
- `load_position` (int) - Стойка при появлении (0-8)

**behavior:**
- `class` (int) - ID класса NPC
- `special` (int) - ID спец-процедуры
- `attack_type` (int) - Тип сообщения атаки
- `exp_bonus` (int) - Модификатор опыта (%)
- `gold` (object) - `{dice_count, dice_size, bonus}`
- `helpers` (array) - Vnum'ы мобов-помощников

**flags:**
- `mob_flags` (array of strings) - Флаги MOB_*
- `affect_flags` (array of strings) - Флаги AFF_*
- `npc_flags` (array of strings) - Флаги NPC_*

**skills:** Умения
- Формат: `{"skill_name": level}` (например, `{"backstab": 50}`)

**features:** Способности
- Формат: `{"feature_name": true}` (например, `{"dodge": true}`)

**spells:** Известные заклинания
- Формат: `{"spell_name": true}` (например, `{"fireball": true}`)

**triggers:** DG Script триггеры
- Массив vnum'ов триггеров (например, `[100, 101]`)

### Команда: `create_mob`

Создать нового моба в зоне.

**Запрос:**
```json
{
  "command": "create_mob",
  "zone": 1,
  "data": {
    "names": {
      "aliases": "новый моб"
    },
    "stats": {
      "level": 1
    }
  }
}
```

**Ответ:**
```json
{
  "status": "ok",
  "message": "Mob created successfully",
  "vnum": 150
}
```

**Примечания:**
- Автоматическое назначение vnum в диапазоне зоны
- В зоне должны быть доступные vnum'ы (максимум 100 на зону)
- Возвращается назначенный vnum

### Команда: `delete_mob`

**НЕ РЕАЛИЗОВАНО** - Удаление требует сложной проверки ссылок (живые экземпляры, команды зон, триггеры).

## Объекты

### Команда: `list_objects`

Список всех объектов в зоне.

**Запрос:**
```json
{
  "command": "list_objects",
  "zone": "1"
}
```

**Ответ:**
```json
{
  "status": "ok",
  "objects": [
    {
      "vnum": 100,
      "name": "тестовый объект",
      "short_desc": "тестовый предмет",
      "type": 1
    }
  ]
}
```

### Команда: `get_object`

Получить детальные данные объекта.

**Запрос:**
```json
{
  "command": "get_object",
  "vnum": 100
}
```

**Ответ:**
```json
{
  "status": "ok",
  "object": {
    "vnum": 100,
    "names": {
      "aliases": "тестовый объект",
      "nominative": "тестовый предмет",
      "genitive": "тестового предмета",
      "dative": "тестовому предмету",
      "accusative": "тестовый предмет",
      "instrumental": "тестовым предметом",
      "prepositional": "тестовом предмете"
    },
    "descriptions": {
      "short_desc": "тестовый предмет",
      "long_desc": "Тестовый предмет лежит здесь.",
      "action_desc": ""
    },
    "stats": {
      "type": 1,
      "wear_flags": ["TAKE", "HOLD"],
      "extra_flags": [],
      "no_flags": [],
      "anti_flags": [],
      "weight": 1,
      "cost": 100,
      "rent": 10,
      "minimum_level": 0,
      "sex": 0,
      "maximum_durability": 100,
      "current_durability": 100,
      "material": 0,
      "timer": 0,
      "quantity": 1
    },
    "type_specific": {
      "value0": 0,
      "value1": 0,
      "value2": 0,
      "value3": 0
    },
    "affects": [],
    "extra_descriptions": [],
    "skills": {},
    "triggers": []
  }
}
```

### Команда: `update_object`

Обновить поля объекта.

**Запрос:**
```json
{
  "command": "update_object",
  "vnum": 100,
  "data": {
    "stats": {
      "weight": 5,
      "cost": 500
    }
  }
}
```

**Ответ:**
```json
{
  "status": "ok",
  "message": "Object updated successfully"
}
```

**Обновляемые группы полей:**

**names:** Все падежные формы русского языка
- `aliases`, `nominative`, `genitive`, `dative`, `accusative`, `instrumental`, `prepositional` (string)

**descriptions:**
- `short_desc` (string) - Название в инвентаре
- `long_desc` (string) - Описание на земле
- `action_desc` (string) - Описание при надетом состоянии

**stats:**
- `type` (int) - Тип объекта (1=свет, 2=свиток, и т.д.)
- `wear_flags` (array of strings) - Флаги ITEM_WEAR_*
- `extra_flags` (array of strings) - Флаги ITEM_*
- `no_flags` (array of strings) - Флаги ITEM_NO_*
- `anti_flags` (array of strings) - Флаги ITEM_ANTI_*
- `weight` (int) - Вес в условных единицах
- `cost` (int) - Цена в магазине
- `rent` (int) - Стоимость ренты
- `minimum_level` (int) - Ограничение по уровню
- `sex` (int) - Грамматический род
- `maximum_durability` (int) - Макс. прочность
- `current_durability` (int) - Текущая прочность
- `material` (int) - ID материала
- `timer` (int) - Таймер распада
- `quantity` (int) - Размер стака

**type_specific:** Значения, зависящие от типа объекта
- `value0`, `value1`, `value2`, `value3` (int)
- Значение варьируется в зависимости от типа объекта (например, для оружия: умение, dice_count, dice_size)

**affects:** Применяемые модификаторы характеристик
- Массив пар `{location: int, modifier: int}`
- Пример: `[{"location": 1, "modifier": 5}]` (+5 к силе)

**extra_descriptions:** Дополнительные тексты для осмотра
- Массив пар `{keywords: string, description: string}`

**skills:** Бонусы к умениям
- Формат: `{"skill_name": bonus}` (например, `{"riding": 10}`)

**triggers:** DG Script триггеры
- Массив vnum'ов триггеров

### Команда: `create_object`

Создать новый объект в зоне.

**Запрос:**
```json
{
  "command": "create_object",
  "zone": 3,
  "data": {
    "names": {
      "aliases": "новый предмет"
    },
    "stats": {
      "type": 1
    }
  }
}
```

**Ответ:**
```json
{
  "status": "ok",
  "message": "Object created successfully",
  "vnum": 390
}
```

### Команда: `delete_object`

**НЕ РЕАЛИЗОВАНО** - По тем же причинам, что и delete_mob.

## Комнаты

### Команда: `list_rooms`

Список всех комнат в зоне.

**Запрос:**
```json
{
  "command": "list_rooms",
  "zone": "1"
}
```

**Ответ:**
```json
{
  "status": "ok",
  "rooms": [
    {
      "vnum": 100,
      "name": "Тестовая Комната"
    }
  ]
}
```

### Команда: `get_room`

Получить детальные данные комнаты.

**Запрос:**
```json
{
  "command": "get_room",
  "vnum": 100
}
```

**Ответ:**
```json
{
  "status": "ok",
  "room": {
    "vnum": 100,
    "name": "Тестовая Комната",
    "description": "Это тестовая комната.\n",
    "sector": 0,
    "room_flags": ["INDOORS"],
    "exits": [
      {
        "direction": 0,
        "description": "",
        "keywords": "",
        "exit_info": 0,
        "key": -1,
        "to_room": 101
      }
    ],
    "extra_descriptions": [],
    "triggers": [100, 198]
  }
}
```

### Команда: `update_room`

Обновить поля комнаты.

**Запрос:**
```json
{
  "command": "update_room",
  "vnum": 100,
  "data": {
    "name": "Измененная Комната",
    "sector": 2,
    "triggers": [100]
  }
}
```

**Ответ:**
```json
{
  "status": "ok",
  "message": "Room updated successfully"
}
```

**Обновляемые поля:**

- `name` (string) - Название комнаты
- `description` (string) - Описание комнаты (должно заканчиваться `\n`)
- `sector` (int) - Тип местности (0=внутри, 1=город, 2=поле, и т.д.)
- `room_flags` (array of strings) - Флаги ROOM_*
- `extra_descriptions` (array) - `[{keywords: string, description: string}]`
- `triggers` (array of int) - Vnum'ы мировых триггеров

**exits:** Массив объектов выходов
- `direction` (int) - 0=север, 1=восток, 2=юг, 3=запад, 4=вверх, 5=вниз
- `description` (string) - Текст при осмотре выхода
- `keywords` (string) - Ключевые слова двери
- `exit_info` (int) - Флаги выхода (0=открыто, 1=дверь, 2=заперто, и т.д.)
- `key` (int) - Vnum объекта-ключа (-1=нет ключа)
- `to_room` (int) - Vnum комнаты назначения

### Команда: `create_room`

**НЕ РЕАЛИЗОВАНО** - Запланировано на будущее.

### Команда: `delete_room`

**НЕ РЕАЛИЗОВАНО** - По тем же причинам, что и delete_mob.

## Триггеры

### Команда: `list_triggers`

Список всех триггеров в зоне.

**Запрос:**
```json
{
  "command": "list_triggers",
  "zone": "1"
}
```

**Ответ:**
```json
{
  "status": "ok",
  "triggers": [
    {
      "vnum": 100,
      "name": "Тестовый Триггер",
      "type": "GREET"
    }
  ]
}
```

### Команда: `get_trigger`

Получить детальные данные триггера.

**Запрос:**
```json
{
  "command": "get_trigger",
  "vnum": 100
}
```

**Ответ:**
```json
{
  "status": "ok",
  "trigger": {
    "vnum": 100,
    "name": "Тестовый Триггер",
    "attach_type": 2,
    "trigger_type": "GREET",
    "narg": 100,
    "argument": "",
    "commands": "say Привет, %actor.name%!\n"
  }
}
```

### Команда: `update_trigger`

Обновить скрипт триггера.

**Запрос:**
```json
{
  "command": "update_trigger",
  "vnum": 100,
  "data": {
    "name": "Измененный Триггер",
    "commands": "say Обновленное приветствие!\n"
  }
}
```

**Ответ:**
```json
{
  "status": "ok",
  "message": "Trigger updated successfully"
}
```

**Обновляемые поля:**

- `name` (string) - Название триггера
- `attach_type` (int) - 0=моб, 1=объект, 2=комната
- `trigger_type` (string) - Тип триггера ("GREET", "COMMAND", "RANDOM", и т.д.)
- `narg` (int) - Числовой аргумент (варьируется в зависимости от типа)
- `argument` (string) - Текстовый аргумент (варьируется в зависимости от типа)
- `commands` (string) - Код DG Script

**Распространенные типы триггеров:**
- MOB: `GREET`, `COMMAND`, `SPEECH`, `FIGHT`, `HITPRCNT`, `DEATH`, `ENTRY`, `RECEIVE`, `BRIBE`, `RANDOM`
- OBJECT: `COMMAND`, `TIMER`, `GET`, `DROP`, `GIVE`, `WEAR`, `REMOVE`, `RANDOM`
- ROOM: `COMMAND`, `SPEECH`, `ENTER`, `DROP`, `RANDOM`

### Команда: `create_trigger`

**НЕ РЕАЛИЗОВАНО** - Запланировано на будущее.

### Команда: `delete_trigger`

**НЕ РЕАЛИЗОВАНО** - По тем же причинам, что и delete_mob.

## Обработка ошибок

Все ошибки возвращают:
```json
{
  "status": "error",
  "error": "Описание сообщения об ошибке"
}
```

**Распространенные ошибки:**

- `"Not authenticated. Use 'auth' command first."` - Отсутствует аутентификация
- `"Authentication failed"` - Неверные учетные данные или недостаточные привилегии
- `"Zone not found"` - Неверный vnum зоны
- `"Mob not found"` - Неверный vnum моба
- `"Object not found"` - Неверный vnum объекта
- `"Room not found"` - Неверный vnum комнаты
- `"Trigger not found"` - Неверный vnum триггера
- `"Max admin connections reached"` - Подключен другой администратор
- `"No available vnums in zone"` - Диапазон vnum зоны исчерпан (использовано 100/100)
- `"JSON parse error: ..."` - Неправильно сформированный запрос
- `"Unknown command"` - Неверное имя команды

## Примеры использования

### Клиент на Python

```python
#!/usr/bin/env python3
import socket
import json

# Подключение к сокету
sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
sock.connect('lib/admin_api.sock')

def send_command(cmd):
    """Отправить JSON команду и получить ответ."""
    sock.sendall((json.dumps(cmd) + '\n').encode('utf-8'))
    response = b''
    while True:
        chunk = sock.recv(4096)
        response += chunk
        if b'\n' in response:
            line = response.split(b'\n', 1)[0]
            return json.loads(line.decode('utf-8'))

# Аутентификация
print(send_command({
    'command': 'auth',
    'username': 'ВашеИмя',
    'password': 'ваш_пароль'
}))

# Список мобов в зоне 1
print(send_command({
    'command': 'list_mobs',
    'zone': '1'
}))

# Получить детали моба
mob_data = send_command({
    'command': 'get_mob',
    'vnum': 100
})
print(f"Уровень моба: {mob_data['mob']['stats']['level']}")

# Обновить моба
print(send_command({
    'command': 'update_mob',
    'vnum': 100,
    'data': {
        'stats': {'level': 10}
    }
}))

sock.close()
```

### Использование netcat (nc)

```bash
# Одиночная команда (требуется повторная аутентификация)
echo '{"command":"auth","username":"ВашеИмя","password":"пароль"}' | nc -U lib/admin_api.sock

# Интерактивная сессия (постоянное соединение)
nc -U lib/admin_api.sock << 'EOF'
{"command":"auth","username":"ВашеИмя","password":"пароль"}
{"command":"list_mobs","zone":"1"}
{"command":"get_mob","vnum":100}
EOF

# Многострочный скрипт
(
    echo '{"command":"auth","username":"ВашеИмя","password":"пароль"}'
    sleep 0.1
    echo '{"command":"get_mob","vnum":100}'
) | nc -U lib/admin_api.sock
```

### Использование curl (через прокси socat)

```bash
# Запустить HTTP→Unix socket прокси
socat TCP-LISTEN:8888,reuseaddr,fork UNIX-CONNECT:lib/admin_api.sock &

# Использовать curl
curl -X POST http://localhost:8888 \
     -H "Content-Type: application/json" \
     -d '{"command":"ping"}'

curl -X POST http://localhost:8888 \
     -H "Content-Type: application/json" \
     -d '{"command":"auth","username":"ВашеИмя","password":"пароль"}'
```

## Ограничения

1. **Одно подключение:** Разрешено только одно одновременное admin-подключение
2. **Нет удаления:** Операции создания поддерживаются, но операции удаления не реализованы
3. **Нет создания/удаления зон:** Управление зонами ограничено обновлениями
4. **Исчерпание Vnum:** Каждая зона ограничена 100 vnum'ами (зона N: N×100 до N×100+99)
5. **Только живые данные:** API работает с данными в памяти; сервер должен сохранить для сохранения изменений
6. **Нет валидации:** Ограниченная валидация входных данных; неправильные данные могут привести к падению сервера
7. **Нет транзакций:** Изменения немедленные; нет поддержки отката
8. **Кодировка:** Весь текст должен быть в валидном UTF-8 (автоматическая конвертация в/из KOI8-R)

## Компиляция

Admin API опциональный и отключен по умолчанию.

**Включить:**
```bash
cmake -DENABLE_ADMIN_API=ON ..
make
```

**Конфигурация:**
Отредактировать `lib/misc/configuration.xml`:
```xml
<admin_api>
    <socket_path>admin_api.sock</socket_path>
    <enabled>true</enabled>
</admin_api>
```

**Проверка:**
```bash
# Проверить существование сокета
ls -la lib/admin_api.sock

# Тестовое подключение
echo '{"command":"ping"}' | nc -U lib/admin_api.sock
# Ожидается: {"status":"pong"}
```

## Безопасность

**Контроль доступа:**
- Unix Domain Socket = только локальный доступ (нет сетевой экспозиции)
- Права файла: 0600 (чтение/запись только владельцем)
- Аутентификация требуется для всех операций кроме `ping`
- Минимальный уровень привилегий: Строитель (kLvlBuilder)

**Рекомендации:**
- Используйте сильные пароли для аккаунтов строителей/иммортале
- Ограничьте доступ к файловой системе игровой директории
- Мониторьте подключения через логи сервера
- Запускайте игровой сервер под выделенным пользовательским аккаунтом
- Рассмотрите SSH-туннелирование для удаленного администрирования

## Устранение неполадок

### Отказ в подключении
```
nc: unix connect failed: Connection refused
```
**Решения:**
- Проверьте что `admin_api.enabled` установлено в `true` в configuration.xml
- Убедитесь что сервер скомпилирован с `-DENABLE_ADMIN_API=ON`
- Подтвердите что файл сокета существует: `ls -la lib/admin_api.sock`
- Проверьте логи сервера на ошибки инициализации

### Достигнуто максимум подключений
```
{"status":"error","error":"Max admin connections reached"}
```
**Решения:**
- Закройте существующее admin-подключение
- Проверьте устаревшие подключения: `lsof lib/admin_api.sock`
- Перезапустите сервер для очистки подключений

### Ошибка аутентификации
```
{"status":"error","error":"Authentication failed"}
```
**Решения:**
- Проверьте что имя пользователя/пароль правильные
- Проверьте что уровень аккаунта >= Строитель: `stat player_file`
- Убедитесь что аккаунт существует в игровых файлах
- Проверьте логи сервера для детальной ошибки

### Ошибка парсинга JSON
```
{"status":"error","error":"JSON parse error: ..."}
```
**Решения:**
- Валидируйте синтаксис JSON (используйте `jq` или онлайн-валидатор)
- Убедитесь в правильной кодировке UTF-8
- Проверьте отсутствующие кавычки, запятые, скобки
- Проверьте завершение строки с `\n`

### Моб/Объект/Комната не найдены
```
{"status":"error","error":"Mob not found"}
```
**Решения:**
- Проверьте что vnum существует: используйте `list_mobs`, `list_objects`, `list_rooms`
- Проверьте что vnum в правильном диапазоне зоны (зона N: N×100 до N×100+99)
- Убедитесь что зона загружена (проверьте `list_zones`)

### Нет доступных vnum'ов
```
{"status":"error","error":"No available vnums in zone"}
```
**Решения:**
- Выберите другую зону с свободными vnum'ами
- Удалите неиспользуемые сущности в зоне (не через API - используйте OLC в игре)
- Используйте явное назначение vnum (если поддерживается)
- Зоны ограничены 100 vnum'ами каждая (0-99)

### Команды обрезаны
**Симптом:** Команды обрываются посреди символа (особенно с кириллическим текстом)

**Решение:**
- Убедитесь что сервер собран с исправлениями process_input() для UTF-8
- Проверьте что флаг `admin_api_mode` правильно установлен на дескрипторе
- Проверьте что автоматическая конвертация UTF-8 ↔ KOI8-R включена

### Пустые ответы
**Симптом:** Поля отсутствуют или пусты в ответах get_*

**Решения:**
- Проверьте что данные прототипа существуют (используйте OLC в игре для проверки)
- Для триггеров: убедитесь что `proto_script` заполнен (а не только `script`)
- Проверьте что поле поддерживается API (см. списки полей выше)

## Смотрите также

- **Документация OLC:** Внутриигровая помощь для `MEDIT`, `OEDIT`, `REDIT`, `ZEDIT`, `TRIGEDIT`
- **DG Scripts:** `/src/engine/scripting/dg_scripts.h` для типов триггеров
- **Исходный код:** `/src/engine/network/admin_api.cpp` для деталей реализации
- **Тесты:** `/tests/test_admin_api.py` для примеров использования
