# Bylins MUD — Data-Driven Affect Manual

*For game designers authoring affects in `lib/cfg/affects.xml` (character affects)
and `lib/cfg/room_affects.xml` (room / exit affects), plus their message files.*

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

| File | Holds | Message file |
|---|---|---|
| `lib/cfg/affects.xml` | **character** affects (`EAffect`) | `lib/cfg/messages/<lang>/affect_msg.xml` |
| `lib/cfg/room_affects.xml` | **room / exit** affects (`ERoomAffect`) | `lib/cfg/messages/<lang>/room_affect_msg.xml` |

The message-file layout is validated against `lib/cfg/messages/affect_msg.scheme`.
As with spells, the small-world/test snapshot is a copy — edit the tracked files
under `lib.template/cfg` (or the live `lib/cfg`) and reload; do not hand-edit a
build's `small/` copy.

**Reload:** `reload affects` / `reload roomaffects` (and the message reloads) pick
up changes without a restart, exactly like `reload spells`.

---

## 2. The spell ↔ affect handoff

When a spell's `<affects><affect id="kX"/>` (Spell Manual §8) lands, the engine
builds one `Affect` instance **per apply** and hands ownership to the affect:

| Owned by the **affect** (`affects.xml`) | Owned by the **imposition** (the spell's / action's `<affects>` block) |
|---|---|
| `<flags>` — behaviour flags (`EAffFlag`) | `<duration>` — how long it lasts |
| `<apply>` — the stat/status changes | the stored **potency** (from the cast's potency roll) |
| `<shield>` — magic-shield weight | `saving=` / `resist=` — the landing contest |
| `<actions>` — everything it does over its life | `battleflag=` — per-instance decrement-timing override (§5.6) |

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
| `id` | the `EAffect` (char) or `ERoomAffect` (room) enum name. Required. |
| `buff` | `Y` = beneficial, `N` = harmful. Drives dispel/cure classification (a "dispel magic" strips debuffs, a "cancellation" strips buffs) and the violent/friendly nature of anything that re-applies it. Default: treated as harmful. |
| `dispel_mod` *(issue.random-noise-rework)* | Additive per-affect modifier to the **dispel-contest threshold**, in percentage points (Spell Manual §9.3). Negative = **harder** to remove, positive = easier. Default `0`. Critical buffs (fire/ice/air shield, sanctuary, prismatic aura, fly, water-breath) ship `dispel_mod="-35"` → ~15 % to dispel at competence parity, vs the ~50 % of an ordinary buff. |

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
| `kAfSameTime` | ticks in step with combat DoT processing (used by damage-over-time affects). |

Other common bits:

| flag | effect |
|---|---|
| `kAfDispellable` | can be removed by dispel-magic / cancellation. |
| `kAfCurable` | can be removed by cure-type spells / first-aid. |
| `kAfUpdateDuration` | re-imposing refreshes to the **longer** duration (instead of a 2nd instance). |
| `kAfAccumulateDuration` | re-imposing **adds** durations. |
| `kAfUpdateMod` | re-imposing keeps the **stronger** modifier. |
| `kAfMaterialize` | the affect can be *materialised* as an intrinsic mob buff (rolled from the same-named spell). |
| `kAfEntanglement` | movement-restriction category (footwork skills test this, not a specific spell). |

> **Multi-instance caveat.** A spell that imposes an affect with several `<apply>`
> children creates **several instances** (one per apply) that share a duration and
> expire together. This matters for `kExpired`/`kDeath` triggers — see §5.5.

### 4.2 `<apply>` — the affect's stat / status changes

```xml
<apply location="kInitiative">
    <modifier min="0.0" dices_weight="0.0" alpha="0" beta="0.9" factor="1" cap="0"/>
</apply>
```

`location` is an `EApply` (`kHitroll`, `kAc`, `kInitiative`, `kSavingReflex`, …).
The `<modifier>` is the **same option-2 formula** the Spell Manual §8.9 documents:

```
raw = min + ceil(dices·dices_weight·(1 + alpha·C) + beta·C)      C = skill_coeff + stat_coeff
raw = min(raw, cap)                                              (cap > 0 only)
modifier = factor · raw                                          (factor = -1 for debuffs)
```

**`C` (competence) is supplied by whatever imposes the affect.** A real spell
cast fills `C` from its potency roll (skill + stat). A *triggered affect-action*
(§5) supplies `C` from the **firing affect's stored potency** — so an affect can
impose another affect that scales with the first one's strength (this is how the
vampirism-kill haste scales, §7). A flat effect uses `min` only (`beta=0`).

