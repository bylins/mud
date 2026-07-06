#!/bin/bash
#
# PoC: demonstrate and measure the SQLite world-cache speedup.
#
# Boots the SAME world with the composite (yaml+sqlite) binary through 6
# stages and prints boot times + a summary table:
#   0. YAML-ONLY        - plain YAML binary, no cache in the picture at all
#                          (the "no cache" reference point)
#   1. COLD              - no world.db yet: engine reads YAML, writes
#                           world.db for every zone (full resync)
#   2. WARM               - world.db is fresher than the YAML files for
#                           every zone: engine reads SQLite only
#   3. STALE              - every zone's YAML was touched (edited) after
#                           the SQLite write: engine falls back to YAML for
#                           the whole world and re-resyncs all of world.db
#                           (worst case -- everything invalidated at once)
#   4. WARM again          - re-confirms the post-resync state is back on
#                           the fast SQLite path
#   5. SINGLE-ZONE STALE   - only ONE zone's YAML was touched: the scenario
#                           that actually matters in production (a builder
#                           edits one zone live). Engine should re-read only
#                           that zone from YAML and resync only it in
#                           world.db -- boot time here should sit close to
#                           WARM, not close to full STALE; the summary
#                           prints the single-zone/full-stale time ratio as
#                           a sanity check (should be well under 1.0).
#
# PREREQUISITES:
#   - build_yaml/circle and build_multi/circle already built, e.g. via:
#       ./tools/run_load_tests.sh --world=small          (small world)
#       FULL_WORLD_DIR=... ./tools/run_load_tests.sh --world=full  (full world)
#     (build_multi = -Dyaml=builtin -Dsqlite=auto)
#
# USAGE:
#   ./tools/poc_sqlite_cache_bench.sh <world_dir_under_build_yaml>
#   e.g.
#   ./tools/poc_sqlite_cache_bench.sh small
#   ./tools/poc_sqlite_cache_bench.sh full
#   ./tools/poc_sqlite_cache_bench.sh full_perfile   # per-file YAML layout
#                                                     # (same stages, same
#                                                     # comparison, just a
#                                                     # different on-disk
#                                                     # world_config.yaml
#                                                     # "layout: per_file")
#
# ENV VARS (optional):
#   POC_BASE_PORT=<port>   - first of 6 consecutive ports used, one per
#                            stage (default 4010). Override on a shared/prod
#                            box where the default range might collide with
#                            something already running.
#   BOOT_TIMEOUT=<seconds> - per-stage boot wait timeout (default 900). Bump
#                            this for a large/slow world or a loaded host --
#                            a stage that never reaches "Boot db -- DONE" in
#                            time is reported as "X TIMEOUT booting" and its
#                            summary row is left blank ("?s").
#
# The named world (already set up under build_yaml/<name>) is copied fresh
# into build_multi/poc_<name> for this benchmark; build_yaml/<name> itself
# is only read, never modified. Each stage's full server output lands in
# build_multi/poc_<name>/stdout_poc_<label>.log (label = yaml_only/cold/
# warm/stale/warm2/single_zone_stale) for post-mortem if a stage times out
# or a checksum looks wrong.
#

set -u
MUD_DIR="$(cd "$(dirname "$0")/.." && pwd)"
WORLD_NAME="${1:-small}"
# Override with POC_BASE_PORT=<port> when the default 4010-4015 range might
# collide with something already running on the host (e.g. a shared/prod box).
BASE_PORT="${POC_BASE_PORT:-4010}"

YAML_BIN="$MUD_DIR/build_yaml/circle"
MULTI_BIN="$MUD_DIR/build_multi/circle"
SRC_WORLD="$MUD_DIR/build_yaml/$WORLD_NAME"
BENCH_DIR="$MUD_DIR/build_multi/poc_$WORLD_NAME"

if [ ! -x "$YAML_BIN" ] || [ ! -x "$MULTI_BIN" ]; then
    echo "X ERROR: build_yaml/circle and build_multi/circle must exist first."
    echo "  Run e.g.: ./tools/run_load_tests.sh --world=$WORLD_NAME"
    exit 1
fi
if [ ! -d "$SRC_WORLD/world" ]; then
    echo "X ERROR: $SRC_WORLD/world not found (set up the world via run_load_tests.sh first)."
    exit 1
fi

echo "=============================================="
echo "SQLite world-cache PoC benchmark: world='$WORLD_NAME'"
echo "=============================================="
echo ""

