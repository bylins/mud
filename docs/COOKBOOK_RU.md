# Bylins MUD — Сборник примеров (cookbook)

*Полные, перекрёстно связанные примеры для авторов `spells.xml`, `affects.xml`,
`room_affects.xml` и `feats.xml`. Сначала прочтите три справочных руководства —
[Заклинания](SPELL_MANUAL_RU.md) · [Эффекты](AFFECT_MANUAL_RU.md) ·
[Способности](FEAT_MANUAL_RU.md) — затем используйте это как шаблоны.*

> **Ссылки на разделы.** Голые номера `§N` указывают на **Руководство по
> заклинаниям**, если ссылка не говорит иначе (напр. «[Руководство по
> эффектам](AFFECT_MANUAL_RU.md) §5»).

---

## 1. Проклятие через все три системы — «Порча увядания»

Один сценарий, затрагивающий сразу все системы. Он показывает:

* заклинание, которое лишь **накладывает** эффект (всё поведение — на эффекте);
* эффект с **несколькими триггерами** — DoT на каждый тик *и* мерзкую нагрузку
  **при истечении**;
* различие **`kExpired` против `kDispell`** — порчу *безопасно развеивать*, но если
  дать ей истечь, она бьёт по союзникам носителя;
* несколько **способностей**, перекраивающих заклинание и оба эффекта через
  `talent_patch`.

### 1.1 Заклинание — `spells.xml`

`kHexOfDecay` — тёмное одноцелевое проклятие, единственная задача которого —
наложить эффект.

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

Всё, что проклятие *делает*, — на `kDecayHex`; заклинание лишь доставляет его (§8)
со спасброском Воли и масштабируемой по навыку длительностью.

### 1.2 Эффект — `affects.xml`, **несколько триггеров**

`kDecayHex` **развеиваемый и излечимый** (безопасно снять) и несёт два триггерных
действия — DoT пока активен и нагрузку при естественном истечении.

```xml
<affect id="kDecayHex" buff="N">
    <flags val="kAfDispellable|kAfCurable"/>
    <actions>
        <!-- (a) ПОКА АКТИВЕН: слабый грызущий DoT каждый тик. -->
        <action target="kTarFightSelf">
            <trigger val="kPulse"/>
            <damage saving="kNone">
                <amount min="0" dices_weight="0.4" alpha="0" beta="0"/>
            </damage>
        </action>

        <!-- (b) ПРИ ЕСТЕСТВЕННОМ ИСТЕЧЕНИИ: нагрузка. Заразить НЕСКОЛЬКИХ союзников
             носителя. Действия kDispell намеренно НЕТ -> развеивать порчу
             безопасно; это срабатывает только по пути kExpired. -->
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

* **`kPulse` (a)** грызёт носителя каждый тик — `dices_weight="0.4"` от сохранённого
  potency ([Руководство по эффектам](AFFECT_MANUAL_RU.md) §4.2). Дайте ему
  сообщение `kDamageToChar/Room`, чтобы он не молчал.
* **`kExpired` (b)** — подвох. При естественном истечении бьёт по `kTarGroup`
  (соратники носителя), а `<area>` ограничивает удар **тремя-пятью** из них
  (`min=3` + небольшой бонус от навыка, `max=2`). Каждый ловит `kBlightCurse`
  (§1.3) на 4 часа.
* **Развеивание без последствий.** Поскольку действия **`kDispell` нет**, раннее
  снятие порчи (развеивание/лечение) ничего не запускает — нагрузка срабатывает
  только на *естественном* пути `kExpired`. Этот контраст `kExpired`-против-`kDispell`
  и есть весь замысел: умная цель развеивает порчу; проигнорированная порча
  наказывает группу. *(Нужен один случайный союзник вместо группы? целитесь
  `kTarRandomAlly` и уберите `<area>`.)*

### 1.3 Эффект-нагрузка — `affects.xml`

`kBlightCurse` — «гораздо более мерзкий» дебафф, который ловят союзники: плоские
штрафы к статам плюс более сильный DoT.

```xml
<affect id="kBlightCurse" buff="N">
    <flags val="kAfSameTime|kAfCurable"/>
    <apply location="kStr"><modifier min="3.0" dices_weight="0" alpha="0" beta="0" factor="-1"/></apply>
    <apply location="kDex"><modifier min="3.0" dices_weight="0" alpha="0" beta="0" factor="-1"/></apply>
    <apply location="kHitroll"><modifier min="5.0" dices_weight="0" alpha="0" beta="0" factor="-1"/></apply>
    <actions>
        <action target="kTarFightSelf">
            <trigger val="kPulse"/>
            <damage saving="kNone"><amount min="0" dices_weight="0.9" alpha="0" beta="0"/></damage>
        </action>
    </actions>
