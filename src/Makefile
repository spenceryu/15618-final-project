EXECUTABLE := compress

CU_FILES   := jpeg.cu

CU_DEPS    :=

CC_FILES   := main.cpp jpeg-seq.cpp

all: $(EXECUTABLE) $(REFERENCE)

LOGS	   := logs

###########################################################

OBJDIR=objs
IMGDIR=images
CXX=g++ -m64
CXXFLAGS=-O3 -Wall
LDFLAGS=-L/usr/local/depot/cuda-8.0/lib64/ -lcudart
NVCC=nvcc
NVCCFLAGS=-O3 -m64 --gpu-architecture compute_35


OBJS=$(OBJDIR)/main.o  $(OBJDIR)/jpeg.o $(OBJDIR)/jpeg-seq.o


.PHONY: dirs clean

default: $(EXECUTABLE)

dirs:
		mkdir -p $(OBJDIR)
		mkdir -p $(IMGDIR)

clean:
		rm -rf $(OBJDIR) $(IMGDIR) *~ $(EXECUTABLE) $(LOGS)

$(EXECUTABLE): dirs $(OBJS)
		$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

$(OBJDIR)/%.o: %.cpp
		$(CXX) $< $(CXXFLAGS) -c -o $@

$(OBJDIR)/%.o: %.cu
		$(NVCC) $< $(NVCCFLAGS) -c -o $@
