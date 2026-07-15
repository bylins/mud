# Lua-скрипты для билдеров зон

Этот документ описывает Lua-триггеры, которые можно использовать вместо DG Script в world-файлах триггеров.

Lua-поддержка сейчас является прототипом и работает только в сборке с `-Dluajit_prototype=true`. Обычная сборка без LuaJIT должна продолжать читать DG-триггеры как раньше.

**Безопасность:** глобалы `os`, `io`, `debug`, `package` и `require` намеренно отключены. Для реального календарного времени используйте `mud.date("%j")`, `mud.date("*t")`, `mud.date("exact")` и т.п. (не os.date).

## Формат триггера в legacy world-файлах

Lua-триггер отличается от DG-триггера пятым словом `lua` в строке параметров:

```text
#4022
lua greet wait test~
0 q 100 0 lua
~
-- Lua-код
return true
~
```

## Формат триггера в YAML world-файлах

В YAML-триггере укажите `language: lua`. Поле `script` содержит Lua-код тем же literal block, что и DG-скрипт:

```yaml
name: lua greet wait test
attach_type: mob
language: lua
narg: 100
trigger_types:
  - greet
script: |
  -- Lua-код
  return true
```

Если `language` не указано, триггер считается обычным DG-триггером. Для совместимости загрузчик также понимает `script_language: lua`.

Lua-триггер можно писать как обычное тело функции: движок выполняет такой chunk с доступным `ctx` - контекстом владельца триггера, актера, объекта и других данных события. Старый явный формат `return function(ctx) ... end` продолжает работать.

Номер строки в ошибке Lua соответствует номеру в `tstat -n <vnum>` и в нумерованном выводе `/n` текстового редактора OLC. Пустые строки учитываются. Пока изменения в OLC не сохранены, runtime-сообщения относятся к предыдущей сохраненной версии триггера.

Номер строки в диагностике DG и Lua имеет разный смысл. Для DG строки команд нумерует сам движок. Для Lua номер сообщает LuaJIT: это строка, на которой парсер или выполняемый код обнаружил ошибку. Первопричина синтаксической ошибки может находиться раньше. Например, если на строке 7 написано незаконченное выражение `asdasd`, а на строке 8 начинается `mud.wait(1)`, Lua может сообщить об ошибке на строке 8: только встретив `mud`, парсер понял, что после `asdasd` ожидался оператор присваивания. Поэтому при синтаксической ошибке проверяйте не только указанную строку, но и несколько строк перед ней, включая незакрытые скобки, строки и блоки.

Поля контекста также доступны как короткие глобальные имена с префиксом `tg`: `tgOwner`, `tgActor`, `tgVictim`, `tgObject`, `tgRoom` и т.п. Это те же live-view значения, что и в `ctx`, поэтому после `mud.wait(...)` они смотрят на обновленный контекст.

Возвращаемое значение:

- `true`, `1` или отсутствие явного результата - триггер считается сработавшим.
- `false` или `0` - триггер возвращает отрицательный результат, аналогично DG-логике для проверок.

Финальный `return true` можно не писать: если Lua-код дошел до конца без явного `return`, результат будет успешным. `return false` нельзя опускать в ветках, где нужно отменить или не подтвердить событие.

## Кодировка

World-файлы в проекте исторически лежат в KOI8-R. Если Lua-код содержит русский текст, команды или алиасы предметов, сохраняйте файл в той же кодировке, что и остальные world-файлы.

Для команд через `force` безопаснее использовать ASCII-команды:

```lua
mud.force(ctx.owner, "give all " .. ctx.actor.name)
```

Русский вариант тоже может работать, но он зависит от кодировки строки:

```lua
mud.force(ctx.owner, "дать все " .. ctx.actor.name)
```

Не используйте `всё` с буквой `ё`: команда `give all` распознает `all`, а русский разбор `all` в коде ожидает старый вариант `все`.

Не используйте символ `~` внутри Lua-кода. В world-файлах `~` завершает текстовый блок триггера, поэтому Lua-оператор `~=` может оборвать скрипт при загрузке. Для проверки "не равно" пишите так:

```lua
if not (value == nil) then
  -- ok
end
```

## Базовый шаблон

```lua
if ctx.owner == nil or not ctx.owner:isValid() then
  return false
end

if ctx.actor == nil or not ctx.actor:isValid() then
  return false
end

-- полезная работа
```

