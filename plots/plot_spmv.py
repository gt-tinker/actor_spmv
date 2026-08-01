#!/usr/bin/env python3
import re
import statistics
from pathlib import Path
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

SRC = Path("/storage/scratch1/9/amandadi3/spmv_scaling/actor_spmv/src")
PETSC = SRC / "petsc"
OUTDIR = SRC.parent / "plots"
OUTDIR.mkdir(exist_ok=True)

ACTOR_DATA = Path("/storage/project/r-vsarkar9-1/shared/spmv_strong_scaling_data")
# variant directory name -> (partition, format)
ACTOR_VARIANTS = {
    "output_strong_column_block_csc": ("column_block", "CSC"),
    "output_strong_column_block_csr": ("column_block", "CSR"),
    "output_strong_row_block_csc":    ("row_block", "CSC"),
    "output_strong_row_block_csr":    ("row_block", "CSR"),
}

MATRICES = ["GAP-kron", "GAP-road", "GAP-twitter", "GAP-urand", "GAP-web"]

ACTOR_FILE_RE = re.compile(r"^out_spmv_(GAP-[a-z]+)_N(\d+)_n(\d+)$")
RUN_RE = re.compile(r"^Run \d+:\s*([\d.eE+-]+)", re.MULTILINE)
PETSC_RE = re.compile(r"^\s*\d+:\s+SpMV \d+:\s+([\d.eE+-]+)", re.MULTILINE)

# Colors: first four categorical slots (validated all-pairs safe) by storage format,
# per dataviz skill palette.md. Petsc gets neutral dark gray/black.
FORMAT_COLOR = {
    "CSR":  "#2a78d6",  # blue
    "CSC":  "#008300",  # green
    "DCSR": "#e87ba4",  # magenta
    "DCSC": "#eda100",  # yellow
}
PETSC_COLOR = "#0b0b0b"

def median_from_runs(text, pattern, expect=10):
    vals = [float(m.group(1)) for m in pattern.finditer(text)]
    if not vals:
        return None, 0
    return statistics.median(vals), len(vals)

def parse_actor_file(path):
    text = path.read_text(errors="ignore")
    med, n = median_from_runs(text, RUN_RE)
    return med, n

def parse_petsc_file(path):
    text = path.read_text(errors="ignore")
    med, n = median_from_runs(text, PETSC_RE)
    return med, n

def collect_matrix_data(matrix):
    # petsc: {nodes: median_time}
    petsc_series = {}
    petsc_dir = PETSC / matrix
    if petsc_dir.is_dir():
        for f in sorted(petsc_dir.glob(f"{matrix}_*.out")):
            m = re.match(rf"^{re.escape(matrix)}_(\d+)_(\d+)\.out$", f.name)
            if not m:
                continue
            nodes = int(m.group(1))
            med, n = parse_petsc_file(f)
            if med is None or n < 10:
                print(f"  [skip] petsc {f.name}: only {n}/10 SpMV stages found")
                continue
            petsc_series[nodes] = med

    # actor: {(partition, fmt): {nodes: median_time}}
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
            nodes = int(nodes)
            med, n = parse_actor_file(f)
            if med is None or n < 10:
                print(f"  [skip] actor {variant_dir}/{f.name}: only {n}/10 runs found")
                continue
            key = (partition, fmt)
            actor_series.setdefault(key, {})[nodes] = med

    return petsc_series, actor_series

def plot_matrix(matrix, petsc_series, actor_series):
    fig, ax = plt.subplots(figsize=(7, 5), dpi=150)

    if petsc_series:
        nodes = sorted(petsc_series)
        ax.plot(nodes, [petsc_series[n] for n in nodes],
                color=PETSC_COLOR, linestyle="-", marker="s",
                markersize=5, linewidth=2, label="petsc")

    for (partition, fmt), series in sorted(actor_series.items()):
        nodes = sorted(series)
        color = FORMAT_COLOR.get(fmt, "#4a3aa7")
        linestyle = "-" if partition == "column_block" else "--"
        ax.plot(nodes, [series[n] for n in nodes],
                color=color, linestyle=linestyle, marker="o",
                markersize=4, linewidth=1.5,
                label=f"actor_{fmt.lower()}_{partition}")

    ax.set_xscale("log", base=2)
    ax.set_yscale("log", base=2)
    all_nodes = set(petsc_series) | {n for s in actor_series.values() for n in s}
    ax.set_xticks(sorted(all_nodes))
    ax.set_xticklabels([str(n) for n in sorted(all_nodes)])
    ax.set_xlabel("Number of nodes")
    ax.set_ylabel("Median SpMV time (s)")
    ax.set_title(matrix)
    ax.grid(True, which="both", color="#e1e0d9", linewidth=0.6, zorder=0)
    ax.set_axisbelow(True)
    for spine in ("top", "right"):
        ax.spines[spine].set_visible(False)
    ax.legend(fontsize=7, ncol=2, frameon=False, loc="best")

    fig.tight_layout()
    outpath = OUTDIR / f"{matrix}.png"
    fig.savefig(outpath)
    plt.close(fig)
    print(f"  wrote {outpath}")

def main():
    for matrix in MATRICES:
        print(f"== {matrix} ==")
        petsc_series, actor_series = collect_matrix_data(matrix)
        plot_matrix(matrix, petsc_series, actor_series)

if __name__ == "__main__":
    main()
