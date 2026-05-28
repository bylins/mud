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
| `violent` | `Y`/`N`. Violent spells trigger PK/agro, can be reflected, and are gated by `GET_AR` (affect-resist) on the victim. |
| `danger` | Integer threat rating used by mob AI / scrolls / pricing. |

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

### 3.8 `<talent_actions>`

Container for one or more `<action>` blocks. Each `<action>` can declare any
mix of:

* Gates: `<blocking>`, `<required>`, `<caster_blocking>`, `<reflection>`
* Effects: `<damage>`, `<heal>`, `<area>`, `<affects>`, `<unaffect>`

Most spells use a single `<action>`. Multiple actions are technically allowed
but the framework processes only one of each effect type, so the common
pattern is a single `<action>` with everything inside it.

---

## 4. Target gates

These all sit *inside* `<talent_actions> → <action>`, alongside the effects.
They gate the **whole cast** (every stage, every effect), so a `<blocking>`
that fires aborts damage *and* affect *and* heal for that target.

### 4.1 `<blocking>`

```xml
<blocking mob_flags="kNoSleep" affect_flags="kHold|kSleep" align="kGood"/>
```

The cast is refused on a target that **carries any one** of the listed
flags/alignment. Mob flags are checked only on NPCs.

| Attribute | Values |
|---|---|
| `mob_flags` | `\|`-separated `EMobFlag` names. Recognised: `kNoBlind`, `kNoSleep`, `kNoSilence`, `kNoHold`, `kNoBash`, `kNoCharm`, `kNoSummon`, `kNoFear`, `kCorpse`, `kResurrected`, `kMounting`, `kHelper`, `kClone`. |
| `affect_flags` | `\|`-separated `EAffect` names (any affect flag the engine recognises). |
| `align` | `kGood`, `kEvil`, `kNeutral` — block if the target carries this alignment. |

On a single-target spell, a block emits `kNoeffect` to the caster. On a
group/mass cast, the block is silent for that target (no per-target spam).

### 4.2 `<required>`

```xml
<required mob_flags="kCorpse" affect_flags="kCharmed" align="kEvil"/>
```

Same attribute shape as `<blocking>` but the cast is refused unless the
target **has all** the listed flags/alignment. Use it for "only on
corpses" (animate dead), "only on charmed minions", "only on evil
creatures" (dispel evil), etc.

### 4.3 `<caster_blocking>`

```xml
<caster_blocking align="kEvil"/>
```

Mirrors `<blocking>` but examines the **caster**. Used to refuse a cast that
the caster cannot wield — e.g. `kDispelEvil` is blocked if the caster is
themselves evil. Always emits `kNoeffect` (no group/mass silent skip:
caster-side blocks concern the one caster, not the per-target loop).

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
    <amount min="0" dices_weight="1.0" competencies_weight="25"/>
    <hits skill_divisor="12" max="2" prob="100"/>
</damage>
```

| Attr / child | Default | Description |
|---|---|---|
| `saving` | `kReflex` | `ESaving`: `kReflex`, `kStability`, `kWill`, `kCritical`, `kNone`. A successful save halves damage. |
| `prob` | `100` | Percent chance the damage actually happens (silent miss otherwise). `prob<100` short-circuits the RNG. |
| `<amount min= dices_weight= competencies_weight=>` | min=0, both weights=1.0 | Final amount = `min + dices · dices_weight + (skill_coeff + stat_coeff) · competencies_weight`. Omit the tag to keep the defaults; an empty `<amount/>` is fine. |
| `<hits skill_divisor= max= prob=>` | divisor=25, max=1, prob=20 | Multi-hit support: `count = 1 + CalcExtraHits(...)`. The extra-hit bonus uses the caster's potency-roll `base_skill`, scaled by `min(skill, 75) / skill_divisor`, capped at `max`, fired at `prob`%. `prob=0` means a uniform random pick between 0 and `extra`. Absent tag means a single hit. |

---

## 6. `<heal>` — restorative

```xml
<heal npc_coeff="0.5" extra="N" prob="100">
    <amount min="0" dices_weight="0.0" competencies_weight="100"/>
