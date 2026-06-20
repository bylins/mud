# Lua-скрипты для билдеров зон

Этот документ описывает Lua-триггеры, которые можно использовать вместо DG Script в world-файлах триггеров.

Lua-поддержка сейчас является прототипом и работает только в сборке с `-Dluajit_prototype=true`. Обычная сборка без LuaJIT должна продолжать читать DG-триггеры как раньше.

## Формат триггера

Lua-триггер отличается от DG-триггера пятым словом `lua` в строке параметров:

```text
#4022
lua greet wait test~
0 q 100 0 lua
~
return function(ctx)
  -- Lua-код
  return true
end
~
```

Lua-триггер должен вернуть функцию `function(ctx) ... end`. Движок вызывает эту функцию и передает в нее `ctx` - контекст с владельцем триггера, актером, объектом и другими данными события.

Возвращаемое значение:

- `true`, `1` или отсутствие явного результата - триггер считается сработавшим.
- `false` или `0` - триггер возвращает отрицательный результат, аналогично DG-логике для проверок.

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
return function(ctx)
  if ctx.owner == nil or not ctx.owner:is_valid() then
    return false
  end

  if ctx.actor == nil or not ctx.actor:is_valid() then
    return false
  end

  -- полезная работа
  return true
end
```

После `mud.wait(...)` всегда заново проверяйте персонажей и объекты через `:is_valid()`: за время ожидания игрок мог уйти, моб мог умереть, объект могли удалить.

## Примеры

### Реплики с задержками

Пример моб-триггера с несколькими репликами и паузами:

```lua
return function(ctx)
  if ctx.owner == nil or not ctx.owner:is_valid() then
    return false
  end

  mud.wait(10)
  ctx.owner:force("say привет")

  mud.wait(2, "s")
  ctx.owner:force("say пока")

  return true
end
```

`mud.wait(10)` ждет 10 игровых pulses. `mud.wait(2, "s")` ждет 2 реальных секунды.

### Загрузить предмет мобу и заставить его отдать игроку

Этот вариант нужен, когда важно сохранить обычное поведение команды `give`: сообщения игроку и комнате, проверки переносимого веса, `give_otrigger` и `receive_mtrigger`.

```lua
return function(ctx)
  if ctx.owner == nil or not ctx.owner:is_valid() then
    return false
  end

  if ctx.actor == nil or not ctx.actor:is_valid() then
    return false
  end

  local obj = mud.load_obj_to_char(2210, ctx.owner)
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

  local obj = mud.load_obj(2210, room)
  local mob = mud.load_mob(4020, room)

  if obj then
    room:echo("Что-то появляется на полу.\r\n")
  end

  return not (mob == nil)
end
```

То же самое можно писать через методы комнаты:

```lua
local obj = ctx.room:load_obj(2210)
local mob = ctx.room:load_mob(4020)
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
    mud.echo("В комнате что-то шуршит.\r\n")
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
| `ctx.trigger.attach_type` | number | Тип привязки: mob/object/room в числовом виде. |
| `ctx.trigger.trigger_type` | number | Битовая маска типа события. |
| `ctx.owner` | Char, Obj, Room или nil | Владелец триггера. |
| `ctx.actor` | Char или nil | Персонаж, вызвавший событие. |
| `ctx.victim` | Char или nil | Жертва события, если есть. |
| `ctx.object` | Obj или nil | Объект события, если есть. |
| `ctx.room` | Room или nil | Комната владельца/события. |
| `ctx.command` | string | Команда для command-триггеров. |
| `ctx.argument` | string | Аргументы команды. |
| `ctx.speech` | string | Текст речи для speech-триггеров. |
| `ctx.direction` | string | Направление для move/door-событий. |
| `ctx.damage_amount` | number | Величина урона для damage-событий. |
| `ctx.damage_type` | string | Тип урона строкой. |
| `ctx.where` | number | Дополнительное числовое поле события. |
| `ctx.time` | number | Дополнительное поле времени. |
| `ctx.time_day` | number | Дополнительное поле времени суток. |

Не все поля заполнены во всех типах триггеров. Всегда проверяйте `nil`.

## Глобальный объект `mud`

### Логирование и случайность

