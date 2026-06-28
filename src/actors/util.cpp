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

struct FilePkt {
    int64_t row;
    int64_t col;
    double val;
};

// File reading based on https://github.com/singhalshubh/imm_hclib/blob/main/src/graph.h
class FileSelector: public hclib::Selector<1, FilePkt> { 
    std::vector<Coordinate>& coo_;
public:
    FileSelector(std::vector<Coordinate>& coo) : coo_(coo) {
        mb[0].process = [this] (FilePkt pkt, int sender_rank) { 
            this->req_process(pkt, sender_rank);
        };
    }

private:
    void req_process(FilePkt pkg, int sender_rank) {
        coo_.push_back({ pkg.row, pkg.col, pkg.val });
    }
};

Problem* read_matrix_market(const std::string& filename, Configuration config)
{
    int64_t nnz = 0;
    int64_t m = 0;
    int64_t n = 0;

    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Failed to open file\n";
        lgp_global_exit(1);
    }
    MMType type = parse_matrix_market_header(file);
    skip_mm_comments(file);
    std::string line;
    std::getline(file, line);
    std::stringstream ss(line);
    ss >> m >> n >> nnz;

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

    std::streampos current = file.tellg();

    FileSelector* fileSelector = new FileSelector(coo);
    hclib::finish([&]() {
        fileSelector->start();
        
        file.seekg(0, std::ios::end);
        std::streampos file_end = file.tellg();
        file.seekg(current);
        std::streamoff remaining = file_end - current;

        std::streamoff start =
            current +
            (remaining * MYTHREAD) / THREADS;

        std::streamoff end =
            current +
            (remaining * (MYTHREAD + 1)) / THREADS;

        // Number of bytes assigned to this thread
        std::streamoff size = end - start;

        file.seekg(start);
        if (MYTHREAD != 0) {
            file.seekg(start - 1);
            char prev = file.get();
            if (prev != '\n') {
                std::getline(file, line);   // discard partial line
            }
        }

        int64_t read_lines = 0;

        while (std::getline(file, line)) {
            FilePkt pkt;
            std::stringstream ss(line);
            if(type == MMType::NUMERIC) {
                ss >> pkt.row >> pkt.col >> pkt.val;
            }
            else {
                ss >> pkt.row >> pkt.col;
                pkt.val = random_double();
            }
            pkt.row--;
            pkt.col--;
            int64_t nnz_owner = partitioner->matrix_get_owner(pkt.row, pkt.col);
            pkt.row = partitioner->matrix_row_to_local_row(pkt.row);
            pkt.col = partitioner->matrix_col_to_local_col(pkt.col);

            fileSelector->send(0, pkt, nnz_owner);
            read_lines++;

            if (file.tellg() >= end) {
                break;
            }
        }
        fileSelector->done(0);
    });
    lgp_barrier();
    delete fileSelector;

    if (config.verify) {
        if (type != MMType::NUMERIC) {
            T0_printf("Can only verify numeric matrices");
            lgp_global_exit(1);
        }
        file.clear();
        file.seekg(current);
        
        int64_t i, j;
        double v;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            ss >> i >> j >> v;
            i--; j--;

            // performing SpMV for verification
            int64_t output_owner = partitioner->vector_get_owner(i);
            if (output_owner == MYTHREAD) {
                int64_t input_owner = partitioner->vector_get_owner(j);
                int64_t remote_idx = partitioner->vector_row_to_local_row(j);
                double remote_value = shmem_double_g(&vec[remote_idx], input_owner);
                int64_t local_row = partitioner->vector_row_to_local_row(i);
                expected_output[local_row] += remote_value * v;
            }
        }
    }
    lgp_barrier();

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
