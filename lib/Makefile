
GP_ROOT =  ..

CUDAPATH = /usr/local/cuda
CUDUMP = $(CUDAPATH)/bin/nvdisasm
CUDUMPFLAGS = --print-code 
# --print-line-info 
# --print-instruction-encoding
# --print-life-ranges --life-range-mode wide

CUCC = $(CUDAPATH)/bin/nvcc
CXX = g++

INCLUDE =  -I$(GP_ROOT)/include  -I$(CUDAPATH)/include \
 -I$(CUDAPATH)/extras/CUPTI/include

ASM_WSRC = $(GP_ROOT)/util/asm-w-src
GPU_GET_PATH = $(GP_ROOT)/util/gpu-get-cc

BUILD_CC_FILE_PATH = $(CURDIR)/build-cc
ifneq ($(MAKECMDGOALS),clean)
  DUMMY := $(shell  $(MAKE) -C $(dir $(GPU_GET_PATH)))
  GPU_ARCH := $(shell $(GPU_GET_PATH) $(BUILD_CC_FILE_PATH))
endif

# Optimization Flag (Omitted from debug builds by make rules.)
#
OPT_FLAG = -O3

# Recognized both by nvcc and g++
#
IDENTICAL_FLAGS = -g $(INCLUDE)

# Used both by nvcc and g++, but requires an -Xcompiler prefix for nvcc
#
COMMON_FLAGS = -Wall -Wno-parentheses  -fopenmp -march=native

# Used only by nvcc.
#
CUCC_ONLY_FLAGS = -std c++11 --gpu-architecture=$(GPU_ARCH) \
   --ptxas-options=-v -lineinfo

# Used only by g++
#
CXX_ONLY_FLAGS = -std=c++11 $(shell Magick++-config --cppflags --cxxflags)

COMMON_FLAGS_PREFIXED = $(COMMON_FLAGS:%=-Xcompiler %)

CXXFLAGS = $(IDENTICAL_FLAGS) $(COMMON_FLAGS) $(CXX_ONLY_FLAGS)
CUFLAGS = $(IDENTICAL_FLAGS) $(COMMON_FLAGS_PREFIXED) $(CUCC_ONLY_FLAGS)

LIBRARIES = -lrt -lcuda \
  -L$(CUDAPATH)/extras/CUPTI/lib64 \
  -Xlinker -rpath,$(CUDAPATH)/extras/CUPTI/lib64 -lcupti -lnvidia-ml


SRC_FILES = nperf.cc

default: libnperf.a

libnperf.a: nperf.cc $(BUILD_CC_FILE_PATH) Makefile
	$(CXX) $(CXXFLAGS) $(OPT_FLAG) $< -c -fPIC -o lib.o
	ar r libnperf.a lib.o
	echo -e "#!/bin/sh\necho $(LIBRARIES) -L$(realpath .) -lnperf " \
	> ld-flags
	chmod 0755 ld-flags

$(BUILD_CC_FILE_PATH):
	;

# Include dependencies that were created by %.d rules.
#
ifneq ($(MAKECMDGOALS),clean)
-include $(SRC_FILES:=.d)
endif

# Prepare file holding dependencies, to be included in this file.
#
%.cc.d: %.cc
	@set -e; rm -f $@; \
         $(CXX) -M $(CXXFLAGS) $< > $@.$$$$; \
         sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
         rm -f $@.$$$$

%.cu.d: %.cu
	@set -e; rm -f $@; \
         $(CUCC) -M $(CUFLAGS) $< > $@.$$$$; \
         sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
         rm -f $@.$$$$

%.o: %.cc Makefile
	$(CXX) $(CXXFLAGS) $(OPT_FLAG) -S $*.cc \
	  -fverbose-asm -o - | $(ASM_WSRC) > $*.s
	$(CXX) $(CXXFLAGS) -c $*.s
	$(CXX) $(CXXFLAGS) -c $*.cc -o $*-debug.o

%.o: %.cu Makefile $(BUILD_CC_FILE_PATH)
	$(CUCC) $(CUFLAGS) -c $*.cu -o $*-debug.o
	$(CUCC) $(CUFLAGS) $(OPT_FLAG) -c $*.cu
	$(CUCC) $(CUFLAGS) -ptx $*.cu -o $*.ptx
	$(CUCC) $(CUFLAGS) -cubin $*.ptx -o $*.cubin
	$(CUDUMP) $(CUDUMPFLAGS) $*.cubin > $*.sass

clean:
	/bin/rm -f $(BUILD_CC_FILE_PATH) *.o *.ptx *.sass *.cubin *.d \
	*.a ld-flags \
	$(all) $(all:=-debug) *.s *.d.[0-9][0-9][0-9][0-9][0-9]
