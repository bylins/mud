# Offline aging of character affect timers — design & implementation plan

Issue: [#3678](https://github.com/bylins/mud/issues/3678) — *"Affect timers freeze in rent while
item timers keep running."*
Branch: `issue.3678-affect-timer` (off `unstable`).

---

## 1. Problem

Two subsystems treat "time passed while the character was offline" in opposite ways:

- **Items age offline.** On rent exit the elapsed real time is deducted from every item timer
  (`src/engine/db/obj_save.cpp:1627`):
  ```cpp
  long timer_dec = time(0) - SAVEINFO(index)->rent.time;
  timer_dec = (timer_dec / kSecsPerMudHour) + (timer_dec % kSecsPerMudHour ? 1 : 0);
  // ...subtracted from each item's timer below
  ```
  The rent file stores **one** absolute timestamp (`rent.time`) for the whole set.

- **Character affects pause offline.** The `Aff3` block saves each affect's `duration` as a
  **relative tick count** with no timestamp (`src/engine/entities/char_player.cpp` ~683); on load
  `af.duration = num2` is restored verbatim and re-imposed via `affect_to_char` (the `Aff3` case).
  Nothing is deducted, so a buff/summon survives an overnight rent unchanged.

The player report is correct: the difference is inconsistent and has no in-fiction justification.

### It is broader than affects

The same "restore relative remaining, no deduction" pattern is used by other character timers, so
they pause too:

- **Cooldowns** (`timed_skill`) — saved as remaining (`char_player.cpp:484`), reloaded as
  `time(0) + num3` (`char_player.cpp:1709`).
- **Feat timers** (`timed_feat`) — reloaded as `time(0) + num3` (`char_player.cpp:1392`).
- **Summons / charmees** — separate saved entities with their own lifetime timers.

If only affects are fixed, cooldowns and summons still pause and reproduce a smaller version of the
same complaint. The plan below fixes the **class**, driven by one timestamp.

---

## 2. Design decision required first (game design, not code)

The issue leaves the *direction* open. This plan implements **Option 1: character timers age
offline** (make affects behave like items). The alternative, **Option 2: freeze item timers during
rent**, is simpler code and softer for players but changes item wear-and-tear balance.

**This must be confirmed with the team before implementation**, because Option 1 changes felt
behaviour: long buffs/summons expire after a night offline. Section 8 lists softening levers if the
raw behaviour is judged too harsh.

The rest of this document assumes Option 1.

---

## 3. Chosen approach: one "last live save" timestamp on the character

Store a single absolute timestamp on the character — the moment its live state was last persisted —
and, on the next login, derive the elapsed offline time from it and age all relative timers. This
mirrors the proven item model (`rent.time`) instead of adding a timestamp to every affect record.

### Why one field, not per-affect timestamps

- **No `Aff3` format change** → no player-file migration; old files load unchanged.
- **Mirrors the item model** the team already maintains (one `rent.time` for all items).
- **Centralizes wall-clock-skew handling** (DST, host migration, clock resets) in one place.
- **Minimal write overhead**, as suggested in the issue.

Trade-off to accept: a single field is only correct if it is written **exactly** on "character left
/ live-state saved" and **never** on an offline file rewrite (see §5). Per-affect absolute
timestamps would be immune to that, but at the cost of a format change and duplicated data. The
single field wins provided the write discipline in §5 is enforced.

---

## 4. New character field

- **Tag:** `SvTm:` (save/leave time), a single `time_t` written as `%ld`. Naming kept 4 chars to
  match existing tags (`LstL`, `Plyd`, ...).
- **Meaning:** the wall-clock time of the last save that reflected the character's *live* state.
- **Storage:** new member on the player data (e.g. `time_t last_state_save_`), with getter/setter,
  serialized in the character save and parsed on load.
- **Do NOT reuse `last_logon`.** It is the *entry* time (`char_data.h:579`, set to `now` on load),
  reset every login — unusable as a "left at" marker.

Backward compatibility: a file with no `SvTm` (pre-upgrade, or a freshly created character) is
treated as "no known offline interval" → **elapsed = 0** (pause, i.e. current behaviour) for that
one login. The field is then written on the next save, so aging begins from the upgrade onward. This
makes the rollout safe and non-retroactive.

---

## 5. Write discipline (the `set file` trap the issue calls out)

`SvTm` must mean "when the player stopped playing", not "when the file was last written".

- **Set `SvTm = time(0)`** on saves that reflect live state: rent, quit, link-death save, and normal
  autosave-while-online. (Autosaving to `now` while online is correct — the player is present, so
  the next real offline interval is measured from the latest live save.)
- **Preserve `SvTm` unchanged** on offline file rewrites — specifically the `set file` admin path
  (`src/engine/ui/cmd_god/do_set.cpp:153`, `is_file`), which loads, edits and re-saves an offline
  character. If that path stamped `now`, an admin edit would zero a player's offline interval.
  Implementation: the offline-edit save must carry through the loaded `SvTm` rather than restamping.

This is the one invariant the whole design rests on; it should be covered by a test (see §9).

---

## 6. Login-time aging

On character load, before the character enters the world, compute once:

```
elapsed_seconds = max(0, time(0) - SvTm)      // clamp negative (clock moved back) to 0
```

Then apply to each saved affect according to the cadence it *would* have ticked at while the
character sat idle out of combat (offline is always out of combat — `GetEnemy()` is null):

- **Normal affects and `kAfBattledec` affects** — both tick in `player_affect_update` at
  `kSecsPerPlayerAffect` (= 2 s), −1 per tick. The `kAfBattledec` skip there is gated on
  `i->GetEnemy()` (`affect_data.cpp:51`), which is false offline, so battle-decay affects age
  **exactly like normal affects**. No special case, no skipping.
  ```
  ticks = elapsed_seconds / kSecsPerPlayerAffect
  duration -= ticks
  ```
- **`kAfPulsedec` affects** — tick in `UpdateAffectOnPulse` at pulse cadence (decrement by pulse
  count). This is the **only** distinct unit. Convert `elapsed_seconds` into the pulse scale used
  there and subtract. Note its player-side ticking path needs verification during implementation:
  the main caller (`mobact.cpp:914`) is NPC-only (`!ch->IsNpc()` → skip), so player pulsedec affects
  may be rare/handled elsewhere; confirm before relying on the conversion.
- **Permanent affects** (`duration == -1`) — never aged (already the invariant everywhere).
- **`kAfFromEquipment` affects** — never saved (re-materialized on equip), so out of scope.

Expiry: any affect whose `duration` reaches `<= 0` after deduction must be **removed through the
normal removal path** so `kExpired`/dispel `<actions>` and apply-cleanup fire — not silently
dropped. Order the load so this happens before the character is placed in the world and before the
first `affect_total`, matching how expired item timers are handled.

### Extend to the rest of the class (same `elapsed_seconds`)

Using the same computed interval:

- **Cooldowns / feat timers** (`timed_skill`, `timed_feat`) — these are reconstructed as
  `time(0) + remaining`. Either subtract `elapsed_seconds` from the reconstructed target, or store
  them as absolute expiry and let the existing `time(0) >= target` check (`player_timed_update`)
  expire them naturally. Absolute-expiry is cleaner and removes the pause entirely.
- **Summons / charmees** — apply the same deduction to their lifetime timers so a summoned/charmed
  follower does not outlive its intended duration across a rent.

---

## 7. Backward / forward compatibility

- `Aff3` on-disk format is **unchanged**; no converter, no migration.
- Old files without `SvTm` → one pause-login, then aging begins (see §4).
- `master` currently does not have this behaviour; this lands on `unstable` first and reaches
  `master` via the normal PR. No cross-format concerns.

---

## 8. Optional softening (only if raw Option 1 is too harsh)

- **Cap the deduction** at a maximum (e.g. no more than N mud-hours removed per offline interval).
- **Exempt flagged "persistent" affects** (curses, diseases, long rituals) via an affect flag so
  they pause while ordinary buffs age.
- **Grace window**: no deduction for very short offline intervals (relogging).

All are data/flag-driven add-ons on top of the core mechanism; decide with the team.

---

## 9. Testing

- **Unit**: given a saved affect of duration D and an elapsed interval E, login yields
  `D - E/kSecsPerPlayerAffect` (and removal when it crosses 0). Cover normal, `kAfBattledec`,
  `kAfPulsedec`, and permanent (`-1`, untouched).
- **`set file` invariant**: `SvTm` is preserved across an offline `set file` edit (no restamp),
  so a subsequent login still ages by the real interval.
- **Round trip**: rent → wait → login deducts; matches the item deduction for the same interval.
- **Clock moved back**: `elapsed` clamps to 0, no negative aging.
- **Expiry side effects**: an affect that expires at login fires its `kExpired` `<actions>` and
  cleans up applies exactly once.
- Full `./build/tests/tests` green; boot-verify on the 5555 test server.

---

## 10. Rollout

- Guard the new behaviour behind a switch during bring-up if desired (config flag), defaulting off
  until the age-vs-pause direction is signed off, then on.
- Land on `unstable`; verify live; PR to `master` after review.

---

## Verified code references

| What | Location |
|---|---|
| Item offline deduction (model to mirror) | `src/engine/db/obj_save.cpp:1627` |
| Affect save (`Aff3`, relative duration) | `src/engine/entities/char_player.cpp` ~683 |
| Affect load (`af.duration = num2`, no deduction) | `char_player.cpp` `Aff3` case |
| Cooldown save/load (pause) | `char_player.cpp:484` / `:1709` |
| Feat timer load (pause) | `char_player.cpp:1392` |
| `kAfBattledec` out-of-combat tick (gated on `GetEnemy()`) | `src/gameplay/affects/affect_data.cpp:51` |
| `kAfPulsedec` tick (per pulse) | `affect_data.cpp` `UpdateAffectOnPulse` |
| Cadence constant `kSecsPerPlayerAffect = 2` | `src/gameplay/affects/affect_contants.h:16` |
| `last_logon` = entry time (do not reuse) | `src/engine/entities/char_data.h:579` |
| `set file` offline edit path (must preserve `SvTm`) | `src/engine/ui/cmd_god/do_set.cpp:153` |
