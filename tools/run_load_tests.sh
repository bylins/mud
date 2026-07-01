#!/bin/bash
#
# Run world loading performance tests
#
# This script automatically builds required binaries and sets up test worlds.
#
# PREREQUISITES:
#   - lib.template/ directory with small world data (from repository)
#   - (Optional) Full YAML world directory at ~/repos/world_yaml
#     Override with: FULL_WORLD_DIR=/path/to/yaml_world ./tools/run_load_tests.sh
#
# USAGE:
#   # Run all tests (default)
#   ./tools/run_load_tests.sh
#
#   # Run specific loader tests
#   ./tools/run_load_tests.sh --loader=yaml
#   ./tools/run_load_tests.sh --loader=sqlite
#
#   # Run specific world size
#   ./tools/run_load_tests.sh --world=small
#   ./tools/run_load_tests.sh --world=full
#
#   # Run with/without checksums
#   ./tools/run_load_tests.sh --checksums
#   ./tools/run_load_tests.sh --no-checksums
#
#   # Combine filters
#   ./tools/run_load_tests.sh --loader=yaml --world=small --checksums
#   ./tools/run_load_tests.sh --loader=sqlite --world=full
#
#   # Quick smoke test (YAML small world, with checksums + round-trip)
#   ./tools/run_load_tests.sh --quick
#
# NOTE:
#   The small world is YAML-only as primary source: it boots the checked-in
#   flat YAML in lib.template/world directly. The SQLite round-trip test
#   (--loader=sqlite) runs after the YAML boot and uses the engine-written
#   world.db -- no converter involved.
#
#   The full world is a YAML directory (FULL_WORLD_DIR env var). No legacy
#   archive extraction or conversion is performed.
#
# OPTIONS:
#   --loader=TYPE     Run only tests for specific loader (yaml|sqlite).
#                     sqlite runs the YAML->SQLite->YAML round-trip.
#   --world=SIZE      Run only tests for specific world size (small|full)
#   --checksums       Run only tests WITH checksum calculation
#   --no-checksums    Run only tests WITHOUT checksum calculation
#   --quick           Run quick smoke: Small_YAML_checksums (+ YAML round-trips)
#   --rebuild         Force full rebuild (ninja -t clean && ninja)
#   --build-type=TYPE Set meson build_profile (release|debug|dev|fasttest|custom)
#   --recreate-builds Delete and recreate all build directories (implies world recreation)
#   --help            Show this help
#
# ENVIRONMENT VARIABLES:
#   FULL_WORLD_DIR   Path to full YAML world directory. REQUIRED for full-world
#                    tests; small-world-only runs (e.g. --quick or --world=small)
#                    ignore it.
#

MUD_DIR="$(cd "$(dirname "$0")/.." && pwd)"
YAML_BIN="$MUD_DIR/build_yaml/circle"
MULTI_BIN="$MUD_DIR/build_multi/circle"

# Parse command line arguments
FILTER_LOADER=""
FILTER_WORLD=""
FILTER_CHECKSUMS=""
QUICK_MODE=0
REBUILD_MODE=0
BUILD_TYPE=""
RECREATE_BUILDS=0

for arg in "$@"; do
    case "$arg" in
        --loader=*)
            FILTER_LOADER="${arg#*=}"
            ;;
        --world=*)
            FILTER_WORLD="${arg#*=}"
            ;;
        --checksums)
            FILTER_CHECKSUMS="yes"
            ;;
        --no-checksums)
            FILTER_CHECKSUMS="no"
            ;;
        --quick)
            QUICK_MODE=1
            ;;
        --rebuild)
            REBUILD_MODE=1
            ;;
        --build-type=*)
            BUILD_TYPE="${arg#*=}"
            ;;
        --recreate-builds)
            RECREATE_BUILDS=1
            ;;
        --help)
            head -n 57 "$0" | grep "^#" | grep -v "^#!/" | sed 's/^# \?//'
            exit 0
            ;;
        *)
            echo "Unknown option: $arg"
            echo "Run with --help for usage information"
            exit 1
            ;;
    esac
done

# Quick mode: small world YAML-only.
if [ $QUICK_MODE -eq 1 ]; then
    FILTER_LOADER="yaml"
    FILTER_WORLD="small"
    FILTER_CHECKSUMS="yes"
    echo "Quick mode: Running Small_YAML_checksums (+ round-trips)"
    echo ""
fi

# Path to full YAML world directory. No default -- caller MUST pass via env.
# When the full-world tests run and this is unset/missing, they are skipped with
# an error message pointing the operator to FULL_WORLD_DIR.
FULL_WORLD_DIR="${FULL_WORLD_DIR:-}"

# Map a CMake-style build type ("Debug"/"Release"/"Test"/"FastTest") to the
# corresponding meson profile. Pass through anything we don't recognise.
map_build_type_to_meson_profile() {
    case "$1" in
        Debug|debug)        echo "debug" ;;
        Release|release)    echo "release" ;;
        Test|test|Dev|dev)  echo "dev" ;;
        FastTest|fasttest)  echo "fasttest" ;;
        Custom|custom)      echo "custom" ;;
        "")                 echo "" ;;
        *)                  echo "$1" ;;
    esac
}

