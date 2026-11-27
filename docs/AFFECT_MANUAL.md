# Bylins MUD — Data-Driven Affect Manual

*For game designers authoring affects in `lib/cfg/affects/affects.xml` (character
affects) and `lib/cfg/affects/room_affects.xml` (room / exit affects), plus their
message files.*

An **affect** is a persistent effect on a character, room, or exit — a buff, a
debuff, a damage-over-time, a shield, a summon marker, a status flag. Where the
[Spell Manual](SPELL_MANUAL.md) describes the *cast* (how a spell reaches its
targets and what it does *once*), this manual describes what an affect does *while
it lasts* and *when things happen to its bearer*.

Historically an affect was just a name + a bag of stat modifiers imposed by one
spell, and every behaviour beyond a stat change was hard-coded in C++. That has
been inverted: **the affect now owns its own data** — its behaviour flags, its
stat applies, its magic-shield weight, and (the big addition) an ordered list of
**trigger-driven `<actions>`**. A spell (or another affect, or a feat) merely
*imposes* the affect; the affect itself decides what it does from then on.

This manual assumes familiarity with the Spell Manual's shared vocabulary
(`<damage>`, `<points>`, `<area>`, `<affects>`, `<unaffect>`, the potency roll,
the `EApply`/`ESaving`/`EResist` enums). Those tags mean the same thing here; the
new material is the **trigger/action layer** (§5) and the affect-authoring rules.

---

## 1. Where affects live

All affect data lives under the **`lib/cfg/affects/`** subdirectory (moved here from
the `cfg/` root); message files stay under `lib/cfg/messages/<lang>/`:

| File | Holds | Message file |
|---|---|---|
| `lib/cfg/affects/affects.xml` | **character** affects (`EAffect`) | `lib/cfg/messages/<lang>/affect_msg.xml` |
| `lib/cfg/affects/room_affects.xml` | **room / exit** affects (`ERoomAffect`) | `lib/cfg/messages/<lang>/room_affect_msg.xml` |
| `lib/cfg/affects/equipment_affects.xml` | affects a **worn** item confers (`EEquipmentAffect`) — §8.1 | `lib/cfg/messages/<lang>/equipment_affect_msg.xml` |
| `lib/cfg/affects/obj_affects.xml` | affects that live **on an item** (`EObjAffect`) — §8.2 | `lib/cfg/messages/<lang>/obj_affect_msg.xml` |

Each `*.xml` has its `*.scheme` sidecar next to it (for Vedun editing). Edit the
tracked files under `lib/cfg/affects` (and `lib/cfg/messages/ru`). The small-world
test snapshot is a copy that `tools/meson/setup_world.py` makes from `lib/cfg` into a
build's `small/` directory — do not hand-edit that copy.

> **Message-file roots** were retagged to match the filename (the loader requires
> `root == name == id`): `affect_msg.xml` → root `<affect_msg>`, `room_affect_msg.xml`
> → `<room_affect_msg>` (both previously shared a generic `<msg_container>`). A custom
> copy with the old root **silently fails to load** — rename the root element.

**Reload.** Live reload targets: `reload affects` (`affects.xml`), `reload objaffects`
(`obj_affects.xml`), `reload affectmsg` (`affect_msg.xml`), `reload roomaffectmsg`
(`room_affect_msg.xml`). `room_affects.xml`, `equipment_affects.xml` and the
`equipment_affect_msg.xml` / `obj_affect_msg.xml` message files have no dedicated
target — they are reloaded only by `reload all`, which re-reads every registered
config. A reload changes what **new** impositions get; modifiers and flags already
stamped on live affect instances keep their old values until the affect is
re-imposed. (`reload spells` / `reload spellmsg` / `reload feats` are separate
commands.)

---

## 2. The spell ↔ affect handoff

When a spell's `<affects><affect id="kX"/>` (Spell Manual §8) lands, the engine
builds one `Affect` instance **per apply** and hands ownership to the affect:

| Owned by the **affect** (`affects.xml`) | Owned by the **imposition** (the spell's / action's `<affects>` block) |
|---|---|
| `<flags>` — behaviour flags (`EAffFlag`) | `<duration>` — how long it lasts |
| `<apply>` — the stat/status changes | the stored **potency** (from the cast's potency roll, scaled by `potency_weight=`) |
| `<shield>` — magic-shield weight | `saving=` / `resist=` / `prob=` — the landing contest and chance |
| `<actions>` — everything it does over its life | `battleflag=` — per-instance decrement-timing override (§5.6); `<charges>` — firing cap (§5.5) |

So two spells can impose the *same* affect with different durations and
strengths, but the affect's **behaviour** is defined once, in `affects.xml`.

> **Rule of thumb.** If the effect is "a number changes while the affect is on"
> → an `<apply>`. If it is "something happens *when* X occurs" (a tick, a hit, a
> kill, an expiry, an incoming attack) → an `<action>` with a `<trigger>`.

---

## 3. The `<affect>` element

```xml
<affect id="kBurning" buff="N">
    <flags val="kAfSameTime|kAfDispellable"/>
    <apply location="kInitiative"><modifier .../></apply>   <!-- 0..N -->
    <shield weight="10"/>                                    <!-- optional -->
    <actions> … </actions>                                   <!-- optional -->
</affect>
```

### 3.1 Header attributes

| attr | meaning |
|---|---|
| `id` | the `EAffect` enum name. Required. (Room affects use a separate `<room_affect id=>` element with an `ERoomAffect` name — §7.) |
| `buff` | `Y` = beneficial, `N` = harmful. Drives dispel/cure classification (a "dispel magic" strips debuffs, a "cancellation" strips buffs) and the violent/friendly nature of anything that re-applies it. Absent (or `A`) = ambiguous; the engine checks test for `Y`, so an ambiguous affect is handled as not-a-buff. The Vedun editor only accepts `Y`/`N`. |
| `dispel_mod` *(issue.random-noise-rework)* | Additive per-affect modifier to the **dispel-contest threshold**, in percentage points (Spell Manual §9.3). Negative = **harder** to remove, positive = easier. Default `0`. Critical buffs (fire/ice/air shield, magic glass, the elemental auras, sanctuary, prismatic aura, fly, water-breath and a few others) ship `dispel_mod="-35"` → ~15 % to dispel at competence parity, vs the ~50 % of an ordinary buff. |

An affect id may exist with an **empty body** (`<affect id="kCommander"/>`) — a
pure marker flag other code tests for. Adding data to it never breaks that.

---

## 4. Passive layers: `<flags>`, `<apply>`, `<shield>`

### 4.1 `<flags val="A|B|…">` — `EAffFlag` behaviour bits

The decrement-timing bits decide **when the affect's duration ticks down** — this
is the single most important thing to get right, because it also decides which
trigger loops see the affect:

| flag | duration decrements… |
|---|---|
| *(none)* | once per **mud-hour** (the out-of-combat point-update). |
| `kAfBattledec` | once per **combat round** while the bearer is fighting (battle-update). |
| `kAfPulsedec` | once per **affect pulse** (the faster mob/short-buff loop). |
| `kAfSameTime` | every 2 s for a PC, once a minute for an NPC, or in step with the combat round while fighting (used by damage-over-time affects). |

Other common bits:

| flag | effect |
|---|---|
| `kAfDispellable` | can be removed by dispel-magic / cancellation. |
| `kAfCurable` | can be removed by cure-type spells / first-aid. |
| `kAfUpdateDuration` | re-imposing refreshes to the **longer** duration (instead of a 2nd instance). |
| `kAfAccumulateDuration` | re-imposing **adds** durations. |
| `kAfUpdateMod` | re-imposing keeps the **stronger** modifier. |
| `kAfDeadkeep` | the affect survives the bearer's death. |
| `kAfMaterialize` | the affect can be *materialised* as an intrinsic mob buff (rolled from the same-named spell). |
| `kAfEntanglement` | movement-restriction category (footwork skills test this, not a specific spell). |
| `kAfPoison` | poison category ("all poisons" mechanics test this flag). |
| `kAfBoon` / `kAfWarding` / `kAfAegis` | buff classes for the class-targeted dispels (`<unaffect affect_flags="kAfBoon">` etc.): minor utility / ordinary combat buffs / strong defensive magic. |
| `kAfFullAbsorb` | total damage immunity (the magic cocoon, `kGodsShield`). |
| `kAfNoPositionBonus` / `kAfNoCritBonus` | while this is the active shield, the attacker gets no low-position / crit damage bonus (air / ice shield). |

The full list is in `src/gameplay/affects/affect_contants.h`. Some bits are **not**
for `affects.xml`: `kAfFromEquipment`, `kAfFromSet`, `kAfCharmBond`, `kAfFailed` are
set by the engine on individual instances, and `kAfNeedControl`,
`kAfCasterInRoom`/`kAfCasterInWorld`/`kAfCasterInWorldDelay`, `kAfUnique`,
`kAfMustBeHandled` belong to room affects (§7).

> **Multi-instance caveat.** A spell that imposes an affect with several `<apply>`
> children creates **several instances** (one per apply) that share a duration and
> expire together. This matters for `kExpired`/`kDeath` triggers — see §5.5.

### 4.2 `<apply>` — the affect's stat / status changes

```xml
<apply location="kInitiative">
    <modifier min="0" beta="0.4143" factor="1"/>
</apply>
```

`location` is an `EApply` (`kHitroll`, `kAc`, `kInitiative`, `kSavingReflex`, …).
The `<modifier>` formula (`ComputeApplyModifier`, shared by spell-side and
affect-side applies):

```
raw = min + ceil(beta·C·(1 + weight·d))      C = skill_coeff + stat_coeff, d = the cast's noise draw
raw = min(raw, cap)                          (cap > 0 only)
modifier = factor · raw                      (factor = -1 for debuffs)
```

| `<modifier>` attr | default | meaning |
|---|---|---|
| `min` | `0` | flat base. |
| `beta` | `0` | competence weight. |
| `weight` | `0` | weight on the cast's random noise draw; `0` = deterministic (every shipped affect). |
| `factor` | `1` | sign/multiplier; `-1` for debuffs. |
| `cap` | `0` | clamp on the pre-factor magnitude; `0` = no clamp. |
| `stack` | `1` | stack cap (§4.2.2). |

**`C` (competence) is supplied by whatever imposes the affect** (Spell Manual §3.7).
A real spell cast fills `C` from its potency roll (skill + stat). A *triggered
affect-action* (§5) supplies `C` from the **firing affect's stored potency** — so an
affect can impose another affect that scales with the first one's strength (this is
how the vampirism-kill haste scales, §9). A flat effect uses `min` only (`beta=0`).

