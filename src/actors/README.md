# Building
```
make
```


# Running
```
./main [OPTIONS]
```
| Option | Description | Default |
|--------|-------------|---------|
| `-f <file>` | Matrix Market input file path | Required |
| `-v` | Include flag to check SpMV correctness (may be very slow) | - |
| `-d <row\|column>` | Dimension to partition matrix along | Required |
| `-p <block\|cyclic>` | Partitions dimension as contiguous block or round robin | Required |
| `-m <csr\|csc>` | Store partition locally as either CSR or CSC | Required |

### Example: Row cyclic CSR
```
./main -f ../../datasets/data.mtx -v -d row -p cyclic -m csr
```
