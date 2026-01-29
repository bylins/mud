# Manual Testing Plan - World Save/Load (All Formats)

## Prerequisites

1. **Build configurations needed:**
   - Legacy: Default build (no special flags)
   - YAML: Build with YAML support enabled
   - SQLite: Build with `-DHAVE_SQLITE=ON`

2. **Test data preparation:**
   - Backup existing world data: `cp -r lib/world lib/world.backup`
   - For SQLite: Create test database with converter

3. **Access requirements:**
   - God-level character
   - OLC permissions enabled
   - Telnet client or MUD client

---

## Test Format

Each test follows this structure:
```
Test ID: [Format]-[Component]-[Number]
Objective: What we're testing
Pre-conditions: Required setup
Steps: Commands to execute
Verification: How to verify success
SQL Queries (for SQLite): Database checks
Expected Files (for YAML/Legacy): File paths to check
```

---

# SQLITE TESTS

## Test Suite: SQLite-Zones

### SQ-ZON-001: Create New Zone
**Objective**: Verify zone creation and save to SQLite

**Pre-conditions**:
- Server started with SQLite data source
- Connected as god character

**Steps**:
```
1. zedit 999
2. Set name: Test Zone SQLite
3. Set top: 99999
4. Set lifespan: 30
5. Set reset_mode: 2
6. save
7. quit (exit zedit)
```

**Verification - SQL**:
```sql
-- Check zone exists
SELECT * FROM zones WHERE vnum = 999;

-- Expected: 1 row with vnum=999, name='Test Zone SQLite', lifespan=30, reset_mode=2

-- Check zone_groups if set
SELECT * FROM zone_groups WHERE zone_vnum = 999;
```

**Verification - Game**:
```
1. shutdown reboot
2. zedit 999
3. Verify: name, top, lifespan, reset_mode match
```

### SQ-ZON-002: Add Zone Commands (All Types)
**Objective**: Test all zone command types (M,O,G,E,P,D,R,T)

**Steps**:
```
zedit 999
commands
add M
  if_flag: 0
  mob_vnum: 1000
  max_in_world: 1
  room: 99900
save
```

**Verification - SQL**:
```sql
SELECT * FROM zone_commands WHERE zone_vnum = 999 ORDER BY cmd_order;

-- Expected columns (check after column name fix):
-- cmd_type, arg_mob_vnum, arg_room_vnum, arg_max_world, etc.
```

**KNOWN BUG**: Column names mismatch (command_order vs cmd_order). Fix before testing.

---

## Test Suite: SQLite-Rooms

### SQ-ROM-001: Create Room with Basic Properties
**Objective**: Room save with name, description, sector

**Steps**:
```
redit 99900
name Test Room SQLite
description
Enter multi-line description:
This is a test room for SQLite save functionality.
It has multiple lines.
@
sector field
save
```

**Verification - SQL**:
```sql
SELECT vnum, name, sector_type FROM rooms WHERE vnum = 99900;
-- Expected: name='Test Room SQLite', sector_type matches 'field' enum value
```

### SQ-ROM-002: Add Room Flags
**Objective**: Test room flag persistence

**Steps**:
```
redit 99900
flags
[Select: indoors, nomob]
save
```

**Verification - SQL**:
```sql
SELECT * FROM room_flags WHERE room_vnum = 99900;
-- Expected: 2 rows with flag_name IN ('kIndoors', 'kNoMob')
```

### SQ-ROM-003: Add Exits
**Objective**: Test room exits with all properties

**Steps**:
```
redit 99900
exit north
  to_room: 99901
  description: A path leads north into darkness.
  keywords: path door
  flags: door closed locked
  key: 99950
  lock_complexity: 10
done
save
```

**Verification - SQL**:
```sql
SELECT * FROM room_exits WHERE room_vnum = 99900;
-- Expected: direction_id=0 (north), to_room=99901, exit_flags match, key_vnum=99950, lock_complexity=10
```

### SQ-ROM-004: Add Extra Descriptions
**Objective**: Test extra description persistence

