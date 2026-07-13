#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
issue.random-noise-rework P2 -- competence-weight rebalance for spells.xml.

Problem: the potency-roll competence weights are tiny and INVERTED (low_skill_bonus
weighted higher than hi_skill_bonus), so a spell's effect barely depends on skill/stat
and is dominated by dice noise. See tests/random.noise.cpp (RebalanceModel).

This converter, per spell that already scales with skill (low>0 or hi>0):
  1. Sets the POTENCY roll weights to the new hi-dominant scheme (low=1, hi=7, stat
     weight=18), keeping each spell's stat THRESHOLD unchanged. success_roll is left
     untouched (spell landing is a separate axis, out of scope).
  2. Rescales every competence-scaled effect (<damage><amount>, <points><heal|moves>,
     <affects><apply><modifier> with beta!=0) so its MEAN is preserved at the reference
     (skill=140, stat=30) while it now scales steeply with the new competence:
         k = old_scaled / C_new(ref)     where old_scaled = mu_dice*dw*(1+alpha*C_old)
                                                            + beta*C_old
     emitting dices_weight=0, alpha=0, beta=k.
  3. Adds sigma to <damage><amount> only (bounded relative variance):
         calm  elements (earth/water/light/mind/life/undefined/...) -> 0.15
         wild  elements (dark/fire/air)                             -> 0.25
     Modifiers/heals stay deterministic (buffs shouldn't jitter each recast).

The file is KOI8-R; we read/write as latin-1 so Russian bytes in <name>/<desc> pass
through untouched (all edited attributes are ASCII). Default is a DRY RUN (report only);
pass --apply to write spells.xml in place.
"""
import argparse, math, re, sys

# --- new weights (percent; parser divides by 100) ---
NEW_LOW, NEW_HI, NEW_STATW = 1.0, 7.0, 18.0
# Anchor: skill=REF_SKILL (R12), stat = spell's own threshold + REF_STAT_OFFSET. A relative
# stat anchor gives every spell the same stat contribution at its reference point, so the
# skill span is uniform (~5x) instead of exploding for spells whose threshold exceeds a flat
# reference stat (e.g. the primary nukes at threshold 32 would go pure-skill under stat=30).
REF_SKILL, REF_STAT_OFFSET = 140, 8
CALM_SIGMA, WILD_SIGMA = 0.15, 0.25
WILD_ELEMENTS = {"kDark", "kFire", "kAir"}


def skill_coeff(skill, low, hi):
    lo = min(skill, 75); hisk = max(0, skill - 75)
    return (lo * low + hisk * hi) / 100.0


def stat_coeff(stat, thr, w):
    return max(0, stat - thr) * w / 100.0


def competence(skill, stat, low, hi, thr, w):
    return skill_coeff(skill, low, hi) + stat_coeff(stat, thr, w)


def getf(s, name, default):
    m = re.search(r'\b%s="([^"]*)"' % name, s)
    return float(m.group(1)) if m else default


def geti(s, name, default):
    m = re.search(r'\b%s="([^"]*)"' % name, s)
    return int(m.group(1)) if m else default


def set_attr(s, name, val):
    """Set attr (in-place if present, else insert before the self-closing />)."""
    if re.search(r'\b%s="[^"]*"' % name, s):
        return re.sub(r'\b%s="[^"]*"' % name, '%s="%s"' % (name, val), s, count=1)
    # preserve the trailing newline (\1) -- a bare \s*$ ate it and merged this line into the next.
    return re.sub(r'\s*/>(\s*)$', ' %s="%s" />\\1' % (name, val), s, count=1)


def fmtk(x):
    """Compact numeric formatting for k (drop trailing zeros)."""
    return ("%.4f" % x).rstrip("0").rstrip(".") or "0"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("path")
    ap.add_argument("--apply", action="store_true", help="write in place (default: dry run)")
    ap.add_argument("--verbose", action="store_true")
    args = ap.parse_args()

    with open(args.path, "r", encoding="latin-1") as f:
        lines = f.readlines()

    # per-spell state
    element = None
    in_pot = in_suc = False
    nd = sd = ad = 0
    plow = phi = pthr = pstatw = None
    ref_stat = None
    c_old = c_new = None
    convert = False  # this spell scales with skill?
    stats = {"spells": 0, "converted": 0, "skipped_flat": 0,
             "amounts": 0, "modifiers": 0, "heals": 0, "fixed_left": 0}
    report = []

    def finalize_potency():
        nonlocal c_old, c_new, convert, pthr, pstatw, ref_stat
        convert = (plow or 0) > 0 or (phi or 0) > 0
        if convert:
            pthr = pthr if pthr is not None else 0
            pstatw = pstatw if pstatw is not None else 0.0
            ref_stat = pthr + REF_STAT_OFFSET
            c_old = competence(REF_SKILL, ref_stat, plow, phi, pthr, pstatw)
            c_new = competence(REF_SKILL, ref_stat, NEW_LOW, NEW_HI, pthr, NEW_STATW)

    def rescale(line, tag, add_sigma):
        """Rewrite a competence-scaled effect line; return (line, changed)."""
        nonlocal stats
        if not convert or not c_new or c_new <= 0:
            return line, False
        mn = getf(line, "min", 0.0)
        dw = getf(line, "dices_weight", 1.0)
        alpha = getf(line, "alpha", 0.0)
        beta = getf(line, "beta", 1.0)
        mu = nd * (sd + 1) / 2.0 + ad
        old_scaled = mu * dw * (1.0 + alpha * c_old) + beta * c_old
        if old_scaled <= 1e-9:
            stats["fixed_left"] += 1
            return line, False  # fixed effect (beta=0, no dice) -> leave as-is
        k = old_scaled / c_new
        new = set_attr(line, "dices_weight", "0")
        new = set_attr(new, "alpha", "0")
        new = set_attr(new, "beta", fmtk(k))
        if add_sigma:
            sig = WILD_SIGMA if element in WILD_ELEMENTS else CALM_SIGMA
            new = set_attr(new, "sigma", sig)
        if args.verbose:
            def at(sk):  # mean at skill sk (stat = spell's anchor stat)
                co = competence(sk, ref_stat, plow, phi, pthr, pstatw)
                cn = competence(sk, ref_stat, NEW_LOW, NEW_HI, pthr, NEW_STATW)
                old = mn + mu * dw * (1 + alpha * co) + beta * co
                return old, mn + k * cn
            o75, n75 = at(75); o140, n140 = at(140); o200, n200 = at(200)
            report.append("    %-9s OLD %.0f/%.0f/%.0f  NEW %.0f/%.0f/%.0f  (k=%s%s)"
                          % (tag, o75, o140, o200, n75, n140, n200, fmtk(k),
                             (", sigma=%s" % (WILD_SIGMA if element in WILD_ELEMENTS else CALM_SIGMA)) if add_sigma else ""))
        return new, True

    out = []
    cur_spell = None
    for line in lines:
        m = re.search(r'<spell id="([^"]*)"(?:[^>]*\belement="([^"]*)")?', line)
        if m:
            cur_spell = m.group(1)
            element = m.group(2)
            in_pot = in_suc = False
            nd = sd = ad = 0
            plow = phi = pthr = pstatw = None
            ref_stat = None
            c_old = c_new = None
            convert = False
            stats["spells"] += 1
            if args.verbose:
                report.append("%s [%s]" % (cur_spell, element))

        if "<potency_roll>" in line:
            in_pot = True
        if "<success_roll>" in line:
            in_suc = True

        if in_pot and "<dices" in line:
            nd = geti(line, "ndice", 0); sd = geti(line, "sdice", 0); ad = geti(line, "adice", 0)
        if in_pot and "<base_skill" in line:
            plow = getf(line, "low_skill_bonus", 0.0); phi = getf(line, "hi_skill_bonus", 0.0)
            if (plow or 0) > 0 or (phi or 0) > 0:
                line = set_attr(line, "low_skill_bonus", fmtk(NEW_LOW))
                line = set_attr(line, "hi_skill_bonus", fmtk(NEW_HI))
        if in_pot and "<base_stat" in line:
            pthr = geti(line, "threshold", 0); pstatw = getf(line, "weight", 0.0)
            if (plow or 0) > 0 or (phi or 0) > 0:
                line = set_attr(line, "weight", fmtk(NEW_STATW))

        if "</potency_roll>" in line:
            finalize_potency()
            in_pot = False
            if convert:
                stats["converted"] += 1
            else:
                stats["skipped_flat"] += 1
        if "</success_roll>" in line:
            in_suc = False

        # effects (never inside success_roll)
        if not in_suc:
            if "<amount" in line:
                line, ch = rescale(line, "damage", add_sigma=True)
                if ch: stats["amounts"] += 1
            elif "<modifier" in line:
                line, ch = rescale(line, "modifier", add_sigma=False)
                if ch: stats["modifiers"] += 1
            elif "<heal" in line or "<moves" in line:
                # heals/moves spread like damage (per-element sigma); modifiers stay deterministic.
                line, ch = rescale(line, "points", add_sigma=True)
                if ch: stats["heals"] += 1

        out.append(line)

    if args.verbose:
        print("\n".join(report))
    print("\n=== summary ===")
    for k, v in stats.items():
        print("  %-14s %d" % (k, v))
    print("  reference: skill=%d stat=threshold+%d   weights: low=%g hi=%g statw=%g" %
          (REF_SKILL, REF_STAT_OFFSET, NEW_LOW, NEW_HI, NEW_STATW))

    if args.apply:
        with open(args.path, "w", encoding="latin-1") as f:
            f.writelines(out)
        print("  -> WROTE %s" % args.path)
    else:
        print("  (dry run; pass --apply to write)")


if __name__ == "__main__":
    main()
