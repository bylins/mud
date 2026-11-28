# World Loading Tests

`tools/run_load_tests.sh` boots the server against a test world, measures boot time, calculates world checksums, and checks that the world survives a save/reload round-trip (YAML → YAML, YAML → flat YAML, YAML → SQLite → engine). YAML is the primary world format. SQLite is tested only as a cache that the engine fills from YAML; Legacy is not tested.

## Quick Start

```bash
# Quick smoke test: small YAML world with checksums, YAML round-trips, Admin API test
./tools/run_load_tests.sh --quick
```

The script handles setup itself:
- Builds the binaries it needs with meson + ninja (configures a build directory on first use, then rebuilds incrementally)
- Materialises the small world from `lib/` + `lib.template/`
- Copies the full world from `FULL_WORLD_DIR` if that is set

No converter runs. The small world is the checked-in YAML from `lib.template/worlddata/world`, and the full world must already be a YAML directory.

## Requirements

- `meson` and `ninja` in `PATH` (or in `~/.local/bin`)
- `python3` (world setup, Admin API test)
- `fuser`, `lsof`, `bc`, `pkg-config`
- SQLite development headers (`pkg-config sqlite3`, package `libsqlite3-dev`) for the SQLite round-trip tests. Without them those tests are skipped with a warning; with `--loader=sqlite` the script exits with an error instead.
- Free TCP ports 4000 and 4001. The script kills whatever holds them (`fuser -k`) before each boot.

### Environment variables

| Variable | Default | Meaning |
|----------|---------|---------|
| `FULL_WORLD_DIR` | (unset) | The full **YAML** world. Required for full-world tests. If it is unset or missing, the full world is dropped from scope with a warning before any build starts. Small-world runs ignore it. Two layouts are accepted: a world tree (`world_config.yaml` and `zones/` at the top, e.g. the world repository) or a complete data dir (`cfg/` and `worlddata/world/`). |
| `BOOT_TIMEOUT` | `900` | Maximum seconds to wait for one boot (or for a cold SQLite resync). |

Example:

```bash
FULL_WORLD_DIR=~/repos/world_yaml ./tools/run_load_tests.sh --world=full
```

## Options

| Option | Effect |
|--------|--------|
| `--loader=TYPE` | Run only tests for one loader: `yaml` or `sqlite`. `sqlite` runs the YAML → SQLite round-trip tests. |
| `--world=SIZE` | Run only one world: `small` or `full`. |
| `--checksums` | Run only the boot tests **with** checksum calculation (`-W`). |
| `--no-checksums` | Run only the boot tests **without** checksum calculation. Also skips the checksum comparison section. |
| `--quick` | Shortcut for `--loader=yaml --world=small --checksums`. |
| `--rebuild` | Run `ninja -t clean` before building. |
| `--build-type=TYPE` | Override the meson `build_profile` (`release`, `debug`, `dev`, `fasttest`, `custom`; CMake-style names such as `Debug` or `Release` are mapped). |
| `--recreate-builds` | Delete and recreate the build directories before configuring. |
| `--help` | Print the usage header of the script. |

Filters combine, e.g. `--loader=yaml --world=small --checksums`.

The checksum filters apply only to plain boot tests. Round-trip tests and Admin API tests ignore them. Exception: `*_SQLite_SingleZoneStale` has neither "RoundTrip" nor "checksums" in its name, so `--checksums` filters it out.

## What Runs

### Build directories

| Directory | meson options | Used for |
|-----------|---------------|----------|
| `build_yaml/` | `-Dadmin_api=true -Dyaml=builtin -Dbuild_profile=debug` | YAML boot tests, YAML round-trips, Admin API test |
| `build_multi/` | `-Dadmin_api=true -Dyaml=builtin -Dsqlite=auto -Dbuild_profile=debug` | SQLite round-trip tests (binary with both YAML and SQLite backends) |

`build_yaml/` is built when the loader filter is empty or `yaml`. `build_multi/` is built when the loader filter is empty or `sqlite` and the SQLite headers are found. Note the default profile here is `debug`, not the `release` profile used for normal development; pass `--build-type=release` to change it.

### Test worlds

| World | Location | Source |
|-------|----------|--------|
| small | `build_yaml/small/`, `build_multi/small/` | `tools/meson/setup_world.py`: copies `cfg/`, `help/`, `state/`, `worlddata/` from `lib/`, then `worlddata/world` from `lib.template/` |
| full | `build_yaml/full/`, `build_multi/full/` | A world tree in `$FULL_WORLD_DIR`: `cfg/`, `help/`, `state/` and `worlddata/` (without its world) from `lib/`, the tree copied to `worlddata/world` (its `.git` dropped). A complete data dir: copied as is. |

