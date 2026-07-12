# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is **Bylins MUD** - a Russian-language Multi-User Dungeon game server based on CircleMUD/DikuMUD. The codebase is ~272K lines of C++20 code with extensive Russian comments and variable names. It implements a complete multiplayer text-based RPG with combat, magic, crafting, quests, and scripting systems.

## Versioning Policy

The engine is named **BRusMUD** and carries an explicit four-part version **`major.minor.patch.release`** (e.g. `0.1.0.12810`), shown by the in-game `version` command and in the boot log.

- **major.minor.patch** are declared explicitly in the repo-root **`VERSION.txt`** file — the single source of truth.
- **release** (the 4th number) is generated automatically at build time from the git commit count (`git rev-list --count HEAD`, via `tools/meson/generate_version.py`). **Never edit it by hand** — it advances on its own with every commit.

### When to bump (rules for Claude Code)

- **patch** — bump **manually and without prompting** on **every bug fix and every feature-branch merge**. Count the merges: if two branches are merged (e.g. one into `master` and one into `unstable`), increase patch by **2**. Treat editing `VERSION.txt` as a routine step of finishing such work, included in the same commit/merge.
- **minor** — bump when a **major rework** lands (for example, the data-driven effects/affects system would qualify). Reset patch to `0`.
- **major** — bump **only when the user explicitly says so**. Reset minor and patch to `0`. You **may prompt** the user to consider a major bump once **minor reaches 100 or more**.

Only the `VERSION.txt` file changes for a version bump; `release` and the build-date stamp update themselves on the next build.

## Build Commands

### Initial Setup (Ubuntu/WSL)
```bash
sudo apt update && sudo apt upgrade
sudo apt install build-essential meson ninja-build libssl-dev libcurl4-gnutls-dev libexpat1-dev gettext unzip gdb libgtest-dev zlib1g-dev
git clone --recurse-submodules https://github.com/bylins/mud
cd mud
cp -n -r lib.template/* lib
```

### Если репозиторий уже склонирован без submodules

```bash
git submodule update --init --recurse-submodules
```

### Standard Build (without tests)
```bash
meson setup build -Dbuild_tests=false -Dbuild_profile=release
ninja -C build -j$(($(nproc)/2))
./build/circle 4000  # Start server on port 4000
```

### Build with Tests
```bash
meson setup build -Dbuild_profile=release -Dbuild_tests=true
ninja -C build -j$(($(nproc)/2))
./build/tests/tests  # Run all tests
```

**Important:** Always use `-j$(($(nproc)/2))` for parallel builds to avoid overloading the system.

**Important:** Always use `-Dbuild_profile=release` for development builds. The `dev` and `fasttest` profiles are legacy — do not use them. Never build without tests first and then rebuild with tests — pass `-Dbuild_tests=true` to `meson setup` from the start to avoid double compilation.

**Important:** YAML is the **default world format** now (`yaml=builtin`), so a plain build expects a YAML world (`world/world_config.yaml` + `zones/`). To boot a legacy-format world, configure with `-Dyaml=disabled` (and `-Dsqlite=disabled`). See "World Data Formats and Testing" below.

### Build Profiles (`-Dbuild_profile=`)
- **release** - Optimized production build (-Og with debug symbols, ggdb3, -Wall)
- **debug** - Debug build (-O0, ggdb3); add `-Dwith_asan=true` for AddressSanitizer
- **dev** - Test build with optimizations (-O3, -DTEST_BUILD)
- **fasttest** - Fast test build (-Ofast, -DTEST_BUILD)
- **custom** - No specific flags, fully user-controlled

### Reconfiguring an existing build directory
```bash
meson configure build_yaml -Dyaml=builtin   # turn yaml on
meson configure build_yaml                  # list current options
```

### Running the Server

**CRITICAL**: ALL binaries (circle, tests, converters, etc.) must ALWAYS be run from their build directory, NEVER from the source directory. Running binaries from the source directory creates runtime files (data/, misc/, etc.) in the wrong location and pollutes the source tree.

