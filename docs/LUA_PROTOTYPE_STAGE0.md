# Lua prototype stage 0 report

## Scope

Stage 0 goal was to validate whether Lua can be used as a practical trigger backend candidate before designing a full migration path.

Implemented scope:

- Binding: sol2.
- VM: LuaJIT.
- Build integration: optional `LUAJIT_PROTOTYPE` CMake mode.
- Prototype command: `luatest`.
- Prototype trigger benchmark: Lua rewrite of `death_mtrigger` control flow compared with current DG `death_mtrigger`.

## Available test commands

```text
luatest smoke
luatest file <path>
luatest deathmtrigger [iterations]
```

`deathmtrigger` creates a synthetic benchmark mob and a temporary DG `MTRIG_DEATH` trigger. The DG trigger body is intentionally minimal:

```text
nop benchmark
```

The benchmark compares:

- DG path: current C++ `death_mtrigger(ch, ch)` -> DG `script_driver` -> `nop benchmark`.
- Lua path: Lua rewrite of the `death_mtrigger` dispatch logic. It performs the same high-level checks as the C++ wrapper: victim state, death trigger presence, charmed state, first death trigger lookup, actor binding, and return code handling. The actual script body is represented by a minimal `script_driver` stub, not by a full DG command interpreter.

`script_driver` stub means a small Lua function that stands in for script execution during the benchmark. It does not execute real DG commands such as `send`, `act`, `load`, `if`, or `eval`.

Both sides use warmup iterations before the measured run.

## Measured result

Command:

```text
luatest deathmtrigger 1000000
```

Observed result in Test build:

```text
DG death_mtrigger avg_ns: 151.632
Lua death_mtrigger rewrite avg_ns: 68.7528
```

Ratio:

```text
151.632 / 68.7528 = 2.21x
```

Lua was about 2.2x faster on this hot-path benchmark.

## Interpretation

This benchmark validates the dispatch/control-flow overhead for a minimal death trigger. It does not claim that a real game death script has been migrated yet.

It does not yet validate:

- Real game script command costs (`send`, `act`, `load`, etc.).
- Full bindings for `CharData`, `ObjData`, rooms, affects, flags, and object lifetime.
- Wait/pause semantics.
- Error isolation, instruction limits, and operational safety.
- Production Release build performance.

## Decision

Stage 0 result: go to the next stage.

Recommended next step:

- Implement Lua backend for one explicit trigger type, starting with `MTRIG_DEATH`.
- Keep DG as fallback.
- Add opt-in marker for Lua triggers.
- Measure real converted death scripts, not only minimal `nop` dispatch.