Inside every data dir the engine reads the world from `worlddata/world` and `worlddata/world.db` (`CreateWorldSourceByName` in `src/engine/db/db.cpp`); all stages work on those paths.

Worlds are recreated from scratch on every run.

### Tests, in order

For each world in scope (`Small_*`, then `Full_*`):

| Test | Binary | What it does |
|------|--------|--------------|
| `<World>_YAML_checksums` | `build_yaml` | Boot with `-W`, report boot time and checksums. This is the baseline. |
| `<World>_YAML_no_checksums` | `build_yaml` | Boot without `-W` (boot time only). |
| `<World>_YAML_RoundTrip` | `build_yaml` | `circle -d . -S worlddata/world_v2` resaves the world, `world_v2` replaces `worlddata/world`, then a `-W` boot computes checksums of the re-emitted world. |
| `<World>_YAML_FlatRoundTrip` | `build_yaml` | Sets `layout: flat` in `world_config.yaml`, resaves with `-S worlddata/world_flat`, checks that the output really is flat (`zones/<n>/mobs.yaml` exists, no `mobs/` directory), boots it with `-W`, then restores the original world and config. |
| `<World>_SQLite_RoundTrip` | `build_multi` | Writes a composite `<world_loader><sources><yaml/><sqlite/></sources></world_loader>` into `cfg/configuration.xml` (replacing any existing one) and deletes the old `worlddata/world.db`. Source order is an authority ranking: the first source wins every disagreement, a later one wins a zone only when it agrees with all before it. Boot 1 loads YAML and the engine writes `world.db`. Boot 2 loads from SQLite (now in sync, so the cache wins) with `-W`. |
| `<World>_SQLite_SingleZoneStale` | `build_multi` | Repeats boots 1-2, then touches one zone's `rooms.yaml` and boots a third time with `-W`. That boot must load one zone from YAML and the rest from SQLite. The script prints the syslog `World source '...': won` lines and warns if it cannot confirm YAML won at least one zone. |
| `<World>_YAML_AdminAPI` | `build_yaml` | Recreates the world, enables `<admin_api>` in `cfg/configuration.xml` with `require_auth=false`, points `socket_path` at `admin_api.sock` in the data dir, starts the server on the aux port and runs `tests/test_admin_api.py full_crud_test` against the socket. Runs last because it mutates the world. |

Plain boot tests use `TEST_PORT` (default 4000). Resaves, the SQLite boot 1 and the Admin API test use `AUX_PORT` (default 4001). Set both when something else holds the defaults: `TEST_PORT=4100 AUX_PORT=4101 ./tools/run_load_tests.sh --quick`.

### Quick mode vs full run

| Mode | Tests that run |
|------|----------------|
| `--quick` | `Small_YAML_checksums`, `Small_YAML_RoundTrip`, `Small_YAML_FlatRoundTrip`, `Small_YAML_AdminAPI` |
| no options | Every test above for the small world, and for the full world if `FULL_WORLD_DIR` is set; SQLite tests only if the SQLite headers are found |
| `--loader=sqlite` | `*_SQLite_RoundTrip` and `*_SQLite_SingleZoneStale` only (`build_yaml` is not built) |

## Checksums and Comparison

A boot with `-W` makes the server write `checksums_detailed.txt` and a `checksums_buffers/` directory into the world directory, and log per-type CRC32 lines (`Zones:`, `Rooms:`, `Mobs:`, `Objects:`, `Triggers:`, plus runtime `Room Scripts:`, `Mob Scripts:`, `Object Scripts:`, `Door Rnums:`). The script copies `checksums_detailed.txt` to `/tmp/<TestName>_checksums.txt`.

Unless `--no-checksums` is given, the `CHECKSUM COMPARISON` section diffs each round-trip against the YAML baseline of the same world:

| Baseline | Compared with |
|----------|---------------|
| `<World>_YAML_checksums` | `<World>_YAML_RoundTrip`, `<World>_YAML_FlatRoundTrip` (loader filter empty or `yaml`) |
| `<World>_YAML_checksums` | `<World>_SQLite_RoundTrip`, `<World>_SQLite_SingleZoneStale` (loader filter empty or `sqlite`) |

Each pair prints `MATCH`, `DIFFER` (with the first 10 lines of `diff`), or `Cannot compare (missing files)`.

The comparison reads whatever is in `/tmp`. With `--loader=sqlite` the YAML baseline is not produced in that run, so the SQLite results are compared against a `/tmp/<World>_YAML_checksums_checksums.txt` left by an earlier run (possibly stale), or reported as missing.

## Understanding Output

For each boot test the script prints:

1. **Boot time**: time from `Boot db -- BEGIN` to `Boot db -- DONE` in the syslog
2. **Checksums**: one line per entity type, with counts
3. **Runtime checksums**: scripts and door rnums (only with `-W`)
4. **Checksum time**: from `Calculating world checksums` to `Detailed buffers saved`

Example (values are illustrative):

```
--- Small_YAML_checksums ---
  Running: /path/to/build_yaml/circle -d . -W 4000 (from small)
  Boot time:     1.234s
  Zones:    7C788E1F (55 zones)
  Rooms:    8C0277A7 (5109 rooms)
  Mobs:     CEBB697B (5123 mobs)
  Objects:  928B1B5A (5192 objects)
  Triggers: 91924F29 (5064 triggers)
  ...
  Checksum time: 0.456s

=== CHECKSUM COMPARISON ===

Small world:
Small_YAML_checksums vs Small_YAML_RoundTrip: MATCH
Small_YAML_checksums vs Small_YAML_FlatRoundTrip: MATCH
```

## Files and Logs

| Path | Content |
|------|---------|
| `/tmp/build_<yaml\|multi>.log`, `/tmp/build_<yaml\|multi>_meson.log` | ninja and meson output |
| `/tmp/setup_small_yaml_<build_dir>.log` | Small world setup |
| `/tmp/admin_api_setup_yaml.log` | World recreation before the Admin API test |
| `/tmp/<TestName>_checksums.txt` | Saved `checksums_detailed.txt` of each `-W` boot |
| `<build_dir>/<small\|full>/syslog` | Server syslog of the last boot in that world |
| `<build_dir>/<small\|full>/stdout.log` | Server stdout of plain boots |
| `<build_dir>/<small\|full>/stdout_rt.log`, `stdout_flat.log`, `stdout_sqlt1.log` | Resave and SQLite boot 1 output |
| `<build_dir>/<small\|full>/stdout_admin.log`, `admin_api_test.log` | Admin API server output and test log |
| `<build_dir>/<small\|full>/checksums_detailed.txt`, `checksums_buffers/` | Checksums of the last `-W` boot |

## Known Issues

Expected failures on the current tree (checked on the small world):

- **`*_YAML_RoundTrip`, `*_YAML_FlatRoundTrip`: checksums differ from the baseline** (small world: zones 1, 11, 40, 46, 50, 60 and a few dozen objects). The checked-in `lib.template` world is in an older form than the saver writes: liquid containers keep `values:` instead of `extra_values:` (`LIQUID_*`), some objects carry an empty extra description or `values: [-1, -1]`. The first resave converts them, and the loader reads the two forms into different in-memory state. A second resave is byte-identical to the first, so it is a one-time conversion, not a loss on every save.
- **`*_SQLite_RoundTrip`, `*_SQLite_SingleZoneStale`: checksums differ from the baseline in every zone, and in rooms, mobs, objects and triggers.** Not investigated yet.

## Troubleshooting

### Build fails
Check `/tmp/build_<yaml|multi>.log` and `/tmp/build_<yaml|multi>_meson.log`. The script prints the last 20 lines on failure.

### Tests time out
Raise `BOOT_TIMEOUT` (default 900 seconds), e.g. `BOOT_TIMEOUT=1800 ./tools/run_load_tests.sh --world=full`.

### "Failed to bind to port"
Something still holds port 4000. The script runs `fuser -k` before every boot; if it still happens, find and stop the process by hand.

### Checksum mismatches
Compare the saved checksum files:
```bash
diff /tmp/Small_YAML_checksums_checksums.txt /tmp/Small_YAML_RoundTrip_checksums.txt
```
For per-entity detail, compare the `checksums_buffers/` directories of two boots (the directory is overwritten by the next `-W` boot in the same world, so copy it aside first).

## Advanced Usage

### Running a boot by hand

```bash
cd build_yaml/small
../circle -d . -W 4000    # with checksums
../circle -d . 4000       # without checksums
grep -a "Zones:" syslog
```

## Adding New Tests

1. Write a `run_*_test` function (or reuse `run_test` with the right binary and world directory). Name the test `<World>_<Loader>_<Kind>` because `should_run_test` takes the world and loader from the first two `_`-separated parts.
2. Call it in the small-world and full-world sections.
3. If it produces checksums, add a `compare_checksums` line against the YAML baseline.
4. Update this document.

## See Also

- `tools/meson/setup_world.py`: builds the small world directory
- `tests/test_admin_api.py`: Admin API CRUD test
- `src/engine/db/world_checksum.cpp`: checksum calculation
- `tools/sqlite-world-schema.md`: SQLite world schema
- `tools/converter/convert_to_yaml.py`: converts a legacy world to YAML/SQLite (not used by this script)
