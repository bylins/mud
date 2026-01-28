-- SQLite schema for MUD world data
-- Normalized tables (3NF) with convenient views

-- Note: Foreign keys disabled during import to allow any insertion order
-- Enable with: PRAGMA foreign_keys = ON; after import is complete

-- ============================================================================
-- Reference tables (Enums)
-- ============================================================================

-- Object types
CREATE TABLE obj_types (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    description TEXT
);

-- Sector types
CREATE TABLE sectors (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    description TEXT
);

-- Position types
CREATE TABLE positions (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE
);

-- Gender types
CREATE TABLE genders (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE
);

-- Direction types
CREATE TABLE directions (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE  -- kNorth, kEast, kSouth, kWest, kUp, kDown
);

-- Skills
CREATE TABLE skills (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    description TEXT
);

-- Apply locations
CREATE TABLE apply_locations (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    description TEXT
);

-- Wear positions
CREATE TABLE wear_positions (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE
);

-- Trigger attach types
CREATE TABLE trigger_attach_types (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE  -- kMob, kObj, kRoom
);

-- Trigger type definitions (predefined set)
-- char_code is the original file character, bit_value is 1 << bit_position
CREATE TABLE trigger_type_defs (
    char_code TEXT PRIMARY KEY,  -- 'c', 'g', 'A', etc.
    name TEXT NOT NULL UNIQUE,   -- 'kCommand', 'kGreet', etc.
    bit_value INTEGER NOT NULL,  -- 4, 64, etc.
    description TEXT
);

-- ============================================================================
-- Main entity tables
-- ============================================================================

-- Zones
CREATE TABLE zones (
    vnum INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    -- Metadata
    comment TEXT,
    location TEXT,
    author TEXT,
    description TEXT,
    builders TEXT,
    -- Parameters
    first_room INTEGER,
    top_room INTEGER,
    mode INTEGER DEFAULT 0,
    zone_type INTEGER DEFAULT 0,
    zone_group INTEGER DEFAULT 1,
    entrance INTEGER,
    lifespan INTEGER DEFAULT 10,
    reset_mode INTEGER DEFAULT 2,
    reset_idle INTEGER DEFAULT 0,
    under_construction INTEGER DEFAULT 0,
    enabled INTEGER DEFAULT 1
);

-- Zone grouping (typeA/typeB)
CREATE TABLE zone_groups (
    zone_vnum INTEGER NOT NULL,
    linked_zone_vnum INTEGER NOT NULL,
    group_type TEXT NOT NULL CHECK(group_type IN ('A', 'B')),
    PRIMARY KEY (zone_vnum, linked_zone_vnum, group_type),
    FOREIGN KEY (zone_vnum) REFERENCES zones(vnum) ON DELETE CASCADE
);