Старый явный вариант остается допустимым:

```lua
return function(ctx)
  -- Lua-код
end
```

Через короткие глобальные имена тот же шаблон можно писать так:

```lua
if tgOwner == nil or not tgOwner:isValid() then
  return false
end

if tgActor == nil or not tgActor:isValid() then
  return false
end

-- полезная работа
```

После `mud.wait(...)` всегда заново проверяйте персонажей и объекты через `:isValid()`: за время ожидания игрок мог уйти, моб мог умереть, объект могли удалить.

## Примеры

### Реплики с задержками

Пример моб-триггера с несколькими репликами и паузами:

```lua
if ctx.owner == nil or not ctx.owner:isValid() then
  return false
end

mud.wait(10)
ctx.owner:force("say привет")

mud.wait(2, "s")
ctx.owner:force("say пока")
```

`mud.wait(10)` ждет 10 игровых pulses. `mud.wait(2, "s")` ждет 2 реальных секунды.

### Загрузить предмет мобу и заставить его отдать игроку

Этот вариант нужен, когда важно сохранить обычное поведение команды `give`: сообщения игроку и комнате, проверки переносимого веса, `give_otrigger` и `receive_mtrigger`.

```lua
return function(ctx)
  if ctx.owner == nil or not ctx.owner:isValid() then
    return false
  end

  if ctx.actor == nil or not ctx.actor:isValid() then
    return false
  end

  local obj = mud.loadObjToChar(2210, ctx.owner)
  if obj then
    mud.force(ctx.owner, "give all " .. ctx.actor.name)
  end

  return true
end
```

Если у моба могут быть другие предметы, вместо `all` используйте alias конкретного предмета:

```lua
mud.force(ctx.owner, "give bow " .. ctx.actor.name)
```

Для русских alias строка должна быть в KOI8-R.

### Загрузить моба или предмет в комнату

```lua
return function(ctx)
  local room = ctx.room
  if room == nil then
    return false
  end

  local obj = mud.loadObj(2210, room)
  local mob = mud.loadMob(4020, room)

  if obj then
    room:echo("Что-то появляется на полу.")
  end

  return not (mob == nil)
end
```

То же самое можно писать через методы комнаты:

```lua
local obj = ctx.room:loadObj(2210)
local mob = ctx.room:loadMob(4020)
```

### Счетчик на владельце

`context` хранит значения в DG-переменных владельца триггера. Имена переменных нечувствительны к регистру, значения хранятся как строки.

```lua
return function(ctx)
  local count = tonumber(ctx.owner.context.visits or "0") or 0
  count = count + 1
  ctx.owner.context.visits = count

  if count == 1 then
    ctx.owner:force("say Вижу тебя впервые.")
  else
    ctx.owner:force("say Мы уже встречались.")
  end

  return true
end
```

Удаление переменной:

```lua
ctx.owner.context:delete("visits")
```

### Проверка случайности

```lua
return function(ctx)
  if mud.percent(25) then
    mud.echo("В комнате что-то шуршит.")
  end

  return true
end
```

## Контекст `ctx`

`ctx` передается в каждый Lua-триггер.

| Поле | Тип | Описание |
| --- | --- | --- |
| `ctx.trigger` | table | Данные текущего триггера. |
| `ctx.trigger.vnum` | number | VNUM триггера. |
| `ctx.trigger.rnum` | number | RNUM триггера. |
| `ctx.trigger.name` | string | Название триггера. |
| `ctx.trigger.attachType` | number | Тип привязки: mob/object/room в числовом виде. |
| `ctx.trigger.triggerType` | number | Битовая маска типа события. |
| `ctx.owner` | Char, Obj, Room или nil | Владелец триггера. |
| `ctx.actor` | Char или nil | Персонаж, вызвавший событие. |
| `ctx.victim` | Char или nil | Жертва события, если есть. |
| `ctx.object` | Obj или nil | Объект события, если есть. |
| `ctx.room` | Room или nil | Комната владельца/события. |
| `ctx.command` | string | Команда для command-триггеров. |
| `ctx.argument` | string | Аргументы команды. |
| `ctx.speech` | string | Текст речи для speech-триггеров. |
| `ctx.direction` | string | Направление для move/door-событий. |
| `ctx.damageAmount` | number | Величина урона для damage-событий. |
| `ctx.damageType` | string | Тип урона строкой. |
| `ctx.where` | number | Дополнительное числовое поле события. |
| `ctx.time` | number | Дополнительное поле времени. |
| `ctx.timeDay` | number | Дополнительное поле времени суток. |

