# SQLite World Schema Documentation

This document describes the SQLite database schema used to store MUD world data. The schema is normalized to Third Normal Form (3NF) and includes convenient views for browsing.

SQLite is an optional world format. YAML is the default build format; the SQLite backend is compiled in with `-Dsqlite=builtin` or `-Dsqlite=system` (`-Dsqlite=auto` picks it up when `libsqlite3` is present). See "World Data Formats and Testing" in `CLAUDE.md`.

## Table of Contents

- [Where the Schema Lives](#where-the-schema-lives)
- [How the Engine Uses world.db](#how-the-engine-uses-worlddb)
- [General Principles](#general-principles)
- [Entity-Relationship Diagram](#entity-relationship-diagram)
- [Reference Tables](#reference-tables)
- [Main Entity Tables](#main-entity-tables)
- [Linking Tables](#linking-tables)
- [Engine-Only Tables](#engine-only-tables)
- [Views](#views)
- [Indexes](#indexes)
- [Example Queries](#example-queries)
- [Conversion from Legacy Format](#conversion-from-legacy-format)

---

## Where the Schema Lives

The schema is defined in two places that are meant to be kept identical:

| Source | Used by |
|--------|---------|
| `tools/converter/world_schema.sql` | Python converter (`tools/converter/convert_to_yaml.py --format sqlite`) |
| `kCoreSchemaDdl` in `src/engine/db/sqlite_world_data_source_freshness.cpp` | Engine, when it opens a `world.db` that has no `zones` table yet (`EnsureCoreSchema()`) |

The C++ literal is documented as a verbatim copy of the `.sql` file, so an engine-bootstrapped `world.db` has the same shape as a converter-built one. Known differences between the two:

- **`v_triggers`**: the C++ copy also selects `t.add_flag`; the converter's `world_schema.sql` does not.
- **Reference table contents**: the converter fills the reference tables (`obj_types`, `sectors`, `positions`, ...) right after creating the schema. The engine only runs the DDL and leaves them empty. The engine's own loader never joins on them, but views that `JOIN` a reference table (`v_mob_skills`, `v_obj_applies`, `v_obj_skills`, `v_room_exits`) return no rows on an engine-built database, and the `LEFT JOIN` name columns in the other views are `NULL`.

On top of the shared schema, the engine creates a few extra tables on demand (see [Engine-Only Tables](#engine-only-tables)).

---

## How the Engine Uses world.db

- The engine opens `worlddata/world.db`, relative to the world data directory passed with `-d` (`src/engine/db/db.cpp`).
- With no `<world_loader>` section in `cfg/configuration.xml`, the compile-time default is used: YAML if compiled in, otherwise SQLite, otherwise Legacy.
- With a `<world_loader><sources>` section listing more than one backend, the engine builds a composite source. The order is an authority ranking (`SelectByHierarchy` in `src/engine/db/composite_world_data_source.cpp`): the first source wins every disagreement, whichever is numerically fresher, and a later source wins a zone only when its freshness matches every source before it. Loaded zones are written back into the sources that disagree with the winner. The recommended order is `<yaml/>` then `<sqlite/>`: YAML stays the source of truth and `world.db` is a cache that wins once it is in sync. With `<sqlite/>` first, an empty or stale `world.db` would outrank the YAML world. Freshness is tracked in `zone_sync` and `world_meta`.
- `-F <kind>` forces one source of a composite configuration, which is useful when a `world.db` copied from another machine should rebuild the YAML tree regardless of mtimes.
- Rows with `enabled = 0` in `zones`, `rooms`, `mobs`, `objects` and `triggers` are skipped by the loader.

---

## General Principles

### Normalization

The schema is normalized to 3NF to eliminate redundancy:

- **1NF**: All columns contain atomic values (no comma-separated lists in main tables)
- **2NF**: All non-key columns depend on the entire primary key
- **3NF**: No transitive dependencies (flags and skills are in separate tables)

Some columns still store packed values as text: `objects.value0`..`value3`, `room_exits.exit_flags`, `mobs.special_bitvector`, `mobs.role` and `obj_extra_values.vals`.

### Foreign Keys

The schema declares foreign keys (several with `ON DELETE CASCADE`), but they are not enforced:

- The converter runs `PRAGMA foreign_keys = OFF` before creating the schema and importing data, so rows can be inserted in any order.
- The engine never turns `PRAGMA foreign_keys` on. SQLite's default is off, so the declared `CASCADE` clauses never fire, and the engine deletes child rows explicitly when it resaves an entity.

To check integrity by hand:

```sql
PRAGMA foreign_keys = ON;
PRAGMA foreign_key_check;
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
┌─────────────────┐       ┌─────────────────┐
│      zones      │◄──────│  zone_groups    │
│ vnum (PK)       │       │ zone_vnum (FK)  │
└────────┬────────┘       └─────────────────┘
         │
         ├──────────────────────────┐
         │ 1:N                      │ 1:N
         ▼                          ▼
┌─────────────────┐       ┌─────────────────┐
│      rooms      │       │  zone_commands  │
│ vnum (PK)       │       │ zone_vnum (FK)  │
│ zone_vnum (FK)  │       └─────────────────┘
└────────┬────────┘
         │ 1:N            ┌─────────────────┐
         ├───────────────►│   room_exits    │
         │                │ room_vnum (FK)  │
         │                │ to_room         │
         │                └─────────────────┘
         │ 1:N            ┌─────────────────┐
         └───────────────►│   room_flags    │
                          │ room_vnum (FK)  │
                          └─────────────────┘

┌─────────────────┐       ┌──────────────────────────────────────┐
│      mobs       │◄──────│ mob_flags, mob_skills,               │
│ vnum (PK)       │       │ mob_resistances, mob_saves,          │
└─────────────────┘       │ mob_feats, mob_spells, mob_helpers,  │
                          │ mob_destinations, mob_death_load     │
                          │ (all keyed by mob_vnum)              │
                          └──────────────────────────────────────┘

┌─────────────────┐       ┌──────────────────────────────────────┐
│    objects      │◄──────│ obj_flags, obj_applies, obj_skills,  │
│ vnum (PK)       │       │ obj_extra_values                     │
└─────────────────┘       │ (all keyed by obj_vnum)              │
                          └──────────────────────────────────────┘

┌─────────────────┐       ┌─────────────────────┐
│    triggers     │◄──────│ trigger_type_bindings│
│ vnum (PK)       │       │ trigger_vnum (FK)   │
└─────────────────┘       └─────────────────────┘
         ▲
         │                ┌─────────────────┐
         └────────────────│ entity_triggers │
                          │ trigger_vnum(FK)│
                          │ entity_type     │
                          │ entity_vnum     │
                          └─────────────────┘

extra_descriptions: (entity_type, entity_vnum) -> objects or rooms (no declared FK)
```

---

## Reference Tables

Reference tables store enum-like values. The converter populates them right after creating the schema; the engine creates them but leaves them empty (see [Where the Schema Lives](#where-the-schema-lives)). The converter fills only `id` and `name`; the `description` columns stay `NULL`.

### obj_types

Object type constants.

| Column | Type | Description |
|--------|------|-------------|
| id | INTEGER PK | Type ID |
| name | TEXT NOT NULL UNIQUE | Type name (e.g., kWeapon) |
| description | TEXT | Human-readable description |

### sectors

Room sector types.

| Column | Type | Description |
|--------|------|-------------|
| id | INTEGER PK | Sector ID |
| name | TEXT NOT NULL UNIQUE | Sector name (e.g., kInside) |
| description | TEXT | Human-readable description |

### positions

Character position types.

| Column | Type | Description |
|--------|------|-------------|
| id | INTEGER PK | Position ID |
| name | TEXT NOT NULL UNIQUE | Position name (e.g., kStanding) |

### genders

Gender types.

| Column | Type | Description |
|--------|------|-------------|
| id | INTEGER PK | Gender ID |
| name | TEXT NOT NULL UNIQUE | Gender name (kMale, kFemale, kNeutral) |

### directions

Exit direction types.

| Column | Type | Description |
|--------|------|-------------|
| id | INTEGER PK | Direction ID (0-5) |
| name | TEXT NOT NULL UNIQUE | Direction name (kNorth, kEast, kSouth, kWest, kUp, kDown) |

### skills

Skill definitions.

| Column | Type | Description |
|--------|------|-------------|
| id | INTEGER PK | Skill ID |
| name | TEXT NOT NULL UNIQUE | Skill name (e.g., kPunch) |
| description | TEXT | Skill description |

### apply_locations

Apply location types for object modifiers.

| Column | Type | Description |
|--------|------|-------------|
| id | INTEGER PK | Apply ID |
| name | TEXT NOT NULL UNIQUE | Apply name (e.g., kStr, kDex) |
| description | TEXT | Apply description |

### wear_positions

Equipment wear positions.

| Column | Type | Description |
|--------|------|-------------|
| id | INTEGER PK | Position ID |
| name | TEXT NOT NULL UNIQUE | Position name |

### trigger_attach_types

Trigger attachment types.

| Column | Type | Description |
|--------|------|-------------|
| id | INTEGER PK | Type ID |
| name | TEXT NOT NULL UNIQUE | Type name (kMob, kObj, kRoom) |

### trigger_type_defs

Trigger type character definitions for normalized trigger type storage.

| Column | Type | Description |
|--------|------|-------------|
| char_code | TEXT PK | Single character code as in the legacy file (`'c'`, `'g'`, `'A'`, ...) |
| name | TEXT NOT NULL UNIQUE | Type name (e.g., kCommand, kGreet) |
| bit_value | INTEGER NOT NULL | Bitmask value, `1 << bit_position` |
| description | TEXT | Description |

Lowercase letters (a-z) map to bits 0-25, uppercase (A-Z) to bits 26-51. `bit_value` stores the mask itself, not the bit position (e.g., `'c'` → 4).

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
| under_construction | INTEGER | 0 | Zone is under construction |
| enabled | INTEGER | 1 | 0 = skipped by the loader |

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
| default_pos | INTEGER FK | | Default position → positions.id |
| start_pos | INTEGER FK | | Starting position → positions.id |
| sex | INTEGER FK | | Gender → genders.id |
| speed | INTEGER | -1 | Movement speed (-1 = default cadence) |
| size | INTEGER | 50 | Size |
| height | INTEGER | 170 | Height |
| weight | INTEGER | 70 | Weight |
| mob_class | INTEGER | | Mob class ID |
| race | INTEGER | | Race ID |
| attr_str | INTEGER | 11 | Strength (E-type) |
| attr_dex | INTEGER | 11 | Dexterity |
| attr_int | INTEGER | 11 | Intelligence |
| attr_wis | INTEGER | 11 | Wisdom |
| attr_con | INTEGER | 11 | Constitution |
| attr_cha | INTEGER | 11 | Charisma |
| attr_str_add | INTEGER | 0 | Strength add |
| hp_regen | INTEGER | 0 | HP regeneration |
| armour_bonus | INTEGER | 0 | Armour bonus |
| mana_regen | INTEGER | 0 | Mana regeneration |
| cast_success | INTEGER | 0 | Cast success |
| morale | INTEGER | 0 | Morale |
| initiative_add | INTEGER | 0 | Initiative bonus |
| absorb | INTEGER | 0 | Absorb |
| aresist | INTEGER | 0 | Affect resist |
| mresist | INTEGER | 0 | Magic resist |
| presist | INTEGER | 0 | Physical resist |
| bare_hand_attack | INTEGER | 0 | Bare-hand attack type |
| like_work | INTEGER | 0 | LikeWork |
| max_factor | INTEGER | 0 | MaxFactor |
| extra_attack | INTEGER | 0 | Extra attacks |
| mob_remort | INTEGER | 0 | Mob remort count |
| special_bitvector | TEXT | | Special flags (packed FlagData) |
| role | TEXT | | Mob role (bitset as text) |
| enabled | INTEGER | 1 | 0 = skipped by the loader |

The fields from `attr_str_add` to `mob_remort` are the extended (E-spec) fields.

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
| max_durability | INTEGER | -1 | Max durability |
| cur_durability | INTEGER | -1 | Current durability |
| timer | INTEGER | -1 | Timer (-1 = none) |
| spell | INTEGER | -1 | Spell ID |
| level | INTEGER | 0 | Required level |
| sex | INTEGER | 0 | Grammatical gender of the object name |
| max_in_world | INTEGER | -1 | Max instances (-1 = unlimited) |
| minimum_remorts | INTEGER | 0 | Minimum remorts required |
| enabled | INTEGER | 1 | 0 = skipped by the loader |

### rooms

Room definitions with descriptions and sector type.

| Column | Type | Default | Description |
|--------|------|---------|-------------|
| vnum | INTEGER PK | | Room VNUM |
| zone_vnum | INTEGER FK | | Parent zone VNUM → zones.vnum |
| name | TEXT | | Room name |
| description | TEXT | | Room description |
| sector_id | INTEGER FK | | Sector type → sectors.id |
| enabled | INTEGER | 1 | 0 = skipped by the loader |

### triggers

DG Script trigger definitions.

| Column | Type | Default | Description |
|--------|------|---------|-------------|
| vnum | INTEGER PK | | Trigger VNUM |
| name | TEXT | | Trigger name |
| attach_type_id | INTEGER FK | | Attach type → trigger_attach_types.id |
| narg | INTEGER | 0 | Numeric argument |
| arglist | TEXT | | Argument list |
| add_flag | INTEGER | 0 | Add flag |
| script | TEXT | | Script code |
| enabled | INTEGER | 1 | 0 = skipped by the loader |

Trigger types are not stored here; see `trigger_type_bindings`.

---

## Linking Tables

All `*_vnum` foreign keys to the owning entity below are declared `ON DELETE CASCADE` unless noted otherwise (not enforced, see [Foreign Keys](#foreign-keys)).

### zone_groups

Zone grouping for typeA/typeB lists.

| Column | Type | Description |
|--------|------|-------------|
| zone_vnum | INTEGER NOT NULL FK | Zone VNUM → zones.vnum |
| linked_zone_vnum | INTEGER NOT NULL | Linked zone VNUM |
| group_type | TEXT NOT NULL | 'A' or 'B' (`CHECK` constraint) |

Primary Key: (zone_vnum, linked_zone_vnum, group_type)

### zone_commands

Zone reset commands.

| Column | Type | Default | Description |
|--------|------|---------|-------------|
| id | INTEGER PK AUTOINCREMENT | | Row ID |
| zone_vnum | INTEGER NOT NULL FK | | Zone VNUM → zones.vnum |
| cmd_order | INTEGER NOT NULL | | Execution order |
| cmd_type | TEXT NOT NULL | | Command type (see below) |
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

`cmd_type` values: `LOAD_MOB`, `LOAD_OBJ`, `GIVE_OBJ`, `EQUIP_MOB`, `PUT_OBJ`, `DOOR`, `REMOVE_OBJ`, `FOLLOW`, `TRIGGER`, `VARIABLE`, `EXTRACT_MOB`. The engine also writes and reads `DISABLED` for a disabled command.

`VARIABLE` (zone command `V`) keeps the room vnum in `arg_room_vnum` (only for a room trigger, `arg_trigger_type` = 2) and the variable context in `arg_context`.

### mob_flags

Mob action and affect flags.

| Column | Type | Description |
|--------|------|-------------|
| mob_vnum | INTEGER NOT NULL FK | Mob VNUM |
| flag_category | TEXT NOT NULL | 'action' or 'affect' |
| flag_name | TEXT NOT NULL | Flag name (e.g., kIsNpc) |

Primary Key: (mob_vnum, flag_category, flag_name)

### mob_skills

Mob skill values.

| Column | Type | Description |
|--------|------|-------------|
| mob_vnum | INTEGER NOT NULL FK | Mob VNUM |
| skill_id | INTEGER NOT NULL FK | Skill ID → skills.id (no cascade) |
| value | INTEGER NOT NULL | Skill value |

Primary Key: (mob_vnum, skill_id)

### mob_resistances

| Column | Type | Description |
|--------|------|-------------|
| mob_vnum | INTEGER NOT NULL FK | Mob VNUM |
| resist_type | INTEGER NOT NULL | Resistance index (0-7+) |
| value | INTEGER NOT NULL | Value |

Primary Key: (mob_vnum, resist_type)

### mob_saves

| Column | Type | Description |
|--------|------|-------------|
| mob_vnum | INTEGER NOT NULL FK | Mob VNUM |
| save_type | INTEGER NOT NULL | Saving throw index (`ESaving`, 0-3) |
| value | INTEGER NOT NULL | Value |

Primary Key: (mob_vnum, save_type)

### mob_feats

| Column | Type | Description |
|--------|------|-------------|
| mob_vnum | INTEGER NOT NULL FK | Mob VNUM |
| feat_id | INTEGER NOT NULL | Feat ID |

Primary Key: (mob_vnum, feat_id)

### mob_spells

| Column | Type | Default | Description |
|--------|------|---------|-------------|
| mob_vnum | INTEGER NOT NULL FK | | Mob VNUM |
| spell_id | INTEGER NOT NULL | | Spell ID |
| count | INTEGER | 1 | Number of charges |

Primary Key: (mob_vnum, spell_id)

### mob_helpers

| Column | Type | Description |
|--------|------|-------------|
| mob_vnum | INTEGER NOT NULL FK | Mob VNUM |
| helper_order | INTEGER NOT NULL | Position in the helper list |
| helper_vnum | INTEGER NOT NULL | Helper mob VNUM |

Primary Key: (mob_vnum, helper_order). The key uses the order, not the helper VNUM, so the same helper can appear more than once (weighted random selection).

### mob_destinations

Patrol destinations.

| Column | Type | Description |
|--------|------|-------------|
| mob_vnum | INTEGER NOT NULL FK | Mob VNUM |
| dest_order | INTEGER NOT NULL | Position in the route |
| room_vnum | INTEGER NOT NULL | Destination room VNUM |

Primary Key: (mob_vnum, dest_order)

### obj_flags

Object flags by category.

| Column | Type | Description |
|--------|------|-------------|
| obj_vnum | INTEGER NOT NULL FK | Object VNUM |
| flag_category | TEXT NOT NULL | 'extra', 'wear', 'affect', 'no', or 'anti' |
| flag_name | TEXT NOT NULL | Flag name |

Primary Key: (obj_vnum, flag_category, flag_name)

### obj_applies

Object stat modifiers.

| Column | Type | Description |
|--------|------|-------------|
| id | INTEGER PK AUTOINCREMENT | Row ID |
| obj_vnum | INTEGER NOT NULL FK | Object VNUM |
| location_id | INTEGER NOT NULL FK | Apply location → apply_locations.id |
| modifier | INTEGER NOT NULL | Modifier value |

### obj_skills

Skills granted by an object (legacy `S` line).

| Column | Type | Description |
|--------|------|-------------|
| obj_vnum | INTEGER NOT NULL FK | Object VNUM |
| skill_id | INTEGER NOT NULL FK | Skill ID → skills.id (no cascade) |
| value | INTEGER NOT NULL | Skill value |

Primary Key: (obj_vnum, skill_id)

### room_flags

Room flags.

| Column | Type | Description |
|--------|------|-------------|
| room_vnum | INTEGER NOT NULL FK | Room VNUM |
| flag_name | TEXT NOT NULL | Flag name |

Primary Key: (room_vnum, flag_name)

### room_exits

Room exit definitions.

| Column | Type | Default | Description |
|--------|------|---------|-------------|
| id | INTEGER PK AUTOINCREMENT | | Row ID |
| room_vnum | INTEGER NOT NULL FK | | Source room VNUM |
| direction_id | INTEGER NOT NULL FK | | Direction → directions.id |
| description | TEXT | | Exit description |
| keywords | TEXT | | Door keywords |
| exit_flags | TEXT | | Exit flags (numeric bitmask stored as text) |
| key_vnum | INTEGER | -1 | Key VNUM |
| to_room | INTEGER | -1 | Destination room VNUM |
| lock_complexity | INTEGER | 0 | Lock complexity |

Unique: (room_vnum, direction_id)

### trigger_type_bindings

Normalized trigger type assignments (many-to-many between triggers and type chars).

| Column | Type | Description |
|--------|------|-------------|
| trigger_vnum | INTEGER NOT NULL FK | Trigger VNUM |
| type_char | TEXT NOT NULL FK | Type character → trigger_type_defs.char_code (no cascade) |

Primary Key: (trigger_vnum, type_char)

### extra_descriptions

Extra descriptions for objects and rooms. No foreign key is declared.

| Column | Type | Description |
|--------|------|-------------|
| id | INTEGER PK AUTOINCREMENT | Row ID |
| entity_type | TEXT NOT NULL | 'obj' or 'room' |
| entity_vnum | INTEGER NOT NULL | Entity VNUM |
| keywords | TEXT NOT NULL | Keywords |
| description | TEXT | Description |

### entity_triggers

Trigger bindings to mobs, objects, or rooms.

| Column | Type | Default | Description |
|--------|------|---------|-------------|
| id | INTEGER PK AUTOINCREMENT | | Row ID |
| entity_type | TEXT NOT NULL | | 'mob', 'obj', or 'room' |
| entity_vnum | INTEGER NOT NULL | | Entity VNUM |
| trigger_vnum | INTEGER NOT NULL FK | | Trigger VNUM → triggers.vnum (no cascade) |
| trigger_order | INTEGER | 0 | Attachment order; the engine loads bindings `ORDER BY trigger_order` |

There is no unique constraint: the same trigger may be bound to an entity more than once.

---

## Engine-Only Tables

These tables are not in `world_schema.sql`. The engine creates them with `CREATE TABLE IF NOT EXISTS`, so a converter-built `world.db` gets them on the first save or freshness check.

### zone_sync

Created by `EnsureSyncTables()` (`sqlite_world_data_source_freshness.cpp`). Per-zone freshness stamp used by the composite loader.

| Column | Type | Description |
|--------|------|-------------|
| zone_vnum | INTEGER PK | Zone VNUM |
| sync_mtime | INTEGER NOT NULL | Freshness stamp of the zone's last sync |

A database without these rows reports freshness 0 until the first sync fills them in.

### world_meta

Created by `EnsureSyncTables()`. World-level key/value stamps.

| Column | Type | Description |
|--------|------|-------------|
| key | TEXT PK | Key (currently `zone_index_mtime`) |
| value | INTEGER NOT NULL | Value |

### mob_death_load

Created on demand by `SaveMobs` (`sqlite_world_data_source.cpp`). Objects loaded into a mob's corpse on death. The converter never writes this table.

| Column | Type | Default | Description |
|--------|------|---------|-------------|
| mob_vnum | INTEGER NOT NULL | | Mob VNUM |
| load_order | INTEGER NOT NULL | | Position in the list |
| obj_vnum | INTEGER NOT NULL | | Object VNUM |
| load_prob | INTEGER NOT NULL | 0 | Load probability |
| load_type | INTEGER NOT NULL | 0 | Load type |
| spec_param | INTEGER NOT NULL | 0 | Special parameter |

Primary Key: (mob_vnum, load_order). No foreign key is declared.

### obj_extra_values

Created on demand by `SaveObjects`. Extra (V-line) object values, which have no column in the converter schema.

| Column | Type | Description |
|--------|------|-------------|
| obj_vnum | INTEGER PK | Object VNUM |
| vals | TEXT NOT NULL | Packed extra values |

---

## Views

Views provide convenient access to joined data. Views that `INNER JOIN` a reference table return rows only when the reference tables are populated (converter-built databases).

### v_mobs

Full mob information with aggregated flags.

```sql
SELECT vnum, name, aliases, short_desc, level, alignment,
       default_pos, sex, hp_dice, damage_dice, experience,
       mob_class, race,
       attr_str, attr_dex, attr_int, attr_wis, attr_con, attr_cha,
       action_flags, affect_flags
FROM v_mobs;
```

### v_mob_skills

Mob skills with mob names (joins `skills`).

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

Object applies with object names (joins `apply_locations`).

```sql
SELECT obj_vnum, obj_name, apply_location, modifier
FROM v_obj_applies;
```

### v_obj_skills

Object skills with object names (joins `skills`).

```sql
SELECT obj_vnum, obj_name, skill_name, skill_value
FROM v_obj_skills;
```

### v_rooms

Full room information with zone name and flags.

```sql
SELECT vnum, name, zone_vnum, zone_name, sector,
       description, room_flags
FROM v_rooms;
```

### v_room_exits

Room exits with room and destination names (joins `directions`).

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
       type_chars, trigger_types, script_length
FROM v_triggers;
```

The engine-created view also has an `add_flag` column (between `arglist` and `type_chars`); the converter-created view does not.

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
CREATE INDEX idx_obj_skills_vnum ON obj_skills(obj_vnum);
CREATE INDEX idx_room_flags_vnum ON room_flags(room_vnum);
CREATE INDEX idx_room_exits_vnum ON room_exits(room_vnum);
CREATE INDEX idx_room_exits_to ON room_exits(to_room);
CREATE INDEX idx_extra_desc_entity ON extra_descriptions(entity_type, entity_vnum);
CREATE INDEX idx_entity_triggers ON entity_triggers(entity_type, entity_vnum);
CREATE INDEX idx_zone_commands_zone ON zone_commands(zone_vnum, cmd_order);
CREATE INDEX idx_rooms_zone ON rooms(zone_vnum);
CREATE INDEX idx_trigger_type_bindings ON trigger_type_bindings(trigger_vnum);
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
JOIN apply_locations al ON oa.location_id = al.id
WHERE al.name = 'kStr'
AND oa.modifier > 0
ORDER BY oa.modifier DESC;
```

(Needs populated reference tables; on an engine-built database filter on `oa.location_id` instead.)

### Find rooms in a zone

```sql
SELECT r.vnum, r.name, r.sector_id
FROM rooms r
WHERE r.zone_vnum = 1
ORDER BY r.vnum;
```

### Find all exits from a room

```sql
SELECT direction_id, to_room, key_vnum
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
SELECT m.vnum, m.name_nom, ms.skill_id, ms.value
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
ORDER BY r.vnum, et.trigger_order;
```

### World statistics

```sql
SELECT * FROM v_world_stats;
```

---

## Conversion from Legacy Format

There are two ways to get a `world.db`:

- **From a YAML world, by the engine.** Configure a composite `<world_loader>` with `<sqlite/>` and `<yaml/>` sources in `cfg/configuration.xml` and boot a binary built with both backends. On the first boot the engine creates `worlddata/world.db` from the built-in DDL and fills it from YAML. This is what `tools/run_load_tests.sh` does for its SQLite round-trip test (see `tools/TESTING.md`). A one-shot resave is also possible: `circle -d <data_root> -S <path/to/world.db> -T sqlite` boots the world and writes it into the given database file (the path is relative to the data root).
- **From a legacy world, by the converter.** Legacy worlds now exist only as full-world archives; `lib.template/` ships YAML only.

### Converter command

The converter is `tools/converter/convert_to_yaml.py`. `--input` is a directory that contains a legacy `world/` subdirectory (`wld/`, `mob/`, `obj/`, `zon/`, `trg/`). Without `--db`, the database is written to `<output>/world.db`. The engine reads `worlddata/world.db` under its data root, so either point `--output` at `<data_root>/worlddata` or pass `--db` explicitly:

```bash
# Convert to SQLite
./tools/converter/convert_to_yaml.py -i <legacy_dir> -o <data_root>/worlddata -f sqlite

# Custom database path
./tools/converter/convert_to_yaml.py -i <legacy_dir> -o <data_root> -f sqlite --db <data_root>/worlddata/world.db

# Parallel parsing (default: CPU count)
./tools/converter/convert_to_yaml.py -i <legacy_dir> -o <data_root>/worlddata -f sqlite -w 8
```

Other options: `--type {mob,obj,wld,zon,trg,all}` (default `all`) and `--delete-source`. Run with `--help` for the full list.

### Output

At the end the converter prints the counts from `v_world_stats` and the database path:

```
SQLite database statistics:
  Zones: ...
  Rooms: ...
  Mobs: ...
  Objects: ...
  Triggers: ...
  Zone commands: ...

Database saved to: <path>/world.db
```

### Architecture

The converter uses a producer-consumer pattern:
- **Producers** (parallel): Parse input files and put entities into a queue
- **Consumer** (single for SQLite): Read from queue and save entities sequentially

This ensures database integrity while maximizing parsing performance.
