#!/usr/bin/env python3
"""Plot GAP strong-scaling results (4 actor configs + PETSc) by parsing the
generated .out files under $AE_ROOT/results/strong. Adapted from
actor_spmv/plots/plot_gap_style.py, with paths made self-contained under
this AE package.
"""
import re
import statistics
from pathlib import Path
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker

AE_ROOT = Path(__file__).resolve().parent.parent
ACTOR_DATA = AE_ROOT / "results" / "strong"
OUTDIR = AE_ROOT / "plots"
OUTDIR.mkdir(exist_ok=True, parents=True)

# variant directory name -> (partition, format)
# format/dimension -> label follows src/actors/main.cpp exactly:
#   CSC + COLUMN  -> CSC    CSC + ROW    -> DCSC
#   CSR + COLUMN  -> DCSR   CSR + ROW    -> CSR
ACTOR_VARIANTS = {
    "output_strong_row_block_csr":    ("row_block", "CSR"),
    "output_strong_row_block_csc":    ("row_block", "DCSC"),
    "output_strong_column_block_csc": ("column_block", "CSC"),
    "output_strong_column_block_csr": ("column_block", "DCSR"),
}

# PETSc: single MPI-only config (OpenMP disabled at build, 24 ranks/node),
# same file-naming convention as actor but no partition/format dimension.
PETSC_STRONG_DIR = "output_strong_petsc"

MATRICES = ["GAP-kron", "GAP-road", "GAP-twitter", "GAP-urand", "GAP-web"]

ACTOR_FILE_RE = re.compile(r"^out_spmv_(GAP-[a-z]+)_N(\d+)_n(\d+)$")
RUN_RE = re.compile(r"^Run \d+:\s*([\d.eE+-]+)", re.MULTILINE)

# Styling sampled from rmat_weak_scaling.png:
#   CSR  -> blue,   solid, diamond (row-partitioned)
#   CSC  -> orange, solid, triangle (column-partitioned)
#   DCSR -> green,  dashed, triangle (column-partitioned)
#   DCSC -> purple, dashed, diamond (row-partitioned)
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
NODES_ROW_COLOR = "#e8e8e8"
PROCS_ROW_COLOR = "#ffd6e7"


def median_from_runs(text, pattern):
    vals = [float(m.group(1)) for m in pattern.finditer(text)]
    if not vals:
        return None, 0
    return statistics.median(vals), len(vals)


def parse_actor_file(path):
    text = path.read_text(errors="ignore")
    return median_from_runs(text, RUN_RE)


def collect_matrix_data(matrix):
    nodes_to_procs = {}
    actor_series = {}
    for variant_dir, (partition, fmt) in ACTOR_VARIANTS.items():
        matrix_dir = ACTOR_DATA / variant_dir / f"out_spmv_{matrix}"
        if not matrix_dir.is_dir():
            continue
        for f in sorted(matrix_dir.glob(f"out_spmv_{matrix}_N*_n*")):
            m = ACTOR_FILE_RE.match(f.name)
            if not m:
                continue
            _, nodes, procs = m.groups()
            nodes, procs = int(nodes), int(procs)
            med, n = parse_actor_file(f)
            if med is None or n < 10:
                print(f"  [skip] actor {variant_dir}/{f.name}: only {n}/10 runs found")
                continue
            key = (partition, fmt)
            actor_series.setdefault(key, {})[nodes] = med
            nodes_to_procs[nodes] = procs

    petsc_series = {}
    petsc_dir = ACTOR_DATA / PETSC_STRONG_DIR / f"out_spmv_{matrix}"
    if petsc_dir.is_dir():
        for f in sorted(petsc_dir.glob(f"out_spmv_{matrix}_N*_n*")):
            m = ACTOR_FILE_RE.match(f.name)
            if not m:
                continue
            _, nodes, procs = m.groups()
            nodes, procs = int(nodes), int(procs)
            med, n = parse_actor_file(f)
            if med is None or n < 10:
                print(f"  [skip] petsc {PETSC_STRONG_DIR}/{f.name}: only {n}/10 runs found")
                continue
            petsc_series[nodes] = med
            nodes_to_procs[nodes] = procs

    return actor_series, petsc_series, nodes_to_procs


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


