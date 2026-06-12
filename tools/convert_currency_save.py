#!/usr/bin/env python3
"""
Convert Bylins player save files (*.player) from the legacy per-currency tags
to the unified currency container format (issue.currency-storage hardening).

Legacy tags  ->  unified `Curr: <text_id> <hand> <bank>` lines:
    Gold: N   ->  kKuna        hand = N
    Bank: N   ->  kKuna        bank = N   (merged with Gold into one kKuna line)
    Hry : N   ->  kCopperGrivna hand = N
    ICur: N   ->  kMagicIce    hand = N

Notes
-----
* Glory is NOT a container currency (it lives in GloryConst's own file); the
  `Tglo:` tag (glory respec time) and `Ruble:` are left untouched.
* All-zero currencies are dropped (absent == 0 on load), matching the new save.
* Files that already contain a `Curr:` line are skipped (idempotent).
* Player files are KOI8-R; only ASCII tag/number lines are touched, so we
  round-trip the bytes through latin-1 and never re-encode the Russian text.

Usage
-----
    python3 tools/convert_currency_save.py <path>            # DRY RUN (default)
    python3 tools/convert_currency_save.py <path> --apply    # write changes

<path> may be a single .player file or a directory (searched recursively).
"""
import os
import re
import sys

# legacy tag -> (currency text_id, slot)   slot: 0 = hand, 1 = bank
TAGMAP = {
    "Gold": ("kKuna", 0),
    "Bank": ("kKuna", 1),
    "Hry ": ("kCopperGrivna", 0),
    "ICur": ("kMagicIce", 0),
}
LINE_RE = re.compile(r"^(Gold|Bank|Hry |ICur): *(-?\d+)\s*$")


def convert_text(text):
    """Return (new_text, summary) or (None, reason) if nothing to do."""
    lines = text.split("\n")
    if any(ln.startswith("Curr:") for ln in lines):
        return None, "already converted (has Curr:)"

    amounts = {}          # text_id -> [hand, bank]
    old_indices = []
    for i, ln in enumerate(lines):
        m = LINE_RE.match(ln)
        if not m:
            continue
        text_id, slot = TAGMAP[m.group(1)]
        amounts.setdefault(text_id, [0, 0])[slot] = int(m.group(2))
        old_indices.append(i)

    if not old_indices:
        return None, "no legacy currency tags"

    curr_lines = [
        f"Curr: {tid} {hb[0]} {hb[1]}"
        for tid, hb in sorted(amounts.items())
        if hb[0] != 0 or hb[1] != 0
    ]

    old_set = set(old_indices)
    out, inserted = [], False
    for i, ln in enumerate(lines):
        if i in old_set:
            if not inserted:
                out.extend(curr_lines)
                inserted = True
            continue                      # drop the legacy line
        out.append(ln)

    summary = ", ".join(curr_lines) if curr_lines else "(all zero -> no lines)"
    return "\n".join(out), summary


def iter_player_files(path):
    if os.path.isfile(path):
        yield path
        return
    for root, _dirs, files in os.walk(path):
        for name in files:
            if name.endswith(".player"):
                yield os.path.join(root, name)


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 1
    path = argv[1]
    apply = "--apply" in argv[2:]

    n_changed = n_skipped = 0
    for fpath in sorted(iter_player_files(path)):
        text = open(fpath, "rb").read().decode("latin-1")
        new_text, info = convert_text(text)
        if new_text is None:
            n_skipped += 1
            print(f"  skip  {fpath}: {info}")
            continue
        n_changed += 1
        print(f"{'WRITE' if apply else 'would'} {fpath}: {info}")
        if apply:
            with open(fpath, "wb") as f:
                f.write(new_text.encode("latin-1"))

    mode = "applied" if apply else "DRY RUN (use --apply to write)"
    print(f"\n{mode}: {n_changed} file(s) to convert, {n_skipped} skipped.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