-- Mobs
CREATE TABLE mobs (
    vnum INTEGER PRIMARY KEY,
    -- Names (6 Russian cases)
    aliases TEXT,
    name_nom TEXT,      -- nominative
    name_gen TEXT,      -- genitive
    name_dat TEXT,      -- dative
    name_acc TEXT,      -- accusative
    name_ins TEXT,      -- instrumental
    name_pre TEXT,      -- prepositional
    -- Descriptions
    short_desc TEXT,
    long_desc TEXT,
    -- Base parameters
    alignment INTEGER DEFAULT 0,
    mob_type TEXT DEFAULT 'S',  -- 'S' or 'E'
    -- Stats
    level INTEGER DEFAULT 1,
    hitroll_penalty INTEGER DEFAULT 0,
    armor INTEGER DEFAULT 100,
    -- HP dice
    hp_dice_count INTEGER DEFAULT 1,
    hp_dice_size INTEGER DEFAULT 1,
    hp_bonus INTEGER DEFAULT 0,
    -- Damage dice
    dam_dice_count INTEGER DEFAULT 1,
    dam_dice_size INTEGER DEFAULT 1,
    dam_bonus INTEGER DEFAULT 0,
    -- Gold dice
    gold_dice_count INTEGER DEFAULT 0,
    gold_dice_size INTEGER DEFAULT 0,
    gold_bonus INTEGER DEFAULT 0,
    -- Experience
    experience INTEGER DEFAULT 0,
    -- Position
    default_pos INTEGER REFERENCES positions(id),
    start_pos INTEGER REFERENCES positions(id),
    -- Appearance
    sex INTEGER REFERENCES genders(id),
    size INTEGER DEFAULT 50,
    height INTEGER DEFAULT 170,
    weight INTEGER DEFAULT 70,
    -- Class/Race
    mob_class INTEGER,
    race INTEGER,
    -- Attributes (E-spec)
    attr_str INTEGER DEFAULT 11,
    attr_dex INTEGER DEFAULT 11,
    attr_int INTEGER DEFAULT 11,
    attr_wis INTEGER DEFAULT 11,
    attr_con INTEGER DEFAULT 11,
    attr_cha INTEGER DEFAULT 11,
    -- Enhanced E-spec fields (COMPLETE support)
    attr_str_add INTEGER DEFAULT 0,
    hp_regen INTEGER DEFAULT 0,
    armour_bonus INTEGER DEFAULT 0,
    mana_regen INTEGER DEFAULT 0,
    cast_success INTEGER DEFAULT 0,
    morale INTEGER DEFAULT 0,
    initiative_add INTEGER DEFAULT 0,
    absorb INTEGER DEFAULT 0,
    aresist INTEGER DEFAULT 0,
    mresist INTEGER DEFAULT 0,
    presist INTEGER DEFAULT 0,
    bare_hand_attack INTEGER DEFAULT 0,
    like_work INTEGER DEFAULT 0,
    max_factor INTEGER DEFAULT 0,
    extra_attack INTEGER DEFAULT 0,
    mob_remort INTEGER DEFAULT 0,
    special_bitvector TEXT,
    role TEXT,
    enabled INTEGER DEFAULT 1
);

-- Mob flags (action_flags, affect_flags)
CREATE TABLE mob_flags (
    mob_vnum INTEGER NOT NULL,
    flag_category TEXT NOT NULL,  -- 'action' or 'affect'
    flag_name TEXT NOT NULL,
    PRIMARY KEY (mob_vnum, flag_category, flag_name),
    FOREIGN KEY (mob_vnum) REFERENCES mobs(vnum) ON DELETE CASCADE
);

-- Mob skills
CREATE TABLE mob_skills (
    mob_vnum INTEGER NOT NULL,
    skill_id INTEGER NOT NULL,
    value INTEGER NOT NULL,
    PRIMARY KEY (mob_vnum, skill_id),
    FOREIGN KEY (mob_vnum) REFERENCES mobs(vnum) ON DELETE CASCADE,
    FOREIGN KEY (skill_id) REFERENCES skills(id)
);


-- Enhanced mob array fields (separate tables)

