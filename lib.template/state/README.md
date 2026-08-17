# state/

Server-wide **runtime state** Б─■ data about the running server as a whole, **not**
tied to an individual character/account and **not** part of the game world.

Paths here are owned by `state::StateManager` (there is no `LIB_STATE` macro);
ask `MUD::StateManager().Path(EStateFile::Б─╕)`. Most files ship empty (or are
absent) and are created/appended as the server runs.

| File | Written by | Purpose |
|------|-----------|---------|
| `schedule` | `db.cpp` | scheduled reboot times |
| `globaluid` | `db.cpp` | global unique object-id counter |
| `stop_offtop.lst` | `offtop.cpp` | off-topic channel block list |
| `proxy.lst` | `proxy.cpp` | known proxy IP list |
| `xnames.lst` | `names.cpp` | invalid name substrings |
| `apr_name.lst` | `names.cpp` | approved names |
| `dis_name.lst` | `names.cpp` | disallowed names |
| `new_name.lst` | `names.cpp` | names pending approval |
| `unfreeze.lst` | `do_unfreeze.cpp` | scheduled un-freeze list |
| `badsites.lst` | `ban.cpp` | banned site (host) list |
| `badproxy.lst` | `ban.cpp` | banned proxy list |
| `titles.lst` | `title.cpp` | approved/pending player titles |
| `registered-email.lst` | `proxy.cpp` | registered player emails (anti-multi-account) |
| `player_save_checksums.lst` | `file_crc.cpp` | CRC-32 snapshots of each character's save files (integrity check; was `crc.lst`) |
| `unique_mobs.xml` | `sets_drop.cpp` | unique-mob registry (regenerable cache) |
| `sets_drop_generated_table.lst` | `sets_drop.cpp` | generated set-drop table, reshuffled on a timer (regenerable cache; was the XML `sets_drop.xml`) |

## statistics/

Server-wide statistics (created at runtime):

| File | Written by | Purpose |
|------|-----------|---------|
| `mob_stat.bin` | `mob_stat.cpp` | mob statistics Б─■ current binary store |
| `mob_stat_new.xml` | `mob_stat.cpp` | mob statistics Б─■ legacy XML fallback (read-only) |
| `zone_traffic.xml` | `db.cpp` | per-zone traffic statistics |
| `global_drop.tmp` | `corpse.cpp` | per-mob kill counts (global-drop stats) |
| `spellstat.txt` | `spell_usage.cpp` | spell-usage append log (was `stat/spellstat.txt`) |
