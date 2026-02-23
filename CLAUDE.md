# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is **Bylins MUD** - a Russian-language Multi-User Dungeon game server based on CircleMUD/DikuMUD. The codebase is ~272K lines of C++20 code with extensive Russian comments and variable names. It implements a complete multiplayer text-based RPG with combat, magic, crafting, quests, and scripting systems.

## Build Commands

### Initial Setup (Ubuntu/WSL)
```bash
sudo apt update && sudo apt upgrade
sudo apt install build-essential make libssl-dev libcurl4-gnutls-dev libexpat1-dev gettext unzip cmake gdb libgtest-dev
git clone --recurse-submodules https://github.com/bylins/mud
cd mud
cp -n -r lib.template/* lib
```

### Standard Build (without tests)
```bash
mkdir build
cd build
cmake -DBUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Test ..
make -j$(($(nproc)/2))
cd ..
./build/circle 4000  # Start server on port 4000
```

### Build with Tests
```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Test ..
make tests -j$(($(nproc)/2))
./tests/tests  # Run all tests
```

**Important:** Always use `-j$(($(nproc)/2))` for parallel builds to avoid overloading the system.

### Build Types
- **Release** - Optimized production build (-O0 with debug symbols, -rdynamic, -Wall)
- **Debug** - Debug build with ASAN (-fsanitize=address, NO _GLIBCXX_DEBUG due to yaml-cpp ABI compatibility)
- **Test** - Test build with optimizations (-O3, -DTEST_BUILD, -DNOCRYPT)
- **FastTest** - Fast test build (-Ofast, -DTEST_BUILD)

### Docker Build & Run
```bash
docker build -t mud-server --build-arg BUILD_TYPE=Test .
docker run -d -p 4000:4000 -e MUD_PORT=4000 -v ./lib:/mud/lib --name mud mud-server
docker stop mud
```

### Running the Server

**CRITICAL**: ALL binaries (circle, tests, converters, etc.) must ALWAYS be run from their build directory, NEVER from the source directory. Running binaries from the source directory creates runtime files (data/, misc/, etc.) in the wrong location and pollutes the source tree.

**CRITICAL**: Build directories must ALWAYS be out-of-source (outside the source tree). Never run cmake or binaries from within the src/ directory or repository root.

**CRITICAL**: CMake automatically creates test world `small/` in the build directory. All world data and configs are in `small/` directory itself, NOT in `small/lib/`. The `lib/` subdirectory DOES NOT EXIST in cmake-generated worlds. All paths in configuration.xml are relative to the `small/` directory.

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

**World Structure (cmake-generated):**
```
build_debug/
├── circle           # Binary
└── small/           # Test world (created by cmake)
    ├── misc/
    │   └── configuration.xml
    ├── world/       # Zone files
    ├── etc/         # Additional configs
    └── admin_api.sock  # Unix socket (if Admin API enabled)
```

**NOTE**: Do NOT confuse with repository's `lib/` directory - that is completely separate and used only for production deployments.

### Running Tests
```bash
# Run all tests
cd build
./tests/tests

# Run with CTest
make checks  # or: ctest -V
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
- **Braces**: Opening brace on new line, same indent as statement
```cpp
if (condition)
{
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
- All new files must be added to `CMakeLists.txt`
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
4. Add to `CMakeLists.txt` SOURCES and HEADERS lists

### Adding a New Skill
1. Define skill in `src/gameplay/skills/skills_info.cpp`
2. Create handler in `src/gameplay/skills/myskill.h` and `.cpp`
3. Hook into skill system via `SkillRollCheck()` or direct calls
4. Add to `CMakeLists.txt`

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
Use separate build directories for different CMake configurations to avoid lengthy rebuilds:
```
build/        - default build (without optional features)
build_sqlite/ - build with -DHAVE_SQLITE=ON
build_debug/  - debug build with -DCMAKE_BUILD_TYPE=Debug
build_test/   - test data and converted worlds (not for compilation)
```
**Always warn the user when changing build directories or running cmake/make in a different directory.**

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
- Running cmake in a different build directory
- Running make in a different build directory  
- Changing the working directory for any build operation

Example: "Switching to build_sqlite/ directory for SQLite-enabled build."

### World Data Formats and Testing

The project supports three world data formats:
1. **Legacy** - Original CircleMUD text format (default, in lib/ + lib.template/)
2. **SQLite** - World data in SQLite database (requires -DHAVE_SQLITE=ON)
3. **YAML** - Human-readable YAML format (requires -DHAVE_YAML=ON)

**CRITICAL: Never use lib/ from repository directly!**
- `lib/` contains base configuration files only (NOT complete world data)
- `lib.template/` contains world files, player data, and additional configs
- To get a working world: copy lib/ to build directory, then overlay lib.template/

**Preparing world for conversion:**
```bash
# Create working copy (example for YAML build)
mkdir -p build_yaml/small
cp -r lib build_yaml/small/
cp -r lib.template/* build_yaml/small/lib/

# Now build_yaml/small/lib contains complete world data ready for conversion
```

**Conversion Tool:**
```bash
# Convert legacy world to YAML (in-place conversion)
./tools/convert_to_yaml.py --input build_yaml/small/lib/world --output build_yaml/small/world --format yaml --type all

# Convert to SQLite database
./tools/convert_to_yaml.py --input build_sqlite/small/lib/world --output build_sqlite/small/world.db --format sqlite --type all
```

**Automated Testing & Conversion:**
```bash
# Run world loading tests (automatically prepares and converts worlds)
./tools/run_load_tests.sh              # Full test suite
./tools/run_load_tests.sh --quick      # Quick test (Legacy + YAML checksums)
./tools/run_load_tests.sh --help       # Show all options
```

The `run_load_tests.sh` script:
- Builds all three variants (Legacy, SQLite, YAML) in separate build directories
- Automatically prepares working worlds (copies lib + lib.template)
- Converts worlds if missing or outdated
- Runs boot tests with configurable timeout (default 5 minutes)
- Calculates checksums (zones, rooms, mobs, objects, triggers) to verify correctness
- Compares checksums between formats to detect discrepancies
- Generates detailed reports with boot times and performance comparison

**Important:**
- Schema/format changes should be tested with `run_load_tests.sh`
- Conversion script is in `tools/convert_to_yaml.py`
- String enum values (like "kWorm") in YAML/SQLite are intentional for human readability - map them in loader
- When fixing loader issues, check if the problem is in converter or loader

