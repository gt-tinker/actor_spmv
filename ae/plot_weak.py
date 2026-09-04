#!/usr/bin/env python3
"""Weak-scaling plots for erdos-renyi and rmat (4 actor configs + PETSc),
parsed from the generated .out files under $AE_ROOT/results/weak.

Unlike ae_eval/hclib_spmv.py (hand-transcribed np.array literals), this
reads real .out files the way plot_strong.py / plot_gap_style.py do.

Each family gets one figure with 3 subplots ("ladders"): node=2^k paired
with scale=scale_start+k.
  ladder A: scale_start=18, k=1..6 -> scales 19-24, nodes 2-64
  ladder B: scale_start=21, k=1..6 -> scales 22-27, nodes 2-64
  ladder C: scale_start=24, k=1..6 -> scales 25-30, nodes 2-64  (erdos-renyi)
  ladder C: scale_start=24, k=1..5 -> scales 25-29, nodes 2-32  (rmat)
"""
import re
import statistics
from pathlib import Path
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker

AE_ROOT = Path(__file__).resolve().parent.parent
ACTOR_DATA = AE_ROOT / "results" / "weak"
OUTDIR = AE_ROOT / "plots"
OUTDIR.mkdir(exist_ok=True, parents=True)

# variant directory name -> (partition, format), same mapping as strong scaling
ACTOR_VARIANTS = {
    "output_weak_row_block_csr":    ("row_block", "CSR"),
    "output_weak_row_block_csc":    ("row_block", "DCSC"),
    "output_weak_column_block_csc": ("column_block", "CSC"),
    "output_weak_column_block_csr": ("column_block", "DCSR"),
}

# PETSc: single MPI-only config (OpenMP disabled at build, 24 ranks/node),
# same file-naming convention as actor but no partition/format dimension.
PETSC_WEAK_DIR = "output_weak_petsc"

# Matches out_spmv_erdos-renyi-19_N2_n48 and out_spmv_rmat_24_N2_n48
WEAK_FILE_RE = re.compile(r"^out_spmv_(erdos-renyi|rmat)[-_](\d+)_N(\d+)_n(\d+)$")
RUN_RE = re.compile(r"^Run \d+:\s*([\d.eE+-]+)", re.MULTILINE)

FORMAT_COLOR = {"CSR": "blue", "CSC": "orange", "DCSR": "green", "DCSC": "purple"}
FORMAT_LINESTYLE = {"CSR": "-", "CSC": "-", "DCSR": "--", "DCSC": "--"}
PARTITION_MARKER = {"row_block": "D", "column_block": "^"}
PARTITION_LABEL = {"row_block": "row", "column_block": "column"}
PETSC_COLOR = "black"
PETSC_LINESTYLE = "-"
PETSC_MARKER = "o"
PETSC_LABEL = "PETSc"

X_MARGIN = 0.04
Y_MARGIN = 0.08
AXES_FACECOLOR = "#f9fbff"
GRID_COLOR = "#dadce2"
SCALE_ROW_COLOR = "#d6e9ff"
NODES_ROW_COLOR = "#e8e8e8"
PROCS_ROW_COLOR = "#ffd6e7"

FAMILIES = {
    "erdos-renyi": {"sep": "-", "title": "Erdos-Renyi", "ladders": [(18, 6), (21, 6), (24, 6)]},
    "rmat":        {"sep": "_", "title": "RMAT",        "ladders": [(18, 6), (21, 6), (24, 5)]},
}


def median_from_runs(text):
    vals = [float(m.group(1)) for m in RUN_RE.finditer(text)]
    if not vals:
        return None, 0
    return statistics.median(vals), len(vals)


def collect_family_data(family):
    # actor_series[(partition, fmt)][(scale, nodes)] = median runtime
    actor_series = {}
    scale_nodes_to_procs = {}
    for variant_dir, key in ACTOR_VARIANTS.items():
        vdir = ACTOR_DATA / variant_dir
        if not vdir.is_dir():
            continue
        for f in sorted(vdir.glob(f"out_spmv_{family}*_N*_n*")):
            m = WEAK_FILE_RE.match(f.name)
            if not m or m.group(1) != family:
                continue
            scale, nodes, procs = int(m.group(2)), int(m.group(3)), int(m.group(4))
            med, n = median_from_runs(f.read_text(errors="ignore"))
            if med is None or n < 10:
                print(f"  [skip] {variant_dir}/{f.name}: only {n}/10 runs found")
                continue
            actor_series.setdefault(key, {})[(scale, nodes)] = med
            scale_nodes_to_procs[(scale, nodes)] = procs

    petsc_series = {}
    petsc_dir = ACTOR_DATA / PETSC_WEAK_DIR
    if petsc_dir.is_dir():
        for f in sorted(petsc_dir.glob(f"out_spmv_{family}*_N*_n*")):
            m = WEAK_FILE_RE.match(f.name)
            if not m or m.group(1) != family:
                continue
            scale, nodes, procs = int(m.group(2)), int(m.group(3)), int(m.group(4))
            med, n = median_from_runs(f.read_text(errors="ignore"))
            if med is None or n < 10:
                print(f"  [skip] petsc {PETSC_WEAK_DIR}/{f.name}: only {n}/10 runs found")
                continue
            petsc_series[(scale, nodes)] = med
            scale_nodes_to_procs[(scale, nodes)] = procs

    return actor_series, petsc_series, scale_nodes_to_procs