> **Affect owns its applies.** If an affect defines `<apply>` children, every
> spell that imposes it uses *those* (the spell's own per-apply data is ignored).
> Give the applies to the affect; leave only `<duration>`/`saving`/`resist` on the
> spell. Legacy affects with no `<apply>` still fall back to the spell's applies.

#### 4.2.1 Tuning modifiers with competence *(the `beta` rules)*

Modifier magnitude should grow with **caster competence (skill + stat)**, not with
level/remort directly. Modifiers are deterministic (`weight=0`), so the formula
reduces to `min + ceil(beta·C)` and `beta` is the additive competence weight.

Conversion from an OLD level/remort formula:

> **1 remort ≈ +5 skill cap + +1 base stat** (`lib/cfg/remort.xml`: skill cap
> `increment="5"`). Above the novice threshold every skill point is worth
> `hi_skill_bonus`, so competence grows `(5·hi_skill_bonus + 1·stat_weight)/100` per
> remort. For the standard potency roll (`low_skill_bonus=1 hi_skill_bonus=7`, stat
> `threshold=22 weight=18`) that is `(5·7 + 1·18)/100 = 0.53` per remort. So a
> modifier meant to grow by `k` per remort needs `beta ≈ k / 0.53`.

Two rules:

* **`min ≥ 0`.** Pick `min`/`beta` so an untrained caster (`C=0`) gets
  `min·factor` (0 or a clean baseline) — never let a `factor="-1"` debuff invert
  into a buff for a low-skill caster. If an aggressive potency roll forces `min<0`,
  lower `beta` or normalise the roll; some affects deliberately stay flat
  (`beta=0`, e.g. `kPatronage`).
* **Anchor a typical caster.** Set `beta ≈ |target at R12 max-skill| / C_at_R12`
  (round *down* to keep `min≥0`), then `min = |target at R12| − ceil(C_R12·beta)`.
  Example — `kBless` ships `min=0 beta=1.2889 factor=-1` on `kSavingStability`
  (plus `beta=1.1508` on `kSavingWill`).

#### 4.2.2 Stacking

`<modifier stack="N"/>` caps how many stacks one apply may build on a target. With
`stack="1"` (default) re-imposing updates/refuses per the `kAfUpdate…` flags. With
`stack>1`, re-imposing **adds a stack** (accumulating the modifier up to `N·mod`,
extending duration if `kAfAccumulateDuration`/`kAfUpdateDuration`). On dispel, one
stack is **peeled** (modifier shrinks proportionally) rather than the whole affect
removed — the affect vanishes only when the last stack goes. This makes
"stacking poison / build-up debuff" pure data.

#### 4.2.3 Random apply pools

Among sibling applies flagged `<apply … random="Y">`, **exactly one** is chosen
per imposition (uniform). Unconditional applies always fire; the random ones share
one pool. E.g. `kFailure`: one guaranteed morale penalty + one random stat penalty
of six.

### 4.3 `<shield weight="N"/>`

For the magic-shield family (fire/ice/air): the relative weight with which this
shield is chosen to intercept an incoming hit when the bearer has several. Only
meaningful alongside the shield's `kWardDamage` actions (`<retaliation>` /
`<damage_change>`, §5.4).

---

## 5. `<actions>` — the trigger-driven layer *(the new core)*

An affect's `<actions>` is an ordered list of `<action>` blocks. Each carries a
`<trigger>` naming **when** it fires and one or more **effect manifestations**
describing **what** it does. This is the same action vocabulary as a spell's
`<talent_actions>` (Spell Manual §3.8) — `<damage>`, `<points>`, `<affects>`,
`<unaffect>`, `<side_spell>`, `<manual_cast>`, `<area>` — plus the ward
manifestations `<damage_change>`, `<retaliation>`, `<reflection>`, `<absorption>`
consumed by the `kWardDamage` / `kWardAttack` triggers (§5.4).

```xml
<actions>
    <action target="kTarFightSelf" base="tag" tag="amount">
        <trigger val="kPostHit"/>
        <points extra="100"><heal npc_coeff="0" min="1" beta="0.1" weight="0"/></points>
    </action>
</actions>
```

