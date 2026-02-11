#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Convert old MUD world format files to YAML or SQLite format.

Architecture:
============

    ┌─────────────────────────────────────────────────────────────────────────┐
    │                    PARALLEL PARSING, SEQUENTIAL SAVING                  │
    ├─────────────────────────────────────────────────────────────────────────┤
    │                                                                         │
    │   PARSING (ThreadPoolExecutor)                SAVING (Main Thread)      │
    │   ┌─────────────────────────────┐            ┌────────────────────┐    │
    │   │  Parser Thread 1            │            │                    │    │
    │   │  parse_file() -> entities   │──┐         │  saver.save_*()    │    │
    │   └─────────────────────────────┘  │         │                    │    │
    │   ┌─────────────────────────────┐  │         │  Sequential due to │    │
    │   │  Parser Thread 2            │──┼─────────│  GIL (YAML) and    │    │
    │   │  parse_file() -> entities   │  │         │  DB safety (SQLite)│    │
    │   └─────────────────────────────┘  │         │                    │    │
    │   ┌─────────────────────────────┐  │         │                    │    │
    │   │  Parser Thread N            │──┘         │                    │    │
    │   │  parse_file() -> entities   │            │                    │    │
    │   └─────────────────────────────┘            └────────────────────┘    │
    │                                                                         │
    └─────────────────────────────────────────────────────────────────────────┘

    Note: Python's GIL prevents parallel execution of CPU-bound code (like YAML
    serialization). Multi-threaded YAML writing provides no speedup and adds
    overhead. Parallelism is only beneficial for I/O-bound parsing.

YAML Libraries:
    - ruamel.yaml (default): Full comment support, literal blocks (|), proper formatting
    - pyyaml: Fast (~3x faster), but limited output (no comments, no literal blocks)

Output Formats:
    - YAML: One file per entity (vnum.yaml), with index.yaml per directory
    - SQLite: Single normalized database with views for convenient queries

Usage:
    python3 convert_to_yaml.py -i lib.template -o lib                    # YAML (ruamel, default)
    python3 convert_to_yaml.py -i lib.template -o lib --yaml-lib pyyaml  # YAML (fast, limited)
    python3 convert_to_yaml.py -i lib.template -o lib -f sqlite          # SQLite database