</heal>
```

| Attr / child | Default | Description |
|---|---|---|
| `npc_coeff` | `1.0` | Multiplier when the target is an NPC (typical use: dampen heal cheese on minions). |
| `extra` | `N` | `Y` allows the heal to push HP above the maximum cap (overheal). |
| `prob` | `100` | Percent chance the heal lands. |
| `<amount min= dices_weight= competencies_weight=>` | min=0, both weights=0.0 | Heal-side weights. Note these *default to 0*, so you must set at least one to get a non-zero heal. |

The heal-side `<amount>` weights are independent of the spell's other
effects, so a designer can rebalance a heal without disturbing damage/affect
on the same spell.

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
        <modifier min="0.0" dices_weight="0.0" competencies_weight="0.0"
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

The last two are the **single source of truth** for whether an affect can
be removed by a given dispel/cure. Drop both and the affect is permanent
until natural expiry.

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
    <modifier min="2.0" dices_weight="0.0" competencies_weight="0.0"
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
| `dices_weight` | `0.0` | Weight on the potency-roll dice contribution. |
| `competencies_weight` | `0.0` | Weight on `(skill_coeff + stat_coeff)`. |
| `factor` | `1` | Final sign/scale multiplier. Use `-1` for debuffs (str penalty, save penalty, etc.). |
| `stack` | `1` | **Stacking cap** — see §8.5. |

Formula:

```
raw      = competencies · competencies_weight + dices · dices_weight
modifier = factor · (min + ceil(raw))
```

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
        <modifier min="2.0" dices_weight="0.0" competencies_weight="0.0"
                  factor="-1" stack="3"/>
    </apply>
    <apply id="kPoisoned" location="kPoison">
        <modifier min="30.0" dices_weight="0.0" competencies_weight="0.0"
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
        <modifier min="5.0" competencies_weight="1.0" factor="-1"/>
    </apply>
    <apply id="kUndefinded" location="kStr" random="true">
        <modifier min="0.0" competencies_weight="0.11" factor="-1"/>
    </apply>
    <apply id="kUndefinded" location="kDex" random="true">
        <modifier min="0.0" competencies_weight="0.11" factor="-1"/>
    </apply>
    <apply id="kUndefinded" location="kInt" random="true">
        <modifier min="0.0" competencies_weight="0.11" factor="-1"/>
    </apply>
    <apply id="kUndefinded" location="kWis" random="true">
        <modifier min="0.0" competencies_weight="0.11" factor="-1"/>
    </apply>
    <apply id="kUndefinded" location="kCon" random="true">
        <modifier min="0.0" competencies_weight="0.11" factor="-1"/>
    </apply>
    <apply id="kUndefinded" location="kCha" random="true">
        <modifier min="0.0" competencies_weight="0.11" factor="-1"/>
    </apply>
</affects>
```

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
| `any_of` | `\|`-separated `ESpell` list. The block fires if **any one** is present (for blocking/breaking) or dispels the **first one present** (for remove blocks). |
| `all_of` | `\|`-separated `ESpell` list. For blocking/breaking, fires only when **all** are simultaneously present. For remove blocks, dispels **every** listed affect that is present. |
| `breaking_by_failure` *(remove / remove_anyway only)* | If `true`, a **failed potency check** on any dispel from this block breaks the cast chain (no later stages run). The default is `false` (a failed check is silent). |

### 9.3 Potency-gated dispel *(new mechanic)*

Each affect imposed by `CastAffect` now **records the cast's potency** (the
roll value at the moment of imposition) and a debuff flag (matches the
spell's `violent`). When `CastUnaffects` tries to remove an affect, the
removal is gated by a strength contest:

* If the dispel is **non-violent** *and* the affect is **not a debuff** (a
  buff being purified by a non-violent cure) — **no check**, always removed.
* Otherwise — flat 5 % chance to remove regardless of strength, else the
  dispel's potency must exceed the affect's recorded potency:

  ```
  dispel_potency = (RollSkillDices + skill_coeff + stat_coeff) · potency_weight
  ```

This means a high-level necromancer's poison resists a weak novice's cure,
but the cure may still win on the 5 % free chance. Tune `potency_weight` on
the cure to make it stronger or weaker against the affects it targets.

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

### 9.5 `kDispellMagic` (random strip)

`kDispellMagic` is special-cased: it picks **one** eligible affect at random
from the victim's affect list (filtered by the spell's `affect_flags`) and
runs the potency check against it. The `<blocking>`/`<breaking>`/`<remove>`
sub-blocks are not used. Its `<unaffect>` therefore has no children — just
the attributes:

```xml
<unaffect affect_flags="kAfDispellable"/>
```

---

## 10. `<heal>` and `<area>` see §6 and §7 above.

---

## 11. `CastToPoints`, `CastToAlterObjs`, `CastSummon`, `CastCreation`, `CastManual`

These stages are gated by their `kMag…` flag and run dedicated logic in
`magic.cpp`. They are **partly data-driven**:

* `CastToPoints` (HP / MV restore) reads `<heal>` from the talent actions
  and the `kPointsToVict` message from `spell_msg.xml`. Spells with the
  `kMagPoints` flag and no `<heal>` are valid (move-restore, food/thirst
  reset) and use built-in logic.
* `CastToAlterObjs` runs a per-spell handler in code (bless, curse,
  invisible, poison, enchant, repair, etc.) and reads its messages from
  `spell_msg.xml` (`kAlterObjToChar`, `kEnchantNotWeapon`, …).
