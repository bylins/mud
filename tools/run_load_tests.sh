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
    
    echo "Building $binary_name..."
    
    if [ ! -d "$build_dir" ]; then
        mkdir -p "$build_dir"
    fi
    
    cd "$build_dir"
    
    # Run cmake if CMakeCache.txt doesn't exist
    if [ ! -f CMakeCache.txt ]; then
        echo "  Running cmake with options: $cmake_opts"
        cmake $cmake_opts .. || {
            echo "ERROR: cmake failed for $binary_name"
            cd "$MUD_DIR"
            return 1
        }
    fi
    
    # Build the binary
    echo "  Compiling..."
    make circle -j$(nproc) > /tmp/build_${binary_name}.log 2>&1 || {
        echo "ERROR: Build failed for $binary_name"
        echo "Log: /tmp/build_${binary_name}.log"
        cd "$MUD_DIR"
        return 1
    }
    
    cd "$MUD_DIR"
    echo "  ✓ Built $build_dir/circle"
    return 0
}


# Function to setup small world (uses lib.template)
setup_small_world() {
    local loader="$1"  # legacy, sqlite, or yaml
    
    # Determine build directory
    if [ "$loader" = "legacy" ]; then
        local build_dir="$MUD_DIR/build_test"
    elif [ "$loader" = "sqlite" ]; then
        local build_dir="$MUD_DIR/build_sqlite"
    elif [ "$loader" = "yaml" ]; then
        local build_dir="$MUD_DIR/build_yaml"
    fi
    
    local dest_dir="$build_dir/small"
    
    if [ "$loader" = "legacy" ]; then
        # Legacy uses symlink to lib
        rm -f "$dest_dir"  # Remove old symlink if exists
        if [ ! -L "$dest_dir" ] && [ ! -d "$dest_dir" ]; then
            echo "Setting up small_legacy (symlink to lib)..."
            ln -sf "$MUD_DIR/lib" "$dest_dir"
            echo "  ✓ Created symlink $dest_dir -> lib"
        fi
        return 0
    fi
    
    # For SQLite/YAML: convert from lib.template into build-specific directory
    echo "Converting small world for $loader..."
    
    mkdir -p "$dest_dir"
    
    if [ "$loader" = "sqlite" ]; then
        python3 "$MUD_DIR/tools/convert_to_yaml.py" \
            --sqlite "$dest_dir/world.db" \
            "$MUD_DIR/lib.template" > /tmp/convert_small_sqlite.log 2>&1 || {
            echo "ERROR: SQLite conversion failed"
            echo "Log: /tmp/convert_small_sqlite.log"
            return 1
        }
        echo "  ✓ Created $dest_dir/world.db"
    elif [ "$loader" = "yaml" ]; then
        python3 "$MUD_DIR/tools/convert_to_yaml.py" \
            -o "$dest_dir" \
            -f yaml \
            "$MUD_DIR/lib.template" > /tmp/convert_small_yaml.log 2>&1 || {
            echo "ERROR: YAML conversion failed"
            echo "Log: /tmp/convert_small_yaml.log"
            return 1
        }
        echo "  ✓ Created $dest_dir/world/"
    fi
    
    return 0
}

# Function to setup full world (extracts from archive)
setup_full_world() {
    local loader="$1"  # legacy, sqlite, or yaml
    
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
        echo "WARNING: Full world archive not found: $FULL_WORLD_ARCHIVE"
        echo "Set FULL_WORLD_ARCHIVE environment variable to override"
        return 1
    fi
    
    # Extract archive to temporary location
    local temp_extract="/tmp/full_world_$$"
    mkdir -p "$temp_extract"
    
    echo "Extracting full world for $loader..."
    tar -xzf "$FULL_WORLD_ARCHIVE" -C "$temp_extract" > /tmp/extract_full.log 2>&1 || {
        echo "ERROR: Failed to extract $FULL_WORLD_ARCHIVE"
        echo "Log: /tmp/extract_full.log"
        rm -rf "$temp_extract"
        return 1
    }
    
    # Find the extracted lib directory
    local extracted_lib="$temp_extract/lib"
    if [ ! -d "$extracted_lib" ]; then
        # Maybe it extracted to world/lib or similar
        extracted_lib=$(find "$temp_extract" -type d -name "lib" | head -1)
        if [ -z "$extracted_lib" ]; then
            echo "ERROR: Cannot find lib directory in extracted archive"
            rm -rf "$temp_extract"
            return 1
        fi
    fi
    
    echo "  Extracted to $temp_extract"
    
    if [ "$loader" = "legacy" ]; then
        # For legacy: just copy the extracted lib
        rm -rf "$dest_dir"
        cp -r "$extracted_lib" "$dest_dir"
        echo "  ✓ Copied to $dest_dir"
    elif [ "$loader" = "sqlite" ]; then
        # For SQLite: convert to database
        mkdir -p "$dest_dir"
        python3 "$MUD_DIR/tools/convert_to_yaml.py" \
            --sqlite "$dest_dir/world.db" \
            "$extracted_lib" > /tmp/convert_full_sqlite.log 2>&1 || {
            echo "ERROR: SQLite conversion failed"
            echo "Log: /tmp/convert_full_sqlite.log"
            rm -rf "$temp_extract"
            return 1
        }
        echo "  ✓ Created $dest_dir/world.db"
    elif [ "$loader" = "yaml" ]; then
        # For YAML: convert to YAML files
        python3 "$MUD_DIR/tools/convert_to_yaml.py" \
            -o "$dest_dir" \
            -f yaml \
            "$extracted_lib" > /tmp/convert_full_yaml.log 2>&1 || {
            echo "ERROR: YAML conversion failed"
            echo "Log: /tmp/convert_full_yaml.log"
            rm -rf "$temp_extract"
            return 1
        }
        echo "  ✓ Created $dest_dir/world/"
    fi
    
    # Cleanup temp extraction
    rm -rf "$temp_extract"
    return 0
}

