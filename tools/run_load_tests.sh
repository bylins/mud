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
#   --rebuild         Force full rebuild (make clean && make)
#   --build-type=TYPE Set CMAKE_BUILD_TYPE (Release|Debug|Test|FastTest)
#   --recreate-builds Delete and recreate all build directories (implies world recreation)
#   --help            Show this help
#
# ENVIRONMENT VARIABLES:
#   FULL_WORLD_ARCHIVE   Path to full world archive (default: ~/repos/world.tgz)
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

# Quick mode: only Small_Legacy_checksums and Small_YAML_checksums
if [ $QUICK_MODE -eq 1 ]; then
    FILTER_LOADER=""  # Will run both legacy and yaml
    FILTER_WORLD="small"
    FILTER_CHECKSUMS="yes"
    echo "Quick mode: Running Small_Legacy_checksums and Small_YAML_checksums"
    echo ""
fi

# Default location of full world archive
FULL_WORLD_ARCHIVE="${FULL_WORLD_ARCHIVE:-$HOME/repos/world.tgz}"

# Function to build a specific binary variant
build_binary() {
    local build_dir="$1"
    local cmake_opts="$2"
    local binary_name="$3"
    local needs_reconfigure=0
    
    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "Building: $binary_name"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    # Check if we need to recreate or reconfigure
    if [ ! -d "$build_dir" ]; then
        mkdir -p "$build_dir"
        needs_reconfigure=1
    elif [ $RECREATE_BUILDS -eq 1 ]; then
        echo "Recreating build directory (--recreate-builds)..."
        rm -rf "$build_dir"
        mkdir -p "$build_dir"
        needs_reconfigure=1
    elif [ -n "$BUILD_TYPE" ] && [ -f "$build_dir/CMakeCache.txt" ]; then
        # Check if BUILD_TYPE changed
        local current_type=$(grep "CMAKE_BUILD_TYPE:" "$build_dir/CMakeCache.txt" 2>/dev/null | cut -d= -f2)
        if [ "$current_type" != "$BUILD_TYPE" ]; then
            echo "Build type changed ($current_type -> $BUILD_TYPE), reconfiguring..."
            needs_reconfigure=1
        fi
    elif [ ! -f "$build_dir/CMakeCache.txt" ]; then
        needs_reconfigure=1
    fi
    
    cd "$build_dir"
    
    # Apply BUILD_TYPE override if specified
    local final_cmake_opts="$cmake_opts"
    if [ -n "$BUILD_TYPE" ]; then
        # Replace existing -DCMAKE_BUILD_TYPE or add if missing
        if echo "$cmake_opts" | grep -q "CMAKE_BUILD_TYPE"; then
            final_cmake_opts=$(echo "$cmake_opts" | sed "s/-DCMAKE_BUILD_TYPE=[^ ]*/-DCMAKE_BUILD_TYPE=$BUILD_TYPE/")
        else
            final_cmake_opts="$cmake_opts -DCMAKE_BUILD_TYPE=$BUILD_TYPE"
        fi
    fi
    
    # Run cmake if needed
    if [ $needs_reconfigure -eq 1 ]; then
        echo "[1/2] Configuring with CMake..."
        echo "      Options: $final_cmake_opts"
        cmake $final_cmake_opts .. > /tmp/build_${binary_name}_cmake.log 2>&1 || {
            echo "X ERROR: CMake configuration failed"
            echo "  Log: /tmp/build_${binary_name}_cmake.log"
            cd "$MUD_DIR"
            return 1
        }
        echo " CMake configuration complete"
    else
        echo "[1/2] Using cached CMake configuration"
    fi
    
    # Build
    echo "[2/2] Compiling (this may take a while)..."
    echo "      Log: /tmp/build_${binary_name}.log"
    echo "      Tip: Run 'tail -f /tmp/build_${binary_name}.log' in another terminal to watch progress"
    echo ""
    # Clean if rebuild requested
    if [ $REBUILD_MODE -eq 1 ]; then
        echo "      Running make clean (rebuild mode)..."
        make clean > /tmp/build_${binary_name}.log 2>&1
        make circle -j$(nproc) >> /tmp/build_${binary_name}.log 2>&1 || {
            echo "X ERROR: Build failed"
            echo "  Log: /tmp/build_${binary_name}.log"
            echo "  Last 20 lines:"
            tail -20 /tmp/build_${binary_name}.log
            cd "$MUD_DIR"
            return 1
        }
    else
        make circle -j$(nproc) > /tmp/build_${binary_name}.log 2>&1 || {
            echo "X ERROR: Build failed"
            echo "  Log: /tmp/build_${binary_name}.log"
            echo "  Last 20 lines:"
            tail -20 /tmp/build_${binary_name}.log
            cd "$MUD_DIR"
            return 1
        }
    fi
    
    cd "$MUD_DIR"
    
    # Verify binary was created
    if [ ! -x "$build_dir/circle" ]; then
        echo "X ERROR: Binary not created despite successful make"
        echo "  Log: /tmp/build_${binary_name}.log"
        echo "  Last 20 lines:"
        tail -20 /tmp/build_${binary_name}.log
        cd "$MUD_DIR"
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

    # Reconfigure CMake to recreate small directory with symlinks
    echo "Reconfiguring CMake to create small world structure..."

    cd "$build_dir"
    if [ "$loader" = "legacy" ]; then
        cmake -DCMAKE_BUILD_TYPE=Debug .. > /tmp/cmake_small_legacy.log 2>&1 || {
            echo "X ERROR: CMake reconfiguration failed"
            echo "  Log: /tmp/cmake_small_legacy.log"
            cd "$MUD_DIR"
            return 1
        }
    elif [ "$loader" = "sqlite" ]; then
        cmake -DHAVE_SQLITE=ON -DCMAKE_BUILD_TYPE=Debug .. > /tmp/cmake_small_sqlite.log 2>&1 || {
            echo "X ERROR: CMake reconfiguration failed"
            echo "  Log: /tmp/cmake_small_sqlite.log"
            cd "$MUD_DIR"
            return 1
        }
    elif [ "$loader" = "yaml" ]; then
        cmake -DHAVE_YAML=ON -DCMAKE_BUILD_TYPE=Debug .. > /tmp/cmake_small_yaml.log 2>&1 || {
            echo "X ERROR: CMake reconfiguration failed"
            echo "  Log: /tmp/cmake_small_yaml.log"
            cd "$MUD_DIR"
            return 1
        }
    fi
    cd "$MUD_DIR"
    echo " CMake created $dest_dir (copied lib + lib.template)"

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
    
    if [ ! -f "$FULL_WORLD_ARCHIVE" ]; then
        echo "⚠ WARNING: Full world archive not found: $FULL_WORLD_ARCHIVE"
        echo "  Set FULL_WORLD_ARCHIVE environment variable to override"
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
    tar -xzf "$FULL_WORLD_ARCHIVE" -C "$dest_dir" > /tmp/extract_full.log 2>&1 || {
        echo "X ERROR: Extraction failed"
        echo "  Log: /tmp/extract_full.log"
        return 1
    }
    echo " Extracted successfully"
    
    # Archive contains ./lib/ which extracts to $dest_dir/lib/
    if [ "$loader" = "legacy" ]; then
        echo "[2/3] Preparing legacy world..."
        mv "$dest_dir/lib/"* "$dest_dir/"
        rmdir "$dest_dir/lib"
        echo " Ready at $dest_dir"
    elif [ "$loader" = "sqlite" ] || [ "$loader" = "yaml" ]; then
        echo "[2/3] Converting world data (this may take several minutes)..."
        echo "      Format: $loader"
        echo "      Log: /tmp/convert_full_${loader}.log"
        echo "      Tip: Run 'tail -f /tmp/convert_full_${loader}.log' in another terminal to watch progress"
        echo ""

        if [ "$loader" = "sqlite" ]; then
            python3 "$MUD_DIR/tools/converter/convert_to_yaml.py" \
                -i "$dest_dir/lib" \
                -o "$dest_dir/lib" \
                -f sqlite \
                --db "$dest_dir/lib/world.db" \
 > /tmp/convert_full_sqlite.log 2>&1 || {
                echo "X ERROR: Conversion failed"
                echo "  Log: /tmp/convert_full_sqlite.log"
                tail -20 /tmp/convert_full_sqlite.log
                return 1
            }
            echo " Converted to SQLite format"
        else
            python3 "$MUD_DIR/tools/converter/convert_to_yaml.py" \
                -i "$dest_dir/lib" \
                -o "$dest_dir/lib" \
                -f yaml \
 > /tmp/convert_full_yaml.log 2>&1 || {
                echo "X ERROR: Conversion failed"
                echo "  Log: /tmp/convert_full_yaml.log"
                tail -20 /tmp/convert_full_yaml.log
                return 1
            }
            echo " Converted to YAML format"
        fi

        echo "[3/3] Flattening world structure..."
        mv "$dest_dir/lib/"* "$dest_dir/"
        rmdir "$dest_dir/lib"

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

    build_binary "$MUD_DIR/build_test" "-DENABLE_ADMIN_API=ON -DCMAKE_BUILD_TYPE=Debug" "legacy" || exit 1
fi

if [ $NEED_SQLITE -eq 1 ]; then

    build_binary "$MUD_DIR/build_sqlite" "-DENABLE_ADMIN_API=ON -DHAVE_SQLITE=ON -DCMAKE_BUILD_TYPE=Debug" "sqlite" || exit 1
fi
if [ $NEED_YAML -eq 1 ]; then


    build_binary "$MUD_DIR/build_yaml" "-DENABLE_ADMIN_API=ON -DHAVE_YAML=ON -DCMAKE_BUILD_TYPE=Debug" "yaml" || exit 1
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

    # Admin API tests ignore checksums filter (they test API, not boot performance)
    if [ $is_admin_api -eq 0 ] && [ -n "$FILTER_CHECKSUMS" ]; then
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
    enable_admin_api "$data_dir/misc/configuration.xml"

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
    [ -n "$LEGACY_BIN" ] && run_test "Small_Legacy_checksums" "$LEGACY_BIN" "$MUD_DIR/build_test/small" "-W"
    [ -n "$LEGACY_BIN" ] && run_test "Small_Legacy_no_checksums" "$LEGACY_BIN" "$MUD_DIR/build_test/small" ""
    [ -n "$SQLITE_BIN" ] && run_test "Small_SQLite_checksums" "$SQLITE_BIN" "$MUD_DIR/build_sqlite/small" "-W"
    [ -n "$SQLITE_BIN" ] && run_test "Small_SQLite_no_checksums" "$SQLITE_BIN" "$MUD_DIR/build_sqlite/small" ""
    [ -n "$YAML_BIN" ] && [ -d "$MUD_DIR/build_yaml/small" ] && run_test "Small_YAML_checksums" "$YAML_BIN" "$MUD_DIR/build_yaml/small" "-W"
    [ -n "$YAML_BIN" ] && [ -d "$MUD_DIR/build_yaml/small" ] && run_test "Small_YAML_no_checksums" "$YAML_BIN" "$MUD_DIR/build_yaml/small" ""

    # Admin API CRUD Tests (only for checksums variant to avoid duplication)
    echo ""
    echo "=== SMALL WORLD - ADMIN API TESTS ==="
    echo ""
    [ -n "$LEGACY_BIN" ] && run_admin_api_test "Small_Legacy_AdminAPI" "$LEGACY_BIN" "$MUD_DIR/build_test/small" "legacy"
    [ -n "$SQLITE_BIN" ] && run_admin_api_test "Small_SQLite_AdminAPI" "$SQLITE_BIN" "$MUD_DIR/build_sqlite/small" "sqlite"
    [ -n "$YAML_BIN" ] && [ -d "$MUD_DIR/build_yaml/small" ] && run_admin_api_test "Small_YAML_AdminAPI" "$YAML_BIN" "$MUD_DIR/build_yaml/small" "yaml"
fi

# Full World Tests
if [ -z "$FILTER_WORLD" ] || [ "$FILTER_WORLD" = "full" ]; then
    echo "=== FULL WORLD ==="
    echo ""
    [ -n "$LEGACY_BIN" ] && run_test "Full_Legacy_checksums" "$LEGACY_BIN" "$MUD_DIR/build_test/full" "-W"
    [ -n "$LEGACY_BIN" ] && run_test "Full_Legacy_no_checksums" "$LEGACY_BIN" "$MUD_DIR/build_test/full" ""
    [ -n "$SQLITE_BIN" ] && run_test "Full_SQLite_checksums" "$SQLITE_BIN" "$MUD_DIR/build_sqlite/full" "-W"
    [ -n "$SQLITE_BIN" ] && run_test "Full_SQLite_no_checksums" "$SQLITE_BIN" "$MUD_DIR/build_sqlite/full" ""
    [ -n "$YAML_BIN" ] && [ -d "$MUD_DIR/build_yaml/full" ] && run_test "Full_YAML_checksums" "$YAML_BIN" "$MUD_DIR/build_yaml/full" "-W"
    [ -n "$YAML_BIN" ] && [ -d "$MUD_DIR/build_yaml/full" ] && run_test "Full_YAML_no_checksums" "$YAML_BIN" "$MUD_DIR/build_yaml/full" ""

    # Admin API CRUD Tests (only for checksums variant to avoid duplication)
    echo ""
    echo "=== FULL WORLD - ADMIN API TESTS ==="
    echo ""
    [ -n "$LEGACY_BIN" ] && run_admin_api_test "Full_Legacy_AdminAPI" "$LEGACY_BIN" "$MUD_DIR/build_test/full" "legacy"
    [ -n "$SQLITE_BIN" ] && run_admin_api_test "Full_SQLite_AdminAPI" "$SQLITE_BIN" "$MUD_DIR/build_sqlite/full" "sqlite"
    [ -n "$YAML_BIN" ] && [ -d "$MUD_DIR/build_yaml/full" ] && run_admin_api_test "Full_YAML_AdminAPI" "$YAML_BIN" "$MUD_DIR/build_yaml/full" "yaml"
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
        echo ""
    fi
fi

echo "=============================================="
echo "Tests complete"
echo "=============================================="