**Steps**:
```
redit 99900
extra
keywords: statue pillar
description
A weathered stone pillar stands here.
@
save
```

**Verification - SQL**:
```sql
SELECT * FROM room_extra_descriptions WHERE entity_id IN
  (SELECT id FROM entity_triggers WHERE entity_type='room' AND entity_vnum=99900);
-- Expected: keywords='statue pillar', description contains 'weathered stone pillar'
```

---

## Test Suite: SQLite-Mobs

### SQ-MOB-001: Create Basic Mob
**Objective**: Mob save with names, stats

**Steps**:
```
medit 99901
name тестовый моб
aliases test mob sqlite
short тестовый моб стоит здесь
long
Тестовый моб для проверки SQLite стоит здесь.
~
level 10
save
```

**Verification - SQL**:
```sql
SELECT vnum, level FROM mobs WHERE vnum = 99901;
-- Expected: level=10

-- Check Russian text encoding (should be UTF-8 in DB)
SELECT name_nom FROM mobs WHERE vnum = 99901;
-- Expected: UTF-8 encoded "тестовый моб"
```

### SQ-MOB-002: Add Mob Skills
**Objective**: Test mob skills save

**Steps**:
```
medit 99901
skills
add backstab 80
add bash 60
save
```

**Verification - SQL**:
```sql
SELECT * FROM mob_skills WHERE mob_vnum = 99901;
-- KNOWN BUG: This table has no save code!
-- Expected AFTER FIX: 2 rows with skill_id and value
```

### SQ-MOB-003: Add Mob Resistances
**Objective**: Test resistances save

**Steps**:
```
medit 99901
resistances
fire: 25
cold: 15
save
```

**Verification - SQL**:
```sql
SELECT * FROM mob_resistances WHERE mob_vnum = 99901;
-- KNOWN BUG: Column name is 'value' not 'resist_value'
-- Expected AFTER FIX: 2 rows with resist_type and value
```

### SQ-MOB-004: Add Mob Flags (Action + Affect)
**Objective**: Test both flag categories

**Steps**:
```
medit 99901
action_flags
[Select: sentinel, aggressive]
affect_flags
[Select: detect_invisible, sanctuary]
save
```

**Verification - SQL**:
```sql
SELECT * FROM mob_flags WHERE mob_vnum = 99901 ORDER BY flag_category, flag_name;
-- Expected: 2 rows with flag_category='action'
-- KNOWN BUG: No save code for flag_category='affect'
```

---

## Test Suite: SQLite-Objects

### SQ-OBJ-001: Create Object with Names
**Objective**: Object save with Russian cases

**Steps**:
```
oedit 99902
names
  nom: меч
  gen: меча
  dat: мечу
  acc: меч
  ins: мечом
  pre: мече
aliases sword blade
short блестящий меч
action Блестящий меч лежит здесь.
save
```

**Verification - SQL**:
```sql
SELECT name_nom, name_gen, aliases FROM objects WHERE vnum = 99902;
-- Expected: All Russian cases in UTF-8, aliases='sword blade'
```

### SQ-OBJ-002: Add Object Applies
**Objective**: Test stat modifiers

**Steps**:
```
oedit 99902
applies
add str 2
add hitroll 5
save
```

**Verification - SQL**:
```sql
SELECT * FROM obj_applies WHERE obj_vnum = 99902;
-- KNOWN BUG: Column names are 'location_id' and 'modifier', not 'affect_location' and 'affect_modifier'
-- Expected AFTER FIX: 2 rows
```

### SQ-OBJ-003: Add Object Flags
**Objective**: Test extra and wear flags

**Steps**:
```
oedit 99902
extra_flags
[Select: glow, magic]
wear_flags
[Select: take, wield]
save
```

**Verification - SQL**:
```sql
SELECT * FROM obj_flags WHERE obj_vnum = 99902;
-- Expected: glow, magic in extra flags
-- KNOWN BUG: No save code for wear flags!
```

---

## Test Suite: SQLite-Triggers