# Enable the composite (sqlite + yaml) loader in a config file. sqlite listed
# first = highest priority, so it wins freshness ties once resynced.
enable_composite_loader() {
    local cfg="$1"
    if grep -q "<world_loader>" "$cfg"; then
        return 0
    fi
    sed -i '/<\/configuration>/i\
\t<world_loader>\
\t\t<sources>\
\t\t\t<sqlite/>\
\t\t\t<yaml/>\
\t\t</sources>\
\t</world_loader>' "$cfg"
}

# Boot once, wait for "Boot db -- DONE", print timing + which source served
# the read, then stop the server. Args: label, binary, dir, port.
boot_once() {
    local label="$1" binary="$2" dir="$3" port="$4"

    rm -rf "$dir/syslog" 2>/dev/null || true
    fuser -k "${port}/tcp" 2>/dev/null || true
    sleep 0.3

    (cd "$dir" && exec "$binary" -d . "$port" > "stdout_poc_${label}.log" 2>&1) &

    # $BOOT_TIMEOUT_ITERS = seconds / 0.5 -- override BOOT_TIMEOUT for a large
    # world (the full world's cold-boot resync writes every zone to a fresh
    # SQLite db and can run well past a plain boot's time).
    local waited=0
    local max_iters=$((${BOOT_TIMEOUT:-900} * 2))
    while [ $waited -lt $max_iters ]; do
        LANG=C grep -qa "Boot db -- DONE" "$dir/syslog" 2>/dev/null && break
        sleep 0.5
        waited=$((waited + 1))
    done

    fuser -k "${port}/tcp" 2>/dev/null || true
    sleep 0.5

    if ! LANG=C grep -qa "Boot db -- DONE" "$dir/syslog" 2>/dev/null; then
        echo "  [$label] X TIMEOUT booting"
        return 1
    fi

    local begin done_t
    begin=$(LANG=C grep -Ea "Boot db -- BEGIN\.?" "$dir/syslog" | head -1 | cut -d' ' -f2)
    done_t=$(LANG=C grep -Ea "Boot db -- DONE\.?" "$dir/syslog" | head -1 | cut -d' ' -f2)
    local begin_sec done_sec duration
    begin_sec=$(echo "$begin" | awk -F: '{printf "%.3f", ($1*3600)+($2*60)+$3}')
    done_sec=$(echo "$done_t" | awk -F: '{printf "%.3f", ($1*3600)+($2*60)+$3}')
    duration=$(echo "$done_sec - $begin_sec" | bc)

    local source_line
    source_line=$(LANG=C grep -a "Using data source:" "$dir/syslog" | head -1 | sed 's/.*Using data source: //')
    local read_line
    read_line=$(LANG=C grep -a "World read source:" "$dir/syslog" | head -1 | sed 's/.*World read source/World read source/')
    # Per-zone breakdown (composite_world_data_source.cpp's roll-up log) --
    # confirms per-zone selection actually ran, not a whole-world fallback
    # that happens to produce the same checksums by coincidence.
    local zone_lines
    zone_lines=$(LANG=C grep -a "World source '.*': won" "$dir/syslog")

    echo "  [$label] boot time: ${duration}s"
    [ -n "$source_line" ] && echo "    data source:  $source_line"
    [ -n "$read_line" ]   && echo "    $read_line"
    if [ -n "$zone_lines" ]; then
        echo "$zone_lines" | sed 's/^/    /'
    fi

    printf '%s\n' "$duration" > "/tmp/poc_${WORLD_NAME}_${label}_time.txt"
}

# --- Reference: plain YAML boot, no cache in the picture at all ---
echo "--- Reference: YAML-only (no cache) ---"
boot_once "yaml_only" "$YAML_BIN" "$SRC_WORLD" "$BASE_PORT"
echo ""

# --- Prepare a fresh working copy for the composite-loader runs ---
rm -rf "$BENCH_DIR"
cp -r "$SRC_WORLD" "$BENCH_DIR"
rm -f "$BENCH_DIR/world.db"
enable_composite_loader "$BENCH_DIR/cfg/configuration.xml"

# --- Stage 1: COLD (no world.db yet -> reads YAML, writes SQLite) ---
echo "--- Stage 1: COLD (world.db absent -> YAML load + SQLite write) ---"
boot_once "cold" "$MULTI_BIN" "$BENCH_DIR" "$((BASE_PORT + 1))"
echo ""

if [ ! -f "$BENCH_DIR/world.db" ]; then
    echo "X ERROR: world.db was not created by the cold boot -- aborting."
    exit 1
