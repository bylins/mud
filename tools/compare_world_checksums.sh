#!/bin/bash
# DETAILED world checksum comparison between Legacy and SQLite builds
#
# This script provides field-level difference dumps for objects with mismatched checksums,
# which is more detailed than run_load_tests.sh (which only reports checksum totals).
#
# For basic checksum comparison across all loaders, use: tools/run_load_tests.sh --checksums
# For detailed field-level diff investigation, use this script.
#
# Usage: ./compare_world_checksums.sh [OPTIONS]
#   --rebuild-legacy   Force rebuild of Legacy version
#   --rebuild-sqlite   Force rebuild of SQLite version
#   --rebuild          Force rebuild of both versions
#   --reconvert        Reconvert world from legacy files to SQLite
#   --skip-encoding    Skip encoding check
#   --dump-count N     Number of objects to dump (default: 10)

set -e

MUD_DIR="/home/kvirund/repos/mud"
MUD_DOCS_DIR="/home/kvirund/repos/mud-docs"
BUILD_TEST="$MUD_DIR/build_test"
BUILD_SQLITE="$MUD_DIR/build_sqlite"
WORLD_DIR="${WORLD_DIR:-small}"
PORT=4000
WAIT_TIME="${WAIT_TIME:-15}"
DUMP_DIR="/tmp/world_compare"
DUMP_COUNT=10
REBUILD_LEGACY=0
REBUILD_SQLITE=0
RECONVERT=0
SKIP_ENCODING=0

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --rebuild)
            REBUILD_LEGACY=1
            REBUILD_SQLITE=1
            shift
            ;;
        --rebuild-legacy)
            REBUILD_LEGACY=1
            shift
            ;;
        --rebuild-sqlite)
            REBUILD_SQLITE=1
            shift
            ;;
        --reconvert)
            RECONVERT=1
            shift
            ;;
        --skip-encoding)
            SKIP_ENCODING=1
            shift
            ;;
        --dump-count)
            DUMP_COUNT="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Check source file encoding (detect UTF-8 corruption in KOI8-R files)
check_encoding() {
    echo "=== Checking source file encoding ==="
    ENCODING_ERRORS=0

    # Check for UTF-8 BOM (EF BB BF) in source files
    while IFS= read -r -d '' file; do
        if head -c3 "$file" | LANG=C grep -q $'\xef\xbb\xbf'; then
            echo "ERROR: UTF-8 BOM found: $file"
            ENCODING_ERRORS=1
        fi
    done < <(find "$MUD_DIR/src" -name "*.cpp" -print0 2>/dev/null)

    # Check for common UTF-8 Cyrillic sequences in key source files
    KEY_FILES="$MUD_DIR/src/engine/db/sqlite_world_data_source.cpp"
    for f in $KEY_FILES; do
        if [ -f "$f" ]; then
            # UTF-8 Cyrillic starts with D0 or D1 byte
            if LANG=C grep -q $'\xd0[\x80-\xbf]' "$f" 2>/dev/null; then
                echo "ERROR: UTF-8 Cyrillic detected in: $f"
                echo "  This indicates encoding corruption. Check recent edits."
                ENCODING_ERRORS=1
            fi
        fi
    done

    if [ $ENCODING_ERRORS -eq 0 ]; then
        echo "  Encoding OK"
    else
        echo ""
        echo "FATAL: Encoding errors detected. Fix before continuing."
        exit 99
    fi
    echo ""
}

