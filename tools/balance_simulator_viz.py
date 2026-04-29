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
    """Returns (label, kind, damage_events, rounds_total, misses).

    `damage_events` is a list of (ts, dam, is_spell) tuples in **emit order**
    (which is chronological because EventSink writes synchronously). Caller
    can split by `is_spell` for stacked bars or keep order for the cumulative
    timeline.
    """
    label = path.stem
    rounds_total = 0
    damage_events = []  # list[(ts, dam, is_spell)]
    misses = 0
    fallback_per_round = []  # list[int]: damage_observed per round event
    first_round = None
    with path.open() as fh:
        for line in fh:
            ev = json.loads(line)
            name = ev.get("name")
            if name == "damage":
                damage_events.append(
                    (int(ev["ts"]), int(ev["dam"]), ev.get("spell_id", 0) != 0)
                )
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
    if damage_events or misses:
        return label, "damage", damage_events, rounds_total, misses
    return label, "round", [(0, d, False) for d in fallback_per_round], rounds_total, 0


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
    runs.sort(key=lambda r: -sum(d for _, d, _ in r[2]))

    fig, (ax_dpr, ax_hit, ax_line) = plt.subplots(1, 3, figsize=(18, 4.8))
    fig.suptitle(args.title, fontsize=12)

    labels = [r[0] for r in runs]

    # 1. stacked dpr (melee + spell)
    melee_means = []
    spell_means = []
    for label, kind, events, rounds_total, misses in runs:
        denom = rounds_total if rounds_total else max(len(events), 1)
        melee_means.append(sum(d for _, d, sp in events if not sp) / denom)
        spell_means.append(sum(d for _, d, sp in events if sp) / denom)
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

    # 2. hit rate (% попаданий)
    rates = []
    for label, kind, events, rounds_total, misses in runs:
        hits = len(events) if kind == "damage" else 0
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

    # 3. cumulative damage по реальному таймлайну (ts), нормализованному в
    # секунды от первого события сценария.
    for (label, kind, events, _, _), color in zip(runs, colors):
        if not events:
            continue
        events_sorted = sorted(events, key=lambda e: e[0])
        t0 = events_sorted[0][0]
        xs = [(ts - t0) / 1000.0 for ts, _, _ in events_sorted]
        cum = []
        s = 0
        for _, d, _ in events_sorted:
            s += d
            cum.append(s)
        ax_line.plot(xs, cum, color=color, label=label, linewidth=1.6)
    ax_line.set_title("Кумулятивный урон по таймлайну (s от старта)")
    ax_line.set_xlabel("секунд от первого события")
    ax_line.set_ylabel("сумма dam")
    ax_line.legend(loc="upper left", fontsize=8)
    ax_line.grid(alpha=0.25, linestyle="--")

    fig.tight_layout()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(args.output, dpi=110, bbox_inches="tight")
    print(f"saved: {args.output}")


if __name__ == "__main__":
    main()
