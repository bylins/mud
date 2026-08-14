# state/

Server-wide **runtime state** Б─■ data about the running server as a whole, **not**
tied to an individual character/account and **not** part of the game world.

Being migrated here from `misc/` and `etc/` (issue.misc-migrate). Currently:

| File | Written by | Purpose |
|------|-----------|---------|
| `schedule` | `db.cpp` | scheduled reboot times |
| `badsites` | `ban.cpp` | banned site (host) list |
| `badproxy` | `ban.cpp` | banned proxy list |
| `globaluid` | `db.cpp` | global unique object-id counter |
| `stop_offtop` | `offtop.cpp` | off-topic channel block list |
| `proxy` | `proxy.cpp` | known proxy IP list (created at runtime) |
| `xnames` | `names.cpp` | invalid name substrings |
| `apr_name` | `names.cpp` | approved names |
| `dis_name` | `names.cpp` | disallowed names |
| `new_name` | `names.cpp` | names pending approval |
| `unfreeze.lst` | `do_unfreeze.cpp` | scheduled un-freeze list (created at runtime) |

Most ship empty and are appended to as the server runs.
