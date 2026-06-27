#ifndef IMPLEMENTATIONS_H
#define IMPLEMENTATIONS_H

double column_csc(Problem* problem, CSC* mtx, int run_number);
double column_csr(Problem* problem, CSR* mtx, int run_number);
double row_csr(Problem* problem, CSR* mtx, int run_number);
double row_csc(Problem* problem, CSC* mtx, int run_number);

#endif // COLUMN_CYCLIC_CSC_H