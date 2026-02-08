# Admin API Documentation

## Overview

Admin API provides a JSON-based interface over Unix Domain Socket for online editing of world data (zones, mobs, objects, rooms, triggers). It serves as a programmatic frontend to the OLC (OnLine Creation) system.

**Key Features:**
- Unix Domain Socket communication (local access only)
- JSON request/response protocol
- Authentication via game account credentials (immortals/builders)
- UTF-8 ↔ KOI8-R automatic conversion
- Single concurrent connection limit
- Direct integration with OLC internals

**Architecture:**
```
Client (Python/curl/netcat) → Unix Socket → Admin API → OLC Methods → World Data
                             JSON/UTF-8              medit_save_internally()
                                                     oedit_save_internally()
                                                     redit_save_internally()
```

## Connection

**Socket Path:** Configured in `lib/misc/configuration.xml`:
```xml
<admin_api>
    <socket_path>admin_api.sock</socket_path>
    <enabled>true</enabled>
</admin_api>
```

Default location: `lib/admin_api.sock` (relative to game directory)

**Protocol:**
- Line-delimited JSON (each request/response ends with `\n`)
- UTF-8 encoding (automatic conversion to/from KOI8-R)
- Single persistent connection (reuse recommended)

**Connection Limit:** Maximum 1 concurrent admin connection

## Authentication

**All API operations (except `ping`) require authentication.**

### Command: `auth`

Authenticate using game account credentials (immortals and builders only).

**Request:**
```json
{
  "command": "auth",
  "username": "YourName",
  "password": "your_password"
}
```

**Response (success):**
```json
{
  "status": "ok",
  "message": "Authentication successful"
}
```

**Response (failure):**
```json
{
  "status": "error",
  "error": "Authentication failed"
}
```

**Access Requirements:**
- Account must exist in game files
- Minimum level: Builder (kLvlBuilder)
- Both immortals and builders have full API access

## Zones

### Command: `list_zones`

Get list of all zones.

**Request:**
```json
{
  "command": "list_zones"
}
```

**Response:**
```json
{
  "status": "ok",
  "zones": [
    {
      "vnum": 1,
      "name": "Test Zone",
      "level": 1,
      "age": 15,
      "lifespan": 20,
      "reset_mode": 2
    }
  ]
}
```

**Fields:**
- `vnum` (int) - Zone virtual number
- `name` (string) - Zone name
- `level` (int) - Recommended level
- `age` (int) - Current age in pulses
- `lifespan` (int) - Reset interval in pulses
- `reset_mode` (int) - Reset behavior (0-3)

### Command: `get_zone`

Get detailed zone information.

**Request:**
```json
{
  "command": "get_zone",
  "vnum": 1
}
```