| Вызов | Возврат | Описание |
| --- | --- | --- |
| `mud.log(message)` | bool | Пишет сообщение в script log с данными Lua-триггера. |
| `mud.random(limit)` | number | Случайное число от 1 до `limit`; при ошибке возвращает 0. |
| `mud.roll(count, sides)` | number | Бросок `count` кубиков по `sides` граней. Ограничения: до 1000 кубиков и до 1000000 граней. |
| `mud.percent(chance)` | bool | Проверка процента от 1 до 100. |

### Поиск и счетчики

| Вызов | Возврат | Описание |
| --- | --- | --- |
| `mud.char_by_uid(uid)` | Char или nil | Находит персонажа по UID. |
| `mud.obj_by_id(id)` | Obj или nil | Находит объект по runtime id. |
| `mud.find_mob(vnum)` | Char или nil | Находит первого live-моба по VNUM. Lua-замена `calcuid ... mob`. |
| `mud.find_obj(vnum)` | Obj или nil | Находит первый live-объект по VNUM. Lua-замена `calcuid ... obj`. |
| `mud.mob_count(vnum)` | number | Сколько мобов данного VNUM сейчас в мире. |
| `mud.obj_count(vnum)` | number | Сколько объектов данного VNUM сейчас в мире. |
| `mud.world.cur_obj_count(vnum)` | number | DG-семантика `%world.curobjs%`: текущий count объекта, но для stable/unlimited timer объектов возвращает 0. |
| `mud.world.game_obj_count(vnum)` | number | DG-семантика `%world.gameobs%`/`%world.gameobjs%`: считает игровые экземпляры, для обычных takeable не-quest предметов учитывает parent-прототип. |
| `mud.world.max_obj_count(vnum)` | number | DG-семантика `%world.maxobj%`: MIW объекта, для unlimited возвращает `9999999`. |

### Комнаты, зоны, время и погода

| Вызов | Возврат | Описание |
| --- | --- | --- |
| `mud.room(vnum)` | Room или nil | Комната по VNUM. |
| `mud.zone(vnum)` | table или nil | Зона по VNUM. |
| `mud.time()` | table | `{ hour, day, month, year }`. |
| `mud.weather()` | table | Данные погоды. |

Поля `mud.zone(vnum)`: `vnum`, `name`, `top`, `age`, `lifespan`, `reset_mode`, `used`, `locked`, `under_construction`.

Поля `mud.weather()`: `temperature`, `pressure`, `change`, `sky`, `sunlight`, `moon_day`, `season`, `weather_type`, `rainlevel`, `snowlevel`, `icelevel`.

### Загрузка сущностей

| Вызов | Возврат | Описание |
| --- | --- | --- |
| `mud.load_obj(vnum, room)` | Obj или nil | Создает предмет и кладет его в комнату. `room` может быть VNUM комнаты или Room-объектом. |
| `mud.load_obj_to_char(vnum, char)` | Obj или nil | Создает предмет и кладет его в инвентарь персонажа. Запускает load object trigger. |
| `mud.load_mob(vnum, room)` | Char или nil | Создает моба в комнате. `room` может быть VNUM комнаты или Room-объектом. |

### Действия

| Вызов | Возврат | Описание |
| --- | --- | --- |
| `mud.purge(entity)` | bool | Удаляет сущность через ее метод `purge`. PC удалить нельзя. |
| `mud.damage(victim, amount, type)` | bool | Наносит урон. `type`: `nil`, `"physic"`, `"magic"`, `"poisonous"`. |
| `mud.cast_spell(spell_name, target)` | bool | Скриптовый каст заклинания от имени владельца триггера; ближайший DG-аналог - `dgcast`. `target`: `nil`, строка поиска, Char, Obj или Room. |
| `mud.transfer(entity, room)` | bool | Перемещает Char или Obj в комнату. |
| `mud.force(char, command)` | bool | Заставляет персонажа выполнить команду. Цель должна быть в комнате владельца триггера. |
| `mud.echo(message)` | bool | Сообщение в комнату владельца триггера. |
| `mud.wait(...)` | yield | Приостанавливает Lua-триггер. |

`mud.force` запрещает внутренние mob script команды (`mload`, `mecho` и т.п.) и пропускает строку через обычный игровой command interpreter.

