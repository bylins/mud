#!/bin/bash
#
# Legacy <-> YAML <-> Legacy round-trip checksum test (5-stage).
#
# Pipeline:
#
#                  convert_to_yaml.py   yaml-build -S -T legacy
#   v1 (legacy) ─────────────────────► v2 (YAML) ──────────────────► v3 (legacy)
#                                                                         │
#                                          convert_to_yaml.py             │
#                       v4 (YAML) ◄──────────────────────────────────────┘
#                            │
#                            ▼  yaml-build -S -T legacy
#                       v5 (legacy)
#
# Checksum probes (legacy-build -W -c):
#   checksum(v1) -- baseline
#   checksum(v3) -- after one legacy-save normalization pass
#   checksum(v5) -- after a second pass
#
# Theory: if the v1 != v3 drift is just OLC normalization (whitespace,
# stripped flag bits, etc. -- `redit_save_to_disk` / `trigedit_save_to_disk`
# rewrites format), then v3 is already in canonical form, so a second
# round-trip is a fixed point: checksum(v3) == checksum(v5). A v3 vs v5
# mismatch instead would indicate a semantic loss across the pipeline.
#
# Pre-requisites:
#   - A legacy world data_dir at <v1_dir> with world/{wld,mob,obj,zon,trg}/
#     index + per-zone files and cfg/configuration.xml.
#   - convert_to_yaml.py deps installed
#     (pip install -r tools/converter/requirements.txt).
#   - YAML build: -Dyaml=builtin or =system.
#   - Legacy build: -Dyaml=disabled -Dsqlite=disabled.
#
# Usage:
#   tools/legacy_yaml_legacy_roundtrip.sh \
#       --v1-dir <path> \
#       [--legacy-bin <path>] \
#       [--yaml-bin <path>] \
#       [--workdir <path>] \
#       [--keep]
#
# Defaults:
#   --legacy-bin = build_legacy/circle
#   --yaml-bin   = build/circle           (must be YAML build)
#   --workdir    = /tmp/legacy-yaml-roundtrip
#
# Exit codes:
#   0  v3 == v5 (round-trip is a fixed point after the first normalization)
#   1  v3 != v5 (round-trip is not idempotent -- real semantic loss)
#   2  pipeline failure
#

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LEGACY_BIN="$REPO_ROOT/build_legacy/circle"
YAML_BIN="$REPO_ROOT/build/circle"
WORKDIR="/tmp/legacy-yaml-roundtrip"
V1_DIR=""
KEEP=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --v1-dir) V1_DIR="$2"; shift 2 ;;
        --legacy-bin) LEGACY_BIN="$2"; shift 2 ;;
        --yaml-bin) YAML_BIN="$2"; shift 2 ;;
        --workdir) WORKDIR="$2"; shift 2 ;;
        --keep) KEEP=1; shift ;;
        -h|--help) sed -n '/^# /,/^$/p' "$0" | head -80; exit 0 ;;
        *) echo "ERROR: unknown arg: $1" >&2; exit 2 ;;
    esac
done

if [[ -z "$V1_DIR" ]]; then
    echo "ERROR: --v1-dir is required (path to a legacy world data dir)." >&2
    exit 2
fi
for f in "$V1_DIR" "$LEGACY_BIN" "$YAML_BIN"; do
    if [[ ! -e "$f" ]]; then
        echo "ERROR: $f not found." >&2
        exit 2
    fi
done
if [[ ! -d "$V1_DIR/world/wld" ]]; then
    echo "ERROR: $V1_DIR is not a legacy data dir (no world/wld/)." >&2
    exit 2
fi
if [[ ! -x "$LEGACY_BIN" || ! -x "$YAML_BIN" ]]; then
    echo "ERROR: binaries must be executable." >&2
    exit 2
fi

V1_ABS="$(cd "$V1_DIR" && pwd)"
LEGACY_BIN_ABS="$(cd "$(dirname "$LEGACY_BIN")" && pwd)/$(basename "$LEGACY_BIN")"
YAML_BIN_ABS="$(cd "$(dirname "$YAML_BIN")" && pwd)/$(basename "$YAML_BIN")"

V2_DIR="$WORKDIR/v2"
V3_DIR="$WORKDIR/v3"
V4_DIR="$WORKDIR/v4"
V5_DIR="$WORKDIR/v5"
CHECKSUM_LOG_V1="$WORKDIR/checksum-v1.log"
CHECKSUM_LOG_V3="$WORKDIR/checksum-v3.log"
CHECKSUM_LOG_V5="$WORKDIR/checksum-v5.log"

mkdir -p "$WORKDIR"
rm -rf "$V2_DIR" "$V3_DIR" "$V4_DIR" "$V5_DIR"
rm -f "$CHECKSUM_LOG_V1" "$CHECKSUM_LOG_V3" "$CHECKSUM_LOG_V5"

