# Bylins MUD — Сборник примеров (cookbook)

*Полные, перекрёстно связанные примеры для авторов `spells.xml`, `affects.xml`,
`obj_affects.xml`, `room_affects.xml` и `feats.xml`. Сначала прочтите три справочных
руководства — [Заклинания](SPELL_MANUAL_RU.md) · [Эффекты](AFFECT_MANUAL_RU.md) ·
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

> **Новые id.** Заклинание `kHexOfDecay`, эффекты `kDecayHex`/`kBlightCurse` и пять
> способностей — всё новое. Любой id — это C++-перечисление, поэтому каждому нужно
> значение перечисления и строка в карте имён, иначе данные не разрешатся (см.
> таблицу в §4; способности идут в `EFeat` — `feats_constants.h` +
> `feats_constants.cpp`). Заклинание можно вместо этого обкатать в свободном тестовом
> слоте (`kTestTwo` … `kTestFive`, Руководство по заклинаниям §17).

### 1.1 Заклинание — `spells.xml`

`kHexOfDecay` — тёмное одноцелевое проклятие, единственная задача которого —
наложить эффект.

```xml
<spell id="kHexOfDecay" element="kDark" mode="kEnabled">
    <name val="hex of decay"/>
    <components><verbal/><weave/></components>
    <misc pos="kFight" violent="Y" danger="1"/>
    <mana max="60" min="45" change="1"/>
    <targets val="kTarCharRoom|kTarFightVict"/>
    <flags val="kMagAffects|kNpcAffectPc"/>
    <potency_roll>
        <base_skill id="kDarkMagic" low_skill_bonus="1" hi_skill_bonus="7"/>
        <base_stat id="kInt" threshold="22" weight="18"/>
    </potency_roll>
    <success_roll>
        <base_skill id="kDarkMagic" low_skill_bonus="3" hi_skill_bonus="1.25"/>
        <base_stat id="kInt" threshold="22" weight="0.5"/>
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
со спасброском Воли и масштабируемой по навыку длительностью. `<name val>` — только
английская админская метка: русское имя для игры и сообщения каста живут в
`cfg/messages/ru/spell_msg.xml` (sheaf `kHexOfDecay`, `<name val="порча увядания"/>`).

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
                <amount min="0" beta="0.4"/>
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

* **`kPulse` (a)** грызёт носителя каждый тик — `beta="0.4"` от сохранённого
  potency ([Руководство по эффектам](AFFECT_MANUAL_RU.md) §4.2). Дайте ему
  сообщение `kDamageToChar/Room`, чтобы он не молчал.
* **`kExpired` (b)** — подвох. При естественном истечении бьёт по `kTarGroup`
  (соратники носителя), а `<area>` задаёт, по скольким: `min=3` плюс бонус от
  навыка, ограниченный `max=2`. Бонус читает навык *кастующего заклинания*, а у
  действия, запущенного эффектом, его нет (оно идёт без заклинания), поэтому на деле
  задевает ровно **трёх** соратников. Каждый ловит `kBlightCurse` (§1.3) на 4 часа.
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

`kAfCurable`, **но не** `kAfDispellable` — заразу *лечат*, а не развеивают — а его
DoT (`0.9`) кусает более чем вдвое сильнее порчи (`0.4`). Плоские `<apply>` не
требуют компетентности кастера, поэтому они полной силы, хотя накладывает их
*эффект* (действие при истечении), а не живой каст.

### 1.4 Как это играется

1. Кастер накладывает `kHexOfDecay` → жертва получает `kDecayHex` (спасбросок Воли,
   6–10 ч до учёта сопротивлений).
2. Каждый тик `kDecayHex` слегка грызёт жертву (`kPulse`).
3. Жертва (или союзник) может **развеять/вылечить** `kDecayHex` в любой момент —
   ничего плохого (действия `kDispell` нет).
4. Если никто не снял — таймер истекает → срабатывает `kExpired` → трое соратников
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
            <modify field="beta" mul="1.6"/>
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
                    <amount min="10" beta="0.5"/>
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
            <modify field="beta" mul="0.5"/>
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
Примеры с настоящим заклинанием повторяют его текущее определение из
`lib/cfg/spells.xml` и `lib/cfg/affects/affects.xml`; гипотетические помечены.

> **Два файла.** Примеры, накладывающие эффект, показывают *и* блок заклинания
> (который называет эффект через `<affect id="…"/>` и владеет длительностью и
> спасброском), **и** блок эффекта в `affects.xml` (который владеет флагами, apply и
> действиями). См. §8 и [Руководство по эффектам](AFFECT_MANUAL_RU.md).

### 3.1 Простое заклинание урона с дополнительными ударами

```xml
<spell id="kMagicMissile" element="kAir" mode="kEnabled">
    <name val="magic missle"/>
    <components><verbal/><weave/></components>
    <misc pos="kFight" violent="Y" danger="1"/>
    <mana max="40" min="30" change="1"/>
    <targets val="kTarCharRoom|kTarFightVict"/>
    <flags val="kMagDamage|kNpcDamagePc"/>
    <potency_roll>
        <noise sigma="0.25"/>
        <base_skill id="kAirMagic" low_skill_bonus="1" hi_skill_bonus="7"/>
        <base_stat id="kWis" threshold="22" weight="18"/>
    </potency_roll>
    <success_roll>
        <base_skill id="kAirMagic" low_skill_bonus="3" hi_skill_bonus="0.75"/>
        <base_stat id="kWis" threshold="22" weight="5"/>
    </success_roll>
    <talent_actions>
        <action id="Main">
            <damage id="Bolt" saving="kReflex">
                <amount min="0" beta="15.3468" weight="1"/>
                <hits skill_divisor="12" max="2" prob="100"/>
            </damage>
        </action>
    </talent_actions>
