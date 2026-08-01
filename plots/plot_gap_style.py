#!/usr/bin/env python3
"""Plot GAP strong-scaling results in the visual style of rmat_weak_scaling.png:
blue/orange solid lines for CSR/CSC, green/purple dashed lines for DCSR/DCSC,
diamond markers for row-partitioned data, triangle markers for column-partitioned
data, log2 axes, and a two-row Nodes/Procs table under the x-axis.
"""
import re
import statistics
from pathlib import Path
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker

SRC = Path("/storage/scratch1/9/amandadi3/spmv_scaling/actor_spmv/src")
PETSC = SRC / "petsc"
OUTDIR = SRC.parent / "plots"
OUTDIR.mkdir(exist_ok=True)

ACTOR_DATA = Path("/storage/project/r-vsarkar9-1/shared/spmv_strong_scaling_data")
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

MATRICES = ["GAP-kron", "GAP-road", "GAP-twitter", "GAP-urand", "GAP-web"]

ACTOR_FILE_RE = re.compile(r"^out_spmv_(GAP-[a-z]+)_N(\d+)_n(\d+)$")
RUN_RE = re.compile(r"^Run \d+:\s*([\d.eE+-]+)", re.MULTILINE)
PETSC_RE = re.compile(r"^\s*\d+:\s+SpMV \d+:\s+([\d.eE+-]+)", re.MULTILINE)

# Styling sampled from rmat_weak_scaling.png:
#   CSR  -> blue,   solid, diamond (row-partitioned)
#   CSC  -> orange, solid, triangle (column-partitioned)
#   DCSR -> green,  dashed, triangle (column-partitioned)
#   DCSC -> purple, dashed, diamond (row-partitioned)
#   PETSc MPI -> black, solid, circle
FORMAT_COLOR = {"CSR": "blue", "CSC": "orange", "DCSR": "green", "DCSC": "purple"}
FORMAT_LINESTYLE = {"CSR": "-", "CSC": "-", "DCSR": "--", "DCSC": "--"}
PARTITION_MARKER = {"row_block": "D", "column_block": "^"}
PARTITION_LABEL = {"row_block": "row", "column_block": "column"}
PETSC_COLOR = "black"

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


def parse_petsc_file(path):
    text = path.read_text(errors="ignore")
    return median_from_runs(text, PETSC_RE)


def collect_matrix_data(matrix):
    nodes_to_procs = {}

    petsc_series = {}
    petsc_dir = PETSC / matrix
    if petsc_dir.is_dir():
        for f in sorted(petsc_dir.glob(f"{matrix}_*.out")):
            m = re.match(rf"^{re.escape(matrix)}_(\d+)_(\d+)\.out$", f.name)
            if not m:
                continue
            nodes, procs = int(m.group(1)), int(m.group(2))
            med, n = parse_petsc_file(f)
            if med is None or n < 10:
                print(f"  [skip] petsc {f.name}: only {n}/10 SpMV stages found")
                continue
            petsc_series[nodes] = med
            nodes_to_procs[nodes] = procs

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

    return petsc_series, actor_series, nodes_to_procs


def pow2_formatter(value, _pos):
    exp = round(mticker.math.log2(value))
    return f"$2^{{{exp}}}$"


def render_axes(ax, matrix, petsc_series, actor_series, nodes_to_procs, *,
                 title=None, title_fontsize=16, ylabel_fontsize=20,
                 ytick_fontsize=19, table_fontsize=15, table_bbox=None,
                 linewidth=2, markersize=6, show_ylabel=True):
    ax.set_facecolor(AXES_FACECOLOR)

    if petsc_series:
        nodes = sorted(petsc_series)
        ax.plot(nodes, [petsc_series[n] for n in nodes],
                color=PETSC_COLOR, linestyle="-", marker="o",
                markersize=markersize, markerfacecolor="none", linewidth=linewidth,
                label="PETSc MPI")

    for (partition, fmt), series in sorted(actor_series.items()):
        nodes = sorted(series)
        ax.plot(nodes, [series[n] for n in nodes],
                color=FORMAT_COLOR[fmt], linestyle=FORMAT_LINESTYLE[fmt],
                marker=PARTITION_MARKER[partition], markersize=markersize,
                markerfacecolor="none", linewidth=linewidth,
                label=f"HClib-Actor ({fmt} {PARTITION_LABEL[partition]})")

    all_nodes = sorted(set(petsc_series) | {n for s in actor_series.values() for n in s})

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
    if table_bbox is None:
        table_bbox = [X_MARGIN, -0.20, 1 - 2 * X_MARGIN, 0.16]
    table = ax.table(
        cellText=[nodes_row, procs_row],
        rowLabels=["Nodes", "Procs"],
        cellLoc="center",
        bbox=table_bbox,
    )
    table.auto_set_font_size(False)
    table.set_fontsize(table_fontsize)
    for (row, col), cell in table.get_celld().items():
        cell.set_linewidth(0)
        cell.get_text().set_fontweight("normal")
        cell.get_text().set_fontsize(table_fontsize)
        if col == -1:
            # row-label cell ("Nodes" / "Procs") stays uncolored
            cell.set_facecolor("none")
        elif row == 0:
            cell.set_facecolor(NODES_ROW_COLOR)
        else:
            cell.set_facecolor(PROCS_ROW_COLOR)

    return ax.get_legend_handles_labels()