**CRITICAL**: Build directories must ALWAYS be out-of-source (outside the source tree). Never run `meson setup`/`ninja` or binaries from within the src/ directory or repository root.

**CRITICAL**: With `-Dsmall_world=true` meson copies test world data into `small/` inside the build directory (see `tools/meson/setup_world.py`). All world data and configs live in `small/` itself, NOT in `small/lib/`. The `lib/` subdirectory DOES NOT EXIST in meson-generated worlds. All paths in configuration.xml are relative to the `small/` directory.

**CRITICAL**: The `small/` world is **YAML-only** (`lib.template` ships flat YAML, no legacy files). YAML is the default build format, so a plain build boots it; a Legacy/SQLite build (`-Dyaml=disabled`) cannot load `-d small`.

**Correct usage**:
```bash
cd build_debug       # Or build_yaml, build_sqlite, etc.
./circle [-W] -d small <port>
```

**Parameters**:
- `-W` - Enable world checksum calculation (optional)
- `-d <world_directory>` - Specify world data directory (e.g., `small`)
- `<port>` - Port number to listen on (e.g., `4000`)

**Example**:
```bash
cd build_debug
./circle -d small 4000
```

**World Structure (meson-generated):**
```
build_debug/
├── circle           # Binary
└── small/           # Test world (created by meson with -Dsmall_world=true)
    ├── misc/
    │   └── configuration.xml
    ├── world/       # Zone files
    ├── etc/         # Additional configs
    └── admin_api.sock  # Unix socket (if Admin API enabled)
```

**NOTE**: Do NOT confuse with repository's `lib/` directory - that is completely separate and used only for production deployments.

### Running Tests
Тесты используют относительные пути к данным (`data/boards/...`, `misc/grouping`, `data/mob_classes/...`), которые meson копирует в `meson.project_build_root()`. Поэтому запускать тесты надо из самой `build_dir`, иначе `Boards_Changelog`, `FightPenalties` и `MobClassesLoaderTest` упадут с «file not found».

```bash
# Способ 1 — через meson (workdir выставляется автоматически)
meson test -C build

# Способ 2 — напрямую (cd в build, иначе пути к данным не найдутся)
cd build && ./tests/tests
```

Фильтр по подмножеству:
```bash
cd build && ./tests/tests --gtest_filter="Boards_Changelog.*:FightPenalties.*"
```

## Code Architecture

### High-Level Organization

The codebase is organized into three major sections:

**`src/engine/`** - Core MUD infrastructure (network, database, scripting VM, entities)
- `boot/` - Initialization and data file loading
- `core/` - Game loop (25Hz heartbeat), communication layer, command processing
- `db/` - World persistence, player save/load, InfluxDB stats integration
- `entities/` - Core entity classes: `CharData`, `ObjData`, `RoomData`, `Zone`
- `network/` - Socket handling, telnet protocol, MSDP, MCCP compression
- `olc/` - Online creation editors (Oedit, Redit, Medit, Zedit, Trigedit)
- `scripting/` - DG Scripts trigger system (20+ trigger types for mobs/objects/rooms)
- `structs/` - Data structures (compact trie, radix trie, blocking queue, flags)
- `ui/` - User interface and command system
  - `cmd/` - 100+ player commands (do_*.cpp/h)
  - `cmd_god/` - Admin/immortal commands

**`src/gameplay/`** - Game-specific mechanics (17 subsystems)
- `abilities/` - Feats and talents system
- `affects/` - Temporary status effects (buffs/debuffs)
- `ai/` - Mobile AI, pathfinding, spec_procs
- `clans/` - Guild/house system
- `classes/` - Character and mob classes with progression
- `communication/` - Boards, mail, messaging, social
- `core/` - Base stats, character generation, constants
- `crafting/` - Item creation, jewelry, mining, frying
- `economics/` - Shops, auction, exchange, currencies
- `fight/` - Combat system with hit calculation, penalties, assists
- `magic/` - 108+ spells, spell effects, magic items/rooms
- `mechanics/` - Core gameplay (corpses, equipment, doors, mounts, etc.)
- `quests/` - Daily quests, quest tracking
- `skills/` - Skill system (backstab, bash, kick, morph, townportal, etc.)
- `statistics/` - DPS tracking, money drops, zone stats, spell usage

