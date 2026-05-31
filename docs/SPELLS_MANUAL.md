# Bylins MUD — Data-Driven Spell Manual

*For game designers authoring spells in `lib/cfg/spells.xml` and the companion
`lib/cfg/spell_msg.xml`.*

This manual catalogues every tag, attribute, and value the data-driven spell
system recognises today. After reading it you should be able to add a new
spell, tune an existing one, or compose any of the mechanics — damage, heal,
affect, dispel, stacking, alignment-restricted, reflected — without touching
C++.

For motivation and per-issue history, see the commit log on `sventovit.work`.
This document focuses strictly on what *you, the designer, can configure*.

---

## 1. The cast pipeline at a glance

When a player casts a spell, the engine runs the following stages **in this
order** for each target. Any stage can short-circuit the whole cast.

1. **Cast preconditions** — mana, position, sleeping, fighting, etc.
2. **Target gates** (`<blocking>`, `<required>`, `<caster_blocking>`) —
   data-driven action-level checks. Failure aborts this target silently
   (group/mass casts) or with a "no effect" message (single-target).
3. **Reflection** (`<reflection>`) — if the target carries a reflecting flag
   or alignment, the spell may bounce back at the caster (one prob roll,
   covers all subsequent stages).
4. **`CastDamage`** — if `kMagDamage` flag set.
5. **`CastUnaffects`** — if `kMagUnaffects` flag set. Runs *before* affect so
   a spell can dispel an existing affect and then apply its own.
6. **`CastAffect`** — if `kMagAffects` flag set.
7. **`CastToPoints`** — if `kMagPoints` flag set (HP/MV restore).
8. **`CastToAlterObjs`** — if `kMagAlterObjs` flag set (modify an item).
9. **`CastSummon`** — if `kMagSummons` flag set (animate dead, clone, …).
10. **`CastCreation`** — if `kMagCreations` flag set (conjure food/light).
11. **`CastManual`** — if `kMagManual` flag set (hand-coded handler in
    `spells.cpp`).
12. **Victim reaction** — NPC hostility / auto-cast of detection.

Stages 4–11 are gated by the spell's `<flags val="kMag…">` bitset, so a single
spell can run any subset of them in the order above. Each stage may return a
"break" code that ends the chain.

---

## 2. File layout

### 2.1 `lib/cfg/spells.xml`

Defines the spells themselves. Top-level element is `<spells>`. Each spell is
one `<spell>` child. Spell IDs come from the `ESpell` enum
(`src/gameplay/magic/spells_constants.h`).

```xml
<spells>
    <spell id="kFireball" element="kFire" mode="kEnabled">
        <name rus="огненный шар" eng="fireball"/>
        <misc pos="kFight" violent="Y" danger="1"/>
        <mana max="80" min="60" change="1"/>
        <targets val="kTarCharRoom|kTarFightVict"/>
        <flags val="kMagDamage|kNpcDamagePc"/>
        <potency_roll> … </potency_roll>
        <success_roll> … </success_roll>          <!-- optional -->
        <talent_actions>
            <action>
                <!-- gates: blocking / required / caster_blocking / reflection -->
                <!-- effects: damage / heal / area / affects / unaffect -->
            </action>
        </talent_actions>
    </spell>
    …
</spells>
```

Encoding is **KOI8-R**. Edit through `iconv` if your editor doesn't speak it
natively.

> **Note on the XML reader.** The Bylins `DataNode` parser has *no inner-text
> support*. Every value must live in an attribute (`val="…"`, `min="…"`,
> etc.). Whitespace inside a tag is ignored.

### 2.2 `lib/cfg/spell_msg.xml`

Per-spell message strings, keyed by spell id and message type (`ESpellMsg`).
See §13.

### 2.3 Small-world snapshot caveat

`./circle -d small` reads from `build/<dir>/small/cfg/`, which is a
**snapshot** of `lib/cfg/` taken once at `meson setup` time. After editing
`lib/cfg/*.xml`, refresh the snapshot before booting:

```bash
for f in spells.xml spell_msg.xml hit_msg.xml; do
  cp "lib/cfg/$f" "build/<your-build>/small/cfg/$f"
done
```

Otherwise your edits won't be loaded and you may see ghost "incorrect
constant" SYSERRs from old values.

---

## 3. The `<spell>` element

### 3.1 Header attributes

| Attribute | Required | Description |
|---|---|---|
| `id` | yes | `ESpell` enum value, e.g. `kFireball`. |
| `element` | no | `EElement`: `kFire`, `kAir`, `kWater`, `kEarth`, `kLife`, `kDark`, `kLight`, `kMind`, `kUndefined`. Used for resist/element-coefficient calcs. |
| `mode` | no | `EItemMode`: `kEnabled` (default), `kDisabled`, `kService` (engine-only stub). |

### 3.2 `<name>`

```xml
<name rus="огненный шар" eng="fireball"/>
```

| Attribute | Description |
|---|---|
| `rus` | Russian display name. Used in-game. |
| `eng` | English name. Used for admin tools and logs. |

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
| `A` | **Ambiguous.** Resolves per-target: helpful on self / charmed pet (walking up the master chain) / groupmate / NPC-vs-NPC, fully violent (with full PK liability) on an outsider. `kDispellMagic` is the current carrier — dispel from an ally hand passes without a contest; dispel from an outsider triggers the strength check. |

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
| `change` | Per-level decrease as the caster levels up. |

### 3.5 `<targets val="…">`

Pipe-separated bitset of `ETarget` flags. Pick at least one of the "where to
look" flags plus any check flags.

| Flag | Meaning |
|---|---|
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
| `kMagAlterObjs` | Run `CastToAlterObjs` (item transform). |
| `kMagSummons` | Run `CastSummon`. |
| `kMagCreations` | Run `CastCreation`. |
| `kMagManual` | Run hand-coded handler in `spells.cpp` (also covers movement spells like teleport, recall, portal, relocate). |
| `kMagGroups` | Spell targets a group. |
| `kMagMasses` | Mass cast — every NPC in the room (excluding allies). |
| `kMagAreas` | Area cast — every hostile in the room with decay. |
| `kMagWarcry` | Marks the spell as a war-cry (skips magic defences). |
| `kMagNeedControl` | Requires caster's pet to be present. |
| `kMagRoom` | Room spell, lives in `magic_rooms.cpp`. |

| PK / agro / damage class | Effect |
|---|---|
| `kNpcDamagePc` | PvE-direction damage: NPC may damage PC. |
| `kNpcDamagePcMinhp` | Damage capped to leave the PC with some HP (no kill). |
| `kNpcAffectPc` | NPC may apply an affect to a PC (PK gate). |
| `kNpcAffectNpc` | NPC may apply an affect to another NPC. |
| `kNpcUnaffectNpc` | Mirror for dispels. |

### 3.7 `<potency_roll>` and `<success_roll>`

Both share the same shape (`talents_actions::Roll`). Each spell has **one
potency roll** that all of its effects share (damage amount, affect modifier,
duration bonus, extra-hits bonus, dispel strength).

```xml
<potency_roll>
    <dices ndice="6" sdice="4" adice="10"/>
    <base_skill id="kFireMagic" low_skill_bonus="3" hi_skill_bonus="0.75"/>
    <base_stat id="kWis" threshold="22" weight="5"/>
</potency_roll>
```

| Child / attr | Description |
|---|---|
| `<dices ndice= sdice= adice=>` | The dice expression: `ndice` dice of `sdice` sides plus `adice` flat. Any zero attribute means "no contribution from that part" — `ndice="0" sdice="0" adice="N"` reliably returns `N`, and an absent `<dices>` block is equivalent to `0,0,0` (returns 0). Negative values are also treated as zero. |
| `<base_skill id= low_skill_bonus= hi_skill_bonus=>` | Adds a skill-based coefficient. Below the novice threshold (skill 75), each point grants `low_skill_bonus`; above 75, each point grants `hi_skill_bonus`. Final coefficient = `(low_skill·low_bonus + hi_skill·hi_bonus) / 100`. Omit the child to skip skill contribution. |
| `<base_stat id= threshold= weight=>` | Adds a stat-based coefficient. Contribution = `max(0, stat − threshold) · weight / 100`. Omit the child to skip stat contribution. |

`<success_roll>` is parsed and evaluated but **not yet consumed** by any game
mechanic — it's a forward-looking hook. Use the same shape when you want a
spell to carry a success/landing roll separate from its potency.

The `base_skill` from `<potency_roll>` also drives the affect duration and
multi-hit count (see §7 and §6.1).

#### The `cast_potency` value

Every place in the manual that talks about "the cast's potency" — the affect
modifier formula, the stored Affect potency, the dispel contest, the
damage / heal / extra-hits scaling — uses the same scalar:

```
cast_potency = RollSkillDices + skill_coeff + stat_coeff
```

Where:

* `RollSkillDices` = result of the `<dices>` roll (one `ndice × sdice + adice`
  sample per cast).
* `skill_coeff` = `(low_skill · low_skill_bonus + hi_skill · hi_skill_bonus) / 100`
  from `<base_skill>` (skill is capped at `kNoviceSkillThreshold = 75` for the
  low part; everything above goes into the hi part).
* `stat_coeff` = `max(0, stat − threshold) · weight / 100` from `<base_stat>`.

A spell that omits a sub-block contributes 0 from that side. A spell with an
empty `<potency_roll>` rolls a flat 0 — its affects land at potency 0, its
dispel always fails the contest (5 % luck floor still applies).

### 3.8 `<talent_actions>`

Container for one or more `<action>` blocks. Each `<action>` can declare any
mix of:

