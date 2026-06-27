#!/bin/bash
#
# Run world loading performance tests
#
# This script automatically builds required binaries and sets up test worlds.
#
# PREREQUISITES:
#   - lib.template/ directory with small world data (from repository)
#   - (Optional) Full world archive at ~/repos/world.tgz
#     Override with: FULL_WORLD_ARCHIVE=/path/to/world.tgz ./tools/run_load_tests.sh
#
# USAGE:
#   # Run all tests (default)
#   ./tools/run_load_tests.sh
#
#   # Run specific loader tests
#   ./tools/run_load_tests.sh --loader=yaml
#   ./tools/run_load_tests.sh --loader=legacy
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
#   # Quick comparison test (YAML vs Legacy, small world, with checksums)
#   ./tools/run_load_tests.sh --quick
#
# OPTIONS:
#   --loader=TYPE     Run only tests for specific loader (legacy|sqlite|yaml)
#   --world=SIZE      Run only tests for specific world size (small|full)
#   --checksums       Run only tests WITH checksum calculation
#   --no-checksums    Run only tests WITHOUT checksum calculation
#   --quick           Run quick comparison: Small_Legacy_checksums vs Small_YAML_checksums
#   --rebuild         Force full rebuild (ninja -t clean && ninja)
#   --build-type=TYPE Set meson build_profile (release|debug|dev|fasttest|custom)
#   --recreate-builds Delete and recreate all build directories (implies world recreation)
#   --help            Show this help
#
# ENVIRONMENT VARIABLES:
#   FULL_WORLD_ARCHIVE   Path to full world archive (tarball). REQUIRED for full-world tests;
#                        small-world-only runs (e.g. --quick or --world=small) ignore it.
#

MUD_DIR="$(cd "$(dirname "$0")/.." && pwd)"
LEGACY_BIN="$MUD_DIR/build_test/circle"
SQLITE_BIN="$MUD_DIR/build_sqlite/circle"
YAML_BIN="$MUD_DIR/build_yaml/circle"

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
            head -n 46 "$0" | grep "^#" | grep -v "^#!/" | sed 's/^# \?//'
            exit 0
            ;;
        *)
            echo "Unknown option: $arg"
            echo "Run with --help for usage information"
            exit 1
            ;;
    esac
done

# Quick mode: only Small_Legacy_checksums and Small_YAML_checksums.
# Skip sqlite explicitly -- the project ships no sqlite3.wrap and the test box
# may not have libsqlite3-dev installed, in which case `meson setup
# -Dsqlite=auto` would error out.
QUICK_SKIP_SQLITE=0
if [ $QUICK_MODE -eq 1 ]; then
    FILTER_LOADER=""  # Will run both legacy and yaml
    FILTER_WORLD="small"
    FILTER_CHECKSUMS="yes"
    QUICK_SKIP_SQLITE=1
    echo "Quick mode: Running Small_Legacy_checksums and Small_YAML_checksums"
    echo ""
fi

# Path to full world archive (tarball). No default -- caller MUST pass via env.
# When the full-world tests run and this is unset/missing, they are skipped with
# an error message pointing the operator to FULL_WORLD_ARCHIVE.
FULL_WORLD_ARCHIVE="${FULL_WORLD_ARCHIVE:-}"

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
#   $3 = binary_name (label for log files, e.g. "legacy"/"yaml")
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