# Function to build a specific binary variant.
# Uses meson + ninja (the project's current build system).
#
# Args:
#   $1 = build_dir (out-of-source directory to set up)
#   $2 = meson_opts (space-separated -Doption=value flags)
#   $3 = binary_name (label for log files, e.g. "yaml"/"multi")
build_binary() {
    local build_dir="$1"
    local meson_opts="$2"
    local binary_name="$3"
    local needs_setup=0

    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "Building: $binary_name"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

    # Detect meson + ninja; allow ~/.local/bin fallback for pip --user installs.
    local meson_bin
    if command -v meson >/dev/null 2>&1; then
        meson_bin=meson
    elif [ -x "$HOME/.local/bin/meson" ]; then
        meson_bin="$HOME/.local/bin/meson"
    else
        echo "X ERROR: meson not found. Install with: pip install --user meson ninja"
        return 1
    fi

    local ninja_bin
    if command -v ninja >/dev/null 2>&1; then
        ninja_bin=ninja
    elif [ -x "$HOME/.local/bin/ninja" ]; then
        ninja_bin="$HOME/.local/bin/ninja"
    else
        echo "X ERROR: ninja not found. Install with: pip install --user meson ninja"
        return 1
    fi

    # Decide if we need a fresh `meson setup`.
    if [ ! -d "$build_dir" ] || [ ! -f "$build_dir/build.ninja" ]; then
        mkdir -p "$build_dir"
        needs_setup=1
    elif [ $RECREATE_BUILDS -eq 1 ]; then
        echo "Recreating build directory (--recreate-builds)..."
        rm -rf "$build_dir"
        mkdir -p "$build_dir"
        needs_setup=1
    fi

    # Translate optional BUILD_TYPE into a meson profile flag.
    local effective_opts="$meson_opts"
    if [ -n "$BUILD_TYPE" ]; then
        local profile
        profile=$(map_build_type_to_meson_profile "$BUILD_TYPE")
        # Strip any existing -Dbuild_profile=... so we don't end up with two.
        effective_opts=$(echo "$effective_opts" | sed -E 's/-Dbuild_profile=[^ ]*//g')
        effective_opts="$effective_opts -Dbuild_profile=$profile"
    fi

    if [ $needs_setup -eq 1 ]; then
        echo "[1/2] Configuring with meson..."
        echo "      Options: $effective_opts"
        "$meson_bin" setup "$build_dir" $effective_opts > /tmp/build_${binary_name}_meson.log 2>&1 || {
            echo "X ERROR: meson setup failed"
            echo "  Log: /tmp/build_${binary_name}_meson.log"
            echo "  Last 20 lines:"
            tail -20 /tmp/build_${binary_name}_meson.log
            return 1
        }
        echo " meson configuration complete"
    else
        echo "[1/2] Using existing meson configuration"
        # Allow `meson configure` to absorb an updated BUILD_TYPE override.
        if [ -n "$BUILD_TYPE" ]; then
            "$meson_bin" configure "$build_dir" -Dbuild_profile="$(map_build_type_to_meson_profile "$BUILD_TYPE")" \
                >> /tmp/build_${binary_name}_meson.log 2>&1 || true
        fi
    fi

    # Build (parallel; use half of cores to keep the box responsive).
    echo "[2/2] Compiling (this may take a while)..."
    echo "      Log: /tmp/build_${binary_name}.log"
    echo "      Tip: Run 'tail -f /tmp/build_${binary_name}.log' in another terminal to watch progress"
    echo ""

    local nprocs=$(($(nproc)/2))
    if [ "$nprocs" -lt 1 ]; then nprocs=1; fi

    if [ $REBUILD_MODE -eq 1 ]; then
        echo "      Cleaning previous build..."
        "$ninja_bin" -C "$build_dir" -t clean > /tmp/build_${binary_name}.log 2>&1 || true
    fi

    "$ninja_bin" -C "$build_dir" circle "-j$nprocs" >> /tmp/build_${binary_name}.log 2>&1 || {
        echo "X ERROR: Build failed"
        echo "  Log: /tmp/build_${binary_name}.log"
        echo "  Last 20 lines:"
        tail -20 /tmp/build_${binary_name}.log
        return 1
    }

    if [ ! -x "$build_dir/circle" ]; then
        echo "X ERROR: Binary not created despite successful build"
        echo "  Log: /tmp/build_${binary_name}.log"
        tail -20 /tmp/build_${binary_name}.log
        return 1
    fi
    echo " Successfully built $build_dir/circle"
    echo ""
    return 0
}


# Function to setup the small world (uses lib.template).
#
# The small world is YAML-only: lib.template/world ships only the checked-in
# flat YAML (zones/, dictionaries/, world_config.yaml), so the YAML build boots
# it directly. shops load from cfg/economics/shops.xml (copied in from lib/),
# not from world/shp.
#
# Args:
#   $1 = loader (always "yaml" for the primary boot; the same world dir is
#        reused for the SQLite round-trip test via build_multi)
#   $2 = target build directory (default: build_yaml)
setup_small_world() {
    local loader="$1"
    local build_dir="${2:-$MUD_DIR/build_yaml}"
    local dest_dir="$build_dir/small"

    if [ "$loader" != "yaml" ]; then
        echo "X ERROR: setup_small_world only supports 'yaml' (got '$loader')."
        echo "  The small world is YAML-only."
        return 1
    fi

    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "Setting up: small world in $(basename $build_dir)"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

    rm -rf "$dest_dir"

    # Materialise the small world via the meson helper (the same one that
    # `-Dsmall_world=true` triggers during configure). It copies lib/ -> small/
    # and overlays lib.template/ (including the checked-in YAML world).
    echo "Running tools/meson/setup_world.py to create small world structure..."
    python3 "$MUD_DIR/tools/meson/setup_world.py" \
        "$build_dir" "$MUD_DIR" "" > /tmp/setup_small_${loader}_$(basename $build_dir).log 2>&1 || {
        echo "X ERROR: small world setup failed"
        echo "  Log: /tmp/setup_small_${loader}_$(basename $build_dir).log"
        cat /tmp/setup_small_${loader}_$(basename $build_dir).log
        return 1
    }
    echo " Small YAML world ready at $dest_dir (lib + lib.template, checked-in YAML)"
    echo ""
    return 0
}

