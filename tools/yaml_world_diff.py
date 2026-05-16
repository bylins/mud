#!/usr/bin/env python3
"""Structural diff for two YAML world trees.

Used by tools/yaml_roundtrip_test.sh to compare an original YAML world (v1,
produced by the Python converter) against its resave by `circle -S` (v2). Both
files are expected to be in KOI8-R and parseable as YAML mappings.

Comparison is per-entity (mob/object/room/trigger/zone) and ignores:
- key order inside mappings;
- string-vs-int scalar form for values that look like integers (the Python
  converter emits `'8'`, the C++ emitter emits `8`; semantically equal);
- comments and blank lines (yaml-cpp/ruamel both strip them on parse).

Files present in one tree but not the other are reported. Per-zone index.yaml
files and the top-level zones/index.yaml are skipped -- they are never written
by SaveZone/SaveObjects/etc., so the C++ side legitimately doesn't produce them.

Exit code: 0 if the trees are equivalent, non-zero on any difference.
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

try:
    from ruamel.yaml import YAML
except ImportError:
    sys.stderr.write("ruamel.yaml not installed. pip install ruamel.yaml\n")
    sys.exit(2)


_yaml = YAML(typ="safe")


# Files that the C++ Save* code path never writes -- skip them on comparison.
SKIP_NAMES = {"index.yaml"}


def load_yaml(path: Path):
    with path.open("rb") as f:
        raw = f.read()
    text = raw.decode("koi8-r", errors="replace")
    return _yaml.load(text)


def normalize_scalar(v):
    """Treat string-form integers as integers; leave everything else as-is."""
    if isinstance(v, str):
        stripped = v.strip()
        # Conservative: only normalize unambiguous decimal integers.
        if stripped and (stripped.lstrip("-").isdigit()):
            try:
                return int(stripped)
            except ValueError:
                pass
    return v


def compare(a, b, path: str, diffs: list[str]):
    a = normalize_scalar(a)
    b = normalize_scalar(b)
    if type(a) is not type(b):
        diffs.append(f"{path}: type {type(a).__name__} != {type(b).__name__}: {a!r} vs {b!r}")
        return
    if isinstance(a, dict):
        all_keys = sorted(set(a.keys()) | set(b.keys()))
        for k in all_keys:
            if k not in a:
                diffs.append(f"{path}/{k}: missing in v1, v2={b[k]!r}")
            elif k not in b:
                diffs.append(f"{path}/{k}: missing in v2, v1={a[k]!r}")
            else:
                compare(a[k], b[k], f"{path}/{k}", diffs)
    elif isinstance(a, list):
        if len(a) != len(b):
            diffs.append(f"{path}: list len {len(a)} != {len(b)}: v1={a!r}, v2={b!r}")
            return
        for i, (x, y) in enumerate(zip(a, b)):
            compare(x, y, f"{path}[{i}]", diffs)
    else:
        if a != b:
            diffs.append(f"{path}: {a!r} != {b!r}")


def collect_files(root: Path):
    """Yield relative paths of yaml files we care about under root."""
    for p in root.rglob("*.yaml"):
        if p.name in SKIP_NAMES:
            continue
        yield p.relative_to(root)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("v1", type=Path, help="reference yaml world (e.g. python-converted)")
    ap.add_argument("v2", type=Path, help="re-emitted yaml world (e.g. circle -S output)")
    ap.add_argument("--max-diffs-per-file", type=int, default=10,
                    help="cap differences printed per file (default 10)")
    ap.add_argument("--max-files", type=int, default=0,
                    help="stop after this many files with diffs (0 = unlimited)")
    args = ap.parse_args()

    v1_files = set(collect_files(args.v1))
    v2_files = set(collect_files(args.v2))

    only_v1 = sorted(v1_files - v2_files)
    only_v2 = sorted(v2_files - v1_files)
    shared = sorted(v1_files & v2_files)

    n_diff_files = 0
    n_total_diffs = 0
    bad_files = 0

    if only_v1:
        print(f"== {len(only_v1)} file(s) only in v1 ==")
        for p in only_v1[:20]:
            print(f"  - {p}")
        if len(only_v1) > 20:
            print(f"  ... and {len(only_v1) - 20} more")
    if only_v2:
        print(f"== {len(only_v2)} file(s) only in v2 ==")
        for p in only_v2[:20]:
            print(f"  + {p}")
        if len(only_v2) > 20:
            print(f"  ... and {len(only_v2) - 20} more")

    for rel in shared:
        try:
            a = load_yaml(args.v1 / rel)
            b = load_yaml(args.v2 / rel)
        except Exception as e:
            print(f"!! {rel}: parse error: {e}")
            bad_files += 1
            continue
        diffs: list[str] = []
        compare(a, b, "", diffs)
        if diffs:
            n_diff_files += 1
            n_total_diffs += len(diffs)
            print(f"== {rel}: {len(diffs)} diff(s) ==")
            for d in diffs[:args.max_diffs_per_file]:
                print(f"  {d}")
            if len(diffs) > args.max_diffs_per_file:
                print(f"  ... and {len(diffs) - args.max_diffs_per_file} more")
            if args.max_files and n_diff_files >= args.max_files:
                print(f"\n(stopped after {args.max_files} files with diffs)")
                break

    print()
    print(f"Summary: {n_diff_files} file(s) with diffs ({n_total_diffs} total), "
          f"{bad_files} parse error(s), {len(only_v1)} only in v1, "
          f"{len(only_v2)} only in v2.")
    return 0 if (n_diff_files == 0 and bad_files == 0 and not only_v1 and not only_v2) else 1


if __name__ == "__main__":
    sys.exit(main())