**`src/administration/`** - User management
- Account system, password handling, name validation
- Ban lists, proxy detection, punishments (karma), privileges

### Key Architectural Concepts

**Pulse-Based Game Loop**
- Runs at 25Hz (40ms per tick, defined as kOptUsec = 0.1s)
- Central `Heartbeat` class manages all periodic actions
- Actions registered as `PulseStep` objects with modulo (frequency) and offset
- Common pulses: `kPulseZone` (1 sec), `PULSE_DG_SCRIPT` (13 sec), mob activity (10 pulses)
- All timing is pulse-relative, not wall-clock time

**Entity Model**
- `CharData` - Base class for both PCs and NPCs (HP, stats, skills, affects, inventory)
- `CharPlayer` - PC-specific data (title, time played, kill lists)
- `ObjData` - Items with type-specific values, affects, and triggers
- `RoomData` - Locations with exits, sector type, inhabitants, and triggers
- `Zone` - Collections of rooms with reset rules

**Global Registry Pattern**
Access world state via `MUD::` namespace functions:
- `MUD::heartbeat()` - Main game clock
- `MUD::world_objects()`, `MUD::characters()` - Entity collections
- `MUD::Skills()`, `MUD::Spells()`, `MUD::Classes()` - Game data
- `MUD::trigger_list()` - Script system access

**DG Scripts Trigger System**
- Event-driven scripting for mobs (MTRIG), objects (OTRIG), and rooms (WTRIG)
- 20+ trigger types (GREET, COMMAND, SPEECH, FIGHT, RANDOM, etc.)
- Scripts execute as linked list of commands with wait/pause support
- Variable scoping (global vs trigger context), 512-depth recursion limit
- Commands: `act`, `send`, `if/else`, `wait`, `force`, target checks

**Vnum vs Rnum**
- **Vnum** (Virtual Number) - Persistent database ID
- **Rnum** (Real Number) - In-memory array index for fast lookup
- Used for rooms, objects, mobs, zones, triggers

**Network Architecture**
- Multiplexing via epoll (Linux) or select (fallback)
- One `DescriptorData` per connection with state machine (18 states)
- Telnet protocol with MSDP (Mud Server Data Protocol) and MCCP compression
- Multiple codepage support (Alt, Win, UTF-8, KOI8-R)

## Development Guidelines (from CONTRIBUTING.md)

### Code Style
- **Standard**: C++20 (C++17 minimum)
- **Indentation**: Tabs (size 4), no spaces for indentation
- **Braces**: An opening brace on the same line separated by a space
```cpp
if (condition) {
    // code here
}
```
- **One statement per line** (applies to variable declarations too)
- **Always use braces** for if/else/for/while bodies, even single statements
- **Pointers/References**: `*` and `&` attached to type, space after
```cpp
const char* message = "example";
const auto& reference = message;
```

### File Management
- All new files must be added to `meson.build` (sources go into `main_sources` / `library_sources`, depending on the section)
- Include vim modeline at end of files:
```cpp
// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
```

### Memory Management
- Prefer `new`/`delete` over `malloc`/`free`
- Use smart pointers (`CharData::shared_ptr`, `ObjData::shared_ptr`) where possible
- Avoid raw pointers for ownership

### Testing
- Run unit tests before committing: `./build/tests/tests`
- Write tests for new code using GoogleTest framework
- Tests are in `tests/` directory, named by module (e.g., `char.affects.cpp`, `fight.penalties.cpp`)

### Compiler Warnings
- Must compile on Windows, Linux, and Cygwin
- Fix all compiler warnings, especially in new code
- Build with `-Wall -Wextra` enabled

## Common Development Patterns