### SQ-TRG-001: Create Trigger with Script
**Objective**: Trigger save with commands

**Steps**:
```
trigedit 99903
name test greet trigger
attach mob
trigger_types greet
narg 100
arglist
script
if %actor.is_pc%
  say Добро пожаловать, %actor.name%!
end
~
save
```

**Verification - SQL**:
```sql
SELECT * FROM triggers WHERE vnum = 99903;
-- Expected: name='test greet trigger', attach_type='mob', narg=100

SELECT * FROM trigger_commands WHERE trigger_id =
  (SELECT id FROM triggers WHERE vnum = 99903)
ORDER BY cmd_order;
-- Expected: Script commands in order, Russian text in UTF-8
```

---

# YAML TESTS

## Test Suite: YAML-Zones

### YA-ZON-001: Create Zone and Verify YAML File
**Objective**: Zone save to YAML format

**Pre-conditions**:
- Server started with YAML data source
- YAML world directory exists

**Steps**:
```
zedit 998
name Test Zone YAML
lifespan 25
save
```

**Verification - Files**:
```bash
# Check YAML file created
ls -la lib/world/yaml/zones/998/

# Expected: zone.yaml file exists

# Check YAML content
cat lib/world/yaml/zones/998/zone.yaml

# Expected YAML structure:
# vnum: 998
# name: Test Zone YAML
# lifespan: 25
# ...

# Verify UTF-8 encoding
file lib/world/yaml/zones/998/zone.yaml
# Expected: UTF-8 Unicode text
```

**KNOWN BUG**: WriteYamlAtomic function broken (patch conflict). Fix before testing!

### YA-ZON-002: Zone with Russian Metadata
**Objective**: Test encoding conversion (KOI8-R → UTF-8)

**Steps**:
```
zedit 998
metadata
  comment: Тестовая зона
  description: Описание зоны для проверки
save
```

**Verification - Files**:
```bash
cat lib/world/yaml/zones/998/zone.yaml | grep -A 5 metadata

# Expected: Russian text in valid UTF-8
# Verify with: iconv -f utf8 -t utf8 (should not error)
```

---

## Test Suite: YAML-Rooms

### YA-ROM-001: Room with All Properties
**Objective**: Complete room save

**Steps**:
```
redit 99800
name Тестовая комната YAML
description
Просторная комната для тестирования YAML формата.
Здесь можно проверить все функции.
@
sector city
flags indoors peaceful
save
```

**Verification - Files**:
```bash
cat lib/world/yaml/wld/998/99800.yaml

# Expected YAML:
# vnum: 99800
# name: Тестовая комната YAML
# description: |
#   Просторная комната...
# sector: city
# flags:
#   - indoors
#   - peaceful
```

### YA-ROM-002: Room with Exits (All Directions)
**Objective**: Test all 6 exits

**Steps**:
```
redit 99800
For each direction (north, east, south, west, up, down):
  exit [direction]
    to_room: [99801-99806]
    description: Path leads [direction].
  done
save
```

**Verification - Files**:
```bash
cat lib/world/yaml/wld/998/99800.yaml | grep -A 10 exits

# Expected: All 6 exits with to_room, descriptions in UTF-8
```

---

## Test Suite: YAML-Mobs

### YA-MOB-001: Mob with Enhanced Fields
**Objective**: Test all enhanced mob fields save to YAML

**Steps**:
```
medit 99801
name тестовый страж
level 15
enhanced
  hp_regen: 10
  mana_regen: 5
  absorb: 20
  resistances:
    fire: 30
    cold: 20
  saves:
    will: 10
    reflex: 5
  feats:
    - dodge
    - improved_initiative
  spells:
    - fireball: 3
    - magic_missile: 5
save
```

**Verification - Files**:
```bash
cat lib/world/yaml/mob/998/99801.yaml

# Expected: All enhanced fields present:
# enhanced:
#   hp_regen: 10
#   resistances:
#     fire: 30
#   feats:
#     - dodge
#   ...
```

---

## Test Suite: YAML-Objects