* Gates: `<blocking>`, `<required>`, `<caster_blocking>`, `<reflection>`
* Effects: `<damage>`, `<points>`, `<area>`, `<affects>`, `<unaffect>`

Most spells use a single `<action>`. Multiple actions are technically allowed
but the framework processes only one of each effect type, so the common
pattern is a single `<action>` with everything inside it.

### 3.9 `<components>` — casting requirements

```xml
<components>
    <verbal/>
    <weave/>
    <material any_of="1234|5678" cost="1"/>
</components>
```

Optional child of `<spell>`. Each inner tag is a presence-only marker or a
material-component descriptor; absent `<components>` means "no requirements".
Three kinds today:

| Tag | Attributes | Effect |
|---|---|---|
| `<verbal/>` | none | The cast needs a spoken phrase. `kSilence` on the caster blocks it (silenced casters can still cast non-verbal spells, and `SaySpell` stays silent for them). |
| `<weave/>` *(issue.weave-component)* | none | Marks the action as "actual magic". A weave spell fizzles in a `kNoMagic` room — emits `kCastForbiddenToChar` / `kCastForbiddenToRoom` exactly like the legacy data-driven `<blocking><room_flags val="kNoMagic"/>` did. Replaces 209 copies of that blocking block — `<weave/>` is now the single source of truth for "is this magic". `<verbal/>` and `<weave/>` are independent (warcries are verbal but **not** weaves). |
| `<material .../>` | `any_of` / `all_of` / `where` / `cost` | Item-based component. `any_of` / `all_of` are `\|`-separated obj vnum lists. `where` is a `\|`-separated `EFind` list (defaults to `kObjEquip\|kObjInventory\|kObjRoom` — equip first, then inventory, then room). `cost` is the charge consumption (default 1; 0 = presence-only focus; -1 = consumed whole). |

The `MayCastInForbiddenRoom` exemption (greater gods, uncharmed NPCs) bypasses
both the `<weave/>` gate and the data-driven `<blocking>` room-flag gate
symmetrically. Per-spell `kCastForbiddenToChar` / `kCastForbiddenToRoom`
sheaves override the kDefault narration.

---

## 4. Target gates

These all sit *inside* `<talent_actions> → <action>`, alongside the effects.
They gate the **whole cast** (every stage, every effect), so a `<blocking>`
that fires aborts damage *and* affect *and* heal for that target.

### 4.1 `<blocking>`

```xml
<blocking>
    <mob_flags val="kNoSleep"/>
    <affect_flags val="kHold|kSleep"/>
    <room_flags val="kNoMagic"/>
    <align val="kGood"/>
</blocking>
```