Не все поля заполнены во всех типах триггеров. Всегда проверяйте `nil`.

Те же поля можно читать через короткие глобальные имена:

| Имя | Эквивалент |
| --- | --- |
| `tgTrigger` | `ctx.trigger` |
| `tgOwner` | `ctx.owner` |
| `tgActor` | `ctx.actor` |
| `tgVictim` | `ctx.victim` |
| `tgObject` | `ctx.object` |
| `tgRoom` | `ctx.room` |
| `tgCommand` | `ctx.command` |
| `tgArgument` | `ctx.argument` |
| `tgSpeech` | `ctx.speech` |
| `tgDirection` | `ctx.direction` |
| `tgDamageAmount` | `ctx.damageAmount` |
| `tgDamageType` | `ctx.damageType` |
| `tgWhere` | `ctx.where` |
| `tgTime` | `ctx.time` |
| `tgTimeDay` | `ctx.timeDay` |

## Глобальный объект `mud`

### Логирование и случайность

| Вызов | Возврат | Описание |
| --- | --- | --- |
| `mud.log(message)` | bool | Пишет сообщение в script log с данными Lua-триггера. |
| `mud.random(limit)` | number | Случайное число от 1 до `limit`; при ошибке возвращает 0. Аналог DG `%random.N%`. |
| `mud.random(m, n)` | number | Случайное число от `m` до `n` включительно; при ошибке возвращает 0. Аналог DG `%number.range(m,n)%`. |
| `mud.roll(count, sides)` | number | Бросок `count` кубиков по `sides` граней. Ограничения: до 1000 кубиков и до 1000000 граней. |
| `mud.percent(chance)` | bool | Проверка процента от 1 до 100. |

### Поиск и счетчики

| Вызов | Возврат | Описание |
| --- | --- | --- |
| `mud.charByUid(uid)` | Char или nil | Находит персонажа по UID. |
| `mud.objById(id)` | Obj или nil | Находит объект по runtime id. |
| `mud.findMob(vnum)` | Char или nil | Находит первого live-моба по VNUM. Lua-замена `calcuid ... mob`. |
| `mud.findObj(vnum)` | Obj или nil | Находит первый live-объект по VNUM. Lua-замена `calcuid ... obj`. |
| `mud.mobCount(vnum)` | number | Сколько мобов данного VNUM сейчас в мире. |
| `mud.objCount(vnum)` | number | Сколько объектов данного VNUM сейчас в мире. |
| `mud.world.curobjCount(vnum)` | number | DG-семантика `%world.curobjs%`: текущий count объекта, но для stable/unlimited timer объектов возвращает 0. |
| `mud.world.gameobjCount(vnum)` | number | DG-семантика `%world.gameobs%`/`%world.gameobjs%`: считает игровые экземпляры, для обычных takeable не-quest предметов учитывает parent-прототип. |
| `mud.world.maxobjCount(vnum)` | number | DG-семантика `%world.maxobj%`: MIW объекта, для unlimited возвращает `9999999`. |
| `mud.world.set(context, name, value)` | bool | Lua-only world-переменная в явном целочисленном context. Значения: string, number, boolean; string-значения приводятся к нижнему регистру. |
| `mud.world.get(context, name [, default])` | string или default/nil | Читает Lua-only world-переменную. При невалидном context/name возвращает nil. |
| `mud.world.delete(context, name)` | bool | Удаляет Lua-only world-переменную. |
| `mud.world.clearContext(context)` | bool | Удаляет все Lua-only world-переменные указанного ненулевого context. Context `0` отклоняется. |

Lua-only world-переменные не используют DG `worlds_vars` и не видны из DG-триггеров. Context передается явно, `Trigger::context` не меняется:

```lua
local CTX = 2005

mud.world.set(CTX, "maze_ready", 1)

if mud.world.get(CTX, "maze_ready") == "1" then
  -- ...
end

mud.world.delete(CTX, "maze_ready")
```

### Комнаты, зоны, время и погода