def plot_matrix(matrix, petsc_series, actor_series, nodes_to_procs):
    fig, ax = plt.subplots(figsize=(7, 6.5), dpi=150)
    render_axes(ax, matrix, petsc_series, actor_series, nodes_to_procs)

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
    for i, matrix in enumerate(MATRICES):
        row, col = COMBINED_GRID_POSITIONS[i]
        ax = fig.add_subplot(gs[row, col])
        row_first_ax.setdefault(row, ax)
        petsc_series, actor_series, nodes_to_procs = data_by_matrix[matrix]
        handles, labels = render_axes(
            ax, matrix, petsc_series, actor_series, nodes_to_procs,
            title=f"({SUBPLOT_LABELS[i]}) {matrix}", title_fontsize=20,
            ytick_fontsize=16, table_fontsize=14, show_ylabel=False,
            table_bbox=[X_MARGIN, COMBINED_TABLE_BOTTOM_FRAC, 1 - 2 * X_MARGIN, 0.16],
            linewidth=1.8, markersize=6,
        )
        for h, l in zip(handles, labels):
            legend_entries.setdefault(l, h)

    # One shared "Runtime (s)" label per row instead of one per subplot.
    # Center it on the full visual block (axes + Nodes/Procs table below),
    # not just the bare axes, so it lines up with what the reader sees.
    for ax in row_first_ax.values():
        pos = ax.get_position()
        h = pos.y1 - pos.y0
        visual_bottom = pos.y0 + COMBINED_TABLE_BOTTOM_FRAC * h
        center = (pos.y1 + visual_bottom) / 2
        fig.text(pos.x0 - 0.075, center, "Runtime (s)",
                  rotation=90, va="center", ha="center", fontsize=20)

    fig.suptitle("GAP Benchmark matrices", fontsize=28, y=0.995)
    # matplotlib fills a multi-column legend column-major (fills column 1
    # top-to-bottom, then column 2), so the flat order is column 1 in full
    # followed by column 2: [CSR row, DCSR column, DCSC row, CSC column,
    # PETSc MPI].
    legend_order = [
        "HClib-Actor (CSR row)", "HClib-Actor (DCSR column)",
        "HClib-Actor (DCSC row)", "HClib-Actor (CSC column)",
        "PETSc MPI",
    ]
    ordered_handles = [legend_entries[label] for label in legend_order]
    fig.legend(ordered_handles, legend_order,
               fontsize=16, ncol=2, frameon=True, facecolor=AXES_FACECOLOR,
               edgecolor="#cfd3da", loc="upper center",
               bbox_to_anchor=(0.5, 0.945))

    fig.subplots_adjust(top=0.81, bottom=0.06, left=0.1)
    outpath = OUTDIR / "GAP_combined.pdf"
    fig.savefig(outpath, bbox_inches="tight")
    plt.close(fig)
    print(f"  wrote {outpath}")


def main():
    data_by_matrix = {}
    for matrix in MATRICES:
        print(f"== {matrix} ==")
        petsc_series, actor_series, nodes_to_procs = collect_matrix_data(matrix)
        data_by_matrix[matrix] = (petsc_series, actor_series, nodes_to_procs)
        plot_matrix(matrix, petsc_series, actor_series, nodes_to_procs)

    print("== combined ==")
    plot_combined(data_by_matrix)


if __name__ == "__main__":
    main()