</spell>
```

* Спасбросок `kReflex`, успех вдвое уменьшает урон.
* Количество = `min + beta·C·(1 + weight·d)` = `15.3468·C`, где `C` —
  компетентность кастера из `<potency_roll>` (навык + стат, §3.7), а `d` — один общий
  на каст бросок шума (`<noise sigma="0.25"/>`); `weight="1"` берёт полный разброс,
  `weight="0"` сделал бы количество детерминированным.
* До 2 дополнительных ударов, срабатывающих каждый каст (`prob=100`); число
  дополнительных ударов — `min(kAirMagic, 75) / 12` (`base_skill` из броска силы),
  но не больше `max=2`.
* `id="Main"` / `id="Bolt"` — устойчивые id блоков, к которым может обратиться патч
  способности (`feats.xml` заменяет именно этот урон через `action="Main"
  effect="Bolt"`; см. [Руководство по способностям](FEAT_MANUAL_RU.md) §4).

### 3.2 Дебаф со спасброском, перепозиционированием и задержкой

```xml
<spell id="kSleep" element="kMind" mode="kEnabled">
    <name val="sleep"/>
    <components><verbal/><weave/></components>
    <misc pos="kFight" violent="Y" danger="0"/>
    <mana max="70" min="55" change="1"/>
    <targets val="kTarCharRoom|kTarFightVict"/>
    <flags val="kMagAffects|kNpcAffectPc"/>
    <potency_roll>
        <base_skill id="kMindMagic" low_skill_bonus="1" hi_skill_bonus="7"/>
        <base_stat id="kInt" threshold="22" weight="18"/>
    </potency_roll>
    <success_roll>
        <base_skill id="kMindMagic" low_skill_bonus="3" hi_skill_bonus="1.25"/>
        <base_stat id="kInt" threshold="22" weight="0.5"/>
    </success_roll>
    <talent_actions>
        <action>
            <target_conditions>
                <blocking>
                    <mob_flags val="kNoSleep"/>
                    <affect_flags val="kHold"/>
                </blocking>
            </target_conditions>
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

* NPC с флагом `kNoSleep` или цели с эффектом `kHold` невосприимчивы (шлюз
  `<target_conditions>` срабатывает до любых бросков). Голый `<blocking>` прямо под
  `<action>` больше не принимается — всегда оборачивайте его (§4).
* Успешный спасбросок `kWill` предотвращает сон.
* При успешном наложении жертва переводится в позицию `kSleep`.
* Длительность: `1 + min(kMindMagic, 75) / 15` часов, ограничение `[1..6]`.
* `kAfDispellable|kAfCurable` означает, что снять эффект могут пытаться и
  `kDispellMagic`, и лечения вроде `kRemovePoison` (через состязание сил).
* Задержки (`<lag>`) у этого заклинания нет; боевая задержка была бы ещё одним
  потомком того же блока `<affects>` (`<lag base="…" bonus_divisor="…"/>`, §8.3).

### 3.3 Чистое лечение (с проверкой силы)

```xml
<spell id="kRemovePoison" element="kLife" mode="kEnabled">
    <name val="remove poison"/>
    <components><verbal/><weave/></components>
    <misc pos="kFight" violent="N" danger="0"/>
    <mana max="60" min="45" change="2"/>
    <targets val="kTarCharRoom|kTarFightSelf|kTarObjInv|kTarObjRoom"/>
    <flags val="kMagUnaffects|kNpcUnaffectNpc"/>
    <potency_roll>
        <base_skill id="kLifeMagic" low_skill_bonus="1" hi_skill_bonus="7"/>
        <base_stat id="kWis" threshold="22" weight="18"/>
    </potency_roll>
    <success_roll>
        <base_skill id="kLifeMagic" low_skill_bonus="3" hi_skill_bonus="1.25"/>
        <base_stat id="kWis" threshold="22" weight="0.5"/>
    </success_roll>
    <talent_actions>
        <action>
            <unaffect dispel_bonus="80" affect_flags="kAfCurable" decay="25">
                <remove all_of="kPoisoned|kAconitumPoison|kScopolaPoison|kBelenaPoison|kDaturaPoison|kSkillReduce|kNoBattleSwitch"/>
            </unaffect>
            <alter_obj handler="AlterRemovePoison"/>
        </action>
    </talent_actions>
</spell>
```

