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
    swings = []  # list[int]: dam per damage event
    misses = 0
    fallback_per_round = []  # list[int]: damage_observed per round event
    first_round = None  # first 'round' event carries attacker_class/_vnum
    with path.open() as fh:
        for line in fh:
            ev = json.loads(line)
            name = ev.get("name")
            if name == "damage":
                swings.append(int(ev["dam"]))
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
    if swings or misses:
        return label, "damage", swings, rounds_total, misses
    return label, "round", fallback_per_round, rounds_total, 0


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
    runs.sort(key=lambda r: -sum(r[2]))

    fig, (ax_dpr, ax_hit, ax_line) = plt.subplots(1, 3, figsize=(17, 4.8))
    fig.suptitle(args.title, fontsize=12)

    labels = [r[0] for r in runs]
    cmap = plt.colormaps.get_cmap("tab10")
    colors = [cmap(i % 10) for i in range(len(runs))]

    # 1. dpr (avg damage per round)
    means = []
    for label, kind, vals, rounds_total, misses in runs:
        denom = rounds_total if rounds_total else max(len(vals), 1)
        means.append(sum(vals) / denom)
    bars = ax_dpr.bar(labels, means, color=colors, edgecolor="black", linewidth=0.5)
    for bar, m in zip(bars, means):
        ax_dpr.text(
            bar.get_x() + bar.get_width() / 2,
            bar.get_height() + max(means + [0.01]) * 0.01,
            f"{m:.2f}",
            ha="center", va="bottom", fontsize=9,
        )
    ax_dpr.set_title("Средний нанесённый урон за раунд")
    ax_dpr.set_ylabel("HP жертвы за один pulse_violence")
    ax_dpr.tick_params(axis="x", rotation=20)
    ax_dpr.grid(axis="y", alpha=0.25, linestyle="--")

    # 2. hit rate (% попаданий)
    rates = []
    for label, kind, vals, rounds_total, misses in runs:
        hits = len(vals) if kind == "damage" else 0
        total = hits + misses
        rates.append(100.0 * hits / total if total else 0.0)
    bars2 = ax_hit.bar(labels, rates, color=colors, edgecolor="black", linewidth=0.5)
    for bar, r in zip(bars2, rates):
        ax_hit.text(
            bar.get_x() + bar.get_width() / 2,
            bar.get_height() + 1.0,
            f"{r:.1f}%",
            ha="center", va="bottom", fontsize=9,
        )
    ax_hit.set_title("Hit rate")
    ax_hit.set_ylabel("% попаданий от всех swing")
    ax_hit.set_ylim(0, max(rates + [10]) * 1.15)
    ax_hit.tick_params(axis="x", rotation=20)
    ax_hit.grid(axis="y", alpha=0.25, linestyle="--")

    # 3. cumulative damage
    for (label, kind, vals, _, _), color in zip(runs, colors):
        cum = []
        s = 0
        for x in vals:
            s += x
            cum.append(s)
        ax_line.plot(
            range(1, len(cum) + 1),
            cum,
            color=color,
            label=f"{label}" + ("" if kind == "damage" else " [round-fallback]"),
            linewidth=1.6,
        )
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
