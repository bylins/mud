#!/usr/bin/env python3
"""Stage 3 data migration for issue.potency-noise (KOI8-safe: latin-1 byte round-trip).
- spells.xml: <dices ndice sdice adice> -> <noise sigma> (sigma = the spell's <amount> sigma;
  removed for pure-buff spells). <amount ... sigma=S ...> -> <amount min beta weight="1">.
  inline <modifier> drop dead dices_weight/alpha.
- affects.xml: <modifier> drop dead dices_weight/alpha; REBALANCE the live-dice ones (kInsanity,
  kDespodency) by folding the imposing spell's dice mean into min/beta (deterministic; the flawed
  dice spread is intentionally dropped). D=7.5 = E[3d4] (kWarcryOfMadness imposes kInsanity; the
  madness-family default is used for kDespodency, whose imposer is code-side).
"""
import re, sys

DICE_MEAN = 7.5  # E[3d4] of kWarcryOfMadness; representative for the madness-family live-dice affects

def fmt(x):
    if abs(x - round(x)) < 1e-9:
        return str(int(round(x)))
    return f"{x:.6g}"

def attrs(tag):
    return dict(re.findall(r'(\w+)="([^"]*)"', tag))

def migrate_spells(t):
    changed = [0, 0]
    # damage <amount> AND the points categories are all Amount structs (min/dices_weight/alpha/beta/sigma,
    # points add npc_coeff). Same migration for all.
    AMT = r'(?:amount|heal|moves|thirst|full)'
    def proc_spell(m):
        b = m.group(0)
        sig = re.findall(r'<' + AMT + r'[^>]*\bsigma="([^"]*)"', b)
        base = sig[0] if sig else None
        # <dices> -> <noise> (or drop for pure-buff)
        if base:
            b = re.sub(r'<dices\b[^>]*/>', f'<noise sigma="{base}" />', b)
        else:
            b = re.sub(r'[ \t]*<dices\b[^>]*/>\r?\n', '', b)
        # <amount>/<heal>/<moves>/<thirst>/<full>
        def rwa(am):
            tag = re.match(r'<(\w+)', am.group(0)).group(1)
            a = attrs(am.group(0)); changed[0] += 1
            parts = []
            if 'npc_coeff' in a:
                parts.append(f'npc_coeff="{a["npc_coeff"]}"')
            parts += [f'min="{a.get("min","0")}"', f'beta="{a.get("beta","0")}"', 'weight="1"']
            return f'<{tag} ' + ' '.join(parts) + ' />'
        b = re.sub(r'<' + AMT + r'\b[^>]*/>', rwa, b)
        # inline <modifier> (drop dead dices_weight/alpha; keep min/beta/factor/cap)
        b = re.sub(r'<modifier\b[^>]*/>', lambda mm: rw_modifier(mm, changed, fold=False), b)
        return b
    out = re.sub(r'<spell id="[^"]*".*?</spell>', proc_spell, t, flags=re.S)
    print(f"  spells.xml: {changed[0]} <amount> rewritten, {changed[1]} inline <modifier> cleaned")
    return out

def rw_modifier(m, changed, fold):
    a = attrs(m.group(0))
    mn = float(a.get('min', 0)); dw = float(a.get('dices_weight', 0))
    al = float(a.get('alpha', 0)); be = float(a.get('beta', 0))
    if fold and dw != 0.0:
        mn = mn + DICE_MEAN * dw            # fold dice MEAN into the flat floor
        be = be + DICE_MEAN * dw * al       # and the alpha*dice*C term into beta
    parts = [f'min="{fmt(mn)}"', f'beta="{fmt(be)}"', f'factor="{a.get("factor","1")}"']
    if 'cap' in a:
        parts.append(f'cap="{a["cap"]}"')
    changed[1] += 1
    return '<modifier ' + ' '.join(parts) + ' />'

def migrate_affects(t):
    changed = [0, 0]
    reb = []
    cur = ['?']
    def track(m):
        cur[0] = m.group(1); return m.group(0)
    # rebalance the live-dice modifiers (fold), clean the rest
    def rw(m):
        a = attrs(m.group(0)); dw = float(a.get('dices_weight', 0))
        if dw != 0.0:
            reb.append((cur[0], m.group(0)))
        return rw_modifier(m, changed, fold=True)
    # affect-owned Amount tags (DoT <amount>, regen <heal>, ...): drop dead dice attrs; a live
    # dices_weight there is the stored-potency DAMAGE scale (base_override), so it folds into beta.
    def rwa(m):
        tag = re.match(r'<(\w+)', m.group(0)).group(1)
        a = attrs(m.group(0)); dw = float(a.get('dices_weight', 0)); be = float(a.get('beta', 0))
        if dw != 0.0:
            be = be + dw                     # DoT: beta*stored_potency reproduces dw*stored_potency
            reb.append((cur[0] + ' (DoT amount)', m.group(0)))
        parts = []
        if 'npc_coeff' in a:
            parts.append(f'npc_coeff="{a["npc_coeff"]}"')
        parts += [f'min="{a.get("min","0")}"', f'beta="{fmt(be)}"', 'weight="0"']
        changed[0] += 1
        return f'<{tag} ' + ' '.join(parts) + ' />'
    lines = t.split('\n')
    out = []
    for ln in lines:
        am = re.search(r'<affect id="([^"]*)"', ln)
        if am: cur[0] = am.group(1)
        if '<modifier' in ln:
            ln = re.sub(r'<modifier\b[^>]*/>', rw, ln)
        if re.search(r'<(?:amount|heal|moves|thirst|full)\b', ln):
            ln = re.sub(r'<(?:amount|heal|moves|thirst|full)\b[^>]*/>', rwa, ln)
        out.append(ln)
    print(f"  affects.xml: {changed[1]} <modifier> cleaned; {len(reb)} rebalanced (live-dice):")
    for aid, tag in reb:
        print(f"    {aid}: {tag.strip()}")
    return '\n'.join(out)

def main():
    base = "/home/sventovit/claude_1/mud-potency-noise/lib/cfg"
    for fn, fn_mig in (("spells.xml", migrate_spells), ("affects.xml", migrate_affects)):
        p = f"{base}/{fn}"
        d = open(p, encoding='latin-1').read()
        print(fn + ":")
        d2 = fn_mig(d)
        # verify no dead attrs remain IN TAGS (comments may still mention them)
        tag_attrs = re.findall(r'<[a-z_]+\b[^>]*\b(dices_weight|alpha)="[^"]*"[^>]*/?>', d2)
        assert not tag_attrs, f"leftover dead attr in a {fn} tag: {tag_attrs[:3]}"
        if fn == "spells.xml":
            assert '<dices' not in d2, "leftover <dices>"
        open(p, 'w', encoding='latin-1').write(d2)
    print("DONE")

if __name__ == "__main__":
    main()