* `affect_flags="kAfCurable"` — не трогает эффекты, у которых есть только
  `kAfDispellable` (например, благословения).
* `<remove all_of="…">` — пробует снять каждый перечисленный эффект, который есть на
  цели (снимает те, что найдёт, а не требует всех сразу). В списке семейство ядов плюс
  `kSkillReduce` и `kNoBattleSwitch`: яд наёмника — связка из нескольких типов
  эффектов, и без двух последних лечение снимало бы только строчку с ядом, а минус к
  умениям и запрет переключения оставались бы. Все они помечены `kAfPoison` в
  `affects.xml`, так что посторонних дебафов список не касается.
* `<alter_obj handler="AlterRemovePoison"/>` — то же заклинание на отравленной
  еде/питье очищает предмет (§3.8.7); обе ветви живут в одном действии.
* Каждое снятие — проверка d100 по навыку (Руководство по заклинаниям §9.3). Лечение
  ставит `dispel_bonus="80"`, поэтому жрец надёжно снимает яд равного по навыку
  отравителя; более сильный яд мастера-убийцы (выше записанная компетентность)
  снижает порог — сильно уступающий жрец скатывается к полу 5 %, превосходящий
  упирается в потолок 95 %.
* `decay="25"` — при **неудачном** снятии оставшийся яд ослабевает на 25 % от вклада
  компетентности лекаря, так что повторные касты постепенно его подтачивают (§9.1).

### 3.4 Раскрытие с разрывом цепочки

```xml
<spell id="kGlitterDust" element="kEarth" mode="kEnabled">
    <name val="glitterdust"/>
    <components><weave/></components>
    <misc pos="kFight" violent="Y" danger="5"/>
    <mana max="120" min="100" change="3"/>
    <targets val="kTarIgnore"/>
    <flags val="kMagMasses|kMagAffects|kMagUnaffects|kNpcAffectPc"/>
    <potency_roll>
        <base_skill id="kEarthMagic" low_skill_bonus="1" hi_skill_bonus="7"/>
        <base_stat id="kWis" threshold="22" weight="18"/>
    </potency_roll>
    <success_roll>
        <base_skill id="kEarthMagic" low_skill_bonus="3" hi_skill_bonus="1.25"/>
        <base_stat id="kWis" threshold="22" weight="0.5"/>
    </success_roll>
    <talent_actions>
        <action>
            <area>
                <targets skill_divisor="15" dice_size="3" min="5" max="20"/>
                <distribution type="kStepped" decay="0.05" free_targets="5"/>
            </area>
            <unaffect decay="25">
                <remove all_of="kInvisible|kDisguise|kHide" breaking_by_failure="true"/>
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
<!-- affects.xml: эффект владеет своим штрафом к спасброску -->
<affect id="kGlitterDust" buff="N">
    <apply location="kSavingReflex">
        <modifier min="0" beta="2.0254" factor="1"/>
    </apply>
</affect>
```

* `<unaffect>` выполняется первым (порядок цепочки): пытается снять
  `kInvisible|kDisguise|kHide`. Без `affect_flags` берётся значение по умолчанию
  `kAfCurable|kAfDispellable`.
* Если состязание сил **проваливается** для любого из них на данной цели,
  `breaking_by_failure="true"` прерывает цепочку каста **для этой цели** — дебаф
  блестящей пыли не накладывается. `decay="25"` всё равно ослабляет уцелевший эффект,
  так что следующий каст имеет больше шансов.
* Если снятие успешно (или эффектов не было), дебаф применяется нормально — штраф к
  спасброску на 4 часа.
* У эффекта нет `<flags>`: он не развеивается и не лечится, а просто держится свои
  4 часа (убывая раз в игровой час).
* Штраф масштабируется с компетентностью кастера: при навыке 140 / мудрости 34
  (типичный кастер R=12) компетентность `C = (75·1 + 65·7)/100 + (34−22)·18/100 = 7.46`,
  и модификатор спасброска равен `+ceil(7.46·2.0254) = +16`; необученный кастер
  (навык 0, стат на пороге) получает `+ceil(0·2.0254) = 0` — правила масштабирования
  см. в [Руководстве по эффектам](AFFECT_MANUAL_RU.md) §4.2.1. (Это значение в полную
  силу; цели после `free_targets="5"` получают его уменьшенным ступенчатым спадом, §7.)

### 3.5 Заклинание с ограничением по мировоззрению