"""

import argparse
import os
import re
import sqlite3
import sys
import threading
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path
from io import StringIO
# Queue removed - no longer needed (sequential saving)

# YAML libraries - lazy import
_pyyaml = None       # PyYAML module (fast, no comments)
_ruamel_yaml = None  # ruamel.yaml YAML class
_ruamel_initialized = False

# Thread-safe logging counters
_counter_lock = threading.Lock()
_warnings_count = 0
_errors_count = 0

# YAML library selection: 'ruamel' (with comments) or 'pyyaml' (fast, ~3x faster)
# Note: Both are single-threaded due to GIL - parallelism is only in parsing
# Using ruamel for proper LiteralScalarString (| blocks) support
_yaml_library = 'ruamel'

# Literal blocks: ruamel automatically uses | format for multi-line strings
# When enabled: CR+LF converted to LF, world_config.yaml gets line_endings=dos
_use_literal_blocks = False  # Will be set to True when ruamel is used


def _init_yaml_libraries():
    """Lazily import YAML libraries."""
    global _pyyaml, _ruamel_yaml, _ruamel_initialized

    # Always need ruamel for CommentedMap/CommentedSeq in to_yaml functions
    if not _ruamel_initialized:
        from ruamel.yaml import YAML
        from ruamel.yaml.comments import CommentedMap, CommentedSeq
        from ruamel.yaml.scalarstring import LiteralScalarString
        # Inject into module globals for use by to_yaml functions
        globals()['YAML'] = YAML
        globals()['CommentedMap'] = CommentedMap
        globals()['CommentedSeq'] = CommentedSeq
        globals()['LiteralScalarString'] = LiteralScalarString
        _ruamel_yaml = YAML
        _ruamel_initialized = True

    # Import pyyaml only if needed
    if _yaml_library == 'pyyaml' and _pyyaml is None:
        import yaml
        _pyyaml = yaml


def log_warning(message, vnum=None, filepath=None):
    """Log a warning message without stack trace (thread-safe)."""
    global _warnings_count
    with _counter_lock:
        _warnings_count += 1
    context = []
    if filepath:
        context.append(f"file={filepath}")
    if vnum is not None:
        context.append(f"vnum={vnum}")
    ctx_str = f" [{', '.join(context)}]" if context else ""
    print(f"WARNING{ctx_str}: {message}", file=sys.stderr)


def log_error(message, vnum=None, filepath=None):
    """Log an error message without stack trace (thread-safe)."""
    global _errors_count
    with _counter_lock:
        _errors_count += 1
    context = []
    if filepath:
        context.append(f"file={filepath}")
    if vnum is not None:
        context.append(f"vnum={vnum}")
    ctx_str = f" [{', '.join(context)}]" if context else ""
    print(f"ERROR{ctx_str}: {message}", file=sys.stderr)


def print_summary():
    """Print conversion summary."""
    if _warnings_count > 0 or _errors_count > 0:
        print(f"\nConversion summary: {_errors_count} errors, {_warnings_count} warnings", file=sys.stderr)


# Thread-local YAML handler for thread-safe dumping (ruamel.yaml)
_thread_local = threading.local()


def get_yaml():
    """Get thread-local ruamel.yaml YAML instance."""
    if not hasattr(_thread_local, 'yaml'):
        y = _ruamel_yaml()
        y.default_flow_style = False
        y.allow_unicode = True
        y.width = 4096  # Prevent line wrapping
        y.preserve_quotes = True
        _thread_local.yaml = y
    return _thread_local.yaml


def to_literal_block(text):
    """Convert text for YAML output.

    If literal blocks are disabled (pyyaml):
        - Returns text as-is
        - YAML will write quoted strings with \\r\\n escape sequences
        - No conversion needed

    If literal blocks are enabled (ruamel):
        - Converts CR+LF to LF
        - Wraps in LiteralScalarString for | block formatting
        - Loader will convert LF back to CR+LF (line_endings=dos)
    """
    if not text or not _use_literal_blocks:
        return text

    # Check for actual CR+LF bytes
    if '\r\n' in text:
        text = text.replace('\r\n', '\n')
        return LiteralScalarString(text)

    return text


def _convert_to_plain(obj):
    """Recursively convert CommentedMap/CommentedSeq to plain dict/list."""
    if isinstance(obj, dict):
        return {k: _convert_to_plain(v) for k, v in obj.items()}
    elif isinstance(obj, list):
        return [_convert_to_plain(v) for v in obj]
    elif hasattr(obj, '__class__') and obj.__class__.__name__ == 'LiteralScalarString':
        # Convert LiteralScalarString to plain string for PyYAML compatibility
        return str(obj)
    else:
        return obj


def yaml_dump_to_string(data):
    """Dump YAML data to string (thread-safe).

    Uses ruamel.yaml (with comments) or PyYAML (fast, no comments) based on _yaml_library.
    """
    if _yaml_library == 'pyyaml':
        # PyYAML: fast but no comment support
        # Add header comment manually
        header = ""
        if hasattr(data, 'ca') and data.ca.comment and data.ca.comment[1]:
            # Extract start comment from ruamel CommentedMap
            for comment in data.ca.comment[1]:
                if comment and hasattr(comment, 'value'):
                    comment_text = comment.value.strip()
                    # Comment already includes # prefix
                    if comment_text.startswith('#'):
                        header = f"{comment_text}\n"
                    else:
                        header = f"# {comment_text}\n"
                    break
        # Convert to plain dict/list recursively for PyYAML
        plain_data = _convert_to_plain(data)
        return header + _pyyaml.dump(plain_data, allow_unicode=True, default_flow_style=False,
                                      sort_keys=False, width=4096)
    else:
        # ruamel.yaml: slower but preserves comments
        stream = StringIO()
        get_yaml().dump(data, stream)
        return stream.getvalue()


# Global YAML instance for main thread operations (index files) - lazy init
_main_yaml = None


def get_main_yaml():
    """Get the main thread YAML instance (for index files)."""
    global _main_yaml
    if _main_yaml is None:
        _main_yaml = _ruamel_yaml()
        _main_yaml.default_flow_style = False
        _main_yaml.allow_unicode = True
        _main_yaml.width = 4096
        _main_yaml.preserve_quotes = True
    return _main_yaml

# Global name registries for cross-references
ROOM_NAMES = {}    # vnum -> name
MOB_NAMES = {}     # vnum -> name
OBJ_NAMES = {}     # vnum -> name
TRIGGER_NAMES = {} # vnum -> name
ZONE_NAMES = {}    # vnum -> name

# Spell names (spell_id -> Russian name)
SPELL_NAMES = {
    1: "доспехи",
    2: "телепортация",
    3: "благословение",
    4: "слепота",
    5: "горящие руки",
    6: "молния",
    7: "очарование",
    8: "ледяное прикосновение",
    9: "клонирование",
    10: "ледяные стрелы",
    11: "изменение погоды",
    12: "сотворение еды",
    13: "сотворение воды",
    14: "снять слепоту",
    15: "тяжелое лечение",
    16: "легкое лечение",
    17: "проклятие",
    18: "распознать мировоззрение",
    19: "видеть невидимое",
    20: "обнаружить магию",
    21: "обнаружить яд",
    22: "изгнание зла",
    23: "землетрясение",
    24: "зачаровать оружие",
    25: "вытягивание жизни",
    26: "огненный шар",
    27: "вред",
    28: "исцеление",
    29: "невидимость",
    30: "молния",
    31: "найти предмет",
    32: "волшебная стрела",
    33: "яд",
    34: "защита от зла",
    35: "снять проклятие",
    36: "святость",
    37: "шоковая хватка",
    38: "сон",
    39: "сила",
    40: "призвать",
    41: "покровительство",
    42: "слово возврата",
    43: "снять яд",
    44: "чувство жизни",
    45: "оживить мертвых",
    46: "изгнание добра",
    47: "групповые доспехи",
    48: "групповое исцеление",
    49: "групповое возвращение",
    50: "инфравидение",
    51: "хождение по воде",
    52: "среднее лечение",
    53: "групповая сила",
    54: "удержание",
    55: "сильное удержание",
    56: "массовое удержание",
    57: "полет",
    58: "разорванные цепи",
    59: "запрет бегства",
    60: "сотворение света",
    61: "тьма",
    62: "каменная кожа",
    63: "затуманивание",
    64: "молчание",
    65: "свет",
    66: "цепная молния",
    67: "огненный взрыв",
    68: "гнев богов",
    69: "слабость",
    70: "групповая невидимость",
    71: "теневой плащ",
    72: "кислота",
    73: "починка",
    74: "увеличение",
    75: "страх",
    76: "жертвоприношение",
    77: "паутина",
    78: "мерцание",
    79: "снять удержание",
    80: "маскировка",
}

# Material names (material_id -> Russian name)
MATERIAL_NAMES = {
    0: "бронза",
    1: "железо",
    2: "сталь",
    3: "булат",
    4: "кристалл",
    5: "дерево",
    6: "слоновая кость",
    7: "обсидиан",
    8: "кость",
    9: "кожа",
    10: "ткань",
    11: "адамантит",
    12: "драгоценный камень",
    13: "камень",
    14: "золото",
    15: "серебро",
    16: "платина",
    17: "митрил",
    18: "алмаз",
    19: "глина",
    20: "стекло",
}

# Action flags (MOB_x) - from constants.cpp action_bits[]
ACTION_FLAGS = [
    "kSpec", "kSentinel", "kScavenger", "kIsNpc", "kAware", "kAggressive",
    "kStayZone", "kWimpy", "kAggressive_Mob", "kMemory", "kHelper", "kNoCharm",
    "kNoSummoned", "kNoSleep", "kNoBash", "kNoBlind", "kNoTrack", "kFireCreature",
    "kAirCreature", "kWaterCreature", "kEarthCreature", "kRacing", "kMounting",
    "kProtected", "kSwimming", "kFlying", "kScrStay", "kNoTerrainAttack",
    "kNoFear", "kNoMagicTerrainAttack",
    # Plane 1
    "kFreemaker", "kProgrammedLootGroup", "kIgnoresFear", "kClone", "kCorpse",
    "kLooting", "kTutelar", "kMentalShadow", "kSummoner", "kNoSilence",
    "kNoHolder", "kDeleted"
]

# Room flags (room_bits[])
# Each plane has 30 bits, so:
#   Plane 0: indices 0-29
#   Plane 1: indices 30-59
#   Plane 2: indices 60+
ROOM_FLAGS = [
    # Plane 0 (bits 0-29)
    "kDarked", "kDeathTrap", "kNoEntryMob", "kIndoors", "kPeaceful", "kSoundproof",
    "kNoTrack", "kNoMagic", "kTunnel", "kNoTeleportIn", "kGodsRoom", "kHouse",
    "kHouseCrash", "kHouseEntry", "UNUSED_14", "kBfsMark", "kForMages", "kForSorcerers",
    "kForThieves", "kForWarriors", "kForAssasines", "kForGuards", "kForPaladines", "kForRangers",
    "kForPoly", "kForMono", "kForge", "kForMerchants", "kForMaguses", "kArena",
    # Plane 1 (bits 0-29, only 0-12 used)
    "kNoSummonOut", "kNoTeleportOut", "kNohorse", "kNoWeather", "kSlowDeathTrap", "kIceTrap",
    "kNoRelocateIn", "kTribune", "kArenaSend", "kNoBattle", "UNUSED_40", "kAlwaysLit", "kMoMapper",
    "UNUSED_43", "UNUSED_44", "UNUSED_45", "UNUSED_46", "UNUSED_47", "UNUSED_48", "UNUSED_49",
    "UNUSED_50", "UNUSED_51", "UNUSED_52", "UNUSED_53", "UNUSED_54", "UNUSED_55", "UNUSED_56",
    "UNUSED_57", "UNUSED_58", "UNUSED_59",
    # Plane 2 (bits 0-29, only 0-1 used)
    "kNoItem", "kDominationArena"
]

# Object types (item_types[])
OBJ_TYPES = [
    "kItemUndefined",   # 0
    "kLightSource",     # 1
    "kScroll",          # 2
    "kWand",            # 3
    "kStaff",           # 4
    "kWeapon",          # 5
    "kElementWeapon",   # 6
    "kMissile",         # 7
    "kTreasure",        # 8
    "kArmor",           # 9
    "kPotion",          # 10
    "kWorm",            # 11
    "kOther",           # 12
    "kTrash",           # 13
    "kTrap",            # 14
    "kContainer",       # 15
    "kNote",            # 16
    "kLiquidContainer", # 17
    "kKey",             # 18
    "kFood",            # 19
    "kMoney",           # 20
    "kPen",             # 21
    "kBoat",            # 22
    "kFountain",        # 23
    "kBook",            # 24
    "kIngredient",      # 25
    "kMagicIngredient", # 26
    "kCraftMaterial",   # 27
    "kBandage",         # 28
    "kLightArmor",      # 29
    "kMediumArmor",     # 30
    "kHeavyArmor",      # 31
    "kEnchant",         # 32
    "kMagicMaterial",   # 33
    "kMagicArrow",      # 34
    "kMagicContaner",   # 35
    "kCraftMaterial2"   # 36
]

# Extra flags (EObjFlag) - 50 flags total (30 in plane 0, 20 in plane 1)
EXTRA_FLAGS = [
    # Plane 0 (0-29)
    "kGlow", "kHum", "kNorent", "kNodonate", "kNoinvis", "kInvisible",
    "kMagic", "kNodrop", "kBless", "kNosell", "kDecay", "kZonedecay",
    "kNodisarm", "kNodecay", "kPoisoned", "kSharpen", "kArmored",
    "kAppearsDay", "kAppearsNight", "kAppearsFullmoon", "kAppearsWinter",
    "kAppearsSpring", "kAppearsSummer", "kAppearsAutumn", "kSwimming",
    "kFlying", "kThrowing", "kTicktimer", "kFire", "kRepopDecay",
    # Plane 1 (30-49)
    "kNolocate", "kTimedLvl", "kNoalter", "kHasOneSlot", "kHasTwoSlots",
    "kHasThreeSlots", "kSetItem", "kNofail", "kNamed", "kBloody",
    "kQuestItem", "k2inlaid", "k3inlaid", "kNopour", "kUnique",
    "kTransformed", "kNoRentTimer", "kLimitedTimer", "kBindOnPurchase",
    "kNotOneInClanChest",
    # Padding for plane 1 remainder (50-59)
    "UNUSED_50", "UNUSED_51", "UNUSED_52", "UNUSED_53", "UNUSED_54",
    "UNUSED_55", "UNUSED_56", "UNUSED_57", "UNUSED_58", "UNUSED_59",
    # Padding for plane 2 (60-89)
    "UNUSED_60", "UNUSED_61", "UNUSED_62", "UNUSED_63", "UNUSED_64",
    "UNUSED_65", "UNUSED_66", "UNUSED_67", "UNUSED_68", "UNUSED_69",
    "UNUSED_70", "UNUSED_71", "UNUSED_72", "UNUSED_73", "UNUSED_74",
    "UNUSED_75", "UNUSED_76", "UNUSED_77", "UNUSED_78", "UNUSED_79",
    "UNUSED_80", "UNUSED_81", "UNUSED_82", "UNUSED_83", "UNUSED_84",
    "UNUSED_85", "UNUSED_86", "UNUSED_87", "UNUSED_88", "UNUSED_89",
    # Padding for plane 3 (90-99) - rarely used
    "UNUSED_90", "UNUSED_91", "UNUSED_92", "UNUSED_93", "UNUSED_94",
    "UNUSED_95", "UNUSED_96", "UNUSED_97", "UNUSED_98", "UNUSED_99"
]

# Wear flags (wear_bits[])
WEAR_FLAGS = [
    "kTake", "kFinger", "kNeck", "kBody", "kHead", "kLegs", "kFeet",
    "kHands", "kArms", "kShield", "kShoulders", "kWaist", "kWrist",
    "kWield", "kHold", "kBoth", "kQuiver",
    # UNUSED bits 17-29 (used in some old files)
    "UNUSED_17", "UNUSED_18", "UNUSED_19", "UNUSED_20", "UNUSED_21",
    "UNUSED_22", "UNUSED_23", "UNUSED_24", "UNUSED_25", "UNUSED_26",
    "UNUSED_27", "UNUSED_28", "UNUSED_29"
]

# No flags (ENoFlag) - restrictions by class/other criteria
NO_FLAGS = [
    # Plane 0 (0-18)
    "kMono", "kPoly", "kNeutral", "kMage", "kSorcerer", "kThief",
    "kWarrior", "kAssasine", "kGuard", "kPaladine", "kRanger", "kVigilant",
    "kMerchant", "kMagus", "kConjurer", "kCharmer", "kWizard",
    "kNecromancer", "kFighter",
    # Padding for plane 0 remainder (19-29) - unnamed but used in files
    "UNUSED_19", "UNUSED_20", "UNUSED_21", "UNUSED_22", "UNUSED_23",
    "UNUSED_24", "UNUSED_25", "UNUSED_26", "UNUSED_27", "UNUSED_28", "UNUSED_29",
    # Plane 1 (30-59)
    "kKiller", "kColored", "kBattle",
    "UNUSED_33", "UNUSED_34", "UNUSED_35", "UNUSED_36", "UNUSED_37", "UNUSED_38",
    "UNUSED_39", "UNUSED_40", "UNUSED_41", "UNUSED_42", "UNUSED_43", "UNUSED_44",
    "UNUSED_45", "UNUSED_46", "UNUSED_47", "UNUSED_48", "UNUSED_49", "UNUSED_50",
    "UNUSED_51", "UNUSED_52", "UNUSED_53", "UNUSED_54", "UNUSED_55", "UNUSED_56",
    "UNUSED_57", "UNUSED_58", "UNUSED_59",
    # Plane 2 (60-89)
    "UNUSED_60", "UNUSED_61", "UNUSED_62", "UNUSED_63", "UNUSED_64", "UNUSED_65",
    "kMale", "kFemale", "kCharmice",
    "UNUSED_69", "UNUSED_70", "UNUSED_71", "UNUSED_72", "UNUSED_73", "UNUSED_74",
    "UNUSED_75", "UNUSED_76", "UNUSED_77", "UNUSED_78", "UNUSED_79", "UNUSED_80",
    "UNUSED_81", "UNUSED_82", "UNUSED_83", "UNUSED_84", "UNUSED_85", "UNUSED_86",
    "UNUSED_87", "UNUSED_88", "UNUSED_89",
    # Plane 3 (90-99) - UNUSED but found in files
    "UNUSED_90", "UNUSED_91", "UNUSED_92", "UNUSED_93", "UNUSED_94",
    "UNUSED_95", "UNUSED_96", "UNUSED_97", "UNUSED_98", "UNUSED_99"
]

# Anti flags (EAntiFlag) - same structure as NO_FLAGS
ANTI_FLAGS = NO_FLAGS.copy()

# Object affect flags (EWeaponAffect)
# Plane 0: bits 0-29, Plane 1: bits 0-16 (indices 30-46)
AFFECT_FLAGS = [
    "kBlindness",           # 0
    "kInvisibility",        # 1
    "kDetectAlign",         # 2
    "kDetectInvisibility",  # 3
    "kDetectMagic",         # 4
    "kDetectLife",          # 5
    "kWaterWalk",           # 6
    "kSanctuary",           # 7
    "kCurse",               # 8
    "kInfravision",         # 9
    "kPoison",              # 10
    "kProtectFromDark",     # 11
    "kProtectFromMind",     # 12
    "kSleep",               # 13
    "kNoTrack",             # 14
    "kBless",               # 15
    "kSneak",               # 16
    "kHide",                # 17
    "kHold",                # 18
    "kFly",                 # 19
    "kSilence",             # 20
    "kAwareness",           # 21
    "kBlink",               # 22
    "kNoFlee",              # 23
    "kSingleLight",         # 24
    "kHolyLight",           # 25
    "kHolyDark",            # 26
    "kDetectPoison",        # 27
    "kSlow",                # 28
    "kHaste",               # 29
    # Plane 1: bits 0-16 (30-46)
    "kWaterBreath",         # 30
    "kHaemorrhage",         # 31
    "kDisguising",          # 32
    "kShield",              # 33
    "kAirShield",           # 34
    "kFireShield",          # 35
    "kIceShield",           # 36
    "kMagicGlass",          # 37
    "kStoneHand",           # 38
    "kPrismaticAura",       # 39
    "kAirAura",             # 40
    "kFireAura",            # 41
    "kIceAura",             # 42
    "kDeafness",            # 43
    "kComamnder",           # 44
    "kEarthAura",           # 45
    "kCloudly",             # 46
    # Padding for plane 1 remainder (47-59)
    "UNUSED_47", "UNUSED_48", "UNUSED_49", "UNUSED_50", "UNUSED_51", "UNUSED_52",
    "UNUSED_53", "UNUSED_54", "UNUSED_55", "UNUSED_56", "UNUSED_57", "UNUSED_58", "UNUSED_59",
    # Padding for plane 2 (60-89)
    "UNUSED_60", "UNUSED_61", "UNUSED_62", "UNUSED_63", "UNUSED_64", "UNUSED_65",
    "UNUSED_66", "UNUSED_67", "UNUSED_68", "UNUSED_69", "UNUSED_70", "UNUSED_71",
    "UNUSED_72", "UNUSED_73", "UNUSED_74", "UNUSED_75", "UNUSED_76", "UNUSED_77",
    "UNUSED_78", "UNUSED_79", "UNUSED_80", "UNUSED_81", "UNUSED_82", "UNUSED_83",
    "UNUSED_84", "UNUSED_85", "UNUSED_86", "UNUSED_87", "UNUSED_88", "UNUSED_89",
    # Plane 3 (90+) - rarely used but exists in some files
    "UNUSED_90", "UNUSED_91", "UNUSED_92", "UNUSED_93", "UNUSED_94", "UNUSED_95",
    "UNUSED_96", "UNUSED_97", "UNUSED_98", "UNUSED_99",
]

# Genders
GENDERS = ["kNeutral", "kMale", "kFemale", "kPoly"]

# Positions (must match EPosition enum constant names exactly)
POSITIONS = [
    "kDead", "kPerish", "kIncap", "kStun", "kSleep",
    "kRest", "kSit", "kFight", "kStand"
]

# Sector types (WARNING: indices 14-19 are unused gap, weather sectors start at 20)
SECTORS = [
    # 0-13: Base sectors
    "kInside", "kCity", "kField", "kForest", "kHills", "kMountain",
    "kWaterSwim", "kWaterNoswim", "kOnlyFlying", "kUnderwater", "kSecret",
    "kStoneroad", "kRoad", "kWildroad",
    # 14-19: GAP (unused in enum, must preserve for correct indexing)
    None, None, None, None, None, None,
    # 20-29: Weather-related sectors
    "kFieldSnow", "kFieldRain", "kForestSnow", "kForestRain",
    "kHillsSnow", "kHillsRain", "kMountainSnow", "kThinIce",
    "kNormalIce", "kThickIce"
]

# Trigger types
TRIGGER_TYPES = {
    # Mob triggers (lowercase a-z = bits 0-25)
    'a': 'kRandomGlobal',  # bit 0
    'b': 'kRandom',        # bit 1
    'c': 'kCommand',       # bit 2
    'd': 'kSpeech',        # bit 3
    'e': 'kAct',           # bit 4
    'f': 'kDeath',         # bit 5
    'g': 'kGreet',         # bit 6
    'h': 'kGreetAll',      # bit 7
    'i': 'kEntry',         # bit 8
    'j': 'kReceive',       # bit 9
    'k': 'kFight',         # bit 10
    'l': 'kHitPercent',    # bit 11
    'm': 'kBribe',         # bit 12
    'n': 'kLoad',          # bit 13
    'o': 'kKill',          # bit 14
    'p': 'kDamage',        # bit 15
    'q': 'kGreetPC',       # bit 16
    'r': 'kGreetPCAll',    # bit 17
    's': 'kIncome',        # bit 18
    't': 'kIncomePC',      # bit 19
    'u': 'kMobTrig20',     # bit 20
    'v': 'kMobTrig21',     # bit 21
    'w': 'kMobTrig22',     # bit 22
    'x': 'kMobTrig23',     # bit 23
    'y': 'kMobTrig24',     # bit 24
    'z': 'kAuto',          # bit 25
}


def numeric_flags_to_letters(n):
    """Convert numeric trigger_type to letter flags (same as asciiflag_conv inverse)."""
    result = []
    for i in range(26):
        if n & (1 << i):
            result.append(chr(ord('a') + i))
    for i in range(26):
        if n & (1 << (26 + i)):
            result.append(chr(ord('A') + i))
    return ''.join(result)

# Attach types
ATTACH_TYPES = {0: 'kMobTrigger', 1: 'kObjTrigger', 2: 'kRoomTrigger'}

# Skill names from ESkill enum (for mob skills comments)
SKILL_NAMES = {
    1: 'kProtect', 2: 'kIntercept', 3: 'kLeftHit', 4: 'kHammer',
    5: 'kOverwhelm', 6: 'kPoisoning', 7: 'kSense', 8: 'kRiding',
    9: 'kHideTrack', 11: 'kSkinning', 12: 'kMultiparry', 13: 'kReforging',
    20: 'kLeadership', 21: 'kPunctual', 22: 'kAwake', 23: 'kIdentify',
    24: 'kHearing', 25: 'kCreatePotion', 26: 'kCreateScroll', 27: 'kCreateWand',
    28: 'kPry', 29: 'kArmoring', 30: 'kHangovering', 31: 'kFirstAid',
    32: 'kCampfire', 33: 'kCreateBow', 34: 'kSlay', 35: 'kFrenzy',
    128: 'kShieldBash', 129: 'kCutting', 130: 'kThrow', 131: 'kBackstab',
    132: 'kBash', 133: 'kHide', 134: 'kKick', 135: 'kPickLock',
    136: 'kPunch', 137: 'kRescue', 138: 'kSneak', 139: 'kSteal',
    140: 'kTrack', 141: 'kClubs', 142: 'kAxes', 143: 'kLongBlades',
    144: 'kShortBlades', 145: 'kNonstandart', 146: 'kTwohands', 147: 'kPicks',
    148: 'kSpades', 149: 'kSideAttack', 150: 'kDisarm', 151: 'kParry',
    152: 'kCharge', 153: 'kMorph', 154: 'kBows', 155: 'kAddshot',
    156: 'kDisguise', 157: 'kDodge', 158: 'kShieldBlock', 159: 'kLooking',
    160: 'kChopoff', 161: 'kRepair', 162: 'kDazzle', 163: 'kThrowout',
    164: 'kSharpening', 165: 'kCourage', 166: 'kJinx', 167: 'kNoParryHit',
    168: 'kTownportal', 169: 'kMakeStaff', 170: 'kMakeBow', 171: 'kMakeWeapon',
    172: 'kMakeArmor', 173: 'kMakeJewel', 174: 'kMakeWear', 175: 'kMakePotion',
    176: 'kDigging', 177: 'kJewelry', 178: 'kWarcry', 179: 'kTurnUndead',
    180: 'kIronwind', 181: 'kStrangle', 182: 'kAirMagic', 183: 'kFireMagic',
    184: 'kWaterMagic', 185: 'kEarthMagic', 186: 'kLightMagic', 187: 'kDarkMagic',
    188: 'kMindMagic', 189: 'kLifeMagic', 190: 'kStun', 191: 'kMakeAmulet'
}

# Apply locations (for object applies comments)
APPLY_LOCATIONS = {
    0: 'kNone', 1: 'kStr', 2: 'kDex', 3: 'kInt', 4: 'kWis', 5: 'kCon', 6: 'kCha',
    7: 'kClass', 8: 'kLvl', 9: 'kAge', 10: 'kWeight', 11: 'kHeight',
    12: 'kManaRegen', 13: 'kHp', 14: 'kMove', 15: 'kGold',
    17: 'kAc', 18: 'kHitroll', 19: 'kDamroll', 20: 'kSavingWill',
    21: 'kResistFire', 22: 'kResistAir', 23: 'kSavingCritical', 24: 'kSavingStability',
    25: 'kHpRegen', 26: 'kMoveRegen', 27: 'kFirstCircle', 28: 'kSecondCircle',
    29: 'kThirdCircle', 30: 'kFourthCircle', 31: 'kFifthCircle', 32: 'kSixthCircle',
    33: 'kSeventhCircle', 34: 'kEighthCircle', 35: 'kNinthCircle',
    36: 'kSize', 37: 'kArmour', 38: 'kPoison', 39: 'kSavingReflex',
    40: 'kCastSuccess', 41: 'kMorale', 42: 'kInitiative', 43: 'kReligion',
    44: 'kAbsorbe', 45: 'kLikes', 46: 'kResistWater', 47: 'kResistEarth',
    48: 'kResistVitality', 49: 'kResistMind', 50: 'kResistImmunity', 51: 'kResistDark',
    52: 'kAffectResist', 53: 'kMagicResist', 54: 'kPhysicResist', 55: 'kPhysicDamage',
    56: 'kMagicDamage', 57: 'kExpBonus', 58: 'kPercent'
}

# Wear positions for EQUIP_MOB command (slot in equipment)
WEAR_POSITIONS = {
    0: 'LIGHT', 1: 'FINGER_R', 2: 'FINGER_L', 3: 'NECK_1', 4: 'NECK_2',
    5: 'BODY', 6: 'HEAD', 7: 'LEGS', 8: 'FEET', 9: 'HANDS', 10: 'ARMS',
    11: 'SHIELD', 12: 'ABOUT', 13: 'WAIST', 14: 'WRIST_R', 15: 'WRIST_L',
    16: 'WIELD', 17: 'HOLD', 18: 'BOTH', 19: 'QUIVER'
}

# Direction names for door commands
DIRECTION_NAMES = {
    0: 'north', 1: 'east', 2: 'south', 3: 'west', 4: 'up', 5: 'down'
}


# ============================================================================
# Saver classes for output abstraction
# ============================================================================

# ============================================================================
# Shared helper functions to eliminate code duplication
# ============================================================================

def extract_entity_names(entity_dict):
    """Extract Russian case names from any entity."""
    names = entity_dict.get('names', {})
    return (
        names.get('aliases'),
        names.get('nominative'),
        names.get('genitive'),
        names.get('dative'),
        names.get('accusative'),
        names.get('instrumental'),
        names.get('prepositional'),
    )


def insert_entity_flags(cursor, entity_type, vnum, flags_dict):
    """Generic flag insertion for any entity type (mobs/objects/rooms)."""
    table_name = f"{entity_type}_flags"
    # Use correct column name for each entity type
    vnum_column = f"{entity_type}_vnum"

    for flag_category, flag_list in flags_dict.items():
        for flag in flag_list:
            cursor.execute(f'''
                INSERT OR IGNORE INTO {table_name}
                ({vnum_column}, flag_category, flag_name)
                VALUES (?, ?, ?)
            ''', (vnum, flag_category, flag))


def insert_entity_triggers(cursor, entity_type, vnum, triggers_list):
    """Insert triggers for any entity type (mob/obj/room)."""
    for trig_order, trig_vnum in enumerate(triggers_list):
        cursor.execute('''
            INSERT INTO entity_triggers
            (entity_type, entity_vnum, trigger_vnum, trigger_order)
            VALUES (?, ?, ?, ?)
        ''', (entity_type, vnum, trig_vnum, trig_order))


def insert_extra_descriptions(cursor, entity_type, vnum, extra_descs_list):
    """Insert extra descriptions for any entity type (obj/room)."""
    for ed in extra_descs_list:
        cursor.execute('''
            INSERT INTO extra_descriptions
            (entity_type, entity_vnum, keywords, description)
            VALUES (?, ?, ?, ?)
        ''', (entity_type, vnum, ed['keywords'], ed['description']))


class BaseSaver:
    """Base class for savers."""

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        pass

    def save_mob(self, mob):
        raise NotImplementedError

    def save_object(self, obj):
        raise NotImplementedError

    def save_room(self, room):
        raise NotImplementedError

    def save_zone(self, zone):
        raise NotImplementedError

    def save_trigger(self, trigger):
        raise NotImplementedError

    def finalize(self):
        """Called after all entities are saved."""
        pass


class YamlSaver(BaseSaver):
    """Save world data to YAML files with zone-based structure.

    Creates:
        world/dictionaries/*.yaml - Dictionary files
        world/zones/{zone_vnum}/zone.yaml - Zone definition
        world/zones/{zone_vnum}/rooms/{NN}.yaml - Room files (NN = 00-99)
        world/mobs/{vnum}.yaml - Mob files
        world/objects/{vnum}.yaml - Object files
        world/triggers/{vnum}.yaml - Trigger files
        world/zones/index.yaml - List of zones to load
    """

    def __init__(self, output_dir):
        self.output_dir = Path(output_dir) / 'world'
        self._zone_vnums = set()
        self._mob_vnums = set()
        self._obj_vnums = set()
        self._trigger_vnums = set()

    def __enter__(self):
        self._generate_dictionaries()
        return self

    def _generate_dictionaries(self):
        """Generate dictionary YAML files from the hardcoded constants."""
        dict_dir = self.output_dir / 'dictionaries'
        dict_dir.mkdir(parents=True, exist_ok=True)

        def write_dict_from_list(filename, items):
            data = {name: i for i, name in enumerate(items) if name is not None and not name.startswith('UNUSED')}
            with open(dict_dir / filename, 'w', encoding='utf-8') as f:
                f.write(f"# Dictionary: {filename.replace('.yaml', '')}\n")
                for name, idx in sorted(data.items(), key=lambda x: x[1]):
                    f.write(f"{name}: {idx}\n")

        def write_dict_from_map(filename, mapping):
            """Write dict with format {str_name: int_value}."""
            with open(dict_dir / filename, 'w', encoding='utf-8') as f:
                f.write(f"# Dictionary: {filename.replace('.yaml', '')}\n")
                for name, idx in sorted(mapping.items(), key=lambda x: x[1]):
                    f.write(f"{name}: {idx}\n")

        def write_dict_inverted(filename, mapping):
            """Write dict that has {int: str} as {str: int}."""
            inverted = {v: k for k, v in mapping.items()}
            with open(dict_dir / filename, 'w', encoding='utf-8') as f:
                f.write(f"# Dictionary: {filename.replace('.yaml', '')}\n")
                for name, idx in sorted(inverted.items(), key=lambda x: x[1]):
                    f.write(f"{name}: {idx}\n")

        def letter_to_bit(letter):
            """Convert trigger type letter to bit position."""
            if 'a' <= letter <= 'z':
                return ord(letter) - ord('a')
            elif 'A' <= letter <= 'Z':
                return 26 + ord(letter) - ord('A')
            return 0

        write_dict_from_list('room_flags.yaml', ROOM_FLAGS)
        write_dict_from_list('sectors.yaml', SECTORS)
        write_dict_from_list('obj_types.yaml', OBJ_TYPES)
        write_dict_from_list('positions.yaml', POSITIONS)
        write_dict_from_list('genders.yaml', GENDERS)
        write_dict_from_list('extra_flags.yaml', EXTRA_FLAGS)
        write_dict_from_list('wear_flags.yaml', WEAR_FLAGS)
        write_dict_from_list('action_flags.yaml', ACTION_FLAGS)
        write_dict_from_list('anti_flags.yaml', ANTI_FLAGS)
        write_dict_from_list('no_flags.yaml', NO_FLAGS)
        write_dict_from_list('affect_flags.yaml', AFFECT_FLAGS)
        write_dict_inverted('apply_locations.yaml', APPLY_LOCATIONS)
        # TRIGGER_TYPES is {letter: kName}, convert to {kName: bit_position}
        trigger_types_dict = {name: letter_to_bit(letter) for letter, name in TRIGGER_TYPES.items()}
        write_dict_from_map('trigger_types.yaml', trigger_types_dict)
        write_dict_from_map('directions.yaml', {name: id_ for id_, name in DIRECTION_NAMES.items()})
        write_dict_from_map('attach_types.yaml', {'kMobTrigger': 0, 'kObjTrigger': 1, 'kRoomTrigger': 2})

        print(f"Generated dictionaries in {dict_dir.relative_to(self.output_dir.parent)}")

    def _ensure_root_dir(self, subdir):
        """Ensure root-level directory exists."""
        out_dir = self.output_dir / subdir
        out_dir.mkdir(parents=True, exist_ok=True)
        return out_dir

    def _ensure_zone_dir(self, zone_vnum, subdir):
        """Ensure zone subdirectory exists."""
        out_dir = self.output_dir / 'zones' / str(zone_vnum) / subdir
        out_dir.mkdir(parents=True, exist_ok=True)
        return out_dir

    def save_mob(self, mob):
        yaml_content = mob_to_yaml(mob)
        out_dir = self._ensure_root_dir('mobs')
        out_file = out_dir / f"{mob['vnum']}.yaml"
        with open(out_file, 'w', encoding='koi8-r') as f:
            f.write(yaml_content)
        # Track enabled mobs for index
        if mob.get('enabled', 1):
            self._mob_vnums.add(mob['vnum'])

    def save_object(self, obj):
        yaml_content = obj_to_yaml(obj)
        out_dir = self._ensure_root_dir('objects')
        out_file = out_dir / f"{obj['vnum']}.yaml"
        with open(out_file, 'w', encoding='koi8-r') as f:
            f.write(yaml_content)
        # Track enabled objects for index
        if obj.get('enabled', 1):
            self._obj_vnums.add(obj['vnum'])

    def save_room(self, room):
        yaml_content = room_to_yaml(room)
        zone_vnum = room['vnum'] // 100
        rel_num = room['vnum'] % 100
        if room.get('enabled', 1):
            self._zone_vnums.add(zone_vnum)
        out_dir = self._ensure_zone_dir(zone_vnum, 'rooms')
        out_file = out_dir / f"{rel_num:02d}.yaml"
        with open(out_file, 'w', encoding='koi8-r') as f:
            f.write(yaml_content)

    def save_zone(self, zone):
        yaml_content = zon_to_yaml(zone)
        zone_vnum = zone['vnum']
        if zone.get('enabled', 1):
            self._zone_vnums.add(zone_vnum)
        zone_dir = self.output_dir / 'zones' / str(zone_vnum)
        zone_dir.mkdir(parents=True, exist_ok=True)
        out_file = zone_dir / 'zone.yaml'
        with open(out_file, 'w', encoding='koi8-r') as f:
            f.write(yaml_content)

    def save_trigger(self, trigger):
        yaml_content = trg_to_yaml(trigger)
        out_dir = self._ensure_root_dir('triggers')
        out_file = out_dir / f"{trigger['vnum']}.yaml"
        with open(out_file, 'w', encoding='koi8-r') as f:
            f.write(yaml_content)
        # Track enabled triggers for index
        if trigger.get('enabled', 1):
            self._trigger_vnums.add(trigger['vnum'])

    def finalize(self):
        """Create index files for all entity types."""
        # Create zones index
        if self._zone_vnums:
            zones_dir = self.output_dir / 'zones'
            zones_dir.mkdir(parents=True, exist_ok=True)
            index_data = CommentedMap()
            index_data['zones'] = CommentedSeq(sorted(self._zone_vnums))
            with open(zones_dir / 'index.yaml', 'w', encoding='koi8-r') as f:
                get_main_yaml().dump(index_data, f)
            print(f"Created zones/index.yaml with {len(self._zone_vnums)} zones")

        # Create mobs index
        if self._mob_vnums:
            mobs_dir = self.output_dir / 'mobs'
            mobs_dir.mkdir(parents=True, exist_ok=True)
            index_data = CommentedMap()
            index_data['mobs'] = CommentedSeq(sorted(self._mob_vnums))
            with open(mobs_dir / 'index.yaml', 'w', encoding='koi8-r') as f:
                get_main_yaml().dump(index_data, f)
            print(f"Created mobs/index.yaml with {len(self._mob_vnums)} mobs")

        # Create objects index
        if self._obj_vnums:
            objs_dir = self.output_dir / 'objects'
            objs_dir.mkdir(parents=True, exist_ok=True)
            index_data = CommentedMap()
            index_data['objects'] = CommentedSeq(sorted(self._obj_vnums))
            with open(objs_dir / 'index.yaml', 'w', encoding='koi8-r') as f:
                get_main_yaml().dump(index_data, f)
            print(f"Created objects/index.yaml with {len(self._obj_vnums)} objects")

        # Create triggers index
        if self._trigger_vnums:
            triggers_dir = self.output_dir / 'triggers'
            triggers_dir.mkdir(parents=True, exist_ok=True)
            index_data = CommentedMap()
            index_data['triggers'] = CommentedSeq(sorted(self._trigger_vnums))
            with open(triggers_dir / 'index.yaml', 'w', encoding='koi8-r') as f:
                get_main_yaml().dump(index_data, f)
            print(f"Created triggers/index.yaml with {len(self._trigger_vnums)} triggers")


class SqliteSaver(BaseSaver):
    """Save world data to SQLite database."""

    # Embedded schema (loaded from world_schema.sql)
    SCHEMA_SQL = None

    def __init__(self, db_path):
        self.db_path = Path(db_path)
        self.conn = None
        self._cmd_order = {}  # zone_vnum -> command order counter

    def __enter__(self):
        self.db_path.parent.mkdir(parents=True, exist_ok=True)
        # Remove existing database
        if self.db_path.exists():
            self.db_path.unlink()
        # Use check_same_thread=False for multi-threaded access (we use locks for safety)
        self.conn = sqlite3.connect(str(self.db_path), check_same_thread=False)
        self._create_schema()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.conn:
            self.conn.commit()
            self.conn.close()

    def _create_schema(self):
        """Create database schema."""
        if SqliteSaver.SCHEMA_SQL is None:
            # Try to load from file next to this script
            schema_path = Path(__file__).parent / 'world_schema.sql'
            if schema_path.exists():
                with open(schema_path, 'r', encoding='utf-8') as f:
                    SqliteSaver.SCHEMA_SQL = f.read()
            else:
                raise RuntimeError(f"Schema file not found: {schema_path}")

        # Disable foreign keys during schema creation and data import
        self.conn.execute("PRAGMA foreign_keys = OFF")
        self.conn.executescript(SqliteSaver.SCHEMA_SQL)
        self._populate_reference_tables()

    def _populate_reference_tables(self):
        """Populate reference/enum tables with constants."""
        cursor = self.conn.cursor()

        # obj_types
        for i, name in enumerate(OBJ_TYPES):
            cursor.execute("INSERT INTO obj_types (id, name) VALUES (?, ?)", (i, name))

        # sectors (skip None values for gaps in enum)
        for i, name in enumerate(SECTORS):
            if name is not None:
                cursor.execute("INSERT INTO sectors (id, name) VALUES (?, ?)", (i, name))

        # positions
        for i, name in enumerate(POSITIONS):
            cursor.execute("INSERT INTO positions (id, name) VALUES (?, ?)", (i, name))

        # genders
        for i, name in enumerate(GENDERS):
            cursor.execute("INSERT INTO genders (id, name) VALUES (?, ?)", (i, name))

        # directions
        for id_, name in DIRECTION_NAMES.items():
            cursor.execute("INSERT INTO directions (id, name) VALUES (?, ?)", (id_, name))

        # skills
        for id_, name in SKILL_NAMES.items():
            cursor.execute("INSERT INTO skills (id, name) VALUES (?, ?)", (id_, name))

        # apply_locations
        for id_, name in APPLY_LOCATIONS.items():
            cursor.execute("INSERT INTO apply_locations (id, name) VALUES (?, ?)", (id_, name))

        # wear_positions
        for id_, name in WEAR_POSITIONS.items():
            cursor.execute("INSERT INTO wear_positions (id, name) VALUES (?, ?)", (id_, name))

        # trigger_attach_types
        for id_, name in ATTACH_TYPES.items():
            cursor.execute("INSERT INTO trigger_attach_types (id, name) VALUES (?, ?)", (id_, name))

        # trigger_type_defs (from TRIGGER_TYPES mapping)
        # bit_value: lowercase 'a'-'z' → 1<<(ch-'a'), uppercase 'A'-'Z' → 1<<(26+ch-'A')
        for char_code, name in TRIGGER_TYPES.items():
            if char_code.islower():
                bit_value = 1 << (ord(char_code) - ord('a'))
            else:
                bit_value = 1 << (26 + ord(char_code) - ord('A'))
            cursor.execute(
                "INSERT INTO trigger_type_defs (char_code, name, bit_value) VALUES (?, ?, ?)",
                (char_code, name, bit_value)
            )

        self.conn.commit()

    def save_mob(self, mob):
        """Save mob dictionary to database."""
        cursor = self.conn.cursor()
        vnum = mob['vnum']
        names = mob.get('names', {})
        descs = mob.get('descriptions', {})
        stats = mob.get('stats', {})
        hp = stats.get('hp', {})
        dmg = stats.get('damage', {})
        gold = mob.get('gold', {})
        pos = mob.get('position', {})
        attrs = mob.get('attributes', {})
        enhanced = mob.get('enhanced', {})

        # Insert main mob record
        cursor.execute('''
            INSERT OR REPLACE INTO mobs (
                vnum, aliases, name_nom, name_gen, name_dat, name_acc, name_ins, name_pre,
                short_desc, long_desc, alignment, mob_type, level, hitroll_penalty, armor,
                hp_dice_count, hp_dice_size, hp_bonus, dam_dice_count, dam_dice_size, dam_bonus,
                gold_dice_count, gold_dice_size, gold_bonus, experience,
                default_pos, start_pos, sex, size, height, weight, mob_class, race,
                attr_str, attr_dex, attr_int, attr_wis, attr_con, attr_cha,
                attr_str_add, hp_regen, armour_bonus, mana_regen, cast_success, morale,
                initiative_add, absorb, aresist, mresist, presist, bare_hand_attack,
                like_work, max_factor, extra_attack, mob_remort, special_bitvector, role,
                enabled
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ''', (
            vnum,
            *extract_entity_names(mob),
            descs.get('short_desc'),
            descs.get('long_desc'),
            mob.get('alignment', 0),
            mob.get('mob_type', 'S'),
            stats.get('level', 1),
            stats.get('hitroll_penalty', 20),
            stats.get('armor', 100),
            hp.get('dice_count', 1),
            hp.get('dice_size', 1),
            hp.get('bonus', 0),
            dmg.get('dice_count', 1),
            dmg.get('dice_size', 1),
            dmg.get('bonus', 0),
            gold.get('dice_count', 0),
            gold.get('dice_size', 0),
            gold.get('bonus', 0),
            mob.get('experience', 0),
            pos.get('default'),
            pos.get('start'),
            mob.get('sex'),
            mob.get('size'),
            mob.get('height'),
            mob.get('weight'),
            mob.get('mob_class'),
            mob.get('race'),
            attrs.get('strength', 11),
            attrs.get('dexterity', 11),
            attrs.get('intelligence', 11),
            attrs.get('wisdom', 11),
            attrs.get('constitution', 11),
            attrs.get('charisma', 11),
            # Enhanced E-spec fields
            enhanced.get('str_add', 0),
            enhanced.get('hp_regen', 0),
            enhanced.get('armour_bonus', 0),
            enhanced.get('mana_regen', 0),
            enhanced.get('cast_success', 0),
            enhanced.get('morale', 0),
            enhanced.get('initiative_add', 0),
            enhanced.get('absorb', 0),
            enhanced.get('aresist', 0),
            enhanced.get('mresist', 0),
            enhanced.get('presist', 0),
            enhanced.get('bare_hand_attack', 0),
            enhanced.get('like_work', 0),
            enhanced.get('max_factor', 0),
            enhanced.get('extra_attack', 0),
            enhanced.get('mob_remort', 0),
            enhanced.get('special_bitvector'),
            enhanced.get('role'),
            mob.get('enabled', 1),
        ))

        # Insert flags
        insert_entity_flags(cursor, 'mob', vnum, {
            'action': mob.get('action_flags', []),
            'affect': mob.get('affect_flags', []),
        })

        # Insert skills
        for skill in mob.get('skills', []):
            cursor.execute('''
                INSERT OR REPLACE INTO mob_skills (mob_vnum, skill_id, value)
                VALUES (?, ?, ?)
            ''', (vnum, skill['skill_id'], skill['value']))

        # Insert triggers
        insert_entity_triggers(cursor, 'mob', vnum, mob.get('triggers', []))

        # Insert Enhanced array fields
        enhanced = mob.get('enhanced', {})
        
        # Resistances
        for idx, value in enumerate(enhanced.get('resistances', [])):
            cursor.execute('''
                INSERT OR REPLACE INTO mob_resistances (mob_vnum, resist_type, value)
                VALUES (?, ?, ?)
            ''', (vnum, idx, value))
        
        # Saves
        for idx, value in enumerate(enhanced.get('saves', [])):
            cursor.execute('''
                INSERT OR REPLACE INTO mob_saves (mob_vnum, save_type, value)
                VALUES (?, ?, ?)
            ''', (vnum, idx, value))
        
        # Feats
        for feat_id in enhanced.get('feats', []):
            cursor.execute('''
                INSERT OR IGNORE INTO mob_feats (mob_vnum, feat_id)
                VALUES (?, ?)
            ''', (vnum, feat_id))
        
        # Spells
        for spell_id in enhanced.get('spells', []):
            cursor.execute('''
                INSERT OR IGNORE INTO mob_spells (mob_vnum, spell_id, count)
                VALUES (?, ?, 1)
                ON CONFLICT(mob_vnum, spell_id) DO UPDATE SET count = count + 1
            ''', (vnum, spell_id))
        
        # Helpers
        for helper_vnum in enhanced.get('helpers', []):
            cursor.execute('''
                INSERT OR IGNORE INTO mob_helpers (mob_vnum, helper_vnum)
                VALUES (?, ?)
            ''', (vnum, helper_vnum))
        
        # Destinations
        for idx, room_vnum in enumerate(enhanced.get('destinations', [])):
            cursor.execute('''
                INSERT OR REPLACE INTO mob_destinations (mob_vnum, dest_order, room_vnum)
                VALUES (?, ?, ?)
            ''', (vnum, idx, room_vnum))
    def save_object(self, obj):
        """Save object dictionary to database."""
        cursor = self.conn.cursor()
        vnum = obj['vnum']
        names = obj.get('names', {})
        values = obj.get('values', [None, None, None, None])

        # Insert main object record
        cursor.execute('''
            INSERT OR REPLACE INTO objects (
                vnum, aliases, name_nom, name_gen, name_dat, name_acc, name_ins, name_pre,
                short_desc, action_desc, obj_type_id, material,
                value0, value1, value2, value3,
                weight, cost, rent_off, rent_on, spec_param,
                max_durability, cur_durability, timer, spell, level, sex, max_in_world,
                minimum_remorts, enabled
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ''', (
            vnum,
            *extract_entity_names(obj),
            obj.get('short_desc'),
            obj.get('action_desc'),
            obj.get('type_id'),
            obj.get('material'),
            values[0] if len(values) > 0 else None,
            values[1] if len(values) > 1 else None,
            values[2] if len(values) > 2 else None,
            values[3] if len(values) > 3 else None,
            obj.get('weight', 0),
            obj.get('cost', 0),
            obj.get('rent_off', 0),
            obj.get('rent_on', 0),
            obj.get('spec_param', 0),
            obj.get('max_durability', 100),
            obj.get('cur_durability', 100),
            obj.get('timer', -1),
            obj.get('spell', -1),
            obj.get('level', 0),
            obj.get('sex', 0),
            obj.get('max_in_world'),
            obj.get('minimum_remorts', 0),
            obj.get('enabled', 1),
        ))

        # Insert flags
        insert_entity_flags(cursor, 'obj', vnum, {
            'extra': obj.get('extra_flags', []),
            'wear': obj.get('wear_flags', []),
            'affect': obj.get('affect_flags', []),
            'no': obj.get('no_flags', []),
            'anti': obj.get('anti_flags', []),
        })

        # Insert applies
        for apply in obj.get('applies', []):
            location_id = apply['location']
            cursor.execute('''
                INSERT INTO obj_applies (obj_vnum, location_id, modifier)
                VALUES (?, ?, ?)
            ''', (vnum, location_id, apply['modifier']))

        # Insert extra descriptions
        insert_extra_descriptions(cursor, 'obj', vnum, obj.get('extra_descs', []))
        # Insert triggers
        insert_entity_triggers(cursor, 'obj', vnum, obj.get('triggers', []))
    def save_room(self, room):
        """Save room dictionary to database."""
        cursor = self.conn.cursor()
        vnum = room['vnum']

        # Insert main room record
        cursor.execute('''
            INSERT OR REPLACE INTO rooms (vnum, zone_vnum, name, description, sector_id, enabled)
            VALUES (?, ?, ?, ?, ?, ?)
        ''', (
            vnum,
            room.get('zone'),
            room.get('name'),
            room.get('description'),
            room.get('sector_id'),
            room.get('enabled', 1),
        ))

        # Insert flags
        for flag in room.get('room_flags', []):
            cursor.execute('''
                INSERT OR IGNORE INTO room_flags (room_vnum, flag_name)
                VALUES (?, ?)
            ''', (vnum, flag))

        # Insert exits
        for exit_data in room.get('exits', []):
            direction_id = exit_data.get('direction', 0)
            if not isinstance(direction_id, int):
                direction_id = 0

            exit_flags = exit_data.get('exit_flags', 0)
            if isinstance(exit_flags, int):
                exit_flags_str = str(exit_flags)
            else:
                exit_flags_str = exit_flags

            cursor.execute('''
                INSERT OR REPLACE INTO room_exits (
                    room_vnum, direction_id, description, keywords, exit_flags,
                    key_vnum, to_room, lock_complexity
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
            ''', (
                vnum,
                direction_id,
                exit_data.get('description'),
                exit_data.get('keywords'),
                exit_flags_str,
                exit_data.get('key', -1),
                exit_data.get('to_room', -1),
                exit_data.get('lock_complexity', 0),
            ))

        # Insert extra descriptions
        insert_extra_descriptions(cursor, 'room', vnum, room.get('extra_descs', []))

        insert_entity_triggers(cursor, 'room', vnum, room.get('triggers', []))

    def save_zone(self, zone):
        """Save zone dictionary to database."""
        cursor = self.conn.cursor()
        vnum = zone['vnum']
        meta = zone.get('metadata', {})

        # Insert main zone record
        cursor.execute('''
            INSERT OR REPLACE INTO zones (
                vnum, name, comment, location, author, description, builders,
                first_room, top_room, mode, zone_type, zone_group, entrance, lifespan, reset_mode, reset_idle, under_construction, enabled
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ''', (
            vnum,
            zone.get('name'),
            meta.get('comment'),
            meta.get('location'),
            meta.get('author'),
            meta.get('description'),
            zone.get('builders'),
            zone.get('first_room'),
            zone.get('top_room'),
            zone.get('mode', 0),
            zone.get('zone_type', 0),
            zone.get('zone_group', 1),
            zone.get('entrance'),
            zone.get('lifespan', 10),
            zone.get('reset_mode', 2),
            zone.get('reset_idle', 0),
            zone.get('under_construction', 0),
            zone.get('enabled', 1),
        ))

        # Insert zone groups
        for linked_zone in zone.get('typeA_list', []):
            cursor.execute('''
                INSERT OR IGNORE INTO zone_groups (zone_vnum, linked_zone_vnum, group_type)
                VALUES (?, ?, 'A')
            ''', (vnum, linked_zone))

        for linked_zone in zone.get('typeB_list', []):
            cursor.execute('''
                INSERT OR IGNORE INTO zone_groups (zone_vnum, linked_zone_vnum, group_type)
                VALUES (?, ?, 'B')
            ''', (vnum, linked_zone))

        # Insert zone commands
        if vnum not in self._cmd_order:
            self._cmd_order[vnum] = 0

        for cmd in zone.get('commands', []):
            self._cmd_order[vnum] += 1
            cmd_type = cmd.get('type', '')

            cursor.execute('''
                INSERT INTO zone_commands (
                    zone_vnum, cmd_order, cmd_type, if_flag,
                    arg_mob_vnum, arg_obj_vnum, arg_room_vnum, arg_trigger_vnum,
                    arg_container_vnum, arg_max, arg_max_world, arg_max_room,
                    arg_load_prob, arg_wear_pos_id, arg_direction_id, arg_state,
                    arg_trigger_type, arg_context, arg_var_name, arg_var_value,
                    arg_leader_mob_vnum, arg_follower_mob_vnum
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            ''', (
                vnum,
                self._cmd_order[vnum],
                cmd_type,
                cmd.get('if_flag', 0),
                cmd.get('mob_vnum'),
                cmd.get('obj_vnum'),
                cmd.get('room_vnum'),
                cmd.get('trigger_vnum'),
                cmd.get('container_vnum'),
                cmd.get('max'),
                cmd.get('max_world'),
                cmd.get('max_room'),
                cmd.get('load_prob'),
                cmd.get('wear_pos'),
                cmd.get('direction'),
                cmd.get('state'),
                cmd.get('trigger_type'),
                cmd.get('context'),
                cmd.get('var_name'),
                cmd.get('var_value'),
                cmd.get('leader_mob_vnum'),
                cmd.get('follower_mob_vnum'),
            ))

    def save_trigger(self, trigger):
        """Save trigger dictionary to database."""
        cursor = self.conn.cursor()
        vnum = trigger['vnum']

        # Insert main trigger record (without trigger_types - normalized)
        cursor.execute('''
            INSERT OR REPLACE INTO triggers (
                vnum, name, attach_type_id, narg, arglist, script, enabled
            ) VALUES (?, ?, ?, ?, ?, ?, ?)
        ''', (
            vnum,
            trigger.get('name'),
            trigger.get('attach_type_id'),
            trigger.get('narg', 0),
            trigger.get('arglist'),
            trigger.get('script'),
            trigger.get('enabled', 1),
        ))

        # Insert trigger type bindings (normalized many-to-many)
        for type_char in trigger.get('type_chars', []):
            cursor.execute('''
                INSERT OR IGNORE INTO trigger_type_bindings (trigger_vnum, type_char)
                VALUES (?, ?)
            ''', (vnum, type_char))

    def finalize(self):
        """Commit and print statistics."""
        if self.conn:
            self.conn.commit()
            cursor = self.conn.cursor()
            cursor.execute("SELECT * FROM v_world_stats")
            row = cursor.fetchone()
            if row:
                print(f"\nSQLite database statistics:")
                print(f"  Zones: {row[0]}")
                print(f"  Rooms: {row[1]}")
                print(f"  Mobs: {row[2]}")
                print(f"  Objects: {row[3]}")
                print(f"  Triggers: {row[4]}")
                print(f"  Zone commands: {row[5]}")
            print(f"\nDatabase saved to: {self.db_path}")


def get_skill_name(skill_id):
    """Get skill name by ID from dictionary."""
    return SKILL_NAMES.get(skill_id, '')


def get_apply_name(apply_id):
    """Get apply location name by ID."""
    return APPLY_LOCATIONS.get(apply_id, '')


def get_wear_pos_name(pos_id):
    """Get wear position name by ID."""
    return WEAR_POSITIONS.get(pos_id, '')


def get_direction_name(dir_id):
    """Get direction name by ID."""
    return DIRECTION_NAMES.get(dir_id, '')


def get_room_name(vnum):
    """Get room name by vnum from registry."""
    return ROOM_NAMES.get(vnum, '')


def get_mob_name(vnum):
    """Get mob name by vnum from registry."""
    return MOB_NAMES.get(vnum, '')


def get_obj_name(vnum):
    """Get object name by vnum from registry."""
    return OBJ_NAMES.get(vnum, '')


def get_trigger_name(vnum):
    """Get trigger name by vnum from registry."""
    return TRIGGER_NAMES.get(vnum, '')


def get_zone_name(vnum):
    """Get zone name by vnum from registry."""
    return ZONE_NAMES.get(vnum, '')



def get_spell_name(spell_id):
    """Get spell Russian name by spell_id."""
    return SPELL_NAMES.get(spell_id, '')


def get_material_name(material_id):
    """Get material Russian name by material_id."""
    return MATERIAL_NAMES.get(material_id, '')

def load_name_from_yaml(filepath):
    """Load name from YAML file."""
    try:
        with open(filepath, 'r', encoding='koi8-r') as f:
            for line in f:
                line = line.strip()
                if line.startswith('name:'):
                    name = line[5:].strip()
                    # Remove quotes
                    if name.startswith('"') and name.endswith('"'):
                        name = name[1:-1]
                    return name
    except Exception:
        pass
    return ''


def load_names_from_yaml_dir(yaml_dir, name_key='name'):
    """Load names from all YAML files in directory."""
    names = {}
    yaml_path = Path(yaml_dir)
    for yaml_file in yaml_path.glob('*.yaml'):
        try:
            with open(yaml_file, 'r', encoding='koi8-r') as f:
                vnum = None
                name = None
                for line in f:
                    line = line.strip()
                    if line.startswith('vnum:'):
                        vnum = int(line[5:].strip())
                    elif line.startswith(f'{name_key}:'):
                        name = line[len(name_key)+1:].strip()
                        # Remove quotes
                        if name.startswith('"') and name.endswith('"'):
                            name = name[1:-1]
                        break
                if vnum is not None and name:
                    names[vnum] = name
        except Exception:
            continue
    return names


def build_name_registries(world_dir):
    """Build name registries from YAML files for cross-references."""
    global ROOM_NAMES, MOB_NAMES, OBJ_NAMES, TRIGGER_NAMES, ZONE_NAMES

    world_path = Path(world_dir)
    if not world_path.exists():
        return

    # Load room names
    if (world_path / 'wld').exists():
        ROOM_NAMES = load_names_from_yaml_dir(world_path / 'wld', 'name')
        print(f"Loaded {len(ROOM_NAMES)} room names for cross-references")

    # Load mob names (need to handle nested structure)
    if (world_path / 'mob').exists():
        for yaml_file in (world_path / 'mob').glob('*.yaml'):
            try:
                with open(yaml_file, 'r', encoding='koi8-r') as f:
                    vnum = None
                    name = None
                    in_names = False
                    for line in f:
                        line_stripped = line.strip()
                        if line_stripped.startswith('vnum:'):
                            vnum = int(line_stripped[5:].strip())
                        elif line_stripped == 'names:':
                            in_names = True
                        elif in_names and line_stripped.startswith('aliases:'):
                            name = line_stripped[8:].strip()
                            if name.startswith('"') and name.endswith('"'):
                                name = name[1:-1]
                            break
                    if vnum is not None and name:
                        MOB_NAMES[vnum] = name
            except Exception:
                continue
        print(f"Loaded {len(MOB_NAMES)} mob names for cross-references")

    # Load object names (need to handle nested structure)
    if (world_path / 'obj').exists():
        for yaml_file in (world_path / 'obj').glob('*.yaml'):
            try:
                with open(yaml_file, 'r', encoding='koi8-r') as f:
                    vnum = None
                    name = None
                    in_names = False
                    for line in f:
                        line_stripped = line.strip()
                        if line_stripped.startswith('vnum:'):
                            vnum = int(line_stripped[5:].strip())
                        elif line_stripped == 'names:':
                            in_names = True
                        elif in_names and line_stripped.startswith('aliases:'):
                            name = line_stripped[8:].strip()
                            if name.startswith('"') and name.endswith('"'):
                                name = name[1:-1]
                            break
                    if vnum is not None and name:
                        OBJ_NAMES[vnum] = name
            except Exception:
                continue
        print(f"Loaded {len(OBJ_NAMES)} object names for cross-references")

    # Load trigger names
    if (world_path / 'trg').exists():
        TRIGGER_NAMES = load_names_from_yaml_dir(world_path / 'trg', 'name')
        print(f"Loaded {len(TRIGGER_NAMES)} trigger names for cross-references")

    # Load zone names
    if (world_path / 'zon').exists():
        ZONE_NAMES = load_names_from_yaml_dir(world_path / 'zon', 'name')
        print(f"Loaded {len(ZONE_NAMES)} zone names for cross-references")



def ascii_flags_to_int(flags_str):
    """Convert ASCII flags to integer value."""
    if not flags_str or flags_str == '0':
        return 0
    
    # If purely numeric, return as integer
    if flags_str.lstrip('-').isdigit():
        return int(flags_str)
    
    # ASCII format: each letter represents a bit, followed by optional plane digit
    result = 0
    i = 0
    while i < len(flags_str):
        letter = flags_str[i]
        plane = 0
        
        # Check if next char is a digit (plane number)
        if i + 1 < len(flags_str) and flags_str[i + 1].isdigit():
            plane = int(flags_str[i + 1])
            i += 2
        else:
            i += 1
        
        # Calculate bit position
        if letter.islower():
            bit_pos = ord(letter) - ord('a')
        elif letter.isupper():
            bit_pos = ord(letter) - ord('A') + 26
        else:
            continue
        
        # Calculate actual bit position with plane offset (30 bits per plane)
        actual_bit = plane * 30 + bit_pos
        result |= (1 << actual_bit)
    
    return result

def parse_ascii_flags(flags_str, flag_names, planes=4):
    """Parse ASCII flags like 'abc0d1' or numeric flags like '100' into list of flag names.

    Formats:
    - ASCII: each flag is a letter (a-z for 0-25, A-Z for 26-51) followed by a digit (plane 0-3)
    - Numeric: decimal number where each bit represents a flag
    """
    if not flags_str or flags_str == '0':
        return []

    result = []

    # Check if it's a numeric value (all digits)
    if flags_str.isdigit():
        value = int(flags_str)
        for i in range(len(flag_names)):
            if value & (1 << i):
                result.append(flag_names[i])
        return result

    # ASCII format parsing
    i = 0
    while i < len(flags_str):
        letter = flags_str[i]
        plane = 0

        # Check if next char is a digit (plane number)
        if i + 1 < len(flags_str) and flags_str[i + 1].isdigit():
            plane = int(flags_str[i + 1])
            i += 2
        else:
            i += 1

        # Calculate bit position
        if letter.islower():
            bit_pos = ord(letter) - ord('a')
        elif letter.isupper():
            bit_pos = ord(letter) - ord('A') + 26
        else:
            continue

        # Calculate flag index - each plane has 30 bits
        # For consistency, all flag arrays use 30-bit planes
        
        if plane == 0:
            flag_index = bit_pos
        elif plane == 1:
            flag_index = 30 + bit_pos
        elif plane == 2:
            flag_index = 60 + bit_pos  # 30 + 30 = 60
        else:
            flag_index = plane * 30 + bit_pos  # fallback

        if flag_index < len(flag_names):
            result.append(flag_names[flag_index])

    return result


def parse_mob_file(filepath):
    """Parse a .mob file and return list of mob dictionaries."""
    mobs = []

    with open(filepath, 'r', encoding='koi8-r') as f:
        content = f.read().replace('\r', '')

    # Split by mob separators (lines starting with #)
    mob_blocks = re.split(r'\n(?=#\d)', content)

    for block in mob_blocks:
        block = block.strip()
        if not block or not block.startswith('#'):
            continue

        lines = block.split('\n')
        mob = {}

        try:
            # First line: #vnum
            vnum_match = re.match(r'#(\d+)', lines[0])
            if not vnum_match:
                continue
            mob['vnum'] = int(vnum_match.group(1))

            # Parse name aliases (line with ~)
            idx = 1
            names_parts = []
            while idx < len(lines) and '~' not in lines[idx]:
                names_parts.append(lines[idx])
                idx += 1
            if idx < len(lines):
                names_parts.append(lines[idx].rstrip('~'))
            mob['names'] = {'aliases': '\r\n'.join(names_parts)}
            # Register mob name for cross-references
            if mob['vnum'] and mob['names'].get('aliases'):
                first_line = mob['names']['aliases'].split('\r\n')[0].strip()
                if first_line:
                    MOB_NAMES[mob['vnum']] = first_line
            idx += 1

            # Parse 6 case forms (each ending with ~)
            case_names = ['nominative', 'genitive', 'dative', 'accusative', 'instrumental', 'prepositional']
            for case_name in case_names:
                if idx >= len(lines):
                    break
                case_parts = []
                while idx < len(lines) and '~' not in lines[idx]:
                    case_parts.append(lines[idx])
                    idx += 1
                if idx < len(lines):
                    case_parts.append(lines[idx].rstrip('~'))
                mob['names'][case_name] = '\r\n'.join(case_parts)
                idx += 1

            # Short description (until ~)
            desc_parts = []
            while idx < len(lines) and '~' not in lines[idx]:
                desc_parts.append(lines[idx])
                idx += 1
            if idx < len(lines):
                desc_parts.append(lines[idx].rstrip('~'))
            mob['descriptions'] = {'short_desc': '\r\n'.join(desc_parts)}
            idx += 1

            # Long description (until ~)
            desc_parts = []
            while idx < len(lines) and '~' not in lines[idx]:
                desc_parts.append(lines[idx])
                idx += 1
            if idx < len(lines):
                desc_parts.append(lines[idx].rstrip('~'))
            mob['descriptions']['long_desc'] = '\r\n'.join(desc_parts)
            idx += 1

            # Action/affect flags and alignment line
            if idx < len(lines):
                parts = lines[idx].split()
                if len(parts) >= 2:
                    mob['action_flags'] = parse_ascii_flags(parts[0], ACTION_FLAGS)
                    mob['affect_flags'] = parse_ascii_flags(parts[1], ACTION_FLAGS)  # Using same for now
                    mob['alignment'] = int(parts[2]) if parts[2].lstrip('-').isdigit() else 0
                    if len(parts) >= 4:
                        mob['mob_type'] = parts[3]
                idx += 1

            # Stats line: level hitroll_bonus armor hp_dice damage_dice
            if idx < len(lines):
                parts = lines[idx].split()
                if len(parts) >= 5:
                    mob['stats'] = {
                        'level': int(parts[0]) if parts[0].isdigit() else 1,
                        'hitroll_penalty': int(parts[1]) if parts[1].lstrip('-').isdigit() else 20,
                        'armor': int(parts[2]) if parts[2].lstrip('-').isdigit() else 100
                    }
                    # Parse HP dice (format: XdY+Z)
                    hp_match = re.match(r'(-?\d+)d(\d+)([+-]\d+)?', parts[3])
                    if hp_match:
                        mob['stats']['hp'] = {
                            'dice_count': int(hp_match.group(1)),
                            'dice_size': int(hp_match.group(2)),
                            'bonus': int(hp_match.group(3)) if hp_match.group(3) else 0
                        }
                    # Parse damage dice
                    dmg_match = re.match(r'(-?\d+)d(\d+)([+-]\d+)?', parts[4])
                    if dmg_match:
                        mob['stats']['damage'] = {
                            'dice_count': int(dmg_match.group(1)),
                            'dice_size': int(dmg_match.group(2)),
                            'bonus': int(dmg_match.group(3)) if dmg_match.group(3) else 0
                        }
                idx += 1

            # Gold dice line
            if idx < len(lines):
                parts = lines[idx].split()
                if parts:
                    gold_match = re.match(r'(-?\d+)d(\d+)([+-]\d+)?', parts[0])
                    if gold_match:
                        mob['gold'] = {
                            'dice_count': int(gold_match.group(1)),
                            'dice_size': int(gold_match.group(2)),
                            'bonus': int(gold_match.group(3)) if gold_match.group(3) else 0
                        }
                    if len(parts) >= 2:
                        mob['experience'] = int(parts[1]) if parts[1].isdigit() else 0
                idx += 1

            # Position and sex line
            if idx < len(lines):
                parts = lines[idx].split()
                if len(parts) >= 2:
                    pos_default = int(parts[0]) if parts[0].isdigit() else 8
                    pos_start = int(parts[1]) if parts[1].isdigit() else 8
                    sex = int(parts[2]) if parts[2].isdigit() else 0

                    mob['position'] = {
                        'default': POSITIONS[pos_default] if pos_default < len(POSITIONS) else pos_default,
                        'start': POSITIONS[pos_start] if pos_start < len(POSITIONS) else pos_start
                    }
                    mob['sex'] = GENDERS[sex] if sex < len(GENDERS) else sex
                idx += 1

            # Parse extended mob info (E-spec)
            mob['triggers'] = []
            mob['skills'] = []
            mob['attributes'] = {}

            while idx < len(lines):
                line = lines[idx].strip()
                if not line:
                    idx += 1
                    continue

                if line.startswith('E'):
                    # Enhanced mob marker - continue parsing
                    idx += 1
                    continue
                elif line.startswith('Str:'):
                    mob['attributes']['strength'] = int(line[4:].strip())
                elif line.startswith('Dex:'):
                    mob['attributes']['dexterity'] = int(line[4:].strip())
                elif line.startswith('Int:'):
                    mob['attributes']['intelligence'] = int(line[4:].strip())
                elif line.startswith('Wis:'):
                    mob['attributes']['wisdom'] = int(line[4:].strip())
                elif line.startswith('Con:'):
                    mob['attributes']['constitution'] = int(line[4:].strip())
                elif line.startswith('Cha:'):
                    mob['attributes']['charisma'] = int(line[4:].strip())
                elif line.startswith('Size:'):
                    mob['size'] = int(line[5:].strip())
                elif line.startswith('Class:'):
                    mob['mob_class'] = int(line[6:].strip())
                elif line.startswith('Race:'):
                    mob['race'] = int(line[5:].strip())
                elif line.startswith('Height:'):
                    mob['height'] = int(line[7:].strip())
                elif line.startswith('Weight:'):
                    mob['weight'] = int(line[7:].strip())
                elif line.startswith('StrAdd:'):
                    if 'enhanced' not in mob:
                        mob['enhanced'] = {}
                    mob['enhanced']['str_add'] = int(line[7:].strip())
                elif line.startswith('HPReg:') or line.startswith('HPreg:'):
                    if 'enhanced' not in mob:
                        mob['enhanced'] = {}
                    mob['enhanced']['hp_regen'] = int(line.split(':', 1)[1].strip())
                elif line.startswith('Armour:'):
                    if 'enhanced' not in mob:
                        mob['enhanced'] = {}
                    mob['enhanced']['armour_bonus'] = int(line[7:].strip())
                elif line.startswith('PlusMem:'):
                    if 'enhanced' not in mob:
                        mob['enhanced'] = {}
                    mob['enhanced']['mana_regen'] = int(line[8:].strip())
                elif line.startswith('CastSuccess:'):
                    if 'enhanced' not in mob:
                        mob['enhanced'] = {}
                    mob['enhanced']['cast_success'] = int(line[12:].strip())
                elif line.startswith('Success:'):
                    if 'enhanced' not in mob:
                        mob['enhanced'] = {}
                    mob['enhanced']['morale'] = int(line[8:].strip())
                elif line.startswith('Initiative:'):
                    if 'enhanced' not in mob:
                        mob['enhanced'] = {}
                    mob['enhanced']['initiative_add'] = int(line[11:].strip())
                elif line.startswith('Absorbe:'):
                    if 'enhanced' not in mob:
                        mob['enhanced'] = {}
                    mob['enhanced']['absorb'] = int(line[8:].strip())
                elif line.startswith('AResist:'):
                    if 'enhanced' not in mob:
                        mob['enhanced'] = {}
                    mob['enhanced']['aresist'] = int(line[8:].strip())
                elif line.startswith('MResist:'):
                    if 'enhanced' not in mob:
                        mob['enhanced'] = {}
                    mob['enhanced']['mresist'] = int(line[8:].strip())
                elif line.startswith('PResist:'):
                    if 'enhanced' not in mob:
                        mob['enhanced'] = {}
                    mob['enhanced']['presist'] = int(line[8:].strip())
                elif line.startswith('BareHandAttack:'):
                    if 'enhanced' not in mob:
                        mob['enhanced'] = {}
                    mob['enhanced']['bare_hand_attack'] = int(line[15:].strip())
                elif line.startswith('LikeWork:'):
                    if 'enhanced' not in mob:
                        mob['enhanced'] = {}
                    mob['enhanced']['like_work'] = int(line[9:].strip())
                elif line.startswith('MaxFactor:'):
                    if 'enhanced' not in mob:
                        mob['enhanced'] = {}
                    mob['enhanced']['max_factor'] = int(line[10:].strip())
                elif line.startswith('ExtraAttack:'):
                    if 'enhanced' not in mob:
                        mob['enhanced'] = {}
                    mob['enhanced']['extra_attack'] = int(line[12:].strip())
                elif line.startswith('MobRemort:'):
                    if 'enhanced' not in mob:
                        mob['enhanced'] = {}
                    mob['enhanced']['mob_remort'] = int(line[10:].strip())
                elif line.startswith('Special_Bitvector:'):
                    if 'enhanced' not in mob:
                        mob['enhanced'] = {}
                    mob['enhanced']['special_bitvector'] = line[18:].strip()
                elif line.startswith('Role:'):
                    if 'enhanced' not in mob:
                        mob['enhanced'] = {}
                    mob['enhanced']['role'] = line[5:].strip()
                elif line.startswith('Resistances:'):
                    if 'enhanced' not in mob:
                        mob['enhanced'] = {}
                    values = line[12:].strip().split()
                    mob['enhanced']['resistances'] = [int(v) for v in values]
                elif line.startswith('Saves:'):
                    if 'enhanced' not in mob:
                        mob['enhanced'] = {}
                    values = line[6:].strip().split()
                    mob['enhanced']['saves'] = [int(v) for v in values]
                elif line.startswith('Feat:'):
                    if 'enhanced' not in mob:
                        mob['enhanced'] = {}
                    if 'feats' not in mob['enhanced']:
                        mob['enhanced']['feats'] = []
                    mob['enhanced']['feats'].append(int(line[5:].strip()))
                elif line.startswith('Spell:'):
                    if 'enhanced' not in mob:
                        mob['enhanced'] = {}
                    if 'spells' not in mob['enhanced']:
                        mob['enhanced']['spells'] = []
                    mob['enhanced']['spells'].append(int(line[6:].strip()))
                elif line.startswith('Helper:'):
                    if 'enhanced' not in mob:
                        mob['enhanced'] = {}
                    if 'helpers' not in mob['enhanced']:
                        mob['enhanced']['helpers'] = []
                    mob['enhanced']['helpers'].append(int(line[7:].strip()))
                elif line.startswith('Destination:'):
                    if 'enhanced' not in mob:
                        mob['enhanced'] = {}
                    if 'destinations' not in mob['enhanced']:
                        mob['enhanced']['destinations'] = []
                    mob['enhanced']['destinations'].append(int(line[12:].strip()))
                elif line.startswith('Skill:'):
                    parts = line[6:].strip().split()
                    if len(parts) >= 2:
                        mob['skills'].append({
                            'skill_id': int(parts[0]),
                            'value': int(parts[1])
                        })
                elif line.startswith('T '):
                    trig_vnum = int(line[2:].strip())
                    mob['triggers'].append(trig_vnum)

                idx += 1

            mobs.append(mob)

        except Exception as e:
            log_error(f"Failed to parse mob: {e}", vnum=mob.get('vnum'), filepath=str(filepath))
            continue

    return mobs


def mob_to_yaml(mob):
    """Convert mob dictionary to YAML string using ruamel.yaml."""
    data = CommentedMap()

    vnum = mob['vnum']
    data['vnum'] = vnum
    data.yaml_set_start_comment(f"Mob #{vnum}")

    # Names
    if 'names' in mob:
        names = CommentedMap()
        for key, value in mob['names'].items():
            names[key] = value
        data['names'] = names

    # Descriptions
    if 'descriptions' in mob:
        descs = CommentedMap()
        for key, value in mob['descriptions'].items():
            descs[key] = to_literal_block(value)
        data['descriptions'] = descs

    # Action flags
    if mob.get('action_flags'):
        flags = CommentedSeq(mob['action_flags'])
        data['action_flags'] = flags

    # Affect flags
    if mob.get('affect_flags'):
        flags = CommentedSeq(mob['affect_flags'])
        data['affect_flags'] = flags

    # Alignment
    if 'alignment' in mob:
        data['alignment'] = mob['alignment']

    # Mob type
    if 'mob_type' in mob:
        data['mob_type'] = mob['mob_type']

    # Stats
    if 'stats' in mob:
        stats = CommentedMap()
        s = mob['stats']
        stats['level'] = s.get('level', 1)
        stats['hitroll_penalty'] = s.get('hitroll_penalty', 20)
        stats['armor'] = s.get('armor', 100)
        if 'hp' in s:
            hp = CommentedMap()
            hp['dice_count'] = s['hp']['dice_count']
            hp['dice_size'] = s['hp']['dice_size']
            hp['bonus'] = s['hp']['bonus']
            stats['hp'] = hp
        if 'damage' in s:
            dmg = CommentedMap()
            dmg['dice_count'] = s['damage']['dice_count']
            dmg['dice_size'] = s['damage']['dice_size']
            dmg['bonus'] = s['damage']['bonus']
            stats['damage'] = dmg
        data['stats'] = stats

    # Gold
    if 'gold' in mob:
        gold = CommentedMap()
        gold['dice_count'] = mob['gold']['dice_count']
        gold['dice_size'] = mob['gold']['dice_size']
        gold['bonus'] = mob['gold']['bonus']
        data['gold'] = gold

    # Experience
    if 'experience' in mob:
        data['experience'] = mob['experience']

    # Position
    if 'position' in mob:
        pos = CommentedMap()
        pos['default'] = mob['position']['default']
        pos['start'] = mob['position']['start']
        data['position'] = pos

    # Sex
    if 'sex' in mob:
        data['sex'] = mob['sex']

    # Attributes
    if mob.get('attributes'):
        attrs = CommentedMap()
        for key, value in mob['attributes'].items():
            attrs[key] = value
        data['attributes'] = attrs

    # Additional fields
    if 'size' in mob:
        data['size'] = mob['size']
    if 'mob_class' in mob:
        data['mob_class'] = mob['mob_class']
    if 'race' in mob:
        data['race'] = mob['race']
    if 'height' in mob:
        data['height'] = mob['height']
    if 'weight' in mob:
        data['weight'] = mob['weight']

    # Skills with skill name comments
    if mob.get('skills'):
        skills = CommentedSeq()
        for skill in mob['skills']:
            s = CommentedMap()
            s['skill_id'] = skill['skill_id']
            skill_name = get_skill_name(skill['skill_id'])
            if skill_name:
                s.yaml_add_eol_comment(skill_name, 'skill_id')
            s['value'] = skill['value']
            skills.append(s)
        data['skills'] = skills

    # Triggers with name comments
    if mob.get('triggers'):
        triggers = CommentedSeq()
        for i, trig in enumerate(mob['triggers']):
            triggers.append(trig)
            trig_name = get_trigger_name(trig)
            if trig_name:
                triggers.yaml_add_eol_comment(trig_name, i)
        data['triggers'] = triggers


    # Enhanced E-spec fields
    if mob.get('enhanced'):
        enhanced = CommentedMap()
        enh = mob['enhanced']
        
        # Scalar fields
        if 'str_add' in enh:
            enhanced['str_add'] = enh['str_add']
        if 'hp_regen' in enh:
            enhanced['hp_regen'] = enh['hp_regen']
        if 'armour_bonus' in enh:
            enhanced['armour_bonus'] = enh['armour_bonus']
        if 'mana_regen' in enh:
            enhanced['mana_regen'] = enh['mana_regen']
        if 'cast_success' in enh:
            enhanced['cast_success'] = enh['cast_success']
        if 'morale' in enh:
            enhanced['morale'] = enh['morale']
        if 'initiative_add' in enh:
            enhanced['initiative_add'] = enh['initiative_add']
        if 'absorb' in enh:
            enhanced['absorb'] = enh['absorb']
        if 'aresist' in enh:
            enhanced['aresist'] = enh['aresist']
        if 'mresist' in enh:
            enhanced['mresist'] = enh['mresist']
        if 'presist' in enh:
            enhanced['presist'] = enh['presist']
        if 'bare_hand_attack' in enh:
            enhanced['bare_hand_attack'] = enh['bare_hand_attack']
        if 'like_work' in enh:
            enhanced['like_work'] = enh['like_work']
        if 'max_factor' in enh:
            enhanced['max_factor'] = enh['max_factor']
        if 'extra_attack' in enh:
            enhanced['extra_attack'] = enh['extra_attack']
        if 'mob_remort' in enh:
            enhanced['mob_remort'] = enh['mob_remort']
        if 'special_bitvector' in enh:
            enhanced['special_bitvector'] = enh['special_bitvector']
        if 'role' in enh:
            enhanced['role'] = enh['role']
        
        # Array fields
        if enh.get('resistances'):
            enhanced['resistances'] = CommentedSeq(enh['resistances'])
        if enh.get('saves'):
            enhanced['saves'] = CommentedSeq(enh['saves'])
        if enh.get('feats'):
            enhanced['feats'] = CommentedSeq(enh['feats'])
        if enh.get('spells'):
            enhanced['spells'] = CommentedSeq(enh['spells'])
        if enh.get('helpers'):
            enhanced['helpers'] = CommentedSeq(enh['helpers'])
        if enh.get('destinations'):
            enhanced['destinations'] = CommentedSeq(enh['destinations'])
        
        if enhanced:
            data['enhanced'] = enhanced
    return yaml_dump_to_string(data)


def parse_obj_file(filepath):
    """Parse a .obj file and return list of object dictionaries."""
    objs = []

    with open(filepath, 'r', encoding='koi8-r') as f:
        content = f.read().replace('\r', '')

    # Split by object separators (lines starting with #)
    obj_blocks = re.split(r'\n(?=#\d)', content)

    for block in obj_blocks:
        block = block.strip()
        if not block or not block.startswith('#'):
            continue

        lines = block.split('\n')
        obj = {}

        try:
            # First line: #vnum
            vnum_match = re.match(r'#(\d+)', lines[0])
            if not vnum_match:
                continue
            obj['vnum'] = int(vnum_match.group(1))

            idx = 1

            # Parse name aliases (line with ~)
            names_parts = []
            while idx < len(lines) and '~' not in lines[idx]:
                names_parts.append(lines[idx])
                idx += 1
            if idx < len(lines):
                names_parts.append(lines[idx].rstrip('~'))
            obj['names'] = {'aliases': '\r\n'.join(names_parts)}
            # Register object name for cross-references
            if obj['vnum'] and obj['names'].get('aliases'):
                first_line = obj['names']['aliases'].split('\r\n')[0].strip()
                if first_line:
                    OBJ_NAMES[obj['vnum']] = first_line
            idx += 1

            # Parse 6 case forms (each ending with ~)
            case_names = ['nominative', 'genitive', 'dative', 'accusative', 'instrumental', 'prepositional']
            for case_name in case_names:
                if idx >= len(lines):
                    break
                case_parts = []
                while idx < len(lines) and '~' not in lines[idx]:
                    case_parts.append(lines[idx])
                    idx += 1
                if idx < len(lines):
                    case_parts.append(lines[idx].rstrip('~'))
                obj['names'][case_name] = '\r\n'.join(case_parts)
                idx += 1

            # Short description (room desc, until ~)
            desc_parts = []
            while idx < len(lines) and '~' not in lines[idx]:
                desc_parts.append(lines[idx])
                idx += 1
            if idx < len(lines):
                desc_parts.append(lines[idx].rstrip('~'))
            obj['short_desc'] = '\r\n'.join(desc_parts)
            idx += 1

            # Action description (until ~)
            desc_parts = []
            while idx < len(lines) and '~' not in lines[idx]:
                desc_parts.append(lines[idx])
                idx += 1
            if idx < len(lines):
                desc_parts.append(lines[idx].rstrip('~'))
            obj['action_desc'] = '\r\n'.join(desc_parts)
            idx += 1


            # Line 1: spec_param, max_durability, cur_durability, material
            if idx < len(lines):
                parts = lines[idx].split()
                if len(parts) >= 1:
                    obj['spec_param'] = ascii_flags_to_int(parts[0])
                if len(parts) >= 2:
                    obj['max_durability'] = int(parts[1]) if parts[1].lstrip('-').isdigit() else 100
                if len(parts) >= 2:
                    obj['cur_durability'] = int(parts[2]) if parts[2].lstrip('-').isdigit() else 100
                if len(parts) >= 4:
                    obj['material'] = int(parts[3]) if parts[3].isdigit() else 0
                idx += 1

            # Line 2: sex, timer, spell, level
            if idx < len(lines):
                parts = lines[idx].split()
                if len(parts) >= 1:
                    obj['sex'] = int(parts[0]) if parts[0].isdigit() else 0
                if len(parts) >= 2:
                    timer_val = int(parts[1]) if parts[1].lstrip('-').isdigit() else -1
                    obj['timer'] = timer_val if timer_val > 0 else 10080  # 7 days in minutes
                if len(parts) >= 2:
                    obj['spell'] = int(parts[2]) if parts[2].lstrip('-').isdigit() else -1
                if len(parts) >= 4:
                    obj['level'] = int(parts[3]) if parts[3].isdigit() else 1
                idx += 1

            # Line 3: affect_flags, anti_flags, no_flags
            if idx < len(lines):
                parts = lines[idx].split()
                if len(parts) >= 1:
                    obj['affect_flags'] = parse_ascii_flags(parts[0], AFFECT_FLAGS) if len(parts) >= 1 else []
                if len(parts) >= 2:
                    obj['anti_flags'] = parse_ascii_flags(parts[1], ANTI_FLAGS) if len(parts) >= 2 else []
                if len(parts) >= 2:
                    obj['no_flags'] = parse_ascii_flags(parts[2], NO_FLAGS) if len(parts) >= 3 else []
                idx += 1

            # Line 4: type, extra_flags, wear_flags
            if idx < len(lines):
                parts = lines[idx].split()
                if len(parts) >= 1:
                    obj_type = int(parts[0]) if parts[0].isdigit() else 0
                    obj['type_id'] = obj_type
                    obj['type'] = OBJ_TYPES[obj_type] if obj_type < len(OBJ_TYPES) else obj_type
                if len(parts) >= 2:
                    obj['extra_flags'] = parse_ascii_flags(parts[1], EXTRA_FLAGS)
                if len(parts) >= 2:
                    obj['wear_flags'] = parse_ascii_flags(parts[2], WEAR_FLAGS)
                idx += 1

            # Line 5: values (val[0], val[1], val[2], val[3])
            # val0 is parsed with asciiflag_conv in Legacy, which treats
            # negative numbers as 0 (doesn't recognize "-" as valid)
            if idx < len(lines):
                parts = lines[idx].split()
                values = []
                for i, p in enumerate(parts[:4]):
                    if i == 0 and p.startswith('-') and not p[1:].isdigit():
                        # asciiflag_conv doesn't handle negative, returns 0
                        values.append('0')
                    elif i == 0 and p.startswith('-'):
                        # asciiflag_conv treats "-N" as 0 because "-" is not a digit
                        values.append('0')
                    else:
                        values.append(p)
                obj['values'] = values
                idx += 1

            # Line 6: weight, cost, rent_off, rent_on
            if idx < len(lines):
                parts = lines[idx].split()
                if len(parts) >= 1:
                    obj['weight'] = int(parts[0]) if parts[0].isdigit() else 0
                if len(parts) >= 2:
                    obj['cost'] = int(parts[1]) if parts[1].isdigit() else 0
                if len(parts) >= 2:
                    obj['rent_off'] = int(parts[2]) if parts[2].isdigit() else 0
                if len(parts) >= 4:
                    obj['rent_on'] = int(parts[3]) if parts[3].isdigit() else 0
                idx += 1


            # Parse extra data (A, E, M, T sections)
            obj['applies'] = []
            obj['extra_descs'] = []
            obj['triggers'] = []

            while idx < len(lines):
                line = lines[idx].strip()

                if line == 'A':
                    # Apply
                    idx += 1
                    if idx < len(lines):
                        parts = lines[idx].split()
                        if len(parts) >= 2:
                            obj['applies'].append({
                                'location': int(parts[0]),
                                'modifier': int(parts[1])
                            })
                elif line == 'E':
                    # Extra description
                    idx += 1
                    ed = {}
                    if idx < len(lines):
                        # Keywords until ~
                        kw_parts = []
                        while idx < len(lines) and '~' not in lines[idx]:
                            kw_parts.append(lines[idx])
                            idx += 1
                        if idx < len(lines):
                            kw_parts.append(lines[idx].rstrip('~'))
                        ed['keywords'] = '\r\n'.join(kw_parts)  # Preserve all whitespace
                        idx += 1

                        # Description until ~
                        desc_parts = []
                        while idx < len(lines) and '~' not in lines[idx]:
                            desc_parts.append(lines[idx])
                            idx += 1
                        if idx < len(lines):
                            desc_parts.append(lines[idx].rstrip('~'))
                        ed['description'] = '\r\n'.join(desc_parts)
                        obj['extra_descs'].append(ed)
                elif line.startswith('M '):
                    obj['max_in_world'] = int(line[2:].strip())
                elif line.startswith('R '):
                    obj['minimum_remorts'] = int(line[2:].strip())
                elif line.startswith('T '):
                    obj['triggers'].append(int(line[2:].strip()))

                idx += 1

            objs.append(obj)

        except Exception as e:
            log_error(f"Failed to parse object: {e}", vnum=obj.get('vnum'), filepath=str(filepath))
            continue

    return objs


def obj_to_yaml(obj):
    """Convert object dictionary to YAML string using ruamel.yaml."""
    data = CommentedMap()

    vnum = obj['vnum']
    data['vnum'] = vnum
    data.yaml_set_start_comment(f"Object #{vnum}")

    # Names
    if 'names' in obj:
        names = CommentedMap()
        for key, value in obj['names'].items():
            names[key] = value
        data['names'] = names

    # Descriptions
    if obj.get('short_desc'):
        data['short_desc'] = to_literal_block(obj['short_desc'])
    if obj.get('action_desc'):
        data['action_desc'] = to_literal_block(obj['action_desc'])

    # Type
    if 'type' in obj:
        data['type'] = obj['type']

    # Flags (now as lists)
    if obj.get('extra_flags'):
        flags = CommentedSeq(obj['extra_flags'])
        data['extra_flags'] = flags
    if obj.get('wear_flags'):
        flags = CommentedSeq(obj['wear_flags'])
        data['wear_flags'] = flags
    if obj.get('no_flags'):
        flags = CommentedSeq(obj['no_flags'])
        data['no_flags'] = flags
    if obj.get('anti_flags'):
        flags = CommentedSeq(obj['anti_flags'])
        data['anti_flags'] = flags
    if obj.get('affect_flags'):
        flags = CommentedSeq(obj['affect_flags'])
        data['affect_flags'] = flags
    if 'material' in obj:
        data['material'] = obj['material']
        material_name = get_material_name(obj['material'])
        if material_name:
            data.yaml_add_eol_comment(material_name, 'material')

    # Values
    if 'values' in obj:
        data['values'] = obj['values']

    # Physical properties
    if 'weight' in obj:
        data['weight'] = obj['weight']
    if 'cost' in obj:
        data['cost'] = obj['cost']
    if 'rent_off' in obj:
        data['rent_off'] = obj['rent_off']
    if 'rent_on' in obj:
        data['rent_on'] = obj['rent_on']

    # Durability
    if 'spec_param' in obj:
        data['spec_param'] = obj['spec_param']
    if 'max_durability' in obj:
        data['max_durability'] = obj['max_durability']
    if 'cur_durability' in obj:
        data['cur_durability'] = obj['cur_durability']

    # Timer/spell/level/sex
    if 'timer' in obj:
        data['timer'] = obj['timer']
    if 'spell' in obj:
        data['spell'] = obj['spell']
        spell_name = get_spell_name(obj['spell'])
        if spell_name:
            data.yaml_add_eol_comment(spell_name, 'spell')
    if 'level' in obj:
        data['level'] = obj['level']
    if 'sex' in obj:
        data['sex'] = GENDERS[obj['sex']] if obj['sex'] < len(GENDERS) else obj['sex']

    # Applies with location name comments
    if obj.get('applies'):
        applies = CommentedSeq()
        for apply in obj['applies']:
            a = CommentedMap()
            a['location'] = apply['location']
            apply_name = get_apply_name(apply['location'])
            if apply_name:
                a.yaml_add_eol_comment(apply_name, 'location')
            a['modifier'] = apply['modifier']
            applies.append(a)
        data['applies'] = applies

    # Extra descriptions
    if obj.get('extra_descs'):
        eds = CommentedSeq()
        for ed in obj['extra_descs']:
            e = CommentedMap()
            e['keywords'] = ed['keywords']
            e['description'] = to_literal_block(ed['description'])
            eds.append(e)
        data['extra_descriptions'] = eds

    # Max in world
    if 'max_in_world' in obj:
        data['max_in_world'] = obj['max_in_world']
    # Minimum remorts
    if obj.get('minimum_remorts') and obj['minimum_remorts'] != 0:
        data['minimum_remorts'] = obj['minimum_remorts']

    # Triggers with name comments
    if obj.get('triggers'):
        triggers = CommentedSeq()
        for i, trig in enumerate(obj['triggers']):
            triggers.append(trig)
            trig_name = get_trigger_name(trig)
            if trig_name:
                triggers.yaml_add_eol_comment(trig_name, i)
        data['triggers'] = triggers

    return yaml_dump_to_string(data)


def parse_wld_file(filepath):
    """Parse a .wld file and return list of room dictionaries."""
    rooms = []

    with open(filepath, 'r', encoding='koi8-r') as f:
        content = f.read().replace('\r', '')

    # Split by room separators (lines starting with #)
    room_blocks = re.split(r'\n(?=#\d)', content)

    for block in room_blocks:
        block = block.strip()
        if not block or not block.startswith('#'):
            continue

        lines = block.split('\n')
        room = {}

        try:
            # First line: #vnum
            vnum_match = re.match(r'#(\d+)', lines[0])
            if not vnum_match:
                continue
            room['vnum'] = int(vnum_match.group(1))

            idx = 1

            # Room name (until ~)
            name_parts = []
            while idx < len(lines) and '~' not in lines[idx]:
                name_parts.append(lines[idx])
                idx += 1
            if idx < len(lines):
                name_parts.append(lines[idx].rstrip('~'))
            room['name'] = '\r\n'.join(name_parts)  # Preserve newlines in multi-line names
            # Register room name for cross-references
            if room['vnum'] and room.get('name'):
                first_line = room['name'].split('\r\n')[0].strip()
                if first_line:
                    ROOM_NAMES[room['vnum']] = first_line
            idx += 1

            # Description (until ~)
            desc_parts = []
            while idx < len(lines) and '~' not in lines[idx]:
                desc_parts.append(lines[idx])
                idx += 1
            if idx < len(lines):
                desc_parts.append(lines[idx].rstrip('~'))
            room['description'] = '\r\n'.join(desc_parts)
            idx += 1

            # Zone/flags/sector line
            if idx < len(lines):
                parts = lines[idx].split()
                if len(parts) >= 2:
                    room['zone'] = int(parts[0]) if parts[0].isdigit() else 0
                    room['room_flags'] = parse_ascii_flags(parts[1], ROOM_FLAGS)
                    sector = int(parts[2]) if parts[2].isdigit() else 0
                    room['sector_id'] = sector
                    room['sector'] = SECTORS[sector] if sector < len(SECTORS) else sector
                idx += 1

            # Parse directions, extra descs, triggers
            room['exits'] = []
            room['extra_descs'] = []
            room['triggers'] = []

            while idx < len(lines):
                line = lines[idx].strip()

                if line.startswith('D'):
                    # Direction
                    direction = int(line[1:]) if line[1:].isdigit() else -1
                    if direction >= 0:
                        exit_data = {'direction': direction}
                        idx += 1

                        # Description until ~
                        desc_parts = []
                        while idx < len(lines) and '~' not in lines[idx]:
                            desc_parts.append(lines[idx])
                            idx += 1
                        if idx < len(lines):
                            desc_parts.append(lines[idx].rstrip('~'))
                        exit_data['description'] = '\r\n'.join(desc_parts)
                        idx += 1

                        # Keywords until ~
                        kw_parts = []
                        while idx < len(lines) and '~' not in lines[idx]:
                            kw_parts.append(lines[idx])
                            idx += 1
                        if idx < len(lines):
                            kw_parts.append(lines[idx].rstrip('~'))
                        exit_data['keywords'] = '\r\n'.join(kw_parts)
                        idx += 1

                        # Exit info line
                        if idx < len(lines):
                            parts = lines[idx].split()
                            if len(parts) >= 2:
                                raw_flags = int(parts[0]) if parts[0].isdigit() else 0
                                exit_data['key'] = int(parts[1]) if parts[1].lstrip('-').isdigit() else -1
                                exit_data['to_room'] = int(parts[2]) if parts[2].lstrip('-').isdigit() else -1

                                if len(parts) == 3:
                                    # Old format: convert bits 1->kHasDoor(1), 2->kPickroof(8), 4->kHidden(16)
                                    new_flags = 0
                                    if raw_flags & 1:
                                        new_flags |= 1   # kHasDoor
                                    if raw_flags & 2:
                                        new_flags |= 8   # kPickroof
                                    if raw_flags & 4:
                                        new_flags |= 16  # kHidden
                                    exit_data['exit_flags'] = new_flags
                                    exit_data['lock_complexity'] = 0
                                else:
                                    # New format (4 values): use flags directly
                                    exit_data['exit_flags'] = raw_flags
                                    exit_data['lock_complexity'] = int(parts[3]) if parts[3].isdigit() else 0

                        room['exits'].append(exit_data)
                elif line == 'E':
                    # Extra description
                    idx += 1
                    ed = {}

                    # Keywords until ~
                    kw_parts = []
                    while idx < len(lines) and '~' not in lines[idx]:
                        kw_parts.append(lines[idx])
                        idx += 1
                    if idx < len(lines):
                        kw_parts.append(lines[idx].rstrip('~'))
                    ed['keywords'] = '\r\n'.join(kw_parts)  # Preserve all whitespace
                    idx += 1

                    # Description until ~
                    desc_parts = []
                    while idx < len(lines) and '~' not in lines[idx]:
                        desc_parts.append(lines[idx])
                        idx += 1
                    if idx < len(lines):
                        desc_parts.append(lines[idx].rstrip('~'))
                    ed['description'] = '\r\n'.join(desc_parts)

                    room['extra_descs'].append(ed)
                elif line.startswith('T '):
                    room['triggers'].append(int(line[2:].strip()))
                elif line == 'S':
                    # S marks end of exits, but triggers may follow
                    idx += 1
                    # Continue reading T lines after S
                    while idx < len(lines):
                        line = lines[idx].strip()
                        if line.startswith('T '):
                            room['triggers'].append(int(line[2:].strip()))
                            idx += 1
                        elif line.startswith('#') or not line:
                            break
                        else:
                            idx += 1
                    break

                idx += 1

            rooms.append(room)

        except Exception as e:
            log_error(f"Failed to parse room: {e}", vnum=room.get('vnum'), filepath=str(filepath))
            continue

    return rooms


def room_to_yaml(room):
    """Convert room dictionary to YAML string using ruamel.yaml."""
    DIRECTION_NAMES = ['north', 'east', 'south', 'west', 'up', 'down']

    data = CommentedMap()

    vnum = room['vnum']
    data['vnum'] = vnum
    data.yaml_set_start_comment(f"Room #{vnum}")

    if 'zone' in room:
        data['zone'] = room['zone']

    if 'name' in room:
        data['name'] = room['name']

    if 'description' in room:
        data['description'] = to_literal_block(room['description'])

    # Room flags
    if room.get('room_flags'):
        flags = CommentedSeq(room['room_flags'])
        data['flags'] = flags

    # Sector
    if 'sector' in room:
        data['sector'] = room['sector']

    # Exits with to_room name comments
    if room.get('exits'):
        exits = CommentedSeq()
        for exit_data in room['exits']:
            e = CommentedMap()
            direction = exit_data.get('direction', 0)
            e['direction'] = DIRECTION_NAMES[direction] if direction < len(DIRECTION_NAMES) else direction

            if exit_data.get('description'):
                e['description'] = to_literal_block(exit_data['description'])
            if exit_data.get('keywords'):
                e['keywords'] = exit_data['keywords']

            e['exit_flags'] = exit_data.get('exit_flags', 0)
            e['key'] = exit_data.get('key', -1)

            to_room = exit_data.get('to_room', -1)
            e['to_room'] = to_room

            # Add room name as comment
            room_name = get_room_name(to_room)
            if room_name:
                e.yaml_add_eol_comment(room_name, 'to_room')

            if 'lock_complexity' in exit_data:
                e['lock_complexity'] = exit_data['lock_complexity']

            exits.append(e)
        data['exits'] = exits

    # Extra descriptions
    if room.get('extra_descs'):
        eds = CommentedSeq()
        for ed in room['extra_descs']:
            e = CommentedMap()
            e['keywords'] = ed['keywords']
            e['description'] = to_literal_block(ed['description'])
            eds.append(e)
        data['extra_descriptions'] = eds

    # Triggers with name comments
    if room.get('triggers'):
        triggers = CommentedSeq()
        for i, trig in enumerate(room['triggers']):
            triggers.append(trig)
            trig_name = get_trigger_name(trig)
            if trig_name:
                triggers.yaml_add_eol_comment(trig_name, i)
        data['triggers'] = triggers

    return yaml_dump_to_string(data)


def parse_trg_file(filepath):
    """Parse a .trg file and return list of trigger dictionaries."""
    triggers = []

    with open(filepath, 'r', encoding='koi8-r') as f:
        content = f.read().replace('\r', '')

    # Split by trigger separators (lines starting with #)
    trg_blocks = re.split(r'\n(?=#\d)', content)

    for block in trg_blocks:
        block = block.strip()
        if not block or not block.startswith('#'):
            continue

        lines = block.split('\n')
        trigger = {}

        try:
            # First line: #vnum
            vnum_match = re.match(r'#(\d+)', lines[0])
            if not vnum_match:
                continue
            trigger['vnum'] = int(vnum_match.group(1))

            idx = 1

            # Name until ~
            name_parts = []
            while idx < len(lines) and '~' not in lines[idx]:
                name_parts.append(lines[idx])
                idx += 1
            if idx < len(lines):
                name_parts.append(lines[idx].rstrip('~'))
            trigger['name'] = '\r\n'.join(name_parts)
            # Register trigger name for cross-references
            if trigger['vnum'] and trigger.get('name'):
                first_line = trigger['name'].split('\r\n')[0].strip()
                if first_line:
                    TRIGGER_NAMES[trigger['vnum']] = first_line
            idx += 1

            # Attach type, trigger type, narg line
            if idx < len(lines):
                parts = lines[idx].split()
                if len(parts) >= 2:
                    attach_type = int(parts[0]) if parts[0].isdigit() else 0
                    trigger['attach_type_id'] = attach_type
                    trigger['attach_type'] = ATTACH_TYPES.get(attach_type, attach_type)

                    # Parse trigger types (letters or numeric)
                    # If numeric, convert to letters (same as asciiflag_conv inverse)
                    flags_str = parts[1]
                    if flags_str.isdigit():
                        flags_str = numeric_flags_to_letters(int(flags_str))
                    
                    trig_types = []
                    type_chars = []
                    for ch in flags_str:
                        if ch.isalpha():
                            if ch in TRIGGER_TYPES:
                                trig_types.append(TRIGGER_TYPES[ch])
                            type_chars.append(ch)
                    trigger['trigger_types'] = trig_types
                    trigger['type_chars'] = type_chars

                    trigger['narg'] = int(parts[2]) if len(parts) > 2 and parts[2].isdigit() else 0
                idx += 1

            # Argument until ~
            arg_parts = []
            while idx < len(lines) and '~' not in lines[idx]:
                arg_parts.append(lines[idx])
                idx += 1
            if idx < len(lines):
                arg_parts.append(lines[idx].rstrip('~'))
            trigger['arglist'] = '\r\n'.join(arg_parts)
            idx += 1

            # Script until ~
            script_parts = []
            while idx < len(lines) and not lines[idx].rstrip().endswith('~'):
                script_parts.append(lines[idx].rstrip('\n'))
                idx += 1
            if idx < len(lines):
                last_line = lines[idx].rstrip('~')
                if last_line:
                    script_parts.append(last_line)
            trigger['script'] = '\r\n'.join(script_parts).replace('~~', '~')

            triggers.append(trigger)

        except Exception as e:
            log_error(f"Failed to parse trigger: {e}", vnum=trigger.get('vnum'), filepath=str(filepath))
            continue

    return triggers


def trg_to_yaml(trigger):
    """Convert trigger dictionary to YAML string using ruamel.yaml."""
    data = CommentedMap()

    vnum = trigger['vnum']
    data['vnum'] = vnum
    data.yaml_set_start_comment(f"Trigger #{vnum}")

    if 'name' in trigger:
        data['name'] = trigger['name']

    if 'attach_type' in trigger:
        data['attach_type'] = trigger['attach_type']

    if 'trigger_types' in trigger:
        types = CommentedSeq(trigger['trigger_types'])
        data['trigger_types'] = types

    if 'narg' in trigger:
        data['narg'] = trigger['narg']

    if 'arglist' in trigger:
        data['arglist'] = trigger['arglist']

    if 'script' in trigger:
        data['script'] = to_literal_block(trigger['script'])

    return yaml_dump_to_string(data)


def parse_zon_file(filepath):
    """Parse a .zon file and return zone dictionary."""
    zone = {}

    with open(filepath, 'r', encoding='koi8-r') as f:
        lines = f.readlines()

    try:
        idx = 0

        # Skip until first # line
        while idx < len(lines) and not lines[idx].strip().startswith('#'):
            idx += 1

        if idx >= len(lines):
            return None

        # First line: #vnum
        vnum_match = re.match(r'#(\d+)', lines[idx].strip())
        if not vnum_match:
            return None
        zone['vnum'] = int(vnum_match.group(1))
        idx += 1

        # Zone name until ~
        name_parts = []
        while idx < len(lines) and '~' not in lines[idx]:
            name_parts.append(lines[idx].rstrip('\n'))
            idx += 1
        if idx < len(lines):
            name_parts.append(lines[idx].rstrip('\n').rstrip('~'))
        zone['name'] = ' '.join(name_parts)
        # Register zone name for cross-references
        if zone['vnum'] and zone.get('name'):
            ZONE_NAMES[zone['vnum']] = zone['name'].strip()
        idx += 1

        # Parse metadata lines (^, &, !, $) and optional builders until next #
        zone['metadata'] = {}
        while idx < len(lines):
            line = lines[idx].rstrip('\n')
            stripped = line.strip()

            # Stop at next # line (zone params)
            if stripped.startswith('#'):
                break

            # Parse metadata prefixes
            if stripped.startswith('^'):
                # Comment - check if ~ is on same line
                content = stripped[1:]
                if '~' in content:
                    zone['metadata']['comment'] = content.rstrip('~')
                else:
                    meta_parts = [content]
                    idx += 1
                    while idx < len(lines) and '~' not in lines[idx]:
                        meta_parts.append(lines[idx].rstrip('\n'))
                        idx += 1
                    if idx < len(lines):
                        meta_parts.append(lines[idx].rstrip('\n').rstrip('~'))
                    zone['metadata']['comment'] = ' '.join(meta_parts)
                idx += 1
                continue
            elif stripped.startswith('&'):
                # Location - check if ~ is on same line
                content = stripped[1:]
                if '~' in content:
                    zone['metadata']['location'] = content.rstrip('~')
                else:
                    meta_parts = [content]
                    idx += 1
                    while idx < len(lines) and '~' not in lines[idx]:
                        meta_parts.append(lines[idx].rstrip('\n'))
                        idx += 1
                    if idx < len(lines):
                        meta_parts.append(lines[idx].rstrip('\n').rstrip('~'))
                    zone['metadata']['location'] = ' '.join(meta_parts)
                idx += 1
                continue
            elif stripped.startswith('!'):
                # Author - check if ~ is on same line
                content = stripped[1:]
                if '~' in content:
                    zone['metadata']['author'] = content.rstrip('~')
                else:
                    meta_parts = [content]
                    idx += 1
                    while idx < len(lines) and '~' not in lines[idx]:
                        meta_parts.append(lines[idx].rstrip('\n'))
                        idx += 1
                    if idx < len(lines):
                        meta_parts.append(lines[idx].rstrip('\n').rstrip('~'))
                    zone['metadata']['author'] = ' '.join(meta_parts)
                idx += 1
                continue
            elif stripped.startswith('$') and not stripped.startswith('$~'):
                # Description (not end of file marker) - check if ~ is on same line
                content = stripped[1:]
                if '~' in content:
                    zone['metadata']['description'] = content.rstrip('~')
                else:
                    meta_parts = [content]
                    idx += 1
                    while idx < len(lines) and '~' not in lines[idx]:
                        meta_parts.append(lines[idx].rstrip('\n'))
                        idx += 1
                    if idx < len(lines):
                        meta_parts.append(lines[idx].rstrip('\n').rstrip('~'))
                    zone['metadata']['description'] = ' '.join(meta_parts)
                idx += 1
                continue
            elif '~' in stripped and not stripped.startswith('#'):
                # Builders line (plain text until ~)
                builder_parts = []
                # Go back to start of this line and parse until ~
                while idx < len(lines) and '~' not in lines[idx]:
                    builder_parts.append(lines[idx].rstrip('\n'))
                    idx += 1
                if idx < len(lines):
                    builder_parts.append(lines[idx].rstrip('\n').rstrip('~'))
                zone['builders'] = '\r\n'.join(builder_parts)
                idx += 1
                continue
            else:
                idx += 1
                continue

        # Remove empty metadata
        if not zone['metadata']:
            del zone['metadata']

        # Zone info line: #first_room mode type [entrance]
        while idx < len(lines):
            line = lines[idx].strip()
            if line.startswith('#'):
                parts = line[1:].split()
                if len(parts) >= 1:
                    zone['mode'] = int(parts[0]) if parts[0].isdigit() else 0
                if len(parts) >= 2:
                    zone['zone_type'] = int(parts[1]) if parts[1].isdigit() else 0
                if len(parts) >= 3:
                    zone['zone_group'] = int(parts[2]) if parts[2].isdigit() else 1
                if len(parts) >= 4:
                    zone['entrance'] = int(parts[3]) if parts[3].isdigit() else 0
                # Next line: top_room lifespan reset_mode reset_idle
                idx += 1
                if idx < len(lines):
                    params = lines[idx].strip().split()
                    # Remove trailing * if present
                    params = [p for p in params if p != '*']
                    if len(params) >= 1:
                        zone['top_room'] = int(params[0]) if params[0].isdigit() else 0
                    if len(params) >= 2:
                        zone['lifespan'] = int(params[1]) if params[1].isdigit() else 10
                    if len(params) >= 3:
                        zone['reset_mode'] = int(params[2]) if params[2].isdigit() else 0
                    if len(params) >= 4:
                        zone['reset_idle'] = int(params[3]) if params[3].isdigit() else 0
                    # Check for 'test' flag (under_construction)
                    for p in params[4:]:
                        if p.lower() == 'test':
                            zone['under_construction'] = 1
                break
            idx += 1
        idx += 1

        # Parse zone grouping commands (A, B) and spawn commands
        zone['commands'] = []
        zone['typeA_list'] = []
        zone['typeB_list'] = []

        while idx < len(lines):
            line = lines[idx].strip()

            if line == 'S' or line.startswith('$'):
                break

            if not line or line.startswith('*'):
                idx += 1
                continue

            # Remove trailing comments in parentheses
            if '(' in line:
                line = line[:line.index('(')].strip()

            parts = line.split()
            if not parts:
                idx += 1
                continue

            cmd_type = parts[0]

            # Zone grouping commands
            if cmd_type == 'A' and len(parts) >= 2:
                # A zone_vnum - add to typeA list
                zone_vnum = int(parts[1]) if parts[1].isdigit() else 0
                if zone_vnum:
                    zone['typeA_list'].append(zone_vnum)
                idx += 1
                continue
            elif cmd_type == 'B' and len(parts) >= 2:
                # B zone_vnum - add to typeB list
                zone_vnum = int(parts[1]) if parts[1].isdigit() else 0
                if zone_vnum:
                    zone['typeB_list'].append(zone_vnum)
                idx += 1
                continue

            cmd = {'type': cmd_type}

            if cmd_type == 'M' and len(parts) >= 6:
                # M if_flag mob_vnum max_world room_vnum max_room
                cmd['type'] = 'LOAD_MOB'
                cmd['if_flag'] = int(parts[1]) if parts[1].isdigit() else 0
                cmd['mob_vnum'] = int(parts[2]) if parts[2].isdigit() else 0
                cmd['max_world'] = int(parts[3]) if parts[3].lstrip('-').isdigit() else 1
                cmd['room_vnum'] = int(parts[4]) if parts[4].lstrip('-').isdigit() else 0
                cmd['max_room'] = int(parts[5]) if parts[5].lstrip('-').isdigit() else -1
            elif cmd_type == 'O' and len(parts) >= 6:
                # O if_flag obj_vnum max room_vnum load_prob
                cmd['type'] = 'LOAD_OBJ'
                cmd['if_flag'] = int(parts[1]) if parts[1].isdigit() else 0
                cmd['obj_vnum'] = int(parts[2]) if parts[2].isdigit() else 0
                cmd['max'] = int(parts[3]) if parts[3].lstrip('-').isdigit() else -1
                cmd['room_vnum'] = int(parts[4]) if parts[4].isdigit() else 0
                cmd['load_prob'] = int(parts[5]) if parts[5].lstrip('-').isdigit() else 100
            elif cmd_type == 'G' and len(parts) >= 4:
                # G if_flag obj_vnum max
                cmd['type'] = 'GIVE_OBJ'
                cmd['if_flag'] = int(parts[1]) if parts[1].isdigit() else 0
                cmd['obj_vnum'] = int(parts[2]) if parts[2].isdigit() else 0
                cmd['max'] = int(parts[3]) if parts[3].lstrip('-').isdigit() else -1
                if len(parts) >= 6:
                    cmd['load_prob'] = int(parts[5]) if parts[5].lstrip('-').isdigit() else -1
            elif cmd_type == 'E' and len(parts) >= 5:
                # E if_flag obj_vnum max wear_pos [load_prob]
                cmd['type'] = 'EQUIP_MOB'
                cmd['if_flag'] = int(parts[1]) if parts[1].isdigit() else 0
                cmd['obj_vnum'] = int(parts[2]) if parts[2].isdigit() else 0
                cmd['max'] = int(parts[3]) if parts[3].lstrip('-').isdigit() else -1
                cmd['wear_pos'] = int(parts[4]) if parts[4].isdigit() else 0
                if len(parts) >= 6:
                    cmd['load_prob'] = int(parts[5]) if parts[5].lstrip('-').isdigit() else -1
            elif cmd_type == 'P' and len(parts) >= 6:
                # P if_flag obj_vnum max container_vnum load_prob
                cmd['type'] = 'PUT_OBJ'
                cmd['if_flag'] = int(parts[1]) if parts[1].isdigit() else 0
                cmd['obj_vnum'] = int(parts[2]) if parts[2].isdigit() else 0
                cmd['max'] = int(parts[3]) if parts[3].lstrip('-').isdigit() else -1
                cmd['container_vnum'] = int(parts[4]) if parts[4].isdigit() else 0
                cmd['load_prob'] = int(parts[5]) if parts[5].lstrip('-').isdigit() else 100
            elif cmd_type == 'D' and len(parts) >= 5:
                # D if_flag room_vnum direction state
                cmd['type'] = 'DOOR'
                cmd['if_flag'] = int(parts[1]) if parts[1].isdigit() else 0
                cmd['room_vnum'] = int(parts[2]) if parts[2].isdigit() else 0
                cmd['direction'] = int(parts[3]) if parts[3].isdigit() else 0
                cmd['state'] = int(parts[4]) if parts[4].isdigit() else 0
            elif cmd_type == 'R' and len(parts) >= 4:
                # R if_flag room_vnum obj_vnum
                cmd['type'] = 'REMOVE_OBJ'
                cmd['if_flag'] = int(parts[1]) if parts[1].isdigit() else 0
                cmd['room_vnum'] = int(parts[2]) if parts[2].isdigit() else 0
                cmd['obj_vnum'] = int(parts[3]) if parts[3].isdigit() else 0
            elif cmd_type == 'T' and len(parts) >= 4:
                # T if_flag trigger_type trigger_vnum [room_vnum]
                # room_vnum is only present for WLD_TRIGGER (type=2)
                cmd['type'] = 'TRIGGER'
                cmd['if_flag'] = int(parts[1]) if parts[1].isdigit() else 0
                cmd['trigger_type'] = int(parts[2]) if parts[2].isdigit() else 0
                cmd['trigger_vnum'] = int(parts[3]) if parts[3].lstrip('-').isdigit() else 0
                # For WLD_TRIGGER, parts[4] contains room_vnum
                if len(parts) > 4 and parts[4].lstrip('-').isdigit():
                    cmd['room_vnum'] = int(parts[4])
            elif cmd_type == 'V' and len(parts) >= 6:
                # V if_flag trigger_type id context var_name var_value
                cmd['type'] = 'VARIABLE'
                cmd['if_flag'] = int(parts[1]) if parts[1].isdigit() else 0
                cmd['trigger_type'] = int(parts[2]) if parts[2].isdigit() else 0
                cmd['context'] = int(parts[3]) if parts[3].isdigit() else 0
                cmd['var_vnum'] = int(parts[4]) if parts[4].isdigit() else 0
                cmd['var_name'] = parts[5] if len(parts) > 5 else ''
                cmd['var_value'] = ' '.join(parts[6:]) if len(parts) > 6 else ''
            elif cmd_type == 'Q' and len(parts) >= 4:
                # Q if_flag mob_vnum max
                cmd['type'] = 'EXTRACT_MOB'
                cmd['if_flag'] = int(parts[1]) if parts[1].isdigit() else 0
                cmd['mob_vnum'] = int(parts[2]) if parts[2].isdigit() else 0
                cmd['max'] = int(parts[3]) if parts[3].lstrip('-').isdigit() else -1
            elif cmd_type == 'F' and len(parts) >= 5:
                # F if_flag room_vnum leader_mob_vnum follower_mob_vnum
                cmd['type'] = 'FOLLOW'
                cmd['if_flag'] = int(parts[1]) if parts[1].isdigit() else 0
                cmd['room_vnum'] = int(parts[2]) if parts[2].isdigit() else 0
                cmd['leader_mob_vnum'] = int(parts[3]) if parts[3].isdigit() else 0
                cmd['follower_mob_vnum'] = int(parts[4]) if parts[4].isdigit() else 0
            else:
                # Unknown command - log warning and skip
                log_warning(f"Unknown zone command '{cmd_type}': {line}", vnum=zone.get('vnum'), filepath=str(filepath))
                idx += 1
                continue

            zone['commands'].append(cmd)
            idx += 1

    except Exception as e:
        log_error(f"Failed to parse zone: {e}", vnum=zone.get('vnum'), filepath=str(filepath))
        return None

    return zone


def zon_to_yaml(zone):
    """Convert zone dictionary to YAML string using ruamel.yaml."""
    data = CommentedMap()

    vnum = zone['vnum']
    data['vnum'] = vnum
    data.yaml_set_start_comment(f"Zone #{vnum}")

    if 'name' in zone:
        data['name'] = zone['name']

    # Metadata
    if zone.get('metadata'):
        meta = CommentedMap()
        for key in ['comment', 'location', 'author', 'description']:
            if key in zone['metadata']:
                value = zone['metadata'][key]
                meta[key] = to_literal_block(value) if key == 'description' else value
        if meta:
            data['metadata'] = meta

    # Builders
    if zone.get('builders'):
        data['builders'] = zone['builders']

    # Zone params
    if 'first_room' in zone:
        data['first_room'] = zone['first_room']
    if 'top_room' in zone:
        data['top_room'] = zone['top_room']
    if 'mode' in zone:
        data['mode'] = zone['mode']
    if 'zone_type' in zone:
        data['zone_type'] = zone['zone_type']
    if zone.get('zone_group'):
        data['zone_group'] = zone['zone_group']
    if 'entrance' in zone:
        data['entrance'] = zone['entrance']
    if 'lifespan' in zone:
        data['lifespan'] = zone['lifespan']
    if 'reset_mode' in zone:
        data['reset_mode'] = zone['reset_mode']
    if 'reset_idle' in zone:
        data['reset_idle'] = zone['reset_idle']
    if zone.get('under_construction'):
        data['under_construction'] = zone['under_construction']

    # Zone grouping lists
    if zone.get('typeA_list'):
        typeA = CommentedSeq()
        for i, z in enumerate(zone['typeA_list']):
            typeA.append(z)
            zone_name = get_zone_name(z)
            if zone_name:
                typeA.yaml_add_eol_comment(zone_name, i)
        data['typeA_zones'] = typeA

    if zone.get('typeB_list'):
        typeB = CommentedSeq()
        for i, z in enumerate(zone['typeB_list']):
            typeB.append(z)
            zone_name = get_zone_name(z)
            if zone_name:
                typeB.yaml_add_eol_comment(zone_name, i)
        data['typeB_zones'] = typeB

    # Commands with name comments
    if zone.get('commands'):
        cmds = CommentedSeq()
        for cmd in zone['commands']:
            c = CommentedMap()
            c['type'] = cmd['type']

            if 'if_flag' in cmd:
                c['if_flag'] = cmd['if_flag']

            # Add fields with name comments
            if 'mob_vnum' in cmd:
                c['mob_vnum'] = cmd['mob_vnum']
                name = get_mob_name(cmd['mob_vnum'])
                if name:
                    c.yaml_add_eol_comment(name, 'mob_vnum')

            if 'obj_vnum' in cmd:
                c['obj_vnum'] = cmd['obj_vnum']
                name = get_obj_name(cmd['obj_vnum'])
                if name:
                    c.yaml_add_eol_comment(name, 'obj_vnum')

            if 'room_vnum' in cmd:
                c['room_vnum'] = cmd['room_vnum']
                name = get_room_name(cmd['room_vnum'])
                if name:
                    c.yaml_add_eol_comment(name, 'room_vnum')

            if 'container_vnum' in cmd:
                c['container_vnum'] = cmd['container_vnum']
                name = get_obj_name(cmd['container_vnum'])
                if name:
                    c.yaml_add_eol_comment(name, 'container_vnum')

            if 'trigger_vnum' in cmd:
                c['trigger_vnum'] = cmd['trigger_vnum']
                name = get_trigger_name(cmd['trigger_vnum'])
                if name:
                    c.yaml_add_eol_comment(name, 'trigger_vnum')

            # FOLLOW command fields
            if 'leader_mob_vnum' in cmd:
                c['leader_mob_vnum'] = cmd['leader_mob_vnum']
                name = get_mob_name(cmd['leader_mob_vnum'])
                if name:
                    c.yaml_add_eol_comment(name, 'leader_mob_vnum')

            if 'follower_mob_vnum' in cmd:
                c['follower_mob_vnum'] = cmd['follower_mob_vnum']
                name = get_mob_name(cmd['follower_mob_vnum'])
                if name:
                    c.yaml_add_eol_comment(name, 'follower_mob_vnum')

            # Wear position with comment
            if 'wear_pos' in cmd:
                c['wear_pos'] = cmd['wear_pos']
                pos_name = get_wear_pos_name(cmd['wear_pos'])
                if pos_name:
                    c.yaml_add_eol_comment(pos_name, 'wear_pos')

            # Direction with comment
            if 'direction' in cmd:
                c['direction'] = cmd['direction']
                dir_name = get_direction_name(cmd['direction'])
                if dir_name:
                    c.yaml_add_eol_comment(dir_name, 'direction')

            # Other fields
            for key in ['max_world', 'max_room', 'max', 'load_prob',
                       'state', 'trigger_type', 'entity_vnum',
                       'context', 'var_vnum', 'var_name', 'var_value']:
                if key in cmd:
                    c[key] = cmd[key]

            cmds.append(c)
        data['commands'] = cmds

    return yaml_dump_to_string(data)


def read_index_file(index_path):
    """Read an index file and return set of enabled filenames.

    Index file format:
        - One filename per line
        - Lines starting with $ indicate end of index
        - Empty lines are ignored

    Returns:
        set of filenames (e.g., {'1.zon', '2.zon', ...}) or None if index doesn't exist
    """
    if not index_path.exists():
        return None

    enabled_files = set()
    try:
        with open(index_path, 'r', encoding='koi8-r', errors='replace') as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith('$'):
                    break
                enabled_files.add(line)
    except Exception as e:
        log_warning(f"Failed to read index file: {e}", filepath=str(index_path))
        return None

    return enabled_files


def convert_file(input_path, output_path, file_type):
    """Convert a single file from old format to YAML (legacy function for single file mode)."""
    input_path = Path(input_path)
    output_path = Path(output_path)

    # Ensure output directory exists
    output_path.parent.mkdir(parents=True, exist_ok=True)

    try:
        if file_type == 'mob':
            entities = parse_mob_file(input_path)
            for entity in entities:
                yaml_content = mob_to_yaml(entity)
                out_file = output_path.parent / f"{entity['vnum']}.yaml"
                with open(out_file, 'w', encoding='koi8-r') as f:
                    f.write(yaml_content)
        elif file_type == 'obj':
            entities = parse_obj_file(input_path)
            for entity in entities:
                yaml_content = obj_to_yaml(entity)
                out_file = output_path.parent / f"{entity['vnum']}.yaml"
                with open(out_file, 'w', encoding='koi8-r') as f:
                    f.write(yaml_content)
        elif file_type == 'wld':
            entities = parse_wld_file(input_path)
            for entity in entities:
                yaml_content = room_to_yaml(entity)
                out_file = output_path.parent / f"{entity['vnum']}.yaml"
                with open(out_file, 'w', encoding='koi8-r') as f:
                    f.write(yaml_content)
        elif file_type == 'zon':
            zone = parse_zon_file(input_path)
            if zone:
                yaml_content = zon_to_yaml(zone)
                out_file = output_path.parent / f"{zone['vnum']}.yaml"
                with open(out_file, 'w', encoding='koi8-r') as f:
                    f.write(yaml_content)
        elif file_type == 'trg':
            entities = parse_trg_file(input_path)
            for entity in entities:
                yaml_content = trg_to_yaml(entity)
                out_file = output_path.parent / f"{entity['vnum']}.yaml"
                with open(out_file, 'w', encoding='koi8-r') as f:
                    f.write(yaml_content)

        return True
    except Exception as e:
        log_error(f"Failed to convert file: {e}", filepath=str(input_path))
        return False


def parse_file(input_path, file_type):
    """Parse a file and return list of entities.

    Args:
        input_path: Path to the input file
        file_type: Type of file (mob, obj, wld, zon, trg)

    Returns:
        List of (file_type, entity) tuples, or empty list on error
    """
    try:
        if file_type == 'mob':
            entities = parse_mob_file(input_path)
            return [(file_type, e) for e in entities]
        elif file_type == 'obj':
            entities = parse_obj_file(input_path)
            return [(file_type, e) for e in entities]
        elif file_type == 'wld':
            entities = parse_wld_file(input_path)
            return [(file_type, e) for e in entities]
        elif file_type == 'zon':
            zone = parse_zon_file(input_path)
            return [(file_type, zone)] if zone else []
        elif file_type == 'trg':
            entities = parse_trg_file(input_path)
            return [(file_type, e) for e in entities]
        return []
    except Exception as e:
        log_error(f"Failed to parse file: {e}", filepath=str(input_path))
        return []


def create_world_config(output_dir):
    """Create world_config.yaml for YAML loader.

    Args:
        output_dir: Output directory (should contain world/)
    """
    output_path = Path(output_dir)

    # Determine if we're writing to world/ or to output_dir directly
    # Check if output_path has a 'world' subdirectory with YAML files
    world_subdir = output_path / 'world'
    if world_subdir.exists() and (world_subdir / 'mobs').exists():
        config_path = world_subdir / 'world_config.yaml'
    else:
        config_path = output_path / 'world_config.yaml'

    # Set line_endings based on literal blocks usage
    if _use_literal_blocks:
        line_endings = "dos"  # Literal blocks use LF, need conversion to CR+LF
        comment = "dos (literal blocks: LF converted back to CR+LF)"
    else:
        line_endings = "unix"  # Quoted strings preserve CR+LF, no conversion
        comment = "unix (quoted strings: CR+LF preserved)"

    config_content = f"""# World configuration for YAML loader
# Generated by convert_to_yaml.py

# Line ending style in text fields
# - dos: CR+LF (\\r\\n) - convert LF to CR+LF (for literal blocks)
# - unix: LF (\\n) - no conversion (for quoted strings)
line_endings: {line_endings}  # {comment}
"""

    with open(config_path, 'w') as f:
        f.write(config_content)

    print(f"Created world configuration: {config_path}")


def convert_directory(input_dir, output_dir, delete_source=False, max_workers=None,
                     output_format='yaml', db_path=None):
    """Convert all files in a world directory.

    Architecture:
        - Parsing: parallel (ThreadPoolExecutor, N threads)
        - Saving: sequential (single thread, due to GIL for YAML / DB safety for SQLite)

    Args:
        input_dir: Input directory containing world files
        output_dir: Output directory for YAML files or database
        delete_source: If True, delete source files after successful conversion
        max_workers: Number of parallel workers for parsing (default: CPU count)
        output_format: 'yaml' or 'sqlite'
        db_path: Path to SQLite database (for sqlite format)
    """
    input_path = Path(input_dir)
    output_path = Path(output_dir)

    # Default to CPU count for parsing
    if max_workers is None:
        max_workers = os.cpu_count() or 4

    # Choose saver based on output format
    if output_format == 'sqlite':
        if db_path is None:
            db_path = output_path / 'world.db'
        saver = SqliteSaver(db_path)
    else:
        saver = YamlSaver(output_path)

    # Track source files for deletion
    source_files_to_delete = []

    # Type mapping: dir_name -> (file_type, extension)
    type_mapping = {
        'mob': ('mob', '.mob'),
        'obj': ('obj', '.obj'),
        'wld': ('wld', '.wld'),
        'zon': ('zon', '.zon'),
        'trg': ('trg', '.trg')
    }

    with saver:
        for dir_name, (file_type, extension) in type_mapping.items():
            source_dir = input_path / dir_name
            if not source_dir.exists():
                continue

            # Read index file to determine which files are enabled
            index_path = source_dir / 'index'
            enabled_files = read_index_file(index_path)

            # Find all files
            all_files = list(source_dir.glob(f"*{extension}"))
            # Filter to only files matching pattern: <number>.<ext> (ignores backup files like 16.old.obj)
            valid_pattern = re.compile(r"^\d+" + re.escape(extension) + r"$")
            files = [f for f in all_files if valid_pattern.match(f.name)]
            if not files:
                continue

            # Count enabled files
            if enabled_files is not None:
                enabled_count = sum(1 for f in files if f.name in enabled_files)
                disabled_count = len(files) - enabled_count
                index_info = f", {enabled_count} indexed, {disabled_count} extra"
            else:
                index_info = ", no index"

            lib_info = f", {_yaml_library}" if output_format == 'yaml' else ""
            print(f"Converting {len(files)} {file_type} files ({max_workers} parsers{lib_info}{index_info})...")

            # Parallel parsing
            all_entities = []
            with ThreadPoolExecutor(max_workers=max_workers) as executor:
                futures = {executor.submit(parse_file, f, file_type): f for f in files}

                for future in as_completed(futures):
                    f = futures[future]
                    try:
                        entities = future.result()
                        # Mark entities as enabled/disabled based on index
                        is_enabled = 1 if (enabled_files is None or f.name in enabled_files) else 0
                        for etype, entity in entities:
                            entity['enabled'] = is_enabled
                        all_entities.extend(entities)
                        if entities and delete_source:
                            source_files_to_delete.append(f)
                    except Exception as e:
                        log_error(f"Parser thread error: {e}", filepath=str(f))

            # Sequential saving (due to GIL / DB safety)
            for file_type, entity in all_entities:
                try:
                    if file_type == 'mob':
                        saver.save_mob(entity)
                    elif file_type == 'obj':
                        saver.save_object(entity)
                    elif file_type == 'wld':
                        saver.save_room(entity)
                    elif file_type == 'zon':
                        saver.save_zone(entity)
                    elif file_type == 'trg':
                        saver.save_trigger(entity)
                except Exception as e:
                    vnum = entity.get('vnum', 'unknown')
                    log_error(f"Failed to save {file_type} {vnum}: {e}")

        # Finalize saver (create index files for YAML, print stats for SQLite)
        saver.finalize()

    # Create world config for YAML format
    if output_format == 'yaml':
        create_world_config(output_dir)

    # Delete source files if requested
    if delete_source and source_files_to_delete:
        print(f"Deleting {len(source_files_to_delete)} source files...")
        for f in source_files_to_delete:
            try:
                f.unlink()
            except Exception as e:
                log_warning(f"Failed to delete source file: {e}", filepath=str(f))


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description='Convert MUD world files to YAML or SQLite format',
        epilog='''Examples:
  python3 convert_to_yaml.py -i lib.template -o lib                    # Convert to YAML
  python3 convert_to_yaml.py -i lib.template -o lib -f sqlite          # Convert to SQLite
  python3 convert_to_yaml.py -i lib.template -o lib -f sqlite --db world.db  # Custom DB path
'''
    )
    parser.add_argument('--input', '-i', required=True,
                       help='Input lib directory (containing world/) or single file')
    parser.add_argument('--output', '-o', required=True,
                       help='Output lib directory or database directory')
    parser.add_argument('--type', '-t', choices=['mob', 'obj', 'wld', 'zon', 'trg', 'all'],
                       default='all', help='File type to convert (default: all)')
    parser.add_argument('--format', '-f', choices=['yaml', 'sqlite'],
                       default='yaml', help='Output format: yaml or sqlite (default: yaml)')
    parser.add_argument('--db', type=str, default=None,
                       help='SQLite database path (for --format sqlite, default: <output>/world.db)')
    parser.add_argument('--delete-source', action='store_true',
                       help='Delete source files after successful conversion')
    default_workers = os.cpu_count() or 4
    parser.add_argument('--workers', '-w', type=int, default=default_workers,
                       help=f'Number of parallel workers (default: {default_workers})')
    parser.add_argument('--yaml-lib', choices=['ruamel', 'pyyaml'], default='ruamel',
                       help='YAML library: ruamel (with comments, default) or pyyaml (fast, no literal blocks)')

    args = parser.parse_args()

    # Set global YAML library choice and initialize
    global _yaml_library, _use_literal_blocks
    _yaml_library = args.yaml_lib
    _use_literal_blocks = (_yaml_library == 'ruamel')  # ruamel automatically uses literal blocks
    if args.format == 'yaml':
        _init_yaml_libraries()

    input_path = Path(args.input)
    output_path = Path(args.output)

    if args.type == 'all':
        # Convert entire directory
        if not input_path.is_dir():
            print(f"Error: {input_path} is not a directory", file=sys.stderr)
            sys.exit(1)

        # Look for world/ subdirectory
        world_dir = input_path / 'world'
        if not world_dir.exists():
            # Maybe the input is already the world directory
            if (input_path / 'mob').exists() or (input_path / 'wld').exists():
                world_dir = input_path
            else:
                print(f"Error: Cannot find world directory in {input_path}", file=sys.stderr)
                sys.exit(1)

#         # Build name registries from output directory if it exists (only for YAML format)
#         if args.format == 'yaml' and output_path.exists():
#             build_name_registries(output_path / 'world')

        convert_directory(world_dir, output_path, delete_source=args.delete_source,
                         max_workers=args.workers, output_format=args.format,
                         db_path=args.db)
    else:
        # Convert single file (only YAML supported for single file mode)
        if args.format == 'sqlite':
            print("Error: SQLite format is only supported for directory conversion", file=sys.stderr)
            sys.exit(1)
        convert_file(input_path, output_path, args.type)

    print_summary()
    print("Conversion complete!")


if __name__ == '__main__':
    main()
