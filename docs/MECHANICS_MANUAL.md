# BRusMUD Mechanics Setup Manual

This document describes the **data-driven** game mechanics of the engine: where their configuration
lives, how to edit and hot-reload it, and what each parameter means.

Engine rule: a configurable mechanic is described by a config under `cfg/mechanics/…`, loaded through
`CfgManager`, hot-reloaded with `reload …`, and (if it has a scheme) edited with the `vedun` editor.
Logic that is kept in code (for example, the effect of ability feats) is not moved into the config —
such places are called out separately.

Contents:

- [Animate Dead](#animate-dead)

---

## Animate Dead

The warlock-construct mechanic: the **"raise corpse"** spell turns a corpse into a controlled undead.
The kind of undead depends on the corpse's level and the caster's mastery, the number of
simultaneously controlled creatures is bounded by a "control budget", and the raised creature's stats
scale off the spell's cast competence. All of these parameters are moved into a single file so game
designers can balance them without rebuilding the engine.

### 1. Where it lives and how to apply changes

| | |
|---|---|
| File | `cfg/mechanics/animate_dead.xml` |
| `vedun` scheme | `cfg/mechanics/animate_dead.scheme` |
| `CfgManager` id | `animate_dead` |
| Hot-reload (no restart) | `reload animatedead` |
| In-game editing | `vedun creature` (list), `vedun creature <vnum>` (edit a tier) |
| Load at boot | `MUD::CfgManager().LoadCfg("animate_dead")` in `db.cpp` (loaded with the world) |

Designer workflow: edit `animate_dead.xml` (by hand or via `vedun`) → `reload animatedead` → check
in game. Changes take effect immediately for **newly** raised undead.

> The file is plain ASCII (English attribute names) and can be edited with any editor.
> **Note:** inside XML comments `<!-- ... -->` you must not use a double dash `--`
> (it is forbidden by the XML spec) — use a single `-`.

### 2. File structure

```xml
<animate_dead>
    <!-- control budget = min(<skill>, budget_cap) - sum of held-undead weights -->
    <control skill="kDarkMagic" budget_cap="75" />

    <creature vnum="0" id="kSkeleton" proto_vnum="3001" weight="8">
        <cost corpse_max_level="5" min_rating="0" />
        <scaling>
            <hp          beta="0.25" mode="mult" />
            <damage_dice beta="1.5"  mode="add"  cap="100" />
            <hitroll     beta="0"    mode="add" />
            <ac          beta="0"    mode="add" />        <!-- lower AC = better; set cap= for a lower bound -->
            <skills      beta="0"    mode="add"  cap="200" />
            <saving      beta="0"    mode="add" />        <!-- all 4 saves; lower = better -->
            <armor       beta="0"    mode="add" />
            <morale      beta="0"    mode="add" />
            <initiative  beta="0"    mode="add" />
        </scaling>
    </creature>

    <!-- the remaining tiers, weakest to strongest ... -->
</animate_dead>
```

Tiers (`<creature>`) are listed **weakest to strongest** — this order is the "ladder" down which the
mechanic demotes the choice when a stronger tier is unavailable.

### 3. The `<control>` block — the control budget

| Attribute | Meaning |
|---|---|
| `skill` | The skill that sets the budget and the caster's "rating". Defaults to `kDarkMagic`. |
| `budget_cap` | Upper bound on the budget: even with a very high skill the budget never exceeds this. |

**Control budget** = `min(skill, budget_cap) − Σ(weight of already-held undead)`.

Each tier "costs" its `weight`. As long as the budget has room for a new creature's weight, it can be
raised; if there is no room even for the weakest tier, the raise fails with
"Вы не сможете управлять столькими последователями." ("You cannot control that many followers.").

> **Important (hidden entry floor):** the weakest tier's weight is the de-facto minimum skill at
> which anything can be raised at all. If the weakest tier has `weight=8`, a caster with a skill below
> 8 raises nothing. Keep the entry tier's `weight` no higher than the skill expected of a beginning
> necromancer.

### 4. The `<creature>` tiers and the `<cost>` block

`<creature>` attributes:

| Attribute | Meaning |
|---|---|
| `vnum` | The tier's index in the config (the `vedun` key); sets the ladder order. |
| `id` | Symbolic tier name, e.g. `kSkeleton` (for lists and logs). |
| `proto_vnum` | VNUM of the mob prototype that gets raised. |
| `weight` | Cost of one such creature against the control budget. |

The `<cost>` block — a tier's availability conditions:

| Attribute | Meaning |
|---|---|
| `corpse_max_level` | Highest source-corpse level that yields this tier (the corpse-level band). |
| `min_rating` | Smallest caster "rating" at which the tier is allowed (see below). |
| `pick` | Relative odds of being chosen within a shared top band (optional; `0`/absent = a single tier). |

### 5. How a tier is chosen on raise

When a player raises a corpse, the tier is determined in three steps (`PickTier`):

1. **Corpse-level band.** The weakest tier whose `corpse_max_level` covers the corpse level is taken.
   So a weak corpse yields weak undead, a strong one yields strong undead.

2. **Random pick within the band.** If several tiers share one top band (equal `corpse_max_level`),
   a lot is drawn between them by the `pick` weights. For example, the same "high-level" corpse yields
   a "damager" or a "breather" 50/50.

3. **Caster-rating gate.** The chosen tier is demoted down the ladder until the caster's rating meets
   the tier's `min_rating`:

   ```
   rating = level + pseudo-remort + 4
   pseudo-remort = max(0, (skill − SkillCapStart) / SkillCapIncrement)
   ```

   where `SkillCapStart` (the skill cap at 0 remorts, usually **80**) and `SkillCapIncrement` (the
   skill-cap increase per remort, usually **5**) are engine constants. So the rating grows off the
   **skill**, not the remort count: skill 80 → +0, skill 85 → +1, skill 90 → +2, and so on.

   Example: a level-30 character with skill 80 → rating = 30 + 0 + 4 = 34; with `min_rating=32` he
   passes the gate and can raise the top tiers.

After the tier is chosen, the budget check fires (`FitUndeadTier`, the step from section 3): if the
chosen tier does not fit the budget, it is **demoted** to the nearest tier that does; if even the
weakest does not fit, the raise fails.

### 6. Example: why a "skeleton", not a rejection

Character "Чернок", level 10, skill `kDarkMagic` = 10, raising a level-4 corpse:

1. Band: a level-4 corpse ≤ the skeleton's `corpse_max_level=5` → **skeleton** chosen.
2. Gate: rating = 10 + max(0, (10−80)/5) + 4 = 10 + 0 + 4 = 14 ≥ `min_rating=0` → skeleton stays.
3. Budget: `min(10, 75) − 0 = 10`; skeleton weight `8 ≤ 10` → the skeleton is raised.

Чернок cannot hold a second skeleton: the remaining budget `10 − 8 = 2 < 8` → "Вы не сможете управлять
столькими последователями." (Had the skeleton weight been `12`, as in the first version, Чернок could
not have raised even one — hence the "hidden entry floor" rule from section 3.)