```xml
<spell id="kDispelEvil" element="kLight" mode="kEnabled">
    <name val="dispel evil"/>
    <components><verbal/><weave/></components>
    <misc pos="kFight" violent="N" danger="1"/>
    <mana max="100" min="90" change="1"/>
    <targets val="kTarCharRoom|kTarFightVict"/>
    <flags val="kMagDamage"/>
    <potency_roll>
        <noise sigma="0.15"/>
        <base_skill id="kLightMagic" low_skill_bonus="1" hi_skill_bonus="7"/>
        <base_stat id="kWis" threshold="22" weight="18"/>
    </potency_roll>
    <success_roll>
        <base_skill id="kLightMagic" low_skill_bonus="2" hi_skill_bonus="1.5"/>
        <base_stat id="kWis" threshold="22" weight="0.5"/>
    </success_roll>
    <caster_conditions>
        <blocking>
            <align val="kEvil"/>
        </blocking>
    </caster_conditions>
    <talent_actions>
        <action>
            <damage saving="kStability">
                <amount min="0" beta="63.6246" weight="1"/>
            </damage>
            <target_conditions>
                <required>
                    <align val="kEvil"/>
                </required>
            </target_conditions>
        </action>
    </talent_actions>
</spell>
```

* `<caster_conditions><blocking><align val="kEvil"/></blocking></caster_conditions>` —
  *кастер* не должен быть злым, иначе `kNoeffect` (старое поведение «гнева света»,
  теперь заданное данными). Уровень заклинания, проверяется один раз до любых целей.
* `<target_conditions><required><align val="kEvil"/></required></target_conditions>` —
  *цель* должна быть злой, иначе `kNoeffect`.
* `<components><weave/></components>` — помечает заклинание как настоящую магию: в
  комнате с `kNoMagic` оно срывается и выдаёт `kCastForbiddenToChar` /
  `kCastForbiddenToRoom` (§3.9). Это заменило поштучный
  `<blocking><room_flags val="kNoMagic"/></blocking>`, который раньше носило каждое
  заклинание, кроме боевых кличей.

### 3.6 Шаблон отражения

Сейчас ни одно заклинание в продакшене не использует `<reflection>` на стороне
заклинания (механика подключена и готова). Магическое зеркало `kMagicGlass`
пользуется другой формой — `<reflection min="…" max="…"/>` на стороне эффекта, под
триггером `kWardAttack` ([Руководство по эффектам](AFFECT_MANUAL_RU.md) §5.4). Вот как
ледяная стрела, обкатываемая в свободном тестовом слоте (Руководство по заклинаниям
§17), могла бы отскакивать от существ под огненным щитом:

```xml
<spell id="kTestTwo" element="kWater" mode="kTesting">
    <!-- … обычные поля … -->
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

* Цели с `kFireShield` или `kFireAura` с шансом **35 %** за каст отражают заклинание
  обратно в кастера (без `prob` шанс 20 %).
* При срабатывании отражения урон получает сам кастер (с учётом всех его
  собственных защит). Отскок описывают три сообщения
  (`kReflectedToChar`/`Vict`/`Room`).
* Мировоззрение: добавьте `align="kEvil"` в тот же тег, чтобы нежить врождённо
  отражала святые заклинания, и т. п.

### 3.7 Стакирующийся яд (гипотетический)

Настоящий `kPoison` сегодня не стакается — пример лишь иллюстрирует механику.
Реальное заклинание накладывает эффект `kPoisoned` со своими `<apply>` на стороне
заклинания (плоский `kStr` −2 и уровень `kPoison` с `beta="18.4125"`), а DoT несёт
сам эффект `kPoisoned`. Здесь мы переносим apply на эффект и добавляем `stack="3"`,
чтобы показать, как записывался бы стакающийся вариант.

```xml
<spell id="kPoison" element="kDark" mode="kEnabled">
    <!-- … обычные поля … -->
    <talent_actions>
        <action>
            <target_conditions>
                <blocking><affect_flags val="kGodsShield"/></blocking>
            </target_conditions>
            <affects saving="kCritical" resist="kDark">
                <duration base="0" skill_divisor="3" min="0" max="0"/>
                <affect id="kPoisoned"/>
            </affects>
        </action>
    </talent_actions>
</spell>
```

```xml
<!-- affects.xml: эффект владеет стакающимися apply (штраф к силе + уровень яда) -->
<affect id="kPoisoned" buff="N">
    <flags val="kAfSameTime|kAfAccumulateDuration|kAfCurable|kAfPoison"/>
    <apply location="kStr">
        <modifier min="2" beta="0" factor="-1" stack="3"/>
    </apply>
    <apply location="kPoison">
        <modifier min="0" beta="18.4125" factor="1" stack="3"/>
    </apply>
    <actions>
        <!-- DoT настоящего kPoisoned: каждый тик наносит суммарный уровень kPoison -->
        <action target="kTarFightSelf">
            <trigger val="kPulse"/>
            <damage source="poison" saving="kNone"/>
        </action>
    </actions>
