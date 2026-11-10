---
name: bylins-world-convert
description: Take a zone sent in the old (legacy/BZEditor) format into the YAML world and plug it in — what the Python converter silently gets wrong, which object fields moved out of val[] into ObjVal keys, which flags and reset commands were retired, and how to verify the result. Use when a builder sends a .zon/.mob/.obj/.wld/.trg archive, when a converted zone logs SYSERRs on boot, or when something about the YAML world format after the switch needs checking.
---

# Bylins MUD — bringing an old-format zone into the YAML world

The world is YAML (`lib/worlddata/world/zones/<vnum>/*.yaml`, a separate git repo). Zones still
arrive from builders as legacy archives — BZEditor writes `.zon .mob .obj .wld .trg` plus `.map`
and `.skt`, in KOI8-R. The on-disk format has moved on since those files were written, and
`tools/converter/convert_to_yaml.py` has NOT followed it. It does not fail — it drops what it does
not know, so **the converter's output is a draft, never the result.**

**One rule above all the rest: the engine is the only canonical writer.** Convert, load, let the
engine rewrite. Everything below exists because something gets lost between those two steps.

## 1. The pipeline

```bash
# 1. the archive is untrusted data -- its own empty directory, nothing run from inside it
mkdir -p <scratch>/x889 && cd <scratch>/x889 && unzip/7z x <archive>

# 2. a minimal legacy tree: world/{mob,obj,wld,zon,trg}/<N>.<ext> + an `index` per dir
#    index format: one "<N>.<ext>" line, then "$", "$", blank
python3 tools/converter/convert_to_yaml.py -i <scratch>/legacy -o <scratch>/out -f yaml -t all

# 3. the converter still WRITES KOI8-R; the world is UTF-8 -- recode its output
for f in zone rooms mobs objects triggers; do
  iconv -f KOI8-R -t UTF-8 <scratch>/out/world/zones/<N>/$f.yaml > <world>/zones/<N>/$f.yaml
done

# 4. fix what §3-§5 list, then register the zone in zones/index.yaml
# 5. verify: ./check_yaml.pl zones/<N> && boot && grep the syslog (§7)
```

`-t all` needs all five dirs to exist, but an empty `index` is fine — that is how a zone with a
missing `.wld` still converts for inspection.

Do NOT use the world repo's `plug_zones.pl`. It installs into `lib/world/{mob,obj,wld,zon,trg}/` +
`index` files, a layout that no longer exists. Shops likewise no longer come from `world/shp` —
they live in `cfg/economics/shops.xml`.

## 2. Layout and what makes a zone exist

- **Flat layout** (the default, `world_config.yaml` records it): one `<sub>.yaml` per zone, a map of
  `vnum % 100` -> entity. The engine enumerates the file's keys, so the file is its own index:
  `task.vnums.push_back(zone_vnum * 100 + rel_num)` in `DiscoverEntityFiles()`.
  The per-file layout (`zones/<v>/<sub>/index.yaml` + one file per entity) is still read but legacy.
- **`zones/index.yaml` is the enable list.** A zone directory that is not listed there is not
  loaded at all. Insert the number in ascending order.
- **Unknown keys are ignored** — there is no schema validation. The converter emits a bogus
  `zone: <first room vnum>` on every room; harmless, and gone after the first engine save.
- **`zone.yaml` defaults** (`ParseZoneNode`): `top_room` -> `vnum*100+99`, `mode` = level,
  `zone_group` -> 1, `entrance` -> 0, `under_construction` -> 0, i.e. **a zone with no
  `under_construction` line is live.** `vnum:` is not read (the directory name decides) but every
  hand-written zone carries it.
- Flag dictionaries live in `dictionaries/*.yaml` next to `zones/`.

## 3. What the Python converter gets wrong (verified, Nov 2026)

1. **Reset command `P`, second argument.** It used to be "max in world"; it is now
   **the room the container stands in, or `0` = find it anywhere in the world**
   (`'P' <flag> <ObjVnum> <room or 0> <target_vnum> <load%>`, `db.cpp`). The converter copies the
   old number through. Symptom: `SYSERR: zone file <N>.zon: room in arg2 not found, command
   omited`, or no error at all and a silently empty container when the old number happens to be a
   real room vnum. The whole world uses `0`.
