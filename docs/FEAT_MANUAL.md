# Bylins MUD — Data-Driven Feat Manual

*For game designers authoring feats (perks) in `lib/cfg/feats.xml`.*

A **feat** (`EFeat`) is a passive character trait: it can grant stat bonuses and
status affects, tune skill/feat cooldowns, and — the newer capability — **modify
the behaviour of spells and affects** the character casts or carries. Everything
in this manual is data; the feat's *availability* (which class gets it, at what
cost) lives with the classes, and a handful of feats still have hard-coded bodies
(noted in §6).

Two independent data layers:

* **`<talent_effects>`** — the *passive* layer (original): stat/affect grants and
  skill/feat cooldown tweaks that apply simply because the character has the feat.
* **`<talent_patches>`** — the *active* layer (newer, "perk-action-patching"): the
  feat edits a spell's or an affect's `<action>` chain when the holder casts it or
  carries it.

This manual assumes the shared action vocabulary of the
[Spell Manual](SPELL_MANUAL.md) (`<action>`, `<damage>`, `<points>`, `<affects>`,
…) and the [Affect Manual](AFFECT_MANUAL.md) (triggers, affect-owned actions).
The `talent_patch` payload *is* that vocabulary; this manual documents how a feat
targets and edits it.

---

## 1. The `<feat>` element

```xml
<feat id="kDodger" mode="kEnabled">
    <talent_effects> … </talent_effects>     <!-- passive layer, §3 -->
    <talent_patches> … </talent_patches>     <!-- active layer,  §4 -->
</feat>
```

### 1.1 Header attributes

| attr | meaning |
|---|---|
| `id` | the `EFeat` enum name. Required. |
| `mode` | lifecycle state (`EItemMode`): `kEnabled` (live), `kTesting` (dev/prototyping only), `kDisabled`, `kFrozen`, `kService` (engine-internal). Only `kEnabled` feats are generally usable in play; `kTesting` is for iterating on new perks without shipping them. |

A feat may have an **empty body** (`<feat id="kBerserker" mode="kEnabled"/>`) — a
pure marker the combat/skill code tests via `CanUseFeat`. Adding data later never
breaks that.

---

## 2. Availability & gating (how a character "has" a feat)

Feat data does **not** declare its own requirements. The commented
`<requirements>` block at the top of `feats.xml` is an aspirational format and is
**not parsed**. Availability is decided elsewhere, and `CanUseFeat(ch, feat)`
(the single gate all game code uses) checks, in order:

1. the feat exists and the character **has** it (`HaveFeat`);
2. NPCs pass immediately (mobs use any feat they're given);
3. the character's **feat-slot budget** covers the feat's slot cost —
   `CalcMaxFeatSlotPerLvl(ch) ≥ MUD::Class(class).feats[feat].GetSlot()`;
4. the character's **remort** meets the feat's minimum —
   `remort ≥ …feats[feat].GetMinRemort()`;
5. a few feats add a **hard-coded stat gate** (e.g. `kWeaponFinesse`/`kFInesseShot`
   need `dex > str` and `dex > 17`; `kPowerAttack` needs `str > 19`).

The per-class **slot cost** and **min-remort** live in the class configs
(`pc_*.xml`), not here. So to make a feat available you (a) add its `EFeat`,
(b) give it data here, and (c) grant it to the relevant class(es) with a slot/
remort cost in the class config.

---

## 3. `<talent_effects>` — the passive layer

Container for effects that apply while the character has the feat. Children:

### 3.1 `<applies>` — passive stat / affect grants

```xml
<applies>
    <apply location="kMagicResist" affect="kUndefinded" val="1"
           lvl_bonus="0.0" remort_bonus="1.0" cap="10"/>
    <apply location="kNone" affect="kInfravision" val="0"
           lvl_bonus="0.0" remort_bonus="0.0" cap="0"/>
</applies>
```

| `<apply>` attr | meaning |
|---|---|
| `location` | `EApply` — the stat/derived value to change (`kMagicResist`, `kSavingReflex`, `kSpelledBlinkPhys`, …). `kNone` = grant only the `affect` flag, no numeric change. |
| `affect` | `EAffect` flag to grant alongside (e.g. `kInfravision`, `kBlink`). `kUndefinded` (sic) = numeric change only, no flag. |
| `val` | base modifier. |
| `lvl_bonus` | per-**level** growth added to `val`. |
| `remort_bonus` | per-**remort** growth added to `val`. |
| `cap` | clamp: `cap > 0` upper-bounds, `cap < 0` lower-bounds, `0` = no clamp. |

**Formula** (`Applies::Impose`):

```
mod = val ± (lvl_bonus·level + remort_bonus·remort)     // sign: '-' for a "negative" EApply
                                                        //       (e.g. saving throws, where lower = better)
mod = clamp(mod, cap)                                   // cap>0 → min; cap<0 → max
affect_modify(ch, location, mod, affect)                // applies the number AND grants the affect flag
```

For "negative" apply locations (savings, where a lower target number is better)
the level/remort bonus is **subtracted**, so a `remort_bonus` *improves* the save.
This is the built-in way to grant a scaling passive (e.g. `kDodger` gives blink +
scaling saving-throw bonuses; `kImpregnable` gives magic/affect resist scaling
with remort, capped at 10; `kNightVision` grants the `kInfravision` affect).

> **Note.** These feat `<apply>` grammar (flat `val`/`lvl_bonus`/`remort_bonus`)
> differs from the *affect* `<apply>` grammar (nested `<modifier>` with
> `beta·competence`, Affect Manual §4.2). Feats scale with **level/remort**;
> affects scale with the caster's **competence**. Don't mix them up.

### 3.2 `<skill_mods>` — flat skill bonuses

```xml
<skill_mods>
    <mod id="kRescue" val="20"/>
</skill_mods>
```
Adds `val` to the character's effective skill (`SetAddSkill`).

### 3.3 `<skill_timer_mods>` / `<feat_timer_mods>` — cooldown tweaks

```xml
<skill_timer_mods><mod id="kZoneName" val="-2"/></skill_timer_mods>
<feat_timer_mods><mod id="kRelocate" val="-1"/></feat_timer_mods>
```
Adjust the cooldown timer of a named skill / feat while the feat is held (read via
`GetTimerMod`).

---

## 4. `<talent_patches>` — modify spells & affects *(the active layer)*

A `<talent_patch>` edits the `<action>` chain of a **spell** (Spell Manual §3.8)
or an **affect** (Affect Manual §5) when the feat's holder casts/carries it. It is
entirely data — the same action vocabulary as spells/affects, plus a *selector*
(which spell/affect), a *scope/relative* (whose feat is checked, who the added
action targets), and an *op* (how to edit the chain).

**How it runs.** At boot, `BuildTalentPatchIndex` resolves every patch's selector
to concrete spells (or buckets it by affect type) and validates its ids. At cast/
trigger time the target's patch list is applied, each gated on
`CanUseFeat(holder, feat)`. Cost is one `.empty()` test on unpatched spells; a
broad selector fans out at boot only (no per-cast cost). Mis-scoped patches are
logged loudly at boot (matched 0 spells, missing action id, absent manifestation).

```xml
<talent_patches>
    <talent_patch spell="kBurningHands" op="replace" action="Main" effect="MainDamage">
        <action> … replacement … </action>
    </talent_patch>
</talent_patches>
```

### 4.1 Selectors — *which* spell(s) / affect

All present selectors are **AND-combined**; an absent one is vacuously true.

| selector | matches |
|---|---|
| `spell="kX"` | one specific spell |
| `element="kFire"` | every spell with that `EElement` |
| `base_skill="kFireMagic"` | every spell whose potency/success base skill matches |
| `flag="kMagDamage"` | every spell with that `ESpellFlag` |
| `category="curse"` | every spell carrying that semantic tag |
| `has_effect="kArea"` | every spell whose action chain contains that manifestation kind |
| `all="true"` | every spell (guarded — use sparingly) |
| `affect="kVampirism"` | **an affect** instead of a spell — the patch edits that affect's action chain (Affect Manual §5). Bucketed by affect type at boot. |

### 4.2 Scope & relative — *whose* feat, *whose* target

* `by="target"` — check/apply on the **target's** feats instead of the caster's
  (default is the caster). (Scope `kCaster`/`kTarget`.)
* `relative="self|master|group_leader"` — *(affect patches)* whose feat gates the
  patch relative to the affect's bearer: `self` (default), `master` (the bearer's
  charm master), `group_leader`. This is how `kSoulLink` lets a **minion's**
  vampirism heal its **master** — the patch is on `kVampirism`, gated on the
  master's feat.
* The added action can target `kTarMaster` (Affect Manual §5.3) to land on that
  master.

### 4.3 Ops — *how* to edit the chain

| `op` | effect | needs |
|---|---|---|
| `append` *(default)* | add the `<action>` payload after the chain | payload |
| `insert` | insert the payload at an anchor | `before="id"` / `after="id"`; payload |
| `replace` | replace a block (or one manifestation in it) with the payload | `action="id"` (+ optional `effect="id"`); payload |
| `remove` | delete a block (or one manifestation) | `action="id"` (+ optional `effect="id"`) |
| `add_effect` | merge the payload's manifestations into a named block | `action="id"`; payload |
| `replace_all` | clear the chain and install the payload | payload |
| `modify` | scale/offset/set a numeric field of a manifestation | `effect="kKind"` + `<modify>` |

**Id addressing.** `action="…"` is a block's `id`; `effect="…"` is a manifestation
`id` within a block (for `replace`/`remove`/`add_effect`), **or** — for `modify` —
the manifestation **kind** (`kDamage`/`kArea`/`kPoints`/`kAffect`). Give the target
spell/affect's `<action>`/`<tag>` stable `id=` attributes so id-based ops can find
them.

**`modify`** scales one field:

```xml
<talent_patch has_effect="kArea" op="modify" effect="kArea">
    <modify field="decay" mul="0.6"/>     <!-- mul= / add= / set= -->
</talent_patch>
```
Fields are the manifestation's numeric members (`Area`: `decay`/`free_targets`/
`max_targets`/…; `Damage`: `dices_weight`/`beta`/`min`/…). `mul`/`add`/`set` pick
the arithmetic.

### 4.4 The `<action>` payload

For `append`/`insert`/`replace`/`replace_all`/`add_effect`, the `<action>` child
is an ordinary action block — same grammar as a spell's `<talent_actions>`
(Spell Manual §3.8) or an affect's `<actions>` (Affect Manual §5). It can carry a
`<trigger>` (for affect patches), a `target=` (incl. `kTarMaster`), and any
manifestation (`<damage>`, `<points><heal>`, `<affects>`, `<side_spell>`, …).

---

## 5. Worked examples (shipping / test)

* **Passive resist (scaling), `kImpregnable`:** `<applies>` of `kMagicResist` +
  `kAffectResist`, `remort_bonus="1.0" cap="10"` — +1 per remort, capped at 10.
* **Passive affect grant, `kNightVision`:** one `<apply location="kNone"
  affect="kInfravision"/>` — grants the affect flag, no number.
* **Passive number + affect, `kDodger`:** blink (`kSpelledBlinkPhys`, affect
  `kBlink`) + scaling saving-throw bonuses.
* **Affect patch (relative), `kSoulLink`:** `<talent_patch affect="kVampirism"
  relative="master" op="append"><action target="kTarMaster" …><trigger
  val="kPostHit"/><points><heal beta="0.05"/></points></action>` — a minion's
  vampirism also heals its master. Gated on the *master's* feat.
* **Spell replace, `kEmberFury`:** `<talent_patch spell="kBurningHands"
  op="replace" action="Main" effect="MainDamage">` — swap a spell's damage block.
* **Class selector append, `kPyromancer`:** `<talent_patch element="kFire"
  flag="kMagDamage" op="append">` — add an effect to every fire damage spell.
* **Modify, area feat:** `<talent_patch has_effect="kArea" op="modify"
  effect="kArea"><modify field="decay" mul="0.6"/>` — soften area falloff on every
  area spell.

---

## 6. What is *not* (yet) data

* **Hard-coded feat bodies.** Many feats are still checked directly in C++
  (combat maneuvers, movement/perception, trade/craft gates, summon buffs). These
  are effectively separate abilities and need the ability pipeline before they can
  be data-driven — they are **not** `talent_effects`/`talent_patch` candidates.
* **A few spell modifiers** (e.g. `kPowerMagic`'s ignore-absorb-vs-NPC flag,
  summon-buff feats like `kFuryOfDarkness`) touch spell *flags* / *summoned mobs* —
  shapes the current `talent_patch` grammar (numeric field edits + action-chain
  ops) does not yet express. Their purely-numeric parts already live in
  `<talent_effects>` (e.g. `kPowerMagic`'s +50% magic damage).

---

## 7. Reference

* **`EItemMode`** — `kEnabled`/`kTesting`/`kDisabled`/`kFrozen`/`kService`.
* **`EApply`** (apply `location=`) — `entities_constants.h`; shared with spells/affects.
* **`EAffect`** (apply `affect=`) — [Affect Manual](AFFECT_MANUAL.md).
* **Patch ops** — `append`/`insert`/`replace`/`remove`/`add_effect`/`replace_all`/`modify`.
* **Patch selectors** — `spell`/`element`/`base_skill`/`flag`/`category`/`has_effect`/`all`/`affect`.
* **Action/manifestation grammar** — Spell Manual §3.8 / §5–§8; Affect Manual §5.

---

## 8. Checklist for adding / migrating a feat

1. **Add the `EFeat` id** and a `<feat id= mode=>` block (`kTesting` while
   iterating).
2. **Grant it to a class** in `pc_*.xml` with a slot cost + min-remort (this is
   what makes it obtainable).
3. **Passive stat/affect?** → `<talent_effects><applies>` (`val` + `lvl_bonus`/
   `remort_bonus`, `cap`, optional `affect=`).
4. **Skill/feat cooldown or skill bonus?** → `<skill_mods>` / `<skill_timer_mods>`
   / `<feat_timer_mods>`.
5. **Modifies a spell/affect?** → `<talent_patches>`: pick a selector (specific or
   class-wide), an op, and (for affect patches) a `relative`; give the target
   stable `id=`s if you use id-based ops.
6. **Flip `mode="kEnabled"`** when ready; verify at boot (no patch warnings) and in
   mud-sim (grant the feat, cast/trigger the affected spell/affect, confirm the
   effect applies to holders and not to non-holders).

---

*Companion documents: [Spell Manual](SPELL_MANUAL.md) · [Affect Manual](AFFECT_MANUAL.md).*
See the **[Cookbook](COOKBOOK.md)** for complete worked examples.
