#include "util.h"

enum class MMType {
    PATTERN,
    NUMERIC,
    INVALID
};

void skip_mm_comments(std::ifstream& fin) {
    while (fin.peek() == '%')
        fin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

MMType parse_matrix_market_header(std::istream& in) {
    std::string line;

    // Read first non-empty line
    while (std::getline(in, line)) {
        if (!line.empty() && line[0] != '%') break;
        if (line.rfind("%%MatrixMarket", 0) == 0) break;
    }

    if (line.rfind("%%MatrixMarket", 0) != 0) {
        return MMType::INVALID;
    }

    std::istringstream iss(line);
    std::string banner, object, format, field, symmetry;

    iss >> banner >> object >> format >> field >> symmetry;

    if (banner != "%%MatrixMarket" || object != "matrix") {
        return MMType::INVALID;
    }

    if (field == "pattern") {
        return MMType::PATTERN;
    } else if (field == "real" || field == "integer" || field == "complex") {
        return MMType::NUMERIC;
    }

    return MMType::INVALID;
}


Problem* read_matrix_market(const std::string& filename, Configuration config)
{
    int64_t nnz = 0;
    int64_t m = 0;
    int64_t n = 0;

    std::ifstream fin(filename);
    if (!fin) {
        std::cerr << "Failed to open file\n";
        lgp_global_exit(1);
    }
    MMType type = parse_matrix_market_header(fin);
    if (type != MMType::NUMERIC) {
        T0_printf("Matrix must be numeric");
        lgp_global_exit(1);
    }
    skip_mm_comments(fin);
    fin >> m >> n >> nnz;
    if (m != n) {
        T0_printf("Matrix must be square");
        lgp_global_exit(1);
    }

    double* vec;
    double* out;
    int64_t local_rows;
    int64_t local_cols;
    int64_t local_vector_size;
    std::vector<Coordinate> coo;
    std::vector<double> expected_output;
    Partitioner* partitioner;

    if (config.dimension == Configuration::Dimension::ROW) {
        local_cols = n;
        if (config.partition == Configuration::Partition::BLOCK) {
            int64_t rows_per_pe = (m + THREADS - 1) / THREADS;
            int64_t row_start   = MYTHREAD * rows_per_pe;
            int64_t row_end     = std::min(row_start + rows_per_pe, m);
            local_rows  = std::max(int64_t(0), row_end - row_start);
            
            partitioner = new RowBlock(rows_per_pe, row_start, local_rows);
        } else if (config.partition == Configuration::Partition::CYCLIC) {
            partitioner = new RowCyclic();
        }
    } else if (config.dimension == Configuration::Dimension::COLUMN) {
        local_rows = m;
        if (config.partition == Configuration::Partition::BLOCK) {
            int64_t cols_per_pe = (n + THREADS - 1) / THREADS;
            int64_t col_start   = MYTHREAD * cols_per_pe;
            int64_t col_end     = std::min(col_start + cols_per_pe, n);
            local_cols  = std::max(int64_t(0), col_end - col_start);

            partitioner = new ColumnBlock(cols_per_pe, col_start, local_cols);
        } else if (config.partition == Configuration::Partition::CYCLIC) {
            partitioner = new ColumnCyclic();
        }
    }
    partitioner->allocate_vectors(m, n, local_rows, local_cols, local_vector_size, vec, out);
    expected_output.resize(local_vector_size);
    for (int i = 0; i < local_vector_size; ++i) {
        vec[i] = random_double();
        out[i] = 0.0;
        expected_output[i] = 0.0;
    }

    lgp_barrier();

    int64_t i, j;
    double v;
    while (fin >> i >> j >> v) {
        i--; j--;
        int64_t nnz_owner = partitioner->matrix_get_owner(i, j);
        if (nnz_owner == MYTHREAD) {
            coo.push_back({ partitioner->matrix_row_to_local_row(i), partitioner->matrix_col_to_local_col(j), v });
        }

        // performing SpMV for verification
        int64_t output_owner = partitioner->vector_get_owner(i);
        if (output_owner == MYTHREAD) {
            int64_t input_owner = partitioner->vector_get_owner(j);
            int64_t remote_idx = partitioner->vector_row_to_local_row(j);
            // std::cout << "Requesting remote index: " << remote_idx << " from PE: " << input_owner << std::endl;
            double remote_value = shmem_double_g(&vec[remote_idx], input_owner);
            int64_t local_row = partitioner->vector_row_to_local_row(i);
            expected_output[local_row] += remote_value * v;
        }
    }

    if (config.dimension == Configuration::Dimension::ROW) {
        std::sort(coo.begin(), coo.end(), [](const Coordinate& a, const Coordinate& b) {
            if (a.row == b.row) {
                return a.col < b.col;
            }
            return a.row < b.row;
        });
    } else if (config.dimension == Configuration::Dimension::COLUMN) {
        std::sort(coo.begin(), coo.end(), [](const Coordinate& a, const Coordinate& b) {
            if (a.col == b.col) {
                return a.row < b.row;
            }
            return a.col < b.col;
        });
    }

    return new Problem { coo, m, n, local_rows, local_cols, local_vector_size, partitioner, vec, out, expected_output };
}