### YA-OBJ-001: Object with All Flag Types
**Objective**: Test all object flag types

**Steps**:
```
oedit 99802
names
  nom: кольцо
  gen: кольца
  [... all cases ...]
extra_flags magic glow
wear_flags take finger
anti_flags anti_evil anti_neutral
no_flags no_drop no_sell
save
```

**Verification - Files**:
```bash
cat lib/world/yaml/obj/998/99802.yaml

# Expected:
# extra_flags: [magic, glow]
# wear_flags: [take, finger]
# anti_flags: [anti_evil, anti_neutral]
# no_flags: [no_drop, no_sell]
```

---

## Test Suite: YAML-Index Files

### YA-IDX-001: Index Update on Save
**Objective**: Verify index.yaml updated

**Steps**:
```
1. Save new mob 99803
2. Save new mob 99804
3. Check index
```

**Verification - Files**:
```bash
cat lib/world/yaml/mob/998/index.yaml

# Expected: List contains both 99803 and 99804
# Format: YAML list of vnums
```

---

# LEGACY TESTS

## Test Suite: Legacy-Zones

### LG-ZON-001: Zone File Format
**Objective**: Verify legacy .zon format

**Steps**:
```
zedit 997
name Legacy Test Zone
lifespan 20
save
```

**Verification - Files**:
```bash
cat lib/world/zon/997.zon

# Expected format:
# #997
# Legacy Test Zone~
# 99799 2 0 -1 99700 20 2 0 0 0
# <zone commands>
# S
# $
```

### LG-ZON-002: Zone Commands All Types
**Objective**: Test all command types in legacy format

**Steps**:
```
zedit 997
commands
Add each command type:
  M - Load mob
  O - Load object to room
  G - Give object to mob
  E - Equip object on mob
  P - Put object in container
  D - Set door state
  R - Remove object from room
  T - Attach trigger
save
```

**Verification - Files**:
```bash
cat lib/world/zon/997.zon | grep -A 20 "zone commands section"

# Expected: Each command on separate line with correct format:
# M 0 <mob_vnum> <max> <room>
# O 0 <obj_vnum> <max> <room>
# etc.
```

---

## Test Suite: Legacy-Rooms

### LG-ROM-001: Room File Format
**Objective**: Verify .wld format

**Steps**:
```
redit 99700
name Legacy Test Room
description
This room tests legacy format.
~
sector city
flags dark nomob
exit north 99701
save
```

**Verification - Files**:
```bash
cat lib/world/wld/997.wld

# Expected format:
# #99700
# Legacy Test Room~
# This room tests legacy format.
# ~
# 997 12 1
# D0
# ~
# ~
# 0 -1 99701
# S
```

---

## Test Suite: Legacy-Mobs

### LG-MOB-001: Mob File Format
**Objective**: Verify .mob format with all optional fields

**Steps**:
```
medit 99701
All 6 Russian name cases + aliases + descriptions
Stats, skills, spells, feats
save
```

**Verification - Files**:
```bash
cat lib/world/mob/997.mob

# Expected format:
# #99701
# aliases~
# nom~gen~dat~acc~ins~pre~
# long_desc~
# description
# ~
# <stats line>
# <extended stats>
# Skill: <id> <value>
# Spell: <id> <count>
# Feat: <id>
# E
```

---

## Test Suite: Legacy-Objects

### LG-OBJ-001: Object File Format
**Objective**: Verify .obj format

**Steps**:
```
oedit 99702
Type: weapon
Values: 2d6+3 damage, slashing
Extra flags, wear flags
Applies
save
```

**Verification - Files**:
```bash
cat lib/world/obj/997.obj

# Expected format:
# #99702
# aliases~
# nom~gen~dat~acc~ins~pre~
# short_desc~
# action_desc~
# ~
# <type> <extra_flags> <wear_flags>
# <values line 1>
# <values line 2>
# <values line 3>
# <values line 4>
# <applies>
# $
```

---

# CROSS-FORMAT TESTS