# Function to setup small world (uses lib.template)
setup_small_world() {
    local loader="$1"
    
    # Determine build directory
    if [ "$loader" = "legacy" ]; then
        local build_dir="$MUD_DIR/build_test"
    elif [ "$loader" = "sqlite" ]; then
        local build_dir="$MUD_DIR/build_sqlite"
    elif [ "$loader" = "yaml" ]; then
        local build_dir="$MUD_DIR/build_yaml"
    fi
    
    local dest_dir="$build_dir/small"

    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "Setting up: small world ($loader)"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

    rm -rf "$dest_dir"

    # Materialise the small world via the meson helper (the same one that
    # `-Dsmall_world=true` triggers during configure). It copies lib/ -> small/
    # and overlays lib.template/.
    echo "Running tools/meson/setup_world.py to create small world structure..."
    python3 "$MUD_DIR/tools/meson/setup_world.py" \
        "$build_dir" "$MUD_DIR" "" > /tmp/setup_small_${loader}.log 2>&1 || {
        echo "X ERROR: small world setup failed"
        echo "  Log: /tmp/setup_small_${loader}.log"
        cat /tmp/setup_small_${loader}.log
        return 1
    }
    echo " Small world created at $dest_dir (lib + lib.template)"

    if [ "$loader" = "legacy" ]; then
        echo " Legacy world ready (using lib.template/world)"
        echo ""
        return 0
    fi

    # Clean old CONVERTED world files (YAML/SQLite), keep legacy source files for conversion
    echo "Cleaning old converted world files..."
    if [ "$loader" = "yaml" ]; then
        # Remove YAML converted directories, keep legacy source files (world/mob/, world/obj/, etc.)
        rm -rf "$dest_dir/world/mobs" "$dest_dir/world/objects" "$dest_dir/world/zones" \
               "$dest_dir/world/triggers" "$dest_dir/world/dictionaries" \
               "$dest_dir/world/world_config.yaml" "$dest_dir/world/index.yaml" 2>/dev/null || true
    elif [ "$loader" = "sqlite" ]; then
        # Remove SQLite database
        rm -f "$dest_dir/world.db" 2>/dev/null || true
    fi

    echo "Converting world data to $loader format..."

    if [ "$loader" = "sqlite" ]; then
        echo "  Log: /tmp/convert_small_sqlite.log"
        python3 "$MUD_DIR/tools/converter/convert_to_yaml.py" \
            -i "$dest_dir" \
            -o "$dest_dir" \
            -f sqlite \
            --db "$dest_dir/world.db" > /tmp/convert_small_sqlite.log 2>&1 || {
            echo "X ERROR: Conversion failed"
            echo "  Log: /tmp/convert_small_sqlite.log"
            tail -10 /tmp/convert_small_sqlite.log
            return 1
        }
        echo " Converted in-place to SQLite format"
    elif [ "$loader" = "yaml" ]; then
        echo "  Log: /tmp/convert_small_yaml.log"
        python3 "$MUD_DIR/tools/converter/convert_to_yaml.py" \
            -i "$dest_dir" \
            -o "$dest_dir" \
            -f yaml > /tmp/convert_small_yaml.log 2>&1 || {
            echo "X ERROR: Conversion failed"
            echo "  Log: /tmp/convert_small_yaml.log"
            tail -10 /tmp/convert_small_yaml.log
            return 1
        }
        echo " Converted in-place to YAML format"
    fi
    
    echo ""
    return 0
}
# Function to setup full world (extracts from archive)
setup_full_world() {
    local loader="$1"
    
    # Determine build directory
    if [ "$loader" = "legacy" ]; then
        local build_dir="$MUD_DIR/build_test"
    elif [ "$loader" = "sqlite" ]; then
        local build_dir="$MUD_DIR/build_sqlite"
    elif [ "$loader" = "yaml" ]; then
        local build_dir="$MUD_DIR/build_yaml"
    fi
    
    local dest_dir="$build_dir/full"
    
    if [ -z "$FULL_WORLD_ARCHIVE" ]; then
        echo "X ERROR: FULL_WORLD_ARCHIVE is not set."
        echo "  Set it to the path of the world tarball, e.g.:"
        echo "    FULL_WORLD_ARCHIVE=~/worlds/world.20260427.cleaned.tgz $0"
        return 1
    fi
    if [ ! -f "$FULL_WORLD_ARCHIVE" ]; then
        echo "X ERROR: Full world archive not found: $FULL_WORLD_ARCHIVE"
        return 1
    fi

    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "Setting up: full world ($loader)"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

    echo "[1/3] Extracting archive..."
    echo "      Source: $FULL_WORLD_ARCHIVE"
    echo "      Target: $dest_dir"

    rm -rf "$dest_dir"
    mkdir -p "$dest_dir"
    # --strip-components=1 drops the archive's top-level directory (whatever it
    # is named -- "lib/", "world.YYYYMMDD/", ...) so contents land directly in
    # $dest_dir. Keeps the script portable across archive layouts.
    tar -xzf "$FULL_WORLD_ARCHIVE" -C "$dest_dir" --strip-components=1 > /tmp/extract_full.log 2>&1 || {
        echo "X ERROR: Extraction failed"
        echo "  Log: /tmp/extract_full.log"
        return 1
    }
    echo " Extracted successfully"

    if [ "$loader" = "legacy" ]; then
        echo "[2/3] Preparing legacy world..."
        echo " Ready at $dest_dir"
    elif [ "$loader" = "sqlite" ] || [ "$loader" = "yaml" ]; then
        echo "[2/3] Converting world data (this may take several minutes)..."
        echo "      Format: $loader"
        echo "      Log: /tmp/convert_full_${loader}.log"
        echo "      Tip: Run 'tail -f /tmp/convert_full_${loader}.log' in another terminal to watch progress"
        echo ""

        if [ "$loader" = "sqlite" ]; then
            python3 "$MUD_DIR/tools/converter/convert_to_yaml.py" \
                -i "$dest_dir" \
                -o "$dest_dir" \
                -f sqlite \
                --db "$dest_dir/world.db" \
 > /tmp/convert_full_sqlite.log 2>&1 || {
                echo "X ERROR: Conversion failed"
                echo "  Log: /tmp/convert_full_sqlite.log"
                tail -20 /tmp/convert_full_sqlite.log
                return 1
            }
            echo " Converted to SQLite format"
        else
            python3 "$MUD_DIR/tools/converter/convert_to_yaml.py" \
                -i "$dest_dir" \
                -o "$dest_dir" \
                -f yaml \
 > /tmp/convert_full_yaml.log 2>&1 || {
                echo "X ERROR: Conversion failed"
                echo "  Log: /tmp/convert_full_yaml.log"
                tail -20 /tmp/convert_full_yaml.log
                return 1
            }
            echo " Converted to YAML format"
        fi
    fi

    echo ""
    return 0
}