`mud.damage(victim, amount, nil)` работает как DG `mdamage` без третьего аргумента: просто снимает HP, показывает damage-сообщение и не начинает бой. Если указать тип урона (`"physic"`, `"magic"`, `"poisonous"`), Lua использует боевой `Damage::Process`, как DG `mdamage ... physic/magic/poisonous`; такой урон может запустить бой.

```lua
mud.damage(ctx.actor, mud.random(50), nil)       -- без боя, DG-style mdamage
mud.damage(ctx.actor, 20, "magic")              -- typed damage через Damage::Process
```

`mud.cast_spell(spell_name, target)` возвращает `true`, если игровая `CallMagic`-логика дала эффект. Ближайший DG-аналог - `dgcast`, но это самостоятельный Lua API, а не точная обертка: строковая цель ищется как в обычном cast-поиске, а direct-цели Char/Obj/Room проверяются по target-флагам заклинания и расположению цели. Direct Obj-цель должна быть в инвентаре, экипировке или комнате кастера; для `locate object` передавайте строковую цель. Для room/object-триггеров без персонажа-кастера Lua создает временного системного кастера в комнате владельца.

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

## Char

Char - это Lua-view персонажа или моба. Все поля read-only.

### Поля

| Поле | Тип | Описание |
| --- | --- | --- |
| `ch.name` | string | Имя персонажа. |
| `ch.uid` | number | UID персонажа. |
| `ch.vnum` | number | VNUM моба; для PC возвращает 0. |
| `ch.hp` | number | Текущие хиты. |
| `ch.max_hp` | number | Максимальные хиты. |
| `ch.mana` | number | Текущее значение mana/mem queue. |
| `ch.move` | number | Очки движения. |
| `ch.room` | Room или nil | Текущая комната. |
| `ch.room_vnum` | number | VNUM текущей комнаты или 0. |
| `ch.is_npc` | bool | `true` для NPC. Это поле, не функция. |
| `ch.class` | number | Числовой ID класса, как DG `%actor.class%`. |
| `ch.iname` | string | Именительный падеж имени, как DG `%actor.iname%`. |
| `ch.g` | string | DG-окончание `%actor.g%`. |
| `ch.u` | string | DG-окончание `%actor.u%`. |
| `ch.context` | Context или nil | Переменные DG-контекста владельца. |

### Методы

| Метод | Возврат | Описание |
| --- | --- | --- |
| `ch:is_valid()` | bool | Проверяет, что персонаж еще существует и не purged. |
| `ch:is_pc()` | bool | `true` для игрока. |
| `ch:is_immortal()` | bool | `true` для имма. |
| `ch:level()` | number | Реальный уровень. |
| `ch:position()` | number | Текущая позиция числом. |
| `ch:has_affect(name)` | bool | Проверяет affect по строковому имени enum. |
| `ch:can_see(target)` | bool | Видит ли `ch` другого персонажа. |
| `ch:enemy()` | Char или nil | Текущий противник. |
| `ch:teleport(room)` | bool | Перемещает персонажа в комнату. |
| `ch:equipment(pos)` | Obj или nil | Предмет в слоте экипировки `pos`, как DG `%actor.eq(pos)%`. |
| `ch:eq(pos)` | Obj или nil | Короткий алиас для `ch:equipment(pos)`. |
| `ch:lag(value)` | bool | Ставит battle lag, как DG `%actor.lag(value)%`. |
| `ch:lag(10, "p")` | bool | Ставит wait state в пульсах, эквивалент DG `%actor.lag(10p)%`. |
| `ch:skill(skill_id)` | number | Возвращает значение skill. |
| `ch:skill(skill_id, value)` | number | Устанавливает skill и возвращает значение. |
| `ch:feat(feat_id)` | bool | Проверяет feat. |
| `ch:feat(feat_id, true/false)` | bool | Устанавливает или снимает feat. |
| `ch:attach_trigger(vnum)` | bool | Прикрепляет mob trigger; работает только для NPC. |
| `ch:detach_trigger(vnum)` | bool | Снимает trigger; работает только для NPC. |
| `ch:send(message)` | bool | Отправляет сообщение персонажу с descriptor. |
| `ch:force(command)` | bool | Заставляет персонажа выполнить команду. |
| `ch:reward_daily_quest(id)` | bool | Запускает выдачу награды daily quest по id; Lua-аналог DG `%actor.questbodrich(id)%`. Если `ch` - charmice, награда оформляется на его мастера. |
| `ch:act(message, options)` | bool | Вызывает игровой `act`. |
| `ch:purge()` | bool | Удаляет NPC. PC и текущего владельца триггера удалить нельзя. |