def render_axes(ax, matrix, actor_series, petsc_series, nodes_to_procs, *,
                 title=None, title_fontsize=16, ylabel_fontsize=20,
                 ytick_fontsize=19, table_fontsize=15, table_y0=None,
                 linewidth=2, markersize=6, show_ylabel=True, show_row_labels=True):
    ax.set_facecolor(AXES_FACECOLOR)

    for (partition, fmt), series in sorted(actor_series.items()):
        nodes = sorted(series)
        ax.plot(nodes, [series[n] for n in nodes],
                color=FORMAT_COLOR[fmt], linestyle=FORMAT_LINESTYLE[fmt],
                marker=PARTITION_MARKER[partition], markersize=markersize,
                markerfacecolor="none", linewidth=linewidth,
                label=f"HClib-Actor ({fmt} {PARTITION_LABEL[partition]})")

    if petsc_series:
        nodes = sorted(petsc_series)
        ax.plot(nodes, [petsc_series[n] for n in nodes],
                color=PETSC_COLOR, linestyle=PETSC_LINESTYLE,
                marker=PETSC_MARKER, markersize=markersize,
                markerfacecolor="none", linewidth=linewidth,
                label=PETSC_LABEL)

    all_nodes = sorted({n for s in actor_series.values() for n in s} | set(petsc_series))

    ax.set_xscale("log", base=2)
    ax.set_yscale("log", base=2)
    ax.margins(x=X_MARGIN, y=Y_MARGIN)
    ax.set_xticks(all_nodes)
    ax.set_xticklabels([])
    ax.tick_params(axis="x", length=0)
    ax.yaxis.set_major_formatter(mticker.FuncFormatter(pow2_formatter))
    if show_ylabel:
        ax.set_ylabel("Runtime (s)", fontsize=ylabel_fontsize, fontweight="normal")
    ax.tick_params(axis="y", labelsize=ytick_fontsize)
    plt.setp(ax.get_yticklabels(), fontweight="bold")
    ax.grid(True, which="major", color=GRID_COLOR, linewidth=0.8, zorder=0)
    ax.set_axisbelow(True)
    for spine in ax.spines.values():
        spine.set_visible(False)

    if title:
        ax.set_title(title, fontsize=title_fontsize)

    # Two-row Nodes/Procs table under the x-axis, columns aligned with the
    # (evenly log2-spaced) node tick positions.
    procs_row = [str(nodes_to_procs.get(n, "")) for n in all_nodes]
    nodes_row = [str(n) for n in all_nodes]
    if table_y0 is None:
        table_y0 = -0.20
    table = add_aligned_table(
        ax, all_nodes, ["Nodes", "Procs"] if show_row_labels else None, [nodes_row, procs_row],
        y0=table_y0, height=0.16, fontsize=table_fontsize,
    )
    for (row, col), cell in table.get_celld().items():
        cell.set_linewidth(0)
        cell.get_text().set_fontweight("normal")
        cell.get_text().set_fontsize(table_fontsize)
        if col == -1:
            cell.set_facecolor("none")
        elif row == 0:
            cell.set_facecolor(NODES_ROW_COLOR)
        else:
            cell.set_facecolor(PROCS_ROW_COLOR)

    return ax.get_legend_handles_labels()


def plot_matrix(matrix, actor_series, petsc_series, nodes_to_procs):
    fig, ax = plt.subplots(figsize=(7, 6.5), dpi=150)
    render_axes(ax, matrix, actor_series, petsc_series, nodes_to_procs)

    fig.suptitle(matrix, fontsize=16, y=0.970)
    ax.legend(fontsize=11, ncol=2, frameon=True, facecolor=AXES_FACECOLOR,
              edgecolor="#cfd3da", loc="upper center",
              bbox_to_anchor=(0.5, 1.252))

    fig.subplots_adjust(top=0.763, bottom=0.15)
    outpath = OUTDIR / f"{matrix}.png"
    fig.savefig(outpath, bbox_inches="tight")
    plt.close(fig)
    print(f"  wrote {outpath}")


# 2 rows x 6 cols grid: top row has 3 subplots spanning 2 cols each; bottom
# row has 2 subplots spanning 2 cols each, centered under the top row so both
# rows read as evenly spaced.
COMBINED_GRID_POSITIONS = [
    (0, slice(0, 2)), (0, slice(2, 4)), (0, slice(4, 6)),
    (1, slice(1, 3)), (1, slice(3, 5)),
]
SUBPLOT_LABELS = "abcde"

COMBINED_TABLE_BOTTOM_FRAC = -0.24