</affect>
```

`kAfCurable`, **но не** `kAfDispellable` — заразу *лечат*, а не развеивают — а его
DoT (`0.9`) кусает более чем вдвое сильнее порчи (`0.4`). Плоские `<apply>` не
требуют компетентности кастера, поэтому они полной силы, хотя накладывает их
*эффект* (действие при истечении), а не живой каст.

### 1.4 Как это играется

1. Кастер накладывает `kHexOfDecay` → жертва получает `kDecayHex` (спасбросок Воли, ~5–15 ч).
2. Каждый тик `kDecayHex` слегка грызёт жертву (`kPulse`).
3. Жертва (или союзник) может **развеять/вылечить** `kDecayHex` в любой момент —
   ничего плохого (действия `kDispell` нет).
4. Если никто не снял — таймер истекает → срабатывает `kExpired` → 3–5 соратников
   жертвы ловят `kBlightCurse` на 4 ч (сильный удар по статам + жёсткий DoT).

---

## 2. Способности, перекраивающие его — `feats.xml`

Пять способностей: `modify`/`append`, патч заклинания против патча эффекта и
гейтинг `relative` (см. [Руководство по способностям](FEAT_MANUAL_RU.md) §4).

```xml
<!-- 1) modify поля ЭФФЕКТА: порча кастера грызёт сильнее (DoT x1.6). -->
<feat id="kHexweaver" mode="kEnabled">
    <talent_patches>
        <talent_patch affect="kDecayHex" op="modify" effect="kDamage">
            <modify field="dices_weight" mul="1.6"/>
        </talent_patch>
    </talent_patches>
</feat>

<!-- 2) modify area ЭФФЕКТА: нагрузка задевает больше союзников (+2 к полу). -->
<feat id="kSpreadingBlight" mode="kEnabled">
    <talent_patches>
        <talent_patch affect="kDecayHex" op="modify" effect="kArea">
            <modify field="min_targets" add="2"/>
        </talent_patch>
    </talent_patches>
</feat>

<!-- 3) append к kExpired ЭФФЕКТА: истечение ещё и заставляет союзников замолчать. -->
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

<!-- 4) append к ЗАКЛИНАНИЮ: порча кастера ещё и наносит немного прямого тёмного урона при касте. -->
<feat id="kBlightbolt" mode="kEnabled">
    <talent_patches>
        <talent_patch spell="kHexOfDecay" op="append">
            <action target="kTarFightVict">
                <damage saving="kStability">
                    <amount min="10" dices_weight="0.5" alpha="0" beta="0"/>
                </damage>
            </action>
        </talent_patch>
    </talent_patches>
</feat>

<!-- 5) гейтинг relative: хороший ЛИДЕР группы смягчает последствия. Если заражённый
     союзник ведом кем-то с этой способностью, его DoT от kBlightCurse вдвое слабее —
     гейт по способности ЛИДЕРА, не союзника. -->
<feat id="kWardingLeader" mode="kEnabled">
    <talent_patches>
        <talent_patch affect="kBlightCurse" relative="group_leader" op="modify" effect="kDamage">
            <modify field="dices_weight" mul="0.5"/>
        </talent_patch>
    </talent_patches>
</feat>
```

Что показывает каждая:

| способность | цель | операция | механика |
|---|---|---|---|
| `kHexweaver` | эффект `kDecayHex` | `modify` `kDamage` | масштаб числового поля проявления |
| `kSpreadingBlight` | эффект `kDecayHex` | `modify` `kArea` | масштаб числа целей area (`min_targets`) |
| `kCruelHex` | эффект `kDecayHex` | `append` | добавить целое действие в цепочку триггера |
| `kBlightbolt` | **заклинание** `kHexOfDecay` | `append` | патч *заклинания*, не эффекта |
| `kWardingLeader` | эффект `kBlightCurse` | `modify` + `relative="group_leader"` | гейт патча по способности *родственника* |

Все под гейтом `CanUseFeat(holder, feat)` и ничего не стоят при касте тем, у кого
их нет.

---

## 3. Галерея рецептов

Более короткие узконаправленные примеры (перенесены из Руководства по заклинаниям).


### 3.1 Простое заклинание урона с дополнительными ударами

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

* Спасбросок `kReflex`, успех вдвое уменьшает урон.
* Количество = `кубики · 1.0 + компетенции · 25`.
* До 2 дополнительных ударов, срабатывающих каждый раз (`prob=100`), масштабируемых `min(kFireMagic, 75) / 12`.

### 3.2 Дебаф со спасброском, перепозиционированием и задержкой

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
<!-- affects.xml: kSleep — чистый статусный флаг (без числового apply) -->
<affect id="kSleep" buff="N">
    <flags val="kAfBattledec|kAfDispellable|kAfCurable"/>
</affect>
```

