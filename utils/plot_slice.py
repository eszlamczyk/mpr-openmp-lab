#!/usr/bin/env python3
"""
Plot line charts for a fixed N or fixed thread-count slice of the benchmark data.

Usage:
    python plot_slice.py <csv_file> <algorithm> <type> <value>

Arguments:
    csv_file   path to the benchmark CSV
    algorithm  algorithm name as it appears in the CSV (e.g. bs2, bs3)
    type       'N' to fix n_elems, 'threads' to fix n_threads
    value      the fixed value (integer)

Examples:
    python plot_slice.py results.csv bs2 N 1000000
    python plot_slice.py results.csv bs3 threads 8

Results:
    <algo>_<type><value>_total.png       — (Figure 1) big line plot: x=free axis, y=t_total
    <algo>_<type><value>_components.png  — (Figure 2) 2x2 subplots with dots+dashed lines
"""

import sys
import os
import matplotlib.pyplot as plt

from bs_common import load, fmt_n, COMPONENTS


def main():
    if len(sys.argv) < 5:
        print(f"Usage: {sys.argv[0]} <csv_file> <algorithm> <type> <value>")
        print("  type: 'N' to fix n_elems, 'threads' to fix n_threads")
        sys.exit(1)

    csv_file  = sys.argv[1]
    algorithm = sys.argv[2]
    ftype     = sys.argv[3]
    value     = int(sys.argv[4])

    if ftype not in ("N", "threads"):
        print(f"Error: type must be 'N' or 'threads', got '{ftype}'")
        sys.exit(1)

    df = load(csv_file)

    df = df[df["algorithm"] == algorithm]
    if df.empty:
        print(f"No rows found for algorithm '{algorithm}'.")
        sys.exit(1)

    if ftype == "N":
        df = df[df["n_elems"] == value]
        x_col      = "n_threads"
        x_label    = "Threads"
        fixed_desc = f"N = {fmt_n(value)}"
    else:
        df = df[df["n_threads"] == value]
        x_col      = "n_elems"
        x_label    = "N (elements)"
        fixed_desc = f"Threads = {value}"

    if df.empty:
        print(f"No data for algorithm='{algorithm}', {ftype}={value}.")
        sys.exit(1)

    grouped = (
        df.groupby(x_col, sort=True)
          .mean(numeric_only=True)
          .reset_index()
    )

    xs = grouped[x_col].tolist()
    x_labels = [fmt_n(v) if ftype == "threads" else str(v) for v in xs]

    MAX_TICKS = 10
    step = max(1, len(xs) // MAX_TICKS)
    tick_xs     = xs[::step]
    tick_labels = x_labels[::step]

    tag     = f"{algorithm}_{ftype}{value}"
    out_dir = "."

    # Figure 1
    fig1, ax1 = plt.subplots(figsize=(9, 5))

    ax1.plot(xs, grouped["t_total"], marker="o", linewidth=2,
             color="steelblue", label="t_total")
    ax1.set_xticks(tick_xs)
    ax1.set_xticklabels(tick_labels, rotation=30, ha="right")
    ax1.set_xlabel(x_label)
    ax1.set_ylabel("Time (s)")
    ax1.set_title(f"{algorithm}  –  Total Time  ({fixed_desc})",
                  fontsize=12, fontweight="bold")
    ax1.grid(True, linestyle="--", alpha=0.5)
    ax1.legend()

    fig1.tight_layout()
    path1 = os.path.join(out_dir, f"{tag}_total.png")
    fig1.savefig(path1, dpi=150)
    plt.close(fig1)
    print(f"Saved {path1}")

    # Figure 2
    fig2, axes = plt.subplots(2, 2, figsize=(13, 8))
    fig2.suptitle(f"{algorithm}  –  Timing Components  ({fixed_desc})",
                  fontsize=13, fontweight="bold")

    colors = ["#e05c5c", "#f0a830", "#4caf7d", "#5b8dd9"]

    for ax, (col, label), color in zip(axes.flat, COMPONENTS, colors):
        ys = grouped[col]
        ax.plot(xs, ys, linestyle="--", linewidth=1.4, color=color, alpha=0.7)
        ax.scatter(xs, ys, color=color, s=60, zorder=5)
        ax.set_xticks(tick_xs)
        ax.set_xticklabels(tick_labels, rotation=30, ha="right", fontsize=8)
        ax.set_xlabel(x_label, fontsize=9)
        ax.set_ylabel("Time (s)", fontsize=9)
        ax.set_title(label, fontsize=10, fontweight="bold")
        ax.grid(True, linestyle="--", alpha=0.4)

    fig2.tight_layout()
    path2 = os.path.join(out_dir, f"{tag}_components.png")
    fig2.savefig(path2, dpi=150)
    plt.close(fig2)
    print(f"Saved {path2}")


if __name__ == "__main__":
    main()