> **Affect owns its applies.** If an affect defines `<apply>` children, every
> spell that imposes it uses *those* (the spell's own per-apply data is ignored).
> Give the applies to the affect; leave only `<duration>`/`saving`/`resist` on the
> spell. Legacy affects with no `<apply>` still fall back to the spell's applies.

#### 4.2.1 Tuning modifiers with competence *(the `beta` rules)*

Modifier magnitude should grow with **caster competence (skill + stat)**, not with
level/remort directly. Modifiers are almost always flat (`dices_weight=0`), so the
formula reduces to `min + ceil(beta·C)` and `beta` is the additive competence
weight (`alpha` only matters when `dices_weight≠0`).

Conversion from an OLD level/remort formula:

> **1 remort ≈ +5 skill cap + +1 base stat.** For the cookie-cutter potency roll
> (`low=3 hi=1.25`, stat `threshold=22 weight=0.5`), competence grows
> `(5·1.25 + 1·0.5)/100 = 0.0675` per remort. So a modifier meant to grow by `k`
> per remort needs `beta ≈ k / 0.0675`.

Two rules:

* **`min ≥ 0`.** Pick `min`/`beta` so an untrained caster (`C=0`) gets
  `min·factor` (0 or a clean baseline) — never let a `factor="-1"` debuff invert
  into a buff for a low-skill caster. If aggressive potency rolls force `min<0`,
  lower `beta` or normalise the roll (a few spells — kPatronage/kEviless/kCurse —
  stay on flat `min` without `beta` until their rolls are normalised).
* **Anchor a typical caster.** Set `beta ≈ |target at R12 max-skill| / C_at_R12`
  (round *down* to keep `min≥0`), then `min = |target at R12| − ceil(C_R12·beta)`.
  Example — `kBless` (old `-5 - R/3` on kSavingStability): `beta=2.8`, `min=0`,
  `factor=-1` → −7 at R0, **−9 at R12** (exact), −11 at R24.

#### 4.2.2 Stacking

`<modifier stack="N"/>` caps how many stacks one apply may build on a target. With
`stack="1"` (default) re-imposing updates/refuses per the `kAfUpdate…` flags. With
`stack>1`, re-imposing **adds a stack** (accumulating the modifier up to `N·mod`,
extending duration if `kAfAccumulateDuration`/`kAfUpdateDuration`). On dispel, one
stack is **peeled** (modifier shrinks proportionally) rather than the whole affect
removed — the affect vanishes only when the last stack goes. This makes
"stacking poison / build-up debuff" pure data.

#### 4.2.3 Random apply pools

Among sibling applies flagged `<apply … random="true">`, **exactly one** is chosen
per imposition (uniform). Unconditional applies always fire; the random ones share
one pool. E.g. `kFailure`: one guaranteed morale penalty + one random stat penalty
of six.

### 4.3 `<shield weight="N"/>`

For the magic-shield family (fire/ice/air): the relative weight with which this
shield is chosen to intercept an incoming hit when the bearer has several. Only
meaningful alongside a `kWardDamage` retaliation action (§5.4).

---

## 5. `<actions>` — the trigger-driven layer *(the new core)*

An affect's `<actions>` is an ordered list of `<action>` blocks. Each carries a
`<trigger>` naming **when** it fires and one or more **effect manifestations**
describing **what** it does. This is the same action vocabulary as a spell's
`<talent_actions>` (Spell Manual §3.8) — `<damage>`, `<points>`, `<affects>`,
`<unaffect>`, `<side_spell>`, `<manual>`, `<area>` — plus one affect-only
manifestation, `<damage_change>` (§5.4).

```xml
<actions>
    <action target="kTarFightSelf" base="tag" tag="amount">
        <trigger val="kPostHit"/>
        <points extra="100"><heal min="1" beta="0.10" .../></points>
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

### 5.2 `<action>` attributes

| attr | meaning |
|---|---|
| `target` | an `EActionTarget` (§5.3), default `kTarSame`. Who the action's effect lands on. |
| `base` | how the action's magnitude scales: default `kCompetence` (potency); `tag` = an event value. |
| `tag` | with `base="tag"`, the event field to read (e.g. `amount` = damage dealt / restored). |

`<trigger>` sub-attributes: `val` (required), `category` (kPoints filter),
`return` (`"0"` = block the triggering action — for `kDeath`, `kEnter*`, door
triggers).

### 5.3 Targets (`EActionTarget`)

`kTarSame` (previous action's targets), `kTarFightSelf` (the bearer),
`kTarFightVict` (the bearer's opponent), `kTarGroup` / `kTarFoes` (need `<area>`),
`kTarRandomFoe` / `kTarRandomAlly`, `kTarMinions` (charmed followers),
`kTarActor` (the char that triggered an event — entrant / dispeller / healer),
`kTarMaster` (the bearer's charm master — for master-side effects like kSoulLink),
`kTarRoomThis` (the room itself).

### 5.4 Effect manifestations

Inside an `<action>`, the same stages as a spell (Spell Manual §5–8), plus:

* `<damage>` — deal damage (a `kPulse`/`kBattlePulse` `<damage>` is a **DoT**; a
  `kWardAttack` reflect deals it back to the attacker). Give the affect its own
  `kDamageToChar/Vict/Room` messages (§6) so a DoT isn't silent.
* `<points><heal>` — restore HP/moves (a `kPulse` heal is a **HoT**; a `kPostHit`
  heal is a leech, scaled off `base="tag" tag="amount"`).
* `<affects>` — impose another affect (see §5.6 for the duration/flag controls).
* `<unaffect>` — dispel/cure (Spell Manual §9 grammar).
* `<side_spell id="…">` — cast a whole spell as a nested effect.
* `<manual name="…">` — hand-coded handler (selection is data; body is C++).
* `<damage_change>` — **affect-only, `kWardDamage`**: passively scale/edit an
  incoming damage instance. Structure:
  ```xml
  <damage_change>
      <conditions>
          <type val="kPhysDmg"/>              <!-- kPhysDmg / kMagicDmg -->
          <flags missing="kIgnoreSanct"/>     <!-- skip if the hit ignores this ward -->
      </conditions>
      <variation min="50" max="50" factor="-1"/>   <!-- -50% here -->
  </damage_change>
  ```
  Sub-forms used by the shield/ward family: `<retaliation>` (reflect a % back to
  the attacker), `<reflection>` / `<absorption>` (bounce / soak a cast),
  `<percent>`, `<element>`. These migrated the old hard-coded sanctuary / prism /
  fire-shield logic; copy an existing shield affect as a template.

### 5.5 `<charges max="N"/>` — bounded firings

A **triggered** affect can carry `<charges max="N"/>` inside its `<affects>`
imposition or its own block: the affect is stripped after firing `N` times,
independent of its duration. `-1` (default) = unlimited (duration-bound only). A
DoT imposed with `<charges max="6"/>` ticks exactly 6 times. Combine with a
`kExpired` action to react when the charges run out.

> **Dedup.** `kPulse` DoTs and `kExpired`/lifecycle triggers each fire **once per
> affect type per pass**, even when the affect has several instances (multi-apply
> affects). Author the on-expire/on-tick action once; it will not double-fire.

### 5.6 Duration & flag control on an imposing `<affects>`

When an affect-action imposes another affect, its `<affects>` block controls the
new instance:

* `<duration base="…" skill_divisor="…" min="…" max="…" skill="kX"/>` — the
  duration formula (Spell Manual §8.3). The **`skill="kX"`** attribute is
  affect-action–specific: a triggered action has no casting spell, so it names the
  skill whose novice-bonus scales the duration (e.g. a hangover scaling with
  `kHangovering`). Without `skill=`, a triggered action gets no skill scaling.
* `battleflag="kAfBattledec|…"` — OR extra decrement-timing flags onto the imposed
  instance. This lets an action grant a normally hours-long affect as a short,
  **combat-round-measured** one: with `kAfBattledec` set, the `<duration>` is read
  as **raw combat rounds** (no PC hours→ticks conversion). This is how the
  vampirism-kill haste (§7) grants a 5–6-round battle-haste.

---

## 6. Messages

Affect narration lives in `lib/cfg/messages/<lang>/affect_msg.xml` (and
`room_affect_msg.xml`), keyed per affect id. The engine reads these keys:

| key family | shown when |
|---|---|
| `kAffImposedToChar/Vict/Room`, `kAffImposeFailToChar/Vict/Room` | the affect lands / fails to land |
| `kAffExpiredToChar/Room` | it wears off (kExpired) |
| `kAffDispelledToChar/Room` | it is dispelled (kDispell) |
| `kDamageToChar/Vict/Room` | a `<damage>` action fires (give DoTs these, else they're silent) |
| `kWardToChar/Vict/Room` | a ward intercepts (kWardDamage/kWardAttack) |
| `kDeathToChar/Room` | a `kDeath` action fires |
| `kTransformToChar/Vict/Room`, `kTransformCrit…` | transform/polymorph flavour |

An empty key = the generic engine line (or silence, for damage). Populate at
least the impose/expire and any damage keys.

---

## 7. Room & exit affects (`room_affects.xml`)

Room/exit affects share the `<affect>`/`<flags>`/`<actions>` grammar but add:

* **On-entry & door triggers** — `kEnter`/`kEnterPC`/`kEnterNPC` and
  `kPick`/`kUnlock`/`kOpen`/`kClose`/`kLock`. A `<trigger return="0"/>` **refuses**
  the entry/door action (a sealed room, a trapped lock). `kTarActor` = the entrant.
* **`<area>` + `<targets>` + `<distribution>`** — a room action can hit everyone
  in the room with per-target falloff (Spell Manual §7 grammar).
* **`kService` side-spell** — a room affect's per-tick effect is often expressed
  as a `<side_spell>` to a linked service spell (`kTarRoomThis`).
* **`<target_conditions>`** — gate the action on the entrant/room (blocking/required).

---

## 8. Worked examples (all shipping)

* **DoT — `kBurning`:** `kAfSameTime` + a `kPulse` `<action target="kTarFightSelf">`
  with `<damage>` scaled by `dices_weight` off the stored potency, plus
  `kDamageTo*` messages. Ticks in and out of combat; bound by `<charges>`.
* **Leech + on-kill — `kVampirism`:** a `kPostHit` `<points><heal base="tag"
  tag="amount" beta="0.10">` (heal 10% of damage dealt) **and** a `kKill`
  `<affects battleflag="kAfBattledec"><duration base="6"><affect id="kHaste"/>` —
  a short battle-haste reward on a kill.
* **Stat buff/debuff — `kHaste`/`kSlowdown`:** a single `<apply location="kInitiative">`
  (`beta=0.9`, `factor=+1`/`-1`) — scales with the caster's skill; the pair stacks
  (net = sum) because both write the same `EApply`.
* **Ward — `kSanctuary`/`kFireShield`:** `kWardDamage` `<damage_change>` blocks
  (`-50%` phys / `+100%` magic for sanctuary; a fire shield adds a `<retaliation>`).
* **On-expire chain — `kDrunked` → `kAbstinent`:** a `kExpired`
  `<action target="kTarFightSelf"><affects><duration … skill="kHangovering"/>
  <affect id="kAbstinent"/>` — natural sober-up leaves a skill-scaled hangover
  (kAbstinent owns its AC/HR/DR applies).
* **Feat-modified affect — `kSoulLink`:** a *feat* patches `kVampirism` to also
  heal the minion's `kTarMaster`. Feat-side patching lives in the
  [Feat Manual](FEAT_MANUAL.md); the affect itself is untouched.

---

## 9. Reference

* **`EAffFlag`** (the `<flags val>` bits) — see §4.1; full list in
  `affect_contants.h`.
* **`EActionTrigger`** — see §5.1; canonical names in `talents_actions.cpp`.
* **`EActionTarget`** — see §5.3.
* **`EApply`** (apply `location=`) — shared with spells; Spell Manual §14.3 /
  `entities_constants.h`.
* **`EAffect` / `ERoomAffect`** — the affect ids; `affect_contants.h` /
  `magic_rooms` enums.

---

## 10. Checklist for adding / migrating an affect

1. **Add / find the `EAffect` id** and its `<affect id= buff=>` block.
2. **Passive part → `<apply>`** (own the applies; leave `<duration>` on the imposer).
3. **Pick decrement flags** (`kAfBattledec` for combat-round effects; none for
   mud-hour buffs) — this also decides which trigger loops see it.
4. **Behavioural part → `<actions>`**: choose the `<trigger>`, the `target`, and
   the manifestation (`<damage>` DoT / `<points>` HoT / `<affects>` impose /
   `<damage_change>` ward). Scale with `base="tag"` or the potency (`beta`).
5. **Messages** — at least impose/expire, plus `kDamageTo*` for any DoT.
6. **Room affect?** put it in `room_affects.xml` and consider entry/door triggers.
7. **Retire the hard-coded C++** the affect replaces (leave the id as a marker if
   other code still tests the flag).
8. **Verify** in mud-sim: impose it, tick/trigger it, confirm the effect + messages
   (note: the sim advances combat-round loops, not the mud-hour point-update — use
   `kAfBattledec` to observe expiry-based triggers).

---

*Companion documents: [Spell Manual](SPELL_MANUAL.md) · [Feat Manual](FEAT_MANUAL.md).*
See the **[Cookbook](COOKBOOK.md)** for complete worked examples.