* NPC с флагом `kNoSleep` или цели с эффектом `kHold` невосприимчивы.
* Успешный спасбросок `kWill` предотвращает сон.
* При успешном наложении жертва переводится в позицию `kSleep`.
* Длительность: `1 + min(kMindMagic, 75) / 15` часов, ограничение `[1..6]`.

### 3.3 Чистое лечение (с проверкой силы)

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

* `affect_flags="kAfCurable"` — не трогает только `kAfDispellable` эффекты.
* `<remove all_of="…">` — пробует снять каждый перечисленный яд, если он есть.
* Каждое снятие — проверка d100 по навыку (§9.3). Лечения ставят `dispel_bonus="80"` (≈80 % при равном навыке); пол/потолок 5 %.
* `<alter_obj handler="AlterRemovePoison"/>` — то же заклинание на отравленной
  еде/питье очищает предмет (§3.8.7); обе ветви живут в одном действии.

### 3.4 Раскрытие с разрывом цепочки

```xml
<spell id="kGlitterDust" element="kEarth" mode="kEnabled">
    <name rus="блестящая пыль" eng="glitterdust"/>
    …
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
<!-- affects.xml: эффект владеет своими флагами и штрафом к спасброску -->
<affect id="kGlitterDust" buff="N">
    <flags val="kAfDispellable|kAfCurable"/>
    <apply location="kSavingReflex">
        <modifier min="0.0" dices_weight="0.0" alpha="0" beta="4.4" factor="1"/>
    </apply>
</affect>
```

* `<unaffect>` выполняется первым: пытается снять `kInvisible|kCamouflage|kHide`.
* Если состязание сил **проваливается** для любого эффекта на данной цели,
  `breaking_by_failure="true"` прерывает цепочку — дебаф не накладывается.
* Если снятие успешно (или эффекты отсутствовали), дебаф применяется нормально.
* Штраф к спасброску масштабируется с компетентностью кастера.

### 3.5 Заклинание с ограничением по мировоззрению

```xml
<spell id="kDispelEvil" element="kLight" mode="kEnabled">
    <name rus="рассеять зло" eng="dispel evil"/>
    …
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
                <amount min="0" dices_weight="1.0" alpha="0.5" beta="61"/>
            </damage>
        </action>
    </talent_actions>
</spell>
```

* `<caster_conditions><blocking><align val="kEvil"/></blocking></caster_conditions>` —
  *кастер* не должен быть злым (на уровне заклинания, проверяется один раз до целей).
* `<target_conditions><required><align val="kEvil"/></required></target_conditions>` —
  *цель* должна быть злой, иначе `kNoeffect`.
* `<target_conditions><blocking><room_flags val="kNoMagic"/></blocking></target_conditions>` —
  универсальный срыв в комнате с NOMAGIC.

### 3.6 Шаблон отражения

```xml
<spell id="kIceBolt" element="kWater" mode="kEnabled">
    <!-- … обычные поля … -->
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

* Цели с `kFireShield` или `kFireAura` имеют **35 %** шанс отразить заклинание.
* При срабатывании отражения кастер получает урон сам. Выдаются три сообщения
  (`kReflectedToChar`/`Vict`/`Room`).

### 3.7 Стакирующийся яд (гипотетический)

```xml
<spell id="kPoison" element="kDark" mode="kEnabled">
    <!-- … обычные поля … -->
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
<!-- affects.xml: эффект владеет стакающимися apply (штраф к силе + тик яда) -->
<affect id="kPoison" buff="N">
    <flags val="kAfSameTime|kAfAccumulateDuration|kAfCurable"/>
    <apply location="kStr">
        <modifier min="2.0" dices_weight="0.0" alpha="0" beta="0.0" factor="-1" stack="3"/>
    </apply>
    <apply location="kPoison">
        <modifier min="0.0" dices_weight="0.0" alpha="0" beta="11.5" factor="1" stack="3"/>
    </apply>
