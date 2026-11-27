# BRusMUD Mechanics Setup Manual

This document describes the **data-driven** game mechanics of the engine: where their configuration
lives, how to edit and hot-reload it, and what each parameter means.

Engine rule: a configurable mechanic is described by a config under `cfg/mechanics/…`, loaded through
`CfgManager`, hot-reloaded with `reload …`, and (if it has a scheme) edited with the `vedun` editor —
the editor has its own manual, [VEDUN_RU.md](VEDUN_RU.md) (Russian only). Logic that is kept in code (for example, the effect of ability feats) is not moved into the config —
such places are called out separately.

Contents:

- [Animate Dead](#animate-dead)
- [Animate Skeleton (the warlock's bone retinue)](#animate-skeleton-the-warlocks-bone-retinue)
- [Piety: prayer, sacrifice and gifts](#piety-prayer-sacrifice-and-gifts)

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
            <hp           beta="0.25" mode="mult" cap="2.0" />
            <damage_dice  beta="1.5"  mode="add"  cap="25" />
            <damage_size  beta="0"    mode="add" />       <!-- die size (dN); 0 = off -->
            <damage_bonus beta="0"    mode="add" />       <!-- flat +B added to every hit (damroll) -->
            <hitroll      beta="0"    mode="add" />
            <ac           beta="0"    mode="add" />       <!-- lower AC = better; set cap= for a lower bound -->
            <skills       beta="0"    mode="add"  cap="200" />
            <saving       beta="0"    mode="add" />       <!-- all 4 saves; lower = better -->
            <armor        beta="0"    mode="add" />
            <morale       beta="0"    mode="add" />
            <initiative   beta="0"    mode="add" />
        </scaling>
    </creature>

    <!-- the remaining tiers, weakest to strongest ... -->
</animate_dead>
```

Tiers (`<creature>`) are listed **weakest to strongest** — this order is the "ladder" down which the
mechanic demotes the choice when a stronger tier is unavailable. The ladder is the order of the
records in the file, not their `vnum` numbers. Tiers with `spell="kAnimateSkeleton"` are not part of
the ladder — they are the "animate skeleton" kinds (next section).

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
| `vnum` | The tier's number in the config — the `vedun` key. It does not set the ladder order: the order of the records in the file does. |
| `id` | Symbolic tier name, e.g. `kSkeleton` (for lists and logs). |
| `proto_vnum` | VNUM of the mob prototype that gets raised. |
| `weight` | Cost of one such creature against the control budget. |
| `damroll_cap` | Optional. This kind's damroll ceiling in auto-attack. Absent or `0` — the generic minion ceiling: 50, or 100 for a resurrected corpse. |

The `spell`, `damage_share` and `corpse_min_level` (in `<cost>`) attributes are for the "animate
skeleton" kinds — they are covered in the next section.

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
   rating = level + max(remorts, pseudo-remort) + 4
   pseudo-remort = max(0, (skill − SkillCapStart) / SkillCapIncrement)
   ```

   where `SkillCapStart` (the skill cap at 0 remorts) and `SkillCapIncrement` (the skill-cap increase
   per remort) come from `cfg/remort.xml`, `<skill_cap start_cap="80" increment="5"/>`. The
   pseudo-remort grows off the skill: skill 80 → 0, skill 85 → 1, skill 90 → 2, and so on. The rating
   takes the larger of the two — the actual remort count and the pseudo-remort (issue #3992: before,
   only the pseudo-remort counted, and a high-remort character right after a skill reset raised only
   weak undead from any corpse).

   Example: a level-30 character with no remorts and skill 80 → rating = 30 + max(0, 0) + 4 = 34; with
   `min_rating=32` he passes the gate and can raise the top tiers.

After the tier is chosen, the budget check fires (`FitUndeadTier`, the step from section 3): if the
chosen tier does not fit the budget, it is **demoted** to the nearest tier that does; if even the
weakest does not fit, the raise fails.

### 6. Example: why a "skeleton", not a rejection

Character "Чернок", level 10, skill `kDarkMagic` = 10, raising a level-4 corpse:

1. Band: a level-4 corpse ≤ the skeleton's `corpse_max_level=5` → **skeleton** chosen.
2. Gate (no remorts): rating = 10 + max(0, max(0, (10−80)/5)) + 4 = 10 + 0 + 4 = 14 ≥
   `min_rating=0` → skeleton stays.
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
| `cap` | Bound on the result. **`cap="0"` or an absent cap = no cap** (unbounded). For `mode="mult"` the cap is the **maximum multiplier** (`value ≤ prototype × cap`); for `mode="add"` it is an absolute **ceiling** for "growing" stats and a **floor** for "shrinking" ones (AC, saves). |

Scaled stats:

- **`hp`** — max HP (`mult` keeps tier proportions).
- **Damage** — the three parts of the prototype's `NdM+B` roll are each separately scalable:
  - **`damage_dice`** — the *number* of dice (N, `damnodice`);
  - **`damage_size`** — the die *size* / magnitude (M, `damsizedice`);
  - **`damage_bonus`** — the flat *+B* added to every hit (the mob's damroll).
- **`hitroll`**, **`armor`**, **`morale`**, **`initiative`** — "growing" stats.
- **`ac`**, **`saving`** (all 4 saves) — "shrinking" stats (lower = better).
- **`skills`** — every skill the prototype already has, uniformly.

There is no `luck` on purpose — mobs do not have it.

`hp`, damage, `hitroll`, `ac` and `skills` are written straight into the raised mob
(`SetupUndeadStats`), while `armor`, `morale`, `initiative` and `saving` are granted as affects
(`ApplyVolatileUndeadStats`): a direct write to those fields is wiped by the first affect recalculation.
These affects are invisible to the player, cannot be removed, and last as long as the charm.

**Formulas.** Let `C` be the competence (see section 8):

- "Growing" stats (`hitroll`, `armor`, `morale`, `initiative`, `skills`, `damage_dice`, `damage_size`,
  `damage_bonus`), `mode="add"`: `value = prototype + round(beta · C)`, then bounded above by `cap`.
- "Shrinking" stats (`ac`, `saving`), where lower = better, `mode="add"`:
  `value = prototype − round(beta · C)`, then bounded below by `cap`.
- Health `hp`, `mode="mult"`: `value = prototype × min(1 + beta · C, cap)` — the cap limits the
  **multiplier** (e.g. `cap="2.0"` ⇒ never above 2× the prototype). It preserves tier proportions.
- `damage_dice` and `damage_size` are additionally clamped to `1…100` (guarding the signed bytes
  `damnodice` / `damsizedice`).

The config's seed values: `hp beta=0.25 mode=mult cap=2.0` (up to +25% · C, ceiling 2× the prototype —
≈ the pre-rework maximum) and `damage_dice beta=1.5 mode=add cap=25`. `damage_size`, `damage_bonus`
and the other combat stats ship **off** (`beta=0`) — enable and tune them as you like.

### 8. What the competence `C` is

`C` is the same "cast power" as for other spells: the sum of the skill contribution and the key-stat
contribution (for undead that stat is Wisdom):

```
C = skill_coeff + stat_coeff
skill_coeff = ( min(skill, 75)·low_bonus + max(0, skill−75)·hi_bonus ) / 100
stat_coeff  = max(0, wisdom − threshold) · stat_weight / 100
```

The coefficients come from the spell's `<potency_roll>` block in `cfg/spells.xml`: `low_bonus` and
`hi_bonus` are the `low_skill_bonus` and `hi_skill_bonus` attributes of `<base_skill>`, `threshold`
and `stat_weight` are the `threshold` and `weight` attributes of `<base_stat>`. For "raise corpse"
that is `kAnimateDead`; "animate skeleton" has its own `<potency_roll>` under `kAnimateSkeleton`. The
"steepness" of the scaling is tuned there, while `beta` in `animate_dead.xml` only sets how strongly a
given stat reacts to `C`.

In practice: `C` grows with the Dark Magic skill and Wisdom. With what both spells currently have in
`spells.xml` (`low_skill_bonus="0" hi_skill_bonus="8"`, Wisdom `threshold="14" weight="1"`),
`C ≈ 0.08 · (skill − 75) + 0.01 · (wisdom − 14)`: skill 100 gives about 2, skill 200 about 10.
Because the exact value depends on `<potency_roll>`, tune `beta`
**empirically**: set a value, raise a creature with casters of differing power, and inspect the
resulting stats (`vstat`/`stat` on the mob).

### 9. Practical recipes

- **Add a new tier.** `vedun creature <free vnum>` (e.g. `vedun creature 11`) creates a record; fill
  in `id`, `proto_vnum`, `weight`, the `<cost>` block and `<scaling>`. Mind the ladder order: `vedun`
  appends the record at the end of the file, so the new tier becomes the strongest. If it must sit
  between its neighbours by strength, move the record in the file by hand.
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
- **Computing `min_rating` from the skill alone** — wrong: the gate takes the larger of the actual
  remort count and the skill-derived pseudo-remort, so a character with remorts passes the gate even
  with a low skill.

### 11. What stays in code (not in the config)

- The effect of feats on raised undead: `kLoyalAssist` gives the skeleton the rescue skill at 100,
  `kHauntingSpirit` gives the bone spirit rescue at 120, `kFavorOfDarkness` cancels a failed raise
  three times out of four. "Fury of Darkness" (`kFuryOfDarkness`, +20% damroll, hitroll and HP)
  applies only to the corpse revived by "оживить труп" (resurrection), which the config does not
  scale.
- The undead's ice shield at effective Wisdom 75+ (`EnhanceAnimateDead` in
  `src/gameplay/magic/action_summon.cpp`).
- The three-step selection itself (band → lot → gate) and the scaling formulas live in code
  (`src/gameplay/mechanics/animate_dead.cpp`); the config only supplies their **data**.

---

## Animate Skeleton (the warlock's bone retinue)

The warlock's third corpse spell (after "raise corpse" and "оживить труп"/resurrection): **"animate
skeleton"** ("оживить скелет") raises a bone servant from a corpse. With the **"bone commander"**
feat ("костяной полководец") three special kinds become available, and the player names the kind:
`колдовать 'оживить скелет' труп свирепого` ("cast 'animate skeleton' corpse fierce"). Without the
feat, from a corpse below the threshold, or from a non-humanoid corpse, an ordinary skeleton rises.

The kinds are described in the same `animate_dead.xml`, but as **a separate list**: they carry
`spell="kAnimateSkeleton"`, and the "raise corpse" ladder never picks them.

### 1. Where things live

| What is tuned | File |
|---|---|
| Skeleton kinds: dice, HP, corpse threshold | `cfg/mechanics/animate_dead.xml` (tiers with `spell="kAnimateSkeleton"`) |
| The retinue's gift affects and their thresholds | `cfg/mechanics/animate_dead.xml`, the `<grants>` block |
| The spell itself: mana, element, target, handler | `cfg/spells.xml`, `<spell id="kAnimateSkeleton">` |
| Spell circle and level, feat circle | `cfg/classes/pc_necromancer.xml` |
| The "bone commander" feat | `cfg/feats.xml`, `<feat id="kBoneCommander">` |
| Russian names and messages | `cfg/messages/ru/spell_msg.xml`, `feat_msg.xml`, `affect_msg.xml` |
| The marker and the debuffs (affect types) | `cfg/affects/affects.xml` |
| Mob prototypes 3011–3013 | the world repository, `zones/30/mobs.yaml` |

Hot reload: `reload animatedead` picks up the kinds' numbers for **newly** raised skeletons. The other
files are reloaded by their own targets: `reload spells`, `reload classes`, `reload feats`,
`reload affects`, messages — `reload spellmsg` and `reload affectmsg`; `feat_msg.xml` has no target of
its own and is reloaded only by `reload all`.

### 2. A kind's tier — what each attribute means

```xml
<creature vnum="8" id="kFierceSkeleton" spell="kAnimateSkeleton" proto_vnum="3011"
          weight="0" damroll_cap="50">
    <cost corpse_min_level="26" corpse_max_level="127" min_rating="0" />
    <scaling>
        <hp          beta="0.12"  mode="mult" cap="4.0" />
        <damage_dice beta="0.624" mode="add"  cap="33" />
        <hitroll     beta="0.3"   mode="add"  cap="30" />
        <!-- everything else beta="0" -->
    </scaling>
</creature>
```

| Attribute | Meaning for skeletons |
|---|---|
| `spell` | Whose tier this is. `kAnimateSkeleton` takes the kind out of the "raise corpse" ladder; without the attribute the tier counts as undead (`kAnimateDead`). |
| `id` | The tier name. **Important:** the hit handler tells the kinds apart by it — `kFierceSkeleton` (claws), `kNimbleSkeleton` (lacerations), `kFetidSkeleton` (breath). Rename it and the kind loses its debuff. |
| `proto_vnum` | The mob prototype from the world. If it is missing from the world, an ordinary skeleton rises and the syslog gets `SYSERR: 'оживить скелет': в мире нет прототипов …`. |
| `weight` | **Keep it zero.** Skeletons do not spend the retinue budget: their number is limited differently — one of each kind. |
| `damroll_cap` | The kind's damroll ceiling in auto-attack (prototype + raise + "forces of evil" buffs). |
| `cost corpse_min_level` | Lower bound on the corpse level. A weaker corpse gives an ordinary skeleton. The attribute was introduced for skeletons: the undead ladder has only an upper bound. |
| `cost corpse_max_level` | Not checked for skeletons — keep `127`, i.e. no upper bound. |
| `cost min_rating` | Not checked for skeletons — keep `0`: the real limiter is the feat circle (see section 5). |
| `scaling` | Stat scaling off the competence `C` — the formulas and the meaning of `beta`/`mode`/`cap` are the same as for undead (sections 7 and 8 above). |

### 3. How to recompute a kind's damage

The shares are designed off the **black basilisk's total damage per round**. The basilisk has three
attacks (`extra_attack: 2` in the prototype) of `60d40` at the cap, i.e. on average

```
3 × 60 × 20.5 ≈ 3690 per round
```

Skeletons have two attacks per round (`extra_attack: 1`) and a `d6` die (average 3.5). So the needed
number of dice **per hit** is:

```
cap = share × 3690 / (2 × 3.5)
```

`beta` is chosen so that the kind reaches its `cap` at the same competence as the basilisk
(about 51):

```
beta = (cap − 1) / 51
```

Current values:

| kind | dice per hit | share of the basilisk | `beta` / `cap` |
|---|---|---|---|
| fierce | 33d6 | 6.3% | 0.624 / 33 |
| nimble | 26d6 | 4.9% | 0.487 / 26 |
| fetid | 20d6 | 3.8% | 0.370 / 20 |
| **together** | **79d6** | **15%** | |

The share does not depend on competence: all three `beta` values are proportional to the basilisk's
(`1.15`), so the ratio holds midway through the growth too, not only at the cap. A handy check: raise
the kind and look at `stat <kind> скелет` — the "NPC сила удара" line shows the current dice.

### 4. What the mob prototype must have

| Field | Why |
|---|---|
| `extra_attack: 1` | Two attacks per round. Change it and recompute `cap` with the formula above. |
| `damage: 1d6` | The base the dice grow from. The die size can be changed, but then `cap` changes too. |
| `bare_hand_attack` | The hit type in messages: `8` claws ("оцарапал"), `3` slash ("рубанул"), `4` bite. |
| `kGasBreath` in `action_flags` | Fetid only: its hit becomes a breath (magic damage instead of physical). |
| `race: 110` | Race `kZombie` (undead). Resistances and part of the mechanics depend on the race. |
| `kNoSkillTrain` | So that the retinue does not award skill-ups. The code also sets this flag itself on raise, so in the prototype it is a safety net. |

The `kBoneServant` marker does **not** go into the prototype — the code puts it on at raise. A flag in
`affect_flags` will not replace it: the engine looks for on-hit actions in the affect list, and the
flag only sets a bit and triggers nothing.

### 5. Availability: spell circle and feat circle

In `cfg/classes/pc_necromancer.xml`:

```xml
<spell id="kAnimateSkeleton" level="26" remort="0" circle="8" mem="0" cast="0.0"/>
<feat  id="kBoneCommander"   level="1"  remort="0" slot="6"  inborn="0"/>
```

- `circle` and `level` — when the spell becomes available by circles.
- A feat's `slot` is **the feat circle minus one**: `slot="6"` is shown as "Круг 7" (circle 7). The
  engine computes when it opens: `slot = level × (5 + remorts / remorts_for_slot) / 28`, where
  `remorts_for_slot` comes from `<feats remorts_for_slot="…">` in the same file. For the warlock it is
  `6`, so circle 7 opens at the sixth remort and level 28.

The player gets the spell itself not by circle but from the world — from a guild, a book or a trigger
(`mspellturn`). The feat is granted by the `mfeatturn` trigger.

### 6. The kinds' debuffs

What is in the config (`cfg/affects/affects.xml` and `messages/ru/affect_msg.xml`):

- `kBoneServant` — the kind marker. Its `<actions>` declare `<trigger val="kPostHit"/>` and the
  `SkeletonHit` handler. Remove the action and skeletons stop applying anything.
- `kCorrodedArmor` ("разъеденный доспех", corroded armor) and `kCorrodedWard` ("разъеденный оберег",
  corroded ward) — the debuffs themselves, flags `kAfBattledec|kAfCurable` (duration in combat rounds,
  curable).
- Lacerations reuse the existing `kLacerations`, the switch ban — the existing `kNoBattleSwitch`.

What is still **in code** (`src/gameplay/handlers/bone_commander.cpp`):

| Value | Setting |
|---|---|
| Resist shred step | 1% per hit |
| Shred ceiling | 12% / 10% / 8% with one, two and three living skeletons |
| Laceration threshold for the switch ban | 4 / 6 / 8 lacerations with one, two and three skeletons |
| Debuff duration | 10 combat rounds, renewed by every hit |
| Against players | debuffs are not applied |

The strength is computed at the moment of application from the master's number of living skeletons:
if one dies, the others' debuff is stronger at once, the raised ones need no recalculation. If these
numbers ever need tuning without a rebuild, they can easily be moved into the same `animate_dead.xml`
as a separate block — for now they stay in code on purpose, until they settle.

### 7. The bone levy: undead weaken for the retinue

issue #4032: while the warlock has at least one special skeleton alive, his "raise corpse" undead lose
part of their damage — both physical and magic. The price is paid **once for the whole retinue**: the
sum of the living skeletons' shares is divided among all of the master's undead rather than taken from
each in full. Three skeletons with one undead — the same 15%, with two — 8% each, with three — 5%
each.

A kind's share is set on its tier with the `damage_share` attribute (percent):

| kind | `damage_share` |
|---|---|
| fierce | 6.3 |
| nimble | 4.9 |
| fetid | 3.8 |
| **all three** | **15** |

This is a separate number, not derived from the dice: the dice are computed from the cast competence
and the prototype's die, and a reverse calculation would be fragile. Change the dice — change the share
too, or the price will drift away from the strength.

The penalty is held by the **"bone levy"** affect ("костяная подать", `kBoneLevy`) on the undead
itself: one line each on `kPhysicDamagePercent` and `kMagicDamagePercent`, the duration does not tick
(`kAfDeadkeep`) — the code sets and removes it. Recalculation happens at three points: a skeleton was
raised, an undead was raised (it arrives already weakened), and someone left the retinue — died, was
released, or the charm ran out. An undead leaving counts too: the divisor changes and the levy on the
rest grows. On a change the room gets the line "$n поник$q, отдав часть своей силы костяной свите."
("$n droops, giving part of its strength to the bone retinue.") or "$n расправил$u: костяная свита
убыла, и сила вернулась." ("$n straightens: the bone retinue has shrunk, and the strength has
returned.").

The resurrected corpse from "оживить труп" is not affected: the levy falls only on tiers with
`spell="kAnimateDead"`, and the resurrected corpse is raised by the corpse's vnum and is not among the
tiers.

### 8. Gift affects: the retinue grows stronger with its master

issue #4038: the higher the Dark Magic skill, the larger the set of affects a skeleton rises with. The
affects are gifts — no mana, no separate cast: once the caster crosses the threshold, the raised one
arrives already carrying them. They are part of the undead package (`kAfCharmBond`), so they cannot be
removed and last as long as the charm.

The thresholds live in `cfg/mechanics/animate_dead.xml` as a separate block:

```xml
<grants>
    <grant spell="kAnimateSkeleton" min_skill="100" affect="kDetectInvisible" />
    <grant spell="kAnimateSkeleton" min_skill="100" affect="kInfravision" />
    <grant spell="kAnimateSkeleton" min_skill="120" affect="kFly" />
    <grant spell="kAnimateSkeleton" min_skill="140" affect="kDetectLife" />
    <grant spell="kAnimateSkeleton" min_skill="200" affect="kIceShield" />
</grants>
```

| Attribute | Meaning |
|---|---|
| `spell` | Whose gift this is. The threshold names the spell, not the tier, so "animate skeleton" gives the affect to an ordinary skeleton too. |
| `min_skill` | The threshold, in percent, of the skill from `<control>` (Dark Magic). Without the attribute the affect goes to any caster. |
| `affect` | The affect type as named in `cfg/affects/affects.xml`. |

Current values:

| Dark Magic | What the skeleton gets |
|---|---|
| 100% | detect invisible, infravision |
| 120% | fly |
| 140% | detect life |
| 200% | ice shield |

A line without `spell` or without `affect`, as well as one with an unknown name, is logged to the
syslog as `animate_dead grant …` and skipped — the others keep working. No block at all means no gift
affects, and that is not an error. Picked up by `reload animatedead` for **newly** raised ones.

The block is shared by both corpse spells: write `spell="kAnimateDead"` and the same gift goes to the
undead from "raise corpse". There are no lines for it now — its ice shield still comes from Wisdom
(75+) and lives in code.

### 9. How to check it as a god

```
featset <name> 'костяной полководец' 1      # otherwise an ordinary skeleton rises
skillset <name> 'магия тьмы' 300            # cast competence: dice and HP grow from it
load mob 10000                              # Solovei the Robber: level 29, humanoid
slay соловей
колдовать 'оживить скелет' труп             # a prompt listing the kinds
колдовать 'оживить скелет' труп свирепого   # the kind, in any grammatical case
stat свирепый скелет                        # dice, attacks, hit type, marker and gift affects
```

Pick a dry room with no aggressive mobs: the spell requires standing, and in combat you get
"Невозможно! Вы сражаетесь!" ("Impossible! You are fighting!"). To watch the debuffs: `приказ все
убить <target>` ("order all kill <target>") and wait for the lines "Ядовитые когти … разъедают
доспехи …", "Смрадное дыхание … разъедает оберег …", "… затих, не в силах оторваться …".

### 10. Pitfalls

- **`purge` in the room kills your own skeletons too**, and `goto` moves the god **without the
  retinue** — raise them where you will fight.
- `stat <name>` searches the whole world: take a victim mob with a unique name, or you will see
  someone else's stats.
- Raising the same kind again is refused: "У вас уже служит …" ("You already have … serving"). First
  `отпустить все` ("release all").
- Renaming a tier's `id` silently disables that kind's debuff — the handler tells the kinds apart by
  `id`.
- A `weight` above zero makes skeletons spend the retinue budget on a par with undead, and the player
  starts hitting the unrelated message "Вы не сможете управлять столькими последователями" ("You cannot
  control that many followers").

## Piety: prayer, sacrifice and gifts

A heathen **sacrifices** to the gods, a Christian **prays** to the saints. Every accepted offering
lays the favour of a god or saint, and the favour's strength comes from the hidden skill **piety**
(`kReligion`). Piety itself grows only from a **worthy gift**: equipment, treasure or a currency.
What is accepted and which gift is worthy is set in `cfg/mechanics/piety.xml`.

### 1. Where it lives

| What | Where |
|---|---|
| What the commands accept, how a gift is valued, growth thresholds | `cfg/mechanics/piety.xml` |
| Scheme for `vedun` | `cfg/mechanics/piety.scheme` |
| Gods and saints, duration, piety and charisma weights | `cfg/spells.xml`, `<spell id="kOffering">` and `<spell id="kPrayer">` |
| Favours (what they add, `beta`) | `cfg/affects/affects.xml`, `kYariloFavor` … `kGeorgeGrace` |
| The skill and its flags | `cfg/skills.xml`, `<skill id="kReligion">`; listed by every class in `classes/pc_*.xml` |
| Messages | `cfg/messages/ru/spell_msg.xml`, sheaves `kOffering` and `kPrayer` |
| New rules or the old ones | `piety::kValuedOfferings` in `src/gameplay/mechanics/piety.h` (rebuild needed) |

| | |
|---|---|
| `CfgManager` id | `piety` |
| Hot reload | `reload piety` |
| In-game editing | `vedun piety` (list), `vedun piety sacrifice` / `prayer` / `growth` |
| Loaded at boot | `MUD::CfgManager().LoadCfg("piety")` in `db.cpp` |

Without `piety.xml` the server boots, but nothing can be offered except a prayer without a gift, and
piety does not grow.

### 2. What the player sees

```
жертвовать <whom> <what>        жертвовать перуну меч
                                жертвовать велесу 200 кун
молиться <whom> [<what>]        молиться николе
                                молиться николе перчатки
```

- `<whom>` is a god or saint: Yarilo, Makosh, Perun, Veles for a heathen; the Theotokos, St Nicholas,
  Elijah the Prophet, St George for a Christian. A word prefix is enough. Without a name the command
  lists the choices.
- `<what>` is an item from the inventory or `<number> <currency>`. The currency is found by the
  currency search; kunas also answer to `кун`, `coin`, `денег`.
- A prayer without a gift is a candle: it costs `default_amount` of `default_currency` (10 kunas now).
- Sacrifice only in a `kForPoly` room, pray only in a `kForMono` room. The other religion's command is
  refused ("Не кощунствуйте!").
- One cooldown: the `kReligion` skill timer, 12 hours (12 real minutes). While the `kPietas` or
  `kPrayerful` marker is on, another offering is refused. Immortals skip the cooldown.
- The player is not told that piety grew: the skill is hidden.

### 3. The favour: strength from piety

An offering is the service spell `kOffering` or `kPrayer`. Its first action lays the marker, the
chosen one lays that god's favour (see `choice` in the spell manual). Strength:

```
C = piety / 100 + (charisma − 10) × 0.03
bonus = ceil(beta × C)          no cap
```

| Whom | Favour | Gives |
|---|---|---|
| Yarilo | `kYariloFavor` | infravision |
| Makosh | `kMakoshFavor` | HP regeneration, % |
| Perun | `kPerunFavor` | strength |
| Veles | `kVelesFavor` | dexterity |
| Theotokos | `kTheotokosGrace` | morale |
| St Nicholas | `kNicholasGrace` | intelligence |
| Elijah | `kElijahGrace` | hitroll |
| St George | `kGeorgeGrace` | damroll |

The favour **does not depend on the gift's value**: a candle and a sword give the same favour. The
value only decides whether piety grows.

### 4. Piety growth

Every class has piety (`kObligatory`); it is set to 10 on first login and survives remort
(`kKeepOnRemort`). It grows by **1 per offering**, with no dice roll, when all of these hold:

1. the spell took effect (the favour was laid);
2. piety is above 0 and below the skill cap: by wisdom and `80 + 5 × remorts`;
3. the skill may train in this room (`kTrainInPeaceful`: a peaceful room counts, and almost all
   temples are peaceful);
4. **the gift is worthy**: its value is at least the threshold for current piety (section 6).

### 5. `piety.xml`: what is accepted

```xml
<piety>
	<sacrifice>
		<items types="kFood" value="none"/>
		<items types="kTreasure" value="cost"/>
		<items types="kArmor|kLightArmor|kMediumArmor|kHeavyArmor|kWeapon|kWorm" value="ilvl"/>
		<currency id="kKuna" min="10"/>
	</sacrifice>
	<prayer default_currency="kKuna" default_amount="10">
		<items types="kTreasure" value="cost"/>
		<items types="kArmor|kLightArmor|kMediumArmor|kHeavyArmor|kWeapon|kWorm" value="ilvl"/>
		<currency id="kKuna" min="10"/>
	</prayer>
	<growth fresh_timer="80"> ... </growth>
</piety>
```

`<sacrifice>` is the "жертвовать" command, `<prayer>` is "молиться". Inside:

| Element / attribute | Meaning |
|---|---|
| `<items types="…" value="…"/>` | Item types (`EObjType`, `\|`-separated) and how they are valued. A type not listed is refused. |
| `value="none"` | Accepted and lays the favour, but never raises piety (food). |
| `value="ilvl"` | Item level: the item's stats, weighted and summed (section 7). |
| `value="cost"` | The item's cost (treasure). |
| `<currency id="…" min="N"/>` | A currency by `text_id` from `economics/currencies.xml` and the least amount; without `min`, 1. A currency not listed is refused. |
| `default_currency`, `default_amount` | The price of an offering without a gift (the candle). Without them a gift is required. The currency must be among the `<currency>` entries. |

Always refused: items flagged `kNodrop` or `kNodonate`, and non-empty containers.

### 6. `piety.xml`: growth thresholds

```xml
<growth fresh_timer="80">
	<tier piety="0" ilvl="5" cost="100">
		<currency id="kKuna" min="100"/>
	</tier>
	<tier piety="50" ilvl="10" cost="500">
		<currency id="kKuna" min="500"/>
		<currency id="kSilverGrivna" min="1"/>
	</tier>
	…
</growth>
```

- `<tier piety="P">` applies from piety `P` up to the next tier. File order does not matter: tiers
  are sorted by `piety`. Below the first tier piety never grows.
- `ilvl`, `cost`, `<currency id min>` are the thresholds per measure. A gift is worthy when its value
  is **at least** the threshold of its own measure.
- **A measure without a threshold on a tier never raises piety there.** That is how you say "kunas no
  longer count from here": if the tiers from `piety="100"` up have no `<currency id="kKuna">`, kunas
  stop raising piety at 100.
- `fresh_timer` is the share of the prototype's timer (percent) from which an item counts as unworn
  (section 7).

The shipped thresholds are a first guess, not yet checked against the world's actual item levels and
prices:

| Piety | `ilvl` | `cost` | kunas |
|---|---|---|---|
| 0 | 5 | 100 | 100 |
| 50 | 10 | 500 | 500 |
| 80 | 15 | 2000 | 2000 |
| 100 | 20 | 5000 | 5000 |
| 130 | 25 | 15000 | 15000 |
| 160 | 30 | 40000 | 40000 |
| 200 | 35 | 100000 | 100000 |
| 230 | 40 | 200000 | 200000 |

The 10-kuna candle does not reach even the first tier: it lays the favour but does not raise piety.

### 7. How an item is valued

- **`ilvl`** is `CalcRemortRequirements` of **the item itself**, not the prototype's cached `ilevel`:
  enchantments and crafted stats count, and an item whose remort requirement a builder set by hand is
  not zeroed.
  - Mob items and set items are valued at 0: favour yes, piety no.
  - An item with a stat listed twice (the function returns 1000000) is refused.
- **`cost`** is the instance's cost (`get_cost`).
- **Timer.** While the item's timer is at least `fresh_timer` of the prototype's, the value is full.
  Below that it falls in proportion: with `fresh_timer="80"` an item at 40% of its timer is worth half,
  at 0% nothing. Otherwise players would wear an item to its last hour and only then offer it.
  Indestructible items, items without a timer and items without a prototype keep their value.
- **A currency** is valued at its amount.

### 8. Recipes

**Add a currency.** Add `<currency id="kSilverGrivna" min="1"/>` to `<sacrifice>` and/or `<prayer>`
(or it is refused) and to the tiers where it should raise piety (or it is accepted for the favour
only).

**Kunas for beginners, a gold sink later.** Keep the kuna thresholds moderate on the low tiers and
steep on the high ones. If kunas must not raise piety above some level at all, drop their
`<currency>` from the higher tiers; they can still be accepted for the favour.

**No candle for Christians.** Remove `default_currency` and `default_amount` from `<prayer>`: a prayer
without a gift is then refused with "Вы должны пожертвовать что-то стоящее".

**Accept a new item type.** Add it to `types` of the right `<items>`, or add a separate `<items>` with
its own measure.

After editing, `reload piety`. Editing through `vedun piety …` dry-runs the loader before saving.

### 9. Vedun

The `piety` data set has three elements, named after the sections: `sacrifice`, `prayer`, `growth`.
Item types (`EObjType`) and the measure (`EPietyMeasure`: `none` / `ilvl` / `cost`) are picked from a
list, currencies from `ECurrencyId`. Saving is refused when:

- `types` has an unknown item type;
- `default_currency` is not among the accepted currencies;
- a tier has no `piety=`, or a number is not a number.

To give a god this data set, their `<vedun list=...>` in `privilege.xml` needs `piety` (or `*`).

### 10. Checking it as a god

- Testers and coders see on every offering
  `Piety: skill=<piety>, gift=<gift value>, threshold=<threshold or none>`, which shows at once why
  piety did or did not grow.
- Piety is hidden from players only: a god sees it in their own `умения`.
- An immortal skips the cooldown and can offer again and again. Their piety grows by the same rules,
  which is handy for checking thresholds; mortals cannot do this.
- Twelve game hours are 12 real minutes.

### 11. The old rules

`constexpr bool kValuedOfferings{true}` in `piety.h` turns gifts on. Set it to `false` and rebuild to
get the old way back:

- a heathen offers food or treasure, a Christian lights a 10-kuna candle;
- piety grows from every offering by the usual skill roll, but not above `kLegacyGrowthLimit = 35`.

At piety 35 and charisma 14 the favour roughly equals the old fixed table: +1 strength, +2 hitroll,
+5 morale, +47% regeneration. In this mode the commands do not use `piety.xml`.

### 12. Pitfalls

- **The prayer message breaks into two lines.** Not a bug: `act` wraps text to the player's screen
  width, `режим ширина` (80 by default, or the client's window width if it reports one). A line such
  as "Вы пожертвовали перчатки позорного мастерства на храм и вознесли молитву Николе-Угоднику." is 89
  characters. Use `режим ширина 120`, or turn `режим перенос строк` off.
- Tiers without `piety="0"`: a character below the first tier never grows. Keep the first tier at 0.
- A threshold for a measure that `<sacrifice>`/`<prayer>` does not accept is useless: such a gift is
  simply refused.
- A parse error (a non-number in an attribute) on `reload piety` is logged and the previous rules
  stay; at boot they stay empty: nothing is accepted and nothing grows. Edit through Vedun: it will
  not save such a file.
- Move the new `piety.xml`/`piety.scheme` to the server together with the binary
  (`tools/migrate_cfg_2689_religion.py` copies them if they are missing).

---

*This file is maintained by hand. When adding a new data-driven mechanic, give it its own section
following the same template: where it lives, how to reload it, the format, a parameter breakdown,
examples, common mistakes.*
