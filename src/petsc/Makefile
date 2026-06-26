TARGETS= clean main

all: $(TARGETS)

%: %.cpp
	mpicxx -o $@ $^ -I${PETSC_DIR}/include \
	-I${PETSC_DIR}/${PETSC_ARCH}/include -L${PETSC_DIR}/${PETSC_ARCH}/lib -lpetsc

clean:
	rm -f $(TARGETS)