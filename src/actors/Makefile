include $(HCLIB_ROOT)/../modules/bale_actor/inc/hclib_bale_actor.pre.mak
include $(HCLIB_ROOT)/include/hclib.mak
include $(HCLIB_ROOT)/../modules/bale_actor/inc/hclib_bale_actor.post.mak

SRUN ?= oshrun

main: main.cpp util.cpp implementations/column_csc.cpp implementations/column_csr.cpp implementations/row_csr.cpp implementations/row_csc.cpp

%: %.cpp
	$(CXX) -g -O3 -march=native -mtune=native -fpermissive -std=c++17 -DUSE_SHMEM=1 $(HCLIB_CFLAGS) $(HCLIB_LDFLAGS) -o $@ $^ $(HCLIB_LDLIBS) -lspmat -lconvey -lexstack -llibgetput -lhclib_bale_actor -lm
	#$(CXX) -g -O3 -std=c++11 $(HCLIB_CFLAGS) $(HCLIB_LDFLAGS) -o $@ $^ $(HCLIB_LDLIBS) -llibgetput
	#$(CXX) -g -O3 -std=c++11 -I $(HPCTOOLKIT_HOME)/include $(HCLIB_CFLAGS) $(HCLIB_LDFLAGS) -o $@ $^ $(HCLIB_LDLIBS) -L $(HPCTOOLKIT_HOME)/lib/hpctoolkit/ -lspmat -lconvey -lexstack -llibgetput  -lhclib_bale_actor -lm -lhpctoolkit
	#$(CXX) -g -O3 -std=c++11 -DUSE_LOCK $(HCLIB_CFLAGS) $(HCLIB_LDFLAGS) -o $@_lock $^ $(HCLIB_LDLIBS) -lconvey -lhclib_bale_actor_lock

clean:
	rm -f main