### Adding a New Command
1. Create `src/engine/ui/cmd/do_mycommand.h` and `.cpp`
2. Add function signature: `void do_mycommand(CharData *ch, char *argument, int cmd, int subcmd)`
3. Register in `src/engine/ui/interpreter.cpp` command table
4. Add the `.cpp` to the appropriate sources list in `meson.build`

### Adding a New Skill
1. Define skill in `src/gameplay/skills/skills_info.cpp`
2. Create handler in `src/gameplay/skills/myskill.h` and `.cpp`
3. Hook into skill system via `SkillRollCheck()` or direct calls
4. Add the `.cpp` to the appropriate sources list in `meson.build`

### Adding a DG Script Trigger Type
1. Define trigger constant in `src/engine/scripting/dg_scripts.h`
2. Add trigger handling in appropriate file (`dg_mobcmd.cpp`, `dg_objcmd.cpp`, `dg_wldcmd.cpp`)
3. Update OLC editor to support new trigger type

### Reading Player Data
```cpp
CharData *ch = /* get character */;
int hp = ch->get_hit();  // Current HP
int max_hp = ch->get_max_hit();  // Max HP
ch->send_to_char("Message to player\r\n");
```

### Sending Messages
```cpp
SendMsgToChar(ch, "Formatted %s message %d\r\n", str, num);
act("$n does something.", false, ch, 0, 0, kToRoom);  // To room
act("You do something.", false, ch, 0, 0, kToChar);   // To char
```

### Working with Objects
```cpp
ObjData::shared_ptr obj = /* get object */;
obj->get_carried_by();  // Character carrying it
obj->get_in_room();     // Room it's in
obj->get_in_obj();      // Container it's in
PlaceObjToInventory(obj, ch);  // Give to character
PlaceObjToRoom(obj, room);     // Place in room
```

### Accessing World Data
```cpp
RoomData *room = world[ch->in_room];  // Get room by rnum
room->people;  // List of characters in room
room->contents;  // List of objects in room
```

## Important File Locations

- **Main entry**: `src/main.cpp` → calls `main_function()` in `src/engine/core/comm.cpp`
- **Game loop**: `src/engine/core/comm.cpp:game_loop()`
- **Heartbeat**: `src/engine/core/heartbeat.cpp`
- **Command interpreter**: `src/engine/ui/interpreter.cpp`
- **Character data**: `src/engine/entities/char_data.h`
- **Object data**: `src/engine/entities/obj_data.h`
- **Room data**: `src/engine/entities/room_data.h`
- **DG Scripts**: `src/engine/scripting/dg_*.cpp`
- **Configuration**: `lib/misc/configuration.xml`

## Game Data Files

Game content is stored in `lib/`:
```
lib/
├── cfg/           # Configuration files
├── world/         # Zone files (rooms/objects/mobs)
├── etc/           # Equipment, misc data
├── text/          # Help files, greetings
└── misc/          # Miscellaneous (grouping, noob_help.xml, configuration.xml)
```

Copy template data: `cp -n -r lib.template/* lib`

## Testing Strategy

**Unit Tests** - GoogleTest framework in `tests/`
- Test individual components (tries, queues, parsers, character affects)
- Example: `tests/compact.trie.cpp`, `tests/char.affects.cpp`

**Integration Tests** - Test build with `-DTEST_BUILD -DNOCRYPT`
- Disables encryption for testing
- Enables auto-flush logging (`-DLOG_AUTOFLUSH`)

## Performance Measurement

The heartbeat system includes built-in profiling:
- Every pulse step is measured (min/max/avg time)
- Access via `Heartbeat::measurements()`
- Rolling window of 1000 measurements per step
- Use for identifying slow operations

## Russian Language Support

- Codebase is heavily Russified (comments, variable names)
- Grammar system handles Russian cases/declensions (`src/utils/grammar/cases.cpp`)
- Multiple character name forms (nominative, genitive, dative, etc.)
- Cyrillic codepage support (Alt, Win, UTF-8, KOI8-R)
- Russian holidays/celebrations system in `src/gameplay/mechanics/celebrates.cpp`

## Debugging Tips