# Function to compare checksum buffers and show field differences
compare_buffers() {
    local TYPE=$1
    local VNUM=$2
    local LEGACY_BUF="$LEGACY_BASELINE/checksums_buffers/${TYPE}s/${VNUM}.txt"
    local SQLITE_BUF="$BUILD_SQLITE/$WORLD_DIR/checksums_buffers/${TYPE}s/${VNUM}.txt"

    if [ -f "$LEGACY_BUF" ] && [ -f "$SQLITE_BUF" ]; then
        echo "  --- Buffer comparison ---"
        # Extract raw data lines (sed works better with binary than grep)
        LEGACY_RAW=$(LANG=C sed -n '/^---RAW---$/{ n; p; }' "$LEGACY_BUF" 2>/dev/null || echo "")
        SQLITE_RAW=$(LANG=C sed -n '/^---RAW---$/{ n; p; }' "$SQLITE_BUF" 2>/dev/null || echo "")

        if [ "$LEGACY_RAW" != "$SQLITE_RAW" ]; then
            echo "  Legacy: ${LEGACY_RAW:0:80}..."
            echo "  SQLite: ${SQLITE_RAW:0:80}..."
            echo "  --- Field-by-field diff (| separated) ---"
            # Show field-by-field diff using | as separator, using temp files to avoid binary issues
            echo "$LEGACY_RAW" | tr '|' '\n' | nl -ba > /tmp/legacy_fields_$$.txt
            echo "$SQLITE_RAW" | tr '|' '\n' | nl -ba > /tmp/sqlite_fields_$$.txt
            diff /tmp/legacy_fields_$$.txt /tmp/sqlite_fields_$$.txt 2>/dev/null | head -20 | sed 's/^/  /' || true
            rm -f /tmp/legacy_fields_$$.txt /tmp/sqlite_fields_$$.txt
        fi
    fi
}

if [ $SKIP_ENCODING -eq 0 ]; then
    check_encoding
fi