</affect>
```

* Повторное применение на той же жертве **стакается до 3 раз**: штраф к силе растёт
  `-2`, `-4`, `-6` (плоско за стак), а уровень яда — и вместе с ним тик `kPulse`,
  который этот уровень читает, — растёт как `1×`, `2×`, `3×` от значения за каст (оно
  равно `ceil(C·18.4125)` — у типичного кастера R=12, `C ≈ 7.46`, ≈ 138 за стак).
* Длительность накапливается (`kAfAccumulateDuration`).
* `kAfCurable` без `kAfDispellable` — `kRemovePoison` работает, а простой
  `kDispellMagic` (он трогает только `kAfDispellable`) — нет.
* Снятие от `kRemovePoison` **снимает один стак**, а не весь эффект — для полного
  снятия тройного яда нужно три лечения (если каждое состязание сил успешно).

---

### 3.8 Мультидейственное заклинание: `kSacrifice` *(пер-действенные цели + cast-chain)*

`kSacrifice` («высосать жизнь») — эталонное мультидейственное заклинание: оно
наносит урон врагу, затем лечит кастера из нанесённого урона, затем лечит
**нежить**-миньонов кастера на ту же величину. Три блока `<action>`, у каждого своя
цель.

```xml
<spell id="kSacrifice" element="kDark" mode="kEnabled">
    <name val="sacrifice"/>
    <components><verbal/><weave/></components>
    <misc pos="kFight" violent="Y" danger="10"/>
    <mana max="140" min="75" change="3"/>
    <targets val="kTarCharRoom|kTarFightVict|kTarNotSelf"/>
    <flags val="kMagDamage|kNpcDamagePc|kNpcDamagePcMinhp"/>
    <potency_roll>
        <noise sigma="0.25"/>
        <base_skill id="kDarkMagic" low_skill_bonus="1" hi_skill_bonus="7"/>
        <base_stat id="kWis" threshold="32" weight="18"/>
    </potency_roll>
    <success_roll>
        <base_skill id="kDarkMagic" low_skill_bonus="2" hi_skill_bonus="2.5"/>
        <base_stat id="kWis" threshold="32" weight="4.5"/>
    </success_roll>
    <talent_actions>
        <!-- A1 — урон по ОДНОМУ врагу (входное действие; цель уровня заклинания). -->
        <action>
            <damage saving="kWill">
                <amount min="0" beta="82.5134" weight="1"/>
            </damage>
            <target_conditions>
                <blocking><affect_flags val="kCharmed"/></blocking>   <!-- никогда по своему питомцу -->
            </target_conditions>
        </action>
        <!-- A2 — лечит КАСТЕРА из только что нанесённого урона (base=kDamage). -->
        <action target="kTarFightSelf" base="kDamage">
            <points extra="300">
                <heal npc_coeff="0" min="0" beta="0.5171" weight="1"/>
            </points>
        </action>
        <!-- A3 — лечит НЕЖИТЬ-миньонов кастера на ту же величину. -->
        <action target="kTarMinions" base="kDamage">
            <points extra="300">
                <heal npc_coeff="0" min="0" beta="0.5171" weight="1"/>
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
  A1 HP*; при `beta="0.5171"` лечение составляет примерно половину урона, а
  `weight="1"` даёт общему броску шума заклинания разбросать и его. Для чистого
  переноса 1:1 ставьте `beta="1.0"` и `weight="0"`. `<points extra="300">` позволяет
  перелечить до 4× от максимума.
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
  учтённым оверкиллом и сопротивлениями), поэтому лечение — доля того, что A1
  действительно нанесло.
* **Пер-действенная область** — охватывает несколько целей только A3; A1/A2
  одноцелевые.
* **Одна реакция** — враг отвечает один раз, после A3, а не между действиями.

> Одноцелевой вариант «нанести урон, затем вылечиться на него» требует только
> A1 + A2; A3 — бонус для нежити-миньонов. *Массовый* вариант вместо этого заставил
> бы охватывать несколько целей само A1 (`kMagMasses` + `<area>` на A1).

---

## 4. Чумной клинок — все три вида эффектов (предмет, комната, персонаж)

Если §1 показывает один сценарий сквозь **заклинание / эффект / способность**, то
этот — сквозь **три подсистемы эффектов**: эффект **на предмете**
(`obj_affects.xml`), эффект **комнаты** (`room_affects.xml`) и эффект **персонажа**
(`affects.xml`). Меч временно заражают чумой; при ударе есть шанс наложить болезнь;
болезнь при истечении перерастает в тяжёлую заразу, которая расползается по жертвам
**и** оставляет в комнате миазмы, заражающие входящих.

Схема заражения:

```
заклинание -> kPlagueBlade (эффект предмета, на мече, с таймером)
   удар (15%) -> kPlagueTouch (эффект персонажа: слабый DoT, -Сила/-Ум; лечится и развеивается)
      истечение -> kSwampPlague на жертву + 3 союзников (эффект персонажа: сильный DoT)
         тик (5%) -> kSwampPlague случайному союзнику в комнате   (персонаж -> персонаж)
         тик (5%) -> kPlagueMiasma на комнату (эффект комнаты)    (персонаж -> комната)
            вход  -> kPlagueTouch входящему                       (комната -> персонаж)
```

