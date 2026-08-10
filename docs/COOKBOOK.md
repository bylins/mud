# Bylins MUD — Cookbook (worked examples)

*Complete, cross-referenced examples for authors of `spells.xml`, `affects.xml`,
`room_affects.xml`, and `feats.xml`. Read the three reference manuals first —
[Spell](SPELL_MANUAL.md) · [Affect](AFFECT_MANUAL.md) · [Feat](FEAT_MANUAL.md) —
then use these as templates.*

> **Section references.** Bare `§N` numbers point to the **Spell Manual** unless a
> link says otherwise (e.g. "[Affect Manual](AFFECT_MANUAL.md) §5").

---

## 1. A curse across all three systems — "Hex of Decay"

One scenario that touches every system at once. It demonstrates:

* a spell that only **imposes** an affect (all behaviour lives on the affect);
* an affect with **several triggers** — a per-tick DoT *and* a nasty **on-expiry**
  payload;
* the **`kExpired` vs `kDispell`** distinction — the hex is *safe to dispel*, but
  letting it run out backfires onto the bearer's allies;
* several **feats** that reshape the spell and both affects via `talent_patch`.

### 1.1 The spell — `spells.xml`

`kHexOfDecay` is a dark single-target curse whose only job is to impose the affect.

```xml
<spell id="kHexOfDecay" element="kDark" mode="kEnabled">
    <name rus="порча увядания" eng="hex of decay"/>
    <components><verbal/><weave/></components>
    <misc pos="kFight" violent="Y" danger="1"/>
    <mana max="60" min="45" change="1"/>
    <targets val="kTarCharRoom|kTarFightVict"/>
    <flags val="kMagAffects|kNpcAffectPc"/>
    <potency_roll>
        <dices ndice="5" sdice="9" adice="12"/>
        <base_skill id="kDarkMagic" low_skill_bonus="3" hi_skill_bonus="1.25"/>
        <base_stat id="kInt" threshold="22" weight="0.5"/>
    </potency_roll>
    <success_roll>
        <base_skill id="kDarkMagic" low_skill_bonus="3" hi_skill_bonus="1.25"/>
    </success_roll>
    <talent_actions>
        <action>
            <affects saving="kWill" resist="kDark">
                <duration base="6" skill_divisor="4" min="4" max="10"/>
                <affect id="kDecayHex"/>
            </affects>
        </action>
    </talent_actions>
</spell>
```

Everything the curse *does* is on `kDecayHex`; the spell just delivers it (§8) with
a Will save and a skill-scaled duration.

### 1.2 The affect — `affects.xml`, **multiple triggers**

`kDecayHex` is **dispellable and curable** (safe to remove) and carries two
trigger actions — a DoT while active, and the payload on natural expiry.

```xml
<affect id="kDecayHex" buff="N">
    <flags val="kAfDispellable|kAfCurable"/>
    <actions>
        <!-- (a) WHILE ACTIVE: a mild gnawing DoT every tick. -->
        <action target="kTarFightSelf">
            <trigger val="kPulse"/>
            <damage saving="kNone">
                <amount min="0" beta="0.4"/>
            </damage>
        </action>

        <!-- (b) ON NATURAL EXPIRY: the payload. Blight SEVERAL of the bearer's
             allies. There is deliberately NO kDispell action -> dispelling the hex
             is safe; only the kExpired path fires this. -->
        <action target="kTarGroup">
            <trigger val="kExpired"/>
            <area>
                <targets skill_divisor="40" dice_size="2" min="3" max="2" stat_weight="0"/>
                <distribution type="kUniform"/>
            </area>
            <affects saving="kNone">
                <duration base="4" skill_divisor="0" min="0" max="0"/>
                <affect id="kBlightCurse"/>
            </affects>
        </action>
    </actions>
</affect>
```

* **`kPulse` (a)** gnaws the bearer each tick — `beta="0.4"` on the stored
  potency ([Affect Manual](AFFECT_MANUAL.md) §4.2). Give it a `kDamageToChar/Room`
  message so it isn't silent.
