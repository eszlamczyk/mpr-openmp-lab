#!/usr/bin/env python3
"""
Plot scheduler benchmark results.

Usage:
    python plot_scheduler.py <csv_file> [output_dir]

CSV columns expected:
    n_elems, n_threads, static_calc, static_1, dynamic_calc, dynamic_1

Produces:
    scheduler_time_by_N.png      — time vs threads, one subplot per problem size
    scheduler_speedup_by_N.png   — speedup vs threads, one subplot per problem size
    scheduler_time_by_threads.png — time vs N, one subplot per thread count
"""

import sys
import os
import math
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np


VARIANTS = [
    ("static_calc",  "static(N/T)",   "#5b8dd9"),
    ("static_1",     "static(1)",     "#4caf7d"),
    ("dynamic_calc", "dynamic(N/T)",  "#e05c5c"),
    ("dynamic_1",    "dynamic(1)",    "#f0a830"),
]


def fmt_n(n: int) -> str:
    if n >= 1_000_000:
        v = n / 1_000_000
        return f"{v:.0f}M" if v == int(v) else f"{v:.1f}M"
    if n >= 1_000:
        v = n / 1_000
        return f"{v:.0f}k" if v == int(v) else f"{v:.1f}k"
    return str(n)


def load(csv_file: str) -> pd.DataFrame:
    df = pd.read_csv(csv_file)
    df["n_elems"] = df["n_elems"].astype(int)
    df["n_threads"] = df["n_threads"].astype(int)
    return df.groupby(["n_elems", "n_threads"], as_index=False).mean(numeric_only=True)


def grid_shape(n: int) -> tuple[int, int]:
    cols = math.ceil(math.sqrt(n))
    rows = math.ceil(n / cols)
    return rows, cols


def save(fig, path: str):
    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"  Saved {path}")