# Build all required binaries and setup worlds
echo "=== Setting up prerequisites ==="
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
    if [ ! -x "$LEGACY_BIN" ]; then
        build_binary "$MUD_DIR/build_test" "-DCMAKE_BUILD_TYPE=Test" "legacy" || exit 1
    fi
fi

if [ $NEED_SQLITE -eq 1 ]; then
    if [ ! -x "$SQLITE_BIN" ]; then
        build_binary "$MUD_DIR/build_sqlite" "-DHAVE_SQLITE=ON -DCMAKE_BUILD_TYPE=Test" "sqlite" || exit 1
    fi
fi

if [ $NEED_YAML -eq 1 ]; then
    if [ ! -x "$YAML_BIN" ]; then
        build_binary "$MUD_DIR/build_yaml" "-DHAVE_YAML=ON -DCMAKE_BUILD_TYPE=Test" "yaml" || exit 1
    fi
fi

# Setup worlds
if [ $NEED_SMALL -eq 1 ]; then
    if [ $NEED_LEGACY -eq 1 ] && [ ! -e "$MUD_DIR/build_test/small" ]; then
        setup_small_world "legacy" || exit 1
    fi
    if [ $NEED_SQLITE -eq 1 ] && [ ! -f "$MUD_DIR/build_sqlite/small/world.db" ]; then
        setup_small_world "sqlite" || exit 1
    fi
    if [ $NEED_YAML -eq 1 ] && [ ! -d "$MUD_DIR/build_yaml/small/world" ]; then
        setup_small_world "yaml" || exit 1
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

    cd "$test_dir"
    rm -rf syslog checksums_detailed.txt checksums_buffers 2>/dev/null || true

    # Start server in background
    "$binary" $extra_flags 4000 > /dev/null 2>&1 &
    local pid=$!

    # Wait for boot to complete (max 5 minutes)
    local waited=0
    while [ $waited -lt 300 ]; do
        if LANG=C grep -qa "Boot db -- DONE" syslog 2>/dev/null; then
            sleep 1
            break
        fi
        sleep 1
        waited=$((waited + 1))
    done

    # Kill server
    kill $pid 2>/dev/null
    wait $pid 2>/dev/null || true

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

        # Checksum timing (if enabled)
        local chk_start=$(LANG=C grep -a "Calculating world checksums" syslog | head -1 | cut -d' ' -f2)
        local chk_end=$(LANG=C grep -a "Detailed checksums saved" syslog | head -1 | cut -d' ' -f2)
        if [ -n "$chk_start" ] && [ -n "$chk_end" ]; then
            local chk_start_sec=$(echo "$chk_start" | awk -F: '{printf "%.3f", ($1*3600)+($2*60)+$3}')
            local chk_end_sec=$(echo "$chk_end" | awk -F: '{printf "%.3f", ($1*3600)+($2*60)+$3}')
            local chk_duration=$(echo "$chk_end_sec - $chk_start_sec" | bc)
            echo "  Checksum time: ${chk_duration}s"
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
}

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
    [ -n "$SQLITE_BIN" ] && run_test "Small_SQLite_no_checksums" "$SQLITE_BIN" "$MUD_DIR/build_sqlite/small" "-C"
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
