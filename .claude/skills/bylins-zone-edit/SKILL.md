---
name: bylins-zone-edit
description: Edit Bylins MUD zones online on the live server via the admin web panel (port 12001) — read/create/update rooms, mobs, objects, loot drops, zone resets. Use when the user (a builder) asks to edit, create, or fix room descriptions, mob flags/loot, items, or zone content on the running game, not in the git world files.
---

# Bylins MUD — online zone editing via the admin panel

The live world is edited through the **web admin panel** (`tools/web_admin/`), which proxies JSON commands to the running server's Admin API socket. Edit the live game here, NOT the git `lib/world` files. Always read a record before changing it, and **read it back to verify** after every write.

## 1. Connect & authenticate

- Panel: `http://127.0.0.1:12001` (Flask/gunicorn in the `mud-web-admin` docker container).
- Login: `POST /login`, form fields `username` + `password` (UTF-8). Auth goes straight to the mud.
  - My god account is **Дедал** (ASCII password — ask the user for it; never store it).
  - **Do not brute-force credentials** — failed logins are logged for fail2ban.
- Use a cookie jar (`curl -c/-b`) to keep the session across calls.

## 2. Read (raw JSON, preferred over scraping HTML)

`GET /api/rooms/<vnum>`, `/api/mobs/<vnum>`, `/api/objects/<vnum>`, `/api/triggers/<vnum>`, `/api/zones/<vnum>` → `{"room"|"mob"|...: {...}}`.
(`/api/search/...` only returns vnum+name. The HTML `/room/<vnum>` pages hide empty fields.)

## 3. Write

- Update: `POST /room|mob|object|trigger/<vnum>/update` with a JSON body of just the fields to change.
- Create: `POST /api/zones/<zone>/objects|mobs|rooms|triggers` (include `vnum` to pin it, else first free in zone).
- Zone resets: `POST /api/zones/<zone>/commands` (e.g. `{"command":"M","mob_vnum":...,"room_vnum":...,"max_in_world":1,"max_in_room":1}`), then `POST /api/zones/<zone>/reset`.
- **Encoding:** send UTF-8; the server converts to KOI8-R on write and returns UTF-8 on read. Don't run reads through iconv.
- **Verify:** after each write, GET the record back and diff.

## 4. Field reference

**Rooms** — `description` is one string. Optional seasonal/day-night variants via inline tags processed in `sight.cpp`:
- Base text = everything before the first `<` (always shown; fallback for seasons with no block).
- Block wrapped by the same tag twice: `<winterday>…snow text…<winterday>`. `R` right after the open tag (`<winterday>R…`) **replaces** the base instead of appending.
- Tags: `<winterday> <winternight> <springday> <springnight> <summerday> <summernight> <autumnday> <autumnnight>`, generic `<day> <night>`. First match by season+time wins. See zones 722 (#72218), 340, 1199 for examples.
- **Paragraph red-line indent = three underscores `___`, NOT spaces.** The engine (`color.cpp`) renders leading `_` as spaces; leading literal spaces get stripped on load. So start the first line of each paragraph (base text and every seasonal block) with `___`, e.g. `<winterday>R___Колкий снег…<winterday>`. **Wrap body to 130 cols — Stribog's constant for ALL descriptions (rooms/redit, mobs/medit, objects/oedit, incl. L-Des & examine).**
- **Use `R` (replace), not append, for seasonal variants.** `<winterday>R…<winterday>` swaps the whole base for one self-contained paragraph; ~99% of the world does this. Append (tag without `R`) tacks the block on as a *second* paragraph after the base — looks ugly and is almost never what you want. To add a seasonal sentence to existing text, build the block as `R` + (base text + the seasonal sentence) so it reads as a single flowing paragraph.

**Mobs**
- Flags are **numbers**, encoding `(plane<<30)|(1<<bit)` = the literal `EMobFlag` value (`kIntOne = 1<<30`). Send via `{"flags":{"mob_flags":[<ints>]}}`. `update_mob` is **additive** for flags (can't unset — use medit for that). Useful: `!ходит`/kSentinel=2; seasonal appear flags kAppearsWinter=1073742848, Spring=1073743872, Summer=1073745920, Autumn=1073750016 (mob loads always, but hides in the zone's auto-created room 99 out of season, returns to its reset room in season).
- `death_load` (loot on death) is an **array, replaced wholesale** on update — send existing + new: `[{"obj_vnum":N,"load_prob":0-100,"load_type":0,"spec_param":0}, …]`.
- Name has 6 grammatical cases under `names` (nominative…prepositional). **Descriptions field mapping is non-obvious** (API ≠ engine naming): `descriptions.short_desc` → the **in-room line** (`long_descr`, printed verbatim); `descriptions.long_desc` → the **look-at/examine** text.
- `stats`: `level`, `sex`, `armor`, `race`, `alignment`, `hp:{dice_count,dice_size,bonus}`, `damage:{…}`. `position:{default_position,load_position}` (8 = standing).
- **MOB-AS-ITEM trick** (a mob that looks like an object/item to players — talking statue, sacred tree, idol, sign): (1) set `stats.race` = **109** (`kConstruct`, displayed «Предмет» — the item NPC type; `ENpcRace` is 100+, so race = 100 + `npc_race_types[]` index); (2) set `descriptions.short_desc` (the long_descr line) to an item-like sentence — the mob then renders among "things" (`list_char_to_char_thing`, sight.cpp:1677) instead of the red people-list, **but only while position == default_position** (just standing); the red list explicitly excludes race==kConstruct; (3) make it `kSentinel`+`kNoFight`, non-aggressive, big HP; (4) put the room on **`kPeaceful`/«мирная»** (room_flags plane0 bit4 = 16) so it can't be attacked. It then catches speech via MTRIG_SPEECH — **this is the only way to make a speech-reacting "item"** (objects can't do speech).