### 5.1 The trigger set (`<trigger val="…">`)

| trigger | fires when… | typical use | actor / notes |
|---|---|---|---|
| `kPulse` | every affect pulse, in or out of combat | DoT / HoT / periodic effect | no actor |
| `kBattlePulse` | an affect pulse, **only while combat is in the room** | combat-only DoT | no actor |
| `kPreHit` | the bearer lands a melee hit (pre-damage swing) | on-swing rider | event: weapon, skill |
| `kPostHit` | a landed hit resolved (post-damage) | leech, on-hit proc | event: amount, weapon, skill, actor=victim |
| `kKill` | the bearer dealt the **fatal** blow (direct or via DoT) | on-kill reward | actor=victim (dead — do **not** target it); use `kTarFightSelf`/`kTarGroup` |
| `kExpired` | this affect's timer/charges ran out | on-wear-off effect | no external actor; self/ally targets |
| `kDispell` | this affect was **forcibly** removed (dispel/cancellation) | sting the dispeller | actor = the dispeller (`kTarActor`) |
| `kDeath` | the bearer is dying (before raw kill) | death prevention / last gasp | actor=killer; `<trigger return="0"/>` + a self-heal *prevents* death |
| `kPoints` | the bearer was **restored** by an external caster | react to being healed | actor=healer; `category=` filters to kHeal/kMoves/kThirst/kFull |
| `kWardDamage` | an **incoming** damage instance on the bearer | passive damage scaling (sanctuary, prism) | see §5.4 |
| `kWardAttack` | the bearer is the **target** of an incoming (magic) attack | reflect / absorb the cast | fired once per cast at the entry gate |
| `kEnter` / `kEnterPC` / `kEnterNPC` | a char / PC / NPC enters (room affects) | on-entry effect / seal | actor = the entrant (`kTarActor`); `return="0"` refuses entry |
| `kPick`/`kUnlock`/`kOpen`/`kClose`/`kLock` | door commands (exit affects) | trapped / sealed doors | `return="0"` refuses the action |

Obj-affects add their own triggers (`kWeaponHit`, `kGet`/`kDrop`/`kEquip`/`kUnequip`/`kDecay`) — §8.2.

### 5.2 `<action>` attributes

| attr | meaning |
|---|---|
| `target` | an `EActionTarget` (§5.3), default `kTarSame`. Who the action's effect lands on. |
| `base` | how the action's magnitude scales: default `kCompetence` (potency); `tag` = an event value; `kDamage`/`kPoints`/`kAffects`/`kDispelled` = a prior action's result (Spell Manual §3.8.2). |
| `tag` | with `base="tag"`, the event field to read (e.g. `amount` = damage dealt / restored). |
| `reset` | `Y`/`true` — chain-reset flag (Spell Manual §3.8.2). |
| `id` | the block's stable name, used by feat patches (§9, `kSoulLink`). |

`<trigger>` sub-attributes: `val` (required; several `|`-separated values allowed),
`category` (kPoints filter), `return` (`"0"` = block the triggering action — for
`kDeath`, `kEnter*`, door triggers), `prob` (firing chance).

### 5.3 Targets (`EActionTarget`)

