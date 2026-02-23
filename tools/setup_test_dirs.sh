#!/bin/bash
#
# Setup test directories for world loading performance tests
# Run from repository root: ./tools/setup_test_dirs.sh
#

set -e

MUD_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$MUD_DIR"

echo "=== Setting up test directories ==="
echo "MUD_DIR: $MUD_DIR"
echo ""

# Create test directories
TEST_DIR="$MUD_DIR/test"
mkdir -p "$TEST_DIR"

# Small world source
SMALL_WORLD_SRC="$MUD_DIR/lib.template"

# Full world source
FULL_WORLD_SRC="/home/kvirund/repos/full.world/lib"

# Check sources exist
if [ ! -d "$SMALL_WORLD_SRC/world" ]; then
    echo "ERROR: Small world source not found: $SMALL_WORLD_SRC/world"
    exit 1
fi

if [ ! -d "$FULL_WORLD_SRC/world" ]; then
    echo "ERROR: Full world source not found: $FULL_WORLD_SRC/world"
    exit 1
fi

# Function to setup a test directory with symlinks
setup_test_dir() {
    local name="$1"
    local world_src="$2"
    local use_sqlite="$3"

    local dir="$TEST_DIR/$name"

    echo "Setting up: $name"
    rm -rf "$dir"
    mkdir -p "$dir/log"

    # Symlink common directories from lib
    ln -sf "$MUD_DIR/lib/cfg" "$dir/cfg"
    ln -sf "$MUD_DIR/lib/etc" "$dir/etc"
    ln -sf "$MUD_DIR/lib/misc" "$dir/misc"
    ln -sf "$MUD_DIR/lib/stat" "$dir/stat"
    ln -sf "$MUD_DIR/lib/text" "$dir/text"
    ln -sf "$MUD_DIR/lib/plralias" "$dir/plralias"
    ln -sf "$MUD_DIR/lib/plrobjs" "$dir/plrobjs"
    ln -sf "$MUD_DIR/lib/plrs" "$dir/plrs"
    ln -sf "$MUD_DIR/lib/plrstuff" "$dir/plrstuff"
    ln -sf "$MUD_DIR/lib/plrvars" "$dir/plrvars"

    # Self-symlink for lib directory (server expects to chdir to lib)
    ln -sf . "$dir/lib"

    if [ "$use_sqlite" = "sqlite" ]; then
        # Convert to SQLite
        echo "  Converting to SQLite..."
        python3 "$MUD_DIR/tools/convert_to_yaml.py" \
            --input "$world_src" \
            --output "$dir" \
            --format sqlite \
            --db "$dir/world.db" \
            2>&1 | tail -5
    else
        # Symlink world directory for legacy
        ln -sf "$world_src/world" "$dir/world"
    fi

    echo "  Done: $dir"
}

# Setup all test directories
echo ""
echo "=== Creating test directories ==="
setup_test_dir "small_legacy" "$SMALL_WORLD_SRC" "legacy"
setup_test_dir "small_sqlite" "$SMALL_WORLD_SRC" "sqlite"
setup_test_dir "full_legacy" "$FULL_WORLD_SRC" "legacy"
setup_test_dir "full_sqlite" "$FULL_WORLD_SRC" "sqlite"

echo ""
echo "=== Test directories ready ==="
ls -la "$TEST_DIR"

echo ""
echo "=== Database sizes ==="
ls -lh "$TEST_DIR"/*/world.db 2>/dev/null || echo "No SQLite databases yet"

echo ""
echo "Done. Now rebuild binaries:"
echo ""
echo "  # Legacy build (no SQLite)"
echo "  cd build_test && make circle -j\$(nproc)"
echo ""
echo "  # SQLite build"
echo "  cd build_sqlite && make circle -j\$(nproc)"
echo ""
echo "Then run tests:"
echo "  ./tools/run_load_tests.sh"