# circle writes syslog into its launch cwd (configuration.xml controls the
# filename; -o is wired into a dead LOGNAME global), so we truncate
# <bin_dir>/syslog before the run and then snapshot it to log_path.
# Returns the combined checksum on stdout; non-zero exit on failure.
compute_checksum() {
    local label="$1" bin="$2" data_dir="$3" log_path="$4"
    local bin_dir
    bin_dir="$(dirname "$bin")"
    echo "[$label] $bin -W -c -d $data_dir" >&2
    : > "$bin_dir/syslog"
    (
        cd "$bin_dir"
        "$bin" -W -c -d "$data_dir" 4001 >/dev/null
    )
    cp "$bin_dir/syslog" "$log_path"
    local combined
    combined="$(grep -E ':: Combined: [0-9A-F]+' "$log_path" | tail -1 | awk '{print $5}')"
    if [[ -z "$combined" ]]; then
        echo "ERROR: no 'Combined:' line in $log_path" >&2
        return 1
    fi
    echo "$combined"
}

# Copy non-world artifacts from src to dst so legacy/yaml builds can boot it.
copy_data_dir_skeleton() {
    local src="$1" dst="$2"
    for sub in misc cfg etc text plrs plrobjs plralias plrstuff plrvars stat; do
        if [[ -e "$src/$sub" ]]; then
            cp -r "$src/$sub" "$dst/"
        fi
    done
}

# legacy data_dir -> YAML data_dir via convert_to_yaml.py
convert_legacy_to_yaml() {
    local src_legacy="$1" dst_yaml="$2"
    mkdir -p "$dst_yaml/world"
    python3 "$REPO_ROOT/tools/converter/convert_to_yaml.py" \
        -i "$src_legacy" -o "$dst_yaml" -t all -f yaml >/dev/null
    copy_data_dir_skeleton "$src_legacy" "$dst_yaml"
}

# YAML data_dir -> legacy data_dir via yaml build's -S -T legacy
yaml_resave_to_legacy() {
    local src_yaml="$1" dst_legacy="$2"
    (
        cd "$(dirname "$YAML_BIN_ABS")"
        "$YAML_BIN_ABS" -d "$src_yaml" -S "$dst_legacy" -T legacy 4001 >/dev/null
    )
    if [[ ! -d "$dst_legacy/world/wld" ]]; then
        echo "ERROR: $dst_legacy/world/wld not produced" >&2
        return 1
    fi
    copy_data_dir_skeleton "$src_yaml" "$dst_legacy"
}

echo "== Stage 1/5: checksum(v1) =="
SUM_V1="$(compute_checksum v1 "$LEGACY_BIN_ABS" "$V1_ABS" "$CHECKSUM_LOG_V1")"
echo "checksum(v1) = $SUM_V1"

echo
echo "== Stage 2/5: v1 (legacy) -> v2 (YAML) =="
convert_legacy_to_yaml "$V1_ABS" "$V2_DIR"
echo "v2 at $V2_DIR"

echo
echo "== Stage 3/5: v2 (YAML) -> v3 (legacy) =="
yaml_resave_to_legacy "$V2_DIR" "$V3_DIR"
SUM_V3="$(compute_checksum v3 "$LEGACY_BIN_ABS" "$V3_DIR" "$CHECKSUM_LOG_V3")"
echo "checksum(v3) = $SUM_V3"

echo
echo "== Stage 4/5: v3 (legacy) -> v4 (YAML) =="
convert_legacy_to_yaml "$V3_DIR" "$V4_DIR"
echo "v4 at $V4_DIR"

echo
echo "== Stage 5/5: v4 (YAML) -> v5 (legacy) =="
yaml_resave_to_legacy "$V4_DIR" "$V5_DIR"
SUM_V5="$(compute_checksum v5 "$LEGACY_BIN_ABS" "$V5_DIR" "$CHECKSUM_LOG_V5")"
echo "checksum(v5) = $SUM_V5"

echo
echo "== Round-trip result =="
echo "  v1 = $SUM_V1   (baseline)"
echo "  v3 = $SUM_V3   (after one yaml<->legacy hop)"
echo "  v5 = $SUM_V5   (after two yaml<->legacy hops)"
echo
if [[ "$SUM_V3" == "$SUM_V5" ]]; then
    if [[ "$SUM_V1" == "$SUM_V3" ]]; then
        echo "MATCH (v1 == v3 == v5)  ←  Round-trip is fully byte-stable."
    else
        echo "FIXED-POINT (v1 != v3, v3 == v5)  ←  First pass normalizes, subsequent passes are idempotent (OLC formatting drift only)."
    fi
    if [[ "$KEEP" -eq 0 ]]; then
        rm -rf "$V2_DIR" "$V3_DIR" "$V4_DIR" "$V5_DIR"
    fi
    exit 0
fi

echo "MISMATCH (v3 != v5)  ←  Round-trip is NOT idempotent. Real semantic loss across the pipeline."
echo
echo "Per-section v1 / v3 / v5:"
for key in Zones Rooms Mobs Objects Triggers Combined; do
    v1_line="$(grep -E ":: $key:" "$CHECKSUM_LOG_V1" | tail -1 | awk -F':: ' '{print $2}')"
    v3_line="$(grep -E ":: $key:" "$CHECKSUM_LOG_V3" | tail -1 | awk -F':: ' '{print $2}')"
    v5_line="$(grep -E ":: $key:" "$CHECKSUM_LOG_V5" | tail -1 | awk -F':: ' '{print $2}')"
    printf "  %-9s v1=%s\n            v3=%s\n            v5=%s\n" "$key" "$v1_line" "$v3_line" "$v5_line"
done
exit 1