| Вызов | Возврат | Описание |
| --- | --- | --- |
| `mud.room(vnum)` | Room или nil | Комната по VNUM. |
| `mud.zone(vnum)` | table или nil | Зона по VNUM. |
| `mud.time()` | table | `{ hour, day, month, year }` (игровое время). |
| `mud.date(fmt [, ts])` | string или table | Реальное календарное время (безопасный аналог os.date). `mud.date("%j")` — день года строкой, `mud.date("*t")` — таблица {year,month,day,...,yday}, `mud.date("exact")` — DG-аналог `%date.exact%`, миллисекунды Unix epoch строкой. |
| `mud.weather()` | table | Данные погоды. |

Поля `mud.zone(vnum)`: `vnum`, `name`, `top`, `age`, `lifespan`, `resetMode`, `used`, `locked`, `underConstruction`.

Для арифметики времени преобразуйте `mud.date("exact")` в число:

```lua
local t1 = tonumber(mud.date("exact"))
-- код
local elapsed = tonumber(mud.date("exact")) - t1
mud.log("время выполнения " .. elapsed .. " миллисекунд")
```

Поля `mud.weather()`: `temperature`, `pressure`, `change`, `sky`, `sunlight`, `moonDay`, `season`, `weatherType`, `rainlevel`, `snowlevel`, `icelevel`.

### Загрузка сущностей

| Вызов | Возврат | Описание |
| --- | --- | --- |
| `mud.loadObj(vnum, room)` | Obj или nil | Создает предмет и кладет его в комнату. `room` может быть VNUM комнаты или Room-объектом. |
| `mud.loadObjToChar(vnum, char)` | Obj или nil | Создает предмет и кладет его в инвентарь персонажа. Запускает load object trigger. |
| `mud.loadMob(vnum, room)` | Char или nil | Создает моба в комнате. `room` может быть VNUM комнаты или Room-объектом. |

### Действия

| Вызов | Возврат | Описание |
| --- | --- | --- |
| `mud.purge(entity)` | bool | Удаляет сущность через ее метод `purge`. PC удалить нельзя. Самопурж владельца триггера (mob/object) разрешён (аналогично DG). |
| `mud.damage(victim, amount, type)` | bool | Наносит урон. `type`: `nil`, `"physic"`, `"magic"`, `"poisonous"`. |
| `mud.castSpell(spell_name, target)` | bool | Скриптовый каст заклинания от имени владельца триггера; ближайший DG-аналог - `dgcast`. `target`: `nil`, строка поиска, Char, Obj или Room. |
| `mud.transfer(entity, room)` | bool | Перемещает Char или Obj в комнату. |
| `mud.force(char, command)` | bool | Заставляет персонажа выполнить команду. Цель должна быть в комнате владельца триггера. |
| `mud.echo(message)` | bool | Сообщение в комнату владельца триггера. |
| `mud.echoaround(actor, message)` | bool | Lua-аналог DG `echoaround`/`mechoaround`/`oechoaround`/`wechoaround`: сообщение всем SENDOK в комнате `actor` (по текущему местоположению актора), без отправки самому `actor` и без запуска ACT-триггеров. |
| `mud.wait(...)` | yield | Приостанавливает Lua-триггер. |

`mud.force` запрещает внутренние mob script команды (`mload`, `mecho` и т.п.) и пропускает строку через обычный игровой command interpreter.

`mud.damage(victim, amount, nil)` работает как DG `mdamage` без третьего аргумента: просто снимает HP, показывает damage-сообщение и не начинает бой. Если указать тип урона (`"physic"`, `"magic"`, `"poisonous"`), Lua использует боевой `Damage::Process`, как DG `mdamage ... physic/magic/poisonous`; такой урон может запустить бой.

```lua
mud.damage(ctx.actor, mud.random(50), nil)       -- без боя, DG-style mdamage
mud.damage(ctx.actor, 20, "magic")              -- typed damage через Damage::Process
```

`mud.castSpell(spell_name, target)` возвращает `true`, если игровая `CallMagic`-логика дала эффект. Ближайший DG-аналог - `dgcast`, но это самостоятельный Lua API, а не точная обертка: строковая цель ищется как в обычном cast-поиске, а direct-цели Char/Obj/Room проверяются по target-флагам заклинания и расположению цели. Direct Obj-цель должна быть в инвентаре, экипировке или комнате кастера; для `locate object` передавайте строковую цель. Для room/object-триггеров без персонажа-кастера Lua создает временного системного кастера в комнате владельца.