# Function to setup full world from a YAML world directory.
#
# Args:
#   $1 = target build directory (e.g. build_yaml or build_multi)
setup_full_world() {
    local build_dir="$1"
    local dest_dir="$build_dir/full"

    if [ -z "$FULL_WORLD_DIR" ]; then
        echo "X ERROR: FULL_WORLD_DIR is not set."
        echo "  Set it to the path of the YAML world directory, e.g.:"
        echo "    FULL_WORLD_DIR=~/repos/world_yaml $0"
        return 1
    fi
    if [ ! -d "$FULL_WORLD_DIR" ]; then
        echo "X ERROR: Full world directory not found: $FULL_WORLD_DIR"
        return 1
    fi

    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "Setting up: full world in $(basename $build_dir)"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "  Source: $FULL_WORLD_DIR"
    echo "  Target: $dest_dir"

    rm -rf "$dest_dir"
    mkdir -p "$dest_dir"
    cp -r "$FULL_WORLD_DIR/." "$dest_dir/" || {
        echo "X ERROR: Failed to copy world from $FULL_WORLD_DIR"
        return 1
    }
    echo " Full YAML world ready at $dest_dir"
    echo ""
    return 0
}


# Build all required binaries and setup worlds
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  SETUP: Building binaries and preparing worlds"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
# Determine which loaders/worlds are needed based on filters.
NEED_YAML=0
NEED_MULTI=0
NEED_SMALL=0
NEED_FULL=0

if [ -z "$FILTER_WORLD" ] || [ "$FILTER_WORLD" = "small" ]; then
    NEED_SMALL=1
fi
if [ -z "$FILTER_WORLD" ] || [ "$FILTER_WORLD" = "full" ]; then
    NEED_FULL=1
fi

# The full world needs a YAML world directory. If it's missing, drop the full
# world from scope now -- before any builds.
if [ $NEED_FULL -eq 1 ] && [ ! -d "$FULL_WORLD_DIR" ]; then
    echo "WARNING: Full world out of scope - directory not found: ${FULL_WORLD_DIR:-<unset>}"
    echo "  Set FULL_WORLD_DIR to run full-world tests."
    echo ""
    NEED_FULL=0
fi

# YAML is needed for any YAML test.
if [ -z "$FILTER_LOADER" ] || [ "$FILTER_LOADER" = "yaml" ]; then
    NEED_YAML=1
fi
# Multi (yaml+sqlite composite) is needed for SQLite round-trip tests.
if [ -z "$FILTER_LOADER" ] || [ "$FILTER_LOADER" = "sqlite" ]; then
    if ! pkg-config --exists sqlite3 2>/dev/null; then
        if [ "$FILTER_LOADER" = "sqlite" ]; then
            echo "X ERROR: SQLite dev headers not found (pkg-config sqlite3)."
            echo "  Install libsqlite3-dev to enable SQLite tests."
            exit 1
        else
            echo "⚠ SQLite dev headers not found (pkg-config sqlite3). Skipping SQLite round-trip tests."
            echo "  Install libsqlite3-dev to enable, or pass --loader=sqlite to force-fail."
            echo ""
        fi
    else
        NEED_MULTI=1
    fi
fi

# Build binaries if needed (with Admin API enabled for testing)
if [ $NEED_YAML -eq 1 ]; then
    build_binary "$MUD_DIR/build_yaml" "-Dadmin_api=true -Dyaml=builtin -Dbuild_profile=debug" "yaml" || exit 1
fi

if [ $NEED_MULTI -eq 1 ]; then
    # Multi binary: both yaml and sqlite enabled. yaml=builtin wins the format
    # pick at runtime only when it is the freshest source; sqlite=auto uses the
    # system libsqlite3 (same as build_multi used during development).
    build_binary "$MUD_DIR/build_multi" "-Dadmin_api=true -Dyaml=builtin -Dsqlite=auto -Dbuild_profile=debug" "multi" || exit 1
fi

# Setup worlds
if [ $NEED_SMALL -eq 1 ]; then
    if [ $NEED_YAML -eq 1 ]; then
        setup_small_world "yaml" "$MUD_DIR/build_yaml" || exit 1
    fi
    if [ $NEED_MULTI -eq 1 ]; then
        setup_small_world "yaml" "$MUD_DIR/build_multi" || exit 1
    fi
fi

if [ $NEED_FULL -eq 1 ]; then
    if [ $NEED_YAML -eq 1 ]; then
        setup_full_world "$MUD_DIR/build_yaml" || exit 1
    fi
    if [ $NEED_MULTI -eq 1 ]; then
        setup_full_world "$MUD_DIR/build_multi" || exit 1
    fi
fi

echo ""
echo "=== Prerequisites ready ==="
echo ""