# Build all required binaries and setup worlds
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  SETUP: Building binaries and preparing worlds"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
# Determine which loaders are needed based on filters
NEED_LEGACY=0
NEED_SQLITE=0
NEED_YAML=0
NEED_SMALL=0
NEED_FULL=0

if [ -z "$FILTER_LOADER" ] || [ "$FILTER_LOADER" = "legacy" ]; then
    NEED_LEGACY=1
fi
if [ -z "$FILTER_LOADER" ] || [ "$FILTER_LOADER" = "sqlite" ]; then
    NEED_SQLITE=1
fi
if [ $QUICK_SKIP_SQLITE -eq 1 ]; then
    NEED_SQLITE=0
fi
# Auto-skip SQLite when libsqlite3 dev headers aren't available. Avoids
# forcing the operator to apt-install just to run the wider test pipeline;
# explicit `--loader=sqlite` invocations still hit the build error so the
# missing dependency is surfaced when actually requested.
if [ $NEED_SQLITE -eq 1 ] && [ "$FILTER_LOADER" != "sqlite" ]; then
    if ! pkg-config --exists sqlite3 2>/dev/null; then
        echo "⚠ SQLite dev headers not found (pkg-config sqlite3). Skipping SQLite tests."
        echo "  Install libsqlite3-dev to enable, or pass --loader=sqlite to force-fail."
        echo ""
        NEED_SQLITE=0
    fi
fi
if [ -z "$FILTER_LOADER" ] || [ "$FILTER_LOADER" = "yaml" ]; then
    NEED_YAML=1
fi

if [ -z "$FILTER_WORLD" ] || [ "$FILTER_WORLD" = "small" ]; then
    NEED_SMALL=1
fi
if [ -z "$FILTER_WORLD" ] || [ "$FILTER_WORLD" = "full" ]; then
    NEED_FULL=1
fi