## Test Suite: Format Consistency

### CF-001: Same Data All Formats
**Objective**: Verify equivalent data across formats

**Steps**:
```
1. Create identical zone in all 3 formats:
   - Legacy: zone 997
   - YAML: zone 998
   - SQLite: zone 999

2. Use same properties for all:
   - Name: "Format Test Zone"
   - Lifespan: 30
   - Reset mode: 2
   - Same zone commands

3. Save all three

4. Compare loaded data
```

**Verification**:
```
For each format:
  1. Restart server with that format
  2. stat zone [vnum]
  3. Document all fields

Compare output - should be identical except vnum
```

### CF-002: Round-Trip Preservation
**Objective**: Save → Load → Save produces identical files

**Steps**:
```
For each format:
  1. Load world
  2. Save world (without changes)
  3. Compare files before/after save
```

**Verification**:
```bash
# For Legacy:
diff lib/world/zon/0.zon lib/world/zon/0.zon.backup

# For YAML:
diff lib/world/yaml/zones/0/zone.yaml <backup>

# For SQLite:
sqlite3 world.db "SELECT * FROM zones ORDER BY vnum" > before.txt
# After reload and save:
sqlite3 world.db "SELECT * FROM zones ORDER BY vnum" > after.txt
diff before.txt after.txt

# Expected: No differences (or only timestamp/order differences)
```

---

# ERROR HANDLING TESTS

## Test Suite: Error Recovery

### ER-001: Read-Only Filesystem (All Formats)
**Objective**: Graceful failure on write errors

**Steps**:
```bash
# Make world directory read-only
chmod -R a-w lib/world/

# Try to save in game
zedit 1
save

# Expected: Error logged, old files intact
```

### ER-002: SQLite Transaction Rollback
**Objective**: Database consistency on failure

**Steps**:
```
1. Create invalid zone (vnum > max allowed)
2. Attempt save
3. Check database state

Expected: Transaction rolled back, database unchanged
```

### ER-003: YAML Atomic Write Failure
**Objective**: .old backup preserved on error

**Steps**:
```
1. Save zone successfully (creates file)
2. Fill disk (or make directory read-only)
3. Modify zone and save
4. Check .old file exists and is intact

Expected: Original file renamed to .old, preserved on error
```

---

# SUMMARY

## Test Execution Order

1. **Fix critical bugs first** (before any testing):
   - SQLite column name mismatches (5 fixes)
   - SQLite missing implementations (5 additions)
   - YAML WriteYamlAtomic patch conflict (1 fix)

2. **Run automated tests** (if implemented):
   ```bash
   cd build && ./tests/tests --gtest_filter=WorldSaveLoad*
   ```

3. **Run manual tests by priority**:
   - Priority 1: Basic save/load (ZON-001, ROM-001, MOB-001, OBJ-001 for each format)
   - Priority 2: Round-trip tests (CF-002)
   - Priority 3: All features (remaining tests)
   - Priority 4: Error handling (ER-*)

## Test Tracking

Use this checklist:

```
[ ] SQLite bugs fixed
[ ] YAML bugs fixed
[ ] SQ-ZON-001 through SQ-ZON-002
[ ] SQ-ROM-001 through SQ-ROM-004
[ ] SQ-MOB-001 through SQ-MOB-004
[ ] SQ-OBJ-001 through SQ-OBJ-003
[ ] SQ-TRG-001
[ ] YA-ZON-001 through YA-ZON-002
[ ] YA-ROM-001 through YA-ROM-002
[ ] YA-MOB-001
[ ] YA-OBJ-001
[ ] YA-IDX-001
[ ] LG-ZON-001 through LG-ZON-002
[ ] LG-ROM-001
[ ] LG-MOB-001
[ ] LG-OBJ-001
[ ] CF-001 through CF-002
[ ] ER-001 through ER-003
```

## Estimated Testing Time

- Bug fixes: 2-4 hours
- Automated test setup: 2-3 hours
- Manual testing (all formats): 4-6 hours
- **Total: 8-13 hours**
