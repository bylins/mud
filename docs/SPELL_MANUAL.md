# Bylins MUD — Data-Driven Spell Manual

*For game designers authoring spells in `lib/cfg/spells.xml` and the companion
`lib/cfg/messages/ru/spell_msg.xml`.*

This manual catalogues every tag, attribute, and value the data-driven spell
system recognises today. After reading it you should be able to add a new
spell, tune an existing one, or compose any of the mechanics — damage, heal,
affect, dispel, stacking, alignment-restricted, reflected — without touching
C++.

For motivation and per-issue history, see the commit log on `sventovit.work`.
This document focuses strictly on what *you, the designer, can configure*.

> **Companion manuals.** Affects (buffs/debuffs/DoTs/wards and their triggers) are
> documented in the **[Affect Manual](AFFECT_MANUAL.md)**; perks/feats that modify
> spells or affects in the **[Feat Manual](FEAT_MANUAL.md)**. A spell *imposes* an
> affect (§8); the affect then owns its own behaviour.
See the **[Cookbook](COOKBOOK.md)** for complete worked examples.

---

## 1. The cast pipeline at a glance

When a player casts a spell, the engine runs the following stages **in this
order** for each target. Any stage can short-circuit the whole cast.

1. **Cast preconditions** — mana, position, sleeping, fighting, and the
   `<components>` gates (verbal → silence, weave → `kNoMagic` room, sight →
   blindness; §3.9), etc.
2. **Target & caster gates** (`<target_conditions>` per action;
   `<caster_conditions>` per spell) — data-driven `<blocking>`/`<required>`
   checks. A target gate aborts this target silently (group/mass casts) or with a
   "no effect" message (single-target); a caster gate or a NOMAGIC-room block
   aborts the whole cast.
3. **Reflection** (`<reflection>`) — if the target carries a reflecting flag
   or alignment, the spell may bounce back at the caster (one prob roll,
   covers all subsequent stages).
4. **`CastDamage`** — if `kMagDamage` flag set.
5. **`CastUnaffects`** — if `kMagUnaffects` flag set. Runs *before* affect so
   a spell can dispel an existing affect and then apply its own.
6. **`CastAffect`** — if `kMagAffects` flag set.
7. **`CastToPoints`** — if `kMagPoints` flag set (HP/MV restore).
8. **Side spells** — if the action carries `<side_spell>` (§3.8.3); a full
   nested cast on the current target.
9. **`CastManual`** — if `kMagManual` flag set (the hand-coded handler named by
   the action's `<manual_cast>`, §3.8.4).
10. **Alter object** — if the action carries an `<alter_obj>` (§3.8.7: bless,
    curse, enchant, repair, acid, …); a per-target stage that runs after the
    manual stage and is skipped only when the target died.
11. **Summon / Create object** — an action carrying a `<summon>` (§3.8.5: summon
    keeper / fire keeper / clone) or an `<obj_creation>` (§3.8.6: conjure food /
    light / weapon / armor) runs it **once per action, instead of** the
    per-target stages 2–10.
12. **Victim reaction** — NPC hostility / auto-cast of detection.

For the **entry** action, stages 4, 5, 6, 7 and 9 are gated by the spell's
`<flags val="kMag…">` bitset; later actions run the stages whose blocks they carry.
Side spells, alter-object, summon and create-object are never flag-gated — the
presence of the block is enough. A stage that returns "break" (a dispel chain break,
or the target dying) skips only **this action's** remaining stages on **this**
target; other targets and later actions continue.

The list above is what runs for **one `<action>`** of the spell. A spell's
`<talent_actions>` is an **ordered list** of actions, each with its own target
set and its own stages; the engine runs them top to bottom (action-outer,
target-inner), and the victim reaction (stage 12) fires once, after the **last**
action. A single-action spell behaves exactly as the list above. See §3.8 for the
multi-action model and [Cookbook](COOKBOOK.md) §3.8 for a worked example (`kSacrifice`).

> **Beyond the cast.** Stage 6 imposes *affects*. Once on the target, an affect
> runs its **own** trigger-driven actions — every tick (`kPulse`), on a hit
> (`kPreHit`/`kPostHit`), on a kill (`kKill`), on expiry (`kExpired`), on an
> incoming attack (`kWard*`), etc. That whole layer is the
> **[Affect Manual](AFFECT_MANUAL.md)**; the cast pipeline here only *delivers* the
> affect.

---

## 2. File layout

### 2.1 `lib/cfg/spells.xml`

Defines the spells themselves. Top-level element is `<spells>`. Each spell is
one `<spell>` child. Spell IDs come from the `ESpell` enum
(`src/gameplay/magic/spells_constants.h`).

```xml
<spells>
    <spell id="kFireball" element="kFire" mode="kEnabled">
        <name val="fireball"/>
        <components> <verbal/> <weave/> </components>
        <misc pos="kFight" violent="Y" danger="2"/>
        <mana max="110" min="100" change="1"/>
        <targets val="kTarCharRoom|kTarFightVict"/>
        <flags val="kMagDamage|kNpcDamagePc|kNpcDamagePcMinhp"/>
        <tags val="…"/>                            <!-- optional -->
        <potency_roll> … </potency_roll>
        <success_roll> … </success_roll>          <!-- optional -->
        <caster_conditions> … </caster_conditions> <!-- optional -->
        <talent_actions>
            <action>
                <!-- gates: target_conditions (blocking/required) / reflection -->
                <!-- effects: damage / points / area / affects / unaffect /
                     side_spell / manual_cast / alter_obj / summon / obj_creation -->
            </action>
        </talent_actions>
    </spell>
    …
</spells>
```

Encoding is **UTF-8** (`<?xml version="1.0" encoding="utf-8"?>`); edit it
directly.

> **Note on the XML reader.** The Bylins `DataNode` parser has *no inner-text
> support*. Every value must live in an attribute (`val="…"`, `min="…"`,
> etc.). Whitespace inside a tag is ignored.

### 2.2 `lib/cfg/messages/ru/spell_msg.xml`

Per-spell message strings, keyed by spell id and message type (`ESpellMsg`).
Each spell's sheaf also carries its **Russian display name** (`<name val="…"/>`).
See §12.

### 2.3 Small-world snapshot caveat

A build configured with `-Dsmall_world=true` gets a `build/<dir>/small/` world
whose `cfg/` is a **snapshot** of `lib/cfg/`, copied once at `meson setup` time
(`tools/meson/setup_world.py`). `./circle -d small` reads that snapshot, so after
editing anything under `lib/cfg/` refresh it before booting:

```bash
cp -r lib/cfg/. build/<your-build>/small/cfg/
```

Otherwise your edits won't be loaded and you may see ghost "incorrect
constant" SYSERRs from old values. On a running server an immortal can re-read the
files without a reboot: `reload spells` and `reload spellmsg`.

---

## 3. The `<spell>` element

### 3.1 Header attributes

| Attribute | Required | Description |
|---|---|---|
| `id` | yes | `ESpell` enum value, e.g. `kFireball`. |
| `element` | no | `EElement`: `kFire`, `kAir`, `kWater`, `kEarth`, `kLife`, `kDark`, `kLight`, `kMind`, `kUndefined`. Used for resist/element-coefficient calcs. |
| `mode` | no | `EItemMode`: `kEnabled` (default), `kDisabled`, `kService` (engine-only stub), `kFrozen` (temporarily off — event/holiday content), `kTesting` (hidden by default, can be granted by an immortal; §17). An unknown value silently falls back to `kEnabled`. |

### 3.2 `<name>`

```xml
<name val="fireball"/>
```

| Attribute | Description |
|---|---|
| `val` | English name — the admin label used in admin tools and logs. |

The **Russian display name** shown in-game is not here: it lives in the spell's
sheaf in `spell_msg.xml` (`<msg_sheaf id="kFireball"><name val="огненный шар"/>`,
§12). A spell without such a sheaf gets an empty display name.

### 3.3 `<misc>`

```xml
<misc pos="kFight" violent="Y" danger="1"/>
```

| Attribute | Description |
|---|---|
| `pos` | Minimum caster position (`EPosition`): `kSleep`, `kRest`, `kSit`, `kFight`, `kStand`. |
| `violent` | `Y` / `N` / `A`. See below. |
| `danger` | Integer threat rating used by mob AI / scrolls / pricing. |

**`violent` (issue.ambiguous-spells).** Three-state:

| Value | Behaviour |
|---|---|
| `N` | Pure-helpful spell. Never triggers PK, retaliation, AR resist, magic-mirror / sonic-barrier reflection, or potency-gated dispel. |
| `Y` | Always violent. PK rules + retaliation + saving-shields + dispel contest + GET_AR check all apply. |
| `A` | **Ambiguous.** Resolves per-target: helpful on self / charmed pet (walking up the master chain) / groupmate / NPC-vs-NPC, fully violent (with full PK liability) on an outsider. `kDispellMagic` is the current carrier — dispel from an ally hand passes without a contest; dispel from an outsider triggers the d100 dispel contest (§9.3). |

The runtime helper that decides per-target is
`SpellInfo::IsViolentAgainst(caster, target)`. The static `IsViolent()`
predicate keeps `bool` semantics for back-compat — it reports `true` only
for `kYes`, so any call site that hasn't been migrated treats `A` as
"helpful by default" (the safer no-PK fallback). `A` is rejected at load
when combined with `<flags val="kMagRoom">` (SYSERR-clamps to `Y`):
room paints have no per-target relationship to resolve.

### 3.4 `<mana>`

```xml
<mana min="60" max="80" change="1"/>
```

| Attribute | Description |
|---|---|
| `min` | Mana cost at the highest practical level. |
| `max` | Mana cost at the lowest practical level. |
| `change` | Per-level decrease as the caster levels up. For a war-cry (`kMagWarcry`) it doubles as the minimum `kWarcry` skill needed to use the cry. |

### 3.5 `<targets val="…">`

Pipe-separated bitset of `ETarget` flags. Pick at least one of the "where to
look" flags plus any check flags.

| Flag | Meaning |
|---|---|
| `kTarNone` | No target at all (service / stub spells). |
| `kTarIgnore` | No target argument — area/mass cast, world spells. |
| `kTarCharRoom` | A character in the same room. |
| `kTarCharWorld` | A character anywhere in the world (summon/teleport). |
| `kTarFightSelf` | While fighting, may target self. |
| `kTarFightVict` | While fighting, may target the current opponent. |
| `kTarSelfOnly` | *Check only:* only self is allowed. |
| `kTarNotSelf` | *Check only:* self is not allowed. |
| `kTarObjInv` | An item in the caster's inventory. |
| `kTarObjRoom` | An item on the ground in the room. |
| `kTarObjWorld` | An item anywhere. |
| `kTarObjEquip` | An item the caster is wearing. |
| `kTarRoomThis` | The current room itself (room spells). |
| `kTarRoomDir` | A neighbouring room by direction. |
| `kTarRoomWorld` | An arbitrary world room. |
| `kTarAllyOnly` | *Check only:* only self or a groupmate. Used with `kTarCharRoom`. |
| `kTarMinionsOnly` | *Check only:* only one of the caster's own NPC followers (master == caster). |
| `kTarDirection` | A direction / exit from the caster (door affects); the argument is a Russian or English direction name. |

### 3.6 `<flags val="…">`

Pipe-separated bitset of `EMagic` flags. These both **route the spell through
the stages** (anything starting `kMag…` enables a stage) and **carry the
gameplay metadata** (PK / agro / damage-type).

| Stage routing | Effect |
|---|---|
| `kMagDamage` | Run `CastDamage`. |
| `kMagAffects` | Run `CastAffect`. |
| `kMagUnaffects` | Run `CastUnaffects`. |
| `kMagPoints` | Run `CastToPoints` (HP/MV restore). |
| `kMagManual` | Run the hand-coded handler named by the entry action's `<manual_cast>` (§3.8.4; also covers movement spells like teleport, recall, portal, relocate). |
| `kMagGroups` | Spell targets a group. |
| `kMagMasses` | Mass cast — every NPC in the room (excluding allies). |
| `kMagAreas` | Area cast — every hostile in the room with decay. |
| `kMagWarcry` | Marks the spell as a war-cry (skips magic defences). |
| `kMagNeedControl` | Requires caster's pet to be present. |
| `kMagRoom` | Room spell: cast by `magic/action_room_affect.cpp`, the affect itself lives in `affects/room_affects.cpp`. |
| `kMagCasterInroom`, `kMagCasterInworld`, `kMagCasterAnywhere`, `kMagCasterInworldDelay` | Declared lifetime markers for room affects (while the caster is in the room / in the world / until it expires / in the world plus a delay). Accepted by the loader, but the engine does not consult them today. |

The `kNpc…` flags are read mainly by the **mob AI**: when an NPC picks a combat spell,
the flag decides whether it casts it at all (`NPC_CALCULATE`) and how it chooses the
target (`fight.cpp`). Two of them also gate PK in `CastAffect`.

| Mob-AI / PK class | Effect |
|---|---|
| `kNpcDamagePc` | Offensive spell: the NPC aims it at a hostile target (`find_target`). |
| `kNpcDamagePcMinhp` | Same targeting as `kNpcDamagePc` (checked first). No HP-floor logic is attached to it in code today. |
| `kNpcAffectPc` | Debuff for an opponent (`find_opp_affectee`). On a violent cast it is also the PK gate: imposing it on a player runs `pk_agro_action`. |
| `kNpcAffectPcCaster` | Debuff aimed at an opposing caster (`find_opp_caster`). |
| `kNpcUnaffectPc` | Offensive dispel: strip an enemy player's buffs (`find_opp_dispellable`). |
| `kNpcAffectNpc` | Buff for an ally (`find_affectee`). On a violent cast against a fighting target it runs `pk_agro_action` against that target's enemy. |
| `kNpcUnaffectNpc` | Cure for an ally (`find_friend`). |
| `kNpcUnaffectNpcCaster` | Cure aimed at an allied caster (`find_caster`). |
| `kNpcDummy` | Heal-type spell for an ally (`find_friend_cure`). |

A spell may also carry `<tags val="curse|…"/>` (spell level, after `<flags>`): a free-form,
`|`-separated set of semantic categories. Tags change nothing by themselves; a feat's
`<talent_patch category="…">` uses them to select the spells it patches (see the
[Feat Manual](FEAT_MANUAL.md)).

### 3.7 `<potency_roll>` and `<success_roll>`

Both blocks have the same skill/stat shape (`talents_actions::Roll` /
`talents_actions::SuccessRoll`). Each spell has **one potency roll** that all of its
effects share (damage amount, points, affect modifier, duration bonus, extra-hits
bonus, dispel strength).

```xml
<potency_roll>
    <noise sigma="0.25" trunc="2"/>
    <base_skill id="kFireMagic" low_skill_bonus="1" hi_skill_bonus="7"/>
    <base_stat id="kWis" threshold="32" weight="18"/>
</potency_roll>
```

| Child / attr | Description |
|---|---|
| `<noise sigma= trunc=>` | The spell's **one shared random draw** (replaced the old `<dices>`, which is no longer read). At cast time one `z ~ N(0,1)` is drawn and clamped to `±trunc` (default `2`); the realised deviation `d = sigma · z` is shared by every effect of the cast, each scaling it by its own `weight`. `sigma` defaults to `0`; an absent `<noise>` makes the spell deterministic. |
| `<base_skill id= low_skill_bonus= hi_skill_bonus=>` | Adds a skill-based coefficient. Below the novice threshold (skill 75), each point grants `low_skill_bonus`; above 75, each point grants `hi_skill_bonus`. Final coefficient = `(low_skill·low_bonus + hi_skill·hi_bonus) / 100`. Attribute defaults when the child is present: `low_skill_bonus=1`, `hi_skill_bonus=7`. Omit the child to skip skill contribution. |
| `<base_stat id= threshold= weight=>` | Adds a stat-based coefficient. Contribution = `max(0, stat − threshold) · weight / 100`. Defaults when present: `threshold=0`, `weight=18`. Omit the child to skip stat contribution. |

`<success_roll>` takes `<base_skill>` / `<base_stat>` (no noise; attribute defaults
`0`) plus the optional `<bonus roll= pvp= pve= evp= eve=>` (flat and per-matchup
bonuses; P = player, E = NPC) and `<thresholds critsuccess= critfail=>` (defaults 6 /
95). Today only its **`base_skill` id** is consumed: it is the skill trained on a cast,
it adds `skill / 20` to the `CalcCastSuccess` percentage, and it is the skill that
scales the `<area>` target count (§7). The weights, `<bonus>` and `<thresholds>` are
parsed and stored but not yet used.

The `base_skill` from `<potency_roll>` drives the affect duration and the
multi-hit count (see §8.2 and §5).

#### Competence `C`, effect magnitude, and the `cast_potency` scalar

The roll produces three numbers — `skill_coeff`, `stat_coeff` and the shared noise
deviation `d`. Two **different** quantities are built from them; do not conflate them.

* `skill_coeff` = `(low_skill · low_skill_bonus + hi_skill · hi_skill_bonus) / 100`
  from `<base_skill>` (skill is capped at `kNoviceSkillThreshold = 75` for the
  low part; everything above goes into the hi part).
* `stat_coeff` = `max(0, stat − threshold) · weight / 100` from `<base_stat>`.
* `d` = `sigma · z` from `<noise>` (one draw per cast).

**Competence** is the skill+stat part, written `C` throughout the manual:

```
C = skill_coeff + stat_coeff
```

**Effect magnitude** (damage, heal/points, affect modifier) scales with `C` and
carries the shared noise multiplicatively (§5, §6, §8.4):

```
amount = min + beta · C · (1 + weight · d)
```

`beta · C` is the competence term; `weight` says how strongly this particular effect
follows the cast's shared draw (`weight="1"` takes the full `±sigma` spread,
`weight="0"` is deterministic). Because the noise multiplies `beta · C`, its relative
spread stays constant as competence grows instead of fading into a large additive
total. Damage and points round the result and never go below `max(0, floor(min))`;
an affect modifier uses `min + ceil(beta · C · (1 + weight · d))`, then its `cap` and
`factor` (Affect Manual).

