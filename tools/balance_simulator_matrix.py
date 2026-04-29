#!/usr/bin/env python3
"""Plot a balance matrix: average damage per cast as a function of one
attacker stat (wisdom, dexterity, level, etc.).

Usage:
    python3 balance_simulator_matrix.py STAT_NAME stat=path.jsonl ... output.png

Each input is one mud-sim run; the leading 'STAT=' marks the value of the
varying parameter. Example:

    python3 balance_simulator_matrix.py wisdom \
        10:/tmp/sim_wis_10.jsonl \
        20:/tmp/sim_wis_20.jsonl \
        30:/tmp/sim_wis_30.jsonl \
        ~/repos/gateway/static/lightning_by_wis.png
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt


def avg_dam_per_cast(path: Path):
    spell_dam = 0
    casts = 0
    for line in path.open():
        e = json.loads(line)
        if e.get("name") == "damage" and e.get("spell_id", 0) != 0:
            spell_dam += e["dam"]
            casts += 1
    return spell_dam / max(casts, 1), casts


def main():
    if len(sys.argv) < 4:
        print(__doc__, file=sys.stderr)
        sys.exit(1)

    stat_name = sys.argv[1]
    output = Path(sys.argv[-1])
    points = []
    for arg in sys.argv[2:-1]:
        stat_val, jsonl_path = arg.split(":", 1)
        avg, casts = avg_dam_per_cast(Path(jsonl_path))
        points.append((int(stat_val), avg, casts))
    points.sort()

    xs = [p[0] for p in points]
    ys = [p[1] for p in points]

    fig, ax = plt.subplots(figsize=(10, 5.5))
    ax.plot(xs, ys, marker="o", linewidth=2, color="#d62728")
    for x, y in zip(xs, ys):
        ax.annotate(f"{y:.0f}", (x, y), textcoords="offset points", xytext=(0, 8),
                    ha="center", fontsize=9)
    ax.set_title(f"Средний урон за каст vs {stat_name} атакующего")
    ax.set_xlabel(f"{stat_name} атакующего")
    ax.set_ylabel("средний урон за каст")
    ax.grid(alpha=0.3, linestyle="--")
    ax.set_xticks(xs)

    fig.tight_layout()
    output.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(output, dpi=110, bbox_inches="tight")
    print(f"saved: {output}")


if __name__ == "__main__":
    main()
