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
  vnum: 1234
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

Two runs with the same scenario produce byte-identical JSONL files. Verify:

```bash
./mud-sim --config scenario.yaml -d small  # produces /tmp/sim.jsonl
cp /tmp/sim.jsonl /tmp/sim_first.jsonl
./mud-sim --config scenario.yaml -d small
diff /tmp/sim_first.jsonl /tmp/sim.jsonl   # should be empty
```

## Status / backlog

Tracked on [#2967](https://github.com/bylins/mud/issues/2967) and PR
[#3220](https://github.com/bylins/mud/pull/3220). After-MVP work (PvP, full
participant customization through YAML, precise combat instrumentation,
OtlpLogsSink, cast scenarios, custom synthetic world loader, visualization
scripts) is listed in the PR description and in the design plan linked from
the issue.

<!-- vim: set ts=4 sw=4 ai tw=0 et syntax=markdown : -->