# ---------------------------------------------------------------------------
# Figure 1: time vs threads, one subplot per N
# ---------------------------------------------------------------------------
def plot_time_by_N(df: pd.DataFrame, out_dir: str):
    ns = sorted(df["n_elems"].unique())
    rows, cols = grid_shape(len(ns))

    fig, axes = plt.subplots(rows, cols,
                             figsize=(cols * 4.5, rows * 3.5),
                             squeeze=False)
    fig.suptitle("Execution time vs thread count", fontsize=14, fontweight="bold")

    for idx, n in enumerate(ns):
        ax = axes[idx // cols][idx % cols]
        sub = df[df["n_elems"] == n].sort_values("n_threads")

        for col, label, color in VARIANTS:
            ax.plot(sub["n_threads"], sub[col],
                    marker="o", linewidth=1.8, color=color, label=label)

        ax.set_title(f"N = {fmt_n(n)}", fontsize=10, fontweight="bold")
        ax.set_xlabel("Threads")
        ax.set_ylabel("Time (s)")
        ax.set_xticks(sub["n_threads"])
        ax.grid(True, linestyle="--", alpha=0.4)
        ax.legend(fontsize=7)

    for idx in range(len(ns), rows * cols):
        axes[idx // cols][idx % cols].set_visible(False)

    fig.tight_layout()
    save(fig, os.path.join(out_dir, "scheduler_time_by_N.png"))


# ---------------------------------------------------------------------------
# Figure 2: speedup vs threads, one subplot per N
# ---------------------------------------------------------------------------
def plot_speedup_by_N(df: pd.DataFrame, out_dir: str):
    ns = sorted(df["n_elems"].unique())
    rows, cols = grid_shape(len(ns))

    fig, axes = plt.subplots(rows, cols,
                             figsize=(cols * 4.5, rows * 3.5),
                             squeeze=False)
    fig.suptitle("Speedup vs thread count  (T₁ / Tₙ)", fontsize=14, fontweight="bold")

    for idx, n in enumerate(ns):
        ax = axes[idx // cols][idx % cols]
        sub = df[df["n_elems"] == n].sort_values("n_threads")
        t1 = sub[sub["n_threads"] == sub["n_threads"].min()]

        thread_vals = sub["n_threads"].tolist()

        # ideal speedup reference line
        ax.plot(thread_vals,
                [t / thread_vals[0] for t in thread_vals],
                linestyle=":", color="gray", linewidth=1.2, label="ideal")

        for col, label, color in VARIANTS:
            if t1.empty or t1[col].values[0] == 0:
                continue
            baseline = t1[col].values[0]
            speedup = baseline / sub[col]
            ax.plot(sub["n_threads"], speedup,
                    marker="o", linewidth=1.8, color=color, label=label)

        ax.set_title(f"N = {fmt_n(n)}", fontsize=10, fontweight="bold")
        ax.set_xlabel("Threads")
        ax.set_ylabel("Speedup")
        ax.set_xticks(thread_vals)
        ax.grid(True, linestyle="--", alpha=0.4)
        ax.legend(fontsize=7)

    for idx in range(len(ns), rows * cols):
        axes[idx // cols][idx % cols].set_visible(False)

    fig.tight_layout()
    save(fig, os.path.join(out_dir, "scheduler_speedup_by_N.png"))


# ---------------------------------------------------------------------------
# Figure 3: time vs N, one subplot per thread count
# ---------------------------------------------------------------------------
def plot_time_by_threads(df: pd.DataFrame, out_dir: str):
    thread_vals = sorted(df["n_threads"].unique())
    rows, cols = grid_shape(len(thread_vals))

    fig, axes = plt.subplots(rows, cols,
                             figsize=(cols * 4.5, rows * 3.5),
                             squeeze=False)
    fig.suptitle("Execution time vs problem size", fontsize=14, fontweight="bold")

    for idx, t in enumerate(thread_vals):
        ax = axes[idx // cols][idx % cols]
        sub = df[df["n_threads"] == t].sort_values("n_elems")
        ns = sub["n_elems"].tolist()
        x_labels = [fmt_n(n) for n in ns]

        for col, label, color in VARIANTS:
            ax.plot(range(len(ns)), sub[col],
                    marker="o", linewidth=1.8, color=color, label=label)

        ax.set_title(f"Threads = {t}", fontsize=10, fontweight="bold")
        ax.set_xlabel("N (elements)")
        ax.set_ylabel("Time (s)")
        ax.set_xticks(range(len(ns)))
        ax.set_xticklabels(x_labels, rotation=30, ha="right", fontsize=8)
        ax.set_yscale("log")
        ax.grid(True, linestyle="--", alpha=0.4, which="both")
        ax.legend(fontsize=7)

    for idx in range(len(thread_vals), rows * cols):
        axes[idx // cols][idx % cols].set_visible(False)

    fig.tight_layout()
    save(fig, os.path.join(out_dir, "scheduler_time_by_threads.png"))


# ---------------------------------------------------------------------------
# Figure 4: summary bar chart — all variants, fixed largest N, all threads
# ---------------------------------------------------------------------------
def plot_summary_bars(df: pd.DataFrame, out_dir: str):
    n_max = df["n_elems"].max()
    sub = df[df["n_elems"] == n_max].sort_values("n_threads")

    thread_vals = sub["n_threads"].tolist()
    x = np.arange(len(thread_vals))
    width = 0.2

    fig, ax = plt.subplots(figsize=(max(8, len(thread_vals) * 1.5), 5))

    for i, (col, label, color) in enumerate(VARIANTS):
        ax.bar(x + i * width, sub[col], width, label=label, color=color, alpha=0.85)

    ax.set_title(f"Time by schedule variant  (N = {fmt_n(n_max)})",
                 fontsize=12, fontweight="bold")
    ax.set_xlabel("Threads")
    ax.set_ylabel("Time (s)")
    ax.set_xticks(x + width * (len(VARIANTS) - 1) / 2)
    ax.set_xticklabels(thread_vals)
    ax.legend()
    ax.grid(True, axis="y", linestyle="--", alpha=0.4)

    fig.tight_layout()
    save(fig, os.path.join(out_dir, "scheduler_summary_bars.png"))


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <csv_file> [output_dir]")
        sys.exit(1)

    csv_file = sys.argv[1]
    out_dir = sys.argv[2] if len(sys.argv) >= 3 else "."
    os.makedirs(out_dir, exist_ok=True)

    df = load(csv_file)
    print(f"Loaded {len(df)} rows — "
          f"N: {sorted(df['n_elems'].unique())} — "
          f"threads: {sorted(df['n_threads'].unique())}")

    plot_time_by_N(df, out_dir)
    plot_speedup_by_N(df, out_dir)
    plot_time_by_threads(df, out_dir)
    plot_summary_bars(df, out_dir)

    print("Done.")


if __name__ == "__main__":
    main()