- Use `log()` function for logging to syslog
- Debug builds include ASAN (AddressSanitizer) for memory issues
- GDB-friendly: builds include `-ggdb3 -rdynamic` flags
- Crash logs go to `syslog.CRASH`
- Set breakpoints in command handlers (e.g., `do_look`, `do_kill`)

## Critical Notes

- **File Encoding**: All project source files MUST be in KOI8-R encoding (not UTF-8). This is critical for proper Russian text handling in the codebase. Use `iconv` to convert if needed: `iconv -f utf8 -t koi8-r input.cpp > output.cpp`
- **Thread Safety**: Main game loop is single-threaded; use `BlockingQueue` for cross-thread communication
- **Shared Pointers**: Always use `CharData::shared_ptr` and `ObjData::shared_ptr` to prevent use-after-free
- **Pulse Timing**: Never use wall-clock delays; register actions with heartbeat system
- **Script Depth**: DG Scripts limited to 512 recursion depth to prevent stack overflow
- **Runtime Encoding**: While source files are KOI8-R, runtime text can be multiple encodings (Alt, Win, UTF-8, KOI8-R) based on client settings

## Claude Code Workflow Rules

### Build Directory Convention
Use separate build directories for different meson configurations to avoid lengthy rebuilds:
```
build/        - default build (without optional features)
build_sqlite/ - build with -Dsqlite=builtin
build_yaml/   - build with -Dyaml=builtin
build_debug/  - debug build with -Dbuild_profile=debug
build_otel/   - build with -Dotel=builtin (requires vcpkg)
build_test/   - test data and converted worlds (not for compilation)
```
Reconfigure an existing dir with `meson configure build_dir -Dyaml=builtin` instead of recreating it from scratch when only options change.

**Always warn the user when changing build directories or running `meson setup`/`meson configure`/`ninja` in a different directory.**

### OpenTelemetry Build (`-Dotel=builtin`)
opentelemetry-cpp is installed via vcpkg at `~/repos/vcpkg`. Point meson at the vcpkg pkg-config / cmake search paths:
```bash
PKG_CONFIG_PATH=~/repos/vcpkg/installed/x64-linux/lib/pkgconfig \
CMAKE_PREFIX_PATH=~/repos/vcpkg/installed/x64-linux \
meson setup build_otel \
  -Dbuild_profile=release \
  -Dotel=system
ninja -C build_otel -j$(($(nproc)/2))
```

### File Encoding - CRITICAL
**Proper workflow for editing KOI8-R files:**

