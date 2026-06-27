#!/usr/bin/env python3
"""
Convert legacy per-currency DG-script reward commands in *.trg files to the new
universal currency commands (issue.currencies / issue.currency-storage).

Why
---
The old per-currency DG actor fields were removed from the engine, EXCEPT the
kKuna pair (`gold` / `bank`), which is kept as a dedicated shorthand so the many
thousands of `%actor.gold(...)%` scripts do not have to be touched. Every other
currency field is gone and must move to the universal field form:

    %<who>.AddCurrency(<text_id>, <amount>)%   - начислить (минус снимает)
    %<who>.Currency(<text_id>)%                - прочитать сумму на руках

Mappings applied by this script
-------------------------------
    %X.nogata(+EXPR)%        ->  %X.AddCurrency(kNogata, EXPR)%
    %X.nogata(-EXPR)%        ->  %X.AddCurrency(kNogata, -EXPR)%
    %X.point_nogata(+EXPR)%  ->  %X.AddCurrency(kNogata, EXPR)%   (см. примечание)
    %X.point_nogata(-EXPR)%  ->  %X.AddCurrency(kNogata, -EXPR)%
    %X.hryvn(+EXPR)%         ->  %X.AddCurrency(kCopperGrivna, EXPR)%
    %X.hryvn(-EXPR)%         ->  %X.AddCurrency(kCopperGrivna, -EXPR)%
    %X.nogata%        (read) ->  %X.Currency(kNogata)%
    %X.point_nogata%  (read) ->  %X.Currency(kNogata)%
    %X.hryvn%         (read) ->  %X.Currency(kCopperGrivna)%

LEFT UNTOUCHED on purpose:
    %X.gold(...)% / %X.bank(...)%   - это и есть оставленная отдельная команда
                                      для kKuna (currencies::kGold == "kKuna").

Примечание про point_nogata: в движке не осталось ни поля nogata, ни поля
point_nogata, и нет отдельной валюты-"очков", поэтому оба отображаются на kNogata
(валюта на руках). Если point_nogata означал иную сущность - поправьте PROFILE.

Поле hryvn в движке работало с currencies::kCopperGrivnaId (медная гривна),
поэтому отображается на её text_id "kCopperGrivna" -- то же, что делает с тегом
`Hry:` конвертер файлов игроков (tools/convert_currency_save.py).

Notes
-----
* .trg files are KOI8-R; we only touch ASCII DG tokens, so we round-trip the
  bytes through latin-1 and never re-encode the Russian text.
* Idempotent: AddCurrency/Currency forms are not matched again on a second run.
* EXPR may be a literal (100) or a script var (%nogata%, %price%); it is copied
  through verbatim. The value never contains ')' in well-formed DG, which the
  parser relies on.

Usage
-----
    python3 tools/convert_currency_triggers.py <path>           # DRY RUN (default)
    python3 tools/convert_currency_triggers.py <path> --apply   # write changes

<path> may be a single .trg file or a directory (searched recursively for *.trg).
"""
import os
import re
import sys

# (field-name, target text_id). Order matters: longer name first so that
# "point_nogata" is matched before "nogata".
PROFILE = [
    ("point_nogata", "kNogata"),
    ("nogata", "kNogata"),
    ("hryvn", "kCopperGrivna"),
]

ENC = "latin-1"  # byte-preserving round-trip for KOI8-R payloads


def _build_patterns():
    pats = []
    for field, text_id in PROFILE:
        # award/spend form:  %who.field(<sign><value>)%
        # The sign is OPTIONAL: a bare value like point_nogata(1) or
        # point_nogata(%var%) is a positive award; only a leading '-' subtracts.
        award = re.compile(
            r"%(\w+)\." + field + r"\(\s*([+-]?)\s*([^)]*?)\s*\)%"
        )
        # bare read form:     %who.field%   (no parentheses)
        read = re.compile(r"%(\w+)\." + field + r"%")
        pats.append((text_id, award, read))
    return pats


PATTERNS = _build_patterns()


def convert_text(text):
    """Return (new_text, n_changes)."""
    n = 0

    for text_id, award, read in PATTERNS:
        def award_sub(m):
            nonlocal n
            n += 1
            who, sign, value = m.group(1), m.group(2), m.group(3)
            amount = "-" + value if sign == "-" else value
            return "%%%s.AddCurrency(%s, %s)%%" % (who, text_id, amount)

        text = award.sub(award_sub, text)

        def read_sub(m):
            nonlocal n
            n += 1
            who = m.group(1)
            return "%%%s.Currency(%s)%%" % (who, text_id)

        text = read.sub(read_sub, text)

    return text, n


def process_file(path, apply):
    with open(path, "rb") as fh:
        raw = fh.read()
    try:
        text = raw.decode(ENC)
    except UnicodeDecodeError:
        print("  SKIP (decode): %s" % path)
        return 0
    new_text, n = convert_text(text)
    if n == 0:
        return 0
    print("  %-40s %3d change(s)%s" % (os.path.basename(path), n,
                                       "" if apply else "  [dry-run]"))
    if apply:
        with open(path, "wb") as fh:
            fh.write(new_text.encode(ENC))
    return n


def iter_trg(path):
    if os.path.isfile(path):
        yield path
        return
    for root, _dirs, files in os.walk(path):
        for fn in sorted(files):
            if fn.endswith(".trg"):
                yield os.path.join(root, fn)


def main(argv):
    args = [a for a in argv[1:] if not a.startswith("-")]
    apply = "--apply" in argv[1:]
    if not args:
        print(__doc__)
        return 2
    total = 0
    for target in args:
        if not os.path.exists(target):
            print("  MISSING: %s" % target)
            continue
        for trg in iter_trg(target):
            total += process_file(trg, apply)
    print("---")
    print("TOTAL: %d change(s)%s" % (total, " written" if apply else " (dry-run; pass --apply to write)"))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