# Function to check if test should run based on filters
should_run_test() {
    local test_name="$1"

    # Extract components from test name (e.g., "Small_YAML_checksums" or "Small_SQLite_RoundTrip")
    local world=$(echo "$test_name" | cut -d_ -f1 | tr 'A-Z' 'a-z')
    local loader=$(echo "$test_name" | cut -d_ -f2 | tr 'A-Z' 'a-z')

    # Check if this is an Admin API test
    local is_admin_api=0
    if echo "$test_name" | grep -qi "adminapi"; then
        is_admin_api=1
    fi

    # Round-trip is a diagnostic stage in its own right (resave + reboot);
    # checksums-filter doesn't apply to it.
    local is_roundtrip=0
    if echo "$test_name" | grep -qi "roundtrip"; then
        is_roundtrip=1
    fi

    if echo "$test_name" | grep -q "checksums$"; then
        local has_checksums="yes"
    else
        local has_checksums="no"
    fi

    # Apply filters
    if [ -n "$FILTER_LOADER" ] && [ "$loader" != "$FILTER_LOADER" ]; then
        return 1
    fi

    if [ -n "$FILTER_WORLD" ] && [ "$world" != "$FILTER_WORLD" ]; then
        return 1
    fi

    # Admin API and round-trip tests ignore checksums filter (they test
    # different aspects of the system, not boot performance).
    if [ $is_admin_api -eq 0 ] && [ $is_roundtrip -eq 0 ] && [ -n "$FILTER_CHECKSUMS" ]; then
        if [ "$FILTER_CHECKSUMS" = "yes" ] && [ "$has_checksums" != "yes" ]; then
            return 1
        fi
        if [ "$FILTER_CHECKSUMS" = "no" ] && [ "$has_checksums" != "no" ]; then
            return 1
        fi
    fi

    # Quick mode: YAML-only.
    if [ $QUICK_MODE -eq 1 ] && [ "$loader" != "yaml" ]; then
        return 1
    fi

    return 0
}

# Function to enable Admin API in configuration.xml
enable_admin_api() {
    local config_file="$1"

    # Check if admin_api section already exists
    if grep -q "<admin_api>" "$config_file"; then
        # Section exists - ensure enabled=true and require_auth=false
        sed -i 's|<enabled>false</enabled>|<enabled>true</enabled>|g' "$config_file"
        sed -i 's|<require_auth>true</require_auth>|<require_auth>false</require_auth>|g' "$config_file"
        # Add require_auth if missing
        if ! grep -q "<require_auth>" "$config_file"; then
            sed -i 's|</admin_api>|\t<require_auth>false</require_auth>\n\t</admin_api>|' "$config_file"
        fi
        return 0
    fi

    # Add admin_api section before </configuration>
    sed -i '/<\/configuration>/i\
\t<admin_api>\
\t\t<enabled>true</enabled>\
\t\t<socket_path>admin_api.sock</socket_path>\
\t\t<require_auth>false</require_auth>\
\t</admin_api>' "$config_file"

    return 0
}

# Ensure configuration.xml has a composite world_loader with both sqlite and
# yaml sources. Idempotent: if the section already exists it is left as-is.
enable_sqlite_composite_loader() {
    local config_file="$1"

    if grep -q "<world_loader>" "$config_file"; then
        return 0
    fi

    sed -i '/<\/configuration>/i\
\t<world_loader>\
\t\t<sources>\
\t\t\t<sqlite/>\
\t\t\t<yaml/>\
\t\t</sources>\
\t</world_loader>' "$config_file"
}

# Function to run Admin API CRUD test
run_admin_api_test() {
    local name="$1"
    local binary="$2"
    local test_dir="$3"
    local world_format="$4"  # yaml, sqlite

    # Check if this test should run
    if ! should_run_test "$name"; then
        return
    fi

    echo "--- $name (Admin API Test) ---"

    local work_dir="$(dirname "$test_dir")"
    local data_dir="$(basename "$test_dir")"
    local world_size=$(basename "$work_dir" | sed 's/build_\(yaml\|multi\)//')

    cd "$work_dir"

    # Recreate world to avoid checksum contamination from previous Admin API tests
    echo "  Recreating world for clean Admin API test..."
    if [ "$data_dir" = "small" ]; then
        setup_small_world "yaml" "$work_dir" > /tmp/admin_api_setup_${world_format}.log 2>&1
    elif [ "$data_dir" = "full" ]; then
        setup_full_world "$work_dir" > /tmp/admin_api_setup_${world_format}.log 2>&1
    fi

    # setup_small_world/setup_full_world change back to MUD_DIR, so cd back to work_dir
    cd "$work_dir"

    # Enable admin_api in configuration
    enable_admin_api "$data_dir/cfg/configuration.xml"

    # Clean previous run; also kill any stale process on port 4001
    rm -rf "$data_dir/syslog" "$data_dir/admin_api.sock" 2>/dev/null || true
    fuser -k 4001/tcp 2>/dev/null || true
    sleep 0.5

    # Start server in background with admin API enabled (cd into data_dir so syslog/log/ land there)
    # exec replaces the subshell so $! captures the binary's PID, not the shell's
    echo "  Starting server with Admin API..."
    (cd "$data_dir" && exec "$binary" -d . 4001 > "stdout_admin.log" 2>&1) &
    local server_pid=$!

    # Wait for socket to appear (max 30 seconds)
    local waited=0
    while [ $waited -lt 60 ] && [ ! -S "$data_dir/admin_api.sock" ]; do
        sleep 0.5
        waited=$((waited + 1))
    done

    if [ ! -S "$data_dir/admin_api.sock" ]; then
        echo "  X ERROR: Admin API socket not created"
        cat "$data_dir/stdout_admin.log" 2>/dev/null || echo "  (no stdout log)"
        kill $server_pid 2>/dev/null || true
        wait $server_pid 2>/dev/null || true
        cd "$MUD_DIR"
        return 1
    fi

    echo "  Running Admin API CRUD tests..."

    # Run admin API test (no auth required for tests)
    SOCKET_PATH="$data_dir/admin_api.sock" \
    WORLD_DIR="$data_dir" \
    WORLD_FORMAT="$world_format" \
    python3 "$MUD_DIR/tests/test_admin_api.py" full_crud_test > "$data_dir/admin_api_test.log" 2>&1

    local test_result=$?

    # Kill server and wait for port to be released
    kill $server_pid 2>/dev/null
    sleep 1
    kill -9 $server_pid 2>/dev/null || true
    wait $server_pid 2>/dev/null || true

    # Display results
    if [ $test_result -eq 0 ]; then
        echo "  ✅ Admin API CRUD tests PASSED"
        # Show summary from log
        grep -E "(PASSED|FAILED|✅|❌)" "$data_dir/admin_api_test.log" | tail -10
    else
        echo "  ❌ Admin API CRUD tests FAILED"
        echo "  Last 30 lines of test log:"
        tail -30 "$data_dir/admin_api_test.log"
    fi

    echo ""
    cd "$MUD_DIR"
}

