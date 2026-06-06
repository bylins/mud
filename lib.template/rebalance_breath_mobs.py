#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
rebalance_breath_mobs.py  (issue.breathe-rebalance)

Breath used to be a pseudo-spell whose damage scaled with the mob's Wisdom and the
element's magic skill; it was reworked (issue.breath) to deal the mob's *melee dice*
as magic damage, so that Wis/skill scaling was lost.  The old and new breath share
the same base (the mob's dice + damroll + STR bonus); the ONLY thing the new breath
dropped is the flat competence term:

    OLD_breath = NEW_breath + (2 + ceil(2 * C))
    C          = skill_coeff + stat_coeff
    skill_coeff = (min(skill,75)*0.4 + max(0,skill-75)*2.5) / 100      # base_skill low/hi bonus
    stat_coeff  = max(0, Wis-22) * 0.5 / 100                            # base_stat kWis weight

So to make the new breath match the old one we add  boost = 2 + ceil(2*C)  to the
mob's damroll (the +Z of its damage dice), leaving the dice (damnodice/damsizedice)
untouched -- this reproduces the old breath's mean AND spread exactly.

The script edits ONLY the damroll number of the one damage-dice token of each breath
mob; every other byte (encoding, line order, other fields/especs) is preserved.

Mob-file facts it relies on (CircleMUD/Bylins, KOI8-R):
  * Flags are <letter><plane-digit> pairs (letter = bit a=0..z=25,A=26..; digit =
    30-bit plane).  Breath flags are EMobFlag = action flags (1st token of the
    "actflags affflags align type" line), plane 2 bits 0-4:
        a2=kFireBreath b2=kGasBreath c2=kFrostBreath d2=kAcidBreath e2=kLightingBreath
    (first match wins, in GetBreathAttack order: Fire, Gas, Frost, Acid, Lighting).
  * The line right after the flag line is  "level thac0 ac HPdice DAMdice"  -- the
    5th token (last) is the damage dice  NdS+Z.
  * Wis is the espec line  "Wis: <n>"; the magic skill is  "Skill: <id> <val>"
    (kAirMagic=182 kFireMagic=183 kWaterMagic=184 kEarthMagic=185 kDarkMagic=187),
    clamped to the skill cap (1000), matching the engine's GetSkill for a mob.

Usage:
    python3 rebalance_breath_mobs.py [--world DIR] [--apply]

Default is a DRY RUN: prints a per-mob old->new report and writes nothing.
Pass --apply to rewrite the .mob files in place (after a round-trip verification
that only the intended damroll tokens changed).
"""

import argparse
import math
import os
import re
import sys

ENCODING = "koi8-r"
NOVICE_THRESHOLD = 75          # abilities::kNoviceSkillThreshold
LOW_SKILL_BONUS = 0.4          # base_skill low_skill_bonus
HI_SKILL_BONUS = 2.5           # base_skill hi_skill_bonus
WIS_THRESHOLD = 22             # base_stat threshold
WIS_WEIGHT = 0.5               # base_stat weight
SKILL_CAP = 1000               # cap of the magic skills (lib/cfg/skills.xml)

# Breath flag (action-flag pair) -> (magic skill id, name), in GetBreathAttack order.
BREATH_ORDER = [
    ("a2", 183, "kFireBreath"),    # kFire  -> kFireMagic
    ("b2", 185, "kGasBreath"),     # kEarth -> kEarthMagic
    ("c2", 182, "kFrostBreath"),   # kAir   -> kAirMagic
    ("d2", 184, "kAcidBreath"),    # kWater -> kWaterMagic
    ("e2", 187, "kLightingBreath"),# kDark  -> kDarkMagic
]

FLAG_RE = re.compile(r"^(?P<act>\S+)\s+(?P<aff>\S+)\s+(?P<align>-?\d+)\s+(?P<type>[SE])\s*\r?$")
STATS_RE = re.compile(
    r"^(?P<pre>\s*\d+\s+-?\d+\s+-?\d+\s+\d+d\d+\+-?\d+\s+)"
    r"(?P<dam>\d+d\d+\+-?\d+)(?P<post>\s*\r?)$"
)
DAM_RE = re.compile(r"^(?P<n>\d+)d(?P<s>\d+)\+(?P<z>-?\d+)$")
FLAGPAIR_RE = re.compile(r"[A-Za-z]\d")
VNUM_RE = re.compile(r"^#(\d+)\s*\r?$")
SKILL_RE = re.compile(r"^Skill:\s*(\d+)\s+(-?\d+)\s*\r?$")
WIS_RE = re.compile(r"^Wis:\s*(-?\d+)\s*\r?$")


def detect_breath(act_field):
    """Return (skill_id, name) for the first breath flag present, else None."""
    pairs = set(FLAGPAIR_RE.findall(act_field))
    for pair, skill_id, name in BREATH_ORDER:
        if pair in pairs:
            return skill_id, name
    return None


def read_espec(lines, start):
    """Scan the espec of one mob (from the stats line forward) for Wis and Skill lines.

    Stops at the espec terminator 'E', the next mob '#', the world terminator '$',
    or the next flag line. Returns (wis_or_None, {skill_id: value})."""
    wis = None
    skills = {}
    for j in range(start, len(lines)):
        ln = lines[j].rstrip("\r")
        if ln == "E" or ln == "$" or ln.startswith("#"):
            break
        if j > start and FLAG_RE.match(lines[j]):  # next mob's flag line
            break
        mw = WIS_RE.match(ln)
        if mw:
            wis = int(mw.group(1))
            continue
        ms = SKILL_RE.match(ln)
        if ms:
            skills[int(ms.group(1))] = int(ms.group(2))
    return wis, skills


def competence(skill, wis):
    skill = max(0, min(skill, SKILL_CAP))
    low = min(skill, NOVICE_THRESHOLD)
    hi = max(0, skill - NOVICE_THRESHOLD)
    skill_coeff = (low * LOW_SKILL_BONUS + hi * HI_SKILL_BONUS) / 100.0
    stat_coeff = max(0, wis - WIS_THRESHOLD) * WIS_WEIGHT / 100.0
    return skill_coeff + stat_coeff


def process_file(path):
    """Return (changes, new_text) where changes is a list of dicts; new_text is the
    rebalanced file content (identical to the original where nothing changed)."""
    raw = open(path, "rb").read()
    try:
        text = raw.decode(ENCODING)
    except UnicodeDecodeError as exc:
        print(f"  !! {path}: not valid {ENCODING} ({exc}); skipped", file=sys.stderr)
        return [], None
    # Always split on '\n'. A CRLF line keeps its trailing '\r' on the line (the regexes use
    # '\r?$' and the rewrite preserves it via the 'post' group), so '\n'.join restores the file
    # byte-for-byte -- robust to LF, CRLF, and mixed line endings (some zone files have a stray CRLF).
    lines = text.split("\n")

    changes = []
    cur_vnum = None
    for i, line in enumerate(lines):
        mv = VNUM_RE.match(line)
        if mv:
            cur_vnum = mv.group(1)
            continue
        mflag = FLAG_RE.match(line)
        if not mflag or i + 1 >= len(lines):
            continue
        mstats = STATS_RE.match(lines[i + 1])
        if not mstats:
            continue
        breath = detect_breath(mflag.group("act"))
        if not breath:
            continue
        skill_id, name = breath
        wis, skills = read_espec(lines, i + 1)
        skill = skills.get(skill_id, 0)
        wis_val = wis if wis is not None else 0
        c = competence(skill, wis_val)
        boost = 2 + math.ceil(2.0 * c)

        mdam = DAM_RE.match(mstats.group("dam"))
        n, s, z = int(mdam.group("n")), int(mdam.group("s")), int(mdam.group("z"))
        new_dam = f"{n}d{s}+{z + boost}"
        # Replace ONLY the dam token in the stats line (preserve all surrounding bytes).
        old_stats = lines[i + 1]
        new_stats = mstats.group("pre") + new_dam + mstats.group("post")
        lines[i + 1] = new_stats

        changes.append({
            "vnum": cur_vnum, "line": i + 1, "breath": name,
            "wis": wis, "skill": skill, "c": c, "boost": boost,
            "old_dam": mstats.group("dam"), "new_dam": new_dam,
            "old_line": old_stats, "new_line": new_stats,
        })

    if not changes:
        return [], None
    return changes, "\n".join(lines)


def verify(path, original_text, new_text, changes):
    """Sanity: re-encodes cleanly and only the recorded stats lines changed."""
    new_text.encode(ENCODING)  # raises if anything is un-encodable
    a = original_text.split("\n")
    b = new_text.split("\n")
    if len(a) != len(b):
        raise AssertionError(f"{path}: line count changed")
    changed_idx = {c["line"] for c in changes}
    for idx, (la, lb) in enumerate(zip(a, b)):
        if la != lb and idx not in changed_idx:
            raise AssertionError(f"{path}: unexpected change at line {idx + 1}")


def main():
    ap = argparse.ArgumentParser(description="Rebalance breath-mob damage dice.")
    ap.add_argument("--world", default=os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "lib", "world", "mob"),
        help="directory of .mob files (default: ../lib/world/mob relative to this script)")
    ap.add_argument("--apply", action="store_true",
        help="rewrite the files in place (default: dry run, prints a report only)")
    args = ap.parse_args()

    mob_dir = os.path.abspath(args.world)
    if not os.path.isdir(mob_dir):
        print(f"error: mob directory not found: {mob_dir}", file=sys.stderr)
        return 2

    files = sorted(f for f in os.listdir(mob_dir) if f.endswith(".mob"))
    total_mobs = 0
    total_files = 0
    print(f"{'mode':6} {'vnum':>7}  {'breath':16} {'Wis':>4} {'skill':>5} "
          f"{'C':>6} {'boost':>5}  {'old dice':>14} -> {'new dice'}")
    print("-" * 86)
    mode = "APPLY" if args.apply else "DRY"

    for fn in files:
        path = os.path.join(mob_dir, fn)
        raw = open(path, "rb").read()
        try:
            original_text = raw.decode(ENCODING)
        except UnicodeDecodeError:
            continue
        changes, new_text = process_file(path)
        if not changes:
            continue
        total_files += 1
        for c in changes:
            total_mobs += 1
            wis_s = str(c["wis"]) if c["wis"] is not None else "n/a"
            print(f"{mode:6} {str(c['vnum']):>7}  {c['breath']:16} {wis_s:>4} "
                  f"{c['skill']:>5} {c['c']:>6.3f} {c['boost']:>5}  "
                  f"{c['old_dam']:>14} -> {c['new_dam']}")
        if args.apply:
            verify(path, original_text, new_text, changes)
            with open(path, "wb") as out:
                out.write(new_text.encode(ENCODING))

    print("-" * 86)
    action = "rewritten" if args.apply else "would change (dry run)"
    print(f"{total_mobs} breath mob(s) in {total_files} file(s) {action}.")
    if not args.apply and total_mobs:
        print("Re-run with --apply to write the changes.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
