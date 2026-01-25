#!/bin/bash
#
# Run world loading performance tests
#
# PREREQUISITES:
#   1. Run ./tools/setup_test_dirs.sh to create test directories
#   2. Build binaries:
#      - build_test/circle (Legacy loader)
#      - build_sqlite/circle (SQLite loader, with -DHAVE_SQLITE=ON)
#      - build_yaml/circle (YAML loader, with -DHAVE_YAML=ON)
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

MUD_DIR="$(cd "$(dirname "$0")/.." && pwd)"
TEST_DIR="$MUD_DIR/test"
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
            grep "^#" "$0" | grep -v "^#!/" | sed 's/^# \?//'
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

# Check prerequisites
if [ ! -x "$LEGACY_BIN" ]; then
    echo "WARNING: Legacy binary not found: $LEGACY_BIN"
    echo "Run: cd build_test && make circle -j\$(nproc)"
    LEGACY_BIN=""
fi

if [ ! -x "$SQLITE_BIN" ]; then
    echo "WARNING: SQLite binary not found: $SQLITE_BIN"
    echo "Run: cd build_sqlite && make circle -j\$(nproc)"
    SQLITE_BIN=""
fi

if [ ! -x "$YAML_BIN" ]; then
    echo "WARNING: YAML binary not found: $YAML_BIN"
    echo "Run: cd build_yaml && cmake -DHAVE_YAML=ON -DCMAKE_BUILD_TYPE=Test .. && make circle -j\$(nproc)"
    YAML_BIN=""
fi

if [ ! -d "$TEST_DIR/small_legacy" ]; then
    echo "ERROR: Test directories not set up"
    echo "Run: ./tools/setup_test_dirs.sh"
    exit 1
fi

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
    [ -n "$LEGACY_BIN" ] && run_test "Small_Legacy_checksums" "$LEGACY_BIN" "$TEST_DIR/small_legacy" ""
    [ -n "$LEGACY_BIN" ] && run_test "Small_Legacy_no_checksums" "$LEGACY_BIN" "$TEST_DIR/small_legacy" "-C"
    [ -n "$SQLITE_BIN" ] && run_test "Small_SQLite_checksums" "$SQLITE_BIN" "$TEST_DIR/small_sqlite" ""
    [ -n "$SQLITE_BIN" ] && run_test "Small_SQLite_no_checksums" "$SQLITE_BIN" "$TEST_DIR/small_sqlite" "-C"
    [ -n "$YAML_BIN" ] && [ -d "$TEST_DIR/small_yaml" ] && run_test "Small_YAML_checksums" "$YAML_BIN" "$TEST_DIR/small_yaml" ""
    [ -n "$YAML_BIN" ] && [ -d "$TEST_DIR/small_yaml" ] && run_test "Small_YAML_no_checksums" "$YAML_BIN" "$TEST_DIR/small_yaml" "-C"
fi

# Full World Tests
if [ -z "$FILTER_WORLD" ] || [ "$FILTER_WORLD" = "full" ]; then
    echo "=== FULL WORLD ==="
    echo ""
    [ -n "$LEGACY_BIN" ] && run_test "Full_Legacy_checksums" "$LEGACY_BIN" "$TEST_DIR/full_legacy" ""
    [ -n "$LEGACY_BIN" ] && run_test "Full_Legacy_no_checksums" "$LEGACY_BIN" "$TEST_DIR/full_legacy" "-C"
    [ -n "$SQLITE_BIN" ] && run_test "Full_SQLite_checksums" "$SQLITE_BIN" "$TEST_DIR/full_sqlite" ""
    [ -n "$SQLITE_BIN" ] && run_test "Full_SQLite_no_checksums" "$SQLITE_BIN" "$TEST_DIR/full_sqlite" "-C"
    [ -n "$YAML_BIN" ] && [ -d "$TEST_DIR/full_yaml" ] && run_test "Full_YAML_checksums" "$YAML_BIN" "$TEST_DIR/full_yaml" ""
    [ -n "$YAML_BIN" ] && [ -d "$TEST_DIR/full_yaml" ] && run_test "Full_YAML_no_checksums" "$YAML_BIN" "$TEST_DIR/full_yaml" "-C"
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