# Round-trip diagnostic: resave the world via `circle -S world_v2`, swap
# `world_v2` over `world`, then reuse run_test() to boot the re-emitted world
# and compute its checksum (cs3). The cs3 file (/tmp/${name}_checksums.txt)
# is later compared with cs2 in the CHECKSUM COMPARISON section.
#
# Mutates the world dir in-place; admin API tests further down re-extract from
# the archive, so no restore is needed here.
run_roundtrip_test() {
    local name="$1"
    local binary="$2"
    local test_dir="$3"

    if ! should_run_test "$name"; then
        return 0
    fi

    echo "--- $name ---"

    if [ ! -d "$test_dir/world" ]; then
        echo "  X ERROR: $test_dir/world not present (cs2 stage didn't run?)"
        return 1
    fi

    (cd "$test_dir" && rm -rf world_v2 stdout_rt.log)

    echo "  Resaving world via '-S world_v2' ..."
    (cd "$test_dir" && exec "$binary" -d . -S world_v2 4001 > stdout_rt.log 2>&1)
    local rc=$?
    if [ $rc -ne 0 ] || [ ! -d "$test_dir/world_v2" ]; then
        echo "  X ERROR: circle -S exited rc=$rc, world_v2 present: $([ -d "$test_dir/world_v2" ] && echo yes || echo no)"
        echo "  Last 30 lines of stdout_rt.log:"
        tail -30 "$test_dir/stdout_rt.log" 2>/dev/null || true
        return 1
    fi

    echo "  Swapping world <- world_v2 ..."
    (cd "$test_dir" && rm -rf world && mv world_v2 world)

    # Re-boot the re-emitted world with -W to compute cs3. run_test handles
    # syslog parsing, boot timing, checksum capture, and copy to /tmp.
    run_test "$name" "$binary" "$test_dir" "-W"
}

# Flat-layout round-trip: switch the YAML world to flat save, re-emit the whole
# world via `-S` (which must produce flat <sub>.yaml files and remove the
# per-file directories), boot the flat world under -W, and capture its
# checksums. The CHECKSUM COMPARISON section diffs these against the per-file
# YAML checksums -- they must be identical. The shared world dir is restored to
# its pristine per-file state afterwards so later tests are unaffected.
run_flat_roundtrip_test() {
    local name="$1"
    local binary="$2"
    local test_dir="$3"

    if ! should_run_test "$name"; then
        return 0
    fi

    echo "--- $name ---"

    local cfg="$test_dir/world/world_config.yaml"
    if [ ! -d "$test_dir/world" ] || [ ! -f "$cfg" ]; then
        echo "  X ERROR: $test_dir/world (with world_config.yaml) not present"
        return 1
    fi

    (cd "$test_dir" && rm -rf world_flat world_pf_keep stdout_flat.log)
    cp "$cfg" "$cfg.flatbak"

    # Switch the source world to flat-save.
    if grep -q '^layout:' "$cfg"; then
        sed -i 's/^layout:.*/layout: flat/' "$cfg"
    else
        printf 'layout: flat\n' >> "$cfg"
    fi

    echo "  Resaving world (layout=flat) via '-S world_flat' ..."
    (cd "$test_dir" && exec "$binary" -d . -S world_flat 4001 > stdout_flat.log 2>&1)
    local rc=$?
    if [ $rc -ne 0 ] || [ ! -d "$test_dir/world_flat" ]; then
        echo "  X ERROR: circle -S (flat) exited rc=$rc, world_flat present: $([ -d "$test_dir/world_flat" ] && echo yes || echo no)"
        tail -30 "$test_dir/stdout_flat.log" 2>/dev/null || true
        mv "$cfg.flatbak" "$cfg"
        return 1
    fi

    # Sanity: the re-emitted world must actually be flat -- a flat <sub>.yaml
    # present and no per-file entity directory left behind in the same zone.
    local zdir
    zdir=$(find "$test_dir/world_flat/zones" -mindepth 1 -maxdepth 1 -type d 2>/dev/null | head -1)
    if [ -z "$zdir" ] || [ ! -f "$zdir/mobs.yaml" ] || [ -d "$zdir/mobs" ]; then
        echo "  X ERROR: -S did not produce a clean flat layout in '$zdir'"
        ls "$zdir" 2>/dev/null
        (cd "$test_dir" && rm -rf world_flat)
        mv "$cfg.flatbak" "$cfg"
        return 1
    fi

    # Boot the flat world under -W and capture checksums.
    (cd "$test_dir" && mv world world_pf_keep && mv world_flat world)
    run_test "$name" "$binary" "$test_dir" "-W"

    # Restore the pristine per-file world + config for later tests.
    (cd "$test_dir" && rm -rf world && mv world_pf_keep world)
    mv "$cfg.flatbak" "$cfg"
}

