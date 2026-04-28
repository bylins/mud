#!/usr/bin/env bash
# Compare two checksums_detailed.txt files (Legacy vs YAML world boots) and
# report only the diffs that are NOT in the known-divergence allowlist.
#
# Usage:
#   tools/compare_load_checksums.sh <legacy.txt> <yaml.txt> [excludes.txt]
#
# Exits 0 if every difference is covered by the excludes file, 1 otherwise.
# Each excluded line we hide is printed under SUPPRESSED so you can audit
# what was filtered out.
#
# Allowlist file format (one rule per line, '#' starts a comment):
#   MOB 11605
#   TRIG 14506
#   MOB_SCRIPTS               # whole-line key, no vnum
#
# This is a *temporary* aid: the entries describe legacy quirks (T-line after
# `$`, unsorted trig_index breaking GetTriggerRnum's binary search, etc).
# Once those data quality issues are fixed in the world, drop entries from
# tools/compare_load_excludes.txt and the comparison should pass cleanly.

set -euo pipefail

if [ $# -lt 2 ] || [ $# -gt 3 ]; then
    echo "Usage: $0 <legacy.txt> <yaml.txt> [excludes.txt]" >&2
    exit 2
fi

LEGACY="$1"
YAML="$2"
EXCLUDES="${3:-$(dirname "$0")/compare_load_excludes.txt}"

[ -r "$LEGACY"   ] || { echo "ERROR: cannot read $LEGACY" >&2;   exit 2; }
[ -r "$YAML"     ] || { echo "ERROR: cannot read $YAML" >&2;     exit 2; }
[ -r "$EXCLUDES" ] || { echo "ERROR: cannot read $EXCLUDES" >&2; exit 2; }

# Build an awk-friendly key set from the excludes file.
# Each rule is the first 1 or 2 whitespace-separated tokens: "MOB 11605",
# "TRIG 14510", "MOB_SCRIPTS". Comments and blank lines are stripped.
KEYS=$(awk '
    {
        sub(/#.*/, "")
        gsub(/^[ \t]+|[ \t]+$/, "")
        if (length($0) == 0) next
        # Print "<TYPE> <VNUM>" or just "<TYPE>" if no vnum follows.
        if (NF == 1) print $1
        else         print $1, $2
    }
' "$EXCLUDES")

REAL_DIFF_FILE=$(mktemp)
SUPPRESSED_FILE=$(mktemp)
trap 'rm -f "$REAL_DIFF_FILE" "$SUPPRESSED_FILE"' EXIT

# diff-driven loop. We classify each `<` / `>` line by its first 1-2 tokens
# (after stripping the leading `<` / `>` marker) and check membership in KEYS.
# diff exits 1 when files differ — that's expected, not an error.
{ diff "$LEGACY" "$YAML" || true; } \
    | awk -v keys="$KEYS" '
        BEGIN {
            n = split(keys, a, "\n")
            for (i = 1; i <= n; ++i) excl[a[i]] = 1
        }
        /^[<>]/ {
            # $1 is "<" or ">", $2 is TYPE, $3 is vnum (optional).
            two = $2 " " $3
            one = $2
            if (one in excl || two in excl) {
                print > "/dev/stderr"
            } else {
                print
            }
        }
    ' >"$REAL_DIFF_FILE" 2>"$SUPPRESSED_FILE"

REAL_COUNT=$(grep -c '^[<>]' "$REAL_DIFF_FILE" 2>/dev/null || true)
SUPPR_COUNT=$(grep -c '^[<>]' "$SUPPRESSED_FILE" 2>/dev/null || true)
REAL_COUNT=${REAL_COUNT:-0}
SUPPR_COUNT=${SUPPR_COUNT:-0}

echo "=== Legacy vs YAML checksum comparison ==="
echo "Suppressed (allowlisted): $SUPPR_COUNT diff line(s)"
echo "Real diffs:               $REAL_COUNT diff line(s)"
echo

if [ "$SUPPR_COUNT" -gt 0 ]; then
    echo "--- SUPPRESSED ---"
    cat "$SUPPRESSED_FILE"
    echo
fi

if [ "$REAL_COUNT" -gt 0 ]; then
    echo "--- REAL DIFFS ---"
    cat "$REAL_DIFF_FILE"
    exit 1
fi

echo "OK: no unexpected differences."
exit 0