`mud.echoaround(actor, message)` подходит для переноса DG `echoaround`/`mechoaround`/`oechoaround`/`wechoaround`, когда нужно показать текст вокруг персонажа без отдельного сообщения самому `actor`. Это простая рассылка сообщения, а не игровой `act`, поэтому `$n`/`$N` act-подстановки не раскрываются; для имени и окончаний собирайте строку явно через `actor.names.*`:

```lua
mud.echoaround(ctx.actor, ctx.actor.names.iname .. " вздрагивает" .. ctx.actor.names.g .. " и осматривается.")
```

Если нужен полный игровой `act` с victim/to-режимами, используйте `actor:act(..., { to = "room" })`.

Если DG-скрипт выбирал имя и окончание через лидера, в Lua берите лидера явно:

```lua
local actor = ctx.actor
local who = actor and (actor.leader or actor)
if who then
  mud.echoaround(actor, who.names.iname .. " осматривается" .. who.names.u .. ".")
end
```

## `mud.wait`

Поддерживаемые формы:

```lua
mud.wait(10)          -- 10 pulses
mud.wait(2, "s")      -- 2 секунды
mud.wait(1, "t")      -- 1 mud-hour/tick
mud.wait("10 s")      -- 10 секунд
mud.wait("3 t")       -- 3 mud-hour/tick
mud.wait("until 7:30")
mud.wait("until 0730")
```

Ограничения:

- Нельзя использовать в death-триггерах мобов.
- Нельзя использовать в purge-триггерах объектов.
- Если владелец, объект или сам триггер удалены во время ожидания, продолжение отменяется.
- После ожидания поля `ctx` обновляются по текущему состоянию мира. `ctx.actor`/`ctx.victim` остаются live-view персонажей из события; если персонаж вышел из комнаты события, действия с ним вернут `false`/пустые значения без ошибки. `room:people()` и `room:objects()` возвращают live-view комнаты, без копирования списка в Lua.

## Char

Char - это Lua-view персонажа или моба. Все поля read-only.

### Поля

| Поле | Тип | Описание |
| --- | --- | --- |
| `ch.name` | string | Имя персонажа. |
| `ch.uid` | number | UID персонажа. |
| `ch.vnum` | number | VNUM моба; для PC возвращает 0. |
| `ch.hp` | number | Текущие хиты. |
| `ch.maxHp` | number | Максимальные хиты. |
| `ch.mana` | number | Текущее значение mana/mem queue. |
| `ch.move` | number | Очки движения. |
| `ch.room` | Room или nil | Текущая комната. |
| `ch.roomVnum` | number | VNUM текущей комнаты или 0. |
| `ch.leader` | Char или nil | Lua-аналог DG `%actor.leader%`: текущий лидер/master, если персонаж за кем-то следует. |
| `ch.names` | table | Read-only таблица имен, местоимений и окончаний для текстов. |
| `ch.isNpc` | bool | `true` для NPC. Это поле, не функция. |
| `ch.class` | number | Числовой ID класса, как DG `%actor.class%`. |
| `ch.sex` | number | Пол персонажа как в DG `%actor.sex%`: `0` - нейтральный, `1` - мужской, `2` - женский. |
| `ch.context` | Context или nil | Переменные DG-контекста владельца. |

Поля `ch.names`: `name`, `iname`, `rname`, `dname`, `vname`, `tname`, `pname`, `UPname`, `UPiname`, `UPrname`, `UPdname`, `UPvname`, `UPtname`, `UPpname`, `m`, `s`, `e`, `g`, `u`, `w`, `q`, `y`, `a`, `r`, `x`, `h`. Они соответствуют одноименным DG-подстановкам персонажа, например `%actor.iname%` -> `actor.names.iname`, `%actor.UPiname%` -> `actor.names.UPiname`, `%actor.u%` -> `actor.names.u`.

### Методы