# SQLite round-trip: boot the YAML world with the composite (yaml+sqlite)
# binary so the engine writes world.db, then reboot from world.db and capture
# checksums. Compares against the YAML-load baseline in the CHECKSUM COMPARISON
# section -- must be 0 diffs.
#
# Args:
#   $1 = test name (e.g. "Small_SQLite_RoundTrip")
#   $2 = composite binary (build_multi/circle)
#   $3 = test dir (build_multi/small or build_multi/full)
run_sqlite_roundtrip_test() {
    local name="$1"
    local binary="$2"
    local test_dir="$3"

    if ! should_run_test "$name"; then
        return 0
    fi

    echo "--- $name ---"

    if [ ! -d "$test_dir/world" ]; then
        echo "  X ERROR: $test_dir/world not present"
        return 1
    fi

    local cfg="$test_dir/cfg/configuration.xml"
    if [ ! -f "$cfg" ]; then
        echo "  X ERROR: $cfg not found"
        return 1
    fi

    # Ensure the composite loader (sqlite + yaml sources) is configured.
    enable_sqlite_composite_loader "$cfg"

    # Remove stale world.db so boot 1 always loads from YAML.
    rm -f "$test_dir/world.db"
    rm -rf "$test_dir/syslog" 2>/dev/null || true

    # Kill any stale process on 4001.
    fuser -k 4001/tcp 2>/dev/null || true
    sleep 0.5

    # Boot 1: YAML wins (SQLite empty) -> engine resyncs to world.db.
    echo "  Boot 1: YAML-load + SQLite resync ..."
    (cd "$test_dir" && exec "$binary" -d . 4001 > "stdout_sqlt1.log" 2>&1) &

    local waited=0
    while [ $waited -lt 300 ]; do
        if LANG=C grep -qa "Boot db -- DONE" "$test_dir/syslog" 2>/dev/null; then
            break
        fi
        sleep 1
        waited=$((waited + 1))
    done

    fuser -k 4001/tcp 2>/dev/null || true
    sleep 1

    if ! LANG=C grep -qa "Boot db -- DONE" "$test_dir/syslog" 2>/dev/null; then
        echo "  X ERROR: Boot 1 timed out (300s)"
        tail -20 "$test_dir/syslog" 2>/dev/null || echo "  (no syslog)"
        return 1
    fi

    if [ ! -f "$test_dir/world.db" ]; then
        echo "  X ERROR: world.db not created after boot 1"
        return 1
    fi
    echo "  Boot 1 complete (world.db populated)"

    # Boot 2: SQLite wins (freshness > YAML mtime) -> load from SQLite -> checksums.
    rm -rf "$test_dir/syslog" "$test_dir/checksums_detailed.txt" "$test_dir/checksums_buffers" 2>/dev/null || true
    run_test "$name" "$binary" "$test_dir" "-W"
}

