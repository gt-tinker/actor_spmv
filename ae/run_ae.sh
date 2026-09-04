#!/bin/bash
# Combined Artifact Evaluation driver for the SpMV benchmark: 4 actor
# configs (row_block CSR, row_block DCSC, column_block CSC, column_block
# DCSR) plus a PETSc baseline (OpenMP disabled at build time, MPI-only,
# 24 ranks/node -- same tasks_per_node as actor, so both scale over the
# same node counts and land on the same plot).
#
#   build          install/build the HClib+bale runtime (scripts/hclib_install.sh)
#                   and PETSc (scripts/petsc_install.sh), then compile
#                   src/actors/main and src/petsc/main. Idempotent -- safe
#                   to re-run.
#   generate       build (if needed), then submit all strong- and weak-scaling
#                   slurm jobs for both actor and PETSc (full sweep, nodes up to 64)
#   generate-test  same, but restricted to N=2 and N=4 nodes only -- for a
#                   quick smoke test. Blocks until the submitted jobs leave
#                   the queue, then plots automatically.
#   generate-test-petsc  same as generate-test, but submits only the PETSc
#                   jobs (N=2,4) -- for adding/refreshing the PETSc line
#                   without re-running the actor sweep.
#   plot           parse the resulting .out files and produce plots (run once
#                   the generate jobs have finished, i.e. squeue is clear)
#
# Output layout, all self-contained under this repo:
#   results/strong/output_strong_<partition>_<format>/out_spmv_<matrix>/...
#   results/strong/output_strong_petsc/out_spmv_<matrix>/...
#   results/weak/output_weak_<partition>_<format>/...
#   results/weak/output_weak_petsc/...
#   plots/{GAP-*.png, GAP_combined.pdf, weak_scaling_erdos_renyi.pdf, weak_scaling_rmat.pdf}
#   (PETSc is plotted as an extra line on each of the above, not separate files)
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
AE_ROOT="$(cd "$HERE/.." && pwd)"
BIN="$AE_ROOT/src/actors/main"
PETSC_BIN="$AE_ROOT/src/petsc/main"

usage() {
    echo "Usage: $0 {build|generate|generate-test|generate-test-petsc|plot}" >&2
    exit 1
}

# Sources scripts/hclib_install.sh in *this* shell (no subshell). Its clone
# and build steps are gated on `[ ! -d bale ]` / `[ ! -d hclib ]` so this is
# cheap/no-op once those exist -- but it must run on *every* invocation
# (not just the first build) because it's also what exports BALE_INSTALL,
# HCLIB_ROOT, and LD_LIBRARY_PATH into this shell, which sbatch then
# inherits for the jobs generate/generate-test submits next. Skipping this
# just because $BIN already exists (e.g. from a prior run) leaves those
# unset, and every srun task then fails with "libhclib.so.0: cannot open
# shared object file".
setup_env() {
    echo "== installing HClib/bale runtime (scripts/hclib_install.sh) =="
    local prev_dir="$PWD"
    cd "$AE_ROOT/scripts"
    set +u
    source hclib_install.sh
    set -u
    cd "$prev_dir"
}

build_actor() {
    setup_env
    echo "== building src/actors/main =="
    local prev_dir="$PWD"
    cd "$AE_ROOT/src/actors"
    make
    cd "$prev_dir"
}

ensure_built() {
    setup_env
    if [ ! -x "$BIN" ]; then
        echo "== building src/actors/main =="
        local prev_dir="$PWD"
        cd "$AE_ROOT/src/actors"
        make
        cd "$prev_dir"
    fi
}

# Same pattern as setup_env, for scripts/petsc_install.sh. Also always
# runs, for the same LD_LIBRARY_PATH-propagation reason as setup_env above
# (PETSC_DIR/PETSC_ARCH/LD_LIBRARY_PATH must be set in this shell before
# sbatch is called). Unsets CC/CXX first: if setup_env (hclib) already ran
# in this shell it left CC=oshcc/CXX=oshc++ exported (OpenSHMEM compiler
# wrappers), which would otherwise leak into PETSc's ./configure and break
# it -- PETSc auto-detects plain mpicc/mpicxx from PATH once unset.
setup_petsc_env() {
    echo "== installing PETSc runtime (scripts/petsc_install.sh) =="
    local prev_dir="$PWD"
    cd "$AE_ROOT/scripts"
    unset CC CXX
    set +u
    source petsc_install.sh
    set -u
    cd "$prev_dir"
}

