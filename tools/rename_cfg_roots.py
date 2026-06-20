#!/usr/bin/env python3
"""
issue.cfg-loader-keys: bring a cfg/ tree in line with the new invariant
"CfgManager key == file name == XML root element".

After issue.cfg-loader-keys the registry uses one token per loader (the key equals
the file name without .xml equals the expected XML root). Older cfg files carry the
previous root names; this script renames the root element of exactly the affected
CfgManager files so the boot log stays clean. Skipping it is non-fatal -- the loader
still loads a file with a "wrong" root, it just logs one warning per file.

IMPORTANT: this is an EXPLICIT, closed list of renames (only CfgManager-registered
files whose root changed). It deliberately does NOT touch other cfg XML such as the
per-class cfg/classes/pc_*.xml (root <class>), cfg/craft/ingredient_magic/*.xml, or
plrstuff sets_drop.xml -- those are read by their own loaders that rely on their
current root and must NOT be renamed.

Encoding-safe: only the ASCII root tag bytes are touched; KOI8-R payloads preserved.

Usage:
    python3 tools/rename_cfg_roots.py <cfg_dir>             # DRY RUN (default)
    python3 tools/rename_cfg_roots.py <cfg_dir> --apply     # write the .xml roots
    python3 tools/rename_cfg_roots.py <cfg_dir> --apply --schemes   # also Vedun .scheme
"""
import re
import sys
import pathlib

# 23 message files: root msg_container -> <leaf>; data in messages/ru, scheme in messages/
MSG = ["currency_msg", "class_msg", "skill_msg", "spell_msg", "hit_msg", "feat_msg",
       "guild_msg", "special_msg", "bank_msg", "mail_msg", "horse_msg", "torc_msg",
       "mercenary_msg", "exchange_msg", "rent_msg", "shop_msg", "board_msg",
       "affect_msg", "common_msg", "cities_msg", "region_msg", "pc_race_msg",
       "rune_stone_msg"]

# (relative xml path under cfg/, old root, new root)
XML_RENAMES = [(f"messages/ru/{leaf}.xml", "msg_container", leaf) for leaf in MSG] + [
    ("classes/pc_classes.xml",      "classes",             "pc_classes"),
    ("economics/shops.xml",         "shop_list",           "shops"),
    ("mechanics/stable_objs.xml",   "equipment_positions", "stable_objs"),
    ("mechanics/global_drop.xml",   "globaldrop",          "global_drop"),
    ("mechanics/noob.xml",          "noob_help",           "noob"),
    ("mechanics/guards.xml",        "guardians",           "guards"),
    ("messages/ru/social_msg.xml",  "socials",             "social_msg"),
]

# Vedun scheme sidecars whose root <tag name=> changed (only those that exist as schemes)
SCHEME_RENAMES = [(f"messages/{leaf}.scheme", "msg_container", leaf) for leaf in MSG] + [
    ("economics/shops.scheme",      "shop_list",  "shops"),
    ("messages/social_msg.scheme",  "socials",    "social_msg"),
]


def root_element(data):
    i = 0
    while i < len(data):
        lt = data.find(b"<", i)
        if lt < 0:
            return None
        if data[lt:lt+2] == b"<?":
            i = data.find(b">", lt) + 1
        elif data[lt:lt+4] == b"<!--":
            i = data.find(b"-->", lt) + 3
        elif data[lt:lt+2] == b"<!":
            i = data.find(b">", lt) + 1
        else:
            m = re.match(rb"<([A-Za-z_][\w.-]*)", data[lt:])
            return m.group(1).decode("ascii") if m else None
    return None


def main(argv):
    args = [a for a in argv[1:] if not a.startswith("-")]
    apply = "--apply" in argv[1:]
    schemes = "--schemes" in argv[1:]
    if not args:
        print(__doc__)
        return 2
    base = pathlib.Path(args[0])
    if not base.is_dir():
        print(f"not a directory: {base}", file=sys.stderr)
        return 1

    changed = skipped = 0
    for rel, old, new in XML_RENAMES:
        p = base / rel
        if not p.exists():
            continue
        raw = p.read_bytes()
        cur = root_element(raw)
        if cur == new:
            continue                      # already done
        if cur != old:
            print(f"  ?? {rel}: root <{cur}> (expected <{old}>) -- left as is")
            skipped += 1
            continue
        raw2 = re.sub(rb"<" + re.escape(old.encode()) + rb"(?=[ \t\r\n>/])",
                      b"<" + new.encode(), raw)
        raw2 = raw2.replace(b"</" + old.encode() + b">", b"</" + new.encode() + b">")
        print(f"  {rel}: <{old}> -> <{new}>{'' if apply else '  [dry-run]'}")
        if apply:
            p.write_bytes(raw2)
        changed += 1

    if schemes:
        for rel, old, new in SCHEME_RENAMES:
            p = base / rel
            if not p.exists():
                continue
            raw = p.read_bytes()
            if b'name="' + old.encode() + b'"' not in raw:
                continue
            raw = raw.replace(b'name="' + old.encode() + b'"', b'name="' + new.encode() + b'"')
            raw = raw.replace(b"<" + old.encode() + b">", b"<" + new.encode() + b">")  # comment
            print(f"  {rel} (scheme): \"{old}\" -> \"{new}\"{'' if apply else '  [dry-run]'}")
            if apply:
                p.write_bytes(raw)
            changed += 1

    print(f"---\n{changed} file(s) {'updated' if apply else 'to change (pass --apply)'}"
          + (f", {skipped} unexpected-root skipped" if skipped else ""))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