**Response:**
```json
{
  "status": "ok",
  "zone": {
    "vnum": 1,
    "name": "Test Zone",
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

**Additional Fields:**
- `type` (int) - Zone type (0=normal, etc.)
- `locked` (bool) - Zone locked for editing
- `reset_idle` (bool) - Reset only when no players
- `top` (int) - Highest vnum in zone
- `group` (int) - Zone group ID

### Command: `update_zone`

Update zone properties.

**Request:**
```json
{
  "command": "update_zone",
  "vnum": 1,
  "data": {
    "name": "Updated Zone Name",
    "level": 5,
    "lifespan": 30,
    "reset_mode": 1
  }
}
```

**Response:**
```json
{
  "status": "ok",
  "message": "Zone updated successfully"
}
```

**Updatable Fields:**
- `name` (string)
- `level` (int)
- `type` (int)
- `locked` (bool)
- `reset_idle` (bool)
- `lifespan` (int)
- `reset_mode` (int)
- `group` (int)

## Mobs

### Command: `list_mobs`

List all mobs in a zone.

**Request:**
```json
{
  "command": "list_mobs",
  "zone": "1"
}
```

**Response:**
```json
{
  "status": "ok",
  "mobs": [
    {
      "vnum": 100,
      "name": "test mob",
      "short_desc": "Тестовый моб стоит здесь.",
      "level": 1
    }
  ]
}
```

### Command: `get_mob`

Get detailed mob data.

**Request:**
```json
{
  "command": "get_mob",
  "vnum": 100
}
```

**Response:**
```json
{
  "status": "ok",
  "mob": {
    "vnum": 100,
    "names": {
      "aliases": "test mob",
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

### Command: `update_mob`

Update mob fields.

**Request:**
```json
{
  "command": "update_mob",
  "vnum": 100,
  "data": {
    "names": {
      "aliases": "modified mob"
    },
    "stats": {
      "level": 5
    }
  }
}
```

**Response:**
```json
{
  "status": "ok",
  "message": "Mob updated successfully"
}
```

**Updatable Field Groups:**

**names:** All Russian case forms
- `aliases` (string) - Keywords for targeting
- `nominative`, `genitive`, `dative`, `accusative`, `instrumental`, `prepositional` (string)

**descriptions:**
- `short_desc` (string) - Room description
- `long_desc` (string) - Look/examine description

**stats:**
- `level` (int) - Mob level
- `sex` (int) - 0=neutral, 1=male, 2=female, 3=plural
- `race` (int) - Race ID
- `alignment` (int) - -1000 to 1000
- `hitroll_penalty` (int) - Attack modifier
- `armor` (int) - AC value
- `hp` (object) - `{dice_count, dice_size, bonus}`
- `damage` (object) - `{dice_count, dice_size, bonus}`

**abilities:** Attributes (3-50)
- `strength`, `dexterity`, `constitution`, `intelligence`, `wisdom`, `charisma` (int)

**resistances:** Elemental resistances (-200 to 200)
- `fire`, `air`, `water`, `earth`, `vitality`, `mind`, `immunity` (int)

**savings:** Save modifiers
- `will`, `stability`, `reflex` (int)

**position:**
- `default_position` (int) - Default stance (0-8)
- `load_position` (int) - Spawn stance (0-8)

**behavior:**
- `class` (int) - NPC class ID
- `special` (int) - Special procedure ID
- `attack_type` (int) - Attack message type
- `exp_bonus` (int) - Experience modifier (%)
- `gold` (object) - `{dice_count, dice_size, bonus}`
- `helpers` (array) - Helper mob vnums

**flags:**
- `mob_flags` (array of strings) - MOB_* flags
- `affect_flags` (array of strings) - AFF_* flags
- `npc_flags` (array of strings) - NPC_* flags

**skills:** Skill proficiencies
- Format: `{"skill_name": level}` (e.g., `{"backstab": 50}`)

**features:** Feats/abilities
- Format: `{"feature_name": true}` (e.g., `{"dodge": true}`)

**spells:** Known spells
- Format: `{"spell_name": true}` (e.g., `{"fireball": true}`)

**triggers:** DG Script triggers
- Array of trigger vnums (e.g., `[100, 101]`)

### Command: `create_mob`

Create new mob in zone.

**Request:**
```json
{
  "command": "create_mob",
  "zone": 1,
  "data": {
    "names": {
      "aliases": "new mob"
    },
    "stats": {
      "level": 1
    }
  }
}
```

**Response:**
```json
{
  "status": "ok",
  "message": "Mob created successfully",
  "vnum": 150
}
```

**Notes:**
- Automatic vnum assignment within zone range
- Zone must have available vnums (max 100 per zone)
- Returns assigned vnum

### Command: `delete_mob`

**NOT IMPLEMENTED** - Deletion requires complex reference checking (live instances, zone commands, triggers).

## Objects

### Command: `list_objects`

List all objects in a zone.

**Request:**
```json
{
  "command": "list_objects",
  "zone": "1"
}
```

**Response:**
```json
{
  "status": "ok",
  "objects": [
    {
      "vnum": 100,
      "name": "test object",
      "short_desc": "тестовый предмет",
      "type": 1
    }
  ]
}
```

### Command: `get_object`

Get detailed object data.

**Request:**
```json
{
  "command": "get_object",
  "vnum": 100
}
```

**Response:**
```json
{
  "status": "ok",
  "object": {
    "vnum": 100,
    "names": {
      "aliases": "test object",
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

### Command: `update_object`

Update object fields.

**Request:**
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

**Response:**
```json
{
  "status": "ok",
  "message": "Object updated successfully"
}
```

**Updatable Field Groups:**

**names:** All Russian case forms
- `aliases`, `nominative`, `genitive`, `dative`, `accusative`, `instrumental`, `prepositional` (string)

**descriptions:**
- `short_desc` (string) - Inventory name
- `long_desc` (string) - Ground description
- `action_desc` (string) - Worn description

**stats:**
- `type` (int) - Object type (1=light, 2=scroll, etc.)
- `wear_flags` (array of strings) - ITEM_WEAR_* flags
- `extra_flags` (array of strings) - ITEM_* flags
- `no_flags` (array of strings) - ITEM_NO_* flags
- `anti_flags` (array of strings) - ITEM_ANTI_* flags
- `weight` (int) - Weight in arbitrary units
- `cost` (int) - Shop price
- `rent` (int) - Rent cost
- `minimum_level` (int) - Level restriction
- `sex` (int) - Grammatical gender
- `maximum_durability` (int) - Max item condition
- `current_durability` (int) - Current condition
- `material` (int) - Material type ID
- `timer` (int) - Decay timer
- `quantity` (int) - Stack size

**type_specific:** Object type-dependent values
- `value0`, `value1`, `value2`, `value3` (int)
- Meaning varies by object type (e.g., for weapons: skill, dice_count, dice_size)

**affects:** Applied stat modifiers
- Array of `{location: int, modifier: int}` pairs
- Example: `[{"location": 1, "modifier": 5}]` (+5 to strength)

**extra_descriptions:** Additional examine texts
- Array of `{keywords: string, description: string}` pairs

**skills:** Skill bonuses
- Format: `{"skill_name": bonus}` (e.g., `{"riding": 10}`)

**triggers:** DG Script triggers
- Array of trigger vnums

### Command: `create_object`

Create new object in zone.

**Request:**
```json
{
  "command": "create_object",
  "zone": 3,
  "data": {
    "names": {
      "aliases": "new item"
    },
    "stats": {
      "type": 1
    }
  }
}
```

**Response:**
```json
{
  "status": "ok",
  "message": "Object created successfully",
  "vnum": 390
}
```

### Command: `delete_object`

**NOT IMPLEMENTED** - Same reasons as delete_mob.

## Rooms

### Command: `list_rooms`

List all rooms in a zone.

**Request:**
```json
{
  "command": "list_rooms",
  "zone": "1"
}
```

**Response:**
```json
{
  "status": "ok",
  "rooms": [
    {
      "vnum": 100,
      "name": "Test Room"
    }
  ]
}
```

### Command: `get_room`

Get detailed room data.

**Request:**
```json
{
  "command": "get_room",
  "vnum": 100
}
```

**Response:**
```json
{
  "status": "ok",
  "room": {
    "vnum": 100,
    "name": "Test Room",
    "description": "This is a test room.\n",
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

### Command: `update_room`

Update room fields.

**Request:**
```json
{
  "command": "update_room",
  "vnum": 100,
  "data": {
    "name": "Modified Room",
    "sector": 2,
    "triggers": [100]
  }
}
```

**Response:**
```json
{
  "status": "ok",
  "message": "Room updated successfully"
}
```

**Updatable Fields:**

- `name` (string) - Room title
- `description` (string) - Room description (should end with `\n`)
- `sector` (int) - Terrain type (0=inside, 1=city, 2=field, etc.)
- `room_flags` (array of strings) - ROOM_* flags
- `extra_descriptions` (array) - `[{keywords: string, description: string}]`
- `triggers` (array of int) - World trigger vnums

**exits:** Array of exit objects
- `direction` (int) - 0=north, 1=east, 2=south, 3=west, 4=up, 5=down
- `description` (string) - Exit look text
- `keywords` (string) - Door keywords
- `exit_info` (int) - Exit flags (0=open, 1=door, 2=locked, etc.)
- `key` (int) - Key object vnum (-1=no key)
- `to_room` (int) - Destination room vnum

### Command: `create_room`

**NOT IMPLEMENTED** - Planned for future.

### Command: `delete_room`

**NOT IMPLEMENTED** - Same reasons as delete_mob.

## Triggers

### Command: `list_triggers`

List all triggers in a zone.

**Request:**
```json
{
  "command": "list_triggers",
  "zone": "1"
}
```

**Response:**
```json
{
  "status": "ok",
  "triggers": [
    {
      "vnum": 100,
      "name": "Test Trigger",
      "type": "GREET"
    }
  ]
}
```

### Command: `get_trigger`

Get detailed trigger data.

**Request:**
```json
{
  "command": "get_trigger",
  "vnum": 100
}
```

**Response:**
```json
{
  "status": "ok",
  "trigger": {
    "vnum": 100,
    "name": "Test Trigger",
    "attach_type": 2,
    "trigger_type": "GREET",
    "narg": 100,
    "argument": "",
    "commands": "say Hello, %actor.name%!\n"
  }
}
```

### Command: `update_trigger`

Update trigger script.

**Request:**
```json
{
  "command": "update_trigger",
  "vnum": 100,
  "data": {
    "name": "Modified Trigger",
    "commands": "say Updated greeting!\n"
  }
}
```

**Response:**
```json
{
  "status": "ok",
  "message": "Trigger updated successfully"
}
```

**Updatable Fields:**

- `name` (string) - Trigger name
- `attach_type` (int) - 0=mob, 1=object, 2=room
- `trigger_type` (string) - Trigger type ("GREET", "COMMAND", "RANDOM", etc.)
- `narg` (int) - Numeric argument (varies by type)
- `argument` (string) - Text argument (varies by type)
- `commands` (string) - DG Script code

**Common Trigger Types:**
- MOB: `GREET`, `COMMAND`, `SPEECH`, `FIGHT`, `HITPRCNT`, `DEATH`, `ENTRY`, `RECEIVE`, `BRIBE`, `RANDOM`
- OBJECT: `COMMAND`, `TIMER`, `GET`, `DROP`, `GIVE`, `WEAR`, `REMOVE`, `RANDOM`
- ROOM: `COMMAND`, `SPEECH`, `ENTER`, `DROP`, `RANDOM`

### Command: `create_trigger`

**NOT IMPLEMENTED** - Planned for future.

### Command: `delete_trigger`

**NOT IMPLEMENTED** - Same reasons as delete_mob.

## Error Handling

All errors return:
```json
{
  "status": "error",
  "error": "Error message description"
}
```

**Common Errors:**

- `"Not authenticated. Use 'auth' command first."` - Missing authentication
- `"Authentication failed"` - Invalid credentials or insufficient privileges
- `"Zone not found"` - Invalid zone vnum
- `"Mob not found"` - Invalid mob vnum
- `"Object not found"` - Invalid object vnum
- `"Room not found"` - Invalid room vnum
- `"Trigger not found"` - Invalid trigger vnum
- `"Max admin connections reached"` - Another admin is connected
- `"No available vnums in zone"` - Zone vnum range exhausted (100/100 used)
- `"JSON parse error: ..."` - Malformed request
- `"Unknown command"` - Invalid command name

## Usage Examples

### Python Client

```python
#!/usr/bin/env python3
import socket
import json

# Connect to socket
sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
sock.connect('lib/admin_api.sock')

def send_command(cmd):
    """Send JSON command and receive response."""
    sock.sendall((json.dumps(cmd) + '\n').encode('utf-8'))
    response = b''
    while True:
        chunk = sock.recv(4096)
        response += chunk
        if b'\n' in response:
            line = response.split(b'\n', 1)[0]
            return json.loads(line.decode('utf-8'))

# Authenticate
print(send_command({
    'command': 'auth',
    'username': 'YourName',
    'password': 'your_password'
}))

# List mobs in zone 1
print(send_command({
    'command': 'list_mobs',
    'zone': '1'
}))

# Get mob details
mob_data = send_command({
    'command': 'get_mob',
    'vnum': 100
})
print(f"Mob level: {mob_data['mob']['stats']['level']}")

# Update mob
print(send_command({
    'command': 'update_mob',
    'vnum': 100,
    'data': {
        'stats': {'level': 10}
    }
}))

sock.close()
```

### Using netcat (nc)

```bash
# Single command (requires re-authentication)
echo '{"command":"auth","username":"YourName","password":"pass"}' | nc -U lib/admin_api.sock

# Interactive session (persistent connection)
nc -U lib/admin_api.sock << 'EOF'
{"command":"auth","username":"YourName","password":"pass"}
{"command":"list_mobs","zone":"1"}
{"command":"get_mob","vnum":100}
EOF

# Multi-line script
(
    echo '{"command":"auth","username":"YourName","password":"pass"}'
    sleep 0.1
    echo '{"command":"get_mob","vnum":100}'
) | nc -U lib/admin_api.sock
```

### Using curl (via socat proxy)

```bash
# Start HTTP→Unix socket proxy
socat TCP-LISTEN:8888,reuseaddr,fork UNIX-CONNECT:lib/admin_api.sock &

# Use curl
curl -X POST http://localhost:8888 \
     -H "Content-Type: application/json" \
     -d '{"command":"ping"}'

curl -X POST http://localhost:8888 \
     -H "Content-Type: application/json" \
     -d '{"command":"auth","username":"YourName","password":"pass"}'
```

## Limitations

1. **Single Connection:** Only one admin connection allowed simultaneously
2. **No Deletion:** Create operations supported, but delete operations not implemented
3. **No Zone Create/Delete:** Zone management limited to updates
4. **Vnum Exhaustion:** Each zone limited to 100 vnums (zone N: N×100 to N×100+99)
5. **Live Data Only:** API operates on in-memory data; server must save to persist changes
6. **No Validation:** Limited input validation; malformed data may crash server
7. **No Transactions:** Changes are immediate; no rollback support
8. **Encoding:** All text must be valid UTF-8 (automatic conversion to/from KOI8-R)

## Compilation

Admin API is optional and disabled by default.

**Enable:**
```bash
cmake -DENABLE_ADMIN_API=ON ..
make
```

**Configuration:**
Edit `lib/misc/configuration.xml`:
```xml
<admin_api>
    <socket_path>admin_api.sock</socket_path>
    <enabled>true</enabled>
</admin_api>
```

**Verify:**
```bash
# Check socket exists
ls -la lib/admin_api.sock

# Test connection
echo '{"command":"ping"}' | nc -U lib/admin_api.sock
# Expected: {"status":"pong"}
```

## Security

**Access Control:**
- Unix Domain Socket = local access only (no network exposure)
- File permissions: 0600 (owner-only read/write)
- Authentication required for all operations except `ping`
- Minimum privilege level: Builder (kLvlBuilder)

**Recommendations:**
- Use strong passwords for builder/immortal accounts
- Restrict filesystem access to game directory
- Monitor connections via server logs
- Run game server under dedicated user account
- Consider SSH tunneling for remote administration

## Troubleshooting

### Connection refused
```
nc: unix connect failed: Connection refused
```
**Solutions:**
- Check `admin_api.enabled` is `true` in configuration.xml
- Verify server compiled with `-DENABLE_ADMIN_API=ON`
- Confirm socket file exists: `ls -la lib/admin_api.sock`
- Check server logs for initialization errors

### Max connections reached
```
{"status":"error","error":"Max admin connections reached"}
```
**Solutions:**
- Close existing admin connection
- Check for stale connections: `lsof lib/admin_api.sock`
- Restart server to clear connections

### Authentication failed
```
{"status":"error","error":"Authentication failed"}
```
**Solutions:**
- Verify username/password are correct
- Check account level is >= Builder: `stat player_file`
- Ensure account exists in game files
- Check server logs for detailed error

### JSON parse error
```
{"status":"error","error":"JSON parse error: ..."}
```
**Solutions:**
- Validate JSON syntax (use `jq` or online validator)
- Ensure proper UTF-8 encoding
- Check for missing quotes, commas, braces
- Verify line termination with `\n`

### Mob/Object/Room not found
```
{"status":"error","error":"Mob not found"}
```
**Solutions:**
- Verify vnum exists: use `list_mobs`, `list_objects`, `list_rooms`
- Check vnum is in correct zone range (zone N: N×100 to N×100+99)
- Ensure zone is loaded (check `list_zones`)

### No available vnums
```
{"status":"error","error":"No available vnums in zone"}
```
**Solutions:**
- Choose different zone with free vnums
- Delete unused entities in zone (not via API - use OLC in-game)
- Use explicit vnum assignment (if supported)
- Zones limited to 100 vnums each (0-99)

### Commands truncated
**Symptom:** Commands cut off mid-character (especially with Cyrillic text)

**Solution:**
- Ensure server built with process_input() fixes for UTF-8
- Check `admin_api_mode` flag properly set on descriptor
- Verify automatic UTF-8 ↔ KOI8-R conversion enabled

### Empty responses
**Symptom:** Fields missing or empty in get_* responses

**Solutions:**
- Check prototype data exists (use OLC in-game to verify)
- For triggers: ensure `proto_script` populated (not just `script`)
- Verify field supported by API (see field lists above)

## See Also

- **OLC Documentation:** In-game help for `MEDIT`, `OEDIT`, `REDIT`, `ZEDIT`, `TRIGEDIT`
- **DG Scripts:** `/src/engine/scripting/dg_scripts.h` for trigger types
- **Source Code:** `/src/engine/network/admin_api.cpp` for implementation details
- **Tests:** `/tests/test_admin_api.py` for usage examples
