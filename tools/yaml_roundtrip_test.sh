#!/bin/bash
#
# YAML world round-trip diagnostic.
#
# Loads the YAML world from <data_dir>/world, re-emits it via the C++
# Koi8rYamlEmitter into <data_dir>/world_v2, then runs a structural diff
# (tools/yaml_world_diff.py) between v1 and v2. Used to catch save -> reload
# inconsistencies in YamlWorldDataSource::Save* methods.
#
# Usage:
#   ./tools/yaml_roundtrip_test.sh [<data_dir>] [<circle_binary>]
#
# Defaults:
#   data_dir = build_yaml/full
#   circle_binary = build_yaml_meson/circle  (must be built with -Dyaml=...)
#
# Pre-requisites:
#   - <data_dir>/world contains a valid YAML world (world_config.yaml,
#     dictionaries/, zones/index.yaml + per-zone trees).
#   - <data_dir>/cfg/configuration.xml exists (circle needs it on boot).
#
# Exit codes:
#   0  resave succeeded and v1 ~= v2 (no structural diffs);
#   1  parse errors or structural diffs (real findings);
#   2  resave itself failed (circle exit code != 0).
#

set -euo pipefail

DATA_DIR=${1:-build_yaml/full}
CIRCLE_BIN=${2:-build_yaml_meson/circle}
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

if [[ ! -d "$DATA_DIR/world" ]]; then
    echo "ERROR: $DATA_DIR/world not found." >&2
    exit 2
fi
if [[ ! -x "$CIRCLE_BIN" ]]; then
    echo "ERROR: $CIRCLE_BIN not found or not executable." >&2
    exit 2
fi

# Resolve to absolute paths before any cd.
CIRCLE_BIN_ABS="$(cd "$(dirname "$CIRCLE_BIN")" && pwd)/$(basename "$CIRCLE_BIN")"
DATA_DIR_ABS="$(cd "$DATA_DIR" && pwd)"

echo "== Round-trip: load $DATA_DIR_ABS/world -> save world_v2 =="
rm -rf "$DATA_DIR_ABS/world_v2"

# circle does chdir(data_dir); target path is interpreted relative to that.
(
    cd "$DATA_DIR_ABS"
    "$CIRCLE_BIN_ABS" -d . -S world_v2 4001
)
rc=$?
if [[ $rc -ne 0 ]]; then
    echo "ERROR: circle -S exited with code $rc" >&2
    exit 2
fi

echo
echo "== Diff: $DATA_DIR_ABS/world vs $DATA_DIR_ABS/world_v2 =="
python3 "$REPO_ROOT/tools/yaml_world_diff.py" \
    "$DATA_DIR_ABS/world" "$DATA_DIR_ABS/world_v2"