</affect>
```

*(Реальный DoT яда брал бы урон из действия `kPulse` `<damage>` — см.
[Руководство по эффектам](AFFECT_MANUAL_RU.md) §5; форма на apply иллюстрирует
стакирование.)*

* Повторное применение на той же жертве **стакируется до 3 раз**.
* `kAfCurable` без `kAfDispellable` — работает `kRemovePoison`, но не `kDispellMagic`.
* `kRemovePoison` **снимает один стак** — нужно три лечения для полного снятия тройного яда.

---

### 3.8 Мультидейственное заклинание: `kSacrifice` *(пер-действенные цели + cast-chain)*

`kSacrifice` («высосать жизнь») — эталонное мультидейственное заклинание: оно
наносит урон врагу, затем лечит кастера на величину нанесённого урона, затем лечит
**нежить**-миньонов кастера на ту же величину. Три блока `<action>`, у каждого своя
цель.

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
        <!-- A1 — урон по ОДНОМУ врагу (входное действие; цель уровня заклинания). -->
        <action>
            <damage saving="kWill">
                <amount min="0" dices_weight="1.0" alpha="0.5" beta="62"/>
            </damage>
            <target_conditions>
                <blocking><affect_flags val="kCharmed"/></blocking>   <!-- никогда по своему питомцу -->
            </target_conditions>
        </action>
        <!-- A2 — лечит КАСТЕРА на только что нанесённый урон (base=kDamage). -->
        <action target="kTarFightSelf" base="kDamage">
            <points extra="300">
                <heal min="0" dices_weight="0" alpha="0" beta="1.0" npc_coeff="0"/>
            </points>
        </action>
        <!-- A3 — лечит НЕЖИТЬ-миньонов кастера на ту же величину. -->
        <action target="kTarMinions" base="kDamage">
            <points extra="300">
                <heal min="0" dices_weight="0" alpha="0" beta="1.0" npc_coeff="0"/>
            </points>
            <area>
                <targets max="-1"/>             <!-- все миньоны, без масштабирования -->
                <distribution type="kUniform"/>
            </area>
            <target_conditions>
                <required><mob_flags val="kCorpse"/></required>   <!-- только нежить -->
            </target_conditions>
        </action>
    </talent_actions>
</spell>
```

Что делает каждая часть:

* **A1 (входное действие)** использует собственную область заклинания
  (`<targets>`/`kMagDamage`) — один выбранный враг — и наносит урон со спасброском
  `kWill`. `<target_conditions><blocking>` по `kCharmed` отказывается высасывать
  собственного питомца. Как входное действие, оно также несёт шлюзы заклинания и
  реакцию жертвы (отложенную до конца).
* **A2 (`target="kTarFightSelf"`, `base="kDamage"`)** перенацеливается на кастера и
  **лечит**. `base="kDamage"` заменяет скаляр компетентности на *фактически снятые
  A1 HP*; при `dices_weight="0"` и `beta="1.0"` лечение равно урону (чистый перенос
  1:1). `<points extra="300">` позволяет перелечить до 4× от максимума.
* **A3 (`target="kTarMinions"`, `base="kDamage"`)** повторяет это лечение на
  очарованных NPC-последователях кастера, но `<required><mob_flags val="kCorpse"/>`
  ограничивает его **нежитью**, а `<area><targets max="-1"/>` раздаёт лечение
  **всем** им равномерно. Заметьте: `<targets max="-1"/>` намеренно опускает
  `skill_divisor`/`dice_size`/`min`/`stat_weight` — они берутся по умолчанию (§7).

Что иллюстрирует пример:

* **Пер-действенные цели** — A1 бьёт врага, A2 кастера, A3 миньонов; заклинание
  перенацеливается по ходу применения (§3.8.1).
* **Cast-chain** — A2 и A3 читают накопленный **урон** вместо повторного броска
  компетентности (§3.8.2). Аккумулятор фиксирует *фактически снятые HP* (с уже
  учтённым оверкиллом и сопротивлениями), поэтому лечение не может превысить урон.
* **Пер-действенная область** — охватывает несколько целей только A3; A1/A2
  одноцелевые.
* **Одна реакция** — враг отвечает один раз, после A3, а не между действиями.

> Одноцелевой вариант «нанести урон, затем вылечиться на него» требует только
> A1 + A2; A3 — бонус для нежити-миньонов. *Массовый* вариант вместо этого заставил
> бы охватывать несколько целей само A1 (`kMagMasses` + `<area>` на A1).

---

