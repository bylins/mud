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
    
    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "Building: $binary_name"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    if [ ! -d "$build_dir" ]; then
        mkdir -p "$build_dir"
    fi
    
    cd "$build_dir"
    
    # Run cmake if needed
    if [ ! -f CMakeCache.txt ]; then
        echo "[1/2] Configuring with CMake..."
        echo "      Options: $cmake_opts"
        cmake $cmake_opts .. > /tmp/build_${binary_name}_cmake.log 2>&1 || {
            echo "✗ ERROR: CMake configuration failed"
            echo "  Log: /tmp/build_${binary_name}_cmake.log"
            cd "$MUD_DIR"
            return 1
        }
        echo "✓ CMake configuration complete"
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
            echo "✗ ERROR: Build failed"
            echo "  Log: /tmp/build_${binary_name}.log"
            echo "  Last 20 lines:"
            tail -20 /tmp/build_${binary_name}.log
            cd "$MUD_DIR"
            return 1
        }
    else
        make circle -j$(nproc) > /tmp/build_${binary_name}.log 2>&1 || {
            echo "✗ ERROR: Build failed"
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
        echo "✗ ERROR: Binary not created despite successful make"
        echo "  Log: /tmp/build_${binary_name}.log"
        echo "  Last 20 lines:"
        tail -20 /tmp/build_${binary_name}.log
        cd "$MUD_DIR"
        return 1
    fi
    echo "✓ Successfully built $build_dir/circle"
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
    
    if [ "$loader" = "legacy" ]; then
        rm -f "$dest_dir"
        if [ ! -L "$dest_dir" ] && [ ! -d "$dest_dir" ]; then
            echo "Creating symlink: $dest_dir -> lib"
            ln -sf "$MUD_DIR/lib" "$dest_dir"
            echo "✓ Symlink created"
        else
            echo "✓ Already set up"
        fi
        echo ""
        return 0
    fi
    
    # For SQLite/YAML: reconfigure CMake to recreate small directory with symlinks
    echo "Reconfiguring CMake to create small world structure..."
    
    cd "$build_dir"
    if [ "$loader" = "sqlite" ]; then
        cmake -DHAVE_SQLITE=ON -DCMAKE_BUILD_TYPE=Test .. > /tmp/cmake_small_sqlite.log 2>&1 || {
            echo "✗ ERROR: CMake reconfiguration failed"
            echo "  Log: /tmp/cmake_small_sqlite.log"
            cd "$MUD_DIR"
            return 1
        }
    elif [ "$loader" = "yaml" ]; then
        cmake -DHAVE_YAML=ON -DCMAKE_BUILD_TYPE=Test .. > /tmp/cmake_small_yaml.log 2>&1 || {
            echo "✗ ERROR: CMake reconfiguration failed"
            echo "  Log: /tmp/cmake_small_yaml.log"
            cd "$MUD_DIR"
            return 1
        }
    fi
    cd "$MUD_DIR"
    echo "✓ CMake recreated $dest_dir with symlinks"
    
    # (lib.template is incomplete - missing text/help/ etc.)
    for dir in text misc cfg; do
        if [ -e "$MUD_DIR/lib/$dir" ]; then
            ln -sfn "$MUD_DIR/lib/$dir" "$dest_dir/$dir"
        fi
    done
    echo "✓ Fixed text/misc/cfg symlinks to use lib"
    
    echo "Converting world data to $loader format..."
    
    if [ "$loader" = "sqlite" ]; then
        echo "  Log: /tmp/convert_small_sqlite.log"
        python3 "$MUD_DIR/tools/convert_to_yaml.py" \
            -i "$MUD_DIR/lib.template" \
            -o "$dest_dir" \
            -f sqlite \
            --db "$dest_dir/world.db" > /tmp/convert_small_sqlite.log 2>&1 || {
            echo "✗ ERROR: Conversion failed"
            echo "  Log: /tmp/convert_small_sqlite.log"
            tail -10 /tmp/convert_small_sqlite.log
            return 1
        }
        echo "✓ Created $dest_dir/world.db"
    elif [ "$loader" = "yaml" ]; then
        echo "  Log: /tmp/convert_small_yaml.log"
        python3 "$MUD_DIR/tools/convert_to_yaml.py" \
            -i "$MUD_DIR/lib.template" \
            -o "$dest_dir" \
            -f yaml > /tmp/convert_small_yaml.log 2>&1 || {
            echo "✗ ERROR: Conversion failed"
            echo "  Log: /tmp/convert_small_yaml.log"
            tail -10 /tmp/convert_small_yaml.log
            return 1
        }
        echo "✓ Created $dest_dir/world/"
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
    
    mkdir -p "$dest_dir"
    tar -xzf "$FULL_WORLD_ARCHIVE" -C "$dest_dir" > /tmp/extract_full.log 2>&1 || {
        echo "✗ ERROR: Extraction failed"
        echo "  Log: /tmp/extract_full.log"
        return 1
    }
    echo "✓ Extracted successfully"
    
    # Archive contains ./lib/ which extracts to $dest_dir/lib/
    if [ "$loader" = "legacy" ]; then
        echo "[2/3] Preparing legacy world..."
        mv "$dest_dir/lib/"* "$dest_dir/"
        rmdir "$dest_dir/lib"
        echo "✓ Ready at $dest_dir"
    elif [ "$loader" = "sqlite" ] || [ "$loader" = "yaml" ]; then
        echo "[2/3] Converting world data (this may take several minutes)..."
        echo "      Format: $loader"
        echo "      Log: /tmp/convert_full_${loader}.log"
        echo "      Tip: Run 'tail -f /tmp/convert_full_${loader}.log' in another terminal to watch progress"
        echo ""
        
        if [ "$loader" = "sqlite" ]; then
            python3 "$MUD_DIR/tools/convert_to_yaml.py" \
                -i "$dest_dir/lib" \
                -o "$dest_dir" \
                -f sqlite \
                --db "$dest_dir/world.db" > /tmp/convert_full_sqlite.log 2>&1 || {
                echo "✗ ERROR: Conversion failed"
                echo "  Log: /tmp/convert_full_sqlite.log"
                tail -20 /tmp/convert_full_sqlite.log
                return 1
            }
            echo "✓ Created $dest_dir/world.db"
        else
            python3 "$MUD_DIR/tools/convert_to_yaml.py" \
                -i "$dest_dir/lib" \
                -o "$dest_dir" \
                -f yaml > /tmp/convert_full_yaml.log 2>&1 || {
                echo "✗ ERROR: Conversion failed"
                echo "  Log: /tmp/convert_full_yaml.log"
                tail -20 /tmp/convert_full_yaml.log
                return 1
            }
            echo "✓ Created $dest_dir/world/"
        fi
        
        echo "[3/3] Cleaning up..."
        rm -rf "$dest_dir/lib"
        echo "✓ Cleanup complete"
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

# Build binaries if needed
if [ $NEED_LEGACY -eq 1 ]; then
    
    build_binary "$MUD_DIR/build_test" "-DCMAKE_BUILD_TYPE=Test" "legacy" || exit 1
fi

if [ $NEED_SQLITE -eq 1 ]; then
    
    build_binary "$MUD_DIR/build_sqlite" "-DHAVE_SQLITE=ON -DCMAKE_BUILD_TYPE=Test" "sqlite" || exit 1
fi
if [ $NEED_YAML -eq 1 ]; then
    

    build_binary "$MUD_DIR/build_yaml" "-DHAVE_YAML=ON -DCMAKE_BUILD_TYPE=Test" "yaml" || exit 1
fi
# Setup worlds
if [ $NEED_SMALL -eq 1 ]; then
    if [ $NEED_LEGACY -eq 1 ] && [ ! -e "$MUD_DIR/build_test/small" ]; then
        setup_small_world "legacy" || exit 1
    fi
    if [ $NEED_SQLITE -eq 1 ]; then
        if [ ! -f "$MUD_DIR/build_sqlite/small/world.db" ] || [ ! -L "$MUD_DIR/build_sqlite/small/cfg" ]; then
        setup_small_world "sqlite" || exit 1
        fi
    fi
    if [ $NEED_YAML -eq 1 ]; then
        if [ ! -d "$MUD_DIR/build_yaml/small/world" ] || [ ! -L "$MUD_DIR/build_yaml/small/cfg" ]; then
        setup_small_world "yaml" || exit 1
        fi
    fi
fi

if [ $NEED_FULL -eq 1 ]; then
    if [ ! -f "$FULL_WORLD_ARCHIVE" ]; then
        echo "WARNING: Full world tests skipped - archive not found: $FULL_WORLD_ARCHIVE"
        NEED_FULL=0
    else
        if [ $NEED_LEGACY -eq 1 ] && [ ! -d "$MUD_DIR/build_test/full" ]; then
            setup_full_world "legacy" || exit 1
        fi
        if [ $NEED_SQLITE -eq 1 ] && [ ! -f "$MUD_DIR/build_sqlite/full/world.db" ]; then
            setup_full_world "sqlite" || exit 1
        fi
        if [ $NEED_YAML -eq 1 ] && [ ! -d "$MUD_DIR/build_yaml/full/world" ]; then
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
    
    # Extract components from test name (e.g., "Small_YAML_checksums")
    local world=$(echo "$test_name" | cut -d_ -f1 | tr 'A-Z' 'a-z')
    local loader=$(echo "$test_name" | cut -d_ -f2 | tr 'A-Z' 'a-z')
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
    
    if [ -n "$FILTER_CHECKSUMS" ]; then
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

    # Start server in background
    echo "  Running: $binary -d $data_dir $extra_flags 4000"
    "$binary" -d "$data_dir" $extra_flags 4000 > "$data_dir/stdout.log" 2>&1 &

    # Wait for syslog to appear (max 10 seconds)
    local waited=0
    while [ $waited -lt 60 ] && [ ! -f "$data_dir/syslog" ]; do
        sleep 0.5
        waited=$((waited + 1))
    done
    
    if [ ! -f "$data_dir/syslog" ]; then
        echo "  ✗ ERROR: Server did not create syslog"
        cat "$data_dir/stdout.log" 2>/dev/null || echo "  (no stdout log)"
        cd "$MUD_DIR"
        return 1
    fi

    # Wait for boot to complete (max 5 minutes)
    waited=0
    local boot_success=0
    while [ $waited -lt 300 ]; do
        
        # Check if boot completed
        if LANG=C grep -qa "Boot db -- DONE" "$data_dir/syslog" 2>/dev/null; then
            boot_success=1
            sleep 1
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
        echo "  ✗ ERROR: Boot timeout (5 minutes exceeded)"
        echo "  Last 30 lines of syslog:"
        tail -30 "$data_dir/syslog" 2>/dev/null || echo "  (no syslog found)"
        cd "$MUD_DIR"
        return 1
    fi

    # Extract and display results
    if [ -f syslog ]; then
        # Boot times
        local begin=$(LANG=C grep -a "Boot db -- BEGIN" syslog | head -1 | cut -d' ' -f2)
        local done_time=$(LANG=C grep -a "Boot db -- DONE" syslog | head -1 | cut -d' ' -f2)

        if [ -n "$begin" ] && [ -n "$done_time" ]; then
            local begin_sec=$(echo "$begin" | awk -F: '{printf "%.3f", ($1*3600)+($2*60)+$3}')
            local done_sec=$(echo "$done_time" | awk -F: '{printf "%.3f", ($1*3600)+($2*60)+$3}')
            local duration=$(echo "$done_sec - $begin_sec" | bc)
            echo "  Boot time:     ${duration}s"
        fi

        # Object counts
        LANG=C grep -aoE "(Zones:|Rooms:|Mobs:|Objects:|Triggers:).*" syslog | tail -5 | while read line; do
            echo "  $line"
        done

        # Checksum calculation time
        local cs_begin=$(LANG=C grep -a "Calculating world checksums" syslog | head -1 | cut -d' ' -f2)
        local cs_done=$(LANG=C grep -a "Detailed buffers saved" syslog | head -1 | cut -d' ' -f2)
        if [ -n "$cs_begin" ] && [ -n "$cs_done" ]; then
            local cs_begin_sec=$(echo "$cs_begin" | awk -F: '{printf "%.3f", ($1*3600)+($2*60)+$3}')
            local cs_done_sec=$(echo "$cs_done" | awk -F: '{printf "%.3f", ($1*3600)+($2*60)+$3}')
            local cs_duration=$(echo "$cs_done_sec - $cs_begin_sec" | bc)
            echo "  Checksum time: ${cs_duration}s"
        fi

        # Save checksums for comparison
        if [ -f checksums_detailed.txt ]; then
            local fname=$(echo "$name" | tr ' ' '_')
            cp checksums_detailed.txt "/tmp/${fname}_checksums.txt"
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
    [ -n "$LEGACY_BIN" ] && run_test "Small_Legacy_checksums" "$LEGACY_BIN" "$MUD_DIR/build_test/small" ""
    [ -n "$LEGACY_BIN" ] && run_test "Small_Legacy_no_checksums" "$LEGACY_BIN" "$MUD_DIR/build_test/small" "-C"
    [ -n "$SQLITE_BIN" ] && run_test "Small_SQLite_checksums" "$SQLITE_BIN" "$MUD_DIR/build_sqlite/small" ""
    [ -n "$SQLITE_BIN" ] && run_test "Small_SQLite_no_checksums" "$SQLITE_BIN" "$MUD_DIR/build_sqlite/small" "-C "
    [ -n "$YAML_BIN" ] && [ -d "$MUD_DIR/build_yaml/small" ] && run_test "Small_YAML_checksums" "$YAML_BIN" "$MUD_DIR/build_yaml/small" ""
    [ -n "$YAML_BIN" ] && [ -d "$MUD_DIR/build_yaml/small" ] && run_test "Small_YAML_no_checksums" "$YAML_BIN" "$MUD_DIR/build_yaml/small" "-C"
fi

# Full World Tests
if [ -z "$FILTER_WORLD" ] || [ "$FILTER_WORLD" = "full" ]; then
    echo "=== FULL WORLD ==="
    echo ""
    [ -n "$LEGACY_BIN" ] && run_test "Full_Legacy_checksums" "$LEGACY_BIN" "$MUD_DIR/build_test/full" ""
    [ -n "$LEGACY_BIN" ] && run_test "Full_Legacy_no_checksums" "$LEGACY_BIN" "$MUD_DIR/build_test/full" "-C"
    [ -n "$SQLITE_BIN" ] && run_test "Full_SQLite_checksums" "$SQLITE_BIN" "$MUD_DIR/build_sqlite/full" ""
    [ -n "$SQLITE_BIN" ] && run_test "Full_SQLite_no_checksums" "$SQLITE_BIN" "$MUD_DIR/build_sqlite/full" "-C"
    [ -n "$YAML_BIN" ] && [ -d "$MUD_DIR/build_yaml/full" ] && run_test "Full_YAML_checksums" "$YAML_BIN" "$MUD_DIR/build_yaml/full" ""
    [ -n "$YAML_BIN" ] && [ -d "$MUD_DIR/build_yaml/full" ] && run_test "Full_YAML_no_checksums" "$YAML_BIN" "$MUD_DIR/build_yaml/full" "-C"
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
