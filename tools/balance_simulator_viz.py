#!/usr/bin/env python3
"""Visualize JSONL output of mud-sim (issue #2967).

Reads several JSONL run files (each: one battle round per line) and renders
a one-page summary suitable for attaching to GitHub issues.

Usage:
    python3 balance_simulator_viz.py [--output PATH] FILE [FILE ...]

Each input file is one mud-sim run; the legend label is taken from
attacker_class (or attacker_vnum for mob attackers) plus the level.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt


def load(path: Path):
    rounds, dmg, label = [], [], path.stem
    with path.open() as fh:
        first = None
        for line in fh:
            ev = json.loads(line)
            if first is None:
                first = ev
            rounds.append(ev["round"])
            dmg.append(ev["damage_observed"])
    if first is not None:
        if first.get("attacker_type") == "player":
            label = f"{first['attacker_class']} (lvl {first['attacker_level']})"
        else:
            label = f"mob#{first['attacker_vnum']}"
    return label, rounds, dmg


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="+", type=Path)
    ap.add_argument("--output", type=Path, default=Path("/tmp/sim_demo.png"))
    ap.add_argument("--title", default="mud-sim: dps по классам vs mob 102, 80 раундов")
    args = ap.parse_args()

    runs = [load(p) for p in args.files]
    runs.sort(key=lambda r: -sum(r[2]))  # rank by total damage

    fig, (ax_bar, ax_line) = plt.subplots(1, 2, figsize=(13.5, 4.8))
    fig.suptitle(args.title, fontsize=12)

    labels = [r[0] for r in runs]
    means = [sum(r[2]) / max(len(r[2]), 1) for r in runs]
    cmap = plt.colormaps.get_cmap("tab10")
    colors = [cmap(i % 10) for i in range(len(runs))]

    bars = ax_bar.bar(labels, means, color=colors, edgecolor="black", linewidth=0.5)
    for bar, m in zip(bars, means):
        ax_bar.text(
            bar.get_x() + bar.get_width() / 2,
            bar.get_height() + max(means) * 0.01,
            f"{m:.2f}",
            ha="center",
            va="bottom",
            fontsize=9,
        )
    ax_bar.set_title("Средний damage_observed за раунд")
    ax_bar.set_ylabel("HP жертвы за один pulse_violence")
    ax_bar.tick_params(axis="x", rotation=20)
    ax_bar.grid(axis="y", alpha=0.25, linestyle="--")

    for (label, rounds, dmg), color in zip(runs, colors):
        cum = []
        s = 0
        for x in dmg:
            s += x
            cum.append(s)
        ax_line.plot(rounds, cum, color=color, label=label, linewidth=1.6)
    ax_line.set_title("Кумулятивный нанесённый урон")
    ax_line.set_xlabel("номер раунда")
    ax_line.set_ylabel("сумма damage_observed")
    ax_line.legend(loc="upper left", fontsize=8)
    ax_line.grid(alpha=0.25, linestyle="--")

    fig.tight_layout()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(args.output, dpi=110, bbox_inches="tight")
    print(f"saved: {args.output}")


if __name__ == "__main__":
    main()
