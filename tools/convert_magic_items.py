#!/usr/bin/env python3
# issue.magic-items: convert scroll / wand / staff object prototypes in a YAML world from the legacy
# val[0..3] layout to the extended ObjVal (kSpellItem*) `extra_values` keys.
#
#   scroll (kScroll): val1/val2/val3 -> SPELLITEM_SPELL1_NUM / SPELL2_NUM / SPELL3_NUM
#   wand   (kWand)  : val3 -> SPELL1_NUM ; val1 -> MAX_CHARGES ; val2 -> CUR_CHARGES
#   staff  (kStaff) : same as wand
#
# The maker skill/stat are left ABSENT (the engine then uses the authored-maker potency default, exactly
# like a builder-made potion); a builder can set them per item in OLC. val[] is left in place -- the
# engine reads the keys, and the legacy array stays as a harmless fallback until a later cleanup. This is
# the offline counterpart of the load-time ConvertSpellItemToEValueKey() in src/engine/db/db.cpp.
#
# Idempotent: an object that already has any SPELLITEM_ key is skipped. World files are KOI8-R.
#
# Usage:
#   tools/convert_magic_items.py <world_dir> [--dry-run]
# where <world_dir> holds zones/<n>/objects.yaml (flat YAML world layout).

import argparse
import glob
import os
import re
import sys

ENC = "koi8-r"
TYPE_RE = re.compile(r"^  type:\s*(kScroll|kWand|kStaff)\b", re.M)
VALUES_RE = re.compile(r"^  values:\n((?:    - -?\d+\n)+)", re.M)
EXTRA_RE = re.compile(r"^  extra_values:\n((?:    \S.*\n)+)", re.M)


def build_keys(vals, typ):
    """Return {KEY: value} for the migrated payload (only positive values are stored)."""
    kv = {}

    def pos(key, v):
        if v > 0:
            kv[key] = v

    if typ == "kScroll":
        pos("SPELL1_NUM", vals[1])
        pos("SPELL2_NUM", vals[2])
        pos("SPELL3_NUM", vals[3])
    else:  # kWand / kStaff
        pos("SPELL1_NUM", vals[3])
        pos("MAX_CHARGES", vals[1])
        pos("CUR_CHARGES", vals[2])
    return kv


def convert_block(block):
    """Convert one object block (text). Return (new_block, changed)."""
    tm = TYPE_RE.search(block)
    if not tm:
        return block, False
    typ = tm.group(1)
    if "SPELL1_NUM" in block or "SPELLITEM_" in block:  # already migrated (canonical or legacy)
        return block, False
    vm = VALUES_RE.search(block)
    if not vm:
        return block, False
    vals = [int(x) for x in re.findall(r"    - (-?\d+)", vm.group(1))]
    while len(vals) < 4:
        vals.append(-1)
    kv = build_keys(vals, typ)
    if not kv:
        return block, False

    em = EXTRA_RE.search(block)
    if em:
        # merge into the existing extra_values block, keep entries sorted
        existing = dict(re.findall(r"    (\S+):\s*(-?\d+)", em.group(1)))
        existing.update({k: str(v) for k, v in kv.items()})
        lines = "".join(f"    {k}: {existing[k]}\n" for k in sorted(existing))
        new_block = block[: em.start()] + "  extra_values:\n" + lines + block[em.end():]
    else:
        # insert a fresh extra_values block right after the values: list
        lines = "".join(f"    {k}: {kv[k]}\n" for k in sorted(kv))
        insert = "  extra_values:\n" + lines
        new_block = block[: vm.end()] + insert + block[vm.end():]
    return new_block, True


def split_objects(text):
    """Split a flat objects.yaml into (prefix, [blocks]) on '# Object #'/'# Obj #' headers."""
    parts = re.split(r"(?m)(?=^# Obj(?:ect)? #\d+)", text)
    return parts


def convert_file(path, dry_run):
    text = open(path, encoding=ENC).read()
    parts = split_objects(text)
    changed = 0
    out = []
    for part in parts:
        nb, ch = convert_block(part)
        if ch:
            changed += 1
        out.append(nb)
    if changed and not dry_run:
        open(path, "w", encoding=ENC).write("".join(out))
    return changed


def main():
    ap = argparse.ArgumentParser(description="Migrate scroll/wand/staff protos to the unified magic-item extra_values keys.")
    ap.add_argument("world_dir", help="world directory containing zones/<n>/objects.yaml")
    ap.add_argument("--dry-run", action="store_true", help="report only, do not write")
    args = ap.parse_args()

    files = sorted(glob.glob(os.path.join(args.world_dir, "zones", "*", "objects.yaml")))
    if not files:
        print(f"no zones/*/objects.yaml under {args.world_dir}", file=sys.stderr)
        return 1

    total = 0
    for f in files:
        n = convert_file(f, args.dry_run)
        if n:
            total += n
            print(f"{'(dry) ' if args.dry_run else ''}{f}: {n} scroll/wand/staff converted")
    print(f"{'(dry) ' if args.dry_run else ''}TOTAL: {total} objects migrated across {len(files)} files")
    return 0


if __name__ == "__main__":
    sys.exit(main())