**`cast_potency`** is a *separate* scalar — the caster's **deterministic competence**
(`CalcCastPotency`, issue.random-noise-rework):

```
cast_potency = skill_coeff + stat_coeff        (no noise)
```

It is used **only as a relative-strength number**, never as an effect's magnitude: the
potency stored on an affect (`cast_potency × <affects potency_weight>`) and the dispel
contest that later tries to remove it (§9.3). Dropping the noise makes the stored strength
reflect the caster's **skill**, not the randomness of the delivered amount — so the dispel
contest is a clean skill comparison (§9.3). Extra-hit counts are a third, separate thing:
they scale on `min(skill, 75) / skill_divisor`, not on `cast_potency` (§5).

A spell that omits a sub-block contributes 0 from that side. A spell with an empty
`<potency_roll>` yields competence 0 — its affects land at potency 0, so any dispel removes
them trivially (the §9.3 threshold hits its 95 % ceiling), and any magnitude reduces to its
`min`.

### 3.8 `<talent_actions>` — the ordered action list *(multi-action)*

> This is the **spell's** action list, run once at cast time. Affects have a
> parallel `<actions>` list that runs on *triggers* over the affect's life — same
> action vocabulary, different driver. See the [Affect Manual](AFFECT_MANUAL.md) §5.

Container for an **ordered list** of `<action>` blocks. Each `<action>` is a
self-contained sub-cast: it resolves its **own** target list and runs its **own**
stages, and the actions execute **top to bottom**. Each `<action>` can declare any
mix of:

* Gates: `<target_conditions>` (wraps per-action `<blocking>`/`<required>`),
  `<reflection>`. (Caster-side gating is the spell-level `<caster_conditions>` — §4.3.)
* Effects: `<damage>`, `<points>`, `<area>`, `<affects>`, `<unaffect>`,
  `<side_spell>`, `<manual_cast>`, `<alter_obj>`, `<summon>`, `<obj_creation>`.

An `<action>` may also carry `id="Name"` — a stable name that feat patches use to
address it (`<talent_patch action="…">`, see the [Feat Manual](FEAT_MANUAL.md)). The
`<trigger>`, `<absorption>`, `<damage_change>` and `<retaliation>` children belong to
the affect-side `<actions>` list and are documented in the
[Affect Manual](AFFECT_MANUAL.md).

Within a single action the per-target effect stages always run in this fixed order:

> **Damage → Unaffect → Affect → Points → SideSpell → Manual → AlterObj**

`<summon>` and `<obj_creation>` are action-level: an action carrying one runs it once
and skips the per-target stages (§3.8.5, §3.8.6).

Most spells still use a single `<action>`, but a spell can now chain several —
each aimed at a different target set and able to scale off what an earlier action
did. See **§3.8.1–3.8.4** below and the worked example in
**[Cookbook](COOKBOOK.md) §3.8 (`kSacrifice`)**.

#### How the action list is executed (action-outer, target-inner)

* **Action 0 (the *entry* action)** uses the **spell-level** scope — the `kMag…`
  flags pick the roster (`kMagAreas`/`kMagMasses` → foes, `kMagGroups` → allies,
  otherwise the single chosen target). It runs the full per-target pipeline:
  the target gates, `<reflection>`, the god guard, the cast `mtrigger`, **and** the
  effect stages, then records each successfully-affected target for the reaction.
* **Actions 1, 2, …** resolve their own roster from their `target=` attribute
  (§3.8.1) and run **only** their effect stages plus their own
  `<blocking>`/`<required>`. They do **not** re-fire reflection, the mtrigger, or
  the victim reaction.
* **Victim reaction** (NPC retaliation / auto-detect) is deferred until **all**
  actions have finished — the whole spell is one event, so a debuff in action 1
  can't make the target retaliate before action 2's damage lands.
* If an action **kills** its target (or a dispel breaks the chain on it), only
  that action's remaining stages on **that** target are skipped — the rest of the
  action list, and the action's other targets, continue.

For a one-action spell only the entry action runs, exactly as before — the whole
mechanism is backward-compatible.

#### 3.8.1 `target="…"` — per-action target selector

A non-entry `<action>` may carry a `target` attribute (an `EActionTarget`, default
`kTarSame`) selecting **which** characters it affects:

| value | resolves to | uses `<area>`? |
|---|---|---|
| `kTarSame` *(default)* | the **previous** action's resolved target list | inherits |
| `kTarFightSelf` | the caster | no |
| `kTarFightVict` | the caster's current opponent | no |
| `kTarGroup` | groupmates in the room | **yes** |
| `kTarFoes` | all foes in the room | **yes** |
| `kTarRandomFoe` | one random foe in the room | no |
| `kTarRandomAlly` | one random ally in the room | no |
| `kTarMinions` | the caster's charmed NPC followers (minions) in the room | yes (for >1) |
| `kTarActor` | the character that fired an event trigger (e.g. the one entering the room); for affect-side actions | no |
| `kTarRoomThis` | the room itself (`ctx.rvict`); the action imposes its effect on the room | no |
| `kTarMaster` | the caster's charm master (used by patches acting on a minion's master, e.g. `kSoulLink`) | no |

`kTarGroup`/`kTarFoes`/`kTarMinions` fan out over several characters; an `<area>`
block (§7) says how many and with what falloff. Without `<area>` the action hits the
whole roster at full strength.

#### 3.8.2 `base="…"` and `reset` — chain off a prior action's result *(cast-chain)*

By default every action scales its effect off the caster's **competence**
(skill + stat coefficients from the potency roll). A non-entry action can instead
scale off **what an earlier action produced**, via `base` (an `EActionBase`):

| `base` value | substitutes the competence scalar with… |
|---|---|
| `kCompetence` *(default)* | the real caster competence (skill + stat) |
| `kDamage` | total HP damage dealt so far in the cast |
| `kPoints` | total points (HP/moves/…) restored so far |
| `kAffects` | total **potency** of affects applied so far |
| `kDispelled` | total **potency** of affects removed so far |
| `tag` (with `tag="NAME"`) | the numeric value of event tag `NAME` (affect-side trigger actions; see the Affect Manual) |

The accumulators **aggregate across an action's targets** (they sum). `base` only
replaces the competence scalar `C` in the formula — the action's own `min`, `beta`
and `weight` still apply, so for a clean 1:1 transfer the designer sets `weight="0"`
and `beta="1.0"` (see `kSacrifice` A2/A3 in [Cookbook](COOKBOOK.md) §3.8).
`reset="true"` (also `Y` / `1`) zeroes the chosen base accumulator **after** this
action runs, so a later action starts the tally fresh.

#### 3.8.3 `<side_spell id="…"/>` — trigger a full nested cast

```xml
<side_spell id="kHold"/>
```

Fires a **complete nested cast** of another spell on the current target — with
that spell's *own* rolls, potency, and effect config. Several `<side_spell>` tags
are allowed (run in document order). This is how a mass/group spell becomes a thin
wrapper over its single-target base: `kMassHold` is just an `<area>` fan-out plus
`<side_spell id="kHold"/>`, reusing `kHold`'s entire definition instead of copying
it. A **cycle guard** refuses any spell already present in the current cast chain,
so `A → B → A` (and deeper loops) can't recurse infinitely. The nested cast
inherits the outer per-target area coefficient, so mass-falloff still reaches the
delegated effect.

#### 3.8.4 `<manual_cast handler="…"/>` — data-driven manual handler

```xml
<manual_cast handler="SpellMentalShadow"/>
```

