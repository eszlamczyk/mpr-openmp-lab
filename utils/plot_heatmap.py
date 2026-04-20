"""
Plot heatmaps from bucket-sort benchmark CSV output.

Usage:
    python plot_heatmap.py <csv_file> [output_dir]

Produces per algorithm:
    <algo>_total_heatmap.png       — (Figure 1) N x threads heatmap coloured by t_total
    <algo>_components_heatmap.png  — (Figure 2) 2x2 heatmaps for the 4 timing components
"""

import sys
import os
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

from bs_common import load, fmt_n, COMPONENTS


def make_pivot(df: pd.DataFrame, value_col: str) -> pd.DataFrame:
    pivot = df.pivot_table(
        index="n_threads", columns="n_elems", values=value_col, aggfunc="mean"
    )
    pivot.columns = [fmt_n(c) for c in pivot.columns]
    return pivot


def draw_heatmap(ax, pivot: pd.DataFrame, title: str, cbar_label: str = "Time (s)"):
    data = pivot.values.astype(float)
    vmin, vmax = np.nanmin(data), np.nanmax(data)
    norm = plt.Normalize(vmin=vmin, vmax=vmax)
    cmap = plt.get_cmap("coolwarm")

    xs, ys, cs = [], [], []
    for row in range(data.shape[0]):
        for col in range(data.shape[1]):
            val = data[row, col]
            if np.isnan(val):
                continue
            xs.append(col)
            ys.append(row)
            cs.append(val)
            ax.text(col, row + 0.25, f"{val:.3f}", ha="center", va="bottom",
                    fontsize=6, color="black")

    sc = ax.scatter(xs, ys, c=cs, cmap=cmap, norm=norm, s=120, zorder=3)

    ax.set_xlim(-0.5, pivot.shape[1] - 0.5)
    ax.set_ylim(-0.5, pivot.shape[0] - 0.5 + 0.4)
    ax.set_xticks(range(pivot.shape[1]))
    ax.set_xticklabels(pivot.columns, rotation=45, ha="right", fontsize=8)
    ax.set_yticks(range(pivot.shape[0]))
    ax.set_yticklabels(pivot.index, fontsize=8)
    ax.set_xlabel("N (elements)")
    ax.set_ylabel("Threads")
    ax.set_title(title, fontsize=10, fontweight="bold")
    ax.set_facecolor("#f5f5f5")

    plt.colorbar(sc, ax=ax, label=cbar_label, shrink=0.8)


def plot_algorithm(df: pd.DataFrame, algo: str, out_dir: str):
    sub = df[df["algorithm"] == algo]
    if sub.empty:
        print(f"  [skip] no data for {algo}", file=sys.stderr)
        return

    # Figure 1
    fig1, ax1 = plt.subplots(figsize=(max(5, sub["n_elems"].nunique() * 0.9),
                                       max(3, sub["n_threads"].nunique() * 0.5)))
    draw_heatmap(ax1, make_pivot(sub, "t_total"),
                 f"{algo}  –  Total Time", "t_total (s)")
    fig1.tight_layout()
    path1 = os.path.join(out_dir, f"{algo}_total_heatmap.png")
    fig1.savefig(path1, dpi=150)
    plt.close(fig1)
    print(f"  Saved {path1}")

    # Figure 2
    fig2, axes = plt.subplots(2, 2,
                               figsize=(max(9, sub["n_elems"].nunique() * 1.5),
                                        max(6, sub["n_threads"].nunique() * 0.9)))
    fig2.suptitle(f"{algo}  –  Timing Components", fontsize=13, fontweight="bold")

    for ax, (col, label) in zip(axes.flat, COMPONENTS):
        draw_heatmap(ax, make_pivot(sub, col), label, f"{col} (s)")

    fig2.tight_layout()
    path2 = os.path.join(out_dir, f"{algo}_components_heatmap.png")
    fig2.savefig(path2, dpi=150)
    plt.close(fig2)
    print(f"  Saved {path2}")


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <csv_file> [output_dir]")
        sys.exit(1)

    csv_file   = sys.argv[1]
    out_dir    = sys.argv[2] if len(sys.argv) >= 3 else "."
    os.makedirs(out_dir, exist_ok=True)

    df = load(csv_file)
    algorithms = sorted(df["algorithm"].unique())
    print(f"Found algorithms: {algorithms}")

    for algo in algorithms:
        print(f"Plotting {algo} …")
        plot_algorithm(df, algo, out_dir)


if __name__ == "__main__":
    main()
