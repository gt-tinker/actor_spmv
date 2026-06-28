#include "util.h"
#include "matrix_formats/csc.h"
#include "matrix_formats/csr.h"
#include "implementations/implementations.h"

void run_spmv(Problem* problem, const std::function<double(int)>& spmv, bool verify) {
    if (verify) {
        T0_fprintf(stderr, "Verifying correctness: \n");
        spmv(-1);
        lgp_barrier();

        for (int64_t i = 0; i < problem->local_vector_size; i++) {
            double expected = problem->expected_output[i];
            double output = problem->out[i];
            bool is_equal = nearly_equal(expected, output);
            if (!is_equal) {
                std::cerr << "[PE" << MYTHREAD << "] local idx " << i << "failed correctness: " << output << " vs " << expected << std::endl;
                lgp_global_exit(1);
            }
        }
        lgp_barrier();

        T0_fprintf(stderr, "Verified correctness\n");
    }

    double laptime = 0.0;
    T0_fprintf(stderr, "Warm up: \n");
    for (int i = 0; i < NUM_RUNS; i++) {
        double run = spmv(-1);
        T0_fprintf(stdout, "Warmup %i: %lf\n", i, run);
    }
    T0_fprintf(stderr, "Experiment: \n");
    for (int i = 0; i < NUM_RUNS; i++) {
        double run = spmv(i);
        T0_fprintf(stdout, "Run %i: %lf\n", i, run);
        laptime += run;
    }
    T0_fprintf(stdout, "Average: %lf\n", laptime / NUM_RUNS);
    lgp_barrier();
}

int main(int argc, char* argv[]) {
    const char *deps[] = { "system", "bale_actor" };

    hclib::launch(deps, 2, [=] {
        int64_t read_graph = 0L;           // read graph from a file
        char filename[512];
        Configuration config;
        int printhelp = 0;
        int opt; 
        while ((opt = getopt(argc, argv, "vf:p:d:m:")) != -1) {
            switch (opt) {
                case 'h': printhelp = 1; break;
                case 'f': read_graph = 1; sscanf(optarg,"%s", filename); break;
                case 'v': config.verify = true; break;
                case 'p':
                    char partition_str[256];
                    sscanf(optarg,"%s", partition_str);
                    if (strcmp(partition_str, "block") == 0) {
                        config.partition = Configuration::Partition::BLOCK;
                    } else if (strcmp(partition_str, "cyclic") == 0) {
                        config.partition = Configuration::Partition::CYCLIC;
                    } else {
                        T0_fprintf(stderr, "[ERROR]: Invalid partition type\n");
                        assert(false);
                    }
                    break;
                case 'd':
                    char dimension_str[256];
                    sscanf(optarg,"%s", dimension_str);
                    if (strcmp(dimension_str, "row") == 0) {
                        config.dimension = Configuration::Dimension::ROW;
                    } else if (strcmp(dimension_str, "column") == 0) {
                        config.dimension = Configuration::Dimension::COLUMN;
                    } else {
                        T0_fprintf(stderr, "[ERROR]: Invalid dimension type\n");
                        assert(false);
                    }
                    break;
                case 'm':
                    char format_str[256];
                    sscanf(optarg,"%s", format_str);
                    if (strcmp(format_str, "csr") == 0) {
                        config.format = Configuration::Format::CSR;
                    } else if (strcmp(format_str, "csc") == 0) {
                        config.format = Configuration::Format::CSC;
                    } else {
                        T0_fprintf(stderr, "[ERROR]: Invalid format type\n");
                        assert(false);
                    }
                    break;

                default:  break;
            }
        }

        if (!read_graph) {
            T0_fprintf(stderr, "[ERROR]: Missing file name\n");
            assert(false);
        }

        Problem* problem = read_matrix_market(filename, config);
        T0_printf("Loaded: %s\n", filename);
        lgp_barrier();

        if (config.format == Configuration::Format::CSC) {
            CSC* csc = new CSC(problem->rows, problem->cols, problem->local_cols, problem->coo);
            if (config.dimension == Configuration::Dimension::COLUMN) {
                run_spmv(problem, [=](int run_number) {
                    return column_csc(problem, csc, run_number);
                }, config.verify);
            } else if (config.dimension == Configuration::Dimension::ROW) {
                run_spmv(problem, [=](int run_number) {
                    return row_csc(problem, csc, run_number);
                }, config.verify);
            }
            delete csc;
        } else if (config.format == Configuration::Format::CSR) {
            CSR* csr = new CSR(problem->rows, problem->cols, problem->local_rows, problem->coo);
            if (config.dimension == Configuration::Dimension::COLUMN) {
                run_spmv(problem, [=](int run_number) {
                    return column_csr(problem, csr, run_number);
                }, config.verify);
            } else if (config.dimension == Configuration::Dimension::ROW) {
                run_spmv(problem, [=](int run_number) {
                    return row_csr(problem, csr, run_number);
                }, config.verify);
            }
            delete csr;
        }
        

        lgp_barrier();
        delete problem;
    });
    return 0;
}