* `CastSummon` (animate dead, clone, summon keeper, …) is mostly code, with
  messages in `spell_msg.xml` (`kSummonToRoom`, `kSummonFail`,
  `kSummonNoCorpse`, etc.).
* `CastCreation`, `CastManual` are pure code stages.

The data-driven parts you can tune for these stages are: `<flags>`,
`<targets>`, `<misc>`, `<mana>`, and the spell's messages.

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
| `kPointsToVict` | A heal/restore landed. | Target |
| `kReflectedToChar` | Reflection: spell bounced back. | Caster |
| `kReflectedToVict` | Reflection: target who reflected. | Original victim |
| `kReflectedToRoom` | Reflection: onlookers. | Room |
| `kCantCastPosition`, `kCantCastSleeping`, … | Cast-precondition failures. | Caster |
| `kAreaToChar`, `kAreaToRoom`, `kAreaToVict` | Area cast announcement. | Various |
| `kSummonToRoom`, `kSummonFail`, `kSummonNoCorpse`, … | Summon outcomes. | Various |
| `kEnchantNotWeapon`, `kEnchantMagic`, `kEnchantSetItem`, `kEnchantMono`, `kEnchantPoly`, `kEnchantOther` | Enchant-weapon outcomes. | Caster |
| `kFightDeathToChar`, `kFightHitToChar`, `kFightMissToChar` (+ `ToVict`, `ToRoom`) | Combat-damage messages keyed by `ESpell`. | Various |

Messages are stored **without** the trailing `\r\n` — `act()` and
`SendMsgToChar` add it themselves.

A message lookup tries the spell's own sheaf first, then falls back to
`kDefault`. Imposition / dispel / reflection messages are exceptions:
they're looked up sheaf-directly with **no kDefault fallback**, so a
missing key just stays silent rather than leaking a wrong default.

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
                <amount min="0" dices_weight="1.0" competencies_weight="25"/>
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
            <blocking mob_flags="kNoSleep" affect_flags="kHold"/>
            <affects type="kSleep" saving="kWill" resist="kMind">
                <reposition pos="kSleep"/>
                <duration base="1" skill_divisor="15" min="1" max="6"/>
                <flags val="kAfBattledec|kAfDispellable|kAfCurable"/>
                <apply id="kSleep" location="kNone">
                    <modifier min="0.0" dices_weight="0.0"
                              competencies_weight="0.0" factor="1"/>
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
                    <modifier min="12.0" dices_weight="0.0"
                              competencies_weight="0.0" factor="1"/>
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
            <damage saving="kStability">
                <amount min="0" dices_weight="1.0" competencies_weight="122"/>
            </damage>
            <required align="kEvil"/>
            <caster_blocking align="kEvil"/>
        </action>
    </talent_actions>
</spell>
```

* `<required align="kEvil"/>` — the *target* must be evil, else `kNoeffect`.
* `<caster_blocking align="kEvil"/>` — the *caster* must not be evil, else
  `kNoeffect` (the old "wrath of light" behaviour, now data-driven).

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
                <amount min="0" dices_weight="1.0" competencies_weight="80"/>
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

### 13.7 Stacking poison

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
                    <modifier min="2.0" factor="-1" stack="3"/>
                </apply>
                <apply id="kPoisoned" location="kPoison">
                    <modifier min="30.0" factor="1" stack="3"/>
                </apply>
            </affects>
        </action>
    </talent_actions>
</spell>
```

* Re-casting on the same victim **stacks up to 3 times**: the strength
  penalty scales `-2`, `-4`, `-6` and the poison-damage tick grows
  `+30`, `+60`, `+90`.
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
`kAfDispellable` · `kAfCurable`.

### 14.7 `EMobFlag` allowed in `<blocking mob_flags>` / `<required mob_flags>`

`kNoBlind` · `kNoSleep` · `kNoSilence` · `kNoHold` · `kNoBash` · `kNoCharm`
· `kNoSummon` · `kNoFear` · `kCorpse` · `kResurrected` · `kMounting` ·
`kHelper` · `kClone`. Add new ones to the `kBlockingFlagByName` table in
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
   new (engineer task).
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
* **Room spells** (`kMagRoom`, `magic_rooms.cpp`) — same XML system but a
  parallel pipeline in `magic_rooms.cpp`.
* **Success roll consumption** — `<success_roll>` is parsed and evaluated
  at cast time but is not yet wired into the cast-success decision. The
  classical percent-based `CalcCastSuccess` still gates landing.
* **Skill-interference modifiers** — feats like `kMagicArrows` that boost
  specific spells (e.g. doubles `kMagicMissile` extra-hits) are still
  hard-coded in `CalcExtraHits` and similar helpers.

If you need any of these, open an issue and the work can be scoped.

---

*Last updated to match the `sventovit.work` head as of the `issue.dicerolls` fix
(dice parser now treats `0` as "no contribution"; absent `<dices>` block is
equivalent to `0,0,0`).*