**Objects**
- `type`: 8 = treasure/valuable (sells to shop, no stats); 20 = money (`type_specific.value0` = amount of куны, `value1`=0 for kuna); 5 weapon, 9 armor, etc.
- `material`: 6 = precious metal (gold/silver), 15 = skin/leather, 14 = cloth, 13 = bone, 8 = wood… (no separate gold vs silver).
- `sex` (grammatical gender): 0 neuter, 1 masculine, 2 feminine.
- `wear_flags`: bitmask, `1` = TAKE (carryable). `affects: []` = no stat bonuses.
- Set `names` (6 cases), `aliases`, `short_desc`, `description` (on ground = one short line), `extra_descriptions:[{keywords,description}]` (on examine = elaborate text + any hints), `cost`, `weight`, `timer`.
- **Scenery objects left on the ground** (non-takeable `wear_flags:0`) must have extra-flag **kNodecay** (`extra_flags:[8192,0,0,0]` — plane0 bit13) AND `timer = 2147483647` (`UNLIMITED_TIMER`), or they vanish in ~10 min / warn "нулевой таймер". (For a talking/reacting scenery 'item' use a **mob-object** instead — see §4 Mobs.)

## 5. Description style (Stribog's rules)

- No direction-of-travel wording ("шли", "за спиной", "выходим из чащи к городу") — only static cardinal landmarks. Terrain transitions ("ельник сменяется сосняком") are fine.
- No semicolons — periods, commas, dashes only.
- Opener must not be a redundant label of the room name ("Околица бора" → "Окраина бора:" is bad).
- On a through-road, mention the road/обочина in every room so players can't think they're lost; anchor side spurs with "в стороне от дороги".
- Geographic consistency (don't call a city gate a "застава"; landmarks must match exits).
- Keep place-features even if they read cold ("незамерзающий ручей", "не стынет в мороз") — those are lore, not season.

## 6. Triggers & DG scripting

- Attach via `update_mob`/`update_object` `{"triggers":[<vnum>,…]}` (whole list, replaces). Trigger record: `trigger_type` (bitvector), `attach_type` (0=mob,1=obj,2=room), `narg`, `arglist`, `script`.
- **Newly attached triggers do NOT apply to an already-spawned mob/obj** — triggers are copied at spawn. After attaching + zone reset, the live instance keeps its old list (`stat <mob>` → "Script information: None"). Repop it: kill/`purge` the instance, then reset the zone (or reboot). Editing the *body* of an existing trigger vnum DOES take effect live (the prototype is shared).
- **Numeric random exists:** `%random.N%` → `number(1,N)`; `%number.range(x,y)%` → `number(x,y)`. Use it to vary mob speech/reactions. `%time.hour%` banding (`if %h% < N`) is a fine alternative for time-of-day flavor.
- **Mobs can run socials inside triggers** (`emote` always works; verified: кланяться, плакать, креститься, вздыхать, причитать, оскал, рычать…). Use `say` / `emote` / `%echo%` (room ambiance) freely.
- **Quests:** `%actor.quested(N)%` reads a persistent, saved per-player "quest done" map; set it with `eval tmp %actor.setquest(N)%`, clear with `unsetquest`. Mark a quest done at BOTH accept and completion so any path counts. **GOTCHA: `setquest`/`quested` is a no-op for immortals** (`Quested::add` skips `IsImmortal()`, level ≥ 31) — quest-gated logic can't be tested on a god; use a mortal alt.
- **The script body goes in the `script` field as ONE newline-joined string** — NOT a `commands` array (an array is ignored → default placeholder). `attach_type`: 0=mob, 1=obj, 2=room.
- **Trigger types & matching:** `trigger_type` bits — SPEECH=8, COMMAND=4, mob GREET_PC=65536, room ENTER=1<<6, RESET=1<<5. **SPEECH (`сказать`) is ONLY on WTRIG (room) / MTRIG (mob) — never OTRIG (object); so a speech-reacting "item" = a room trigger or a mob-object (§4).** SPEECH `narg`: 0=word_check (word in the phrase), 1=substring, else exact; `arglist`=the word. COMMAND `narg` = where the obj must be (OCMD_ROOM=4). GREET/RANDOM `narg` = % chance.
- **Send/echo** (prefix m=mob / o=obj / w=room): `msend %actor% text` (one char), `mecho text` (room). **Religion:** `%actor.religion%` → 0 = язычник (`kReligionPoly`), 1 = христианин (`kReligionMono`). **Self-detach:** `detach <trig_vnum> %self.id%` (2nd arg = UID, not a bare vnum). **Self-purge mob:** `mpurge <keyword>` (needs an arg; no-arg = no-op).
- **`dgaffect <target> <property> <spell> <value> <duration>`** (verified order) — apply an affect. Property from `apply_types` (`сила`=STR, `ловкость`=DEX…). **`<spell>` MUST be a valid spell** (from `lib/cfg/spells.xml` `<name rus=…>`, e.g. `сила`, `благословение`) or the affect is **silently not applied** (despite a "ставим чары" log). **Real seconds = `duration × 120`** → 1 hour RL = `duration=30` (the affect line then shows "~30 часов" — that display unit ≈ 2 real min each, so 30 ≈ 1h). E.g. `dgaffect %actor% сила сила 20 30` = STR+20 for 1h.
- **Once-per-repop (anti-farm):** the rite script **`detach`es its own trigger(s)** so a *visible* scenery mob stays put but goes dormant — don't `mpurge` a visible mob (it vanishes). Pair zone **`Q` (remove mob) + `M` (load)** so each repop purges the dormant instance and reloads a fresh one with triggers (and propagates proto edits — a reset does NOT auto-purge a living sentinel mob). For object scenery use **`R` (remove) + `O` (load)**. Worked example: zone-738 «Священный дуб» (mob 73810 race=109, GREET_PC for pagans + MTRIG_SPEECH «требу» → STR+20/1h then detach, Q+M each repop, room kPeaceful).

## 7. Seasons & time (for testing seasonal descriptions)

- Season is derived from the game **month** and is purely clock-driven — **there is no immortal command to change it.** Month is computed from `kBeginningOfTime` at boot, then ticks on its own.
- Rates: 1 MUD month = **24 real hours**, 1 MUD year = 12 real days (`kTimeKoeff=2`). Winter = Dec/Jan/Feb (Mar & Nov are transitional — season holds its previous value).
- To see a `<winter…>` description right now, restart the server with the host/container clock shifted so boot lands in a winter month (waiting for the cycle is impractical).

## 8. Zone reset commands

`POST /api/zones/<zone>/commands` with `command` = one of:
- `M` (load mob): `mob_vnum, room_vnum, max_in_world, max_in_room`.
- `Q` (remove/purge mob by vnum): `mob_vnum`. Put **Q before M** to refresh a mob each reset (mob analogue of `R`+`O`).
- `O` (load obj in room): `obj_vnum, room_vnum, max_in_world, load_percent`.
- `R` (remove obj from room): `room_vnum, obj_vnum`.
- `G`/`E` (give/equip to last mob), `P` (put in container), `D` (door state).
Commands run in list order; new ones append. **Delete by index** `DELETE /api/zones/<zone>/commands/<index>` — delete the HIGHEST index first (deleting shifts later indices down).

## Related memory
`reference_web_admin_panel`, `reference_admin_api_mob_flags`, `feedback_room_descriptions`, `reference_seasonal_room_descriptions`.