For files marked as `working-tree-encoding=KOI8-R` in .gitattributes (all files in /src/**, /tests/**):

```bash
# 1. Convert to UTF-8 for editing
iconv -f koi8-r -t utf-8 src/file.cpp > /tmp/file_utf8.cpp

# 2. Edit the UTF-8 version with Edit tool or text editor
# (make your changes here)

# 3. Convert back to KOI8-R
iconv -f utf-8 -t koi8-r /tmp/file_utf8.cpp > src/file.cpp
```

**NEVER use the Edit tool directly on existing .cpp/.h files that contain Russian text.**
Only use Edit for:
- Newly created files that will be pure ASCII/English
- Temporary UTF-8 converted files (in /tmp)
- Files in .gitattributes marked as UTF-8 (e.g., Python files)

**NEVER use sed for editing source files.** Sed has tendency to:
- Modify files in unexpected places (matching wrong lines)
- Lead to file corruption detection and accidental `git checkout` (losing all uncommitted work)
- Cause cumulative errors from multiple sed operations
- Corrupt KOI8-R encoding

**Alternative: unified diff patches** - for small targeted changes:
```bash
cat > /tmp/fix.patch << 'PATCH'
--- a/src/file.cpp
+++ b/src/file.cpp
@@ -10,3 +10,4 @@
 existing line
-old line
+new line
PATCH
patch -p1 < /tmp/fix.patch
```
Patches preserve encoding and fail cleanly if context doesn't match.

### _GLIBCXX_DEBUG Disabled in Debug Build
**Important:** `_GLIBCXX_DEBUG` is intentionally **disabled** for Debug builds due to ABI incompatibility with external libraries (yaml-cpp, SQLite).

**Why:**
- External libraries (yaml-cpp) are compiled without `_GLIBCXX_DEBUG`
- They return STL objects (`std::string`) to our code
- If our code uses `_GLIBCXX_DEBUG`, ABI mismatch causes heap-buffer-overflow
- Solution: disable the flag for all Debug builds

**Trade-off:** We lose STL iterator/bounds checking in Debug mode, but gain ASAN (AddressSanitizer) which catches most memory errors.

### Directory Change Notifications
Always explicitly notify the user before:
- Running `meson setup`/`meson configure` in a different build directory
- Running `ninja` in a different build directory
- Changing the working directory for any build operation

Example: "Switching to build_sqlite/ directory for SQLite-enabled build."

### World Data Formats and Testing

The project supports three world data formats. The runtime picks one at
compile time by priority: **YAML > SQLite > Legacy** (`#ifdef HAVE_YAML` first,
see `src/engine/db/db.cpp`).
1. **YAML** - Human-readable YAML format. **This is the default build format** (`yaml=builtin`); it's what `lib.template/` ships.
2. **SQLite** - World data in SQLite database (requires `-Dsqlite=builtin` or `-Dsqlite=system`, plus `-Dyaml=disabled` so it wins the format pick)
3. **Legacy** - Original CircleMUD text format (full-world archives only; no longer shipped in `lib.template/`). Requires `-Dyaml=disabled -Dsqlite=disabled`.

**The template/small world is YAML-only.** `lib.template/world` contains flat YAML
(`zones/`, `dictionaries/`, `world_config.yaml`); the legacy `mob/obj/wld/zon/trg/shp`
files were removed. Since YAML is the default build format, a plain build boots it;
a Legacy/SQLite build (`-Dyaml=disabled`) can **no longer** load the small/template
world. Shops come from `cfg/economics/shops.xml`, not `world/shp`.

**CRITICAL: Never use lib/ from repository directly!**
- `lib/` contains base configuration files only (NOT complete world data)
- `lib.template/` overlays the YAML world files, player data, and additional configs
- To get a working world: copy lib/ to build directory, then overlay lib.template/

**Conversion Tool (full world only):**
The converter is kept for **full worlds**, which are distributed as legacy archives.
The small world no longer needs conversion (it already ships YAML). Convert a legacy
world dir in place:
```bash
# Convert legacy world to YAML
./tools/converter/convert_to_yaml.py --input <world_dir> --output <world_dir> --format yaml --type all

# Convert legacy world to a SQLite database
./tools/converter/convert_to_yaml.py --input <world_dir> --output <world_dir> --format sqlite --db <world_dir>/world.db --type all
```

**Automated Testing & Conversion:**
```bash
# Run world loading tests (automatically prepares and converts worlds)
./tools/run_load_tests.sh              # Full test suite
./tools/run_load_tests.sh --quick      # Quick smoke (small YAML world)
./tools/run_load_tests.sh --help       # Show all options
```

The `run_load_tests.sh` script:
- Small world is **YAML-only** (boots the checked-in `lib.template` YAML directly, no conversion)
- Builds the YAML variant for the small world; the Legacy/SQLite variants are built only when the full world is in scope
- Full world is extracted from a legacy archive (`FULL_WORLD_ARCHIVE`) and converted to YAML/SQLite on the fly
- Runs boot tests with configurable timeout (default 5 minutes)
- Calculates checksums (zones, rooms, mobs, objects, triggers) to verify correctness
- Compares checksums between formats (full world) and across YAML round-trips
- Generates detailed reports with boot times and performance comparison

**Important:**
- Schema/format changes should be tested with `run_load_tests.sh`
- Conversion script is in `tools/converter/convert_to_yaml.py`
- String enum values (like "kWorm") in YAML/SQLite are intentional for human readability - map them in loader
- When fixing loader issues, check if the problem is in converter or loader