The cast is refused if **any one** of the listed conditions matches: a
mob flag (NPC only), an affect flag (any target), the alignment selector
(target alignment), or a room flag (CASTER's current room). Each axis is
optional; combine as many as the spell needs.

The child-tag form is used because lines get long when several axes
combine (e.g. `kNoMagic` blocking now lives on almost every spell). The
older attribute form (`<blocking mob_flags="..."/>`) has been retired.

| Child tag | Values |
|---|---|
| `<mob_flags val="…"/>` | `\|`-separated `EMobFlag` names. Recognised: `kNoBlind`, `kNoSleep`, `kNoSilence`, `kNoHold`, `kNoBash`, `kNoCharm`, `kNoSummon`, `kNoFear`, `kCorpse`, `kResurrected`, `kMounting`, `kHelper`, `kClone`. |
| `<affect_flags val="…"/>` | `\|`-separated `EAffect` names (any affect flag the engine recognises). |
| `<room_flags val="…"/>` | `\|`-separated `ERoomFlag` names. Recognised: `kNoMagic`, `kPeaceful`, `kTunnel`, `kNoTeleportIn`, `kNoBattle`, `kGodsRoom`, `kForMono`, `kForPoly`. Matches against the caster's room. |
| `<align val="…"/>` | `kGood`, `kEvil`, `kNeutral` — block if the target carries this alignment. |

On a single-target spell, a target-side block emits `kNoeffect` to the
caster. On a group/mass cast, a target-side block is silent for that
target (no per-target spam). A **room-flag block** is different: it
fizzles the *whole* cast before any per-target dispatch and emits
`kCastForbiddenToChar` / `kCastForbiddenToRoom` (see §15). Greater gods
and uncharmed/non-tutelar NPCs bypass the room-flag block
(`MayCastInForbiddenRoom`); warcry spells simply do not list `kNoMagic`
as a blocking flag, so the same bypass isn't needed for them.

### 4.2 `<required>`

```xml
<required>
    <mob_flags val="kCorpse"/>
    <affect_flags val="kCharmed"/>
    <align val="kEvil"/>
</required>
```

Same child-tag shape as `<blocking>` but the cast is refused unless the
target **has all** the listed flags/alignment. Use it for "only on
corpses" (animate dead), "only on charmed minions", "only on evil
creatures" (dispel evil), etc. `<required>` does not currently use
`<room_flags>`.

### 4.3 `<caster_blocking>`

```xml
<caster_blocking>
    <caster align="kEvil" affect_flags="kHold|kCharmed"/>
</caster_blocking>
```

Mirrors `<blocking>` but examines the **caster** instead of the victim.
Used to refuse a cast that the caster cannot wield — e.g. `kDispelEvil`
is blocked if the caster is themselves evil. Always emits `kNoeffect`
(no group/mass silent skip: caster-side blocks concern the one caster,
not the per-target loop).

**Schema** *(issue.caster-blocking-refine)*. Unlike `<blocking>` / `<required>`
which use a multi-child-tag form (loose `<mob_flags val>` / `<affect_flags val>`
/ `<room_flags val>` / `<align val>` children), `<caster_blocking>` collapses
its axes onto attributes of a single `<caster>` child:

| Attribute | Default | Description |
|---|---|---|
| `align` | none | `kGood` / `kEvil` / `kNeutral`. Block fires when the caster matches the listed alignment (`IsGood` / `IsEvil` / `IsNeutral`). |
| `affect_flags` | none | `\|`-separated `EAffect` list. Block fires when the caster carries **any** of the listed affect flags (e.g. `kHold` so a held caster can't fire a self-only buff). |

Both attributes are optional and additive — any matching axis triggers the
block. Storage is shared with `<blocking>` / `<required>` (the same
`FlagCondition` struct), so internally the data lands on `cond.align` and
`cond.affect_flags`; the schema asymmetry is deliberate (caster gating is
a single descriptive entry rather than a multi-axis AND of loose conditions).

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
    <amount min="0" dices_weight="1.0" alpha="0.5" beta="12.5"/>
    <hits skill_divisor="12" max="2" prob="100"/>
</damage>
```

| Attr / child | Default | Description |
|---|---|---|
| `saving` | `kReflex` | `ESaving`: `kReflex`, `kStability`, `kWill`, `kCritical`, `kNone`. A successful save halves damage. |
| `prob` | `100` | Percent chance the damage actually happens (silent miss otherwise). `prob<100` short-circuits the RNG. |
| `<amount min= dices_weight= alpha= beta=>` | min=0, dices_weight=1.0, alpha=0, beta=1.0 | Final amount (issue.potency-formula) = `min + dice · dices_weight · (1 + alpha · C) + beta · C`, where `C = skill_coeff + stat_coeff`. `alpha` scales the dice multiplicatively with competence, so the random spread keeps growing with the output instead of going flat (sub-quadratic). `beta` is the additive competence term. `alpha=0` reduces to the legacy additive Formula A. Omit the tag to keep the defaults. |
| `<hits skill_divisor= max= prob=>` | divisor=25, max=1, prob=20 | Multi-hit support: `count = 1 + CalcExtraHits(...)`. The extra-hit bonus uses the caster's potency-roll `base_skill`, scaled by `min(skill, 75) / skill_divisor`, capped at `max`, fired at `prob`%. `prob=0` means a uniform random pick between 0 and `extra`. Absent tag means a single hit. |

---

## 6. `<points>` — restoration: HP / moves / thirst / full

```xml
<points extra="0" prob="100">
    <heal   npc_coeff="3" min="0" dices_weight="1.0" alpha="0.5" beta="45"/>
    <moves                min="0" dices_weight="0.0" alpha="0" beta="20"/>
    <thirst               min="0" dices_weight="0.0" alpha="0" beta="0"/>
    <full                 min="0" dices_weight="0.0" alpha="0" beta="0"/>
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
| `min` | `0` | Flat amount. May be **negative** — negative numbers drain instead of restore (`<moves min="-30">` drains 30 movement points). |
| `dices_weight` | `0` | Base scale on the potency roll's dice. |
| `alpha` | `0` | Multiplicative: how much competence `C` amplifies the dice (sub-quadratic spread growth). `0` = legacy additive behaviour. |
| `beta` | `0` | Additive weight on `C = (skill_coeff + stat_coeff)` (the old `competencies_weight`). |
| `npc_coeff` (`<heal>` only) | `1.0` | Extra multiplicative boost when the caster is an NPC. Default `1.0` reproduces the legacy `*2` effective heal for mob casters; set to `0` to disable. |

### Sign convention for thirst / full

In the engine the thirst/full fields are inverted — `0` is fully sated,
`kMaxCondition` (48) is "starving". The XML uses the gameplay-natural sign:

* **Positive XML value** = restore (less thirsty / less hungry).
* **Negative XML value** = make worse (more thirsty / more hungry).

`CastToPoints` negates the computed amount before calling
`gain_condition`, so designers can read `<thirst min="50"/>` as
"restore 50 thirst points" without keeping the inverted engine field in
mind.

### Formula

Each amount, before clamps:

```
amount = ceil(min + dice · dices_weight · (1 + alpha · C) + beta · C)   // C = skill_coeff + stat_coeff
amount += amount · (caster's spellpower bonus / 100)
if caster is NPC: amount += amount · npc_coeff   // 0 for non-heal categories
```

The dice / competencies are rolled **once per cast** from the spell's
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

The intensity table (`lib/cfg/mechanics/points.xml`) carries `<improve>`
and `<degrade>` grades for each category. Resolution walks the table
descending and returns the first tier `T` such that `T ≤ percent` —
unified for both directions. `<heal><degrade>` is intentionally empty
(heal is positive-only by design; a negative `<heal>` amount silently
no-ops). Thirst/full `<degrade>` thresholds are **negative** because the
percent formula is signed.

---

## 7. `<area>` — area cast scaling

```xml
<area>
    <decay cast="0.05" level="7" free_targets="3"/>
    <victims skill_divisor="15" dice_size="2" min_targets="5" max_targets="20"/>
</area>
```

`<decay>` controls how the cast's effective level/cost decays per extra
target; `<victims>` controls how many targets the cast picks.

| `<decay>` attr | Description |
|---|---|
| `cast` | Decay multiplier applied to cost each step. |
| `level` | Caster-level decrement per target beyond `free_targets`. |
| `free_targets` | The first N targets cost nothing extra. |

| `<victims>` attr | Description |
|---|---|
| `skill_divisor` | Caster-skill divisor that scales the target count (≥1). |
| `dice_size` | Sides per random target-count die. |
| `min_targets` | Floor for victim count. |
| `max_targets` | Ceiling for victim count. |

Used together with `kMagAreas` / `kMagMasses` flags. `CalcTargetsQuantity =
min_targets + min(RollDices(skill / skill_divisor, dice_size), max_targets)`.

---

## 8. `<affects>` — apply a magical affect

This is the workhorse for buffs and debuffs.

```xml
<affects type="kSleep" saving="kWill" resist="kMind" prob="100">
    <flags val="kAfBattledec|kAfDispellable|kAfCurable"/>
    <duration base="1" skill_divisor="15" min="1" max="6"/>
    <apply id="kSleep" location="kNone">
        <modifier min="0.0" dices_weight="0.0" alpha="0" beta="0"
                  factor="1" stack="1"/>
    </apply>
    <reposition pos="kSleep" stop_fight="false"/>
    <lag base="2" bonus_divisor="-1"/>
</affects>
```

### 8.1 `<affects>` attributes

| Attribute | Default | Description |
|---|---|---|
| `type` | required | `ESpell` enum value — the **affect identity** recorded on the target. Used as the lookup key for dispel/cure and for messages. |
| `saving` | `kReflex` | The save the victim makes to avoid the affect. `kNone` means no save. |
| `resist` | `kFire` | `EResist` — the resist channel applied to the affect's duration. |
| `prob` | `100` | Percent chance the affect block fires at all (silent miss otherwise). The `<lag>` and `<reposition>` are *gated* by this same prob — failure suppresses them too. |
| `potency_weight` | `1.0` | *(issue.affects-potency-weight)* Scale on the stored `Affect::potency` value (i.e. on `cast_potency`, not on the modifier). The Affect lands at `cast_potency * potency_weight`; the dispel contest in `DispelSucceeds` reads that scaled value back. **Use case:** a big-modifier spell where the same `<potency_roll>` feeds both the modifier formula *and* the stored potency can become undispellable just because the modifier needs to be big. `potency_weight="0.4"` lets the author decouple — keep the modifier strong, scale only the stored potency down to a dispellable range. Symmetric in spirit with `<unaffect potency_weight=>` on the dispel side (see §9.1). |

### 8.2 `<flags val="…">` — `EAffFlag` bits

| Flag | Meaning |
|---|---|
| `kAfBattledec` | Duration decrements per fight round in addition to per tick. |
| `kAfDeadkeep` | Affect persists on the corpse / through death. |
| `kAfPulsedec` | Decrement on every pulse instead of every tick. |
| `kAfSameTime` | Same duration for PC and NPC (no PC tick-conversion). |
| `kAfUpdateDuration` | Re-casting overwrites the duration (the longer of old / new). |
| `kAfAccumulateDuration` | Re-casting adds to the existing duration. |
| `kAfUpdateMod` | Re-casting overwrites the modifier. |
| `kAfDispellable` | **Eligible for dispel** (e.g. `kDispellMagic`). |
| `kAfCurable` | **Eligible for cure** (e.g. `kRemovePoison`). |
| `kAfMustBeHandled` | Room affects only: this affect has a periodic-tick handler in code (e.g. kDeadlyFog's poison tick, kMeteorStorm's meteor drops, kBlackTentacles' grab attempts). The room-affect loop calls `HandleRoomAffect` each tick when this bit is set. Char affects don't currently use this flag. |
| `kAfUnique` | Room affects only: before imposing, remove any prior cast of this same spell by this same caster. Used by kRuneLabel ("one rune label in the world per caster"). |

`kAfDispellable` / `kAfCurable` are the **single source of truth** for
whether an affect can be removed by a given dispel/cure. Drop both and
the affect is permanent until natural expiry.

`kAfMustBeHandled` and `kAfUnique` are properties of room affects only;
char affects do not currently use them. They replaced the per-affect
`must_handled` member and the per-call `only_one` local bool that used
to live in `CallMagicToRoom`.

### 8.3 `<duration>` (skill-scaled)

```xml
<duration base="2" skill_divisor="5" min="0" max="0"/>
```

| Attr | Description |
|---|---|
| `base` | Flat base duration (in hours, PC unit-converted to ticks). |
| `skill_divisor` | Per-`min(skill,75) / skill_divisor` hour bonus, using the spell's potency-roll `base_skill`. `0` means no skill scaling. |
| `min` / `max` | Clamps in hours. `0` on either side means "no clamp on that side" (matches old behaviour). |

The skill bonus is capped at the novice threshold (75) divided by
`skill_divisor`, so monster skills can't grow affect durations unbounded.

### 8.4 `<apply>` — one stat / status change per `<apply>`

```xml
<apply id="kPoisoned" location="kStr" random="false">
    <modifier min="2.0" dices_weight="0.0" alpha="0" beta="0"
              factor="-1" stack="3"/>
</apply>
```

| `<apply>` attr | Description |
|---|---|
| `id` | `EAffect` flag this apply imposes (e.g. `kSleep`, `kPoisoned`, `kInvisible`). `kUndefinded` for a stat-only apply with no flag. |
| `location` | `EApply`: where the modifier lands. `kNone` for a flag-only apply, `kStr`/`kDex`/…, `kHitroll`, `kSavingReflex`, `kResistDark`, `kManaRegen`, `kCastSuccess`, etc. |
| `random` | `Y`/`N` (default N). When Y, this apply is **not always applied**: see §8.7. |

| `<modifier>` attr | Default | Description |
|---|---|---|
| `min` | `0.0` | Floor for the modifier magnitude. |
| `dices_weight` | `0.0` | Base scale on the potency-roll dice contribution. |
| `alpha` | `0.0` | Multiplicative: competence `C` amplifies the dice. `0` (the usual value for modifiers, which are flat) = legacy additive behaviour. |
| `beta` | `0.0` | Additive weight on `C = (skill_coeff + stat_coeff)` (the old `competencies_weight`). |
| `factor` | `1` | Final sign/scale multiplier. Use `-1` for debuffs (str penalty, save penalty, etc.). |
| `cap` | `0` | Optional upper bound on the raw magnitude **before** factor. `0` (default) = no cap. Used by `kForbidden` (cap=100, mirroring the OLD `MIN(100, …)`) and by the elemental auras (cap=30, saturating around R15). |
| `stack` | `1` | **Stacking cap** — see §8.5. |

Formula:

```
raw      = min + ceil(dices · dices_weight · (1 + alpha · C) + beta · C)   // C = competencies = skill_coeff + stat_coeff
if cap > 0: raw = min(raw, cap)            # optional clamp before factor
modifier = factor · raw
```

The cap clamps **raw magnitude**, so a debuff with `factor="-1" cap="30"` is
bounded between `-30` and `-min`. Use `cap` for "hard ceiling" effects whose
balance was previously enforced by `std::min(…)` calls in code.

### 8.5 Stacking *(new mechanic)*

`stack` is the maximum number of stacks an apply may build on a single
target. With `stack="1"` (the default), re-casting the spell either updates
or refuses to re-apply (depending on the affect's `kAfUpdate…` flags). With
`stack="3"`, re-casting **adds a stack** up to 3, accumulating the modifier.

**What stacks accumulate:**

* The modifier grows: stack 2 reaches `2·modifier`, stack 3 reaches `3·modifier`, etc.
* Duration is also extended if the flag set says so
  (`kAfAccumulateDuration` or `kAfUpdateDuration`).
* When the affect is dispelled (e.g. by `kDispellMagic`), one stack is
  *peeled* rather than the whole affect removed — the modifier shrinks
  proportionally, the stack count drops by one, the affect persists. Only
  when the last stack is removed does the affect disappear.

This makes "stacking poison", "stacking morale loss", or "build-up debuffs"
expressible purely in XML.

Example design — a poison that stacks up to 3 times:

```xml
<affects type="kSlowPoison" saving="kCritical" resist="kDark">
    <flags val="kAfAccumulateDuration|kAfCurable"/>
    <duration base="0" skill_divisor="3" min="0" max="0"/>
    <apply id="kPoisoned" location="kStr">
        <modifier min="2.0" dices_weight="0.0" alpha="0" beta="0"
                  factor="-1" stack="3"/>
    </apply>
    <apply id="kPoisoned" location="kPoison">
        <modifier min="30.0" dices_weight="0.0" alpha="0" beta="0"
                  factor="1" stack="3"/>
    </apply>
</affects>
```

### 8.6 `<reposition>` — forced position / fight-stop

```xml
<reposition pos="kSleep" stop_fight="false"/>
```

| Attr | Default | Description |
|---|---|---|
| `pos` | (none) | `EPosition`. The victim is forced to this position when the affect lands. Omit to skip the position change but keep `stop_fight`. |
| `stop_fight` | `N` | If `Y`, everyone fighting the victim stops fighting them (the "peaceful" effect). |

Useful for sleep, stun, peaceful, knockdown — moves what used to be
hard-coded post-cast nudges into data.

### 8.7 `<lag>` — battle-lag the victim

```xml
<lag base="6" bonus_divisor="-1"/>
```

| Attr | Description |
|---|---|
| `base` | Flat lag in battle rounds when the affect lands. |
| `bonus_divisor` | If `> 0`, the caster's low-skill coefficient is added as `low_skill_coeff / bonus_divisor` extra rounds. `≤ 0` means flat lag (no skill scaling). |

### 8.8 Random apply pools

Among the `<apply random="true">` siblings inside one `<affects>`, **exactly
one** is picked at random per cast (uniform, reservoir sampling). Mix
random-flagged and unconditional applies freely in the same `<affects>` —
the unconditional ones always fire, the random ones share a single pool.

Example (from `kFailure`): one guaranteed morale penalty + one random stat
penalty out of six.

```xml
<affects type="kFailure" saving="kWill" resist="kDark">
    <flags val="kAfDispellable|kAfCurable"/>
    <duration base="2" skill_divisor="5" min="0" max="0"/>
    <apply id="kUndefinded" location="kMorale">
        <modifier min="5.0" alpha="0" beta="1" factor="-1"/>
    </apply>
    <apply id="kUndefinded" location="kStr" random="true">
        <modifier min="0.0" alpha="0" beta="0.11" factor="-1"/>
    </apply>
    <apply id="kUndefinded" location="kDex" random="true">
        <modifier min="0.0" alpha="0" beta="0.11" factor="-1"/>
    </apply>
    <apply id="kUndefinded" location="kInt" random="true">
        <modifier min="0.0" alpha="0" beta="0.11" factor="-1"/>
    </apply>
    <apply id="kUndefinded" location="kWis" random="true">
        <modifier min="0.0" alpha="0" beta="0.11" factor="-1"/>
    </apply>
    <apply id="kUndefinded" location="kCon" random="true">
        <modifier min="0.0" alpha="0" beta="0.11" factor="-1"/>
    </apply>
    <apply id="kUndefinded" location="kCha" random="true">
        <modifier min="0.0" alpha="0" beta="0.11" factor="-1"/>
    </apply>
</affects>
```

### 8.9 Scaling modifiers with caster competence

The new system's design philosophy is that **modifier magnitude grows with
caster competence (skill + stat), not with caster level or remort
directly**. The OLD per-spell formulas like `-1 - R/2` or `(L+R)/3` are
re-expressed via `beta` (the additive competence weight). Modifiers are almost
always flat (`dices_weight=0`), where the formula is just `min + ceil(beta · C)`
and `beta` is exactly the old `competencies_weight` renamed — so the tuning rules
below carry over unchanged. `alpha` only matters when `dices_weight ≠ 0`.

The conversion rule used when translating from OLD formulas:

> **1 remort = +5 skill cap + +1 base stat**
>
> (Newly-created character has skill cap 80 and base stat = class
> default; each transformation/remort raises the cap by 5 and the
> base stat by 1.)

For the cookie-cutter potency_roll (`low=3 hi=1.25`,
`threshold=22 weight=0.5`), per-remort competencies grow by
`(5·1.25 + 1·0.5)/100 = 0.0675`. So a modifier that should grow by
`k` per remort needs `beta ≈ k / 0.0675`.

**The min ≥ 0 rule.** Choose `min` and `beta` so an
untrained caster (competencies = 0) gets `min·factor` = 0 modifier
(or a small clean baseline). This avoids untrained casters
accidentally getting an inverted-sign buff from a debuff spell.

**Recommended algorithm** (used for the bulk of the `<modifier>`
scaling in `spells.xml`):

```
cw_exact   = |target at R=12 max-skill| / competencies_at_R12_max
cw         = floor(cw_exact * 10) / 10        # round down to keep min ≥ 0
min        = |target at R=12| - ceil(competencies_at_R12 * cw)
```

This anchors the R=12 typical caster (skill cap 140, key stat
threshold+12) exactly, lets the modifier scale monotonically with
competence in either direction, and gives untrained casters a clean
zero baseline.

**Example.** `kBless`'s old formula was `-5 - R/3` on
`kSavingStability`. With cookie-cutter potency_roll
(competencies ≈ 3.12 at R=12 max-skill), `cw_exact = 9/3.12 ≈ 2.88`,
so `cw = 2.8` and `min = 9 - ceil(3.12·2.8) = 9 - 9 = 0`. The current
XML reads:

```xml
<apply id="kBless" location="kSavingStability">
    <modifier min="0.0" dices_weight="0.0"
              alpha="0" beta="2.8" factor="-1"/>
</apply>
```

Verification:
- Untrained caster (competencies = 0): modifier = `-1·(0 + 0)` = 0.
- R=0 max-skill (competencies ≈ 2.31): modifier = `-1·ceil(6.47)` = −7.
- R=12 typical (competencies ≈ 3.12): modifier = `-1·ceil(8.74)` = **−9** ✓
- R=24 max (competencies ≈ 3.93): modifier = `-1·ceil(11.0)` = −11.

The slope is slightly gentler than OLD's `-1/3` per remort, but R=12
matches exactly and competence drives the value end-to-end.

**When `min` ends up negative.** With aggressive potency_rolls (large
`low_skill_bonus`/`hi_skill_bonus` or large stat `weight`), the
algorithm above can produce `min < 0`. *Avoid this.* A negative `min`
means an untrained caster's modifier becomes `-1·(negative)` = positive
— inverting the spell's sign. For example, a debuff with `min=-7
factor=-1` gives an untrained mage a `+7` *buff* on enemy saves. Fix by:

- reducing `beta` until `min ≥ 0` (gentler scaling), or
- normalising the spell's potency_roll to cookie-cutter shape so the
  arithmetic works out without overshoot.

A handful of spells in the codebase have unusually aggressive
potency_rolls (`kPatronage`, `kEviless`, `kCurse`) and currently use
flat-`min` modifiers without `beta`. They're left that
way until their potency_rolls are normalised.

---

## 9. `<unaffect>` — dispels / cures *(new mechanics)*

```xml
<unaffect affect_flags="kAfCurable" potency_weight="1.0" prob="100">
    <blocking      any_of="kGodsShield"
                   all_of=""/>
    <breaking      any_of="kSanctuary"/>
    <remove_anyway any_of="kQuestMark"/>
    <remove        all_of="kInvisible|kCamouflage|kHide"
                   breaking_by_failure="true"/>
</unaffect>
```

### 9.1 `<unaffect>` attributes

| Attribute | Default | Description |
|---|---|---|
| `affect_flags` | `kAfCurable\|kAfDispellable` | **Which affects this dispel may touch.** An affect is eligible only if it carries at least one of these `EAffFlag` bits. Narrow to `kAfCurable` for cures (cures don't dispel), `kAfDispellable` for `kDispellMagic` (doesn't cure poisons), etc. |
| `potency_weight` | `1.0` | Multiplier on the dispel spell's potency when contested against an affect's recorded potency. See §9.3. |
| `prob` | `100` | Percent chance the unaffect block fires at all (silent skip otherwise). |

### 9.2 The four sub-blocks

Each of the four children carries two `ESpell` lists:

| Block | When checked | Effect |
|---|---|---|
| `<blocking>` | First | If any listed affect is present, **all `<remove>` entries are skipped** (but `<remove_anyway>` still runs). The chain is *not* broken. |
| `<breaking>` | First | If any listed affect is present, the cast **chain breaks** after this stage (no later stages run). |
| `<remove_anyway>` | Second | Affects to dispel **regardless of blocking**. Useful for "always strip the quest mark even if shielded". |
| `<remove>` | Third (only if not blocked) | Normal removal list — these are dispelled when `<blocking>` didn't fire. |

Within each block:

| Attribute | Meaning |
|---|---|
| `any_of` | `\|`-separated `ESpell` list. The block fires if **any one** is present (for blocking/breaking) or dispels the **first one present** (for remove blocks). The single-token value `"*"` is the **wildcard** — see §9.5. |
| `all_of` | `\|`-separated `ESpell` list. For blocking/breaking, fires only when **all** are simultaneously present. For remove blocks, dispels **every** listed affect that is present. The single-token value `"*"` is the **wildcard** — see §9.5. |
| `breaking_by_failure` *(remove / remove_anyway only)* | If `true`, a **failed potency check** on any dispel from this block breaks the cast chain (no later stages run). The default is `false` (a failed check is silent). |

Mixing `"*"` with explicit names (e.g. `any_of="*\|kPoison"`) is rejected
at parse time with a warning — the explicit names are redundant when the
wildcard is present, and the semantic is ambiguous. Use one or the other.

### 9.3 Potency-gated dispel *(new mechanic)*

Each affect imposed by `CastAffect` **records the cast's potency** (the
roll value at the moment of imposition) and a debuff flag (matches the
spell's `violent`). When `CastUnaffects` tries to remove an affect, the
removal is gated by a strength contest.

**The math**, side by side. Both sides use the same `cast_potency` formula
(§3.7); the only knob is each side's `potency_weight`:

```
stored_potency = cast_potency(affect_spell, caster_at_impose) · <affects potency_weight>     (impose-time, §8.1)
dispel_potency = cast_potency(dispel_spell, caster_at_dispel) · <unaffect potency_weight>    (contest-time, §9.1)
```

The dispel succeeds when `dispel_potency > stored_potency`. The full ruleset:

1. **Non-violent dispel + non-debuff affect** (a buff being purified by a
   non-violent cure) — **no check**, the affect is always removed. This is
   the path that lets a benign `kRemovePoison` strip a player's own
   accidental buffs without a contest. For an ambiguous (`A`) dispel like
   `kDispellMagic`, the "non-violent" predicate resolves per-target
   (§3.3): from an ally hand the dispel passes; from an outsider it goes
   to the contest below.
2. **Flat 5 % luck floor** — regardless of strength, the dispel always has
   a 5 % chance to succeed. Keeps even a novice cure relevant against a
   strong affect.
3. **Strength contest** — otherwise `dispel_potency > stored_potency`.

So a high-level necromancer's poison resists a weak novice's cure, but the
cure may still win on the 5 % free chance. The two `potency_weight` knobs
are independent — the affect-side knob (issue.affects-potency-weight) lets
big-modifier spells stay dispellable by recording a deliberately weaker
stored potency than the raw roll would suggest; the dispel-side knob
(issue.#3342) lets a designer make a specific cure stronger or weaker
against the affects it targets.

**Worked example.** A kFireball with `<affects potency_weight="0.4">` and a
`<potency_roll>` rolling 60 lands its burning affect at `stored = 60 × 0.4 = 24`.
A kDispellMagic with `<unaffect potency_weight="0.5">` rolling 50 contests at
`dispel = 50 × 0.5 = 25`. Dispel wins (25 > 24) — but only just; a slightly
lower roll on the dispel side would fail the contest and rely on the 5 %
floor.

A failed potency check emits `kNoeffect` to the caster (for a pure dispel
spell — a `kMagAffects` spell that also dispels remains silent on failure
and still applies its own affect).

### 9.4 The chain-break path *(new mechanic)*

Two ways to break the cast chain from an unaffect stage:

1. **Present-affect breaking** — `<breaking any_of="…">` finds a present
   affect; the chain breaks without any removal attempt.
2. **Failed-removal breaking** — `<remove … breaking_by_failure="true">`
   tries to dispel and the potency check fails on any candidate.

A broken chain means later stages — affect, damage, points, alter, etc. —
**don't run**. Use this for "reveal first, then debuff" patterns: if the
victim resists having their invisibility stripped, the follow-up debuff
should also miss.

### 9.5 The `"*"` wildcard *(new mechanic)*

`any_of="*"` and `all_of="*"` extend `<remove>` / `<remove_anyway>` from
"fixed list of spell names" to "everything matching the unaffect's
`affect_flags` filter". The two wildcards have different semantics —
matching the explicit-list semantics extended:

| Wildcard | Selection |
|---|---|
| `any_of="*"` | Pick **one** eligible affect, uniformly at random. (Reservoir-sampled across the victim's affect list.) |
| `all_of="*"` | Queue **every** eligible affect on the victim for removal. |

"Eligible" means `affect_matches(victim->aff, unaffect.affect_flags)` —
i.e. the affect carries at least one of the `EAffFlag` bits listed in
the surrounding `<unaffect affect_flags=…>` attribute. Each candidate
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
            <unaffect affect_flags="kAfDispellable">
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
            <unaffect affect_flags="kAfCurable">
                <remove all_of="*"/>
            </unaffect>
        </action>
    </talent_actions>
</spell>
```

Every affect carrying `kAfCurable` (poisons currently, plus any future
"curable" affect) gets a potency check; each is stripped or resisted
independently.

### 9.6 Cross-reference: the dispel pipeline at a glance

For each cast that runs `<unaffect>`:

1. Apply the `prob` gate (silent skip if missed).
2. Evaluate `<blocking>` and `<breaking>` (with wildcard support).
3. Collect candidates from `<remove_anyway>` (always) and `<remove>`
   (only when not blocked) — explicit lists or wildcards.
4. If candidates exist, run the optional PK-action check, then per
   candidate: `DispelSucceeds` → `RemoveAffectAndAnnounce` or
   record a resisted attempt.
5. If a `breaking_by_failure` candidate resisted, or a `<breaking>`
   condition was present, return `kBreak` to stop the cast chain.
6. Otherwise return `kSuccess` and the cast continues to the next stage.

---

## 10. `<points>` and `<area>` see §6 and §7 above.

---

## 11. `CastToPoints`, `CastToAlterObjs`, `CastSummon`, `CastCreation`, `CastManual`

These stages are gated by their `kMag…` flag and run dedicated logic in
`magic.cpp`. They are **partly data-driven**:

* `CastToPoints` (HP / MV / thirst / full restore) is fully data-driven
  via the spell's `<points>` block (issue.mag-points). All four inner
  amounts (`<heal>`, `<moves>`, `<thirst>`, `<full>`) share a single
  potency roll and a single `prob` gate. Each non-zero amount fires its
  **own** message — one `act()` per category in heal/moves/thirst/full
  order, keyed `kHealToVict` / `kMovesToVict` / `kThirstToVict` /
  `kFullToVict` (see §6 "Per-category narration"). The legacy single
  `kPointsToVict` key was retired in issue.point-bugs.
* `CastToAlterObjs` runs a per-spell handler in code (bless, curse,
  invisible, poison, enchant, repair, etc.) and reads its messages from
  `spell_msg.xml` (`kAlterObjToChar`, `kEnchantNotWeapon`, …).
* `CastSummon` (animate dead, clone, summon keeper, …) is mostly code, with
  messages in `spell_msg.xml` (`kSummonToRoom`, `kSummonFail`,
  `kSummonNoCorpse`, etc.).
* `CastCreation`, `CastManual` are pure code stages.

The data-driven parts you can tune for these stages are: `<flags>`,
`<targets>`, `<misc>`, `<mana>`, and the spell's messages.

### 11.5 Room affects (`kMagRoom`) — same `<affects>` block, **raw-pulse durations**

Room spells (the `kMagRoom` flag, dispatched through `CallMagicToRoom` in
`magic_rooms.cpp`) reuse the **same `<affects>` schema** as char affects
(§8). The reader pulls duration, battleflag, and modifier from
`talent.GetAffect()` and assembles an `Affect<ERoomApply>` from it. There
are two differences from char-affect semantics:

1. **Duration unit is RAW PULSES, not hours.** For room affects, the
   value `talent.GetDurationBase() + CalcNoviceSkillBonus(...)` is used
   directly with no `kSecsPerMudHour / kSecsPerPlayerAffect` multiplier
   for PC casters. Why: room-affect ticks fire per pulse, and several
   long-standing room durations (kDeadlyFog=8, kMeteorStorm=3,
   kThunderstorm=7) are sub-hour values that hours-based duration
   couldn't express in integers. The room reader matches the existing
   pulse-direct convention.
2. **The affect-flag enum is `ERoomAffect`, not `EAffect`.** Most room
   affects today don't set a flag (kNone). The reader populates
   `af[0].affect_type` from the apply's `id=` attribute, but the
   `<apply id=...>` parser is currently typed against `EAffect`, so
   room-affect flag emission via XML isn't fully wired yet — set the
   apply's `id="kUndefinded"` for room affects unless you're certain
   you want a char-affect-typed flag.

**Battleflag bits that matter for room affects:**

| Flag | Effect |
|---|---|
| `kAfUpdateDuration` | Re-casting your own room affect refreshes its duration (`max(old, new)`). Without this, a self-recast is a no-op. |
| `kAfAccumulateDuration` | Re-casting adds durations instead of refreshing. |
| `kAfMustBeHandled` | The affect has a per-tick code handler in `HandleRoomAffect` (e.g. kDeadlyFog's poison tick, kMeteorStorm's meteor drops, kBlackTentacles' grab attempts). The room-affect loop calls the handler each pulse when this bit is set. |
| `kAfUnique` | Before imposing, remove any prior cast of this same spell by this same caster (room-affect "only one in the world per caster", used by kRuneLabel). |

**Material-component check is universal.** Any room spell that has a
configured component in `ProcessMatComponents` (currently
`kHypnoticPattern`, `kFascination`, `kEnchantWeapon`) aborts the cast
if the component is missing. The check runs at the top of
`CallMagicToRoom` before any affect data is read — same position as
the equivalent check in `CastAffect`. Spells without a configured
component fall through to the default branch in
`ProcessMatComponents` and the cast proceeds.

**Code-side overrides** still required (these are mechanics the XML
schema can't express):

| Override | Where | What it does |
|---|---|---|
| Mana-caster modifier | `kForbidden` case | `IS_MANA_CASTER(ch)` → modifier = 95 (constant), overriding the formula. |
| Modifier cap | `kForbidden` case | `min(100, ...)` cap (the XML modifier formula can over-shoot). |
| Modifier-tier message | `kForbidden` case | The 3-tier seal-quality narration (>99 / >79 / else) depends on the runtime modifier. |
| Fizzle path | `kRuneLabel` case | Room-flag check (`kPeaceful`/`kTunnel`/`kNoTeleportIn`) cancels the affect and emits failure-specific narration. |
| Incompatible-room block | `kBlackTentacles` case | `kForMono` / `kForPoly` room flags abort the cast. |

**Authoring a new room spell:**

```xml
<spell id="kMyRoomSpell" element="kEarth" mode="kEnabled">
    <name rus="..." eng="..."/>
    <misc pos="kStand" violent="N" danger="1"/>
    <mana .../>
    <targets val="kTarIgnore"/>
    <flags val="kMagRoom"/>          <!-- this is what routes through CallMagicToRoom -->
    <potency_roll>
        <!-- Only needed if duration or modifier scales with a skill. -->
        <base_skill id="kEarthMagic" low_skill_bonus="0" hi_skill_bonus="0"/>
    </potency_roll>
    <talent_actions>
        <action>
            <affects type="kMyRoomSpell" saving="kNone" resist="kVitality">
                <!-- Duration in RAW PULSES. -->
                <duration base="20" skill_divisor="5" min="0" max="0"/>
                <flags val="kAfMustBeHandled"/>
                <!-- Optional <apply> for a modifier; omit for flag-only affects. -->
            </affects>
        </action>
    </talent_actions>
</spell>
```

If the spell needs per-tick logic (`kAfMustBeHandled`), add a case in
`HandleRoomAffect`. If it needs any of the code-side overrides above,
add a minimal case in `CallMagicToRoom`'s switch — but keep all the
plain parameters (duration / battleflag / modifier formula) in the
XML.

---

## 12. Messages (`lib/cfg/spell_msg.xml`)

Per-spell message strings, keyed by spell id (the sheaf) and `ESpellMsg`
type. `id="kDefault"` is the shared sheaf — fallback for messages a
specific spell hasn't overridden.

```xml
<spell_msg>
    <sheaf id="kDefault">
        <message type="kNoeffect" val="Заклинание не оказало никакого эффекта."/>
        <message type="kAffExpired" val="Магия, наложенная на вас, рассеялась."/>
        <message type="kCantCastPosition" val="Вам нужно встать."/>
        …
    </sheaf>
    <sheaf id="kFireball">
        <message type="kAffImposedToChar" val="Огонь обжигает вас!"/>
        <message type="kAffImposedToRoom" val="$n окутывает $N3 пламенем!"/>
        <message type="kAffDispelledToChar" val="Пламя гаснет."/>
        <message type="kAffExpired" val="Жар, охвативший вас, утихает."/>
    </sheaf>
    …
</spell_msg>
```

### Common message keys

| Key | When emitted | Audience |
|---|---|---|
| `kNoeffect` | Generic "no effect" outcome. | Caster |
| `kAffImposedToChar` | An affect from this spell landed. | Affected char |
| `kAffImposedToRoom` | Same, to onlookers. | Room |
| `kAffDispelledToChar` | An affect of this **affect type** was dispelled. Sheaf is keyed on the **removed** affect, not the dispel spell. | Cured char |
| `kAffDispelledToRoom` | Same, to onlookers. | Room |
| `kAffExpired` | An affect timed out naturally on the target. | Affected char |
| `kAffDispelledToOwner` | An affect of yours was dispelled from someone you cast it on. | Original caster |
| `kHealToVict` | `<heal>` produced a non-zero amount in a `<points>` cast. | Target |
| `kMovesToVict` | `<moves>` produced a non-zero amount in a `<points>` cast. | Target |
| `kThirstToVict` | `<thirst>` produced a non-zero amount in a `<points>` cast. | Target |
| `kFullToVict` | `<full>` produced a non-zero amount in a `<points>` cast. | Target |
| `kReflectedToChar` | Reflection: spell bounced back. | Caster |
| `kReflectedToVict` | Reflection: target who reflected. | Original victim |
| `kReflectedToRoom` | Reflection: onlookers. | Room |
| `kCastForbiddenToChar` | Room-block fizzle: caster's room carries a `<blocking><room_flags val>` flag (`kNoMagic` etc.). | Caster |
| `kCastForbiddenToRoom` | Room-block fizzle: announcement to onlookers. Empty means stay silent. | Room |
| `kCantCastPosition`, `kCantCastSleeping`, … | Cast-precondition failures. | Caster |
| `kCantCastNotAlly` | `kTarAllyOnly` target gate (§3.5) — victim isn't self / groupmate. | Caster |
| `kCastHereForbiddenToChar`, `kCastHereForbiddenToRoom` | `<blocking><room_flags>` fizzle (caster's room carries the blocking flag, e.g. `kNoMagic`). Empty room-side message stays silent. | Caster / Room |
| `kAreaToChar`, `kAreaToRoom`, `kAreaToVict` | Area cast announcement. | Various |
| `kCastIncantToChar`, `kCastDisappearToRoom`, `kCastAppearToRoom` | Magic-word incantation banner + appearance/disappearance of the spell name in the room (`SaySpell`). | Various |
| `kCastSayToSelf`, `kCastSayToOther`, `kCastSayToObj`, `kCastSayToSomething`, `kCastSayDamageeToVict`, `kCastSayHelpeeToVict`, `kCastSaySound` | The fly-by "$n stares at $N3 and utters …" lines emitted while the caster pronounces the incantation. | Room / victim |
| `kCastInterruptedToChar`, `kCastPreparedToChar` | Concentration-broken / spell-ready announcements. | Caster |
| `kNoTarget`, `kWrongTarget` | The caster's target argument missed (no such target / target type rejected by `<targets val>`). | Caster |
| `kItemNoPrototype`, `kItemCreatedToChar`, `kItemCreatedToRoom`, `kItemCreationFailToChar` | `CastCreation` outcomes. | Various |
| `kSummonToRoom`, `kSummonFail`, `kSummonNoCorpse`, `kResurrectNoPower`, `kResurrectProtected` | Summon / resurrect outcomes. | Various |
| `kEnchantNotWeapon`, `kEnchantMagic`, `kEnchantSetItem`, `kEnchantMono`, `kEnchantPoly`, `kEnchantOther` | Enchant-weapon outcomes. | Caster |
| `kFightDeathToChar`, `kFightHitToChar`, `kFightMissToChar` (+ `ToVict`, `ToRoom`) | Combat-damage messages keyed by `ESpell`. | Various |
| `kCustomMsgOne` … `kCustomMsgFive` | Generic per-spell slots for one-off lines that don't deserve their own enum constant. Use these for the second/third/etc. line a specific spell needs (e.g. kSummon's secondary failure narration). | Various |

Messages are stored **without** the trailing `\r\n` — `act()` and
`SendMsgToChar` add it themselves.

A message lookup tries the spell's own sheaf first, then falls back to
`kDefault`. The exceptions worth knowing:

* **Imposition** (`kAffImposedToChar` / `kAffImposedToRoom`) and **reflection**
  (`kReflectedToChar` / `kReflectedToVict` / `kReflectedToRoom`) are looked
  up sheaf-directly with **no kDefault fallback** — a missing key stays
  silent rather than leaking a wrong default.
* **Dispel narration** (`kAffDispelledToChar` / `kAffDispelledToRoom`) was
  originally also sheaf-direct, but *(issue.dispel-default-msg)* now goes
  through the kDefault-aware lookup. The kDefault sheaf carries a generic
  fallback so every dispel emits *something* — previously most affects
  (kSanctuary / kBless / kArmor / kFly / etc.) were stripped silently
  because their own sheaves never authored these keys. Per-affect overrides
  (kCurse, kPoison, kBlindness, kHold, kSilence, kFever, kDeafness, the
  4 poison variants, kPatronage) still win where present.
* `kCastForbiddenToChar` falls back to `kDefault` (every spell shares the
  generic "Ваша магия потерпела неудачу…" line by default), and
  `kCastForbiddenToRoom` is treated as optional — an empty result means
  "announce nothing to the room", letting spells like `kRuneLabel`
  override only when they want their own narration.

---

## 13. Worked examples

### 13.1 A simple damage spell with extra hits

```xml
<spell id="kMagicMissile" element="kAir" mode="kEnabled">
    <name rus="волшебная стрела" eng="magic missile"/>
    <misc pos="kFight" violent="Y" danger="1"/>
    <mana max="40" min="30" change="1"/>
    <targets val="kTarCharRoom|kTarFightVict"/>
    <flags val="kMagDamage|kNpcDamagePc"/>
    <potency_roll>
        <dices ndice="6" sdice="4" adice="10"/>
        <base_skill id="kFireMagic" low_skill_bonus="3" hi_skill_bonus="0.75"/>
        <base_stat id="kWis" threshold="22" weight="5"/>
    </potency_roll>
    <talent_actions>
        <action>
            <damage saving="kReflex">
                <amount min="0" dices_weight="1.0" alpha="0.5" beta="12.5"/>
                <hits skill_divisor="12" max="2" prob="100"/>
            </damage>
        </action>
    </talent_actions>
</spell>
```

* Saving throw `kReflex`, halves on save.
* Amount = `dices · 1.0 + competencies · 25`.
* Up to 2 extra hits, fired every cast (`prob=100`), with the extra-hit
  bonus scaled by `min(kFireMagic, 75) / 12`.

### 13.2 A debuff with saving throw, reposition, and lag

```xml
<spell id="kSleep" element="kMind" mode="kEnabled">
    <name rus="сон" eng="sleep"/>
    <misc pos="kFight" violent="Y" danger="0"/>
    <mana max="70" min="55" change="1"/>
    <targets val="kTarCharRoom|kTarFightVict"/>
    <flags val="kMagAffects|kNpcAffectPc"/>
    <potency_roll>
        <dices ndice="5" sdice="9" adice="12"/>
        <base_skill id="kMindMagic" low_skill_bonus="3" hi_skill_bonus="1.25"/>
        <base_stat id="kInt" threshold="22" weight="0.5"/>
    </potency_roll>
    <talent_actions>
        <action>
            <blocking>
                <mob_flags val="kNoSleep"/>
                <affect_flags val="kHold"/>
                <room_flags val="kNoMagic"/>
            </blocking>
            <affects type="kSleep" saving="kWill" resist="kMind">
                <reposition pos="kSleep"/>
                <duration base="1" skill_divisor="15" min="1" max="6"/>
                <flags val="kAfBattledec|kAfDispellable|kAfCurable"/>
                <apply id="kSleep" location="kNone">
                    <modifier min="0.0" dices_weight="0.0"
                              alpha="0" beta="0" factor="1"/>
                </apply>
            </affects>
        </action>
    </talent_actions>
</spell>
```

* Targets are NPCs flagged `kNoSleep` or anyone holding the `kHold` affect
  are immune (the gate fires before any roll).
* A successful `kWill` save averts the sleep.
* On a successful imposition, the victim drops to `kSleep` position.
* Duration: `1 + min(kMindMagic, 75) / 15` hours, clamped `[1..6]`.
* `kAfDispellable|kAfCurable` means both `kDispellMagic` and
  `kRemovePoison`-style cures can attempt removal (subject to potency
  contest).

### 13.3 A pure cure (potency-gated)

```xml
<spell id="kRemovePoison" element="kLife" mode="kEnabled">
    <name rus="лечить яд" eng="remove poison"/>
    <misc pos="kFight" violent="N" danger="0"/>
    <mana max="60" min="45" change="2"/>
    <targets val="kTarCharRoom|kTarFightSelf|kTarObjInv|kTarObjRoom"/>
    <flags val="kMagUnaffects|kMagAlterObjs|kNpcUnaffectNpc"/>
    <potency_roll>
        <dices ndice="5" sdice="9" adice="12"/>
        <base_skill id="kLifeMagic" low_skill_bonus="3" hi_skill_bonus="1.25"/>
        <base_stat id="kWis" threshold="22" weight="0.5"/>
    </potency_roll>
    <talent_actions>
        <action>
            <unaffect affect_flags="kAfCurable">
                <remove all_of="kPoison|kAconitumPoison|kScopolaPoison|kBelenaPoison|kDaturaPoison"/>
            </unaffect>
        </action>
    </talent_actions>
</spell>
```

* `affect_flags="kAfCurable"` — won't touch affects that are only
  `kAfDispellable` (e.g. blessings).
* `<remove all_of="…">` — tries every listed poison present on the target.
* Each removal is a potency contest. A novice priest's `remove poison`
  may fail against a master assassin's `poison` (high recorded potency),
  but always has the 5 % free chance.

### 13.4 Reveal with chain-break

```xml
<spell id="kGlitterDust" element="kEarth" mode="kEnabled">
    <name rus="блестящая пыль" eng="glitterdust"/>
    <misc pos="kFight" violent="Y" danger="5"/>
    <mana max="120" min="100" change="3"/>
    <targets val="kTarIgnore"/>
    <flags val="kMagMasses|kMagAffects|kMagUnaffects|kNpcAffectPc"/>
    <potency_roll>
        <dices ndice="5" sdice="9" adice="12"/>
        <base_skill id="kEarthMagic" low_skill_bonus="3" hi_skill_bonus="1.25"/>
        <base_stat id="kWis" threshold="22" weight="0.5"/>
    </potency_roll>
    <talent_actions>
        <action>
            <area>
                <decay cast="0.05" level="3" free_targets="5"/>
                <victims skill_divisor="15" dice_size="3"
                         min_targets="5" max_targets="20"/>
            </area>
            <unaffect>
                <remove all_of="kInvisible|kCamouflage|kHide"
                        breaking_by_failure="true"/>
            </unaffect>
            <affects type="kGlitterDust" saving="kReflex" resist="kEarth">
                <flags val="kAfDispellable|kAfCurable"/>
                <duration base="4" skill_divisor="0" min="0" max="0"/>
                <apply id="kGlitterDust" location="kSavingReflex">
                    <modifier min="0.0" dices_weight="0.0"
                              alpha="0" beta="4.4" factor="1"/>
                </apply>
            </affects>
        </action>
    </talent_actions>
</spell>
```

* `<unaffect>` runs first (chain order): tries to strip
  `kInvisible|kCamouflage|kHide`.
* If the potency contest **fails** for any of them on a given target,
  `breaking_by_failure="true"` makes the cast chain break **for that
  target** — the glitterdust debuff doesn't land.
* If the dispels succeed (or the affects weren't present), the debuff
  applies normally — a saving-throw penalty for 4 hours.
* The penalty scales with caster competence: at competencies ≈ 3.12
  (skill 140 / wis 34, the R=12 typical caster) the saving-throw
  modifier is `+ceil(3.12·4.4) = +14`; an untrained caster (skill 0,
  stat at threshold) gets `+ceil(0·4.4) = 0` — see §8.9.

### 13.5 Alignment-restricted spell

```xml
<spell id="kDispelEvil" element="kLight" mode="kEnabled">
    <name rus="рассеять зло" eng="dispel evil"/>
    <misc pos="kFight" violent="Y" danger="2"/>
    <mana max="120" min="100" change="2"/>
    <targets val="kTarCharRoom|kTarFightVict"/>
    <flags val="kMagDamage|kNpcDamagePc"/>
    <potency_roll>
        <dices ndice="6" sdice="9" adice="20"/>
        <base_skill id="kLightMagic" low_skill_bonus="3" hi_skill_bonus="1.25"/>
        <base_stat id="kWis" threshold="22" weight="0.5"/>
    </potency_roll>
    <talent_actions>
        <action>
            <blocking>
                <room_flags val="kNoMagic"/>
            </blocking>
            <required>
                <align val="kEvil"/>
            </required>
            <caster_blocking>
                <align val="kEvil"/>
            </caster_blocking>
            <damage saving="kStability">
                <amount min="0" dices_weight="1.0" alpha="0.5" beta="61"/>
            </damage>
        </action>
    </talent_actions>
</spell>
```

* `<required><align val="kEvil"/></required>` — the *target* must be evil, else `kNoeffect`.
* `<caster_blocking><align val="kEvil"/></caster_blocking>` — the *caster* must not be
  evil, else `kNoeffect` (the old "wrath of light" behaviour, now data-driven).
* `<blocking><room_flags val="kNoMagic"/></blocking>` — universal NOMAGIC-room fizzle
  added to all non-warcry spells; emits `kCastForbidden*` (see §15).

### 13.6 A reflection template

No spell currently uses `<reflection>` in production (it's plumbed and
ready). Here's how a fire-shielded creature might reflect cold spells:

```xml
<spell id="kIceBolt" element="kWater" mode="kEnabled">
    <!-- … usual fields … -->
    <talent_actions>
        <action>
            <reflection affect_flags="kFireShield|kFireAura" prob="35"/>
            <damage saving="kReflex">
                <amount min="0" dices_weight="1.0" alpha="0.5" beta="40"/>
            </damage>
        </action>
    </talent_actions>
</spell>
```

* Targets carrying `kFireShield` or `kFireAura` have a **35 %** chance per
  cast to reflect the spell back at the caster.
* Once reflection fires, the caster takes the damage themselves (subject
  to all of their own defences). Three messages
  (`kReflectedToChar`/`Vict`/`Room`) describe the bounce.
* Alignments: add `align="kEvil"` to the same tag to make undead innately
  reflect holy spells, etc.

### 13.7 Stacking poison (hypothetical)

The real `kPoison` doesn't stack today — this example illustrates the
mechanic. The actual `kPoison` uses `beta` to scale the
poison-damage tick (no stacking); here we re-add `stack="3"` to show
how a stacking variant would be authored.

```xml
<spell id="kPoison" element="kDark" mode="kEnabled">
    <!-- … usual fields … -->
    <talent_actions>
        <action>
            <blocking affect_flags="kGodsShield"/>
            <affects type="kPoison" saving="kCritical" resist="kDark">
                <duration base="0" skill_divisor="3" min="0" max="0"/>
                <flags val="kAfSameTime|kAfAccumulateDuration|kAfCurable"/>
                <apply id="kPoisoned" location="kStr">
                    <modifier min="2.0" dices_weight="0.0"
                              alpha="0" beta="0"
                              factor="-1" stack="3"/>
                </apply>
                <apply id="kPoisoned" location="kPoison">
                    <modifier min="0.0" dices_weight="0.0"
                              alpha="0" beta="11.5"
                              factor="1" stack="3"/>
                </apply>
            </affects>
        </action>
    </talent_actions>
</spell>
```

* Re-casting on the same victim **stacks up to 3 times**: the strength
  penalty scales `-2`, `-4`, `-6` (flat per stack) and the
  poison-damage tick scales as `1×`, `2×`, `3×` the per-cast value
  (where the per-cast value is `ceil(competencies·11.5)` — at the R=12
  typical caster ≈ 36 per stack).
* Duration accumulates (`kAfAccumulateDuration`).
* `kAfCurable` (without `kAfDispellable`) — `kRemovePoison` works, plain
  `kDispellMagic` does not.
* `kRemovePoison`'s dispel **peels one stack** rather than removing the
  whole thing — three cures to fully clear a triply-stacked poison
  (assuming each potency contest succeeds).

---

## 14. Reference: enum cheat-sheet

The full enums live in C++ headers; the most-used values are listed here.

### 14.1 `ESaving`

`kReflex` · `kStability` · `kWill` · `kCritical` · `kNone`

### 14.2 `EResist`

`kFire` · `kAir` · `kWater` · `kEarth` · `kDark` · `kLight` · `kMind` ·
`kLife` · `kVitality` · `kImmunity`

### 14.3 `EApply` (a curated subset — the full list is in `entities_constants.h`)

`kNone` · `kStr`/`kDex`/`kCon`/`kInt`/`kWis`/`kCha` · `kHitroll` · `kDamroll`
· `kAc` · `kMorale` · `kPoison` · `kManaRegen` · `kHitRegen` · `kMoveRegen` ·
`kCastSuccess` · `kSavingReflex`/`kSavingStability`/`kSavingWill`/
`kSavingCritical` · `kResistFire`/`kResistAir`/`kResistWater`/`kResistEarth`/
`kResistDark`/`kResistLight`/`kResistMind`/`kResistLife`

### 14.4 `EPosition`

`kDead` · `kMortal` · `kIncap` · `kStun` · `kSleep` · `kRest` · `kSit` ·
`kFight` · `kStand` · `kUndefined` (used by `<reposition>` to mean "no
position change, only `stop_fight`").

### 14.5 `EAlign`

`kAny` (default — no alignment check) · `kGood` · `kEvil` · `kNeutral`.
The thresholds for "good / evil / neutral" come from
`alignment ≤ kAligEvilLess` and `alignment ≥ kAligGoodMore` in
`entities_constants.h`.

### 14.6 `EAffFlag` (affect-level flags, the `<flags val>` inside `<affects>`)

`kAfBattledec` · `kAfDeadkeep` · `kAfPulsedec` · `kAfSameTime` ·
`kAfUpdateDuration` · `kAfAccumulateDuration` · `kAfUpdateMod` ·
`kAfDispellable` · `kAfCurable` · `kAfMustBeHandled` · `kAfUnique`.

### 14.7 `EMobFlag` allowed in `<blocking><mob_flags val>` / `<required><mob_flags val>`

`kNoBlind` · `kNoSleep` · `kNoSilence` · `kNoHold` · `kNoBash` · `kNoCharm`
· `kNoSummon` · `kNoFear` · `kCorpse` · `kResurrected` · `kMounting` ·
`kHelper` · `kClone`. Add new ones to the `kBlockingFlagByName` table in
`src/gameplay/abilities/talents_actions.cpp` to expose more.

### 14.7b `ERoomFlag` allowed in `<blocking><room_flags val>`

`kNoMagic` · `kPeaceful` · `kTunnel` · `kNoTeleportIn` · `kNoBattle` ·
`kGodsRoom` · `kForMono` · `kForPoly`. The room check matches against the
**caster's** current room, fires *before* per-target dispatch, and emits
`kCastForbiddenToChar` / `kCastForbiddenToRoom` (see §15). Add new ones
to the `kBlockingRoomFlagByName` table in
`src/gameplay/abilities/talents_actions.cpp` to expose more.

### 14.8 `EAffect` (status flags for `<apply id>` and the various
`affect_flags` attributes)

The list is large (`src/gameplay/affects/affect_contants.h`). Some commonly
referenced ones:

`kCharmed` · `kPoisoned` · `kInvisible` · `kCamouflage` · `kHide` ·
`kBlind` · `kSleep` · `kHold` · `kSanctuary` · `kPrismaticAura` ·
`kMagicGlass` · `kSonicBarrier` · `kMagicShield` · `kShadowCloak` ·
`kFireShield` · `kFireAura` · `kIceShield` · `kGodsShield` · `kPeaceful` ·
`kStopFight` · `kMagicStopFight` · `kInfravision` · `kDetectInvisible` ·
`kDetectLife` · `kGlitterDust` · `kCurse` · `kProtectFromEvil` ·
`kCharmed` · `kHelper` · `kQuestMark`.

Use `kUndefinded` (sic — preserved spelling in the enum) when an `<apply>`
should change a stat *without* imposing an affect flag.

---

## 15. Quick checklist for adding a new spell

1. **Pick an `ESpell` id.** Add it to `spells_constants.h` if it's truly
   new (engineer task). **Prototyping?** Use one of the reserved
   `kTestOne … kTestFive` slots instead — see §17 — to iterate on the
   XML alone without touching code.
2. **Add the `<spell>` element** to `lib/cfg/spells.xml`. Required:
   header, `<name>`, `<misc>`, `<mana>`, `<targets>`, `<flags>`.
3. **Pick the stages** you want by listing the `kMag…` flags in
   `<flags val>`. Multiple stages allowed.
4. **Author `<potency_roll>`** — at least one of `<dices>`, `<base_skill>`,
   `<base_stat>`. Without a roll, the spell deals zero/no-magnitude
   effects.
5. **Write the `<talent_actions>` block** — gates first, then effects.
6. **Add per-spell messages** to `lib/cfg/spell_msg.xml`: at minimum the
   imposition / dispel / expiration messages if the spell adds affects, or
   the damage description messages if it deals damage.
7. **Refresh the build snapshot** (see §2.3) and boot-test:
   `./circle -d small <port>` — look for SYSERR lines mentioning your
   spell's id or its tag names.

---

## 16. What this manual does **not** cover (yet)

* **Hand-coded handlers** (`kMagManual` spells, `CastSummon`,
  `CastCreation`, `CastToAlterObjs` switch cases) — those still live in
  `spells.cpp` and `magic.cpp`. Migrating them piece by piece into the
  data-driven system is ongoing work.
* **Success roll consumption** — `<success_roll>` is parsed and evaluated
  at cast time but is not yet wired into the cast-success decision. The
  classical percent-based `CalcCastSuccess` still gates landing.
* **Skill-interference modifiers** — feats like `kMagicArrows` that boost
  specific spells (e.g. doubles `kMagicMissile` extra-hits) are still
  hard-coded in `CalcExtraHits` and similar helpers.

If you need any of these, open an issue and the work can be scoped.

---

## 17. Test spell constants for prototyping

To let designers iterate on a new spell's XML **without recompiling
the engine**, five placeholder `ESpell` slots are permanently reserved:

| Constant | id | Russian display name |
|---|---|---|
| `kTestOne`   | 359 | "тест 1" |
| `kTestTwo`   | 360 | "тест 2" |
| `kTestThree` | 361 | "тест 3" |
| `kTestFour`  | 362 | "тест 4" |
| `kTestFive`  | 363 | "тест 5" |

Each slot ships in `lib/cfg/spells.xml` as a minimal stub:

```xml
<spell id="kTestOne" mode="kTesting">
    <name rus="тест 1" eng="test 1"/>
</spell>
```

`mode="kTesting"` keeps the slot out of class spell-lists in production
(it's loaded but not granted) while still letting an immortal force-cast
it for verification.

**Workflow:**

1. Pick a free slot (`kTestOne` … `kTestFive`).
2. Edit its `<spell>` element in `spells.xml` — give it real
   `<misc>`, `<mana>`, `<targets>`, `<flags>`, `<potency_roll>`, and
   the `<talent_actions>` block you want to try. Optionally override
   the Russian name with your prototype's working title (keep
   `mode="kTesting"`).
3. Add per-spell messages to `spell_msg.xml` under the same id.
4. Refresh the build snapshot (see §2.3) and boot-test:
   `./circle -d small <port>`, log in as an immortal, force-cast it
   on a dummy.
5. Iterate on the XML alone — no rebuilds needed beyond the snapshot
   refresh.
6. When the prototype graduates, an engineer renames the constant +
   the XML `id=` to its final name (e.g. `kTestOne` → `kFrostshield`),
   flips the mode to `kEnabled`, and adds it to the appropriate class
   spell-lists. That's a single rename pass — all the design work
   already done against the test slot transfers verbatim.

Keep test slots short-lived: revert them to the minimal stub once your
prototype graduates (or once you abandon it), so the next designer
finds them empty.

---

*Last updated to match the `sventovit.work` head after the
`issue.ambiguous-spells` / `issue.dispel-default-msg` /
`issue.weave-component` / `issue.caster-blocking-refine` /
`issue.affects-potency-weight` sequence: `<misc violent>` gained a
third value `A` ("ambiguous") that resolves per-target via
`IsViolentAgainst(caster, target)` — helpful on self / charmed pet /
groupmate / NPC-vs-NPC, fully violent (with full PK liability) on an
outsider (§3.3, kDispellMagic is the first carrier); a new
`<components>` block at the `<spell>` level (§3.9) collects
`<verbal/>` + `<weave/>` + `<material>` requirements, with
`<weave/>` now the single source of truth for "is this magic" and
the canonical `kNoMagic`-room gate (replacing 209 copies of the
data-driven blocking pattern); `<caster_blocking>` schema was
reshaped to use a single `<caster align="..." affect_flags="..."/>`
child (§4.3) — `affect_flags` is a new optional axis that blocks the
cast when the caster carries any of the listed `EAffect` flags
(e.g. `kHold` so a held caster can't fire a self-only buff); a new
`<affects potency_weight="0.4">` attribute (§8.1) scales the stored
Affect potency at impose time so big-modifier spells stay dispellable
(symmetric with `<unaffect potency_weight=>` on the dispel side); and
the `kAffDispelledToChar` / `kAffDispelledToRoom` keys now fall back
to the kDefault sheaf so every dispel emits *something* — previously
most affects (kSanctuary / kBless / kArmor / kFly / etc.) were stripped
silently because their sheaves never authored these keys (§12).
Carries forward earlier additions: `<cond>` → `<full>` under `<points>`,
the per-category `kHealToVict` / `kMovesToVict` / `kThirstToVict` /
`kFullToVict` keys, the migrated `kCastSay*` / `kCastIncantToChar` /
`kCastHereForbidden*` / `kNoTarget` / `kWrongTarget` /
`kCastPreparedToChar` / `kCastInterruptedToChar` / `kSummonFail`
narration family + `kCustomMsgOne…Five`, the `BYLINS_FIRSTAID_NEW`
First Aid rework, and the `kTestOne…kTestFive` prototyping slots
(§17).*