`kTarSame` (previous action's targets), `kTarFightSelf` (the bearer),
`kTarFightVict` (the bearer's opponent), `kTarGroup` / `kTarFoes` (need `<area>`),
`kTarRandomFoe` / `kTarRandomAlly`, `kTarMinions` (charmed followers),
`kTarActor` (the char that triggered an event — entrant / dispeller / healer),
`kTarMaster` (the bearer's charm master — for master-side effects like kSoulLink),
`kTarRoomThis` (the room itself).

### 5.4 Effect manifestations

Inside an `<action>`, the same stages as a spell (Spell Manual §5–8), plus:

* `<damage>` — deal damage (`<amount min= beta= weight=>`; a `kPulse`/`kBattlePulse`
  `<damage>` is a **DoT**, scaled off the stored potency via `beta`). Give the affect
  its own `kDamageToChar/Vict/Room` messages (§6) so a DoT isn't silent.
* `<points><heal>` — restore HP/moves (a `kPulse` heal is a **HoT**; a `kPostHit`
  heal is a leech, scaled off `base="tag" tag="amount"`).
* `<affects>` — impose another affect (see §5.6 for the duration/flag controls).
* `<unaffect>` — dispel/cure (Spell Manual §9 grammar).
* `<side_spell id="…">` — cast a whole spell as a nested effect.
* `<manual_cast handler="…">` — hand-coded handler (selection is data; body is C++).
* `<damage_change>` — **`kWardDamage`**: passively scale/edit an incoming damage
  instance. Structure:
  ```xml
  <damage_change>                             <!-- optional: prob=, stage="late", msg="crit|none" -->
      <conditions>
          <type val="kPhysDmg"/>              <!-- kPhysDmg / kMagicDmg / kPoisonDmg / kPureDmg / kUndefDmg -->
          <flags missing="kIgnoreSanct"/>     <!-- skip if the hit ignores this ward; also present= -->
      </conditions>
      <variation min="50" max="50" factor="-1"/>   <!-- -50% here -->
      <flags add="kShieldApplied"/>                <!-- optional: edit the hit's flags (add= / remove=) -->
  </damage_change>
  ```
* `<retaliation>` — **`kWardDamage`**: reflect a percent of the hit back to the
  attacker (`<conditions>`, `<percent min= max= npc_bonus= boss_bonus=>`,
  `<element type=>`; optional `passive=`, `prob=`).
* `<reflection>` / `<absorption>` — **`kWardAttack`**: bounce / soak an incoming cast
  (`<reflection min= max=>` contest; `<absorption scope="all|damage|affect" chance=
  prob=>`).

These migrated the old hard-coded sanctuary / prism / fire-shield logic; copy an
existing shield affect as a template.

### 5.5 `<charges max="N"/>` — bounded firings

A **triggered** affect can carry `<charges max="N"/>` inside the `<affects>` block
that imposes it (in `spells.xml`, `room_affects.xml`, or another action — not on the
affect's own `<affect>` entry): the affect is stripped after firing `N` times,
independent of its duration. `-1` (default) = unlimited (duration-bound only). A
DoT imposed with `<charges max="7"/>` ticks exactly 7 times (`kFireBlast` imposes
`kBurning` that way). Combine with a `kExpired` action to react when the charges run
out.

> **Dedup.** `kPulse` DoTs and `kExpired`/lifecycle triggers each fire **once per
> affect type per pass**, even when the affect has several instances (multi-apply
> affects). Author the on-expire/on-tick action once; it will not double-fire.

### 5.6 Duration & flag control on an imposing `<affects>`

When an affect-action imposes another affect, its `<affects>` block controls the
new instance:

* `<duration base="…" skill_divisor="…" min="…" max="…" skill="kX"/>` — the
  duration formula (Spell Manual §8.2). The **`skill="kX"`** attribute is
  affect-action–specific: a triggered action has no casting spell, so it names the
  skill whose novice-bonus scales the duration (e.g. a hangover scaling with
  `kHangovering`). Without `skill=`, a triggered action gets no skill scaling.
* `battleflag="kAfBattledec|…"` — OR extra decrement-timing flags onto the imposed
  instance. This lets an action grant a normally hours-long affect as a short,
  **combat-round-measured** one: with `kAfBattledec` set, the `<duration>` is read
  as **raw combat rounds** (no PC hours→ticks conversion). This is how the
  vampirism-kill haste (§9) grants a 5–6-round battle-haste.

---

## 6. Messages

Affect narration lives in `lib/cfg/messages/<lang>/affect_msg.xml` (and
`room_affect_msg.xml`), keyed per affect id (`<msg_sheaf id="kX">`, plus a shared
`kDefault` sheaf). The engine reads these keys:

| key family | shown when |
|---|---|
| `kAffImposedToChar/Vict/Room`, `kAffImposeFailToChar/Vict/Room` | the affect lands / fails to land |
| `kAffExpiredToChar/Room` | it wears off (kExpired) |
| `kAffExpireSoon` | to the room on an NPC's last tick before the affect ends |
| `kAffDispelledToChar/Room` | it is dispelled (kDispell) |
| `kDamageToChar/Vict/Room` | a `<damage>` action fires (give DoTs these, else they're silent) |
| `kWardToChar/Vict/Room` | a ward intercepts (kWardDamage/kWardAttack) |
| `kDeathToChar/Room` | a `kDeath` action fires |
| `kTransformToChar/Vict/Room`, `kTransformCrit…` | transform/polymorph flavour |
| `kShortDesc`, `kLook`/`kLookPoly`, `kShieldFrame`/`kShieldNoun…`, `kAuraFrame`/`kAuraNoun…` | display: the affect's short name (affect lists, identify), the look line on its bearer, and the merged shield/aura line |

An empty key = the generic engine line (or silence, for damage). Populate at
least the impose/expire and any damage keys.

---

## 7. Room & exit affects (`room_affects.xml`)

Room/exit affects are `<room_affect id="…">` entries (an `ERoomAffect` name) under the
`<room_affects>` root. They share the `<flags>`/`<actions>` grammar but add:

* **On-entry & door triggers** — `kEnter`/`kEnterPC`/`kEnterNPC` and
  `kPick`/`kUnlock`/`kOpen`/`kClose`/`kLock`. A `<trigger return="0"/>` **refuses**
  the entry/door action (a sealed room, a trapped lock). `kTarActor` = the entrant.
* **`<area>` + `<targets>` + `<distribution>`** — a room action can hit everyone
  in the room with per-target falloff (Spell Manual §7 grammar).
* **Per-tick effect as an action** — a `kPulse` action carries either a
  `<side_spell>` (an ordinary spell, usually on `kTarFoes` with `<area>`) or a
  `<manual_cast>` (a hand-coded handler, e.g. `kThunderstorm`). This replaced the
  old dedicated tick spell / tick handler.
* **`<target_conditions>`** — gate the action on the entrant/room (blocking/required).
* **`<seal_strength min= beta= factor= cap=>`** — the seal-strength formula of a
  sealing affect (`kForbidden`), shown as stars on look.
* **Room-affect flags** — `kAfNeedControl`, `kAfCasterInRoom`, `kAfCasterInWorld`,
  `kAfCasterInWorldDelay`, `kAfUnique` (§4.1).

---

## 8. Item affects *(new core — issue.equipment-affects / issue.obj-affects)*

Beyond character affects (§3–6) and room affects (§7), two affect classes are tied to
**items**:

* **Equipment affects** (`equipment_affects.xml`, §8.1) — what a **worn** item confers
  on its wearer (invisibility, fly, fire shield; also the debuffs of cursed gear). They
  live on the **character** while the item is worn.
* **Obj-affects** (`obj_affects.xml`, §8.2) — affects that live **on the item itself**
  (poison on a weapon, bless/curse, a trap).

### 8.1 Equipment affects (`equipment_affects.xml`)

Historically this was the "weapon affect" flag set (`EWeaponAffect`) — a misnomer, since
**any** worn item, not just a weapon, confers them. The rework renamed
(`EEquipmentAffect`) and data-drove it. Each entry is an `EEquipmentAffect` token (the
identity) with one `<impose>` child:

```xml
<equipment_affects>
    <affect id="kInvisibility">
        <impose flag="kInvisible" spell="kUndefined" timer="-1" power_percent="60"/>
    </affect>
    <affect id="kBlindness">
        <impose flag="kUndefined" spell="kBlindness" power_percent="60"/>
    </affect>
    <affect id="kFireShield">
        <impose flag="kFireShield" spell="kUndefined" timer="-1" power_percent="60"/>
    </affect>
</equipment_affects>
```

| `<impose>` attr | Meaning |
|---|---|
| `flag` | the `EAffect` bit set on the wearer (`kUndefined` = none, i.e. spell-driven). |
| `spell` | an `ESpell` cast on equip (`kUndefined` = none). |
| `timer` | duration in ticks; **`-1` = permanent while worn**; **absent = temporary / spell-driven** (the blind/sleep/hold/poison debuffs stay only while worn and drop the instant the item is removed — a permanent worn "sleep" must be impossible). |
| `power_percent` | power-scaling percent fed to the potency pipeline (below). Default `100`. |

Two idioms: a **persistent buff** (`flag="kX" spell="kUndefined" timer="-1"`) and a
**cursed-item debuff** (`flag="kUndefined" spell="kBlindness"` with no `timer`).

**Materialization & potency.** Equipment affects used to be flags re-derived on every
`affect_total` (no potency, undispellable). Now, on equip, `MaterializeEquipmentAffects`
adds one **real `Affect`** per entry that has both a `timer` and a `flag`, tagged with
**`kAfFromEquipment`** and `caster_id` = the item's id; `RemoveEquipmentAffects` strips
them on unequip (these affects are **not written** to the player file — they
re-materialize on wear). Strength no longer comes from a hardcoded table
(`GetApplyByEquipmentAffect` is gone): the affect's `<apply>`/`<modifier>` live in
`affects.xml`, and the magnitude is derived from the item (item level → skill and stat,
scaled by `power_percent`) and run through the **same potency pipeline a cast uses** —
deterministically (noise 0: an item bonus is fixed). This finally gives an equipment
affect a meaningful **dispel-contest strength** (was 0).

**Set** affects are unified under the same mechanism: **`kAfFromSet`** marks an affect
materialized by the active set (`reconcile_set_affects`); it belongs to the set, not any
one piece.

> **Resists, not affects.** `kProtectFromDark` / `kProtectFromMind` (bits 11–12) were
> **retired** from `EEquipmentAffect`: they are numeric resist parameters, not on/off
> affects. Grant them on the item directly via apply — **`EApply::kResistDark` /
> `EApply::kResistMind`**. Bits 11–12 are **reserved, do not reuse** (old object data
> stays valid; in OLC the slots show as `UNUSED`).

### 8.2 Obj-affects (`obj_affects.xml`)

An affect that lives **on the item** (not the character), with its own identity
`obj_affects::EObjAffect` (independent of `ESpell`/`EAffect`); materialized as a real
`Affect<EObjApply>` in the item's affect list. The registry file is short:

```xml
<obj_affects>
    <obj_affect id="kInvisible" flag="kInvisible" msg_case="kGen" see_affect="kDetectInvisible"/>
    <obj_affect id="kCurse"     flag="kNodrop"    msg_case="kGen" see_affect="kDetectMagic"/>
    <obj_affect id="kPoisoned" msg_case="kGen" see_affect="kDetectMagic">
        <actions>
            <action target="kTarActor">
                <trigger val="kWeaponHit"/>
                <manual_cast handler="WeaponPoisonHit"/>
            </action>
        </actions>
    </obj_affect>
    <obj_affect id="kBless" flag="kBless"   msg_case="kAcc" see_affect="kDetectMagic"/>
    <obj_affect id="kFly"   flag="kFlying"  msg_case="kGen" see_affect="kUndefined"/>
    <obj_affect id="kLight" flag="kGlow"    msg_case="kGen" see_affect="kUndefined"/>
    <obj_affect id="kDartTrap" see_affect="kDetectMagic">
        <actions>
            <action target="kTarActor">
                <trigger val="kPick|kOpen" return="0"/>
                <damage saving="kNone"><amount min="200" beta="0" weight="0"/></damage>
                <side_spell id="kPoison"/>
            </action>
        </actions>
    </obj_affect>
    <obj_affect id="kSuppressed" dispellable="0" see_affect="kUndefined"/>
</obj_affects>
```

| Attr | Meaning |
|---|---|
| `id` | the `EObjAffect` token (identity). |
| `flag` | the `EObjFlag` the affect **owns** while active: set on the item **instance** on impose, cleared on expiry **only if the item's PROTOTYPE lacks it** (an innate prototype flag is permanent). Empty = no owned flag (e.g. `kPoisoned` reads the poison from the affect itself). |
| `dispellable` | `0/1` (default `1`): whether a player effect (remove curse / dispel magic / darkness) may strip it. The prototype-innate-flag rule always applies separately. `kSuppressed` = `0`. |
| `msg_case` | grammatical case the item name (`%s`) takes in lifecycle messages: `kNom/kGen/kDat/kAcc/kIns/kPre` (default `kGen`). |
| `see_affect` | the `EAffect` an onlooker must carry to **see** the affect (`kUndefined` = always visible, e.g. fly/light; immortals and god `stat` always do). |
| `<actions>` | the same `talents_actions` repertoire as room affects, plus the obj triggers (below). |

**New obj triggers:** `kWeaponHit` (the item — a weapon — landed a blow; this is how
**poison-on-hit** works: `kPoisoned` → `<manual_cast handler="WeaponPoisonHit">` on
`kTarActor` = the strike victim) plus the item-lifecycle triggers
`kGet`/`kDrop`/`kEquip`/`kUnequip`/`kDecay`. The **`kDartTrap`** trap fires on
`kPick|kOpen` with `return="0"` (**blocks** the open), dealing `<damage>` (impersonal —
no attacker) and a `<side_spell id="kPoison">` to the actor who tripped it.

Serialization: an `OAff:` block, one `<token> <timer> <modifier> <potency>` line per
affect, closed by `~` (the 4th field, potency, is for suppressions; old 3-field lines
load potency 0). A legacy `TSpl:` (timed-spell) line is read and mapped onto
`EObjAffect` (the 4 poisons → `kPoisoned`; bless/fly/light → `kBless`/`kFly`/`kLight`).
**The world is not converted** — this is the player-object file format, backward
compatible.

### 8.3 Suppression & restoration *(issue.obj-suppressor-affect / issue.affect-suppression-dispell)*

Dispelling an equipment affect used to lose it until re-equip. Now the affect is
**suppressed on its source item** for a while and **auto-returns** when the suppression
expires:

* Dispelling a `kAfFromEquipment` affect calls `SuppressSourceEquipmentAffect`: it finds
  the source item (by `caster_id`) among the worn equipment and suppresses the affect on
  it for **10 game hours**; the real affect is then removed (the buff drops **now**) and
  the wearer is told when the magic will return. A set-conferred affect (`kAfFromSet`)
  has no single owner, so every worn piece of the set gets the suppression.
* Suppression is a **`kSuppressed` obj-affect** (one per suppressed `EAffect`), so it
  serializes and ticks like any other obj-affect. It **stores a potency** = the
  competence of the suppressing dispel (the resistance a later un-suppress must beat; a
  refresh keeps the **MAX**).
* **Paused offline:** the offline catch-up (rent/relog) skips `kSuppressed` (else it
  would expire offline and the buff would return on login). Other obj-affects (poison
  DoT, traps) keep ticking offline.
* On timer expiry (or early — by a spell) the equipment affect **auto-returns** to the
  wearer.

**Lifting it early — the weave-restoration spell (`kWeaveRestoration`).** A new wizard
spell (circle 4) that **contests** to lift a suppression and return the affect early. It
is cast on an item (in inventory or on the ground), or on a character (then it hits a
**random** suppressed worn item of the wearer). The `DispelContest` mechanic is
documented in Spell Manual §9.3.1 (the suppression potency is taken directly, no
`dispel_mod`; it always rolls; a miss erodes the suppression by `decay`%). Its action
grammar is
`<alter_obj handler="AlterWeaveRestore" collateral="first_suppressed" dispel_bonus= decay= bypass_noalter=>` (ibid).

**Suppression display** — four inspect surfaces:

* Item listings (`экипировка` / equipment and the like) show a compact marker
  ` <магия подавлена>` after the item.
* `осмотреть` (examine) shows a header and an indented column:
  ```
  Временно подавлено волшебство:
      летит (8 часов)
      доблесть (6 часов)
  ```
* `опознать` (identify) shows the same column under the header `Временно подавлены     :`.
* God `stat` shows one line: `Подавленные аффекты   : летит (8 часов), …`.
* Times are in **game hours** (the counter decrements once per MUD hour).

Related fixes: dispelled buffs now use **their own** removal message (chain
`kAffDispelled` → `kAffExpired` → `kDefault`, not the generic "heat washes over you"); a
permanent affect (`duration < 0`) renders as "(постоянно)" (permanent) in `аффекты`
(affects).

---

## 9. Worked examples (all shipping)

* **DoT — `kBurning`:** `kAfSameTime` + a `kPulse` `<action target="kTarFightSelf">`
  with `<damage saving="kNone"><amount min="1" beta="6" weight="0"/>`, scaled off the
  stored potency, plus `kDamageTo*` messages. Ticks in and out of combat; bound by the
  imposer's `<charges max="7"/>` (`kFireBlast`, the `kFireTrap` dispel sting).
* **Leech + on-kill — `kVampirism`:** a `kPostHit` `<points><heal base="tag"
  tag="amount" min="1" beta="0.1">` (heal 10% of damage dealt) **and** a `kKill`
  `<affects battleflag="kAfBattledec"><duration base="6"><affect id="kHaste"/>` —
  a short battle-haste reward on a kill.
* **Stat buff/debuff — `kHaste`/`kSlowdown`:** a single `<apply location="kInitiative">`
  (`beta=0.4143`, `factor=+1`/`-1`) — scales with the caster's skill; the pair stacks
  (net = sum) because both write the same `EApply`.
* **Ward — `kSanctuary`/`kFireShield`:** `kWardDamage` `<damage_change>` blocks
  (`-50%` phys / `+100%` magic for sanctuary); a fire shield adds a `<retaliation>`
  action (15 % reflected) and `stage="late"` reductions of 30–50 %.
* **On-expire chain — `kDrunked` → `kAbstinent`:** a `kExpired`
  `<action target="kTarFightSelf"><affects><duration … skill="kHangovering"/>
  <affect id="kAbstinent"/>` — natural sober-up leaves a skill-scaled hangover
  (kAbstinent owns its AC/HR/DR applies).
* **Feat-modified affect — `kSoulLink`:** a *feat* patches `kVampirism` to also
  heal the minion's `kTarMaster`. Feat-side patching lives in the
  [Feat Manual](FEAT_MANUAL.md); the affect itself is untouched.

---

## 10. Reference

* **`EAffFlag`** (the `<flags val>` bits) — see §4.1; full list in
  `affect_contants.h`.
* **`EActionTrigger`** — see §5.1; canonical names in `talents_actions.cpp`.
* **`EActionTarget`** — see §5.3.
* **`EApply`** (apply `location=`) — shared with spells; Spell Manual §14.3 /
  `affect_contants.h`.
* **`EAffect` / `ERoomAffect`** — the affect ids; `affect_contants.h` /
  `room_affects.h` enums.
* **`EEquipmentAffect`** — worn-item affects (§8.1); `equipment_affects.{h,cpp}`.
* **`EObjAffect`** — on-item affects (§8.2); `obj_affects.h`.

All of these headers live in `src/gameplay/affects/`, except `talents_actions.cpp`
(`src/gameplay/abilities/`).

---

## 11. Checklist for adding / migrating an affect

1. **Add / find the `EAffect` id** and its `<affect id= buff=>` block.
2. **Passive part → `<apply>`** (own the applies; leave `<duration>` on the imposer).
3. **Pick decrement flags** (`kAfBattledec` for combat-round effects; none for
   mud-hour buffs) — this also decides which trigger loops see it.
4. **Behavioural part → `<actions>`**: choose the `<trigger>`, the `target`, and
   the manifestation (`<damage>` DoT / `<points>` HoT / `<affects>` impose /
   `<damage_change>` ward). Scale with `base="tag"` or the potency (`beta`).
5. **Messages** — at least impose/expire, plus `kDamageTo*` for any DoT.
6. **Room affect?** put it in `room_affects.xml` as a `<room_affect>` and consider
   entry/door triggers.
7. **Retire the hard-coded C++** the affect replaces (leave the id as a marker if
   other code still tests the flag).
8. **Verify** in mud-sim: impose it, tick/trigger it, confirm the effect + messages
   (note: the sim advances combat-round loops, not the mud-hour point-update — use
   `kAfBattledec` to observe expiry-based triggers).
9. **Item affect?** these are separate subsystems — a worn-item affect → §8.1
   (`equipment_affects.xml`), an on-item affect → §8.2 (`obj_affects.xml`).

---

*Companion documents: [Spell Manual](SPELL_MANUAL.md) · [Feat Manual](FEAT_MANUAL.md).*
See the **[Cookbook](COOKBOOK.md)** for complete worked examples.