# Function to run a single test
run_test() {
    local name="$1"
    local binary="$2"
    local test_dir="$3"
    local extra_flags="$4"

    # Check if this test should run
    if ! should_run_test "$name"; then
        return
    fi

    echo "--- $name ---"

    local work_dir="$(dirname "$test_dir")"
    local data_dir="$(basename "$test_dir")"
    cd "$work_dir"
    rm -rf "$data_dir/syslog" "$data_dir/checksums_detailed.txt" "$data_dir/checksums_buffers" 2>/dev/null || true

    # Start server in background (cd into data_dir so syslog/log/ land there)
    # exec replaces the subshell so $! captures the binary's PID, not the shell's
    echo "  Running: $binary -d . $extra_flags 4000 (from $data_dir)"
    (cd "$data_dir" && exec "$binary" -d . $extra_flags 4000 > "stdout.log" 2>&1) &

    # Wait for syslog to appear (max 10 seconds)
    local waited=0
    while [ $waited -lt 60 ] && [ ! -f "$data_dir/syslog" ]; do
        sleep 0.5
        waited=$((waited + 1))
    done

    if [ ! -f "$data_dir/syslog" ]; then
        echo "  X ERROR: Server did not create syslog"
        cat "$data_dir/stdout.log" 2>/dev/null || echo "  (no stdout log)"
        cd "$MUD_DIR"
        return 1
    fi

    # Determine what to wait for based on -W flag (checksums enabled)
    local wait_pattern
    if echo "$extra_flags" | grep -q -- "-W"; then
        # Wait for checksums to complete (they appear after boot)
        wait_pattern="Zones:.*zones\)"
    else
        # Just wait for boot completion
        wait_pattern="Boot db -- DONE"
    fi

    # Wait for completion (max 5 minutes)
    waited=0
    local boot_success=0
    while [ $waited -lt 300 ]; do

        # Check if pattern found (match with or without trailing period)
        if LANG=C grep -qEa "$wait_pattern" "$data_dir/syslog" 2>/dev/null; then
            boot_success=1
            sleep 1  # Give 1 extra second for file writes
            break
        fi

        sleep 1
        waited=$((waited + 1))
    done

    # Kill server - find by port
    local server_pid=$(lsof -t -i:4000 2>/dev/null)
    if [ -n "$server_pid" ]; then
        kill $server_pid 2>/dev/null
        sleep 1
        kill -9 $server_pid 2>/dev/null || true
    fi

    # Check if boot succeeded
    if [ $boot_success -eq 0 ]; then
        echo "  X ERROR: Boot timeout (5 minutes exceeded)"
        echo "  Last 30 lines of syslog:"
        tail -30 "$data_dir/syslog" 2>/dev/null || echo "  (no syslog found)"
        cd "$MUD_DIR"
        return 1
    fi

    # Extract and display results
    if [ -f "$data_dir/syslog" ]; then
        # Boot times
        local begin=$(LANG=C grep -Ea "Boot db -- BEGIN\.?" "$data_dir/syslog" | head -1 | cut -d' ' -f2)
        local done_time=$(LANG=C grep -Ea "Boot db -- DONE\.?" "$data_dir/syslog" | head -1 | cut -d' ' -f2)

        if [ -n "$begin" ] && [ -n "$done_time" ]; then
            local begin_sec=$(echo "$begin" | awk -F: '{printf "%.3f", ($1*3600)+($2*60)+$3}')
            local done_sec=$(echo "$done_time" | awk -F: '{printf "%.3f", ($1*3600)+($2*60)+$3}')
            local duration=$(echo "$done_sec - $begin_sec" | bc)
            echo "  Boot time:     ${duration}s"
        fi

        # World checksums
        LANG=C grep -aoE "(Zones:|Rooms:|Mobs:|Objects:|Triggers:).*" "$data_dir/syslog" | tail -5 | while read line; do
            echo "  $line"
        done

        # Runtime checksums (only if -W flag was used)
        if echo "$extra_flags" | grep -q -- "-W"; then
            LANG=C grep -aoE "(Room Scripts:|Mob Scripts:|Object Scripts:|Door Rnums:).*" "$data_dir/syslog" | tail -4 | while read line; do
                echo "  $line"
            done
        fi

        # Checksum calculation time
        local cs_begin=$(LANG=C grep -a "Calculating world checksums" "$data_dir/syslog" | head -1 | cut -d' ' -f2)
        local cs_done=$(LANG=C grep -a "Detailed buffers saved" "$data_dir/syslog" | head -1 | cut -d' ' -f2)
        if [ -n "$cs_begin" ] && [ -n "$cs_done" ]; then
            local cs_begin_sec=$(echo "$cs_begin" | awk -F: '{printf "%.3f", ($1*3600)+($2*60)+$3}')
            local cs_done_sec=$(echo "$cs_done" | awk -F: '{printf "%.3f", ($1*3600)+($2*60)+$3}')
            local cs_duration=$(echo "$cs_done_sec - $cs_begin_sec" | bc)
            echo "  Checksum time: ${cs_duration}s"
        fi

        # Save checksums for comparison
        if [ -f "$data_dir/checksums_detailed.txt" ]; then
            local fname=$(echo "$name" | tr ' ' '_')
            cp "$data_dir/checksums_detailed.txt" "/tmp/${fname}_checksums.txt"
        fi
    else
        echo "  ERROR: Boot failed (no syslog)"
    fi
    echo ""

    cd "$MUD_DIR"
}

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  Setup complete. Starting tests..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "=============================================="
echo "World Loading Performance Tests"
echo "Date: $(date)"
if [ -n "$FILTER_LOADER" ]; then
    echo "Filter: loader=$FILTER_LOADER"
fi
if [ -n "$FILTER_WORLD" ]; then
    echo "Filter: world=$FILTER_WORLD"
fi
if [ -n "$FILTER_CHECKSUMS" ]; then
    echo "Filter: checksums=$FILTER_CHECKSUMS"
fi
if [ $QUICK_MODE -eq 1 ]; then
    echo "Mode: QUICK (small YAML smoke)"
fi
echo "=============================================="
echo ""