> **Что здесь код, а что данные.** Идентификаторы эффектов и заклинаний — это
> фиксированные C++-перечисления, поэтому **новый id нельзя объявить только в XML**.
> Перед данными добавьте по одному значению перечисления **и** строке в карту имён:
>
> | Новый id | Перечисление / карта имён |
> |---|---|
> | `kPlagueTouch`, `kSwampPlague` | `EAffect` — `affect_contants.h` + `affect_contants.cpp` |
> | `kPlagueMiasma` | `ERoomAffect` — `affects/room_affects.h` + `affects/room_affects.cpp` |
> | `kPlagueBlade` | `EObjAffect` — `obj_affects.h` + `obj_affects.cpp` |
> | `kInfectBlade` | `ESpell` — `magic/spells_constants.h` + `magic/spells_constants.cpp` |
>
> Плюс один маленький обработчик `<alter_obj>` (§4.1), который вешает *временный*
> эффект на оружие. **Всё остальное ниже — чистые данные** (флаги, apply, DoT,
> триггеры, шансы). Это и есть граница «данные ↔ код».

### 4.1 Заклинание — `spells.xml` (+ крошечный обработчик)

`kInfectBlade` («заразить клинок») кастуется на оружие и через `<alter_obj>` вешает
на него **временный** эффект `kPlagueBlade`.

```xml
<spell id="kInfectBlade" element="kDark" mode="kEnabled">
    <name val="infect blade"/>
    <components><verbal/><weave/></components>
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

(Чтобы требовать реагент, добавьте в `<components>` `<material any_of="…"/>` с vnum
предмета-реагента — голый `<material/>` без `any_of` и `all_of` при загрузке
пропускается с ошибкой, §3.9.)

Готовые обработчики `<alter_obj>` `AlterBless`/`AlterCurse` вешают **постоянный**
эффект (`obj_affects::Impose(obj, …, -1)`). Для *временного* нужен свой обработчик —
копия `AlterCurse`, где вместо `-1` передан положительный таймер (схематично):

```cpp
// src/gameplay/handlers/alter_infect_blade.cpp — временно заражает оружие чумой.
EStageResult AlterInfectBlade(ActionContext &ctx) {
    ObjData *obj = ctx.ovict;
    if (!obj || obj->get_type() != EObjType::kWeapon) {
        return EStageResult::kFail;                 // -> общая строка «нет эффекта»
    }
    const int timer = 24;                           // игровых часов; -1 было бы «навсегда»
    obj_affects::Impose(obj, obj_affects::EObjAffect::kPlagueBlade,
                        timer, /*modifier*/0,
                        ctx.caster() ? ctx.caster()->get_uid() : 0,
                        ctx.CompetenceBase());       // сила -> потенция эффекта предмета
    return AlterMsg(ctx, ESpellMsg::kAlterObjToChar);
}
```

Объявите его в `handlers/spell_handlers.h` и зарегистрируйте в реестре
`kAlterObjHandlers` (`magic/action_handlers.cpp`):
`{"AlterInfectBlade", handlers::AlterInfectBlade}`. Таймер — это аргумент
`duration` у `Impose`; `obj_affects::Tick` убавляет его раз в игровой час.

### 4.2 Эффект предмета — `obj_affects.xml`

`kPlagueBlade` на ударе с шансом накладывает `kPlagueTouch` на жертву. Так как
нагрузка — **обычный эффект персонажа**, это выражается **чисто данными** через
`<affects>` (обработчик не нужен — в отличие от `kPoisoned`, которому нужен код ради
подтипа яда и броска навыка).

```xml
<obj_affect id="kPlagueBlade" msg_case="kIns" see_affect="kDetectMagic">
    <actions>
        <action target="kTarActor">
            <trigger val="kWeaponHit" prob="15"/>
            <affects saving="kCritical" resist="kDark">
                <duration base="4" skill_divisor="0" min="0" max="0"/>
                <affect id="kPlagueTouch"/>
            </affects>
        </action>
    </actions>
