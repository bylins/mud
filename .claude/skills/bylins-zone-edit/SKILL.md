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
- **Paragraph red-line indent = three underscores `___`, NOT spaces.** The engine (`color.cpp`) renders leading `_` as spaces; leading literal spaces get stripped on load. So start the first line of each paragraph (base text and every seasonal block) with `___`, e.g. `<winterday>R___Колкий снег…<winterday>`. Wrap body to ~75 cols.
- **Use `R` (replace), not append, for seasonal variants.** `<winterday>R…<winterday>` swaps the whole base for one self-contained paragraph; ~99% of the world does this. Append (tag without `R`) tacks the block on as a *second* paragraph after the base — looks ugly and is almost never what you want. To add a seasonal sentence to existing text, build the block as `R` + (base text + the seasonal sentence) so it reads as a single flowing paragraph.

**Mobs**
- Flags are **numbers**, encoding `(plane<<30)|(1<<bit)` = the literal `EMobFlag` value (`kIntOne = 1<<30`). Send via `{"flags":{"mob_flags":[<ints>]}}`. `update_mob` is **additive** for flags (can't unset — use medit for that). Useful: `!ходит`/kSentinel=2; seasonal appear flags kAppearsWinter=1073742848, Spring=1073743872, Summer=1073745920, Autumn=1073750016 (mob loads always, but hides in the zone's auto-created room 99 out of season, returns to its reset room in season).
- `death_load` (loot on death) is an **array, replaced wholesale** on update — send existing + new: `[{"obj_vnum":N,"load_prob":0-100,"load_type":0,"spec_param":0}, …]`.
- Name has 6 grammatical cases under `names` (nominative…prepositional); descriptions under `descriptions` (long_desc/short_desc).

**Objects**
- `type`: 8 = treasure/valuable (sells to shop, no stats); 20 = money (`type_specific.value0` = amount of куны, `value1`=0 for kuna); 5 weapon, 9 armor, etc.
- `material`: 6 = precious metal (gold/silver), 15 = skin/leather, 14 = cloth, 13 = bone, 8 = wood… (no separate gold vs silver).
- `sex` (grammatical gender): 0 neuter, 1 masculine, 2 feminine.
- `wear_flags`: bitmask, `1` = TAKE (carryable). `affects: []` = no stat bonuses.
- Set `names` (6 cases), `aliases`, `short_desc`, `description` (on ground), `extra_descriptions:[{keywords,description}]` (on examine), `cost`, `weight`, `timer`.

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
- `dgaffect <target> <name> <type> <value> <duration> <location>` is a script-driver builtin (all trigger types) — e.g. charm a mob without casting.

## 7. Seasons & time (for testing seasonal descriptions)

- Season is derived from the game **month** and is purely clock-driven — **there is no immortal command to change it.** Month is computed from `kBeginningOfTime` at boot, then ticks on its own.
- Rates: 1 MUD month = **24 real hours**, 1 MUD year = 12 real days (`kTimeKoeff=2`). Winter = Dec/Jan/Feb (Mar & Nov are transitional — season holds its previous value).
- To see a `<winter…>` description right now, restart the server with the host/container clock shifted so boot lands in a winter month (waiting for the cycle is impractical).

## Related memory
`reference_web_admin_panel`, `reference_admin_api_mob_flags`, `feedback_room_descriptions`, `reference_seasonal_room_descriptions`.