fi

# --- Stage 2: WARM (world.db is now fresher than the YAML files) ---
echo "--- Stage 2: WARM (world.db present + fresh -> SQLite load) ---"
boot_once "warm" "$MULTI_BIN" "$BENCH_DIR" "$((BASE_PORT + 2))"
echo ""

# --- Stage 3: STALE (a YAML file was edited after the SQLite write) ---
# Touch zone.yaml, not rooms.yaml -- rooms.yaml only exists in flat layout
# (per-file layout has rooms/<n>.yaml + rooms/index.yaml instead). Freshness
# is "newest mtime under the zone directory" (see GetZoneFreshness), so
# zone.yaml -- present under both layouts -- staled the zone either way.
echo "--- Stage 3: STALE (a YAML zone file touched -> falls back to YAML, re-resyncs) ---"
touch "$BENCH_DIR/world/zones/"*/zone.yaml 2>/dev/null | head -1
# Sleep isn't needed: freshness is compared as mtime, and touch always moves
# it strictly forward from whatever the prior boot's resync stamped.
boot_once "stale" "$MULTI_BIN" "$BENCH_DIR" "$((BASE_PORT + 3))"
echo ""

# --- Stage 4: WARM again (re-synced after the stale reload) ---
echo "--- Stage 4: WARM again (post-resync, back to fast path) ---"
boot_once "warm2" "$MULTI_BIN" "$BENCH_DIR" "$((BASE_PORT + 4))"
echo ""

# --- Stage 5: SINGLE-ZONE STALE (only ONE zone's rooms.yaml touched) ---
# This is the scenario that actually matters in production: the server runs
# for days, a builder edits one zone live via OLC/admin-api, and the cache
# must NOT fall back to reloading the whole world -- only that one zone
# should come from YAML, every other zone should still hit SQLite. Boot time
# here should be close to WARM, not close to the full-STALE case above.
echo "--- Stage 5: SINGLE-ZONE STALE (one zone's zone.yaml touched -> only that zone re-read from YAML) ---"
first_zone_dir=$(find "$BENCH_DIR/world/zones" -mindepth 1 -maxdepth 1 -type d | sort | head -1)
if [ -n "$first_zone_dir" ] && [ -f "$first_zone_dir/zone.yaml" ]; then
    touch "$first_zone_dir/zone.yaml"
    boot_once "single_zone_stale" "$MULTI_BIN" "$BENCH_DIR" "$((BASE_PORT + 5))"
else
    echo "  X SKIPPED: could not find a zone directory with zone.yaml under $BENCH_DIR/world/zones"
fi
echo ""

echo "=============================================="
echo "SUMMARY"
echo "=============================================="
read_time() { cat "/tmp/poc_${WORLD_NAME}_$1_time.txt" 2>/dev/null || echo "?"; }
t_yaml=$(read_time yaml_only)
t_cold=$(read_time cold)
t_warm=$(read_time warm)
t_stale=$(read_time stale)
t_warm2=$(read_time warm2)
t_single=$(read_time single_zone_stale)

printf "  %-28s %8ss\n" "YAML-only (no cache)"        "$t_yaml"
printf "  %-28s %8ss\n" "Composite, COLD (1st boot)"  "$t_cold"
printf "  %-28s %8ss\n" "Composite, WARM (cached)"    "$t_warm"
printf "  %-28s %8ss\n" "Composite, STALE (all zones edited)" "$t_stale"
printf "  %-28s %8ss\n" "Composite, WARM again"        "$t_warm2"
printf "  %-28s %8ss\n" "Composite, SINGLE-ZONE STALE" "$t_single"
echo ""
if [ "$t_yaml" != "?" ] && [ "$t_warm" != "?" ] && [ "$(echo "$t_warm > 0" | bc)" = "1" ]; then
    speedup=$(echo "scale=1; $t_yaml / $t_warm" | bc)
    echo "  Speedup (YAML-only / warm SQLite cache): ${speedup}x"
fi
if [ "$t_single" != "?" ] && [ "$t_stale" != "?" ] && [ "$(echo "$t_stale > 0" | bc)" = "1" ]; then
    ratio=$(echo "scale=2; $t_single / $t_stale" | bc)
    echo "  Single-zone-stale / full-stale time ratio: ${ratio} (should be well under 1.0 -- if"
    echo "  it's close to 1.0, per-zone selection isn't actually saving anything)"
fi
echo "=============================================="