### 7. Stat scaling — the `<scaling>` block

A raised creature's stats improve relative to the **prototype** in proportion to the spell's
**cast competence** `C` (`SetupUndeadStats`). Each `<scaling>` line is one stat with its own
parameters:

| Attribute | Meaning |
|---|---|
| `beta` | Scaling coefficient. `0` = the stat is unchanged (taken from the prototype). |
| `mode` | `add` — an addition; `mult` — a multiplier (see the formulas below). |
| `cap` | A bound on the result: a ceiling for "growing" stats, a floor for "shrinking" ones (AC, saves). Optional. |

Scaled stats: `hp`, `damage_dice` (number of damage dice), `hitroll`, `ac`, `skills` (all of the
prototype's skills, uniformly), `saving` (all 4 saves), `armor`, `morale`, `initiative`. There is no
`luck` on purpose — mobs do not have it.

**Formulas.** Let `C` be the competence (see section 8):

- "Growing" stats (`hitroll`, `armor`, `morale`, `initiative`, `skills`, `damage_dice`), `mode="add"`:
  `value = prototype + round(beta · C)`, then bounded above by `cap`.
- "Shrinking" stats (`ac`, `saving`), where lower = better, `mode="add"`:
  `value = prototype − round(beta · C)`, then bounded below by `cap`.
- Health `hp`, `mode="mult"`: `value = prototype × (1 + beta · C)` (not above `cap` if set).
  The multiplier preserves the proportions between tiers.
- `damage_dice` is additionally clamped to `1…100` (guarding the signed `damnodice`).

The config's seed values reproduce the historical behaviour: `hp beta=0.25 mode=mult` (+25% · C to
health) and `damage_dice beta=1.5 mode=add cap=100`. The other seven stats ship at `beta=0` (off) —
enable and tune them as you like.