| Метод | Возврат | Описание |
| --- | --- | --- |
| `ch:isValid()` | bool | Проверяет, что персонаж еще существует и не purged. |
| `ch:isPc()` | bool | `true` для игрока. |
| `ch:isImmortal()` | bool | `true` для имма. |
| `ch:level()` | number | Реальный уровень. |
| `ch:position()` | number | Текущая позиция числом. |
| `ch:hasAffect(name)` | bool | Проверяет affect по строковому имени enum. |
| `ch:canSee(target)` | bool | Видит ли `ch` другого персонажа. |
| `ch:enemy()` | Char или nil | Текущий противник. |
| `ch:gold()` | number | Возвращает наличные деньги персонажа. |
| `ch:gold(delta)` | number | Изменяет наличные на `delta` и возвращает итоговое значение. Положительное значение добавляет, отрицательное снимает. Заменяет DG `mgold` и `%actor.gold(+amount)%`. |
| `ch:teleport(room)` | bool | Перемещает персонажа в комнату. |
| `ch:equipment(pos)` | Obj или nil | Предмет в слоте экипировки `pos`, как DG `%actor.eq(pos)%`. |
| `ch:eq(pos)` | Obj или nil | Короткий алиас для `ch:equipment(pos)`. |
| `ch:haveObj(vnum_or_name)` / `ch:haveobj(...)` | Obj или nil | Lua-аналог DG `%self.haveobj(...)%`: ищет предмет только в инвентаре. Число ищется как VNUM, строка - как видимое имя. |
| `ch:lag(value)` | bool | Ставит battle lag, как DG `%actor.lag(value)%`. |
| `ch:lag(10, "p")` | bool | Ставит wait state в пульсах, эквивалент DG `%actor.lag(10p)%`. |
| `ch:skill(skill_id)` | number | Возвращает значение skill. |
| `ch:skill(skill_id, value)` | number | Устанавливает skill и возвращает значение. |
| `ch:skillTurn(skill_id_or_name, true/false)` | bool | Включает или снимает умение по аналогии с DG `skillturn`: при включении ставит 5, при снятии ставит 0. Также принимает `"set"`/`"clear"`. |
| `ch:spellTurn(spell_id_or_name, true/false)` | bool | Включает или снимает знание заклинания по аналогии с DG `spellturn`. Также принимает `"set"`/`"clear"`. |
| `ch:canGetSkill(skill_id_or_name)` | bool | Проверяет, может ли персонаж получить skill с учетом класса, уровня и ремортов. |
| `ch:canGetSpell(spell_id_or_name)` | bool | Проверяет, может ли персонаж получить spell с учетом класса, уровня и ремортов. |
| `ch:feat(feat_id)` | bool | Проверяет feat. |
| `ch:feat(feat_id, true/false)` | bool | Устанавливает или снимает feat. |
| `ch:attachTrigger(vnum)` | bool | Прикрепляет mob trigger; работает только для NPC. |
| `ch:detachTrigger(vnum)` | bool | Снимает trigger; работает только для NPC. |
| `ch:send(message)` | bool | Отправляет сообщение персонажу с descriptor. |
| `ch:force(command)` | bool | Заставляет персонажа выполнить команду. |
| `ch:rewardDailyQuest(id)` | bool | Запускает выдачу награды daily quest по id; Lua-аналог DG `%actor.questbodrich(id)%`. Если `ch` - charmice, награда оформляется на его мастера. |
| `ch:hasQuest(id)` | bool | Lua-аналог DG `%actor.quested(id)%`: проверяет наличие quested-записи. |
| `ch:getQuest(id)` | string | Lua-аналог DG `%actor.getquest(id)%`: возвращает текст quested-записи или пустую строку. |
| `ch:setQuest(id, text)` | bool | Lua-аналог DG `%actor.setquest(id text)%`: сохраняет quested-запись. Работает по тем же правилам, что DG: только для игроков, не для иммов. |
| `ch:unsetQuest(id)` | bool | Lua-аналог DG `%actor.unsetquest(id)%`: удаляет quested-запись. |
| `ch:act(message, options)` | bool | Вызывает игровой `act`. |
| `ch:purge()` | bool | Удаляет NPC. PC удалить нельзя. Самопурж владельца триггера (текущий `ctx.owner`, если это NPC) разрешён — работает как в DG-триггерах (скрипт останавливается). |

`ch:skillTurn(...)` и `ch:spellTurn(...)` меняют состояние тихо: они не отправляют персонажу сообщения и не пишут DG-style лог. Если нужно видимое сообщение, делайте его явно через `ch:send(...)` или `ch:act(...)`.