build_petsc() {
    setup_petsc_env
    echo "== building src/petsc/main =="
    local prev_dir="$PWD"
    cd "$AE_ROOT/src/petsc"
    make
    cd "$prev_dir"
}

ensure_petsc_built() {
    setup_petsc_env
    if [ ! -x "$PETSC_BIN" ]; then
        echo "== building src/petsc/main =="
        local prev_dir="$PWD"
        cd "$AE_ROOT/src/petsc"
        make
        cd "$prev_dir"
    fi
}

# Runs a submit script, echoes its output live (via tee), and appends every
# "Submitted batch job <id>" it prints to the global JOB_IDS array.
JOB_IDS=()
run_and_collect() {
    local logfile
    logfile=$(mktemp)
    "$@" | tee "$logfile"
    local id
    while read -r id; do
        JOB_IDS+=("$id")
    done < <(grep -oE 'Submitted batch job [0-9]+' "$logfile" | awk '{print $4}')
    rm -f "$logfile"
}

# Blocks until none of the given job IDs are left in squeue. Checks
# squeue's exit code, not just its output line count: a transient squeue
# failure (scheduler hiccup, network blip) also prints nothing to stdout,
# and treating that the same as "0 jobs left" would falsely declare the
# wait done while jobs are still queued/running.
wait_for_jobs() {
    local ids=("$@")
    if [ ${#ids[@]} -eq 0 ]; then
        echo "no jobs to wait on"
        return
    fi
    local idlist
    idlist=$(IFS=,; echo "${ids[*]}")
    echo "waiting on ${#ids[@]} jobs: $idlist"
    while true; do
        local out rc remaining
        out=$(squeue -h -j "$idlist" -o '%A' 2>&1)
        rc=$?
        if [ "$rc" -ne 0 ]; then
            echo "  squeue query failed (rc=$rc), retrying: $out"
            sleep 30
            continue
        fi
        remaining=$(grep -c '^[0-9]' <<< "$out")
        if [ "$remaining" -eq 0 ]; then
            break
        fi
        echo "  $remaining/${#ids[@]} jobs still in queue/running..."
        sleep 30
    done
    echo "all jobs finished"
}

case "${1:-}" in
    build)
        build_actor
        build_petsc
        ;;
    generate)
        ensure_built
        ensure_petsc_built
        "$HERE/submit_strong.sh"
        "$HERE/submit_weak.sh"
        "$HERE/submit_strong_petsc.sh"
        "$HERE/submit_weak_petsc.sh"
        echo "Jobs submitted. Check with 'squeue -u \$USER'; once clear, run: $0 plot"
        ;;
    generate-test)
        ensure_built
        ensure_petsc_built
        export MIN_NODES=2
        export MAX_NODES=4
        run_and_collect "$HERE/submit_strong.sh"
        run_and_collect "$HERE/submit_weak.sh"
        run_and_collect "$HERE/submit_strong_petsc.sh"
        run_and_collect "$HERE/submit_weak_petsc.sh"
        unset MIN_NODES MAX_NODES
        echo "Test jobs submitted (nodes restricted to 2 and 4)."
        wait_for_jobs "${JOB_IDS[@]}"
        echo "== plotting =="
        python3 "$HERE/plot_strong.py"
        python3 "$HERE/plot_weak.py"
        ;;
    generate-test-petsc)
        ensure_petsc_built
        export MIN_NODES=2
        export MAX_NODES=4
        run_and_collect "$HERE/submit_strong_petsc.sh"
        run_and_collect "$HERE/submit_weak_petsc.sh"
        unset MIN_NODES MAX_NODES
        echo "PETSc test jobs submitted (nodes restricted to 2 and 4)."
        wait_for_jobs "${JOB_IDS[@]}"
        echo "== plotting =="
        python3 "$HERE/plot_strong.py"
        python3 "$HERE/plot_weak.py"
        ;;
    plot)
        python3 "$HERE/plot_strong.py"
        python3 "$HERE/plot_weak.py"
        ;;
    *)
        usage
        ;;
esac