# Build binaries if needed (with Admin API enabled for testing)
if [ $NEED_LEGACY -eq 1 ]; then

    build_binary "$MUD_DIR/build_test" "-Dadmin_api=true -Dbuild_profile=debug" "legacy" || exit 1
fi

if [ $NEED_SQLITE -eq 1 ]; then

    # sqlite=auto: prefer system libsqlite3, fall back to the meson subproject
    # (which requires a checked-in subprojects/sqlite3.wrap that we currently
    # don't ship).
    build_binary "$MUD_DIR/build_sqlite" "-Dadmin_api=true -Dsqlite=auto -Dbuild_profile=debug" "sqlite" || exit 1
fi
if [ $NEED_YAML -eq 1 ]; then


    build_binary "$MUD_DIR/build_yaml" "-Dadmin_api=true -Dyaml=builtin -Dbuild_profile=debug" "yaml" || exit 1
fi
# Setup worlds
if [ $NEED_SMALL -eq 1 ]; then
    if [ $NEED_LEGACY -eq 1 ]; then
        setup_small_world "legacy" || exit 1
    fi
    if [ $NEED_SQLITE -eq 1 ]; then
        setup_small_world "sqlite" || exit 1
    fi
    if [ $NEED_YAML -eq 1 ]; then
        setup_small_world "yaml" || exit 1
    fi
fi

if [ $NEED_FULL -eq 1 ]; then
    if [ ! -f "$FULL_WORLD_ARCHIVE" ]; then
        echo "WARNING: Full world tests skipped - archive not found: $FULL_WORLD_ARCHIVE"
        NEED_FULL=0
    else
        if [ $NEED_LEGACY -eq 1 ]; then
            setup_full_world "legacy" || exit 1
        fi
        if [ $NEED_SQLITE -eq 1 ]; then
            setup_full_world "sqlite" || exit 1
        fi
        if [ $NEED_YAML -eq 1 ]; then
            setup_full_world "yaml" || exit 1
        fi
    fi
fi

echo ""
echo "=== Prerequisites ready ==="
echo ""