Пример `act`:

```lua
ctx.owner:act("$n смотрит на $N3.", {
  victim = ctx.actor,
  to = "room",
  hideInvisible = true
})
```

`options.to`: `"room"`, `"char"`, `"victim"`, `"notvictim"`.

## Obj

Obj - Lua-view объекта. Все поля read-only.

### Поля

| Поле | Тип | Описание |
| --- | --- | --- |
| `obj.name` | string | Short description объекта. |
| `obj.names` | table | Read-only таблица имен объекта: `name` - aliases как DG `%object.name%`; `iname`, `rname`, `dname`, `vname`, `tname`, `pname` - падежные имена. |
| `obj.iname`, `obj.rname`, `obj.dname`, `obj.vname`, `obj.tname`, `obj.pname` | string | Прямые алиасы к `obj.names.*`, например Lua-аналог DG `%object.iname%` - `object.iname`. |
| `obj.id` | number | Runtime id объекта. |
| `obj.vnum` | number | VNUM объекта. |
| `obj.roomVnum` | number | VNUM комнаты, если объект лежит в комнате; иначе 0. |
| `obj.room` | Room или nil | Комната объекта, если он лежит в комнате. |
| `obj.carriedBy` | Char или nil | Кто несет объект. |
| `obj.wornBy` | Char или nil | На ком надет объект. |
| `obj.container` | Obj или nil | Контейнер, в котором лежит объект. |
| `obj.timer` | number | Таймер объекта. |

### Методы

| Метод | Возврат | Описание |
| --- | --- | --- |
| `obj:isValid()` | bool | Проверяет, что объект еще существует и не extracted. |
| `obj:purge()` | bool | Удаляет объект из мира. Самопурж владельца триггера (если `ctx.owner` — объект) разрешён. |
| `obj:val(index)` | number | Читает старый object value по индексу 0..3, как DG `%obj.val0%`..`%obj.val3%`. |
| `obj:val(index, value)` | number | Устанавливает старый object value по индексу 0..3 и возвращает итоговое значение, как DG `%obj.valN(value)%`. |
| `obj:movetoRoom(room)` | bool | Перемещает объект в комнату. |
| `obj:giveTo(char)` | bool | Тихо кладет объект в инвентарь персонажа. Не показывает `give`-сообщения. |
| `obj:contains(vnum_or_name)` | Obj или nil | Ищет предмет только в непосредственном содержимом объекта. Число ищется как VNUM, строка - по aliases. |
| `obj:attachTrigger(vnum)` | bool | Прикрепляет object trigger. |
| `obj:detachTrigger(vnum)` | bool | Снимает object trigger. |

Важно: `obj:giveTo(char)` не равен игровой команде `give`. Если нужны обычные сообщения и give/receive-триггеры, загрузите объект в инвентарь моба и используйте `mud.force(mob, "give ...")`.

DG-паттерн `%LoadedUid.valN(...)%` после `oload` переносится через объект, который вернул `loadObj`:

```lua
local scroll = ctx.room:loadObj(11026)
if scroll then
  scroll:val(0, skill_id)
  scroll:val(1, 1)
end
```

## Room

Room - Lua-view комнаты. Все поля read-only.

### Поля

| Поле | Тип | Описание |
| --- | --- | --- |
| `room.vnum` | number | VNUM комнаты. |
| `room.name` | string | Название комнаты. |

### Методы

| Метод | Возврат | Описание |
| --- | --- | --- |
| `room:echo(message)` | bool | Сообщение в комнату. Доступно для `ctx.room` и owner-room. Для `mud.room(vnum)` echo отключен. |
| `room:people()` | live-view | Текущий список Char в комнате, индексация с 1, поддерживает `ipairs`. |
| `room:objects()` | live-view | Текущий список Obj в комнате, индексация с 1, поддерживает `ipairs`. |
| `room:wteleport(target, room)` | bool | Lua-аналог DG `wteleport`: переносит `target` из этой комнаты в указанную комнату с DG-обвязкой для charmice/лошадей, look и greet-триггеров. `target`: Char, `"all"` или `"allchar"`. |
| `room:exit(direction)` | Room или nil | Комната по направлению. `direction` может быть числом или строкой. |
| `room:setExit(direction, options)` | bool | Создает или изменяет выход. `options`: `flags`, `toRoom`, `description`, `key`, `name`, `lock`. `flags` задаются как в DG `wdoor ... flags`. `toRoom` можно передать числом-VNUM или Lua-объектом `Room`; если целевая комната не существует, метод вернет `false`. |
| `room:purgeExit(direction)` | bool | Удаляет выход в направлении, как DG `wdoor ... purge`. |
| `room:loadObj(vnum)` | Obj или nil | Создает объект в комнате. |
| `room:loadMob(vnum)` | Char или nil | Создает моба в комнате. |
| `room:attachTrigger(vnum)` | bool | Прикрепляет room trigger. |
| `room:detachTrigger(vnum)` | bool | Снимает room trigger. |