2. **A `L` line in a `.zon` is dropped without a word.** The engine's reset commands are exactly
   **M O E G P D R Q F T V** (see the `zedit` menu) — there is no `L`. BZEditor writes
   `L <obj_vnum> <prob> 0 0` right after an `M` line to mean loot-on-death, and that is now a field
   on the mob, not a reset command: `dead_load: [{obj_vnum, load_prob, load_type, spec_param}]`
   (legacy `.mob` files carried it as a `D` line; `dead_load::ParseDeadLoadLine`). Grep the incoming
   `.zon` for `^L ` and move every hit onto the mob of the preceding `M` line, or the zone loses all
   its drops. **But `dead_load` is off limits for new content — among the builders loot is handed
   out from a death trigger with `load obj`.** For a converted zone, write what the `L` lines said
   into the commit message or the zone's README and let the author place it in triggers; don't
   invent `dead_load` entries. Equally: a zone with no `dead_load` at all is the norm, so never
   read that as missing loot.
3. **Retired equipment-affect bits.** `EEquipmentAffect` bits 11-12 (`kProtectFromDark`,
   `kProtectFromMind`) are gone for good — they were never on/off affects but resist parameters.
   Symptom: `SYSERR: unknown EEquipmentAffect 'kProtectFromMind' on obj vnum N, skipped`. Grant the
   resist through the item's `EApply` instead (`kResistDark` / `kResistMind`); the number is a
   balance decision, so it belongs to the zone's author. **Mobs keep these flags** — their
   `affect_flags` are `EAffect`, a different enum, and bits 13/14 there are alive. Only strip them
   from objects.
4. **Trigger type names are the old unprefixed ones** (`kSpeech`, `kAct`, `kIncome`…). Harmless:
   bit numbers are shared across mob/obj/room and `dictionaries/trigger_types.yaml` keeps a
   "старые имена" read-alias block for exactly this. The engine writes the prefixed names
   (`kMobSpeech`, `kWldEnterPc`, `kObjPick`) on the first save. Watch for
   `SYSERR: trigger N: unknown trigger type '<name>' -- the type is lost` — that means the alias is
   missing and the bit really was dropped.
5. **Encoding.** Converter output is KOI8-R (so is `tools/convert_magic_items.py`); the world is
   UTF-8 on disk and in git. Recode the output, never the world.

## 4. Object fields that moved out of val[] into ObjVal keys

This is the part that bites hardest, because `values:` in the file still looks right. For
**scroll, wand, staff, potion, drink container and fountain** the payload now lives in named keys
and the YAML writer **skips `val[0..3]` entirely** for those six types
(`hotfix_extended` in `yaml_world_data_source.cpp`). On-disk key names come from
`text_id::InitObjVals()`; the old `POTION_*`/`SPELLITEM_*` spellings stay as read-aliases.

| type | legacy val[] | key today |
|---|---|---|
| kScroll | val1..val3 = spells | `SPELL1_NUM`, `SPELL2_NUM`, `SPELL3_NUM` |
| kWand, kStaff | val1 = max charges, val2 = current, val3 = spell | `MAX_CHARGES`, `CUR_CHARGES`, `SPELL1_NUM` |
| kPotion | val1..val3 = spells | `SPELL1_NUM`, `SPELL2_NUM`, `SPELL3_NUM` (+ `POTION_PROTO_VNUM`, `POTION_POTENCY`, `POTION_BREW_ROLL` on instances) |
| kLiquidContainer, kFountain | val0, val1, val2 | `LIQUID_CAPACITY`, `LIQUID_CURRENT`, `LIQUID_TYPE` |
| drink/fountain/food | val3 == 1 poisoned, val3 > 1 freshness countdown | `LIQUID_POISON` / `LIQUID_TIMER` |

Shared across all of them: `MAKER_SKILL`, `MAKER_STAT` (left absent = the authored-maker default).
Retired, read-only for old saves: `POTION_SPELL1_LVL` and friends.
`ObjVal::remove_incorrect_keys(type)` drops any key that does not belong to the type, so a key on
the wrong type is lost silently — getting `type:` right comes first.

