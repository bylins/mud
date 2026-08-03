#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Audit byte-vs-char assumptions ahead of the KOI8-R -> UTF-8 migration (issue #3681).

The engine currently assumes "1 byte == 1 character" in three shapes: length/size used as a
character count, fixed byte-offset access/truncation, and per-byte case/classification. Under
UTF-8 every one of those breaks on multibyte (Cyrillic) text. This script scans the C++ sources,
classifies the suspect call sites into categories, and *prioritises files that actually contain
non-ASCII (Russian) bytes* -- those are where a byte-vs-char bug is observable.

It is a triage aid, not a linter: every hit needs a human to decide whether it means bytes
(fine) or characters (needs utf8::). Source files are KOI8-R today, so snippets are decoded from
KOI8-R for readable output.

Usage:
    tools/audit_utf8_migration.py [PATHS ...]        # default: src
    tools/audit_utf8_migration.py --full             # list hits in ASCII-only files too
    tools/audit_utf8_migration.py --category substr  # only one category (repeatable)
    tools/audit_utf8_migration.py --list-categories
"""

import argparse
import os
import re
import sys

# Each category: (regex over a latin-1-decoded line, one-line description).
# The regexes are deliberately broad -- false positives are cheap, missed sites are not.
CATEGORIES = {
    "printf-width": (
        re.compile(r"%[-+ 0#]*(?:\d+|\*)(?:\.(?:\d+|\*))?s|%\.(?:\d+|\*)s"),
        "printf width/precision on a string (%-20s, %.*s) -- pads/truncates by bytes",
    ),
    "substr": (
        re.compile(r"\.substr\s*\("),
        "substr() -- byte offsets, can cut a code point in half",
    ),
    "rel-index": (
        re.compile(r"\[[^\]]*-\s*[123]\s*\]"),
        "indexing relative to length (str[len-1]) -- last byte, not last character",
    ),
    "strchr-cyr": (
        re.compile(r'strchr\s*\(\s*"([^"]*)"'),
        "strchr() over a literal that contains Cyrillic -- matches a byte, not a letter",
    ),
    "case-deref": (
        re.compile(r"\b(?:UPPER|LOWER)\s*\(\s*(?:\*|\w+\s*\[)"),
        "UPPER/LOWER on a dereferenced/indexed byte -- mangles a UTF-8 lead byte",
    ),
    "fixed-copy": (
        re.compile(r"\b(?:strn?cpy|strncat)\s*\("),
        "strcpy/strncpy/strncat into a fixed buffer -- byte length may split a character",
    ),
    "strlen": (
        re.compile(r"\bstrlen\s*\("),
        "strlen() -- byte count; suspect only when used as a character/display count",
    ),
    "size-length": (
        re.compile(r"\.(?:size|length)\s*\(\s*\)"),
        "std::string size()/length() -- byte count used as a character count",
    ),
}

# High-volume categories: reported in the summary, but only listed for Cyrillic-bearing files
# unless --full, to keep the output actionable.
HIGH_VOLUME = {"strlen", "size-length"}

SOURCE_EXTENSIONS = (".cpp", ".h", ".hpp", ".cc", ".cxx")


def has_cyrillic(raw: bytes) -> bool:
    """True if the file carries any high-bit byte (KOI8-R Russian text lives at >= 0x80)."""
    return any(b >= 0x80 for b in raw)


def literal_has_high_byte(fragment: str) -> bool:
    return any(ord(c) >= 0x80 for c in fragment)


def decode_snippet(line: str) -> str:
    """`line` is latin-1 (bytes 1:1); re-render it from KOI8-R so Russian reads correctly."""
    return line.encode("latin-1", "replace").decode("koi8-r", "replace").rstrip()


def iter_source_files(paths):
    for path in paths:
        if os.path.isfile(path):
            yield path
            continue
        for root, _dirs, files in os.walk(path):
            if "third_party_libs" in root:
                continue
            for name in files:
                if name.endswith(SOURCE_EXTENSIONS):
                    yield os.path.join(root, name)


def scan_file(path, wanted):
    with open(path, "rb") as handle:
        raw = handle.read()
    cyrillic = has_cyrillic(raw)
    text = raw.decode("latin-1")
    hits = []  # (category, lineno, snippet)
    for lineno, line in enumerate(text.splitlines(), 1):
        for category, (pattern, _desc) in CATEGORIES.items():
            if category not in wanted:
                continue
            match = pattern.search(line)
            if not match:
                continue
            if category == "strchr-cyr" and not literal_has_high_byte(match.group(1)):
                continue
            hits.append((category, lineno, decode_snippet(line)))
    return cyrillic, hits


def main(argv=None):
    parser = argparse.ArgumentParser(description="Audit byte-vs-char sites for the UTF-8 migration.")
    parser.add_argument("paths", nargs="*", default=["src"], help="files or directories (default: src)")
    parser.add_argument("--category", action="append", dest="categories",
                        help="restrict to a category (repeatable); default: all")
    parser.add_argument("--full", action="store_true",
                        help="also list hits in ASCII-only files and high-volume categories")
    parser.add_argument("--list-categories", action="store_true", help="print category names and exit")
    args = parser.parse_args(argv)

    if args.list_categories:
        for name, (_re, desc) in CATEGORIES.items():
            print(f"{name:<14} {desc}")
        return 0

    wanted = set(args.categories) if args.categories else set(CATEGORIES)
    unknown = wanted - set(CATEGORIES)
    if unknown:
        parser.error(f"unknown category: {', '.join(sorted(unknown))}")

    # counts[category] = [hits_in_cyrillic_files, hits_in_ascii_files]
    counts = {name: [0, 0] for name in CATEGORIES}
    listing = []  # (priority, path, category, lineno, snippet)

    for path in sorted(iter_source_files(args.paths)):
        cyrillic, hits = scan_file(path, wanted)
        for category, lineno, snippet in hits:
            counts[category][0 if cyrillic else 1] += 1
            show = cyrillic or args.full
            if category in HIGH_VOLUME and not args.full:
                show = False
            if show:
                listing.append((0 if cyrillic else 1, path, category, lineno, snippet))

    print("=" * 78)
    print("byte-vs-char audit  (Cyr = files with Russian text -> where bugs are observable)")
    print("=" * 78)
    print(f"{'category':<14}{'Cyr':>8}{'ASCII':>8}   description")
    print("-" * 78)
    for name, (_re, desc) in CATEGORIES.items():
        if name not in wanted:
            continue
        c_cyr, c_ascii = counts[name]
        flag = "  [high-volume]" if name in HIGH_VOLUME else ""
        print(f"{name:<14}{c_cyr:>8}{c_ascii:>8}   {desc}{flag}")
    print("-" * 78)
    total_cyr = sum(c[0] for c in counts.values())
    total_ascii = sum(c[1] for c in counts.values())
    print(f"{'TOTAL':<14}{total_cyr:>8}{total_ascii:>8}")
    print()

    listing.sort(key=lambda row: (row[0], row[1], row[3]))
    current_file = None
    for _priority, path, category, lineno, snippet in listing:
        if path != current_file:
            current_file = path
            print(f"\n### {path}")
        print(f"  {lineno:>6}  [{category}]  {snippet}")

    if not args.full:
        print("\n(high-volume categories and ASCII-only files hidden; re-run with --full)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
