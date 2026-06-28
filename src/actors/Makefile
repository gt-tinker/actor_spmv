include $(HCLIB_ROOT)/../modules/bale_actor/inc/hclib_bale_actor.pre.mak
include $(HCLIB_ROOT)/include/hclib.mak
include $(HCLIB_ROOT)/../modules/bale_actor/inc/hclib_bale_actor.post.mak

SRUN ?= oshrun

main: main.cpp util.cpp implementations/column_csc.cpp implementations/column_csr.cpp implementations/row_csr.cpp implementations/row_csc.cpp implementations/row_csr_opt.cpp

%: %.cpp
	$(CXX) -g -O3 -march=native -mtune=native -fpermissive -std=c++17 -DUSE_SHMEM=1 $(HCLIB_CFLAGS) $(HCLIB_LDFLAGS) -o $@ $^ $(HCLIB_LDLIBS) -lspmat -lconvey -lexstack -llibgetput -lhclib_bale_actor -lm

clean:
	rm -f main
