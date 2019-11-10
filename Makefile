EXECUTABLE := compress

CU_FILES   := src/jpeg.cu

CU_DEPS    :=

CC_FILES   := src/main.cpp src/jpeg-seq.cpp

all: $(EXECUTABLE) $(REFERENCE)

LOGS	   := logs

###########################################################

OBJDIR=objs
IMGDIR=images
PNGDIR=lodepng
CXX=g++ -m64
CXXFLAGS=-O3 -Wall
LDFLAGS=-L/usr/local/depot/cuda-8.0/lib64/ -lcudart
NVCC=nvcc
NVCCFLAGS=-O3 -m64 --gpu-architecture compute_35


OBJS=$(OBJDIR)/main.o  $(OBJDIR)/jpeg.o $(OBJDIR)/jpeg-seq.o $(OBJDIR)/$(PNGDIR)/lodepng.o


.PHONY: default dirs clean lodepng

default: $(EXECUTABLE)

dirs:
		mkdir -p $(OBJDIR)
		mkdir -p $(OBJDIR)/$(PNGDIR)
		mkdir -p $(IMGDIR)

clean:
		rm -rf $(OBJDIR) $(IMGDIR) *~ $(EXECUTABLE) $(LOGS)

$(EXECUTABLE): dirs $(OBJS)
		$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

$(OBJDIR)/%.o: src/%.cpp
		$(CXX) $< $(CXXFLAGS) -c -o $@

$(OBJDIR)/$(PNGDIR)/%.o: src/$(PNGDIR)/%.cpp
		$(CXX) $< $(CXXFLAGS) -c -o $@

$(OBJDIR)/%.o: src/%.cu
		$(NVCC) $< $(NVCCFLAGS) -c -o $@