* **`kExpired` (b)** is the twist. On natural timeout it hits `kTarGroup` (the
  bearer's groupmates), with `<area>` capping the hit at **three-to-five** of them
  (`min=3` + a small skill-scaled bonus, `max=2`). Each catches `kBlightCurse`
  (§1.3) for 4 hours.
* **Dispel with impunity.** Because there is **no `kDispell` action**, removing the
  hex early (dispel magic / cure) fires *nothing* — the payload triggers only on
  the *natural* `kExpired` path. That `kExpired`-vs-`kDispell` contrast is the whole
  design: a smart target dispels the hex; an ignored hex punishes the group.
  *(Want one random ally instead of a group? target `kTarRandomAlly` and drop the
  `<area>`.)*

### 1.3 The payload affect — `affects.xml`

`kBlightCurse` is the "much nastier" debuff the allies catch: flat stat penalties
plus a stronger DoT.

```xml
<affect id="kBlightCurse" buff="N">
    <flags val="kAfSameTime|kAfCurable"/>
    <apply location="kStr"><modifier min="3.0" beta="0" factor="-1"/></apply>
    <apply location="kDex"><modifier min="3.0" beta="0" factor="-1"/></apply>
    <apply location="kHitroll"><modifier min="5.0" beta="0" factor="-1"/></apply>
    <actions>
        <action target="kTarFightSelf">
            <trigger val="kPulse"/>
            <damage saving="kNone"><amount min="0" beta="0.9"/></damage>
        </action>
    </actions>
</affect>
```

`kAfCurable` **but not** `kAfDispellable` — a blight you *cure*, not dispel — and
its DoT (`0.9`) bites more than twice as hard as the hex's (`0.4`). Its flat
`<apply>` penalties don't need caster competence, so they land at full strength
even though the imposer is an *affect* (the on-expiry action), not a live cast.

### 1.4 How it plays out

1. Caster lands `kHexOfDecay` → victim gets `kDecayHex` (Will save, ~5–15 h).
2. Each tick, `kDecayHex` gnaws the victim a little (`kPulse`).
3. The victim (or an ally) can **dispel/cure** `kDecayHex` at any time — nothing
   bad happens (no `kDispell` action).
4. If nobody removes it, the timer runs out → `kExpired` fires → 3–5 of the
   victim's groupmates catch `kBlightCurse` for 4 h (big stat hit + a hard DoT).

---

## 2. Feats that reshape it — `feats.xml`

Five feats, covering `modify`/`append`, spell-patch vs affect-patch, and
`relative` gating (see [Feat Manual](FEAT_MANUAL.md) §4).

```xml
<!-- 1) modify an AFFECT field: the caster's hex gnaws harder (DoT x1.6). -->
<feat id="kHexweaver" mode="kEnabled">
    <talent_patches>
        <talent_patch affect="kDecayHex" op="modify" effect="kDamage">
            <modify field="beta" mul="1.6"/>
        </talent_patch>
    </talent_patches>
</feat>

<!-- 2) modify an AFFECT's area: the payload catches more allies (+2 to the floor). -->
<feat id="kSpreadingBlight" mode="kEnabled">
    <talent_patches>
        <talent_patch affect="kDecayHex" op="modify" effect="kArea">
            <modify field="min_targets" add="2"/>
        </talent_patch>
    </talent_patches>
</feat>

<!-- 3) append to an AFFECT's kExpired: the expiry also silences the blighted allies. -->
<feat id="kCruelHex" mode="kEnabled">
    <talent_patches>
        <talent_patch affect="kDecayHex" op="append">
            <action target="kTarGroup">
                <trigger val="kExpired"/>
                <area><targets min="3" max="2" dice_size="2" skill_divisor="40"/></area>
                <affects saving="kWill" resist="kDark">
                    <duration base="2" skill_divisor="0" min="0" max="0"/>
                    <affect id="kSilence"/>
                </affects>
            </action>
        </talent_patch>
    </talent_patches>
</feat>

<!-- 4) append to the SPELL: the caster's hex also deals a little direct dark damage on cast. -->
<feat id="kBlightbolt" mode="kEnabled">
    <talent_patches>
        <talent_patch spell="kHexOfDecay" op="append">
            <action target="kTarFightVict">
                <damage saving="kStability">
                    <amount min="10" beta="0.5"/>
                </damage>
            </action>
        </talent_patch>
    </talent_patches>
</feat>

<!-- 5) relative gating: a good group LEADER softens the fallout. If a blighted ally
     is led by someone with this feat, their kBlightCurse DoT is halved -- gated on
     the LEADER's feat, not the ally's. -->
<feat id="kWardingLeader" mode="kEnabled">
    <talent_patches>
        <talent_patch affect="kBlightCurse" relative="group_leader" op="modify" effect="kDamage">
            <modify field="beta" mul="0.5"/>
        </talent_patch>
    </talent_patches>
</feat>
```

What each shows:

| feat | target | op | mechanic taught |
|---|---|---|---|
| `kHexweaver` | affect `kDecayHex` | `modify` `kDamage` | scale a manifestation's numeric field |
| `kSpreadingBlight` | affect `kDecayHex` | `modify` `kArea` | scale an area count (`min_targets`) |
| `kCruelHex` | affect `kDecayHex` | `append` | add a whole action to a trigger chain |
| `kBlightbolt` | **spell** `kHexOfDecay` | `append` | patch the *spell*, not the affect |
| `kWardingLeader` | affect `kBlightCurse` | `modify` + `relative="group_leader"` | gate a patch on a *relative's* feat |

All are gated on `CanUseFeat(holder, feat)` and cost nothing at cast time for
characters without them.

---

## 3. Recipe gallery

Shorter single-purpose examples (relocated from the Spell Manual).


> **Two files.** Examples that impose an affect show *both* the spell block (which
> names the affect via `<affect id="…"/>` and owns duration/save) **and** the affect
> block in `affects.xml` (which owns the flags/applies/actions). See §8 and the
> [Affect Manual](AFFECT_MANUAL.md).

### 3.1 A simple damage spell with extra hits

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
                <amount min="0" beta="12.5"/>
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

### 3.2 A debuff with saving throw, reposition, and lag

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
            <affects saving="kWill" resist="kMind">
                <reposition pos="kSleep"/>
                <duration base="1" skill_divisor="15" min="1" max="6"/>
                <affect id="kSleep"/>
            </affects>
        </action>
    </talent_actions>
</spell>
```

```xml
<!-- affects.xml: kSleep is a pure status flag (no numeric apply) -->
<affect id="kSleep" buff="N">
    <flags val="kAfBattledec|kAfDispellable|kAfCurable"/>
</affect>
```

* Targets are NPCs flagged `kNoSleep` or anyone holding the `kHold` affect
  are immune (the gate fires before any roll).
* A successful `kWill` save averts the sleep.
* On a successful imposition, the victim drops to `kSleep` position.
* Duration: `1 + min(kMindMagic, 75) / 15` hours, clamped `[1..6]`.
* `kAfDispellable|kAfCurable` means both `kDispellMagic` and
  `kRemovePoison`-style cures can attempt removal (subject to potency
  contest).

### 3.3 A pure cure (potency-gated)

```xml
<spell id="kRemovePoison" element="kLife" mode="kEnabled">
    <name rus="лечить яд" eng="remove poison"/>
    <misc pos="kFight" violent="N" danger="0"/>
    <mana max="60" min="45" change="2"/>
    <targets val="kTarCharRoom|kTarFightSelf|kTarObjInv|kTarObjRoom"/>
    <flags val="kMagUnaffects|kNpcUnaffectNpc"/>
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
            <alter_obj handler="AlterRemovePoison"/>
        </action>
    </talent_actions>
</spell>
```

* `affect_flags="kAfCurable"` — won't touch affects that are only
  `kAfDispellable` (e.g. blessings).
* `<remove all_of="…">` — tries every listed poison present on the target.
* `<alter_obj handler="AlterRemovePoison"/>` — the same spell cast on a poisoned
  food/drink cleans the object instead (§3.8.7); both manifestations live in the
  one action.
* Each removal is a d100 skill contest (Spell Manual §9.3). A cure ships
  `dispel_bonus="80"`, so a priest reliably clears a same-skill poison; a master
  assassin's stronger `poison` (higher recorded competence) lowers the threshold —
  a much weaker priest falls toward the 5 % floor, a stronger one reaches the 95 % ceiling.

### 3.4 Reveal with chain-break

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
                <targets skill_divisor="15" dice_size="3" min="5" max="20"/>
                <distribution type="kStepped" decay="0.05" free_targets="5"/>
            </area>
            <unaffect>
                <remove all_of="kInvisible|kCamouflage|kHide" breaking_by_failure="true"/>
            </unaffect>
            <affects saving="kReflex" resist="kEarth">
                <duration base="4" skill_divisor="0" min="0" max="0"/>
                <affect id="kGlitterDust"/>
            </affects>
        </action>
    </talent_actions>
</spell>
```

```xml
<!-- affects.xml: the affect owns its flags and the saving-throw penalty -->
<affect id="kGlitterDust" buff="N">
    <flags val="kAfDispellable|kAfCurable"/>
    <apply location="kSavingReflex">
        <modifier min="0.0" beta="4.4" factor="1"/>
    </apply>
</affect>
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
  stat at threshold) gets `+ceil(0·4.4) = 0` — see the [Affect Manual](AFFECT_MANUAL.md) §4.2.1 for the competence-scaling rules.

### 3.5 Alignment-restricted spell

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
    <caster_conditions>
        <blocking>
            <align val="kEvil"/>
        </blocking>
    </caster_conditions>
    <talent_actions>
        <action>
            <target_conditions>
                <blocking>
                    <room_flags val="kNoMagic"/>
                </blocking>
                <required>
                    <align val="kEvil"/>
                </required>
            </target_conditions>
            <damage saving="kStability">
                <amount min="0" beta="61"/>
            </damage>
        </action>
    </talent_actions>
</spell>
```

* `<caster_conditions><blocking><align val="kEvil"/></blocking></caster_conditions>` —
  the *caster* must not be evil, else `kNoeffect` (the old "wrath of light" behaviour,
  now data-driven). Spell-level, checked once before any target.
* `<target_conditions><required><align val="kEvil"/></required></target_conditions>` —
  the *target* must be evil, else `kNoeffect`.
* `<target_conditions><blocking><room_flags val="kNoMagic"/></blocking></target_conditions>` —
  universal NOMAGIC-room fizzle added to all non-warcry spells; emits `kCastForbidden*`
  (see §15).

### 3.6 A reflection template

No spell currently uses `<reflection>` in production (it's plumbed and
ready). Here's how a fire-shielded creature might reflect cold spells:

```xml
<spell id="kIceBolt" element="kWater" mode="kEnabled">
    <!-- … usual fields … -->
    <talent_actions>
        <action>
            <reflection affect_flags="kFireShield|kFireAura" prob="35"/>
            <damage saving="kReflex">
                <amount min="0" beta="40"/>
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

### 3.7 Stacking poison (hypothetical)

The real `kPoison` doesn't stack today — this example illustrates the
mechanic. The actual `kPoison` uses `beta` to scale the
poison-damage tick (no stacking); here we re-add `stack="3"` to show
how a stacking variant would be authored.

```xml
<spell id="kPoison" element="kDark" mode="kEnabled">
    <!-- … usual fields … -->
    <talent_actions>
        <action>
            <blocking><affect_flags val="kGodsShield"/></blocking>
            <affects saving="kCritical" resist="kDark">
                <duration base="0" skill_divisor="3" min="0" max="0"/>
                <affect id="kPoison"/>
            </affects>
        </action>
    </talent_actions>
</spell>
```

```xml
<!-- affects.xml: the affect owns the stacking applies (str penalty + poison tick) -->
<affect id="kPoison" buff="N">
    <flags val="kAfSameTime|kAfAccumulateDuration|kAfCurable"/>
    <apply location="kStr">
        <modifier min="2.0" beta="0" factor="-1" stack="3"/>
    </apply>
    <apply location="kPoison">
        <modifier min="0.0" beta="11.5" factor="1" stack="3"/>
    </apply>
</affect>
```

*(A real poison DoT would drive its damage from a `kPulse` `<damage>` action — see
the [Affect Manual](AFFECT_MANUAL.md) §5; this apply-based form illustrates
stacking.)*

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

### 3.8 A multi-action spell: `kSacrifice` *(per-action targets + cast-chain)*

`kSacrifice` ("высосать жизнь") is the reference multi-action spell: it damages a
foe, then heals the caster by the damage dealt, then heals the caster's **undead**
minions by the same amount. Three `<action>` blocks, each with a different target.

```xml
<spell id="kSacrifice" element="kDark" mode="kEnabled">
    <name rus="высосать жизнь" eng="sacrifice"/>
    <components><verbal/><weave/></components>
    <misc pos="kFight" violent="Y" danger="10"/>
    <mana max="140" min="75" change="3"/>
    <targets val="kTarCharRoom|kTarFightVict|kTarNotSelf"/>
    <flags val="kMagDamage|kNpcDamagePc|kNpcDamagePcMinhp"/>
    <potency_roll>
        <dices ndice="8" sdice="15" adice="60"/>
        <base_skill id="kDarkMagic" low_skill_bonus="2" hi_skill_bonus="2.5"/>
        <base_stat id="kWis" threshold="32" weight="4.5"/>
    </potency_roll>
    <talent_actions>
        <!-- A1 — damage ONE foe (the entry action; uses the spell-level target). -->
        <action>
            <damage saving="kWill">
                <amount min="0" beta="62"/>
            </damage>
            <target_conditions>
                <blocking><affect_flags val="kCharmed"/></blocking>   <!-- never your own charmed pet -->
            </target_conditions>
        </action>
        <!-- A2 — heal the CASTER by the damage just dealt (base=kDamage). -->
        <action target="kTarFightSelf" base="kDamage">
            <points extra="300">
                <heal min="0" beta="1.0" npc_coeff="0"/>
            </points>
        </action>
        <!-- A3 — heal the caster's UNDEAD minions by the same amount. -->
        <action target="kTarMinions" base="kDamage">
            <points extra="300">
                <heal min="0" beta="1.0" npc_coeff="0"/>
            </points>
            <area>
                <targets max="-1"/>             <!-- all minions, no scaling -->
                <distribution type="kUniform"/>
            </area>
            <target_conditions>
                <required><mob_flags val="kCorpse"/></required>   <!-- undead only -->
            </target_conditions>
        </action>
    </talent_actions>
</spell>
```

What each piece does:

* **A1 (entry action)** uses the spell's own `<targets>`/`kMagDamage` scope — one
  chosen foe — and deals `kWill`-saved damage. `<target_conditions><blocking>` on
  `kCharmed` refuses to drain your own pet. Because it's the entry action, it also
  carries the spell's gates and the victim reaction (deferred to the end).
* **A2 (`target="kTarFightSelf"`, `base="kDamage"`)** re-aims at the caster and
  **heals**. `base="kDamage"` swaps the competence scalar for *the HP A1 actually
  removed*; with `beta="1.0"` (and `base="kDamage"`) the heal equals the damage
  (a clean 1:1 transfer). `<points extra="300">` lets the heal overheal to 4× max.
* **A3 (`target="kTarMinions"`, `base="kDamage"`)** repeats that heal on the
  caster's charmed NPC followers, but its `<required><mob_flags val="kCorpse"/>`
  limits it to **undead** minions, and its `<area><targets max="-1"/>` fans the
  heal out to **all** of them uniformly. Note `<targets max="-1"/>` deliberately
  omits `skill_divisor`/`dice_size`/`min`/`stat_weight` — they default (§7).

Things this example illustrates:

* **Per-action targets** — A1 hits a foe, A2 the caster, A3 the minions; the
  spell re-aims mid-cast (§3.8.1).
* **Cast-chain** — A2 and A3 read the running **damage** accumulator instead of
  re-rolling competence (§3.8.2). The accumulator captures *actual HP removed*
  (overkill and resists already applied), so the heal can never exceed the damage.
* **Per-action area** — only A3 fans out; A1/A2 are single-target.
* **One reaction** — the foe retaliates once, after A3, not between actions.

> The single-target version of "deal damage, then heal by it" needs only A1 + A2;
> A3 is the undead-minion bonus. A *mass* version would instead make A1 fan out
> (`kMagMasses` + an `<area>` on A1).

---

## 4. The Plague Blade — all three affect kinds (item, room, character)

Where §1 runs one scenario through **spell / affect / feat**, this one runs through
the **three affect subsystems**: an affect **on an item** (`obj_affects.xml`), a
**room** affect (`room_affects.xml`), and **character** affects (`affects.xml`). A
sword is temporarily plague-tainted; on hit it has a chance to infect; the infection
escalates on expiry, spreading through victims **and** leaving a miasma in the room
that infects anyone who enters.

Infection chain:

```
spell -> kPlagueBlade (item affect, on the sword, timed)
   hit (15%) -> kPlagueTouch (char affect: weak DoT, -Str/-Int; curable & dispellable)
      expiry -> kSwampPlague on the victim + 3 allies (char affect: heavy DoT)
         tick (5%) -> kSwampPlague to a random ally in the room  (char -> char)
         tick (5%) -> kPlagueMiasma on the room (room affect)    (char -> room)
            entry  -> kPlagueTouch on the entrant                (room -> char)
```

> **What is code vs. data.** Affect ids are fixed C++ enums, so a **new id cannot be
> declared in XML alone**. Before the data resolves, add one enumerator **and** one
> name-map string per id:
>
> | New id | Enum / name map |
> |---|---|
> | `kPlagueTouch`, `kSwampPlague` | `EAffect` — `affect_contants.h` + `affect_contants.cpp` |
> | `kPlagueMiasma` | `ERoomAffect` — `magic_rooms.h` + name map |
> | `kPlagueBlade` | `EObjAffect` — `obj_affects.h` + `obj_affects.cpp` |
>
> Plus one tiny `<alter_obj>` handler (§4.1) that puts a *timed* affect on the
> weapon. **Everything else below is pure data** (flags, applies, DoT, triggers,
> chances). That is the data ↔ code boundary.

### 4.1 The spell — `spells.xml` (+ a tiny handler)

`kInfectBlade` ("infect blade") is cast on a weapon and, via `<alter_obj>`, puts a
**timed** `kPlagueBlade` affect on it.

```xml
<spell id="kInfectBlade" element="kDark" mode="kEnabled">
    <name val="infect blade"/>
    <components><verbal/><weave/><material/></components>
    <misc pos="kStand" violent="N" danger="1"/>
    <mana max="50" min="35" change="1"/>
    <targets val="kTarObjInv"/>
    <potency_roll>
        <base_skill id="kDarkMagic" low_skill_bonus="2" hi_skill_bonus="2.5"/>
        <base_stat id="kInt" threshold="22" weight="0.5"/>
    </potency_roll>
    <success_roll>
        <base_skill id="kDarkMagic" low_skill_bonus="3" hi_skill_bonus="1.25"/>
    </success_roll>
    <talent_actions>
        <action>
            <alter_obj handler="AlterInfectBlade"/>
        </action>
    </talent_actions>
</spell>
```

The shipped `<alter_obj>` handlers (`AlterBless`/`AlterCurse`) impose a **permanent**
affect (`obj_affects::Impose(obj, …, -1)`). A *timed* one needs its own handler — a
copy of `AlterCurse` that passes a positive timer instead of `-1` (sketch):

```cpp
// src/gameplay/handlers/alter_infect_blade.cpp — temporarily taints a weapon with plague.
EStageResult AlterInfectBlade(ActionContext &ctx) {
    ObjData *obj = ctx.ovict;
    if (!obj || GET_OBJ_TYPE(obj) != EObjType::kWeapon) {
        return EStageResult::kFail;                 // -> generic "no effect"
    }
    const int timer = 24;                           // game-hours; -1 would be permanent
    obj_affects::Impose(obj, obj_affects::EObjAffect::kPlagueBlade,
                        timer, /*modifier*/0,
                        ctx.caster() ? ctx.caster()->get_uid() : 0,
                        ctx.CompetenceBase());       // strength -> item-affect potency
    return AlterMsg(ctx, ESpellMsg::kAlterObjToChar);
}
```

Register it in the `kAlterObjHandlers` registry (`magic.cpp`):
`{"AlterInfectBlade", handlers::AlterInfectBlade}`. The timer is `Impose`'s `duration`
argument; `obj_affects::Tick` decrements it once per game hour.

### 4.2 The item affect — `obj_affects.xml`

On hit, `kPlagueBlade` has a chance to inflict `kPlagueTouch` on the victim. Because
the payload is a **plain character affect**, this is expressed **purely in data** via
`<affects>` — no handler needed (unlike `kPoisoned`, whose handler exists only for the
poison subtype and skill roll).

```xml
<obj_affect id="kPlagueBlade" msg_case="kIns" see_affect="kDetectMagic">
    <actions>
        <action target="kTarActor">
            <trigger val="kWeaponHit" prob="15"/>
            <affects saving="kCritical" resist="kDark">
                <duration base="4" skill_divisor="20" min="3" max="8"/>
                <affect id="kPlagueTouch"/>
            </affects>
        </action>
    </actions>
</obj_affect>
```

* `kWeaponHit` fires when the tainted weapon lands a blow; `prob="15"` = 15% of hits.
  A `saving=` is mandatory (else the action is dropped).
* `target="kTarActor"` = the struck character.
* `msg_case="kIns"` — the item name declines in the instrumental case in the affect's
  messages.

### 4.3 The character affect — the "touch" (`affects.xml`)

`kPlagueTouch` is **curable and dispellable** — a weak DoT plus Str/Int penalties; on
natural expiry it infects the victim **and three allies** with `kSwampPlague`.

```xml
<affect id="kPlagueTouch" buff="N">
    <flags val="kAfSameTime|kAfDispellable|kAfCurable"/>
    <apply location="kStr"><modifier min="2" beta="0" factor="-1"/></apply>
    <apply location="kInt"><modifier min="2" beta="0" factor="-1"/></apply>
    <actions>
        <!-- (a) WHILE ACTIVE: a weak DoT each tick. -->
        <action target="kTarFightSelf">
            <trigger val="kPulse"/>
            <damage saving="kNone"><amount min="0" beta="0.4" weight="0"/></damage>
        </action>
        <!-- (b) ON EXPIRY: the victim itself -> kSwampPlague. -->
        <action target="kTarFightSelf">
            <trigger val="kExpired"/>
            <affects saving="kNone">
                <duration base="6" skill_divisor="0" min="6" max="6"/>
                <affect id="kSwampPlague"/>
            </affects>
        </action>
        <!-- (c) ON EXPIRY: 3 random allies in the room -> kSwampPlague.
             No kDispell action -> curing/dispelling early is safe. -->
        <action target="kTarGroup">
            <trigger val="kExpired"/>
            <area>
                <targets min="3" max="3"/>
                <distribution type="kUniform"/>
            </area>
            <affects saving="kNone">
                <duration base="6" skill_divisor="0" min="6" max="6"/>
                <affect id="kSwampPlague"/>
            </affects>
        </action>
    </actions>
</affect>
```

* `buff="N"` = **debuff** — the cure system gates on this (`kAfCurable` alone is not
  enough). `kAfCurable` + `kAfDispellable` → removable by both cure and dispel.
* Flat −2 Str/Int (`beta="0"` → potency-independent).
* **(b) and (c)** both use `kExpired`: the victim and three of its group. The
  once-per-type dedup fires once per affect type per pass, but **all** actions bearing
  `kExpired` run, so both impositions happen.
* **No `kDispell`** → removing it early triggers nothing; the payload only fires on the
  natural-expiry path (the `kExpired`/`kDispell` contrast, as in §1.2).

### 4.4 The character affect — "swamp plague" (`affects.xml`)

`kSwampPlague` is the heavy stage: a DoT several times the "touch", plus **contagion** —
each tick a 5% chance to infect an ally and a 5% chance to infect the room itself.

```xml
<affect id="kSwampPlague" buff="N">
    <flags val="kAfSameTime|kAfCurable"/>
    <apply location="kStr"><modifier min="4" beta="0" factor="-1"/></apply>
    <apply location="kInt"><modifier min="4" beta="0" factor="-1"/></apply>
    <actions>
        <!-- (a) heavy DoT — several times the "touch" (beta 1.5 vs 0.4). -->
        <action target="kTarFightSelf">
            <trigger val="kPulse"/>
            <damage saving="kNone"><amount min="0" beta="1.5" weight="0"/></damage>
        </action>
        <!-- (b) 5% per tick: infect one random ally in the room. -->
        <action target="kTarRandomAlly">
            <trigger val="kPulse" prob="5"/>
            <affects saving="kNone">
                <duration base="6" skill_divisor="0" min="6" max="6"/>
                <affect id="kSwampPlague"/>
            </affects>
        </action>
        <!-- (c) 5% per tick: settle a miasma into the ROOM (room affect, §4.5). -->
        <action target="kTarRoomThis">
            <trigger val="kPulse" prob="5"/>
            <affects saving="kNone">
                <duration base="8" skill_divisor="0" min="8" max="8"/>
                <affect id="kPlagueMiasma"/>
            </affects>
        </action>
    </actions>
</affect>
```

* `kAfCurable` **but not** `kAfDispellable` — the full plague is **cured, not
  dispelled** (like `kBlightCurse` in §1.3). Its DoT (`beta="1.5"`) bites nearly four
  times harder than the touch.
* **(b)** `target="kTarRandomAlly"` + `prob="5"` — each tick, a 5% chance to jump to
  one friendly in the room: **character → character** contagion.
* **(c)** `target="kTarRoomThis"` + `prob="5"` — a 5% chance to settle `kPlagueMiasma`
  into the room: **character → room** contagion. `<affect id>` resolves as both
  `EAffect` and `ERoomAffect`, so the same grammar block imposes a room affect when the
  target is a room.

### 4.5 The room affect — "miasma" (`room_affects.xml`)

`kPlagueMiasma` infects **entering** players with `kPlagueTouch` — so the plague
spreads through the location even with no carrier present.

```xml
<room_affect id="kPlagueMiasma">
    <flags val="kAfDispellable"/>
    <actions>
        <action target="kTarActor">
            <trigger val="kEnterPC"/>
            <target_conditions>
                <blocking><affect_flags val="kPlagueTouch"/></blocking>
            </target_conditions>
            <affects saving="kCritical" resist="kDark">
                <duration base="4" skill_divisor="0" min="4" max="4"/>
                <affect id="kPlagueTouch"/>
            </affects>
        </action>
    </actions>
</room_affect>
```

* `kEnterPC` — fires on a player entering (`kTarActor`). We do **not** add `return="0"`
  — entry is not forbidden, only infected.
* `<target_conditions><blocking>` on `kPlagueTouch` — don't re-infect the already-sick.
* The payload is `kPlagueTouch` again with a save: the loop closes.

### 4.6 Messages

Each new id needs its own messages (or it renders silent):

* `cfg/messages/ru/obj_affect_msg.xml` — sheaf `kPlagueBlade`: `kShortDesc` (a plague
  film), `kDiagToChar` (the examine line).
* `cfg/messages/ru/affect_msg.xml` — sheaves `kPlagueTouch` / `kSwampPlague`:
  impose/expire + `kDamageToChar`/`kDamageToRoom` for the DoT (else the tick is silent).
* `cfg/messages/ru/room_affect_msg.xml` — sheaf `kPlagueMiasma`: the room's miasma
  description.

### 4.7 How it plays

1. A mage casts `kInfectBlade` on a sword → the sword carries `kPlagueBlade` for 24h.
2. Each hit: 15% → the victim gets `kPlagueTouch` (3–8h).
3. `kPlagueTouch`: a weak DoT + −2 Str/Int. Dispel or cure it and you're safe.
4. If ignored → the timer runs out → the victim **and 3 allies** get `kSwampPlague`
   for 6h.
5. `kSwampPlague`: a heavy DoT; each tick 5% → the plague jumps to a random ally
   (**char→char**) and 5% → settles into the room as a miasma (**char→room**).
6. `kPlagueMiasma`: anyone entering the room catches `kPlagueTouch` (**room→char**) —
   and the cycle repeats.

All three kinds are in play: **item** (`kPlagueBlade`), **character** (`kPlagueTouch`,
`kSwampPlague`), and **room** (`kPlagueMiasma`).

---

## 5. Feats reworking the Plague Blade — `feats.xml`

Just as §2 reshapes the Hex of Decay, these feats edit the plague chain from §4 via
`talent_patch` (see the [Feat Manual](FEAT_MANUAL.md) §4): `modify`/`append`, a spell
patch vs. an affect patch, and `relative` gating.

```xml
<!-- 1) modify a numeric EFFECT field: swamp plague bites harder (DoT x1.5). -->
<feat id="kPestilence" mode="kEnabled">
    <talent_patches>
        <talent_patch affect="kSwampPlague" op="modify" effect="kDamage">
            <modify field="beta" mul="1.5"/>
        </talent_patch>
    </talent_patches>