def plot_combined(data_by_matrix):
    fig = plt.figure(figsize=(17, 12), dpi=150)
    gs = fig.add_gridspec(2, 6, hspace=0.55, wspace=0.35)

    legend_entries = {}
    row_first_ax = {}
    present_matrices = [m for m in MATRICES if m in data_by_matrix]
    for i, matrix in enumerate(present_matrices):
        row, col = COMBINED_GRID_POSITIONS[i]
        ax = fig.add_subplot(gs[row, col])
        row_first_ax.setdefault(row, ax)
        actor_series, petsc_series, nodes_to_procs = data_by_matrix[matrix]
        handles, labels = render_axes(
            ax, matrix, actor_series, petsc_series, nodes_to_procs,
            title=f"({SUBPLOT_LABELS[i]}) {matrix}", title_fontsize=20,
            ytick_fontsize=16, table_fontsize=14, show_ylabel=False,
            table_y0=COMBINED_TABLE_BOTTOM_FRAC, show_row_labels=False,
            linewidth=1.8, markersize=6,
        )
        for h, l in zip(handles, labels):
            legend_entries.setdefault(l, h)

    # Must happen before the row_first_ax loop below: it reads each axes'
    # get_position() to place text relative to it, and subplots_adjust
    # changes those positions -- computing text positions from the
    # pre-adjustment layout would anchor them to stale coordinates.
    fig.subplots_adjust(top=0.81, bottom=0.06, left=0.1)

    # Each subplot's per-axes row-label column was suppressed above
    # (show_row_labels=False) because its rendered width doesn't fit in
    # these narrow, tightly-packed subplots without overlapping the
    # previous one -- render "Nodes"/"Procs" once per row instead, same
    # pattern as the shared "Runtime (s)" label below.
    table_height = 0.16
    for ax in row_first_ax.values():
        pos = ax.get_position()
        h = pos.y1 - pos.y0
        visual_bottom = pos.y0 + COMBINED_TABLE_BOTTOM_FRAC * h
        center = (pos.y1 + visual_bottom) / 2
        fig.text(pos.x0 - 0.075, center, "Runtime (s)",
                  rotation=90, va="center", ha="center", fontsize=20)

        table_bottom = pos.y0 + COMBINED_TABLE_BOTTOM_FRAC * h
        table_h = table_height * h
        # 0.035 clears the widest y-tick labels (e.g. "$2^{-7}$") in this
        # figure, measured empirically -- the bottommost tick label sits
        # close in y to this table and would otherwise collide with it.
        fig.text(pos.x0 - 0.035, table_bottom + 0.75 * table_h, "Nodes",
                  va="center", ha="right", fontsize=14)
        fig.text(pos.x0 - 0.035, table_bottom + 0.25 * table_h, "Procs",
                  va="center", ha="right", fontsize=14)

    fig.suptitle("GAP Benchmark matrices (strong scaling)", fontsize=28, y=0.995)
    legend_order = [
        "HClib-Actor (CSR row)", "HClib-Actor (DCSR column)",
        "HClib-Actor (DCSC row)", "HClib-Actor (CSC column)", PETSC_LABEL,
    ]
    ordered_handles = [legend_entries[label] for label in legend_order if label in legend_entries]
    ordered_labels = [label for label in legend_order if label in legend_entries]
    fig.legend(ordered_handles, ordered_labels,
               fontsize=16, ncol=2, frameon=True, facecolor=AXES_FACECOLOR,
               edgecolor="#cfd3da", loc="upper center",
               bbox_to_anchor=(0.5, 0.945))

    outpath = OUTDIR / "GAP_combined.pdf"
    fig.savefig(outpath, bbox_inches="tight")
    plt.close(fig)
    print(f"  wrote {outpath}")


def main():
    data_by_matrix = {}
    for matrix in MATRICES:
        print(f"== {matrix} ==")
        actor_series, petsc_series, nodes_to_procs = collect_matrix_data(matrix)
        if not actor_series and not petsc_series:
            print(f"  [warn] no data found for {matrix}, skipping plot")
            continue
        data_by_matrix[matrix] = (actor_series, petsc_series, nodes_to_procs)
        plot_matrix(matrix, actor_series, petsc_series, nodes_to_procs)

    if data_by_matrix:
        print("== combined ==")
        plot_combined(data_by_matrix)


if __name__ == "__main__":
    main()