### 8. What the competence `C` is

`C` is the same "cast power" as for other spells: the sum of the skill contribution and the key-stat
contribution (for undead that stat is Wisdom):

```
C = skill_coeff + stat_coeff
skill_coeff = ( min(skill, 75)·low_bonus + max(0, skill−75)·hi_bonus ) / 100
stat_coeff  = max(0, wisdom − threshold) · stat_weight / 100
```

The coefficients `low_bonus`, `hi_bonus`, `threshold` and `stat_weight` come from the `<potency_roll>`
block of the `kAnimateDead` spell in `cfg/…/spells.xml` — i.e. the "steepness" of the scaling is tuned
there, while `beta` in `animate_dead.xml` only sets how strongly a given stat reacts to `C`.

In practice: `C` grows with the Dark Magic skill and Wisdom, and for an experienced necromancer it is
on the order of single digits. Because the exact value depends on `<potency_roll>`, tune `beta`
**empirically**: set a value, raise a creature with casters of differing power, and inspect the
resulting stats (`vstat`/`stat` on the mob).

### 9. Practical recipes

- **Add a new tier.** `vedun creature new` creates a record with the first free `vnum`; fill in `id`,
  `proto_vnum`, `weight`, the `<cost>` block and `<scaling>`. Mind the ladder order: the tier must sit
  between its neighbours by strength.
- **Allow an earlier raise of a strong tier.** Lower its `min_rating` and/or `weight`.
- **Limit the "crowd".** Raise the tiers' `weight` or lower `budget_cap` — fewer creatures can be
  controlled. Keep the strongest tier's `weight` equal to `budget_cap` if you want "max skill = exactly
  one top creature".
- **Make a creature tougher/stronger.** Raise the `beta` of the needed stat; for "shrinking" stats (AC,
  saves) always set a `cap` as the floor, otherwise the value slides into the negatives without bound.
- **Widen/narrow the corpse-level band.** Change `corpse_max_level`: a tier is given to every corpse of
  level no higher than stated (and above the previous tier's `corpse_max_level`).

### 10. Common mistakes

- **Entry tier's weight above the starting skill** → a beginning necromancer raises nothing and sees
  "too many followers" although he has no undead. Keep the weakest tier's `weight` ≤ the skill expected
  of a newcomer.
- **`beta` on a "shrinking" stat without a `cap`** → AC or saves slide into an unbounded negative.
- **A broken ladder order** (`<creature>` not in ascending strength) → gate/budget demotion picks the
  "wrong" tier. List them weakest to strongest.
- **`--` inside an XML comment** → a strict parser rejects the file; use a single dash.
- **Editing `min_rating` for remorts** — unnecessary: the gate derives a pseudo-remort from the skill,
  remorts do not participate directly.

### 11. What stays in code (not in the config)

- The effect of ability feats (for example, "Fury of Darkness" / `kFuryOfDarkness`, which adds to
  undead stats) is applied in code on top of the config-driven scaling.
- The three-step selection itself (band → lot → gate) and the scaling formulas live in code
  (`src/gameplay/mechanics/animate_dead.cpp`); the config only supplies their **data**.

---

*This file is maintained by hand. When adding a new data-driven mechanic, give it its own section
following the same template: where it lives, how to reload it, the format, a parameter breakdown,
examples, common mistakes.*