# Small World Tests
if [ -z "$FILTER_WORLD" ] || [ "$FILTER_WORLD" = "small" ]; then
    echo "=== SMALL WORLD - YAML ==="
    echo ""
    [ -x "$YAML_BIN" ] && [ -d "$MUD_DIR/build_yaml/small" ] && run_test "Small_YAML_checksums" "$YAML_BIN" "$MUD_DIR/build_yaml/small" "-W"
    [ -x "$YAML_BIN" ] && [ -d "$MUD_DIR/build_yaml/small" ] && run_test "Small_YAML_no_checksums" "$YAML_BIN" "$MUD_DIR/build_yaml/small" ""

    echo ""
    echo "=== SMALL WORLD - YAML ROUND-TRIPS ==="
    echo ""
    [ -x "$YAML_BIN" ] && [ -d "$MUD_DIR/build_yaml/small" ] && run_roundtrip_test "Small_YAML_RoundTrip" "$YAML_BIN" "$MUD_DIR/build_yaml/small"
    [ -x "$YAML_BIN" ] && [ -d "$MUD_DIR/build_yaml/small" ] && run_flat_roundtrip_test "Small_YAML_FlatRoundTrip" "$YAML_BIN" "$MUD_DIR/build_yaml/small"

    echo ""
    echo "=== SMALL WORLD - SQLITE ROUND-TRIP ==="
    echo ""
    [ -x "$MULTI_BIN" ] && [ -d "$MUD_DIR/build_multi/small" ] && run_sqlite_roundtrip_test "Small_SQLite_RoundTrip" "$MULTI_BIN" "$MUD_DIR/build_multi/small"

    # Admin API CRUD Tests last -- they recreate the world from scratch and
    # mutate it, so anything order-sensitive has to run before this stage.
    echo ""
    echo "=== SMALL WORLD - ADMIN API TESTS ==="
    echo ""
    [ -x "$YAML_BIN" ] && [ -d "$MUD_DIR/build_yaml/small" ] && run_admin_api_test "Small_YAML_AdminAPI" "$YAML_BIN" "$MUD_DIR/build_yaml/small" "yaml"
fi

# Full World Tests
if [ -z "$FILTER_WORLD" ] || [ "$FILTER_WORLD" = "full" ]; then
    echo "=== FULL WORLD - YAML ==="
    echo ""
    [ -x "$YAML_BIN" ] && [ -d "$MUD_DIR/build_yaml/full" ] && run_test "Full_YAML_checksums" "$YAML_BIN" "$MUD_DIR/build_yaml/full" "-W"
    [ -x "$YAML_BIN" ] && [ -d "$MUD_DIR/build_yaml/full" ] && run_test "Full_YAML_no_checksums" "$YAML_BIN" "$MUD_DIR/build_yaml/full" ""

    echo ""
    echo "=== FULL WORLD - YAML ROUND-TRIPS ==="
    echo ""
    [ -x "$YAML_BIN" ] && [ -d "$MUD_DIR/build_yaml/full" ] && run_roundtrip_test "Full_YAML_RoundTrip" "$YAML_BIN" "$MUD_DIR/build_yaml/full"
    [ -x "$YAML_BIN" ] && [ -d "$MUD_DIR/build_yaml/full" ] && run_flat_roundtrip_test "Full_YAML_FlatRoundTrip" "$YAML_BIN" "$MUD_DIR/build_yaml/full"

    echo ""
    echo "=== FULL WORLD - SQLITE ROUND-TRIP ==="
    echo ""
    [ -x "$MULTI_BIN" ] && [ -d "$MUD_DIR/build_multi/full" ] && run_sqlite_roundtrip_test "Full_SQLite_RoundTrip" "$MULTI_BIN" "$MUD_DIR/build_multi/full"

    # Admin API CRUD Tests last.
    echo ""
    echo "=== FULL WORLD - ADMIN API TESTS ==="
    echo ""
    [ -x "$YAML_BIN" ] && [ -d "$MUD_DIR/build_yaml/full" ] && run_admin_api_test "Full_YAML_AdminAPI" "$YAML_BIN" "$MUD_DIR/build_yaml/full" "yaml"
fi

# Compare checksums (only if checksums were calculated)
if [ -z "$FILTER_CHECKSUMS" ] || [ "$FILTER_CHECKSUMS" = "yes" ]; then
    echo "=== CHECKSUM COMPARISON ==="
    echo ""

    compare_checksums() {
        local name1="$1"
        local name2="$2"
        local file1="/tmp/${name1}_checksums.txt"
        local file2="/tmp/${name2}_checksums.txt"

        if [ -f "$file1" ] && [ -f "$file2" ]; then
            if diff -q "$file1" "$file2" > /dev/null 2>&1; then
                echo "$name1 vs $name2: MATCH"
            else
                echo "$name1 vs $name2: DIFFER"
                diff "$file1" "$file2" | head -10
            fi
        else
            echo "$name1 vs $name2: Cannot compare (missing files)"
        fi
    }

    if [ -z "$FILTER_WORLD" ] || [ "$FILTER_WORLD" = "small" ]; then
        echo "Small world:"
        if [ -z "$FILTER_LOADER" ] || [ "$FILTER_LOADER" = "yaml" ]; then
            compare_checksums "Small_YAML_checksums" "Small_YAML_RoundTrip"
            compare_checksums "Small_YAML_checksums" "Small_YAML_FlatRoundTrip"
        fi
        if [ -z "$FILTER_LOADER" ] || [ "$FILTER_LOADER" = "sqlite" ]; then
            compare_checksums "Small_YAML_checksums" "Small_SQLite_RoundTrip"
        fi
        echo ""
    fi

    if [ -z "$FILTER_WORLD" ] || [ "$FILTER_WORLD" = "full" ]; then
        echo "Full world:"
        if [ -z "$FILTER_LOADER" ] || [ "$FILTER_LOADER" = "yaml" ]; then
            compare_checksums "Full_YAML_checksums" "Full_YAML_RoundTrip"
            compare_checksums "Full_YAML_checksums" "Full_YAML_FlatRoundTrip"
        fi
        if [ -z "$FILTER_LOADER" ] || [ "$FILTER_LOADER" = "sqlite" ]; then
            compare_checksums "Full_YAML_checksums" "Full_SQLite_RoundTrip"
        fi
        echo ""
    fi
fi

echo "=============================================="
echo "Tests complete"
echo "=============================================="