</feat>

<!-- 2) modify the EFFECT area: on expiry the "touch" infects +2 allies (floor 3 -> 5). -->
<feat id="kVirulentStrain" mode="kEnabled">
    <talent_patches>
        <talent_patch affect="kPlagueTouch" op="modify" effect="kArea">
            <modify field="min_targets" add="2"/>
        </talent_patch>
    </talent_patches>
</feat>

<!-- 3) append to the EFFECT: the plague leaps to enemies too (5% per tick). -->
<feat id="kUnbridledPlague" mode="kEnabled">
    <talent_patches>
        <talent_patch affect="kSwampPlague" op="append">
            <action target="kTarRandomFoe">
                <trigger val="kPulse" prob="5"/>
                <affects saving="kCritical" resist="kDark">
                    <duration base="6" skill_divisor="0" min="6" max="6"/>
                    <affect id="kSwampPlague"/>
                </affects>
            </action>
        </talent_patch>
    </talent_patches>
</feat>

<!-- 4) append to the SPELL: tainting the blade also empowers the plaguecrafter. The
     spell targets an OBJECT (kTarObjInv), but the appended action has its own target
     (kTarFightSelf) -> it hits the caster. This patches the SPELL, not the affect. -->
<feat id="kPlaguecrafter" mode="kEnabled">
    <talent_patches>
        <talent_patch spell="kInfectBlade" op="append">
            <action target="kTarFightSelf">
                <affects>
                    <duration base="6" skill_divisor="0" min="6" max="6"/>
                    <affect id="kHaste"/>
                </affects>
            </action>
        </talent_patch>
    </talent_patches>