def pow2_formatter(value, _pos):
    exp = round(mticker.math.log2(value))
    return f"$2^{{{exp}}}$"


def add_aligned_table(ax, tick_data_x, row_labels, cell_rows, *, y0, height, fontsize):
    """Draws a table whose data columns are centered under ax's existing
    x-ticks at tick_data_x (in data coordinates). Pass row_labels=None to
    omit the row-label column entirely (see note below).

    ax.table() with an explicit bbox divides that bbox into N equal-width
    column segments for N columns -- but N tick points span only N-1 gaps,
    so "N equal segments across the tick span" puts column centers at a
    different spacing than the ticks themselves. Exact centering for every
    column would require widening the table to N/(N-1) of the original
    span, which can be far too wide for N=2-3 (e.g. nearly double) and
    overlap neighboring subplots in a tightly-packed multi-panel figure.

    Column boundaries are placed at the midpoints between consecutive
    ticks (giving every *interior* column an exact center on its tick),
    while the two outer edges are clipped to the original [margin,
    1-margin] span -- so the data columns never grow wider than before, at
    the cost of the two edge columns being only approximately centered.
    colWidths must be given as exactly N absolute axes-fraction widths
    (one per data column, summing to the bbox width) -- matplotlib ignores
    any extra entries rather than using them for the row-label column, and
    normalizes/repositions unpredictably if they don't sum to the bbox
    width, which is what actually causes the misalignment this fixes.

    The row-label column (when present) is entirely outside this bbox --
    matplotlib renders it further left, sized to fit its text, on top of
    the (already-bounded) data width. For a narrow subplot in a
    tightly-packed multi-panel figure that can still collide with the
    previous subplot; pass row_labels=None there and have the caller
    render a single shared label instead (the same pattern already used
    for the "Runtime (s)" y-axis label).

    Draws once (via fig.canvas.draw()) before measuring tick positions, so
    this works regardless of fontsize/subplot size/margins.
    """
    fig = ax.figure
    n = len(tick_data_x)
    margin = 0.04
    if n < 2:
        table = ax.table(cellText=cell_rows, rowLabels=row_labels, cellLoc="center",
                          bbox=[margin, y0, 1 - 2 * margin, height])
        table.auto_set_font_size(False)
        table.set_fontsize(fontsize)
        return table

    fig.canvas.draw()
    inv = ax.transAxes.inverted()

    def data_to_axfrac_x(xval):
        disp = ax.transData.transform((xval, ax.get_ylim()[0]))
        return inv.transform(disp)[0]

    ticks = [data_to_axfrac_x(x) for x in tick_data_x]
    s = (ticks[-1] - ticks[0]) / (n - 1)

    left_edge = max(ticks[0] - s / 2, margin)
    right_edge = min(ticks[-1] + s / 2, 1 - margin)
    internal = [(ticks[i] + ticks[i + 1]) / 2 for i in range(n - 1)]
    boundaries = [left_edge] + internal + [right_edge]
    seg_widths = [boundaries[i + 1] - boundaries[i] for i in range(n)]

    table = ax.table(
        cellText=cell_rows, rowLabels=row_labels, cellLoc="center",
        bbox=[left_edge, y0, right_edge - left_edge, height], colWidths=seg_widths,
    )
    table.auto_set_font_size(False)
    table.set_fontsize(fontsize)
    return table