Пример `act`:

```lua
ctx.owner:act("$n смотрит на $N3.", {
  victim = ctx.actor,
  to = "room",
  hide_invisible = true
})
```

`options.to`: `"room"`, `"char"`, `"victim"`, `"notvictim"`.

## Obj

Obj - Lua-view объекта. Все поля read-only.

### Поля

| Поле | Тип | Описание |
| --- | --- | --- |
| `obj.name` | string | Short description объекта. |
| `obj.id` | number | Runtime id объекта. |
| `obj.vnum` | number | VNUM объекта. |
| `obj.room_vnum` | number | VNUM комнаты, если объект лежит в комнате; иначе 0. |
| `obj.room` | Room или nil | Комната объекта, если он лежит в комнате. |
| `obj.carried_by` | Char или nil | Кто несет объект. |
| `obj.worn_by` | Char или nil | На ком надет объект. |
| `obj.container` | Obj или nil | Контейнер, в котором лежит объект. |
| `obj.timer` | number | Таймер объекта. |

### Методы

| Метод | Возврат | Описание |
| --- | --- | --- |
| `obj:is_valid()` | bool | Проверяет, что объект еще существует и не extracted. |
| `obj:purge()` | bool | Удаляет объект из мира. |
| `obj:move_to_room(room)` | bool | Перемещает объект в комнату. |
| `obj:give_to(char)` | bool | Тихо кладет объект в инвентарь персонажа. Не показывает `give`-сообщения. |
| `obj:attach_trigger(vnum)` | bool | Прикрепляет object trigger. |
| `obj:detach_trigger(vnum)` | bool | Снимает object trigger. |

Важно: `obj:give_to(char)` не равен игровой команде `give`. Если нужны обычные сообщения и give/receive-триггеры, загрузите объект в инвентарь моба и используйте `mud.force(mob, "give ...")`.

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
| `room:people()` | table | Список Char в комнате, индексация с 1. |
| `room:objects()` | table | Список Obj в комнате, индексация с 1. |
| `room:exit(direction)` | Room или nil | Комната по направлению. `direction` может быть числом или строкой. |
| `room:set_exit(direction, options)` | bool | Создает или изменяет выход. `options`: `flags`, `to_room`, `description`, `key`, `name`, `lock`. `flags` задаются как в DG `wdoor ... flags`. `to_room` можно передать числом-VNUM или Lua-объектом `Room`; если целевая комната не существует, метод вернет `false`. |
| `room:purge_exit(direction)` | bool | Удаляет выход в направлении, как DG `wdoor ... purge`. |
| `room:load_obj(vnum)` | Obj или nil | Создает объект в комнате. |
| `room:load_mob(vnum)` | Char или nil | Создает моба в комнате. |
| `room:attach_trigger(vnum)` | bool | Прикрепляет room trigger. |
| `room:detach_trigger(vnum)` | bool | Снимает room trigger. |

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

- Проверяйте `nil` и `:is_valid()` перед использованием `ctx.actor`, `ctx.victim`, `ctx.object`.
- После `mud.wait` проверки нужно повторить.
- Для команд через `force` предпочитайте ASCII-команды (`give`, `say`, `look`) и ASCII-служебные слова (`all`), если нет явной необходимости в русском alias.
- `mud.load_obj_to_char` кладет объект в инвентарь без видимых сообщений. Чтобы игроки увидели передачу, используйте затем обычную команду через `force`.
- `mud.damage(victim, amount, nil)` повторяет DG `mdamage` без типа и не начинает бой.
- `mud.damage(victim, amount, "physic"|"magic"|"poisonous")` атакует от имени владельца триггера, если он валиден; иначе от имени самой жертвы. Атакующий и жертва должны быть в одной комнате; такой урон может начать бой.
- `mud.transfer` вызывает `teleport` для Char и `move_to_room` для Obj.
- Lua-view read-only: нельзя писать `ctx.actor.hp = 10` или `obj.timer = 0`.