# Function to check if test should run based on filters
should_run_test() {
    local test_name="$1"

    # Extract components from test name (e.g., "Small_YAML_checksums" or "Small_Legacy_AdminAPI")
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

    # Quick mode: only legacy and yaml
    if [ $QUICK_MODE -eq 1 ] && [ "$loader" != "legacy" ] && [ "$loader" != "yaml" ]; then
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

# Function to run Admin API CRUD test
run_admin_api_test() {
    local name="$1"
    local binary="$2"
    local test_dir="$3"
    local world_format="$4"  # legacy, yaml, sqlite

    # Check if this test should run
    if ! should_run_test "$name"; then
        return
    fi

    echo "--- $name (Admin API Test) ---"

    local work_dir="$(dirname "$test_dir")"
    local data_dir="$(basename "$test_dir")"
    local world_size=$(basename "$work_dir" | sed 's/build_\(test\|sqlite\|yaml\)//')

    cd "$work_dir"

    # Recreate world to avoid checksum contamination from previous Admin API tests
    echo "  Recreating world for clean Admin API test..."
    local loader_type=""
    if [ "$world_format" = "legacy" ]; then
        loader_type="legacy"
    elif [ "$world_format" = "sqlite" ]; then
        loader_type="sqlite"
    elif [ "$world_format" = "yaml" ]; then
        loader_type="yaml"
    fi

    if [ "$data_dir" = "small" ]; then
        setup_small_world "$loader_type" > /tmp/admin_api_setup_${loader_type}.log 2>&1
    elif [ "$data_dir" = "full" ]; then
        setup_full_world "$loader_type" > /tmp/admin_api_setup_${loader_type}.log 2>&1
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

    return $test_result
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
    echo "Mode: QUICK (Legacy vs YAML comparison)"
fi
echo "=============================================="
echo ""

# Small World Tests
if [ -z "$FILTER_WORLD" ] || [ "$FILTER_WORLD" = "small" ]; then
    echo "=== SMALL WORLD ==="
    echo ""
    [ -x "$LEGACY_BIN" ] && run_test "Small_Legacy_checksums" "$LEGACY_BIN" "$MUD_DIR/build_test/small" "-W"
    [ -x "$LEGACY_BIN" ] && run_test "Small_Legacy_no_checksums" "$LEGACY_BIN" "$MUD_DIR/build_test/small" ""
    [ -x "$SQLITE_BIN" ] && run_test "Small_SQLite_checksums" "$SQLITE_BIN" "$MUD_DIR/build_sqlite/small" "-W"
    [ -x "$SQLITE_BIN" ] && run_test "Small_SQLite_no_checksums" "$SQLITE_BIN" "$MUD_DIR/build_sqlite/small" ""
    [ -x "$YAML_BIN" ] && [ -d "$MUD_DIR/build_yaml/small" ] && run_test "Small_YAML_checksums" "$YAML_BIN" "$MUD_DIR/build_yaml/small" "-W"
    [ -x "$YAML_BIN" ] && [ -d "$MUD_DIR/build_yaml/small" ] && run_test "Small_YAML_no_checksums" "$YAML_BIN" "$MUD_DIR/build_yaml/small" ""

    # YAML round-trip on the small world (resave -> re-boot -> capture cs3).
    echo ""
    echo "=== SMALL WORLD - YAML ROUND-TRIP ==="
    echo ""
    [ -x "$YAML_BIN" ] && [ -d "$MUD_DIR/build_yaml/small" ] && run_roundtrip_test "Small_YAML_RoundTrip" "$YAML_BIN" "$MUD_DIR/build_yaml/small"
    # Flat-layout round-trip: per-file world -> flat re-emit -> reboot. Checksums
    # must match the per-file YAML run (compared below).
    [ -x "$YAML_BIN" ] && [ -d "$MUD_DIR/build_yaml/small" ] && run_flat_roundtrip_test "Small_YAML_FlatRoundTrip" "$YAML_BIN" "$MUD_DIR/build_yaml/small"

    # Admin API CRUD Tests last -- they recreate the world from scratch and
    # mutate it, so anything order-sensitive has to run before this stage.
    echo ""
    echo "=== SMALL WORLD - ADMIN API TESTS ==="
    echo ""
    [ -x "$LEGACY_BIN" ] && run_admin_api_test "Small_Legacy_AdminAPI" "$LEGACY_BIN" "$MUD_DIR/build_test/small" "legacy"
    [ -x "$SQLITE_BIN" ] && run_admin_api_test "Small_SQLite_AdminAPI" "$SQLITE_BIN" "$MUD_DIR/build_sqlite/small" "sqlite"
    [ -x "$YAML_BIN" ] && [ -d "$MUD_DIR/build_yaml/small" ] && run_admin_api_test "Small_YAML_AdminAPI" "$YAML_BIN" "$MUD_DIR/build_yaml/small" "yaml"
fi

# Full World Tests
if [ -z "$FILTER_WORLD" ] || [ "$FILTER_WORLD" = "full" ]; then
    echo "=== FULL WORLD ==="
    echo ""
    [ -x "$LEGACY_BIN" ] && run_test "Full_Legacy_checksums" "$LEGACY_BIN" "$MUD_DIR/build_test/full" "-W"
    [ -x "$LEGACY_BIN" ] && run_test "Full_Legacy_no_checksums" "$LEGACY_BIN" "$MUD_DIR/build_test/full" ""
    [ -x "$SQLITE_BIN" ] && run_test "Full_SQLite_checksums" "$SQLITE_BIN" "$MUD_DIR/build_sqlite/full" "-W"
    [ -x "$SQLITE_BIN" ] && run_test "Full_SQLite_no_checksums" "$SQLITE_BIN" "$MUD_DIR/build_sqlite/full" ""
    [ -x "$YAML_BIN" ] && [ -d "$MUD_DIR/build_yaml/full" ] && run_test "Full_YAML_checksums" "$YAML_BIN" "$MUD_DIR/build_yaml/full" "-W"
    [ -x "$YAML_BIN" ] && [ -d "$MUD_DIR/build_yaml/full" ] && run_test "Full_YAML_no_checksums" "$YAML_BIN" "$MUD_DIR/build_yaml/full" ""

    # YAML round-trip: resave the YAML world via -S, reboot the re-emitted
    # copy, capture cs3, and compare with cs2 in the CHECKSUM COMPARISON
    # section. Runs after the regular YAML tests because it mutates the world
    # in place; admin API tests below re-extract from the archive anyway.
    echo ""
    echo "=== FULL WORLD - YAML ROUND-TRIP ==="
    echo ""
    [ -x "$YAML_BIN" ] && [ -d "$MUD_DIR/build_yaml/full" ] && run_roundtrip_test "Full_YAML_RoundTrip" "$YAML_BIN" "$MUD_DIR/build_yaml/full"
    [ -x "$YAML_BIN" ] && [ -d "$MUD_DIR/build_yaml/full" ] && run_flat_roundtrip_test "Full_YAML_FlatRoundTrip" "$YAML_BIN" "$MUD_DIR/build_yaml/full"

    # Admin API CRUD Tests last -- they recreate the world from scratch and
    # mutate it (CRUD operations on running server), so anything order-sensitive
    # has to run before this stage.
    echo ""
    echo "=== FULL WORLD - ADMIN API TESTS ==="
    echo ""
    [ -x "$LEGACY_BIN" ] && run_admin_api_test "Full_Legacy_AdminAPI" "$LEGACY_BIN" "$MUD_DIR/build_test/full" "legacy"
    [ -x "$SQLITE_BIN" ] && run_admin_api_test "Full_SQLite_AdminAPI" "$SQLITE_BIN" "$MUD_DIR/build_sqlite/full" "sqlite"
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
        if [ -z "$FILTER_LOADER" ] || [ "$FILTER_LOADER" = "legacy" ] || [ "$FILTER_LOADER" = "sqlite" ]; then
            compare_checksums "Small_Legacy_checksums" "Small_SQLite_checksums"
        fi
        if [ -z "$FILTER_LOADER" ] || [ "$FILTER_LOADER" = "legacy" ] || [ "$FILTER_LOADER" = "yaml" ]; then
            compare_checksums "Small_Legacy_checksums" "Small_YAML_checksums"
        fi
        if [ -z "$FILTER_LOADER" ] || [ "$FILTER_LOADER" = "sqlite" ] || [ "$FILTER_LOADER" = "yaml" ]; then
            compare_checksums "Small_SQLite_checksums" "Small_YAML_checksums"
        fi
        if [ -z "$FILTER_LOADER" ] || [ "$FILTER_LOADER" = "yaml" ]; then
            compare_checksums "Small_YAML_checksums" "Small_YAML_RoundTrip"
            compare_checksums "Small_YAML_checksums" "Small_YAML_FlatRoundTrip"
        fi
        echo ""
    fi

    if [ -z "$FILTER_WORLD" ] || [ "$FILTER_WORLD" = "full" ]; then
        echo "Full world:"
        if [ -z "$FILTER_LOADER" ] || [ "$FILTER_LOADER" = "legacy" ] || [ "$FILTER_LOADER" = "sqlite" ]; then
            compare_checksums "Full_Legacy_checksums" "Full_SQLite_checksums"
        fi
        if [ -z "$FILTER_LOADER" ] || [ "$FILTER_LOADER" = "legacy" ] || [ "$FILTER_LOADER" = "yaml" ]; then
            compare_checksums "Full_Legacy_checksums" "Full_YAML_checksums"
        fi
        if [ -z "$FILTER_LOADER" ] || [ "$FILTER_LOADER" = "sqlite" ] || [ "$FILTER_LOADER" = "yaml" ]; then
            compare_checksums "Full_SQLite_checksums" "Full_YAML_checksums"
        fi
        if [ -z "$FILTER_LOADER" ] || [ "$FILTER_LOADER" = "yaml" ]; then
            compare_checksums "Full_YAML_checksums" "Full_YAML_RoundTrip"
            compare_checksums "Full_YAML_checksums" "Full_YAML_FlatRoundTrip"
        fi
        echo ""
    fi
fi

echo "=============================================="
echo "Tests complete"
echo "=============================================="