def render_ladder(ax, actor_series, petsc_series, scale_nodes_to_procs, scale_start, max_k, *,
                   title=None, title_fontsize=15, ylabel_fontsize=16,
                   ytick_fontsize=13, table_fontsize=11, linewidth=2,
                   markersize=6, show_ylabel=True):
    ax.set_facecolor(AXES_FACECOLOR)
    nodes_list = [2 ** k for k in range(1, max_k + 1)]
    scales_list = [scale_start + k for k in range(1, max_k + 1)]

    for (partition, fmt), series in sorted(actor_series.items()):
        xs, ys = [], []
        for nodes, scale in zip(nodes_list, scales_list):
            v = series.get((scale, nodes))
            if v is not None:
                xs.append(nodes)
                ys.append(v)
        if not xs:
            continue
        ax.plot(xs, ys, color=FORMAT_COLOR[fmt], linestyle=FORMAT_LINESTYLE[fmt],
                 marker=PARTITION_MARKER[partition], markersize=markersize,
                 markerfacecolor="none", linewidth=linewidth,
                 label=f"HClib-Actor ({fmt} {PARTITION_LABEL[partition]})")

    xs, ys = [], []
    for nodes, scale in zip(nodes_list, scales_list):
        v = petsc_series.get((scale, nodes))
        if v is not None:
            xs.append(nodes)
            ys.append(v)
    if xs:
        ax.plot(xs, ys, color=PETSC_COLOR, linestyle=PETSC_LINESTYLE,
                 marker=PETSC_MARKER, markersize=markersize,
                 markerfacecolor="none", linewidth=linewidth,
                 label=PETSC_LABEL)

    ax.set_xscale("log", base=2)
    ax.set_yscale("log", base=2)
    ax.margins(x=X_MARGIN, y=Y_MARGIN)
    ax.set_xticks(nodes_list)
    ax.set_xticklabels([])
    ax.tick_params(axis="x", length=0)
    ax.yaxis.set_major_formatter(mticker.FuncFormatter(pow2_formatter))
    if show_ylabel:
        ax.set_ylabel("Runtime (s)", fontsize=ylabel_fontsize)
    ax.tick_params(axis="y", labelsize=ytick_fontsize)
    ax.grid(True, which="major", color=GRID_COLOR, linewidth=0.8, zorder=0)
    ax.set_axisbelow(True)
    for spine in ax.spines.values():
        spine.set_visible(False)
    if title:
        ax.set_title(title, fontsize=title_fontsize)

    scale_row = [str(s) for s in scales_list]
    nodes_row = [str(n) for n in nodes_list]
    procs_row = [str(scale_nodes_to_procs.get((s, n), "")) for s, n in zip(scales_list, nodes_list)]
    table = add_aligned_table(
        ax, nodes_list, ["Scale", "Nodes", "Procs"], [scale_row, nodes_row, procs_row],
        y0=-0.32, height=0.26, fontsize=table_fontsize,
    )
    for (row, col), cell in table.get_celld().items():
        cell.set_linewidth(0)
        cell.get_text().set_fontsize(table_fontsize)
        if col == -1:
            cell.set_facecolor("none")
        elif row == 0:
            cell.set_facecolor(SCALE_ROW_COLOR)
        elif row == 1:
            cell.set_facecolor(NODES_ROW_COLOR)
        else:
            cell.set_facecolor(PROCS_ROW_COLOR)

    return ax.get_legend_handles_labels()


def plot_family(family):
    cfg = FAMILIES[family]
    actor_series, petsc_series, scale_nodes_to_procs = collect_family_data(family)
    if not actor_series and not petsc_series:
        print(f"  [warn] no data found for {family}, skipping plot")
        return

    fig, axes = plt.subplots(1, 3, figsize=(18, 6.5), dpi=150)
    legend_entries = {}
    for i, (scale_start, max_k) in enumerate(cfg["ladders"]):
        subtitle = f"({chr(97 + i)}) scale {scale_start + 1}-{scale_start + max_k}"
        handles, labels = render_ladder(
            axes[i], actor_series, petsc_series, scale_nodes_to_procs, scale_start, max_k,
            title=subtitle, show_ylabel=(i == 0),
        )
        for h, l in zip(handles, labels):
            legend_entries.setdefault(l, h)

    fig.suptitle(f"{cfg['title']} weak scaling", fontsize=22, y=1.03)
    legend_order = [
        "HClib-Actor (CSR row)", "HClib-Actor (DCSR column)",
        "HClib-Actor (DCSC row)", "HClib-Actor (CSC column)", PETSC_LABEL,
    ]
    ordered_handles = [legend_entries[l] for l in legend_order if l in legend_entries]
    ordered_labels = [l for l in legend_order if l in legend_entries]
    fig.legend(ordered_handles, ordered_labels, fontsize=12, ncol=5, frameon=True,
               facecolor=AXES_FACECOLOR, edgecolor="#cfd3da", loc="upper center",
               bbox_to_anchor=(0.5, 1.15))

    fig.subplots_adjust(top=0.76, bottom=0.30, wspace=0.35)
    outpath = OUTDIR / f"weak_scaling_{family.replace('-', '_')}.pdf"
    fig.savefig(outpath, bbox_inches="tight")
    plt.close(fig)
    print(f"  wrote {outpath}")


def main():
    for family in FAMILIES:
        print(f"== {family} ==")
        plot_family(family)


if __name__ == "__main__":
    main()