Names a hand-coded handler from the `kManualHandlers` registry
(`src/gameplay/magic/action_handlers.cpp`; the handler bodies live in
`src/gameplay/handlers/`). The name must match the registry, or boot logs
`Spell …: unknown <manual_cast> handler '…'`. It runs as the **Manual** stage of the
action, so a manual handler is gated and ordered like any other stage. On the entry
action the stage still requires the `kMagManual` flag; a later action runs it
whenever it names a handler.

#### 3.8.5 `<summon>` — spawn a charmed minion *(issue.summon-pipeline)*

```xml
<summon base_fail="50" min_fail="0" handler="SetupKeeperStats">
    <mob vnum="3021" competence_weight="16" keeper="Y"/>
</summon>
```

Spawns a mob in the **caster's room** and charms it to the caster. Like a room
action (§3.8.1 `kTarRoomThis`), a `<summon>` runs **once per action — not once per
target** — so it is dispatched at the action level (instead of the per-target path:
the same action's other effect blocks do not run) and
can chain off a prior action via `base=` (§3.8.2): the entry summon reads the real
potency `C`, a chained one reads its `base=` accumulator (e.g. a familiar whose
strength scales with the damage just dealt). It replaced the old `kMagSummons` flag,
which no longer exists.

`<summon>` attributes:

| Attr | Meaning |
|---|---|
| `base_fail` | Base failure chance, percent. |
| `min_fail` | Floor for the failure chance (default `0`). |
| `handler` | Optional named post-spawn handler (see below). |

The failure chance is `pfail = clamp(min_fail, base_fail − C·competence_weight, 100)`,
where `C` is the action's competence (§3.7). So a competent caster drives the fail
toward `min_fail`; a weak one approaches `base_fail`. The `kFavorOfDarkness` feat
re-rolls a failure at ¼ (only 1 in 4 sticks). Immortals never fail. A charmed
caster cannot summon (`kSummonCharmed`); a missing prototype emits `kSummonNoProto`.

`<mob>` attributes:

| Attr | Meaning |
|---|---|
| `vnum` | The mob prototype vnum (plain positive, e.g. `3021`). It is loaded as a *summoned* instance — no triggers / online count, `EMobFlag::kCompanion` set (`ReadMobile` negates internally). The current handlers additionally set `EMobFlag::kSummoned` (a true conjuration, banishable). |
| `competence_weight` | How strongly `C` reduces the fail (the `k` above; a real number). Author it against whatever scale `base=` feeds the action. |
| `keeper` | `Y` → the minion gets `EAffect::kHelper` + `ESkill::kRescue=100` (a guardian that rescues its master). |

The minion is charmed (`ESpell::kCharm`) for a wisdom + moon-phase duration, its
gold/exp are zeroed, and it is placed and made a follower. `handler` then runs the
spell-specific post-spawn work — it must match an entry in the `kSummonHandlers`
registry in `src/gameplay/magic/action_summon.cpp` (signature
`(ch, mob, ctx, duration)`); an unknown name logs `unknown summon handler` at cast
time. Current handlers: `SetupKeeperStats`, `SetupFirekeeperStats`, `CloneCascade`.

Use `<summon>` for **vnum-based** summons whose failure scales with `C`
(summon keeper / fire keeper / clone). Corpse-based summons (animate dead,
resurrection — the mob and a con/level-based fail come from the source corpse) and
the non-mob summons (guardian angel, mental shadow) don't fit this data model and
live in `<manual_cast>` handlers instead (§3.8.4): `SpellAnimateDead`,
`SpellAnimateSkeleton`, `SpellResurrection`, `SummonTutelar`, `SpellMentalShadow`.

#### 3.8.6 `<obj_creation>` — conjure an object *(issue.obj-casting)*

```xml
<obj_creation vnum="125" handler="CreateWeapon"/>
```

Creates an object in the caster's room/inventory. Runs **once per action** (like `<summon>`), at the
action level. `vnum` (required) is the prototype to load; the skeleton narrates it (`kItemCreatedToChar` /
`kItemCreatedToRoom`; a missing prototype emits `kItemNoPrototype` and a SYSERR) and places it
(inventory, or dropped to the room when the caster is over-encumbered). `handler` (optional) names a
post-load customizer in the `kCreationHandlers` registry (`src/gameplay/magic/action_handlers.cpp`) — it runs **before** the
narration/placement so it can shape the freshly-loaded base object. An unregistered name logs
`unknown creation handler` at cast time (same as `<manual_cast>` / `<alter_obj>`).

| Attr | Meaning |
|---|---|
| `vnum` | Prototype vnum to load (required). |
| `handler` | Optional post-load customizer (`kCreationHandlers`). |

`kCreateFood` / `kCreateLight` use no handler (plain load). `kCreateWeapon` / `kCreateArmor` carry
`CreateWeapon` / `CreateArmor` — **plumbing stubs today** (the base vnum is loaded; the stat/type
customization is a documented TODO in each handler body). It replaced the old `kMagCreations` flag.

#### 3.8.7 `<alter_obj>` — transform the target object *(issue.obj-casting)*

```xml
<alter_obj handler="AlterBless"/>
```

Transforms a target object, running as a **per-target stage** (so an area corrode hits each target's
item). The skeleton resolves the object (an explicit `ovict`, else — **only when the `<alter_obj>`
opts in with `collateral="on_damage"` and the cast dealt damage this stage** (`ctx.result.damage != 0`)
— a random equipped/carried item of the victim; issue.spells-hotfix), guards
the `kNoalter` flag (a no-alter item emits `kObjResist`), then dispatches the named handler from the
`kAlterObjHandlers` registry (`src/gameplay/magic/action_handlers.cpp`). The transform itself is irreducibly per-spell, so
the main attribute is `handler`, plus the optional `collateral` / `dispel_bonus` / `decay` / `bypass_noalter` (mirroring `<manual_cast>`, but with the
shared object-resolution + guard skeleton around it):

| Attr | Default | Description |
|---|---|---|
| `handler` | required | The per-spell transform, resolved from the `kAlterObjHandlers` registry. |
| `collateral` | `none` | On a **character** cast (no explicit object target), splash onto one equipped/carried item: `on_damage` = only when the cast dealt damage this stage (acid corroding gear); `first_suppressed` = a random worn item of the victim carrying a **suppressed** affect (for weave restoration, see §9.3.1). Absent / `none` = never — cast the spell **directly on an object** to affect one. |
| `dispel_bonus` | `50` | For contest handlers (e.g. `AlterWeaveRestore`): the flat `DispelContest` threshold offset — **win % at equal competence** (§9.3.1). |
| `decay` | `0` | For contest handlers: on a **miss**, erode the stored suppression potency by this % of the dispeller's strength (repeated casts wear it down). |
| `bypass_noalter` | `N` | `Y` skips the `kNoalter` guard: the handler restores the item's **own** magic rather than transmuting it (needed for no-alter set items). |

The handler does its **own** messaging and returns `kSuccess` when it acted (or chose to stay
silent), or **`kFail`** to ask the skeleton for the generic "no effect" line. Current handlers:
`AlterBless` / `AlterCurse` / `AlterInvisible` / `AlterPoison` / `AlterRemoveCurse` /
`AlterEnchantWeapon` / `AlterRemovePoison` / `AlterFly` / `AlterAcid` (kAcid + kAcidArrow) /
`AlterRepair` / `AlterTimerRestore` / `AlterRestoration` / `AlterLight` / `AlterDarkness` / `AlterWeaveRestore` (lifts an item suppression — §9.3.1). A
multi-purpose spell (e.g. `kBless`, which also affects a character) carries `<alter_obj>` alongside
its `<affects>` in the same action. It replaced the old `kMagAlterObjs` flag. The random-item splash
is now **opt-in per spell** via `collateral="on_damage"` (issue.spells-hotfix): casting a spell on a
*character* applies only its `<affects>` / `<damage>` unless the `<alter_obj>` opts in — so `kCurse` /
`kPoison` / `kBless` cast on a character no longer touch their items (cast the spell **directly on an
object** to affect one). Only `kAcid` / `kAcidArrow` opt in, corroding a random worn/carried item when
the hit dealt damage. The object branch otherwise runs only when an object was explicitly targeted.

> **Why "on damage" and not "violent"?** This replaced the earlier violent-only rule
> (`IsViolentAgainst`), under which *every* violent cast splashed onto gear — so `kCurse`/`kPoison`
> aimed at a character also cursed/poisoned a random worn item. The opt-in flag scopes the splash to
> the spells that mean it (acid). `ctx.result.damage` is the computed damage of the stage; tying it to
> actually-dealt damage is a separate `Damage::Process` matter.

> **Object lifetime:** if the cast destroys the target object (the acid corrode, or a consumed
> material component), it is **deferred-extracted** — flagged `ObjData::purged()` and freed at the
> next heartbeat, never dangling. The skeleton treats a `purged()` target as absent. See §3.8.7's
> handlers and `DamageObj` / `ProcessMatComponents`.

#### 3.8.8 `choice="…"` — selectable actions *(issue #2689)*

```xml
<action>                                   <!-- no choice: always runs -->
	<affects …><affect id="kPietas"/></affects>
</action>
<action choice="Перуну|Перун">             <!-- runs only when chosen -->
	<affects …><affect id="kPerunFavor"/></affects>
</action>
<action choice="Велесу|Велес">
	<affects …><affect id="kVelesFavor"/></affects>
</action>
```

A spell whose actions carry `choice` lets the caster pick **one** of them by a
word typed after the target (`ActionContext::cast_args`, a `CastArgs`: every
word after the spell's name plus how many of them named the target; the choice is
the words after the target, joined by spaces. A spell that ignores targets gets all
of its words. A command such as `жертвовать` passes them itself). The cast runs every action **without** `choice` plus exactly
the chosen one, in list order; the chain is narrowed in `CallMagic` right after
the perk patches, so a patched chain is narrowed too, and the first remaining
action is the entry action.

- `choice` is a `|`-separated list of words; the **first** one is canonical — it
  is shown in the list of options and substituted for `{choice}` in messages.
- Matching: an exact (case-insensitive) word first, then the first action (in
  order) one of whose words **starts with** what was typed. A mere substring does
  not match.
- Nothing typed or nothing matched: the cast does not happen
  (`ECastResult::kNotCast`); the caster gets `kUnknownChoice` with `{choices}`
  replaced by the canonical words.
- `choice` is not `id`: `id` is the stable address perk patches use, `choice` is
  what the player types.

The prayer and the sacrifice (`kPrayer`, `kOffering`) are built this way: the
marker action has no choice, and each god or saint is a selectable action with its
own favour.

### 3.9 `<components>` — casting requirements

```xml
<components>
    <verbal/>
    <weave/>
    <sight/>
    <material any_of="1234|5678" cost="1"/>
</components>
```

Optional child of `<spell>`. Each inner tag is a presence-only marker or a
material-component descriptor; absent `<components>` means "no requirements".
Four kinds today:

| Tag | Attributes | Effect |
|---|---|---|
| `<verbal/>` | none | The cast needs a spoken phrase. `kSilence` on the caster blocks it (silenced casters can still cast non-verbal spells, and `SaySpell` stays silent for them). |
| `<weave/>` *(issue.weave-component)* | none | Marks the action as "actual magic". A weave spell fizzles in a `kNoMagic` room — emits `kCastForbiddenToChar` / `kCastForbiddenToRoom` exactly like the legacy data-driven `<blocking><room_flags val="kNoMagic"/>` did. Replaces 209 copies of that blocking block — `<weave/>` is now the single source of truth for "is this magic". `<verbal/>` and `<weave/>` are independent (warcries are verbal but **not** weaves). |
| `<sight/>` *(issue.sight-component)* | none | The cast needs the caster to **see**. A blind caster (`AFF kBlind`) cannot cast it — gets the inline notice `Вы ослеплены!` plus the spell's `kNoeffect` message, and the cast fizzles. Currently on `kRuneLabel` and `kFascination`. This is a **caster-state** gate (blindness), not a room gate. |
| `<material .../>` | `any_of` / `all_of` / `where` / `cost` | Item-based component. `any_of` / `all_of` are `\|`-separated obj vnum lists. `where` is a `\|`-separated `EFind` list (defaults to `kObjEquip\|kObjInventory\|kObjRoom` — equip first, then inventory, then room). `cost` is the charge consumption (default 1; 0 = presence-only focus; -1 = consumed whole). |

The `MayCastInForbiddenRoom` exemption (greater gods, uncharmed NPCs) bypasses
both the `<weave/>` gate and the data-driven `<blocking>` room-flag gate
symmetrically. The `<sight/>` gate is a caster-state check (blindness) and is
**not** subject to that room exemption. Per-spell `kCastForbiddenToChar` /
`kCastForbiddenToRoom` sheaves override the kDefault narration.

---

## 4. Target & caster gates

Casts are gated by reusable condition blocks — `<blocking>` (refuse if **any**
listed condition matches) and `<required>` (refuse unless **all** match). They
never appear bare; each is wrapped by one of two containers:

- **`<target_conditions>`** — a child of an `<action>`. Its `<blocking>` /
  `<required>` examine the **target** and gate **that action** for the current
  target (all of the action's stages). Most spells are single-action, so in
  practice this gates the whole cast for that target.
- **`<caster_conditions>`** — a **spell-level** child of `<spell>` (not of an
  `<action>`). Its `<blocking>` / `<required>` examine the **caster** and run once,
  before any per-target dispatch — see §4.3.

Bare `<blocking>` / `<required>` directly under `<action>` is **no longer accepted**;
always wrap them. (`<caster_blocking>`, a single-tag caster gate, was removed — its
job is now done by `<caster_conditions>`.)

### 4.1 `<blocking>`

```xml
<target_conditions>
    <blocking>
        <mob_flags val="kNoSleep"/>
        <affect_flags val="kHold|kSleep"/>
        <room_flags val="kNoMagic"/>
        <align val="kGood"/>
    </blocking>
</target_conditions>
```

The cast is refused if **any one** of the listed conditions matches: a
mob flag (NPC only), a runtime character mark, an affect flag (any target),
the alignment selector (target alignment), or a room flag (CASTER's current
room). Each axis is optional; combine as many as the spell needs.

The child-tag form is used because lines get long when several axes
combine. The older attribute form (`<blocking mob_flags="..."/>`) has been
retired. (The `kNoMagic` room block that once sat on almost every spell is now
expressed by `<weave/>`, §3.9.)

| Child tag | Values |
|---|---|
| `<mob_flags val="…"/>` | `\|`-separated `EMobFlag` names. Recognised: `kNoBlind`, `kNoSleep`, `kNoSilence`, `kNoHold`, `kNoBash`, `kNoCharm`, `kNoSummon`, `kNoFear`, `kCorpse`, `kMounting`, `kHelper`, `kClone`, `kIgnoreForbidden`, `kCompanion`. |
| `<extra_flags val="…"/>` | `\|`-separated `ECharExtraFlag` names — runtime marks set only by code, never saved. Recognised: `kResurrected` (a corpse revived by `kResurrection`; used by `kStoneBones`). |
| `<affect_flags val="…"/>` | `\|`-separated `EAffect` names (any affect flag the engine recognises). |
| `<room_flags val="…"/>` | `\|`-separated `ERoomFlag` names. Recognised: `kNoMagic`, `kPeaceful`, `kTunnel`, `kNoTeleportIn`, `kNoTeleportOut`, `kNoBattle`, `kGodsRoom`, `kForMono`, `kForPoly`. Matches against the caster's room. |
| `<align val="…"/>` | `kGood`, `kEvil`, `kNeutral` — block if the target carries this alignment. |

On a single-target spell, a target-side block emits `kNoeffect` to the
caster. On a group/mass cast, a target-side block is silent for that
target (no per-target spam). A **room-flag block** is different: it
fizzles the *whole* cast before any per-target dispatch and emits
`kCastForbiddenToChar` / `kCastForbiddenToRoom` (see §12). Greater gods
and uncharmed/non-tutelar NPCs bypass the room-flag block
(`MayCastInForbiddenRoom`); warcry spells carry no `<weave/>` and list no
`kNoMagic` block, so the same bypass isn't needed for them.

### 4.2 `<required>`

```xml
<target_conditions>
    <required>
        <mob_flags val="kCorpse"/>
        <affect_flags val="kCharmed"/>
        <align val="kEvil"/>
    </required>
</target_conditions>
```

Same child-tag shape as `<blocking>` but the cast is refused unless the
target **has all** the listed flags/alignment. Use it for "only on
corpses" (animate dead), "only on charmed minions", "only on evil
creatures" (dispel evil), etc. `<required>` does not currently use
`<room_flags>`.

### 4.3 `<caster_conditions>` — gate on the **caster**

```xml
<caster_conditions>
    <blocking>
        <align val="kEvil"/>
    </blocking>
</caster_conditions>
```

A **spell-level** block — a direct child of `<spell>`, *not* of an `<action>`.
It wraps the same `<blocking>` / `<required>` blocks as `<target_conditions>`
(§4.1, §4.2), but they examine the **caster** instead of the target, and the check
runs **once**, before any per-target dispatch — so a failure aborts the *whole*
cast and emits `kNoeffect` (there is no group/mass silent skip: a caster gate
concerns the one caster, not the per-target loop).

Only the `<align>` and `<affect_flags>` axes are meaningful for a caster
(`<mob_flags>` / `<room_flags>` are accepted by the parser but unused for casters
today). Semantics match the target side: `<blocking>` refuses on **any** match;
`<required>` refuses unless the caster has **all**.

Use it to refuse a cast the caster cannot wield. `kDispelEvil` carries the block
above so an evil caster can't cast it; a held caster can be locked out of a
self-buff with `<blocking><affect_flags val="kHold"/></blocking>`; and
`<required>` can demand the caster carry a flag instead.

> Replaces the old single-tag `<caster_blocking><caster align= affect_flags=/>`
> form — caster gating now uses exactly the same nested `<blocking>` / `<required>`
> shape as the target side, so the two are symmetric.

### 4.4 `<reflection>` *(new mechanic)*

```xml
<reflection affect_flags="kFireShield" align="kGood" prob="35"/>
```

If the original target carries any of the listed flags or matches the
alignment, the spell may **bounce back at the caster** on a percentile roll
against `prob`. Self-casts never bounce.

| Attribute | Default | Description |
|---|---|---|
| `affect_flags` | (empty) | `\|`-separated `EAffect` names. Any match triggers the reflection roll. |
| `align` | `kAny` | `kGood`/`kEvil`/`kNeutral`. Target's matching alignment also triggers the roll. |
| `prob` | `20` | Percent chance the spell actually reflects when the flag/align match. |

(`min` / `max` are also parsed: they turn a *defender's* magic-mirror ward into a
potency contest — an affect-side feature, see the Affect Manual.)

When reflection fires, three messages from `spell_msg.xml` are emitted:
`kReflectedToChar` (to the caster), `kReflectedToVict` (to the original
target), `kReflectedToRoom` (to onlookers). After reflection, the spell
continues against the caster — damage, affect, lag, everything.

**Why we use bare flag + prob rather than a strength contest:** mob and
object affects carry no potency value, so a flag-vs-potency comparison isn't
meaningful today. The `prob` knob is the designer's lever for tuning
reflection rates per spell.

---

## 5. `<damage>` — direct damage

```xml
<damage saving="kReflex" prob="100">
    <amount min="0" beta="111.2" weight="1"/>
    <hits skill_divisor="12" max="2" prob="100"/>
    <instant_death prob="100"/>
</damage>
```

| Attr / child | Default | Description |
|---|---|---|
| `saving` | **required** | `ESaving`: `kReflex`, `kStability`, `kWill`, `kCritical`, `kNone`. An absent `saving` fails to parse and drops the spell's whole action list, so always write it (`kNone` opts out). The save is rolled once up front and cached on the context as `last_saving_result` to gate `<instant_death>`; then **each hit** rolls magic resist (a resisted hit deals 0) and its own saving throw (a **success halves** that hit). `kNone` or a self-cast = no save (full damage). |
| `prob` | `100` | Percent chance the damage actually happens (silent miss otherwise). `prob<100` short-circuits the RNG. |
| `<amount min= beta= weight=>` | min=0, beta=1.0, weight=0 | Per-hit amount = `min + beta · C · (1 + weight · d)` (§3.7), where `C = skill_coeff + stat_coeff` and `d` is the cast's shared noise deviation. `weight` says how much of the spread this damage takes (`0` = deterministic). The result is then multiplied by the element coefficient, the caster's spell-power bonus, gear modifiers and the area coefficient. Omit the tag to keep the defaults. |
| `<hits skill_divisor= max= prob=>` | divisor=25, max=1, prob=20 | Multi-hit support: `count = 1 + CalcExtraHits(...)`. The extra-hit bonus uses the caster's potency-roll `base_skill`, scaled by `min(skill, 75) / skill_divisor`, capped at `max`, fired at `prob`%. `prob=0` means a uniform random pick between 0 and `extra`. Absent tag means a single hit. |
| `<instant_death prob=>` *(new mechanic)* | absent; `prob=100` | Marks the spell as able to **kill outright**; the `prob` percent is the chance the attempt is even rolled. See the note below. |

> **Saving throw — now live (issue.instant-death).** Regular spell damage used to be gated only by
> magic resist (`GET_MR`); the `<damage saving>` throw is now actually rolled, and a **successful save
> halves** the hit (rolled per hit, alongside magic resist). Set `saving="kNone"` to exempt a
> spell (no save → always full damage, and `<instant_death>` is not blocked).
>
> `<damage source="poison">` is an affect-side variant (damage-over-time ticks that skip magic resist,
> saves and wards) — see the Affect Manual.

> **`<instant_death prob="N"/>` *(new mechanic, issue.instant-death)*.** After the save is rolled, an
> instant-death attempt fires only when, in order: (1) the victim is **not a boss** (`EMobClass::kBoss`
> role); (2) the victim **failed** the saving throw; (3) the `prob` roll passes (percent, default 100);
> and (4) the caster **wins** the opposed luck/morale contest (`AgainstRivalRoll::OppositeLuckTest` —
> the inverse of the actor-morale-failure test, now callable standalone). On success the normal damage
> is irrelevant: an **NPC** is dealt enough to die (`hit + 11`, past the −11 death floor — killed, not
> merely incapacitated), and a **PC** is left at exactly **1 HP** (`hit − 1`, never actually killed).
> The lethal hit ignores armour / absorb / sanctuary / prism so the computed amount lands exactly.

---

## 6. `<points>` — restoration: HP / moves / thirst / full

```xml
<points extra="0" prob="100">
    <heal   npc_coeff="3" min="0"  beta="47.5" weight="1"/>
    <moves                min="20" beta="65.8" weight="1"/>
    <thirst               min="0"  beta="0"    weight="0"/>
    <full                 min="0"  beta="0"    weight="0"/>
</points>
```

The `<points>` tag (formerly `<heal>`, renamed in issue.mag-points)
carries up to four optional inner amount tags. Each amount independently
restores (or drains) one category from the same potency roll:

* **`<heal>`** — hit points. `extra="N"` on the outer `<points>` is an integer
  overheal cap in percent above max HP (issue.points-extra-cap): `0` caps at max,
  `20` allows 120%, `33` allows 133%. Has the optional
  `npc_coeff` attribute (default `1.0`) that boosts the amount when the
  caster is an NPC — legacy behaviour, designed to keep mob-cast heals
  punchy on minions.
* **`<moves>`** — movement points. Same attribute shape as `<heal>`
  minus `npc_coeff`.
* **`<thirst>`** — thirst (engine slot `THIRST`).
* **`<full>`** — hunger (engine slot `FULL`). Renamed from `<cond>` in
  issue.point-bugs to free the word "cond" for the overall
  `{DRUNK, FULL, THIRST}` triplet rather than just hunger.

| Outer `<points>` attr | Default | Description |
|---|---|---|
| `extra` | `0` | Integer overheal cap **in percent above** max HP (issue.points-extra-cap). `0` = strict cap at max; `20` = up to 120%; `33` = up to 133%. Affects only `<heal>`; the other categories saturate at their natural caps. |
| `prob` | `100` | Percent chance the whole points action fires. A failed roll restores zero across all four categories. |

| Inner amount attr | Default | Description |
|---|---|---|
| `min` | `0` | Flat amount. The shared amount formula never returns less than `max(0, floor(min))`, so a **negative** `min` yields 0 today — it no longer drains. |
| `beta` | `0` | Weight on `C = (skill_coeff + stat_coeff)` (the old `competencies_weight`). |
| `weight` | `0` | How much of the cast's shared noise `d` this amount takes (§3.7); `0` = deterministic. |
| `npc_coeff` (`<heal>` only) | `1.0` | Extra multiplicative boost when the caster is an NPC. Default `1.0` reproduces the legacy `*2` effective heal for mob casters; set to `0` to disable. (The loader parses it for `<heal>` only.) |

### Sign convention for thirst / full

In the engine the thirst/full fields are inverted — `0` is fully sated,
`kMaxCondition` (48) is "starving". The XML uses the gameplay-natural sign:

* **Positive XML value** = restore (less thirsty / less hungry).
* **Negative XML value** = make worse (more thirsty / more hungry) — but see the
  `min` note above: the amount formula currently clamps at 0, so this direction is
  unreachable from data today.

`CastToPoints` negates the computed amount before calling
`gain_condition`, so designers can read `<thirst min="50"/>` as
"restore 50 thirst points" without keeping the inverted engine field in
mind.

### Formula

Each amount, before clamps:

```
amount = round(min + beta · C · (1 + weight · d)), at least max(0, floor(min))   // C = skill_coeff + stat_coeff
amount += amount · (caster's spellpower bonus / 100)
if caster is NPC: amount += amount · npc_coeff   // 0 for non-heal categories
amount *= area coefficient;  heal only: gear spell-effect modifiers
```

Competence and the noise draw are computed **once per cast** from the spell's
`<potency_roll>` and shared across all four amounts — a single skill
check drives every category on the same cast.

### Per-category narration (issue.point-bugs)

Each non-zero category emits its own message — one `act()` per category
in `heal → moves → thirst → full` order. The keys are:

| Key | Fires when | Category percent fed to `{intensity}` |
|---|---|---|
| `kHealToVict` | `<heal>` produced a non-zero amount | `amount * 100 / max_hp` |
| `kMovesToVict` | `<moves>` produced a non-zero amount | `amount * 100 / max_moves` (signed; negative for drains) |
| `kThirstToVict` | `<thirst>` produced a non-zero amount | improve: `removed * 100 / current`; degrade: `-(cond_after * 100) / kMaxCondition` |
| `kFullToVict` | `<full>` produced a non-zero amount | same shape as kThirstToVict |

The legacy single-key `kPointsToVict` was retired — each spell's sheaf
specifies whichever per-category keys it wants (most heal spells just
override `kHealToVict`; drain spells like a future "exhaust" would
override `kMovesToVict`; etc.).

The intensity table (`lib/cfg/messages/ru/points_intensity.xml`) carries `<improve>`
and `<degrade>` grades for each category. Resolution walks the table
descending and returns the first tier `T` such that `T ≤ percent` —
unified for both directions. `<heal><degrade>` is intentionally empty
(heal is positive-only by design; a negative `<heal>` amount silently
no-ops). Thirst/full `<degrade>` thresholds are **negative** because the
percent formula is signed.

---

## 7. `<area>` — how many targets, and per-target falloff

> **Schema change (issue.area-cast).** The old `<victims>` / `<decay>` children
> are gone. `<area>` now has two children — `<targets>` (how many) and
> `<distribution>` (per-target falloff). Area and group casts share one pipeline:
> a group cast is just an area cast over the ally roster.

```xml
<area>
    <targets skill_divisor="15" dice_size="2" min="5" max="20"/>
    <distribution type="kStepped" decay="0.05" free_targets="3"/>
</area>
```

An `<area>` block tunes any action whose target selector fans out over more
than one character — the entry action of a `kMagAreas`/`kMagMasses`/`kMagGroups`
spell, or a later action with `target="kTarFoes"` / `kTarGroup` / `kTarMinions`
(§3.8.1). Without it (or with `max ≤ 0`) such an action hits the whole roster at full
strength.

### `<targets>` — the count

| attr | default | meaning |
|---|---|---|
| `skill_divisor` | 1 | divisor of the caster's skill — the **`<success_roll>` `base_skill`** — that scales the count (clamped ≥1) |
| `dice_size` | 1 | sides per random target-count die |
| `min` | 1 | floor for the count |
| `max` | 1 | cap on the **bonus** (total ∈ `[min, min+max]`); **`max ≤ 0` means "no limit"** — hit the whole roster |
| `stat_weight` | 0 | optional secondary-stat nudge from the potency roll's `stat_coeff` (0 = exactly the historical count) |

> **Every attribute is optional** and falls back to the default above. (A missing
> attribute must *not* be read as an empty string — that used to throw and silently
> drop the whole action; fixed in issue.area-spell-bug. So `<targets max="-1"/>`
> alone is valid and means "everyone, no scaling".)

`CalcTargetsQuantity = min + min(RollDices(skill / skill_divisor, dice_size) +
ceil(stat_weight · stat_coeff), max)`. The actual number hit is
`N = min(CalcTargetsQuantity, roster size)` (or the whole roster when `max ≤ 0`).

### `<distribution>` — the per-target coefficient `f_j`

`type` ∈ `{kUniform (default), kLinear, kStepped}`. For the j-th target (1-based,
target #1 = the primary victim / first ally, hit first) of `N`, the coefficient
`f_j` scales **both** the effect magnitude **and** the effective cast-success:

| `type` | `f_j` |
|---|---|
| `kUniform` | `1` — nothing decays (all targets full strength) |
| `kLinear` | `(N − j + 1) / N` — #1 full, linearly down to `1/N` |
| `kStepped` | `1` for `j ≤ free_targets`, else `max(0, 1 − decay·(j − free_targets))` |

`decay` and `free_targets` are read **only** for `kStepped`. `f_j` is always in
`[0, 1]` and never amplifies. **NPC casters bypass** the falloff (`f_j = 1`
everywhere). The `kMultipleCast` feat softens the decay (`decay ×= 0.6`) through a
`<talent_patch has_effect="kArea" op="modify">` in `feats.xml` — data, not code.

---

## 8. `<affects>` — impose a magical affect

The `<affects>` block imposes one or more **affects** on the action's targets. An
affect's *behaviour* — its `<flags>`, its stat `<apply>` list (and their
competence-scaling), stacking, random pools, and everything it *does* while it
lasts (damage-over-time, on-hit/on-kill/on-expire triggers, wards) — is defined on
the affect itself in `lib/cfg/affects/affects.xml` and documented in the
**[Affect Manual](AFFECT_MANUAL.md)**. This section covers only the **imposition**:
which affect, for how long, against what save.

```xml
<affects saving="kWill" resist="kMind" prob="100">
    <duration base="1" skill_divisor="15" min="1" max="6"/>
    <affect id="kSleep"/>
    <reposition pos="kSleep" stop_fight="false"/>
    <lag base="2" bonus_divisor="-1"/>
</affects>
```

Each `<affect id="…">` child names an `EAffect` (or, for a room spell, an
`ERoomAffect`) to impose — a block may name several (e.g. the `slow` spell imposes
`kSlow` **and** `kSlowdown`); `random="Y"` marks it a member of the spell's
random-one-of pool. The engine
reads that affect's flags and `<apply>` list from `affects.xml`; the spell
contributes only the imposition fields below plus the cast's stored potency
(which the affect's competence-scaled applies then read).

> **Migration note (spells.xml → affects.xml).** Older spells named the affect
> with a `type="…"` attribute and carried its `<flags>` and `<apply>` *inline*
> here. That data now lives on the affect. Inline `<apply>` is still honoured as a
> **fallback** for affects that have not yet been given their own applies; inline
> `<flags>` is obsolete (the instance's flags come from the affect). Author new
> content as `<affect id=>` + affect-owned data.

### 8.1 `<affects>` attributes

| Attribute | Default | Description |
|---|---|---|
| `saving` | `kReflex` | The save the victim makes to avoid the affect. `kNone` = no save. |
| `resist` | `kFire` | `EResist` — the resist channel applied to the affect's duration. |
| `prob` | `100` | Percent chance the block fires at all (silent miss otherwise). `<lag>`/`<reposition>` are gated by this same prob. |
| `potency_weight` | `1.0` | *Multiplier* on the stored `Affect::potency` (not the modifier) that `DispelSucceeds` reads as `affect_potency`. Lets a big-modifier affect stay dispellable: keep the modifier strong, scale only the stored potency down. Complements the **additive** dispel knobs — `<unaffect dispel_bonus=>` and the affect's own `<affect dispel_mod=>` (§9.3). |
| `battleflag` | *(none)* | `\|`-separated `EAffFlag` bits OR'd onto each imposed affect instance. With `kAfBattledec` the `<duration>` is read as raw combat rounds (no PC hour→tick scaling). |

The `tick_spell` / `tick_handler` attributes are **retired** (no longer read): a room
affect's per-tick behaviour is now its own `<actions>` in `room_affects.xml` with a
`<trigger val="kPulse">` — a `<side_spell>` action replaces `tick_spell`, a
`<manual_cast>` action replaces `tick_handler` (Affect Manual §7). An `<affects>`
block may also carry `<charges max="N"/>`: how many times a *triggered* affect fires
before it is removed (`-1` / absent = unlimited).

### 8.2 `<duration>` (skill-scaled)

```xml
<duration base="2" skill_divisor="5" min="0" max="0"/>
```

| Attr | Description |
|---|---|
| `base` | Flat base duration (hours; PC unit-converted to ticks). |
| `skill_divisor` | Per-`min(skill,75) / skill_divisor` hour bonus, using the spell's potency-roll `base_skill`. `0` = no skill scaling. |
| `min` / `max` | Clamps in hours. `0` on a side = no clamp there. |
| `skill` | *(affect-action only)* Names the duration-scaling skill when there is no casting spell (e.g. a triggered hangover scaling with `kHangovering`). For a spell cast the base_skill comes from `<potency_roll>`, so this is omitted. A triggered action without `skill` gets a flat duration. See Affect Manual §5.6. |

The skill bonus is capped at the novice threshold (75) / `skill_divisor`. `base`,
`skill_divisor`, `min` and `max` are all read unconditionally — write all four (a
missing one fails to parse and drops the spell's action list). The same holds for
`<lag base= bonus_divisor=>`.

### 8.3 `<reposition>` and `<lag>` — landing side-effects

```xml
<reposition pos="kSleep" stop_fight="false"/>
<lag base="6" bonus_divisor="-1"/>
```

`<reposition>` forces the victim to `pos` (an `EPosition`; omit to only stop the
fight) and, if `stop_fight="Y"`, stops everyone fighting them (the "peaceful"
effect). A **saved** cast with a `<reposition>` suppresses the generic "no effect"
line — the absent knockdown message is signal enough.

`<lag>` imposes battle-lag on the victim: `base` rounds, plus
`low_skill_coeff / bonus_divisor` extra when `bonus_divisor > 0` (`≤ 0` = flat).

### 8.4 The affect's own data — see the Affect Manual

`<flags>`, the `<apply>` stat-change list and its competence-scaling formula,
stacking, random apply pools, and the affect's trigger-driven `<actions>`
(DoT/HoT, on-hit/on-kill/on-expire/ward triggers) are all defined on the affect in
`affects.xml` — see **[Affect Manual](AFFECT_MANUAL.md) §4–§5**.

## 9. `<unaffect>` — dispels / cures *(new mechanics)*

```xml
<unaffect affect_flags="kAfCurable" dispel_bonus="50" prob="100" decay="0">
    <blocking      any_of="kGodsShield" all_of=""/>
    <breaking      any_of="kSanctuary"/>
    <remove_anyway any_of="kGlitterDust"/>
    <remove        all_of="kInvisible|kDisguise|kHide" breaking_by_failure="true"/>
</unaffect>
```

### 9.1 `<unaffect>` attributes

| Attribute | Default | Description |
|---|---|---|
| `affect_flags` | `kAfCurable\|kAfDispellable` | **Which affects this dispel may touch.** An affect is eligible only if it carries at least one of these `EAffFlag` bits. Narrow to `kAfCurable` for cures (cures don't dispel), `kAfDispellable` for `kDispellMagic` (doesn't cure poisons), etc. |
| `dispel_bonus` *(issue.random-noise-rework)* | `50` | **Base dispel win-chance, in percentage points, at competence parity** — a flat, level-independent probability offset (replaces the old `potency_weight` multiplier). The dispeller's competence advantage and the affect's own `dispel_mod` shift it from there. See §9.3. Debuff cures set ~80; leave 50 for a neutral dispel. |
| `prob` | `100` | Percent chance the unaffect block fires at all (silent skip otherwise). |
| `decay` *(issue.debuff-decay)* | `0` | On a **failed** removal, shift the surviving affect's potency by this percent of **the dispeller's competence contribution** (`kDispelSkillWeight · competence`): positive **weakens** the affect (easier next time), negative **strengthens** it. Result is clamped to `[0, 30000]` (floor 0 per design; the cap guards a negative `decay` from growing potency without bound / overflowing). `0` = no change. Applied only on the contest-fail path in `DispelSucceeds`. |
| `debuff_only` *(issue.new-unaffect-spells)* | `N` | `Y` restricts removal to non-buffs: affects marked as clear buffs are skipped, so a friendly wildcard cleanse never strips the target's own buffs. No effect on room targets. |

### 9.2 The four sub-blocks

Each of the four children carries two lists of **affect** names (issue.affect-migration:
the tags name the affect, not the casting spell) — `EAffect` names for a character
target, `ERoomAffect` names for a room target:

| Block | When checked | Effect |
|---|---|---|
| `<blocking>` | First | If any listed affect is present, **all `<remove>` entries are skipped** (but `<remove_anyway>` still runs). The chain is *not* broken. |
| `<breaking>` | First | If any listed affect is present, the cast **chain breaks** after this stage (this action's later stages on this target don't run, §9.4). |
| `<remove_anyway>` | Second | Affects to dispel **regardless of blocking**. Useful for "always strip the glitter dust even if shielded". |
| `<remove>` | Third (only if not blocked) | Normal removal list — these are dispelled when `<blocking>` didn't fire. |

Within each block:

| Attribute | Meaning |
|---|---|
| `any_of` | `\|`-separated affect list. The block fires if **any one** is present (for blocking/breaking) or dispels the **first one present** (for remove blocks). The single-token value `"*"` is the **wildcard** — see §9.5. |
| `all_of` | `\|`-separated affect list. For blocking/breaking, fires only when **all** are simultaneously present. For remove blocks, dispels **every** listed affect that is present. The single-token value `"*"` is the **wildcard** — see §9.5. |
| `breaking_by_failure` *(remove / remove_anyway only)* | If `true`, a **failed potency check** on any dispel from this block breaks the cast chain (§9.4). The default is `false` (a failed check is silent). |

Mixing `"*"` with explicit names (e.g. `any_of="*\|kPoisoned"`) is rejected
at parse time with a warning — the explicit names are redundant when the
wildcard is present, and the semantic is ambiguous. Use one or the other.

### 9.3 The dispel contest *(reworked — issue.random-noise-rework)*

Each affect imposed by `CastAffect` **records the cast's competence** — the caster's
**deterministic** skill+stat potency at the moment of imposition (`CalcCastPotency`, §3.7;
no noise leaks in). When `CastUnaffects` tries to remove it, the removal is a **d100
skill check**, not the old dice-vs-dice roll:

```
threshold = clamp( k·(area_coeff·C_dispel − affect_potency) + dispel_bonus + dispel_mod,  5,  95 )
removed   = number(1,100) ≤ threshold
```

| Term | Meaning |
|---|---|
| `C_dispel` | the dispeller's competence (skill+stat) for the dispel spell |
| `affect_potency` | the affect's stored competence (`stored_potency` below) |
| `dispel_bonus` | the `<unaffect dispel_bonus=>` knob (§9.1) — the **win-% at competence parity**, a flat, level-independent offset. Default 50. |
| `dispel_mod` | the affect's own `<affect dispel_mod=>` additive modifier (**Affect Manual** — it lives on the affect in `affects.xml`). Negative = harder to remove. Default 0. |
| `k` | `kDispelSkillWeight` (global, `4.0`). Turns one point of competence advantage into `k` percentage points; with the rebalanced range (~2..12) a ±10 gap ≈ ±40 points. |
| `clamp(5,95)` | symmetric **5 % upset floor and 5 % save ceiling** — nothing is ever a guaranteed strip or a guaranteed save. (Subsumes the old flat 5 % luck roll.) |

`stored_potency = cast_potency(affect_spell, caster_at_impose) · <affects potency_weight>`
(impose-time, §8.1). The affect-side `potency_weight` still scales the recorded competence —
a secondary *multiplicative* "hard/easy to out-muscle" knob that complements the additive
`dispel_mod`.

**Ruleset:**

1. **Non-violent dispel of a buff** (an ally cleansing a friendly buff) — **no contest**, the
   affect is removed. For an ambiguous (`A`) dispel like `kDispellMagic`, "non-violent"
   resolves per-target (§3.3): from an ally hand it passes; from an outsider it rolls.
2. **The d100 contest** otherwise — `number(1,100) ≤ threshold`.

Because the bonus is **additive**, it is a probability knob a designer can read off directly:
`dispel_bonus=50` → 50 % at parity, `80` → 80 %, `85`+ → near-certain. The competence gap then
shifts each contest up or down by `k` points, so a clearly stronger dispeller reliably beats a
weaker affect and vice-versa, with genuine uncertainty only near parity. (Unlike the old
*multiplier*, whose probability effect depended on the absolute potency level.)

**Category tuning — the shipped defaults:**

| Affect kind | Knob | Chance at parity |
|---|---|---|
| Debuff (curse, blind, hold, poison…) | its cure's `<unaffect dispel_bonus="80">` | **~80 %** (≈1.25 casts) |
| Ordinary buff (strength, armor, magic shield…) | none — the default | **~50 %** |
| Critical buff (fire/ice/air shield, sanctuary, prismatic aura, fly, water-breath) | `<affect dispel_mod="-35">` | **~15 %** |

**Worked example.** `kDispellMagic` (bonus 50) at competence parity vs a plain buff rolls a
threshold of `50` → 50 %. Vs sanctuary (`dispel_mod=-35`) the threshold is `50 − 35 = 15`
→ ~15 %. A dedicated `kRemoveCurse` (bonus 80) on a curse from an equal-skill enemy rolls
`80` → ~80 %, and a curer 10 competence points stronger hits the 95 % ceiling.

A failed contest emits `kNoeffect` to the caster (for a pure dispel spell — a `kMagAffects`
spell that also dispels remains silent on failure and still applies its own affect). When
nothing on the target matched the filter at all, a pure dispel emits `kUnaffectNoMatch`
instead, so players can tell "nothing to cure" from "the cure failed".

### 9.3.1 The contest for on-item suppressions — `DispelContest` *(issue.affect-suppression-dispell)*

An affect **suppressed on an item** (the `kSuppressed` obj-affect — see the Affect Manual,
"Suppression & restoration") is not on the character's `->affected` list, so it is lifted by a
separate `DispelContest` function rather than `DispelSucceeds` (§9.3). The weave-restoration spell
(`kWeaveRestoration`) uses it via
`<alter_obj handler="AlterWeaveRestore" collateral="first_suppressed" dispel_bonus="50" decay="25" bypass_noalter="Y"/>`:

```
threshold = clamp( k·(C_dispel − suppression_potency) + dispel_bonus,  5,  95 )
lifted    = number(1,100) ≤ threshold
```

Differences from §9.3:

* `suppression_potency` is read **directly** off the `kSuppressed` obj-affect (the competence of
  the dispel that imposed the suppression) — it is not on any character's affect list.
* **No** `dispel_mod` term (that lives on a character affect in `affects.xml`; the suppression is
  on the item).
* **No** "non-violent cleanse skips the contest" shortcut — it **always** rolls.
* `dispel_bonus` (`<alter_obj dispel_bonus=>`, default 50) and `k` (`kDispelSkillWeight` = 4) are
  the same.
* On a **miss** the stored suppression potency is **eroded** by `decay`% (`<alter_obj decay=>`) of
  the dispeller's strength — repeated casts wear the suppression down until it lifts.

On success the suppression is removed and the item's affect **auto-returns early** (the manual
counterpart of the timer-driven auto-return). The full suppression / obj-affect model is in the
Affect Manual.

### 9.4 The chain-break path *(new mechanic)*

Two ways to break the cast chain from an unaffect stage:

1. **Present-affect breaking** — `<breaking any_of="…">` finds a present
   affect; the chain breaks without any removal attempt.
2. **Failed-removal breaking** — `<remove … breaking_by_failure="true">`
   tries to dispel and the potency check fails on any candidate.

A broken chain means this action's later stages on this target — affect,
points, side spells, manual — **don't run** (damage has already run before the
dispel; `<alter_obj>` still runs, it stops only when the target died). Other
targets and later actions are not affected. Use this for "reveal first, then
debuff" patterns: if the victim resists having their invisibility stripped, the
follow-up debuff should also miss.

### 9.5 The `"*"` wildcard *(new mechanic)*

`any_of="*"` and `all_of="*"` extend `<remove>` / `<remove_anyway>` from
"fixed list of affect names" to "everything matching the unaffect's
`affect_flags` filter". The two wildcards have different semantics —
matching the explicit-list semantics extended:

| Wildcard | Selection |
|---|---|
| `any_of="*"` | Pick **one** eligible affect, uniformly at random. (Reservoir-sampled across the victim's affect list.) |
| `all_of="*"` | Queue **every** eligible affect on the victim for removal. |

"Eligible" means `AffectMatchesFlags(aff, unaffect.affect_flags)` —
i.e. the affect carries at least one of the `EAffFlag` bits listed in
the surrounding `<unaffect affect_flags=…>` attribute (and, with
`debuff_only="Y"`, is not a clear buff). Each candidate
the wildcard selects then goes through the same potency-gated
`DispelSucceeds` rule (§9.3), and `breaking_by_failure` works the same
way as for explicit lists.

**Use this for:**
- generic "dispel one random magic" / "cure all poisons" spells
  (`kDispellMagic` itself is now expressed this way — see below),
- future sphere-specific dispels: tag affects with a new `EAffFlag`
  bit (e.g. `kAfSorcerySphere`, `kAfHazeSphere`) and write a dispel
  with `<unaffect affect_flags="kAfHazeSphere"><remove all_of="*"/>`
  to strip every haze-sphere affect.

**`kDispellMagic` migrated example:**

```xml
<spell id="kDispellMagic" element="kLight" mode="kEnabled">
    <!-- … usual fields … -->
    <flags val="kMagUnaffects"/>
    <talent_actions>
        <action>
            <unaffect affect_flags="kAfDispellable" decay="25">
                <remove any_of="*"/>
            </unaffect>
        </action>
    </talent_actions>
</spell>
```

This replaced a dedicated `DispelRandomAffect()` helper in `magic.cpp`
(now deleted) — the runtime path is now identical to every other
dispel, just with wildcard-collected candidates instead of an explicit
spell list.

**A hypothetical "cure all poisons" (using a wildcard plus a narrower
`affect_flags` filter):**

```xml
<spell id="kCureAllPoisons" element="kLife" mode="kEnabled">
    <!-- … usual fields … -->
    <flags val="kMagUnaffects"/>
    <talent_actions>
        <action>
            <unaffect affect_flags="kAfPoison">
                <remove all_of="*"/>
            </unaffect>
        </action>
    </talent_actions>
</spell>
```

Every affect carrying the `kAfPoison` category flag (set in `affects.xml` on every
poison affect) gets a potency check; each is stripped or resisted independently.
(`kAfCurable` would be far wider: it marks every affliction a cure may touch.)

### 9.6 Cross-reference: the dispel pipeline at a glance

For each cast that runs `<unaffect>`:

1. Apply the `prob` gate (silent skip if missed).
2. Evaluate `<blocking>` and `<breaking>` (with wildcard support).
3. Collect candidates from `<remove_anyway>` (always) and `<remove>`
   (only when not blocked) — explicit lists or wildcards. Duplicates of the
   same affect are merged (one removal, one message). A non-violent cast
   (ally cleanse, self-cast) drops affects conferred by equipment.
4. If candidates exist, run the optional PK-action check, then per
   candidate: `DispelSucceeds` → `RemoveAffectAndAnnounce` or
   record a resisted attempt. If nothing matched at all, a pure dispel
   emits `kUnaffectNoMatch`.
5. If a `breaking_by_failure` candidate resisted, or a `<breaking>`
   condition was present, return `kBreak` to stop this action's remaining
   stages on this target (§9.4).
6. Otherwise return `kSuccess` and the cast continues to the next stage.

---

## 10. `<points>` and `<area>` see §6 and §7 above.

---

## 11. `CastToPoints`, `CastManual` (and the object stages)

These stages run dedicated logic in `src/gameplay/magic/` (`action_points.cpp`,
`action_handlers.cpp`, `action_summon.cpp`). They are **partly data-driven**:

* `CastToPoints` (HP / MV / thirst / full restore) is fully data-driven
  via the spell's `<points>` block (issue.mag-points). All four inner
  amounts (`<heal>`, `<moves>`, `<thirst>`, `<full>`) share a single
  potency roll and a single `prob` gate. Each non-zero amount fires its
  **own** message — one `act()` per category in heal/moves/thirst/full
  order, keyed `kHealToVict` / `kMovesToVict` / `kThirstToVict` /
  `kFullToVict` (see §6 "Per-category narration"). The legacy single
  `kPointsToVict` key was retired in issue.point-bugs.
* Object transforms are the data-driven `<alter_obj>` action (§3.8.7): a shared
  resolve+`kNoalter`+no-effect skeleton plus a per-spell handler (bless, curse,
  enchant, repair, acid, …) that does its own messaging (`kAlterObjToChar`, …).
  The old `kMagAlterObjs` flag is gone.
* Object creation is the data-driven `<obj_creation>` action (§3.8.6): load a
  base vnum + an optional post-load handler (food / light / weapon / armor).
  The old `kMagCreations` flag is gone.
* Summons are the data-driven `<summon>` action (§3.8.5) for the vnum-based
  ones (summon keeper / fire keeper / clone) and `<manual_cast>` handlers (§3.8.4)
  for the corpse and non-mob ones (animate dead, resurrection, guardian angel,
  mental shadow). Messages stay in `spell_msg.xml` (`kSummonToRoom`, `kSummonFail`,
  `kSummonNoCorpse`, etc.). The old `kMagSummons` flag is gone.
* `CastManual` runs a hand-coded handler, but **which** handler is now data-driven:
  name it with `<manual_cast handler="SpellX"/>` inside the action (§3.8.4). The
  handler body lives in `src/gameplay/handlers/`; the XML just selects and
  stage-gates it.

Besides the action blocks above, the data-driven parts you can tune for these
stages are: `<flags>`, `<targets>`, `<misc>`, `<mana>`, and the spell's messages.

### 11.5 Room affects (`kMagRoom`)

Room spells (the `kMagRoom` flag, dispatched through `CallMagicToRoom`) impose
**room affects** (`ERoomAffect`, defined in `lib/cfg/affects/room_affects.xml`) rather than
char affects. They reuse the `<affects>` imposition grammar of §8, but room affects
have their own trigger set (on-entry `kEnter`/`kEnterPC`/`kEnterNPC`, door triggers
`kPick`/`kOpen`/…) and **raw-pulse durations** (no hours→ticks conversion). The
per-tick effect is the room affect's own `<actions>` fired by `<trigger val="kPulse">`
— typically a `<side_spell>` to a linked `kService` spell or a `<manual_cast>`. See the
**[Affect Manual](AFFECT_MANUAL.md) §7** for the full room/exit affect grammar.

## 12. Messages (`lib/cfg/messages/ru/spell_msg.xml`)

Per-spell message strings, keyed by spell id (the sheaf) and `ESpellMsg`
type (`src/gameplay/magic/spell_messages.h`). `id="kDefault"` is the shared
sheaf — fallback for messages a specific spell hasn't overridden. Each sheaf
also carries the spell's Russian display name in `<name val="…"/>`.

```xml
<spell_msg>
    <msg_sheaf id="kDefault">
        <name val="!undefined!"/>
        <message type="kNoeffect" val="Ваши потуги оказались напрасными."/>
        <message type="kCantCastPosition" val="Вам вряд ли это удастся."/>
        …
    </msg_sheaf>
    <msg_sheaf id="kFireball">
        <name val="огненный шар"/>
        <message type="kFightDeathToChar" val="Ваш огненный шар испепелил $N3 дотла."/>
        <message type="kFightDeathToChar" val="Огненный шар окутал и скрыл в себе $N3."/>
        …
    </msg_sheaf>
    …
</spell_msg>
```

A sheaf may hold several messages of the same type; one of them is picked at
random on each emission.

### Common message keys

| Key | When emitted | Audience |
|---|---|---|
| `kNoeffect` | Generic "no effect" outcome (also: a dispel that matched something but lost the contest). | Caster |
| `kUnaffectNoMatch` | A dispel/cure found nothing matching its `<unaffect>` filter (§9.3). | Caster |
| `kAffImposedToChar` | An affect from this spell landed. | Affected char |
| `kAffImposedToRoom` | Same, to onlookers. | Room |
| `kHealToVict` | `<heal>` produced a non-zero amount in a `<points>` cast. | Target |
| `kMovesToVict` | `<moves>` produced a non-zero amount in a `<points>` cast. | Target |
| `kThirstToVict` | `<thirst>` produced a non-zero amount in a `<points>` cast. | Target |
| `kFullToVict` | `<full>` produced a non-zero amount in a `<points>` cast. | Target |
| `kReflectedToChar` | Reflection: spell bounced back. | Caster |
| `kReflectedToVict` | Reflection: target who reflected. | Original victim |
| `kReflectedToRoom` | Reflection: onlookers. | Room |
| `kCastForbiddenToChar` | Room-block fizzle: a `<weave/>` spell in a `kNoMagic` room, or the caster's room carries a `<blocking><room_flags val>` flag. | Caster |
| `kCastForbiddenToRoom` | Room-block fizzle: announcement to onlookers. Empty means stay silent. | Room |
| `kCantCastHereToChar`, `kCantCastHereToRoom` | `MayCastHere` refused (e.g. a violent spell in a peaceful room); warcries override with a louder variant. Empty room-side message stays silent. | Caster / Room |
| `kCantCastSleeping`, `kCantCastResting`, `kCantCastSitting`, `kCantCastFighting`, `kCantCastPosition` | Cast-position preconditions. | Caster |
| `kCantCastMaster`, `kCantCastSelfOnly`, `kCantCastNotSelf` | Target preconditions (the caster's charmer; `kTarSelfOnly`; `kTarNotSelf`). | Caster |
| `kCantCastNotAlly` | `kTarAllyOnly` target gate (§3.5) — victim isn't self / groupmate. | Caster |
| `kCantCastNotMinion` | `kTarMinionsOnly` target gate (§3.5) — victim isn't the caster's own NPC follower. | Caster |
| `kTargetUnavailable`, `kCantCastPeaceful` | The target is unavailable; a peaceful caster refuses to do harm. | Caster |
| `kCantCastSilenced` | `<verbal/>` spell while the caster is under `kSilence` (§3.9). | Caster |
| `kComponentUse`, `kComponentMissing`, `kComponentExhausted` | `<material>` component used / missing (cast aborts) / used up and destroyed. Sheaf-direct, no `kDefault` fallback. | Caster |
| `kAreaToChar`, `kAreaToRoom`, `kAreaToVict` | Area cast announcement. | Various |
| `kCastIncantToChar` | The "you uttered the spell …" line `SaySpell` prints to the caster (`{color}`/`{name}`/`{nrm}` placeholders); warcries override with the shouting variant. | Caster |
| `kCastPhraseHeathen`, `kCastPhraseChristian` | The incantation phrase spoken while casting, chosen by the caster's religion (poly / mono). | Room |
| `kCastSayToSelf`, `kCastSayToOther`, `kCastSayToObj`, `kCastSayToSomething`, `kCastSayDamageeToVict`, `kCastSayHelpeeToVict`, `kCastSaySound` | The fly-by "$n stares at $N3 and utters …" lines emitted while the caster pronounces the incantation (`kCastSaySound` replaces them all when the caster can't speak). | Room / victim |
| `kCastDisappearToRoom`, `kCastAppearToRoom` | Teleport-style relocation: the actor leaves one room and appears in another (recall, teleport, relocate, summon, …). | Room |
| `kCastPreparedToChar` | Spell-ready announcement when a controlled spell replaces the previous one. | Caster |
| `kNoTarget`, `kWrongTarget` | The caster's target argument missed (no such target / target type rejected by `<targets val>`). | Caster |
| `kOfferingToChar`, `kOfferingToRoom` | Prayer / sacrifice spells only (`kPrayer`, `kOffering`): what the praying character and the room see when the offering is made; `{gift}` is what was offered (an item or an amount of a currency, accusative), `{choice}` the god or saint (the chosen action's canonical word). Printed by the "молиться"/"жертвовать" command, not by the cast. | Caster / Room |
| `kDefaultOfferingToChar`, `kDefaultOfferingToRoom` | The same for an offering without a gift, paid by the default amount from `mechanics/piety.xml` (a Christian's candle). | Caster / Room |
| `kUnknownChoice` | A spell with selectable actions (§3.8.8) was cast without naming one, or with an unknown word; `{choices}` is the list of canonical words. `kDefault` carries the generic line. | Caster |
| `kItemNoPrototype`, `kItemCreatedToChar`, `kItemCreatedToRoom`, `kItemCreationFailToChar` | `<obj_creation>` outcomes. | Various |
| `kObjResist`, `kAlterObjToChar` | `<alter_obj>`: the item is no-alter / the per-spell "object transformed" line. | Caster |
| `kAcidCorrodeObj`, `kRemovePoisonUnknown`, `kRemovePoisonRotten` | Per-spell object outcomes (acid corrodes an item; remove-poison on an item without a prototype / with rotten contents). | Caster |
| `kEnchantNotWeapon`, `kEnchantMagic`, `kEnchantSetItem`, `kEnchantMono`, `kEnchantPoly`, `kEnchantOther` | Enchant-weapon outcomes. | Caster |
| `kSummonToRoom`, `kSummonFail`, `kSummonNoCorpse`, `kSummonCharmed`, `kSummonNoProto`, `kSummonWarhorse` | Summon outcomes. | Various |
| `kResurrectBadCorpse`, `kResurrectConsecrated`, `kResurrectNoPower`, `kResurrectProtected` | Resurrection outcomes. | Caster |
| `kKnockdownToChar`, `kKnockdownToRoom` | Shared knockdown narration. | Victim / Room |
| `kFightDeathToChar`, `kFightHitToChar`, `kFightMissToChar`, `kFightGodToChar` (+ `ToVict`, `ToRoom`) | Combat-damage messages keyed by `ESpell`. | Various |
| `kCustomMsgOne` … `kCustomMsgTen` | Generic per-spell slots for one-off lines without their own enum constant (e.g. kSummon's secondary failure narration). Also carry room-affect per-tick narration, cycled by `Affect::apply_time` — one slot per tick phase (kDeadlyFog uses 8). | Various |

Messages are stored **without** the trailing `\r\n` — `act()` and
`SendMsgToChar` add it themselves.

A message lookup tries the spell's own sheaf first, then falls back to
`kDefault`. The exceptions worth knowing:

* **Imposition** (`kAffImposedToChar` / `kAffImposedToRoom`), **reflection**
  (`kReflectedToChar` / `kReflectedToVict` / `kReflectedToRoom`) and the
  **material-component** keys (`kComponent*`) are looked up sheaf-directly
  with **no kDefault fallback** — a missing key stays silent rather than
  leaking a wrong default.
* **Dispel and expiry narration** is not a spell message: it belongs to the
  affect and lives in `lib/cfg/messages/ru/affect_msg.xml` (character affects) or
  `lib/cfg/messages/ru/room_affect_msg.xml` (room affects), keyed by the affect id.
  See the Affect Manual.
* `kCastForbiddenToChar` falls back to `kDefault` (every spell shares the
  generic "Ваша магия потерпела неудачу…" line by default), and
  `kCastForbiddenToRoom` is treated as optional — an empty result means
  "announce nothing to the room", letting spells like `kRuneLabel`
  override only when they want their own narration.

---

## 13. Worked examples

Worked examples now live in a dedicated **[Cookbook](COOKBOOK.md)** — a flagship
cross-system example (a spell that imposes a multi-trigger affect whose on-expiry
payload blights the bearer's allies, plus feats that reshape it) and a gallery of
shorter recipes.

## 14. Reference: enum cheat-sheet

The full enums live in C++ headers; the most-used values are listed here.

### 14.1 `ESaving`

`kReflex` · `kStability` · `kWill` · `kCritical` · `kNone`

### 14.2 `EResist`

`kFire` · `kAir` · `kWater` · `kEarth` · `kVitality` · `kMind` ·
`kImmunity` · `kDark` (`src/gameplay/mechanics/resist.h`)

### 14.3 `EApply` (a curated subset — the full list is in `src/gameplay/affects/affect_contants.h`)

`kNone` · `kStr`/`kDex`/`kCon`/`kInt`/`kWis`/`kCha` · `kHitroll` · `kDamroll`
· `kAc` · `kMorale` · `kPoison` · `kManaRegen` · `kHpRegen` · `kMoveRegen` ·
`kCastSuccess` · `kSavingReflex`/`kSavingStability`/`kSavingWill`/
`kSavingCritical` · `kResistFire`/`kResistAir`/`kResistWater`/`kResistEarth`/
`kResistDark`/`kResistVitality`/`kResistMind`/`kResistImmunity`

### 14.4 `EPosition`

`kDead` · `kPerish` · `kIncap` · `kStun` · `kSleep` · `kRest` · `kSit` ·
`kFight` · `kStand` · `kUndefined` (used by `<reposition>` to mean "no
position change, only `stop_fight`").

### 14.5 `EAlign`

`kAny` (default — no alignment check) · `kGood` · `kEvil` · `kNeutral`.
The thresholds for "good / evil / neutral" come from
`alignment ≤ kAligEvilLess` (−300) and `alignment ≥ kAligGoodMore` (300) in
`src/gameplay/mechanics/alignment.h`.

### 14.6 `EAffFlag` (affect behaviour flags)

Now defined on the affect (`affects.xml <flags>`), not the spell — see the
**[Affect Manual](AFFECT_MANUAL.md) §4.1** for the full list and the
decrement-timing rules (`kAfBattledec`/`kAfPulsedec`/`kAfSameTime`).

### 14.7 `EMobFlag` allowed in `<blocking><mob_flags val>` / `<required><mob_flags val>`

`kNoBlind` · `kNoSleep` · `kNoSilence` · `kNoHold` · `kNoBash` · `kNoCharm`
· `kNoSummon` · `kNoFear` · `kCorpse` · `kMounting` · `kHelper` · `kClone` ·
`kIgnoreForbidden` · `kCompanion`. Add new ones to the `kBlockingFlagByName` table in
`src/gameplay/abilities/talents_actions.cpp` to expose more. (`kResurrected` is not a
mob flag any more: it is the runtime mark `<extra_flags val="kResurrected"/>`,
table `kBlockingExtraFlagByName`.)

### 14.7b `ERoomFlag` allowed in `<blocking><room_flags val>`

`kNoMagic` · `kPeaceful` · `kTunnel` · `kNoTeleportIn` · `kNoTeleportOut` ·
`kNoBattle` · `kGodsRoom` · `kForMono` · `kForPoly`. The room check matches against the
**caster's** current room, fires *before* per-target dispatch, and emits
`kCastForbiddenToChar` / `kCastForbiddenToRoom` (see §12). Add new ones
to the `kBlockingRoomFlagByName` table in
`src/gameplay/abilities/talents_actions.cpp` to expose more.

### 14.8 `EAffect` (the affect ids named by `<affect id=>`)

The full catalogue and each affect's behaviour is the
**[Affect Manual](AFFECT_MANUAL.md)**; enum source in
`src/gameplay/affects/affect_contants.h`. Some commonly referenced ids:

`kCharmed` · `kPoisoned` · `kInvisible` · `kDisguise` · `kHide` ·
`kBlind` · `kSleep` · `kHold` · `kSilence` · `kSanctuary` · `kPrismaticAura` ·
`kMagicGlass` · `kMagicShield` · `kShadowCloak` · `kAirShield` ·
`kFireShield` · `kFireAura` · `kIceShield` · `kGodsShield` · `kPeaceful` ·
`kStopFight` · `kMagicStopFight` · `kInfravision` · `kDetectInvisible` ·
`kDetectLife` · `kGlitterDust` · `kCurse` · `kProtectFromDark` ·
`kHelper`.

Use `kUndefined` (the legacy data spelling `kUndefinded` is still accepted) when an
`<apply>` should change a stat *without* imposing an affect flag.

---

## 15. Quick checklist for adding a new spell

1. **Pick an `ESpell` id.** Add it to `spells_constants.h` if it's truly
   new (engineer task). **Prototyping?** Use one of the reserved
   `kTestTwo … kTestFive` slots instead — see §17 — to iterate on the
   XML alone without touching code.
2. **Add the `<spell>` element** to `lib/cfg/spells.xml`. Required:
   header, `<name val>`, `<misc>`, `<mana>`, `<targets>`, `<flags>`.
3. **Pick the stages** you want by listing the `kMag…` flags in
   `<flags val>`. Multiple stages allowed.
4. **Author `<potency_roll>`** — `<base_skill>` and/or `<base_stat>`, plus
   `<noise>` if the effects should vary. Without them `C = 0` and the spell
   delivers only its flat `min` amounts.
5. **Write the `<talent_actions>` block** — gates first, then effects. For a
   multi-action spell, list the actions in execution order; give actions 2+ a
   `target=` (and `base=` if they should chain off an earlier result), and an
   `<area>` for any action that fans out (§3.8, §7, [Cookbook](COOKBOOK.md) §3.8).
6. **Add a `<msg_sheaf>`** to `lib/cfg/messages/ru/spell_msg.xml`: at minimum the
   Russian `<name val>` and the imposition messages if the spell adds affects, or
   the damage description messages if it deals damage. Dispel/expiry lines belong
   to the affect (`affect_msg.xml`, §12).
7. **Refresh the build snapshot** (see §2.3) and boot-test:
   `./circle -d small <port>`. **Read the right log:** boot SYSERR goes to
   `build/syslog` (append-mode) and `build/log/errlog.txt`, **not** to stdout
   (stdout only has the startup banner). Truncate `build/syslog` first, boot, then
   `grep SYSERR build/syslog` (or use `reload spells` on a running server) — a malformed `<action>` is swallowed-and-logged
   (the whole action list is dropped) rather than crashing, so syslog is the only
   place the error appears. Look for `Incorrect value '' in 'talent_actions'`
   (a required attribute read as empty) and lines mentioning your spell's tags.

---

## 16. What this manual does **not** cover (yet)

* **Hand-coded handler *bodies*** (the `<alter_obj>` / `<obj_creation>` / `<summon>`
  post-spawn handlers, and the `<manual_cast>` handlers themselves) — those still
  live in code (`src/gameplay/handlers/`, registries in
  `src/gameplay/magic/action_handlers.cpp` and `action_summon.cpp`). Their *selection and ordering* are now
  data-driven (§3.8.4–§3.8.7), but the logic inside is still code. Migrating it
  piece by piece into the data-driven system is ongoing work.
* **Success roll consumption** — of `<success_roll>` only the `base_skill`
  id is used (skill training, a `skill / 20` bonus in `CalcCastSuccess`, the
  `<area>` count, §3.7). Its weights, `<bonus>` and `<thresholds>` are not yet
  wired into the cast-success decision; the classical percent-based
  `CalcCastSuccess` still gates landing.

Perk→spell modifiers are **no longer** hard-coded: feats such as `kMagicArrows`
(doubled `kMagicMissile` extra hits) and `kMultipleCast` (softer area decay) are
`<talent_patch>` entries in `feats.xml` — see the [Feat Manual](FEAT_MANUAL.md).

Affects and their triggers, and feats, are **no longer** in the "not covered" list —
they have their own companion manuals ([Affect](AFFECT_MANUAL.md) / [Feat](FEAT_MANUAL.md)).

If you need any of these, open an issue and the work can be scoped.

---

## 17. Test spell constants for prototyping

To let designers iterate on a new spell's XML **without recompiling
the engine**, placeholder `ESpell` slots are reserved. The first one (359, formerly
`kTestOne`) has graduated into `kFireTrap` (a door-affect spell still in
`kTesting`); four remain free:

| Constant | id | Russian display name |
|---|---|---|
| `kTestTwo`   | 360 | "тест 2" |
| `kTestThree` | 361 | "тест 3" |
| `kTestFour`  | 362 | "тест 4" |
| `kTestFive`  | 363 | "тест 5" |

Each slot ships in `lib/cfg/spells.xml` as a minimal stub:

```xml
<spell id="kTestTwo" mode="kTesting">
    <name val="test 2"/>
</spell>
```

plus its sheaf in `spell_msg.xml`: `<msg_sheaf id="kTestTwo"><name val="тест 2"/></msg_sheaf>`.

`mode="kTesting"` keeps the slot out of class spell-lists in production
(it's loaded but not granted) while still letting an immortal force-cast
it for verification.

**Workflow:**

1. Pick a free slot (`kTestTwo` … `kTestFive`).
2. Edit its `<spell>` element in `spells.xml` — give it real
   `<misc>`, `<mana>`, `<targets>`, `<flags>`, `<potency_roll>`, and
   the `<talent_actions>` block you want to try (keep
   `mode="kTesting"`).
3. Add per-spell messages to the slot's sheaf in `spell_msg.xml`; optionally
   replace its Russian `<name val>` with your prototype's working title.
4. Refresh the build snapshot (see §2.3) and boot-test:
   `./circle -d small <port>`, log in as an immortal, force-cast it
   on a dummy.
5. Iterate on the XML alone — no rebuilds needed beyond the snapshot
   refresh (or `reload spells` / `reload spellmsg` on a running server).
6. When the prototype graduates, an engineer renames the constant +
   the XML `id=` in both `spells.xml` and `spell_msg.xml` to its final name
   (e.g. `kTestOne` → `kFireTrap`),
   flips the mode to `kEnabled`, and adds it to the appropriate class
   spell-lists. That's a single rename pass — all the design work
   already done against the test slot transfers verbatim.

Keep test slots short-lived: revert them to the minimal stub once your
prototype graduates (or once you abandon it), so the next designer
finds them empty.

---

*Last updated for the `issue.unstable-hotfixes` / `issue.spell-ally-aggression` /
`issue.debuff-decay` / `issue.instant-death` batch on `unstable` (changes since `7cc7cc4d4`):*

- **`<damage><instant_death prob="N"/>`** *(new, §5)* — lethal-strike: on a **failed** save, a `prob`
  roll, and a **won** caster luck/morale contest (`OppositeLuckTest`), the spell kills an NPC outright
  (`hit + 11`, past the −11 death floor) or leaves a PC at exactly 1 HP (`hit − 1`); **never** on
  `EMobClass::kBoss` mobs.
- **The `<damage saving>` throw is now live (§5)** — previously regular spell damage was gated only by
  magic resist; the save is now actually rolled (once, cached as `ctx.last_saving_result`) and a
  **success halves** the damage. `saving="kNone"` opts a spell out. The same single roll gates the
  instant-death attempt above.
- **`<unaffect decay="N">`** *(new, §9.1)* — a **failed** dispel shifts the surviving affect's potency
  by N% of **the dispel's** potency (positive weakens / negative strengthens, clamped `[0, 30000]`).
- **`<alter_obj>` collateral item is now opt-in via `collateral="on_damage"` (§3.8.7, issue.spells-hotfix)**
  — supersedes the earlier violent-only rule. A spell cast on a *character* alters a random carried item
  only if its `<alter_obj>` sets `collateral="on_damage"` **and** the cast dealt damage this stage; only
  `kAcid`/`kAcidArrow` opt in. So `kCurse`/`kPoison` aimed at a character no longer touch their gear
  (cast directly on an object to affect one).
- **First aid (`лечить`) cures only harmful affects (issue.spells-hotfix)** — `PickCureTarget` now also
  requires the affect's runtime `debuff` flag, so a cure never strips the target's own buffs (many buffs
  also carry `kAfCurable`). Cooldown now applies on success too (was failure-only). Paired data fix:
  `kAfCurable` was removed from all buff affects in `spells.xml` — it now marks afflictions only.
- **Dispel/cure of a multi-affect type announces once (issue.spells-hotfix)** — `RunCastUnaffects` dedups
  the removal queue by spell type, so removing e.g. `kPoisoned` (several locations) strips all of it and
  shows one message instead of one per affect.
- **Reposition affects suppress the "no effect" line on a save (issue.spells-hotfix)** — a saved
  knockdown/stun shows nothing rather than "no effect"; the absent knockdown line already signals failure.
- **Benign / ally casts are not PK actions** — a non-violent (or ambiguous-resolved-helpful) spell on
  an ally no longer trips `pk_agro_action`; the clan-castle "protective magic" now evicts the caster
  only on a genuine hostile PK classification (not on a benign cast or a PvE action).
- **Animate dead no longer marks its minions `kResurrected`** — that flag identifies a corpse revived
  by `kResurrection` (reusing the original mob's vnum); animate-dead summons are fresh necro mobs
  (still `kUndead`).

*Earlier — last updated to match the `sventovit.work` head after the
`issue.ambiguous-spells` / `issue.dispel-default-msg` /
`issue.weave-component` / `issue.caster-blocking-refine` /
`issue.affects-potency-weight` sequence: `<misc violent>` gained a
third value `A` ("ambiguous") that resolves per-target via
`IsViolentAgainst(caster, target)` — helpful on self / charmed pet /
groupmate / NPC-vs-NPC, fully violent (with full PK liability) on an
outsider (§3.3, kDispellMagic is the first carrier); a new
`<components>` block at the `<spell>` level (§3.9) collects
`<verbal/>` + `<weave/>` + `<sight/>` + `<material>` requirements, with
`<weave/>` now the single source of truth for "is this magic" and
the canonical `kNoMagic`-room gate (replacing 209 copies of the
data-driven blocking pattern), and `<sight/>` blocking a blind caster
(issue.sight-component); caster gating moved to a spell-level
`<caster_conditions>` block wrapping the same `<blocking>`/`<required>`
shape as the target side (§4.3), replacing the old single-tag
`<caster_blocking>` (so a held or wrong-alignment caster can be locked
out via `<align>`/`<affect_flags>`); a new
`<affects potency_weight="0.4">` attribute (§8.1) scales the stored
Affect potency at impose time so big-modifier spells stay dispellable
(complementing the additive `<unaffect dispel_bonus=>` and `<affect dispel_mod=>`
levers on the dispel side); and
the `kAffDispelledToChar` / `kAffDispelledToRoom` keys now fall back
to the kDefault sheaf so every dispel emits *something* — previously
most affects (kSanctuary / kBless / kArmor / kFly / etc.) were stripped
silently because their sheaves never authored these keys (§12).
Carries forward earlier additions: `<cond>` → `<full>` under `<points>`,
the per-category `kHealToVict` / `kMovesToVict` / `kThirstToVict` /
`kFullToVict` keys, the migrated `kCastSay*` / `kCastIncantToChar` /
`kCastHereForbidden*` / `kNoTarget` / `kWrongTarget` /
`kCastPreparedToChar` / `kCastInterruptedToChar` / `kSummonFail`
narration family + `kCustomMsgOne…Ten`, the `BYLINS_FIRSTAID_NEW`
First Aid rework, and the `kTestOne…kTestFive` prototyping slots
(§17).*
