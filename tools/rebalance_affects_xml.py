#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
issue.random-noise-rework, unstable adaptation: rebalance the affect-owned modifiers in affects.xml.

On unstable the affect stat-change modifiers moved out of spells.xml into affects.xml (the affect is a
first-class entity). The spells.xml converter (rebalance_potency.py) rescales the spell's inline
effects; this companion rescales the affect-owned <modifier> the same way, and stamps the additive
<affect dispel_mod=> on the critical buffs.

An affect is SHARED across imposing spells, so it has no own potency roll -- it anchors off the
cookie-cutter competence (the standard old vs new potency weights at the reference skill=140, stat=
threshold+8=30). Every affect modifier is dw=0 (deterministic), so the rescale collapses to a constant
factor that preserves the modifier's mean at the anchor while it now scales with the steeper new
competence:

    C_old = skill_coeff(140, low=3, hi=1.25)   + stat_coeff(30, thr=22, w=0.5)  = 3.0625 + 0.04 = 3.1025
    C_new = skill_coeff(140, low=1, hi=7)       + stat_coeff(30, thr=22, w=18)   = 5.30   + 1.44 = 6.74
    beta_new = beta_old * (C_old / C_new)       # ~= beta_old * 0.4603

Modifiers stay deterministic (no sigma -- buffs shouldn't jitter). beta=0 modifiers are fixed and left
alone. Critical buffs get dispel_mod="-35" (50 - 35 = 15% dispel at competence parity).

KOI8-R safe (latin-1 IO; edited attrs are ASCII). Default dry-run; --apply writes.
"""
import argparse, re

C_OLD = (75*3 + 65*1.25)/100.0 + max(0, 30-22)*0.5/100.0   # 3.1025
C_NEW = (75*1 + 65*7)/100.0    + max(0, 30-22)*18.0/100.0   # 6.74
FACTOR = C_OLD / C_NEW
CRIT_BUFFS = {"kSanctuary", "kPrismaticAura", "kFireShield", "kIceShield", "kAirShield", "kFly", "kWaterBreath"}
DISPEL_MOD = -35


def fmt(x):
    return ("%.4f" % x).rstrip("0").rstrip(".") or "0"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("path")
    ap.add_argument("--apply", action="store_true")
    args = ap.parse_args()
    d = open(args.path, encoding="latin-1").read()
    stats = {"dispel_mod": 0, "rescaled": 0, "fixed_left": 0}

    # 1) dispel_mod on critical-buff affects (attribute on the <affect ...> open tag)
    def stamp(m):
        tag, aid = m.group(0), m.group(1)
        if aid in CRIT_BUFFS and "dispel_mod=" not in tag:
            stats["dispel_mod"] += 1
            return tag[:-1] + ' dispel_mod="%d">' % DISPEL_MOD
        return tag
    d = re.sub(r'<affect id="(\w+)"[^>]*>', stamp, d)

    # 2) rescale affect-owned <modifier> beta (dw=0 -> constant factor); leave beta=0 fixed
    def resc(m):
        line = m.group(0)
        bm = re.search(r'beta="(-?[0-9.]+)"', line)
        if not bm:
            return line
        b = float(bm.group(1))
        if b == 0.0:
            stats["fixed_left"] += 1
            return line
        stats["rescaled"] += 1
        return line.replace(bm.group(0), 'beta="%s"' % fmt(b * FACTOR))
    d = re.sub(r'<modifier[^>]*/>', resc, d)

    print("C_old=%.4f C_new=%.4f factor=%.4f" % (C_OLD, C_NEW, FACTOR))
    for k, v in stats.items():
        print("  %-12s %d" % (k, v))
    if args.apply:
        open(args.path, "w", encoding="latin-1").write(d)
        print("  -> WROTE %s" % args.path)
    else:
        print("  (dry run)")


if __name__ == "__main__":
    main()