</obj_affect>
```

* `kWeaponHit` срабатывает, когда заражённое оружие наносит удар; `prob="15"` = 15 %
  ударов. `saving=` у `<affects>` можно опустить, но тогда по умолчанию будет
  `kReflex`, а не «без спасброска» — для этого пишите `saving="kNone"`. (У `<damage>`
  `saving=` действительно обязателен: без него при загрузке отбрасывается весь список
  действий.)
* `target="kTarActor"` — тот, кого ударили.
* Плоские 4 часа: действие по триггеру идёт без кастующего заклинания, поэтому
  `skill_divisor` ничего бы не добавил (навык для длительности берётся из броска силы
  заклинания либо из явного `skill="kX"` у `<duration>` —
  [Руководство по эффектам](AFFECT_MANUAL_RU.md) §5.6).
* `msg_case="kIns"` — имя предмета склоняется в творительном («клинком») в
  сообщениях эффекта.

### 4.3 Эффект персонажа — «касание» (`affects.xml`)

`kPlagueTouch` **развеиваемый и излечимый** — слабый DoT + штрафы к Силе и Уму; при
естественном истечении заражает жертву **и трёх союзников** `kSwampPlague`.

```xml
<affect id="kPlagueTouch" buff="N">
    <flags val="kAfSameTime|kAfDispellable|kAfCurable"/>
    <apply location="kStr"><modifier min="2" beta="0" factor="-1"/></apply>
    <apply location="kInt"><modifier min="2" beta="0" factor="-1"/></apply>
    <actions>
        <!-- (a) ПОКА АКТИВЕН: слабый DoT каждый тик. -->
        <action target="kTarFightSelf">
            <trigger val="kPulse"/>
            <damage saving="kNone"><amount min="0" beta="0.4" weight="0"/></damage>
        </action>
        <!-- (b) ПРИ ИСТЕЧЕНИИ: сама жертва -> kSwampPlague. -->
        <action target="kTarFightSelf">
            <trigger val="kExpired"/>
            <affects saving="kNone">
                <duration base="6" skill_divisor="0" min="6" max="6"/>
                <affect id="kSwampPlague"/>
            </affects>
        </action>
        <!-- (c) ПРИ ИСТЕЧЕНИИ: 3 случайных союзника в комнате -> kSwampPlague.
             Действия kDispell НЕТ -> развеять/вылечить рано безопасно. -->
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

* `buff="N"` = **дебаф** — на это гейтится лечение (одного `kAfCurable` мало).
  `kAfCurable` + `kAfDispellable` → снять можно и лечением, и развеиванием.
* Плоские `-2` к Силе/Уму (`beta="0"` → без зависимости от компетентности).
* **(b) и (c)** — оба на `kExpired`: жертва и трое из её группы. Дедуп «раз на тип»
  срабатывает раз на тип эффекта за проход, но **все** действия с `kExpired`
  выполняются, поэтому оба наложения проходят.
* **Нет `kDispell`** → раннее снятие ничего не запускает; нагрузка только на пути
  естественного истечения (контраст `kExpired`/`kDispell`, как в §1.2).

### 4.4 Эффект персонажа — «болотная чума» (`affects.xml`)

`kSwampPlague` — тяжёлая стадия: DoT в несколько раз сильнее «касания», плюс
**заразность** — каждый тик по 5 % шанса заразить союзника и по 5 % заразить саму
комнату.

```xml
<affect id="kSwampPlague" buff="N">
    <flags val="kAfSameTime|kAfCurable"/>
    <apply location="kStr"><modifier min="4" beta="0" factor="-1"/></apply>
    <apply location="kInt"><modifier min="4" beta="0" factor="-1"/></apply>
    <actions>
        <!-- (a) сильный DoT — в несколько раз больше «касания» (beta 1.5 против 0.4). -->
        <action target="kTarFightSelf">
            <trigger val="kPulse"/>
            <damage saving="kNone"><amount min="0" beta="1.5" weight="0"/></damage>
        </action>
        <!-- (b) 5% за тик: заразить одного случайного союзника в комнате. -->
        <action target="kTarRandomAlly">
            <trigger val="kPulse" prob="5"/>
            <affects saving="kNone">
                <duration base="6" skill_divisor="0" min="6" max="6"/>
                <affect id="kSwampPlague"/>
            </affects>
        </action>
        <!-- (c) 5% за тик: осадить в самой КОМНАТЕ миазмы (эффект комнаты, §4.5). -->
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

* `kAfCurable`, **но не** `kAfDispellable` — развитую заразу **лечат, а не
  развеивают** (как `kBlightCurse` в §1.3). DoT `beta="1.5"` кусает почти вчетверо
  сильнее «касания».
* **(b)** `target="kTarRandomAlly"` + `prob="5"` — на каждый тик 5 % шанс перекинуть
  чуму на одного дружественного в комнате: заражение **персонаж → персонаж**.
* **(c)** `target="kTarRoomThis"` + `prob="5"` — 5 % шанс осадить в комнате эффект
  `kPlagueMiasma`: заражение **персонаж → комната**. `<affect id>` резолвится и как
  `EAffect`, и как `ERoomAffect`, поэтому один и тот же грамматический блок вешает
  эффект комнаты, если цель — комната.

### 4.5 Эффект комнаты — «миазмы» (`room_affects.xml`)

`kPlagueMiasma` заражает **входящих** игроков `kPlagueTouch` — так зараза
распространяется по локации даже без носителя.

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

* `kEnterPC` — срабатывает на входящего игрока (`kTarActor`). `return="0"` **не**
  ставим — вход не запрещаем, только заражаем.
* `<target_conditions><blocking>` по `kPlagueTouch` — не заражать повторно уже
  больного.
* Нагрузка — снова `kPlagueTouch` со спасброском: круг замыкается.

### 4.6 Сообщения

Каждому новому id нужны свои сообщения (иначе он молчит):

* `cfg/messages/ru/spell_msg.xml` — sheaf `kInfectBlade`: русское имя для игры
  (`<name val="заразить клинок"/>`) и `kAlterObjToChar` (строка, которую шлёт
  обработчик).
* `cfg/messages/ru/obj_affect_msg.xml` — sheaf `kPlagueBlade`: `kShortDesc`
  («налёт чумы»), `kDiagToChar` (строка при осмотре).
* `cfg/messages/ru/affect_msg.xml` — sheaves `kPlagueTouch` / `kSwampPlague`:
  наложение/спадение + `kDamageToChar`/`kDamageToRoom` для DoT (иначе тик молчит).
* `cfg/messages/ru/room_affect_msg.xml` — sheaf `kPlagueMiasma`: описание миазмов в
  комнате (`kRoomAffectVisible`).

### 4.7 Как это играется

1. Маг кастует `kInfectBlade` на меч → меч на 24 ч несёт `kPlagueBlade`.
2. Каждый удар: 15 % → жертва получает `kPlagueTouch` (4 ч).
3. `kPlagueTouch`: слабый DoT + −2 Сила/Ум. Развейте или вылечите — и всё, безопасно.
4. Если проигнорировать → таймер истёк → жертва **и 3 союзника** получают
   `kSwampPlague` на 6 ч.
5. `kSwampPlague`: тяжёлый DoT; каждый тик 5 % → зараза прыгает на случайного
   союзника (**персонаж→персонаж**) и 5 % → оседает в комнате как миазмы
   (**персонаж→комната**).
6. `kPlagueMiasma`: всякий вошедший в комнату ловит `kPlagueTouch`
   (**комната→персонаж**) — и цикл повторяется.

Задействованы все три вида: **предмет** (`kPlagueBlade`), **персонаж**
(`kPlagueTouch`, `kSwampPlague`) и **комната** (`kPlagueMiasma`).

---

## 5. Способности, перекраивающие Чумной клинок — `feats.xml`

Как §2 перекраивает «Порчу увядания», эти способности правят чумную цепочку из §4
через `talent_patch` (см. [Руководство по способностям](FEAT_MANUAL_RU.md) §4):
`modify`/`append`, патч заклинания против патча эффекта и гейтинг `relative`.

```xml
<!-- 1) modify поля ЭФФЕКТА: болотная чума грызёт сильнее (DoT x1.5). -->
<feat id="kPestilence" mode="kEnabled">
    <talent_patches>
        <talent_patch affect="kSwampPlague" op="modify" effect="kDamage">
            <modify field="beta" mul="1.5"/>
        </talent_patch>
    </talent_patches>
