# SQLite World Schema Documentation

This document describes the SQLite database schema used to store MUD world data. The schema is normalized to Third Normal Form (3NF) and includes convenient views for browsing.

## Table of Contents

- [General Principles](#general-principles)
- [Entity-Relationship Diagram](#entity-relationship-diagram)
- [Reference Tables](#reference-tables)
- [Main Entity Tables](#main-entity-tables)
- [Linking Tables](#linking-tables)
- [Views](#views)
- [Indexes](#indexes)
- [Example Queries](#example-queries)
- [Conversion from Legacy Format](#conversion-from-legacy-format)

---

## General Principles

### Normalization

The schema is normalized to 3NF to eliminate redundancy:

- **1NF**: All columns contain atomic values (no comma-separated lists in main tables)
- **2NF**: All non-key columns depend on the entire primary key
- **3NF**: No transitive dependencies (flags and skills are in separate tables)

### Foreign Keys

Foreign keys are disabled during import to allow any insertion order:

```sql
PRAGMA foreign_keys = OFF;
-- ... import data ...
PRAGMA foreign_keys = ON;
```

### VNUMs

All main entities use their VNUM (Virtual Number) as the primary key for direct lookup:

- `zones.vnum`
- `mobs.vnum`
- `objects.vnum`
- `rooms.vnum`
- `triggers.vnum`

---

## Entity-Relationship Diagram

```
┌─────────────────┐
│      zones      │
│ vnum (PK)       │◄─────────────┐
└────────┬────────┘              │
         │                       │
         │ 1:N                   │
         ▼                       │
┌─────────────────┐              │
│      rooms      │──────────────┤
│ vnum (PK)       │              │
│ zone_vnum (FK)  │              │
└────────┬────────┘              │
         │                       │
         │ 1:N                   │
         ▼                       │
┌─────────────────┐              │
│   room_exits    │              │
│ room_vnum (FK)  │              │
│ to_room         │──────────────┘
└─────────────────┘

┌─────────────────┐       ┌─────────────────┐
│      mobs       │       │    mob_flags    │
│ vnum (PK)       │◄──────│ mob_vnum (FK)   │
└────────┬────────┘       └─────────────────┘
         │
         │ 1:N            ┌─────────────────┐
         └───────────────►│   mob_skills    │
                          │ mob_vnum (FK)   │
                          └─────────────────┘

┌─────────────────┐       ┌─────────────────┐
│    objects      │       │   obj_flags     │
│ vnum (PK)       │◄──────│ obj_vnum (FK)   │
└────────┬────────┘       └─────────────────┘
         │
         │ 1:N            ┌─────────────────┐
         └───────────────►│   obj_applies   │
                          │ obj_vnum (FK)   │
                          └─────────────────┘

┌─────────────────┐       ┌─────────────────┐
│    triggers     │◄──────│entity_triggers  │
│ vnum (PK)       │       │trigger_vnum(FK) │
└─────────────────┘       │entity_type      │
                          │entity_vnum      │
                          └─────────────────┘
```

---

## Reference Tables

Reference tables store enum-like values for foreign key relationships. These are populated at schema creation time.

### obj_types

Object type constants.

| Column | Type | Description |
|--------|------|-------------|
| id | INTEGER PK | Type ID |
| name | TEXT UNIQUE | Type name (e.g., kWeapon) |
| description | TEXT | Human-readable description |

### sectors

Room sector types.

| Column | Type | Description |
|--------|------|-------------|
| id | INTEGER PK | Sector ID |
| name | TEXT UNIQUE | Sector name (e.g., kInside) |
| description | TEXT | Human-readable description |

### positions

Character position types.

| Column | Type | Description |
|--------|------|-------------|
| id | INTEGER PK | Position ID |
| name | TEXT UNIQUE | Position name (e.g., kStanding) |

### genders

Gender types.

| Column | Type | Description |
|--------|------|-------------|
| id | INTEGER PK | Gender ID |
| name | TEXT UNIQUE | Gender name (kMale, kFemale, kNeutral) |

### directions

Exit direction types.

| Column | Type | Description |
|--------|------|-------------|
| id | INTEGER PK | Direction ID (0-5) |
| name | TEXT UNIQUE | Direction name (kNorth, kEast, etc.) |

### skills

Skill definitions.

| Column | Type | Description |
|--------|------|-------------|
| id | INTEGER PK | Skill ID |
| name | TEXT UNIQUE | Skill name (e.g., kPunch) |
| description | TEXT | Skill description |

### apply_locations

Apply location types for object modifiers.

| Column | Type | Description |
|--------|------|-------------|
| id | INTEGER PK | Apply ID |
| name | TEXT UNIQUE | Apply name (e.g., kStr, kDex) |
| description | TEXT | Apply description |

### wear_positions

Equipment wear positions.

| Column | Type | Description |
|--------|------|-------------|
| id | INTEGER PK | Position ID |
| name | TEXT UNIQUE | Position name |

### trigger_attach_types

Trigger attachment types.

| Column | Type | Description |
|--------|------|-------------|
| id | INTEGER PK | Type ID |
| name | TEXT UNIQUE | Type name (kMob, kObj, kRoom) |

### trigger_type_defs

Trigger type character definitions for normalized trigger type storage.

| Column | Type | Description |
|--------|------|-------------|
| char_code | TEXT PK | Single character code (a-z, A-Z) |
| name | TEXT UNIQUE | Type name (e.g., kGlobal, kRandom) |
| bit_position | INTEGER | Bit position for bitmask calculation |

Lowercase letters (a-z) map to bits 0-25, uppercase (A-Z) to bits 26-51.

---

## Main Entity Tables

### zones

Zone definitions with metadata and reset parameters.

| Column | Type | Default | Description |
|--------|------|---------|-------------|
| vnum | INTEGER PK | | Zone VNUM |
| name | TEXT NOT NULL | | Zone name |
| comment | TEXT | | Optional comment |
| location | TEXT | | Location description |
| author | TEXT | | Author name |
| description | TEXT | | Zone description |
| builders | TEXT | | Builder names |
| first_room | INTEGER | | First room VNUM |
| top_room | INTEGER | | Last room VNUM |
| mode | INTEGER | 0 | Zone mode |
| zone_type | INTEGER | 0 | Zone type |
| zone_group | INTEGER | 1 | Zone group for checksum calculation |
| entrance | INTEGER | | Entrance room VNUM |
| lifespan | INTEGER | 10 | Reset interval (minutes) |
| reset_mode | INTEGER | 2 | Reset mode (0=never, 1=empty, 2=always) |
| reset_idle | INTEGER | 0 | Reset on idle flag |

### mobs

Mobile (NPC) definitions with all stats.

| Column | Type | Default | Description |
|--------|------|---------|-------------|
| vnum | INTEGER PK | | Mob VNUM |
| aliases | TEXT | | Keywords for targeting |
| name_nom | TEXT | | Nominative case |
| name_gen | TEXT | | Genitive case |
| name_dat | TEXT | | Dative case |
| name_acc | TEXT | | Accusative case |
| name_ins | TEXT | | Instrumental case |
| name_pre | TEXT | | Prepositional case |
| short_desc | TEXT | | Short description |
| long_desc | TEXT | | Long description |
| alignment | INTEGER | 0 | Alignment (-1000 to 1000) |
| mob_type | TEXT | 'S' | 'S' (simple) or 'E' (extended) |
| level | INTEGER | 1 | Mob level |
| hitroll_penalty | INTEGER | 0 | Hit roll penalty |
| armor | INTEGER | 100 | Armor class |
| hp_dice_count | INTEGER | 1 | HP dice count |
| hp_dice_size | INTEGER | 1 | HP dice size |
| hp_bonus | INTEGER | 0 | HP bonus |
| dam_dice_count | INTEGER | 1 | Damage dice count |
| dam_dice_size | INTEGER | 1 | Damage dice size |
| dam_bonus | INTEGER | 0 | Damage bonus |
| gold_dice_count | INTEGER | 0 | Gold dice count |
| gold_dice_size | INTEGER | 0 | Gold dice size |
| gold_bonus | INTEGER | 0 | Gold bonus |
| experience | INTEGER | 0 | Experience reward |
| default_pos | INTEGER FK | | Default position |
| start_pos | INTEGER FK | | Starting position |
| sex | INTEGER FK | | Gender |
| size | INTEGER | 50 | Size |
| height | INTEGER | 170 | Height |
| weight | INTEGER | 70 | Weight |
| mob_class | INTEGER | | Mob class ID |
| race | INTEGER | | Race ID |
| attr_str | INTEGER | 11 | Strength (E-type only) |
| attr_dex | INTEGER | 11 | Dexterity |
| attr_int | INTEGER | 11 | Intelligence |
| attr_wis | INTEGER | 11 | Wisdom |
| attr_con | INTEGER | 11 | Constitution |
| attr_cha | INTEGER | 11 | Charisma |

### objects

Object definitions with type-specific values.

| Column | Type | Default | Description |
|--------|------|---------|-------------|
| vnum | INTEGER PK | | Object VNUM |
| aliases | TEXT | | Keywords for targeting |
| name_nom | TEXT | | Nominative case |
| name_gen | TEXT | | Genitive case |
| name_dat | TEXT | | Dative case |
| name_acc | TEXT | | Accusative case |
| name_ins | TEXT | | Instrumental case |
| name_pre | TEXT | | Prepositional case |
| short_desc | TEXT | | Description on ground |
| action_desc | TEXT | | Action description |
| obj_type_id | INTEGER FK | | Object type → obj_types.id |
| material | INTEGER | | Material ID |
| value0 | TEXT | | Type-specific value 0 |
| value1 | TEXT | | Type-specific value 1 |
| value2 | TEXT | | Type-specific value 2 |
| value3 | TEXT | | Type-specific value 3 |
| weight | INTEGER | 0 | Weight |
| cost | INTEGER | 0 | Cost |
| rent_off | INTEGER | 0 | Rent when not worn |
| rent_on | INTEGER | 0 | Rent when worn |
| spec_param | INTEGER | 0 | Special parameter |
| max_durability | INTEGER | 100 | Max durability |
| cur_durability | INTEGER | 100 | Current durability |
| timer | INTEGER | -1 | Timer (-1 = none) |
| spell | INTEGER | -1 | Spell ID |
| level | INTEGER | 0 | Required level |
| max_in_world | INTEGER | -1 | Max instances (-1 = unlimited) |

### rooms

Room definitions with descriptions and sector type.

| Column | Type | Default | Description |
|--------|------|---------|-------------|
| vnum | INTEGER PK | | Room VNUM |
| zone_vnum | INTEGER FK | | Parent zone VNUM |
| name | TEXT | | Room name |
| description | TEXT | | Room description |
| sector_id | INTEGER FK | | Sector type → sectors.id |

### triggers

DG Script trigger definitions.

| Column | Type | Default | Description |
|--------|------|---------|-------------|
| vnum | INTEGER PK | | Trigger VNUM |
| name | TEXT | | Trigger name |
| attach_type_id | INTEGER FK | | Attach type → trigger_attach_types.id |
| narg | INTEGER | 0 | Numeric argument |
| arglist | TEXT | | Argument list |
| script | TEXT | | Script code |

---

## Linking Tables

### zone_groups

Zone grouping for typeA/typeB lists.

| Column | Type | Description |
|--------|------|-------------|
| zone_vnum | INTEGER FK | Zone VNUM |
| linked_zone_vnum | INTEGER | Linked zone VNUM |
| group_type | TEXT | 'A' or 'B' |

Primary Key: (zone_vnum, linked_zone_vnum, group_type)

### mob_flags

Mob action and affect flags.

| Column | Type | Description |
|--------|------|-------------|
| mob_vnum | INTEGER FK | Mob VNUM |
| flag_category | TEXT | 'action' or 'affect' |
| flag_name | TEXT | Flag name (e.g., kIsNpc) |

Primary Key: (mob_vnum, flag_category, flag_name)

### mob_skills

Mob skill values.

| Column | Type | Description |
|--------|------|-------------|
| mob_vnum | INTEGER FK | Mob VNUM |
| skill_id | INTEGER FK | Skill ID → skills.id |
| value | INTEGER | Skill value |

Primary Key: (mob_vnum, skill_id)

### trigger_type_bindings

Normalized trigger type assignments (many-to-many between triggers and type chars).

| Column | Type | Description |
|--------|------|-------------|
| trigger_vnum | INTEGER FK | Trigger VNUM |
| type_char | TEXT FK | Type character (a-z, A-Z) |

Primary Key: (trigger_vnum, type_char)

### obj_flags

Object flags by category (extra, wear, affect, no, anti).

| Column | Type | Description |
|--------|------|-------------|
| obj_vnum | INTEGER FK | Object VNUM |
| flag_category | TEXT | 'extra', 'wear', 'affect', 'no', or 'anti' |
| flag_name | TEXT | Flag name |

Primary Key: (obj_vnum, flag_category, flag_name)

### obj_applies

Object stat modifiers.

| Column | Type | Description |
|--------|------|-------------|
| id | INTEGER PK | Auto-increment ID |
| obj_vnum | INTEGER FK | Object VNUM |
| location_id | INTEGER FK | Apply location → apply_locations.id |
| modifier | INTEGER | Modifier value |

### room_flags

Room flags.

| Column | Type | Description |
|--------|------|-------------|
| room_vnum | INTEGER FK | Room VNUM |
| flag_name | TEXT | Flag name |

Primary Key: (room_vnum, flag_name)

### room_exits

Room exit definitions.

| Column | Type | Default | Description |
|--------|------|---------|-------------|
| id | INTEGER PK | | Auto-increment ID |
| room_vnum | INTEGER FK | | Source room VNUM |
| direction_id | INTEGER FK | | Direction → directions.id |
| description | TEXT | | Exit description |
| keywords | TEXT | | Door keywords |
| exit_flags | TEXT | | Exit flags |
| key_vnum | INTEGER | -1 | Key VNUM |
| to_room | INTEGER | -1 | Destination room VNUM |
| lock_complexity | INTEGER | 0 | Lock complexity |

Unique: (room_vnum, direction_id)

### extra_descriptions

Extra descriptions for objects and rooms.

| Column | Type | Description |
|--------|------|-------------|
| id | INTEGER PK | Auto-increment ID |
| entity_type | TEXT | 'obj' or 'room' |
| entity_vnum | INTEGER | Entity VNUM |
| keywords | TEXT | Keywords |
| description | TEXT | Description |

### entity_triggers

Trigger bindings to mobs, objects, or rooms.

| Column | Type | Description |
|--------|------|-------------|
| id | INTEGER PK | Auto-increment ID |
| entity_type | TEXT | 'mob', 'obj', or 'room' |
| entity_vnum | INTEGER | Entity VNUM |
| trigger_vnum | INTEGER FK | Trigger VNUM |

Unique: (entity_type, entity_vnum, trigger_vnum)

### zone_commands

Zone reset commands.

| Column | Type | Default | Description |
|--------|------|---------|-------------|
| id | INTEGER PK | | Auto-increment ID |
| zone_vnum | INTEGER FK | | Zone VNUM |
| cmd_order | INTEGER | | Execution order |
| cmd_type | TEXT | | Command type |
| if_flag | INTEGER | 0 | Conditional flag |
| arg_mob_vnum | INTEGER | | Mob VNUM |
| arg_obj_vnum | INTEGER | | Object VNUM |
| arg_room_vnum | INTEGER | | Room VNUM |
| arg_trigger_vnum | INTEGER | | Trigger VNUM |
| arg_container_vnum | INTEGER | | Container VNUM |
| arg_max | INTEGER | | Max count |
| arg_max_world | INTEGER | | Max in world |
| arg_max_room | INTEGER | | Max in room |
| arg_load_prob | INTEGER | | Load probability |
| arg_wear_pos_id | INTEGER FK | | Wear position → wear_positions.id |
| arg_direction_id | INTEGER FK | | Direction → directions.id |
| arg_state | INTEGER | | Door state |
| arg_trigger_type | TEXT | | Trigger type |
| arg_context | INTEGER | | Context ID |
| arg_var_name | TEXT | | Variable name |
| arg_var_value | TEXT | | Variable value |
| arg_leader_mob_vnum | INTEGER | | Leader mob VNUM |
| arg_follower_mob_vnum | INTEGER | | Follower mob VNUM |

---

## Views

Views provide convenient access to joined data.

### v_mobs

Full mob information with aggregated flags.

```sql
SELECT vnum, name, aliases, short_desc, level, alignment,
       default_pos, sex, hp_dice, damage_dice, experience,
       mob_class, race, attributes, action_flags, affect_flags
FROM v_mobs;
```

### v_mob_skills

Mob skills with mob names.

```sql
SELECT mob_vnum, mob_name, skill_name, skill_value
FROM v_mob_skills;
```

### v_objects

Full object information with aggregated flags.

```sql
SELECT vnum, name, aliases, short_desc, obj_type, weight,
       cost, level, max_durability, timer, max_in_world,
       extra_flags, wear_flags
FROM v_objects;
```

### v_obj_applies

Object applies with object names.

```sql
SELECT obj_vnum, obj_name, apply_location, modifier
FROM v_obj_applies;
```

### v_rooms

Full room information with zone name and flags.

```sql
SELECT vnum, name, zone_vnum, zone_name, sector,
       description, room_flags
FROM v_rooms;
```

### v_room_exits

Room exits with room and destination names.

```sql
SELECT room_vnum, room_name, direction, to_room,
       destination_name, exit_flags, key_vnum, lock_complexity
FROM v_room_exits;
```

### v_zones

Zone information with room count and group lists.

```sql
SELECT vnum, name, author, location, first_room, top_room,
       lifespan, reset_mode, room_count, typeA_zones, typeB_zones
FROM v_zones;
```

### v_zone_commands

Zone commands with human-readable descriptions.

```sql
SELECT zone_vnum, zone_name, cmd_order, cmd_type, if_flag, description
FROM v_zone_commands;
```

### v_triggers

Trigger overview with script length.

```sql
SELECT vnum, name, attach_type, narg, arglist,
       trigger_types, script_length
FROM v_triggers;
```

### v_entity_triggers

Triggers attached to entities with names.

```sql
SELECT entity_type, entity_vnum, entity_name,
       trigger_vnum, trigger_name
FROM v_entity_triggers;
```

### v_world_stats

World statistics summary.

```sql
SELECT total_zones, total_rooms, total_mobs,
       total_objects, total_triggers, total_zone_commands
FROM v_world_stats;
```

---

## Indexes

Performance indexes for common queries:

```sql
CREATE INDEX idx_mob_flags_vnum ON mob_flags(mob_vnum);
CREATE INDEX idx_mob_skills_vnum ON mob_skills(mob_vnum);
CREATE INDEX idx_obj_flags_vnum ON obj_flags(obj_vnum);
CREATE INDEX idx_obj_applies_vnum ON obj_applies(obj_vnum);
CREATE INDEX idx_room_flags_vnum ON room_flags(room_vnum);
CREATE INDEX idx_room_exits_vnum ON room_exits(room_vnum);
CREATE INDEX idx_room_exits_to ON room_exits(to_room);
CREATE INDEX idx_extra_desc_entity ON extra_descriptions(entity_type, entity_vnum);
CREATE INDEX idx_entity_triggers ON entity_triggers(entity_type, entity_vnum);
CREATE INDEX idx_zone_commands_zone ON zone_commands(zone_vnum, cmd_order);
CREATE INDEX idx_rooms_zone ON rooms(zone_vnum);
```

---

## Example Queries

### Find all mobs with a specific flag

```sql
SELECT m.vnum, m.name_nom, m.level
FROM mobs m
JOIN mob_flags mf ON m.vnum = mf.mob_vnum
WHERE mf.flag_name = 'kAggressive'
ORDER BY m.level DESC;
```

### Find all objects with strength bonus

```sql
SELECT o.vnum, o.name_nom, oa.modifier AS str_bonus
FROM objects o
JOIN obj_applies oa ON o.vnum = oa.obj_vnum
WHERE oa.location = 'kStr'
AND oa.modifier > 0
ORDER BY oa.modifier DESC;
```

### Find rooms in a zone

```sql
SELECT r.vnum, r.name, r.sector
FROM rooms r
WHERE r.zone_vnum = 1
ORDER BY r.vnum;
```

### Find all exits from a room

```sql
SELECT direction, to_room, key_vnum
FROM room_exits
WHERE room_vnum = 100;
```

### Count mobs by level range

```sql
SELECT
    CASE
        WHEN level BETWEEN 1 AND 10 THEN '1-10'
        WHEN level BETWEEN 11 AND 20 THEN '11-20'
        WHEN level BETWEEN 21 AND 30 THEN '21-30'
        ELSE '31+'
    END AS level_range,
    COUNT(*) AS count
FROM mobs
GROUP BY level_range
ORDER BY level_range;
```

### Find mobs with specific skills

```sql
SELECT m.vnum, m.name_nom, ms.skill_name, ms.value
FROM mobs m
JOIN mob_skills ms ON m.vnum = ms.mob_vnum
WHERE ms.value > 100
ORDER BY ms.value DESC;
```

### Find zone reset commands for a zone

```sql
SELECT cmd_order, cmd_type, description
FROM v_zone_commands
WHERE zone_vnum = 1
ORDER BY cmd_order;
```

### Find triggers attached to rooms

```sql
SELECT r.vnum, r.name, t.vnum AS trigger_vnum, t.name AS trigger_name
FROM rooms r
JOIN entity_triggers et ON et.entity_type = 'room' AND et.entity_vnum = r.vnum
JOIN triggers t ON et.trigger_vnum = t.vnum
ORDER BY r.vnum;
```

### World statistics

```sql
SELECT * FROM v_world_stats;
```

---

## Conversion from Legacy Format

### Command

```bash
# Convert to SQLite
python3 tools/convert_to_yaml.py -i lib.template -o lib -f sqlite

# Custom database path
python3 tools/convert_to_yaml.py -i lib.template -o lib -f sqlite --db my_world.db

# With parallel parsing
python3 tools/convert_to_yaml.py -i lib.template -o lib -f sqlite -w 8
```

### Output

```
Converting 3 mob files (4 parsers, 1 writers)...
Converting 4 obj files (4 parsers, 1 writers)...
Converting 4 wld files (4 parsers, 1 writers)...
Converting 5 zon files (4 parsers, 1 writers)...
Converting 3 trg files (4 parsers, 1 writers)...

SQLite database statistics:
  Zones: 5
  Rooms: 103
  Mobs: 123
  Objects: 192
  Triggers: 64
  Zone commands: 27

Database saved to: lib/world.db
```

### Architecture

The converter uses a producer-consumer pattern:
- **Producers** (parallel): Parse input files and put entities into a queue
- **Consumer** (single for SQLite): Read from queue and save entities sequentially

This ensures database integrity while maximizing parsing performance.
