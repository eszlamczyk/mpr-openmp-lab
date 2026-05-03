"""Shared constants and helpers for bucket-sort benchmark plotting scripts."""

import pandas as pd

COLS = [
    "algorithm", "n_threads", "n_elems",
    "t_random", "t_distribute", "t_merge", "t_sort", "t_writeback",
    "t_total", "sorted",
]

COMPONENTS = [
    ("t_distribute", "Distribute"),
    ("t_merge",      "Merge"),
    ("t_sort",       "Sort"),
    ("t_writeback",  "Write-back"),
]


def load(csv_file: str) -> pd.DataFrame:
    df = pd.read_csv(csv_file)
    df["n_elems"]   = df["n_elems"].astype(int)
    df["n_threads"] = df["n_threads"].astype(int)
    return df


def fmt_n(n: int) -> str:
    """Human-readable element count: 1000000 -> '1M', 500000 -> '500k'."""
    if n >= 1_000_000:
        v = n / 1_000_000
        return f"{v:.0f}M" if v == int(v) else f"{v:.1f}M"
    if n >= 1_000:
        v = n / 1_000
        return f"{v:.0f}k" if v == int(v) else f"{v:.1f}k"
    return str(n)