</feat>

<!-- 2) modify area ЭФФЕКТА: при истечении «касание» заражает +2 союзников (пол 3 -> 5). -->
<feat id="kVirulentStrain" mode="kEnabled">
    <talent_patches>
        <talent_patch affect="kPlagueTouch" op="modify" effect="kArea">
            <modify field="min_targets" add="2"/>
        </talent_patch>
    </talent_patches>
</feat>

<!-- 3) append к ЭФФЕКТУ: чума перекидывается ещё и на врагов (5% за тик). -->
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

<!-- 4) append к ЗАКЛИНАНИЮ: заражая клинок, чумодел укрепляется сам. Заклинание
     нацелено на ПРЕДМЕТ (kTarObjInv), но у добавленного действия своя цель
     (kTarFightSelf) -> оно бьёт по кастеру. Это патч ЗАКЛИНАНИЯ, не эффекта. -->
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

<!-- 5) гейтинг relative: заботливый ЛИДЕР группы ослабляет чуму у ведомых. Если больной
     союзник ведом кем-то с этой способностью, DoT его kSwampPlague вдвое слабее —
     гейт по способности ЛИДЕРА, не самого больного. -->
<feat id="kPlagueWard" mode="kEnabled">
    <talent_patches>
        <talent_patch affect="kSwampPlague" relative="group_leader" op="modify" effect="kDamage">
            <modify field="beta" mul="0.5"/>
        </talent_patch>
    </talent_patches>
</feat>
```

Что показывает каждая:

| способность | цель | операция | механика |
|---|---|---|---|
| `kPestilence` | эффект `kSwampPlague` | `modify` `kDamage` | масштаб числового поля проявления (`beta`) |
| `kVirulentStrain` | эффект `kPlagueTouch` | `modify` `kArea` | масштаб числа целей area (`min_targets`) |
| `kUnbridledPlague` | эффект `kSwampPlague` | `append` | добавить действие (заражение врагов) в цепочку тика |
| `kPlaguecrafter` | **заклинание** `kInfectBlade` | `append` | патч *заклинания*; у действия своя цель `kTarFightSelf` |
| `kPlagueWard` | эффект `kSwampPlague` | `modify` + `relative="group_leader"` | гейт патча по способности *родственника* |

Все под гейтом `CanUseFeat(holder, feat)` и ничего не стоят при касте/тике тем, у кого
их нет. Способности — данные (`feats.xml`) поверх идентификаторов из §4; новой
логики они не требуют — только своих значений перечисления `EFeat` и строк в карте
имён (`feats_constants.h` + `feats_constants.cpp`), как и любой id.