</feat>

<!-- 5) relative gating: a caring group LEADER softens the plague on the led. If a sick
     ally is led by someone with this feat, their kSwampPlague DoT is halved -- gated on
     the LEADER's feat, not the sick ally's. -->
<feat id="kPlagueWard" mode="kEnabled">
    <talent_patches>
        <talent_patch affect="kSwampPlague" relative="group_leader" op="modify" effect="kDamage">
            <modify field="beta" mul="0.5"/>
        </talent_patch>
    </talent_patches>
</feat>
```

What each shows:

| feat | target | op | mechanic |
|---|---|---|---|
| `kPestilence` | affect `kSwampPlague` | `modify` `kDamage` | scale a manifestation's numeric field (`beta`) |
| `kVirulentStrain` | affect `kPlagueTouch` | `modify` `kArea` | scale the area target count (`min_targets`) |
| `kUnbridledPlague` | affect `kSwampPlague` | `append` | add an action (infect enemies) to a trigger chain |
| `kPlaguecrafter` | **spell** `kInfectBlade` | `append` | patch the *spell*; the action's own `kTarFightSelf` target |
| `kPlagueWard` | affect `kSwampPlague` | `modify` + `relative="group_leader"` | gate the patch on a *relative's* feat |

All are gated by `CanUseFeat(holder, feat)` and cost nothing on cast/tick for anyone
without them. The feats are pure data (`feats.xml`) layered over the §4 ids; they need
no new code.