**The engine migrates legacy `values:` for these types by itself at boot**, which is why a
converted zone is not as broken as it looks. `ConvertObjValues()` (`db.cpp`, after all prototypes
are read) runs `ConvertDrinkconSkillField`, `ConvertDrinkPoisonField`, `ConvertPotionToEValueKey`,
`ConvertSpellItemToEValueKey`, `ConvertDrinkconLiquidCore` over every prototype, all idempotent,
and queues each touched zone with `olc_add_to_save_list(zone, OLC_SAVE_OBJ)` — the boot log says
`Converted obj values, zones queued for save: …`.

Two consequences worth knowing:

- **The queue is flushed only on a clean shutdown**, and `reboot 2` skips it (`comm.cpp`). So the
  migration persists after a normal stop or an explicit `oedit save all` — not after a crash, and
  not after reboot-2. If the same zones appear in that log line every boot, the migration is not
  persisting (that was issue #3749).
- **The liquid core needs no migration at all**: for `kLiquidContainer`/`kFountain`,
  `get_val`/`set_val` transparently redirect indices 0..2 to the keys
  (`uses_liquid_core()` in `obj_data.h`), and the loader sets `type` before `values`, so plain
  `values: [999, 999, 15, 0]` lands in the keys on read. `LIQUID_TYPE` indexes `drinks[]`
  (0 = воды, 15 = родниковой воды, …, `NUM_LIQ_TYPES` = 29).

`tools/convert_magic_items.py` does the scroll/wand/staff half of this offline, on a YAML world —
useful only if you cannot boot, and remember it reads and writes KOI8-R.

## 5. Checking the author's own data while you are there

A 2024 zone was tested against a 2024 engine, so some of what breaks is the author's, not the
converter's. Both kinds show up in the same boot log, so separate them before writing back:

- A reset `DOOR` command needs an exit to hang on. In zone 889 it closed south out of «В избушке»
  #88902, where the room has no exits at all — the way out is created by a speech trigger
  (`wdoor 88902 south room 88903`) and removed again by the leave trigger. `SYSERR: door does not
  exist, command disabled`; the command simply has to go.
- Cross-check the room list in `.map` (one `vnum x y z` line per room) against the `.wld`. In zone
  881 all 32 rooms were in the map and everything referenced them, but `881.wld` was not in the
  archive at all — the zone cannot be plugged in until the builder sends it. The `.skt` (BZEditor
  sketch) and `.map` are editor files; nothing in the engine reads them.
- Object types nobody uses are not an error to fix silently: `kWorm` (11) is "Unimplemented", and
  builders do set it on rings and gloves. Report it, let the author decide.

## 6. Plugging the zone in

- Add the number to `zones/index.yaml` (ascending).
- `entrance: <first room>` and **`under_construction: 1`** until somebody has walked the zone.
  Under construction means: no death penalty, no daily quests, no set drops (`fight_stuff.cpp`,
  `daily_quest.cpp`, `sets_drop.cpp`). Flip it to 0 when the zone is accepted, exactly as zone 780
  was.
- **A new zone has no way in.** Its entrance room has no exit back out to the world, and nothing in
  the world points at it, so the zone is inert until the map is cut open: an exit from a
  neighbouring room plus the matching exit back. Where it attaches is the world owner's call
  (Ярило) — ask, do not pick.
- Vnums: zone `N` owns `N*100 .. N*100+99` for every entity type. Check the number is free in
  `zones/` and that no other zone's `top_room` swallows the range (the bottom of a zone is the
  previous zone's top + 1, so the boot log's `Resetting <name> (rooms A-B)` is the real answer).

## 7. Verifying

```bash
cd lib/worlddata/world && ./check_yaml.pl zones/<N>      # indentation, tabs, dupes, encoding
cd build && ./circle -d <world_dir> <port>               # a real boot is the only honest check
```

`check_yaml.pl` catches what keeps the world from parsing and gives true line numbers (a yaml-cpp
message points at where the parser gave up, not at the damage). It says nothing about semantics —
only a boot does.

The syslog is `<build_dir>/syslog` (not `log/syslog`), and it is appended to across boots: cut it
at the current date before reading. Then look for the zone's own name
(`Resetting <name> (rooms …)`, `[Reset] Start/Stop zone (N)`) — a clean zone has nothing between
Start and Stop — and grep `SYSERR` for the zone's vnum range. A world snapshot older than the
engine produces its own noise (hundreds of retired-flag lines from other zones); filter by the new
zone's vnums before believing anything.
