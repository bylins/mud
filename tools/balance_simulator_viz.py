#!/usr/bin/env python3
"""Visualize JSONL output of mud-sim (issue #2967).

Reads several JSONL run files and renders a one-page summary.

Each input file is one mud-sim run. The legend label is taken from
attacker_class (or attacker_vnum for mob attackers) plus the level. JSONL may
contain two event kinds:
  - 'damage' (preferred): one per successful swing, exact `dam` from
    Damage::Process. Used for histograms and the cumulative line.
  - 'round'  (fallback): one per battle round, HP-delta `damage_observed`
    that includes regeneration. Used only if there are no 'damage' events.

Usage:
    python3 balance_simulator_viz.py [--output PATH] FILE [FILE ...]
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt


def load(path: Path):
    label = path.stem
    rounds_total = 0
    melee_swings = []  # list[int]: dam per melee damage event (spell_id == 0)
    spell_swings = []  # list[int]: dam per spell damage event (spell_id != 0)
    misses = 0
    fallback_per_round = []  # list[int]: damage_observed per round event
    first_round = None
    with path.open() as fh:
        for line in fh:
            ev = json.loads(line)
            name = ev.get("name")
            if name == "damage":
                if ev.get("spell_id", 0) != 0:
                    spell_swings.append(int(ev["dam"]))
                else:
                    melee_swings.append(int(ev["dam"]))
            elif name == "miss":
                misses += 1
            elif name == "round":
                rounds_total += 1
                fallback_per_round.append(int(ev["damage_observed"]))
                if first_round is None:
                    first_round = ev
    if first_round is not None:
        if first_round.get("attacker_type") == "player":
            label = f"{first_round['attacker_class']} (lvl {first_round['attacker_level']})"
        else:
            label = f"mob#{first_round['attacker_vnum']}"
    if melee_swings or spell_swings or misses:
        return label, "damage", melee_swings, spell_swings, rounds_total, misses
    return label, "round", fallback_per_round, [], rounds_total, 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="+", type=Path)
    ap.add_argument("--output", type=Path, default=Path("/tmp/sim_demo.png"))
    ap.add_argument(
        "--title",
        default="mud-sim: dps по классам vs mob 102",
    )
    args = ap.parse_args()

    runs = [load(p) for p in args.files]
    runs.sort(key=lambda r: -(sum(r[2]) + sum(r[3])))

    fig, (ax_dpr, ax_hit, ax_line) = plt.subplots(1, 3, figsize=(18, 4.8))
    fig.suptitle(args.title, fontsize=12)

    labels = [r[0] for r in runs]

    # 1. stacked dpr (melee + spell)
    melee_means = []
    spell_means = []
    for label, kind, melee, spell, rounds_total, misses in runs:
        denom = rounds_total if rounds_total else max(len(melee) + len(spell), 1)
        melee_means.append(sum(melee) / denom)
        spell_means.append(sum(spell) / denom)
    ax_dpr.bar(labels, melee_means, color="#4878d0", edgecolor="black", linewidth=0.5, label="melee")
    ax_dpr.bar(labels, spell_means, bottom=melee_means, color="#ee854a", edgecolor="black", linewidth=0.5, label="spell")
    for i, (m, s) in enumerate(zip(melee_means, spell_means)):
        ax_dpr.text(i, m + s + max(map(sum, zip(melee_means, spell_means))) * 0.01,
                    f"{m + s:.1f}", ha="center", va="bottom", fontsize=9)
    ax_dpr.set_title("Средний нанесённый урон за раунд (melee + spell)")
    ax_dpr.set_ylabel("HP жертвы за один pulse_violence")
    ax_dpr.tick_params(axis="x", rotation=20)
    ax_dpr.grid(axis="y", alpha=0.25, linestyle="--")
    ax_dpr.legend(loc="upper right", fontsize=9)

    # 2. hit rate (% попаданий) — ortho к melee/spell, считаем всё сразу
    rates = []
    for label, kind, melee, spell, rounds_total, misses in runs:
        hits = len(melee) + len(spell) if kind == "damage" else 0
        total = hits + misses
        rates.append(100.0 * hits / total if total else 0.0)
    cmap = plt.colormaps.get_cmap("tab10")
    colors = [cmap(i % 10) for i in range(len(runs))]
    bars2 = ax_hit.bar(labels, rates, color=colors, edgecolor="black", linewidth=0.5)
    for bar, r in zip(bars2, rates):
        ax_hit.text(bar.get_x() + bar.get_width() / 2, bar.get_height() + 1.0,
                    f"{r:.1f}%", ha="center", va="bottom", fontsize=9)
    ax_hit.set_title("Hit rate (попадания / все swing)")
    ax_hit.set_ylabel("%")
    ax_hit.set_ylim(0, max(rates + [10]) * 1.15)
    ax_hit.tick_params(axis="x", rotation=20)
    ax_hit.grid(axis="y", alpha=0.25, linestyle="--")

    # 3. cumulative damage — все damage-события (melee + spell) в порядке появления
    for (label, kind, melee, spell, _, _), color in zip(runs, colors):
        vals = melee + spell  # порядок не сохраняем, но кумулятив всё равно монотонен
        cum = []
        s = 0
        for x in vals:
            s += x
            cum.append(s)
        ax_line.plot(range(1, len(cum) + 1), cum, color=color, label=label, linewidth=1.6)
    ax_line.set_title("Кумулятивный нанесённый урон")
    ax_line.set_xlabel("номер damage-события")
    ax_line.set_ylabel("сумма dam")
    ax_line.legend(loc="upper left", fontsize=8)
    ax_line.grid(alpha=0.25, linestyle="--")

    fig.tight_layout()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(args.output, dpi=110, bbox_inches="tight")
    print(f"saved: {args.output}")


if __name__ == "__main__":
    main()
