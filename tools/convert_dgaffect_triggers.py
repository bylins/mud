#!/usr/bin/env python3
"""issue.affect-migration: drop the obsolete <spell> argument from dgaffect trigger commands.

Old: dgaffect <target> <property> <spell> <value> <duration> [battlepos]
New: dgaffect <target> <property> <value> <duration> [battlepos]

The <spell> existed only to give the affect a descriptive name; affects now carry their own
description (an APPLY-type affect falls back to the generic "чары"/kWitchery affect), so the spell
argument is removed from do_dg_affect and every trigger that calls it must drop it too.

Byte-safe (world files are KOI8-R). Idempotent: a line whose 3rd token is already a value
(starts with '-', '%', or a digit) is left untouched, so re-running is harmless.

Usage: convert_dgaffect_triggers.py <world_dir> [<world_dir> ...] [--dry-run]
"""
import os, re, sys

# <indent>[*]dgaffect <target> <property>   <spell>   <rest = value duration [bp]>
# The optional leading '*' covers DG-commented-out commands (so they stay valid if re-enabled).
PAT = re.compile(rb'^(\s*\*?dgaffect\s+\S+\s+\S+)\s+(\S+)(\s+\S.*)$')

def is_value(tok: bytes) -> bool:
    # a value/var (new-format arg-3): a number, signed number, or %var% -- never a spell name.
    return tok[:1] in (b'-', b'+', b'%') or tok[:1].isdigit()

def convert_bytes(data: bytes):
    out, changed = [], 0
    for line in data.split(b'\n'):
        m = PAT.match(line)
        if m and not is_value(m.group(2)):
            line = m.group(1) + m.group(3)
            changed += 1
        out.append(line)
    return b'\n'.join(out), changed

def main(argv):
    dry = '--dry-run' in argv
    dirs = [a for a in argv if a != '--dry-run']
    total_files = total_lines = 0
    for root_dir in dirs:
        for root, _, files in os.walk(root_dir):
            for fn in files:
                p = os.path.join(root, fn)
                try:
                    data = open(p, 'rb').read()
                except OSError:
                    continue
                if b'dgaffect' not in data:
                    continue
                new, changed = convert_bytes(data)
                if changed:
                    total_files += 1
                    total_lines += changed
                    if not dry:
                        open(p, 'wb').write(new)
                    print(f"{'[dry] ' if dry else ''}{p}: {changed} dgaffect line(s)")
    print(f"=== {'would convert' if dry else 'converted'} {total_lines} line(s) in {total_files} file(s) ===")

if __name__ == '__main__':
    main(sys.argv[1:])