-- Mob resistances (Resistances: # # # # # # # #)
CREATE TABLE mob_resistances (
    mob_vnum INTEGER NOT NULL,
    resist_type INTEGER NOT NULL,  -- 0-7+ index
    value INTEGER NOT NULL,
    PRIMARY KEY (mob_vnum, resist_type),
    FOREIGN KEY (mob_vnum) REFERENCES mobs(vnum) ON DELETE CASCADE
);

-- Mob saves (Saves: # # # #)
CREATE TABLE mob_saves (
    mob_vnum INTEGER NOT NULL,
    save_type INTEGER NOT NULL,  -- 0-3 (ESaving)
    value INTEGER NOT NULL,
    PRIMARY KEY (mob_vnum, save_type),
    FOREIGN KEY (mob_vnum) REFERENCES mobs(vnum) ON DELETE CASCADE
);

-- Mob feats (Feat: #)
CREATE TABLE mob_feats (
    mob_vnum INTEGER NOT NULL,
    feat_id INTEGER NOT NULL,
    PRIMARY KEY (mob_vnum, feat_id),
    FOREIGN KEY (mob_vnum) REFERENCES mobs(vnum) ON DELETE CASCADE
);

-- Mob spells (Spell: #)
CREATE TABLE mob_spells (
    mob_vnum INTEGER NOT NULL,
    spell_id INTEGER NOT NULL,
    count INTEGER DEFAULT 1,
    PRIMARY KEY (mob_vnum, spell_id),
    FOREIGN KEY (mob_vnum) REFERENCES mobs(vnum) ON DELETE CASCADE
);

-- Mob helpers (Helper: #)
CREATE TABLE mob_helpers (
    mob_vnum INTEGER NOT NULL,
    helper_vnum INTEGER NOT NULL,
    PRIMARY KEY (mob_vnum, helper_vnum),
    FOREIGN KEY (mob_vnum) REFERENCES mobs(vnum) ON DELETE CASCADE
);

-- Mob patrol destinations (Destination: #)
CREATE TABLE mob_destinations (
    mob_vnum INTEGER NOT NULL,
    dest_order INTEGER NOT NULL,
    room_vnum INTEGER NOT NULL,
    PRIMARY KEY (mob_vnum, dest_order),
    FOREIGN KEY (mob_vnum) REFERENCES mobs(vnum) ON DELETE CASCADE
);

-- Objects
CREATE TABLE objects (
    vnum INTEGER PRIMARY KEY,
    -- Names (6 Russian cases)
    aliases TEXT,
    name_nom TEXT,
    name_gen TEXT,
    name_dat TEXT,
    name_acc TEXT,
    name_ins TEXT,
    name_pre TEXT,
    -- Descriptions
    short_desc TEXT,
    action_desc TEXT,
    -- Type and material
    obj_type_id INTEGER REFERENCES obj_types(id),
    material INTEGER,
    -- Values (interpretation depends on type)
    value0 TEXT,
    value1 TEXT,
    value2 TEXT,
    value3 TEXT,
    -- Physical properties
    weight INTEGER DEFAULT 0,
    cost INTEGER DEFAULT 0,
    rent_off INTEGER DEFAULT 0,
    rent_on INTEGER DEFAULT 0,
    spec_param INTEGER DEFAULT 0,
    max_durability INTEGER DEFAULT -1,
    cur_durability INTEGER DEFAULT -1,
    timer INTEGER DEFAULT -1,
    spell INTEGER DEFAULT -1,
    level INTEGER DEFAULT 0,
    sex INTEGER DEFAULT 0,
    max_in_world INTEGER DEFAULT -1,
    minimum_remorts INTEGER DEFAULT 0,
    enabled INTEGER DEFAULT 1
);

-- Object flags
CREATE TABLE obj_flags (
    obj_vnum INTEGER NOT NULL,
    flag_category TEXT NOT NULL,  -- 'extra' or 'wear'
    flag_name TEXT NOT NULL,
    PRIMARY KEY (obj_vnum, flag_category, flag_name),
    FOREIGN KEY (obj_vnum) REFERENCES objects(vnum) ON DELETE CASCADE
);

-- Object applies
CREATE TABLE obj_applies (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    obj_vnum INTEGER NOT NULL,
    location_id INTEGER NOT NULL REFERENCES apply_locations(id),
    modifier INTEGER NOT NULL,
    FOREIGN KEY (obj_vnum) REFERENCES objects(vnum) ON DELETE CASCADE
);

-- Rooms
CREATE TABLE rooms (
    vnum INTEGER PRIMARY KEY,
    zone_vnum INTEGER REFERENCES zones(vnum),
    name TEXT,
    description TEXT,
    sector_id INTEGER REFERENCES sectors(id),
    enabled INTEGER DEFAULT 1
);

-- Room flags
CREATE TABLE room_flags (
    room_vnum INTEGER NOT NULL,
    flag_name TEXT NOT NULL,
    PRIMARY KEY (room_vnum, flag_name),
    FOREIGN KEY (room_vnum) REFERENCES rooms(vnum) ON DELETE CASCADE
);

-- Room exits
CREATE TABLE room_exits (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    room_vnum INTEGER NOT NULL,
    direction_id INTEGER NOT NULL REFERENCES directions(id),
    description TEXT,
    keywords TEXT,
    exit_flags TEXT,
    key_vnum INTEGER DEFAULT -1,
    to_room INTEGER DEFAULT -1,
    lock_complexity INTEGER DEFAULT 0,
    UNIQUE(room_vnum, direction_id),
    FOREIGN KEY (room_vnum) REFERENCES rooms(vnum) ON DELETE CASCADE
);

-- Triggers
CREATE TABLE triggers (
    vnum INTEGER PRIMARY KEY,
    name TEXT,
    attach_type_id INTEGER REFERENCES trigger_attach_types(id),
    narg INTEGER DEFAULT 0,
    arglist TEXT,
    script TEXT,
    enabled INTEGER DEFAULT 1
);

-- Trigger type bindings (many-to-many: trigger can have multiple types)
CREATE TABLE trigger_type_bindings (
    trigger_vnum INTEGER NOT NULL,
    type_char TEXT NOT NULL,
    PRIMARY KEY (trigger_vnum, type_char),
    FOREIGN KEY (trigger_vnum) REFERENCES triggers(vnum) ON DELETE CASCADE,
    FOREIGN KEY (type_char) REFERENCES trigger_type_defs(char_code)
);

-- Extra descriptions (shared for objects and rooms)
CREATE TABLE extra_descriptions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    entity_type TEXT NOT NULL,  -- 'obj' or 'room'
    entity_vnum INTEGER NOT NULL,
    keywords TEXT NOT NULL,
    description TEXT
);

-- Trigger bindings to entities
CREATE TABLE entity_triggers (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    entity_type TEXT NOT NULL,  -- 'mob', 'obj', 'room'
    entity_vnum INTEGER NOT NULL,
    trigger_vnum INTEGER NOT NULL,
    trigger_order INTEGER DEFAULT 0,
    FOREIGN KEY (trigger_vnum) REFERENCES triggers(vnum)
);

-- Zone commands
CREATE TABLE zone_commands (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    zone_vnum INTEGER NOT NULL,
    cmd_order INTEGER NOT NULL,  -- execution order
    cmd_type TEXT NOT NULL,      -- LOAD_MOB, LOAD_OBJ, GIVE_OBJ, EQUIP_MOB, PUT_OBJ, DOOR, REMOVE_OBJ, FOLLOW, TRIGGER, VARIABLE, EXTRACT_MOB
    if_flag INTEGER DEFAULT 0,
    -- Universal fields (used depending on cmd_type)
    arg_mob_vnum INTEGER,
    arg_obj_vnum INTEGER,
    arg_room_vnum INTEGER,
    arg_trigger_vnum INTEGER,
    arg_container_vnum INTEGER,
    arg_max INTEGER,
    arg_max_world INTEGER,
    arg_max_room INTEGER,
    arg_load_prob INTEGER,
    arg_wear_pos_id INTEGER REFERENCES wear_positions(id),
    arg_direction_id INTEGER REFERENCES directions(id),
    arg_state INTEGER,
    arg_trigger_type TEXT,
    arg_context INTEGER,
    arg_var_name TEXT,
    arg_var_value TEXT,
    arg_leader_mob_vnum INTEGER,
    arg_follower_mob_vnum INTEGER,
    FOREIGN KEY (zone_vnum) REFERENCES zones(vnum) ON DELETE CASCADE
);

-- ============================================================================
-- Indexes for performance
-- ============================================================================

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
CREATE INDEX idx_trigger_type_bindings ON trigger_type_bindings(trigger_vnum);

-- ============================================================================
-- Views for convenient browsing
-- ============================================================================

-- Full mob information
CREATE VIEW v_mobs AS
SELECT
    m.vnum,
    m.name_nom AS name,
    m.aliases,
    m.short_desc,
    m.level,
    m.alignment,
    m.default_pos,
    m.sex,
    m.hp_dice_count || 'd' || m.hp_dice_size || '+' || m.hp_bonus AS hp_dice,
    m.dam_dice_count || 'd' || m.dam_dice_size || '+' || m.dam_bonus AS damage_dice,
    m.experience,
    m.mob_class,
    m.race,
    m.attr_str, m.attr_dex, m.attr_int, m.attr_wis, m.attr_con, m.attr_cha,
    GROUP_CONCAT(DISTINCT CASE WHEN mf.flag_category = 'action' THEN mf.flag_name END) AS action_flags,
    GROUP_CONCAT(DISTINCT CASE WHEN mf.flag_category = 'affect' THEN mf.flag_name END) AS affect_flags
FROM mobs m
LEFT JOIN mob_flags mf ON m.vnum = mf.mob_vnum
GROUP BY m.vnum;

-- Mob skills
CREATE VIEW v_mob_skills AS
SELECT
    ms.mob_vnum,
    m.name_nom AS mob_name,
    s.name AS skill_name,
    ms.value AS skill_value
FROM mob_skills ms
JOIN mobs m ON ms.mob_vnum = m.vnum
JOIN skills s ON ms.skill_id = s.id
ORDER BY ms.mob_vnum, s.name;

-- Full object information
CREATE VIEW v_objects AS
SELECT
    o.vnum,
    o.name_nom AS name,
    o.aliases,
    o.short_desc,
    ot.name AS obj_type,
    o.weight,
    o.cost,
    o.level,
    o.max_durability,
    o.timer,
    o.max_in_world,
    GROUP_CONCAT(DISTINCT CASE WHEN of.flag_category = 'extra' THEN of.flag_name END) AS extra_flags,
    GROUP_CONCAT(DISTINCT CASE WHEN of.flag_category = 'wear' THEN of.flag_name END) AS wear_flags
FROM objects o
LEFT JOIN obj_types ot ON o.obj_type_id = ot.id
LEFT JOIN obj_flags of ON o.vnum = of.obj_vnum
GROUP BY o.vnum;

-- Object applies
CREATE VIEW v_obj_applies AS
SELECT
    oa.obj_vnum,
    o.name_nom AS obj_name,
    al.name AS apply_location,
    oa.modifier
FROM obj_applies oa
JOIN objects o ON oa.obj_vnum = o.vnum
JOIN apply_locations al ON oa.location_id = al.id
ORDER BY oa.obj_vnum, al.name;

-- Full room information
CREATE VIEW v_rooms AS
SELECT
    r.vnum,
    r.name,
    r.zone_vnum,
    z.name AS zone_name,
    s.name AS sector,
    r.description,
    GROUP_CONCAT(DISTINCT rf.flag_name) AS room_flags
FROM rooms r
LEFT JOIN zones z ON r.zone_vnum = z.vnum
LEFT JOIN sectors s ON r.sector_id = s.id
LEFT JOIN room_flags rf ON r.vnum = rf.room_vnum
GROUP BY r.vnum;

-- Room exits
CREATE VIEW v_room_exits AS
SELECT
    re.room_vnum,
    r.name AS room_name,
    d.name AS direction,
    re.to_room,
    r2.name AS destination_name,
    re.exit_flags,
    re.key_vnum,
    re.lock_complexity
FROM room_exits re
JOIN rooms r ON re.room_vnum = r.vnum
JOIN directions d ON re.direction_id = d.id
LEFT JOIN rooms r2 ON re.to_room = r2.vnum
ORDER BY re.room_vnum, d.id;

-- Zone information
CREATE VIEW v_zones AS
SELECT
    z.vnum,
    z.name,
    z.author,
    z.location,
    z.first_room,
    z.top_room,
    z.lifespan,
    z.reset_mode,
    (SELECT COUNT(*) FROM rooms WHERE zone_vnum = z.vnum) AS room_count,
    GROUP_CONCAT(DISTINCT CASE WHEN zg.group_type = 'A' THEN zg.linked_zone_vnum END) AS typeA_zones,
    GROUP_CONCAT(DISTINCT CASE WHEN zg.group_type = 'B' THEN zg.linked_zone_vnum END) AS typeB_zones
FROM zones z
LEFT JOIN zone_groups zg ON z.vnum = zg.zone_vnum
GROUP BY z.vnum;

-- Zone commands (human-readable format)
CREATE VIEW v_zone_commands AS
SELECT
    zc.zone_vnum,
    z.name AS zone_name,
    zc.cmd_order,
    zc.cmd_type,
    zc.if_flag,
    CASE zc.cmd_type
        WHEN 'LOAD_MOB' THEN 'Load mob ' || zc.arg_mob_vnum || ' in room ' || zc.arg_room_vnum || ' (max ' || zc.arg_max_world || '/' || zc.arg_max_room || ')'
        WHEN 'LOAD_OBJ' THEN 'Load obj ' || zc.arg_obj_vnum || ' in room ' || zc.arg_room_vnum || ' (max ' || zc.arg_max || ', prob ' || zc.arg_load_prob || '%)'
        WHEN 'GIVE_OBJ' THEN 'Give obj ' || zc.arg_obj_vnum || ' to last mob'
        WHEN 'EQUIP_MOB' THEN 'Equip obj ' || zc.arg_obj_vnum || ' on last mob at pos ' || wp.name
        WHEN 'PUT_OBJ' THEN 'Put obj ' || zc.arg_obj_vnum || ' in container ' || zc.arg_container_vnum
        WHEN 'DOOR' THEN 'Set door in room ' || zc.arg_room_vnum || ' dir ' || dir.name || ' to state ' || zc.arg_state
        WHEN 'FOLLOW' THEN 'Mob ' || zc.arg_follower_mob_vnum || ' follows ' || zc.arg_leader_mob_vnum || ' in room ' || zc.arg_room_vnum
        WHEN 'TRIGGER' THEN 'Attach trigger ' || zc.arg_trigger_vnum || ' to entity'
        ELSE zc.cmd_type
    END AS description
FROM zone_commands zc
JOIN zones z ON zc.zone_vnum = z.vnum
LEFT JOIN wear_positions wp ON zc.arg_wear_pos_id = wp.id
LEFT JOIN directions dir ON zc.arg_direction_id = dir.id
ORDER BY zc.zone_vnum, zc.cmd_order;

-- Triggers
CREATE VIEW v_triggers AS
SELECT
    t.vnum,
    t.name,
    tat.name AS attach_type,
    t.narg,
    t.arglist,
    GROUP_CONCAT(ttb.type_char, '') AS type_chars,
    GROUP_CONCAT(ttd.name) AS trigger_types,
    LENGTH(t.script) AS script_length
FROM triggers t
LEFT JOIN trigger_attach_types tat ON t.attach_type_id = tat.id
LEFT JOIN trigger_type_bindings ttb ON t.vnum = ttb.trigger_vnum
LEFT JOIN trigger_type_defs ttd ON ttb.type_char = ttd.char_code
GROUP BY t.vnum;

-- Triggers attached to entities
CREATE VIEW v_entity_triggers AS
SELECT
    et.entity_type,
    et.entity_vnum,
    CASE et.entity_type
        WHEN 'mob' THEN m.name_nom
        WHEN 'obj' THEN o.name_nom
        WHEN 'room' THEN r.name
    END AS entity_name,
    et.trigger_vnum,
    t.name AS trigger_name
FROM entity_triggers et
LEFT JOIN mobs m ON et.entity_type = 'mob' AND et.entity_vnum = m.vnum
LEFT JOIN objects o ON et.entity_type = 'obj' AND et.entity_vnum = o.vnum
LEFT JOIN rooms r ON et.entity_type = 'room' AND et.entity_vnum = r.vnum
JOIN triggers t ON et.trigger_vnum = t.vnum
ORDER BY et.entity_type, et.entity_vnum;

-- World statistics
CREATE VIEW v_world_stats AS
SELECT
    (SELECT COUNT(*) FROM zones) AS total_zones,
    (SELECT COUNT(*) FROM rooms) AS total_rooms,
    (SELECT COUNT(*) FROM mobs) AS total_mobs,
    (SELECT COUNT(*) FROM objects) AS total_objects,
    (SELECT COUNT(*) FROM triggers) AS total_triggers,
    (SELECT COUNT(*) FROM zone_commands) AS total_zone_commands;

