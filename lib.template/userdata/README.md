# userdata/

Data about the **people playing** Б─■ accounts, their characters' save files, and
everything a character owns or carries. As distinct from server-wide state
(`state/`) and world content (`worlddata/`).

The per-subsystem directories and their alphabetic shards (`A-E`, `F-J`, `K-O`,
`P-T`, `U-Z`, `ZZZ`) are created at boot; player files appear as characters play.

## chardata/ Б─■ per-character data (sharded by character name)

| Path | Read/written by | Purpose |
|------|-----------------|---------|
| `characters/<shard>/<name>.player` | `player_index.cpp` / save-load | the character record |
| `characters/players.lst` | `player_index.cpp` | the character registry (nameБ├■uidБ├■metadata) |
| `characters/players.dup` | Б─■ | legacy backup file (not read by the engine) |
| `items/<shard>/<name>.textobjs`, `.timeobjs` | `obj_save.cpp` | the crash/rent save Б─■ worn equipment + carried inventory + container contents |
| `aliases/<shard>/<name>.alias` | `db.cpp` | command aliases |
| `variables/<shard>/<name>.mem` | `db.cpp` | DG-script variables |
| `depots/<shard>/<name>.pers`, `.share`, `.purge` | `depot.cpp` | per-character depot (personal storage) |
| `depots/depot.db`, `depots/purge.db` | `depot.cpp` | depot bulk stores |

## Account and clan data

| Path | Read/written by | Purpose |
|------|-----------------|---------|
| `accounts/<email>` | `accounts.cpp` | per-account data (character roster, currencies, daily quests) |
| `clans/Б─╕` | `house.cpp` | clan (house/castle) data |
| `clans/item_desc.xml` | `shop_ext.cpp` | clan-store item descriptions |

## Cross-player and loose files

| Path | Read/written by | Purpose |
|------|-----------------|---------|
| `boards/*.board` | `boards.cpp` | message boards (general/news/idea/error/god-*/Б─╕) |
| `plrmail.xml` | `mail.cpp` | pending (unread) in-game player mail |
| `exchange.db`, `exchange.backup` | `exchange.cpp` | auction/exchange ledger (player items in escrow) |
| `parcel.db` | `parcel.cpp` | mailed parcels (player items in transit) |
| `glory.lst` | `glory.cpp` | legacy "temporary glory" balances |
| `glory_const.xml` | `glory_const.cpp` | permanent-glory balances + purchased stat bonuses (per character) |
| `named_items.xml` | `named_stuff.cpp` | registry of "named" items (was `named_stuff_list.xml`) |