## Context

`context` доступен через `ctx.owner.context` для Char-owner. Это обертка над DG-переменными скрипта владельца.

```lua
ctx.owner.context.foo = "bar"
ctx.owner.context.count = 10
ctx.owner.context.enabled = true

local value = ctx.owner.context.foo
local fallback = ctx.owner.context:get("missing", "default")
ctx.owner.context:delete("foo")
```

Допустимые типы значений: string, number, boolean. Таблицы и функции сохранять нельзя.

## Практические замечания

- Проверяйте `nil` и `:isValid()` перед использованием `ctx.actor`, `ctx.victim`, `ctx.object`.
- После `mud.wait` проверки нужно повторить.
- Самопурж владельца триггера (`ctx.owner:purge()` или `mud.purge(ctx.owner)`) разрешён для NPC и объектов — триггер останавливается (как в DG). После пуржа не выполняйте код, который обращается к `ctx.owner`.
- Для команд через `force` предпочитайте ASCII-команды (`give`, `say`, `look`) и ASCII-служебные слова (`all`), если нет явной необходимости в русском alias.
- Lua `force` передает строку только в обычный игровой command interpreter и не исполняет DG-команды. Для бывших DG-команд используйте Lua API: например `mgold actor 2000` переносится как `ctx.actor:gold(2000)`.
- `mud.loadObjToChar` кладет объект в инвентарь без видимых сообщений. Чтобы игроки увидели передачу, используйте затем обычную команду через `force`.
- `mud.damage(victim, amount, nil)` повторяет DG `mdamage` без типа и не начинает бой.
- `mud.damage(victim, amount, "physic"|"magic"|"poisonous")` атакует от имени владельца триггера, если он валиден; иначе от имени самой жертвы. Атакующий и жертва должны быть в одной комнате; такой урон может начать бой.
- `mud.transfer` вызывает `teleport` для Char и `moveToRoom` для Obj.
- Lua-view read-only: нельзя писать `ctx.actor.hp = 10` или `obj.timer = 0`.

### Перенос некоторых DG-команд

`osend`/`wsend` в Lua обычно заменяются прямой отправкой известному персонажу:

```lua
ctx.actor:send("Текст сообщения.")
```

Lua не запускает DG `sub_write` для `$n`/`%actor.name%` внутри строки, поэтому подстановки собираются явно через Lua API:

```lua
local n = ctx.actor.names
ctx.actor:send(n.UPiname .. " кивает" .. n.g .. ".")
```

`wat <room> <command>` в Lua чаще не нужен: берите комнату явно и вызывайте нужный метод:

```lua
local room = mud.room(3001)
if room then
  room:loadMob(30010)
  room:setExit("north", { toRoom = 3002 })
end
```

Для `mud.room(vnum)` намеренно отключен `room:echo`; прямого аналога `wat <room> echo ...` сейчас нет.

`wteleport <target> <room>` покрывается методом комнаты:

```lua
ctx.room:wteleport(ctx.actor, 3001)
ctx.room:wteleport("all", 3001)
ctx.room:wteleport("allchar", 3001)
```

`room:wteleport` повторяет DG-обвязку `wteleport`: для одиночной цели переносит charmice владельца, учитывает лошадь, вызывает `look` и greet-триггеры; для `"all"` переносит всех из source-room; для `"allchar"` переносит игроков и charmice.

Если нужна простая техническая перестановка без DG-обвязки, используйте `ch:teleport(room)` или `mud.transfer(ch, room)`:

```lua
ctx.actor:teleport(3001)
```