# Create dump directory
mkdir -p "$DUMP_DIR"
rm -f "$DUMP_DIR"/*

echo "========================================"
echo "World Checksum Comparison Tool"
echo "========================================"
echo ""

# Build Legacy if requested or if binary doesn't exist
if [ $REBUILD_LEGACY -eq 1 ] || [ ! -f "$BUILD_TEST/circle" ]; then
    echo "=== Building Legacy version ==="
    cd "$BUILD_TEST"
    make -j$(nproc) circle 2>&1 | tail -3
    echo ""
fi

# Reconvert world if requested
if [ $RECONVERT -eq 1 ]; then
    echo "=== Reconverting world to SQLite ==="
    WORLD_PATH="$BUILD_SQLITE/$WORLD_DIR"
    DB_PATH="$BUILD_SQLITE/$WORLD_DIR/world.db"
    rm -f "$DB_PATH"
    cd "$MUD_DIR"
    python3 tools/convert_to_yaml.py -i "$WORLD_PATH" -o "$WORLD_PATH" -f sqlite 2>&1 | tail -10
    echo ""
fi

# Build SQLite if requested or if binary doesn't exist
if [ $REBUILD_SQLITE -eq 1 ] || [ ! -f "$BUILD_SQLITE/circle" ]; then
    echo "=== Building SQLite version ==="
    cd "$BUILD_SQLITE"
    make -j$(nproc) circle 2>&1 | tail -3
    echo ""
fi

# Kill any running servers
echo "=== Stopping any running servers ==="
pkill -9 -f "circle" 2>/dev/null || true
sleep 3

# Run Legacy server
echo "=== Running Legacy server ==="
cd "$BUILD_TEST"
rm -f "$WORLD_DIR/checksums_detailed.txt"
./circle -d "$WORLD_DIR" &
LEGACY_PID=$!
sleep $WAIT_TIME

# Wait for server to finish and try to kill it
kill $LEGACY_PID 2>/dev/null || true
wait $LEGACY_PID 2>/dev/null || true
sleep 2

# Copy Legacy checksums to temp location (needed when both builds use same world symlink)
LEGACY_BASELINE="/tmp/legacy_baseline_$$"
mkdir -p "$LEGACY_BASELINE"
cp -r "$BUILD_TEST/$WORLD_DIR/checksums_detailed.txt" "$LEGACY_BASELINE/" 2>/dev/null || true
cp -r "$BUILD_TEST/$WORLD_DIR/checksums_buffers" "$LEGACY_BASELINE/" 2>/dev/null || true
echo "Legacy baseline copied to $LEGACY_BASELINE"

# Run SQLite server with baseline for comparison
echo "=== Running SQLite server ==="
cd "$BUILD_SQLITE"
rm -f "$WORLD_DIR/checksums_detailed.txt"
BASELINE_DIR="$LEGACY_BASELINE" ./circle -d "$WORLD_DIR" &
SQLITE_PID=$!
sleep $WAIT_TIME

# Wait for server to finish and try to kill it
kill $SQLITE_PID 2>/dev/null || true
wait $SQLITE_PID 2>/dev/null || true
sleep 2

echo ""
echo "========================================"
echo "Checksum Comparison Results"
echo "========================================"

LEGACY_CHECKSUMS="$LEGACY_BASELINE/checksums_detailed.txt"
SQLITE_CHECKSUMS="$BUILD_SQLITE/$WORLD_DIR/checksums_detailed.txt"

if [ ! -f "$LEGACY_CHECKSUMS" ]; then
    echo "ERROR: Legacy checksum file not found: $LEGACY_CHECKSUMS"
    exit 1
fi

if [ ! -f "$SQLITE_CHECKSUMS" ]; then
    echo "ERROR: SQLite checksum file not found: $SQLITE_CHECKSUMS"
    exit 1
fi

# Sort checksum files for consistent comparison (order may differ due to dynamic room creation)
LEGACY_SORTED="/tmp/legacy_checksums_sorted.txt"
SQLITE_SORTED="/tmp/sqlite_checksums_sorted.txt"
sort "$LEGACY_CHECKSUMS" > "$LEGACY_SORTED"
sort "$SQLITE_CHECKSUMS" > "$SQLITE_SORTED"

# Count differences by type (using sorted files)
ZONE_DIFF=$(diff "$LEGACY_SORTED" "$SQLITE_SORTED" 2>/dev/null | grep "^< ZONE" | wc -l || echo 0)
ROOM_DIFF=$(diff "$LEGACY_SORTED" "$SQLITE_SORTED" 2>/dev/null | grep "^< ROOM" | wc -l || echo 0)
MOB_DIFF=$(diff "$LEGACY_SORTED" "$SQLITE_SORTED" 2>/dev/null | grep "^< MOB" | wc -l || echo 0)
OBJ_DIFF=$(diff "$LEGACY_SORTED" "$SQLITE_SORTED" 2>/dev/null | grep "^< OBJ" | wc -l || echo 0)
TRIG_DIFF=$(diff "$LEGACY_SORTED" "$SQLITE_SORTED" 2>/dev/null | grep "^< TRIG " | wc -l || echo 0)

# Get total counts from syslog
ZONE_TOTAL=$(grep 'Zones:' "$BUILD_TEST/$WORLD_DIR/syslog" | tail -1 | grep -oP '\(\K[0-9]+')
ROOM_TOTAL=$(grep 'Rooms:' "$BUILD_TEST/$WORLD_DIR/syslog" | tail -1 | grep -oP '\(\K[0-9]+')
MOB_TOTAL=$(grep 'Mobs:' "$BUILD_TEST/$WORLD_DIR/syslog" | tail -1 | grep -oP '\(\K[0-9]+')
OBJ_TOTAL=$(grep 'Objects:' "$BUILD_TEST/$WORLD_DIR/syslog" | tail -1 | grep -oP '\(\K[0-9]+')
TRIG_TOTAL=$(grep 'Triggers:' "$BUILD_TEST/$WORLD_DIR/syslog" | tail -1 | grep -oP '\(\K[0-9]+')

# Calculate match percentages
calc_percent() {
    local diff=$1
    local total=$2
    if [ "$total" -gt 0 ]; then
        local match=$((total - diff))
        echo "scale=1; $match * 100 / $total" | bc
    else
        echo "0"
    fi
}

ZONE_PCT=$(calc_percent $ZONE_DIFF $ZONE_TOTAL)
ROOM_PCT=$(calc_percent $ROOM_DIFF $ROOM_TOTAL)
MOB_PCT=$(calc_percent $MOB_DIFF $MOB_TOTAL)
OBJ_PCT=$(calc_percent $OBJ_DIFF $OBJ_TOTAL)
TRIG_PCT=$(calc_percent $TRIG_DIFF $TRIG_TOTAL)

echo ""
echo "Summary:"
printf "  Zones:    %4d / %5d different  (%5.1f%% match)\n" $ZONE_DIFF $ZONE_TOTAL $ZONE_PCT
printf "  Rooms:    %4d / %5d different  (%5.1f%% match)\n" $ROOM_DIFF $ROOM_TOTAL $ROOM_PCT
printf "  Mobs:     %4d / %5d different  (%5.1f%% match)\n" $MOB_DIFF $MOB_TOTAL $MOB_PCT
printf "  Objects:  %4d / %5d different  (%5.1f%% match)\n" $OBJ_DIFF $OBJ_TOTAL $OBJ_PCT
printf "  Triggers: %4d / %5d different  (%5.1f%% match)\n" $TRIG_DIFF $TRIG_TOTAL $TRIG_PCT

# Get overall checksums
echo ""
echo "Overall checksums:"
echo "  Legacy:  $(grep 'Objects:' "$BUILD_TEST/$WORLD_DIR/syslog" | tail -1 | awk '{print $4, $5}')"
echo "  SQLite:  $(grep 'Objects:' "$BUILD_SQLITE/$WORLD_DIR/syslog" | tail -1 | awk '{print $4, $5}')"

# If everything matches, exit
TOTAL_DIFF=$((ZONE_DIFF + ROOM_DIFF + MOB_DIFF + OBJ_DIFF + TRIG_DIFF))
if [ $TOTAL_DIFF -eq 0 ]; then
    echo ""
    echo "SUCCESS: All checksums match!"
    exit 0
fi

# Extract different object vnums and create dumps
if [ $OBJ_DIFF -gt 0 ]; then
    echo ""
    echo "========================================"
    echo "Different Objects (first $DUMP_COUNT)"
    echo "========================================"

    DIFF_OBJECTS=$(diff "$LEGACY_SORTED" "$SQLITE_SORTED" 2>/dev/null | grep "^< OBJ" | sed 's/< OBJ //' | awk '{print $1}' | head -n $DUMP_COUNT)

    for VNUM in $DIFF_OBJECTS; do
        LEGACY_CRC=$(grep "^OBJ $VNUM " "$LEGACY_SORTED" | awk '{print $3}')
        SQLITE_CRC=$(grep "^OBJ $VNUM " "$SQLITE_SORTED" | awk '{print $3}')

        echo ""
        echo "Object $VNUM: Legacy=$LEGACY_CRC SQLite=$SQLITE_CRC"

        # Compare checksum buffers
        compare_buffers "object" "$VNUM"

        # Find object in .obj files
        OBJ_FILE=""
        OBJ_LINE=""
        for f in "$BUILD_TEST/$WORLD_DIR/world/obj/"*.obj; do
            LINE=$(LANG=C grep -n "^#${VNUM}$" "$f" 2>/dev/null | head -1 | cut -d: -f1)
            if [ -n "$LINE" ]; then
                OBJ_FILE="$f"
                OBJ_LINE="$LINE"
                break
            fi
        done

        if [ -n "$OBJ_FILE" ]; then
            echo "  Source: $(basename $OBJ_FILE):$OBJ_LINE"

            # Extract object data from file (approximately 20 lines)
            LANG=C sed -n "${OBJ_LINE},$((OBJ_LINE+19))p" "$OBJ_FILE" > "$DUMP_DIR/obj_${VNUM}_source.txt"

            # Show key lines
            echo "  --- Raw data ---"
            head -15 "$DUMP_DIR/obj_${VNUM}_source.txt" | sed 's/^/  /'
        fi

        # Query SQLite for this object
        echo "  --- SQLite data ---"
        sqlite3 "$BUILD_SQLITE/$WORLD_DIR/world.db" "
            SELECT 'vnum:', vnum FROM objects WHERE vnum = $VNUM;
            SELECT 'type:', obj_type FROM objects WHERE vnum = $VNUM;
            SELECT 'max_in_world:', max_in_world FROM objects WHERE vnum = $VNUM;
        " 2>/dev/null | sed 's/^/  /'

        # Get flags from SQLite
        echo "  Flags in SQLite:"
        sqlite3 "$BUILD_SQLITE/$WORLD_DIR/world.db" "
            SELECT '    ' || flag_category || ': ' || flag_name FROM obj_flags WHERE obj_vnum = $VNUM;
        " 2>/dev/null
    done
fi

# Show different rooms if any
if [ $ROOM_DIFF -gt 0 ]; then
    echo ""
    echo "========================================"
    echo "Different Rooms (first $DUMP_COUNT)"
    echo "========================================"
    DIFF_ROOMS=$(diff "$LEGACY_SORTED" "$SQLITE_SORTED" 2>/dev/null | grep "^< ROOM" | sed 's/< ROOM //' | awk '{print $1}' | head -n $DUMP_COUNT)

    for VNUM in $DIFF_ROOMS; do
        LEGACY_CRC=$(grep "^ROOM $VNUM " "$LEGACY_SORTED" | awk '{print $3}')
        SQLITE_CRC=$(grep "^ROOM $VNUM " "$SQLITE_SORTED" | awk '{print $3}')
        echo ""
        echo "Room $VNUM: Legacy=$LEGACY_CRC SQLite=$SQLITE_CRC"
        compare_buffers "room" "$VNUM"
    done
fi

# Show different triggers if any
if [ $TRIG_DIFF -gt 0 ]; then
    echo ""
    echo "========================================"
    echo "Different Triggers (first $DUMP_COUNT)"
    echo "========================================"
    DIFF_TRIGS=$(diff "$LEGACY_SORTED" "$SQLITE_SORTED" 2>/dev/null | grep "^< TRIG " | sed 's/< TRIG //' | awk '{print $1}' | head -n $DUMP_COUNT)

    for VNUM in $DIFF_TRIGS; do
        LEGACY_CRC=$(grep "^TRIG $VNUM " "$LEGACY_SORTED" | awk '{print $3}')
        SQLITE_CRC=$(grep "^TRIG $VNUM " "$SQLITE_SORTED" | awk '{print $3}')
        echo ""
        echo "Trigger $VNUM: Legacy=$LEGACY_CRC SQLite=$SQLITE_CRC"
        compare_buffers "trigger" "$VNUM"
    done
fi

# Show different zones if any
if [ $ZONE_DIFF -gt 0 ]; then
    echo ""
    echo "========================================"
    echo "Different Zones (first $DUMP_COUNT)"
    echo "========================================"
    DIFF_ZONES=$(diff "$LEGACY_SORTED" "$SQLITE_SORTED" 2>/dev/null | grep "^< ZONE" | sed 's/< ZONE //' | awk '{print $1}' | head -n $DUMP_COUNT)

    for VNUM in $DIFF_ZONES; do
        LEGACY_CRC=$(grep "^ZONE $VNUM " "$LEGACY_SORTED" | awk '{print $3}')
        SQLITE_CRC=$(grep "^ZONE $VNUM " "$SQLITE_SORTED" | awk '{print $3}')
        echo ""
        echo "Zone $VNUM: Legacy=$LEGACY_CRC SQLite=$SQLITE_CRC"
        compare_buffers "zone" "$VNUM"
    done
fi

echo ""
echo "========================================"
echo "Dump files saved to: $DUMP_DIR"
echo "========================================"
ls -la "$DUMP_DIR/" 2>/dev/null || true

exit $TOTAL_DIFF
