# Headless balance simulator (mud-sim)

Implementation of issue [#2967](https://github.com/bylins/mud/issues/2967):
generate balance data (damage / saves / charms across stat ranges) without
needing live players to grind.

The simulator boots the same engine code as the main `circle` binary, but
without the network layer. World loading, RNG, combat, magic — all the same.
A scenario YAML file describes the run; output is written as one JSON object
per line (JSONL).

## Building

The simulator is built when both `BUILD_TESTS` and `HAVE_YAML` are enabled:

```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DHAVE_YAML=ON ..
make -j$(($(nproc)/2)) mud-sim tests
```

`HAVE_YAML=ON` is required: scenarios are parsed from YAML, and the engine
itself is migrating off the legacy world format.

## Preparing a world

The simulator runs against a regular MUD world. With `HAVE_YAML=ON`, the engine
loads YAML world files; if you have a legacy world, convert it in-place first:

```bash
# Build a working world directory (legacy + YAML side by side):
mkdir -p build/small
cp -r lib/* build/small/
cp -r lib.template/* build/small/

# Convert the world to YAML in place (YAML files are written next to the
# legacy ones; the engine prefers YAML when HAVE_YAML=ON):
pip install --user --break-system-packages ruamel.yaml   # one-time
python3 tools/converter/convert_to_yaml.py -i build/small -o build/small -f yaml
```

## Running

```bash
cd build
./mud-sim --config /path/to/scenario.yaml -d small
```

`-d` accepts any prepared world directory (not only `small`).

## Scenario format

Minimal example:

```yaml
seed: 42                # RNG seed for reproducibility (default: 0)
rounds: 100             # number of heartbeat pulses to tick (default: 100)
output: /tmp/sim.jsonl  # path for the JSONL event log (required)

attacker:
  type: player          # 'player' or 'mob'
  class: sorcerer       # for player: class name (resolved via FindAvailableCharClassId)
  level: 30             # for player: level

victim:
  type: mob             # for mob: vnum
  vnum: 102
```

Class names are resolved through the engine's existing
`FindAvailableCharClassId()`, so any name accepted there (singular or plural)
works.

## Output format

JSONL — one JSON object per event, attributes flat at the top level. Layout
matches the OTLP log record schema, so the same files can later be ingested
into Loki without conversion. Schema is documented in
`src/engine/observability/event_sink.h`.

The scenario runner that emits combat events is **step 6** of the MVP and
lands in a follow-up commit; right now `mud-sim` just loads the scenario,
seeds the RNG, boots the world, and ticks the requested number of pulses.

## Reproducibility

Two runs with the same scenario produce semantically identical events: the
same HP-before/after, the same damage, the same victim_alive flag. The `ts`
attribute is wall-clock time and naturally differs between runs, so verify
reproducibility by ignoring it:

```bash
./mud-sim --config scenario.yaml -d small  # produces /tmp/sim.jsonl
cp /tmp/sim.jsonl /tmp/sim_first.jsonl
./mud-sim --config scenario.yaml -d small
diff -I '"ts":' /tmp/sim_first.jsonl /tmp/sim.jsonl   # should be empty
```

A different seed produces different numbers:

```bash
sed -i 's/seed: 42/seed: 99/' scenario.yaml
./mud-sim --config scenario.yaml -d small
diff -I '"ts":' /tmp/sim_first.jsonl /tmp/sim.jsonl   # should be non-empty
```

## Status / backlog

Tracked on [#2967](https://github.com/bylins/mud/issues/2967) and PR
[#3220](https://github.com/bylins/mud/pull/3220). After-MVP work (PvP, full
participant customization through YAML, precise combat instrumentation,
OtlpLogsSink, cast scenarios, custom synthetic world loader, visualization
scripts) is listed in the PR description and in the design plan linked from
the issue.

Known limitations of the MVP:
- Player class names in YAML scenarios are resolved through the engine's
  Russian-language class table (`pc_classes.xml`), so `class: колдун` works
  but `class: sorcerer` does not. A class-name alias table for English names
  is in the backlog.
- Mob-vs-mob runs may show zero damage on most rounds because spawned mobs
  do not aggress each other without `set_fighting`. The simulator forces
  `SetFighting(attacker, victim)` once per round, but spec_procs and triggers
  may interfere. PC-vs-mob and PvP scenarios (after the alias table lands)
  give cleaner numbers.

<!-- vim: set ts=4 sw=4 ai tw=0 et syntax=markdown : -->
