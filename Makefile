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
CXXFLAGSDEBUG=-ggdb -O0 -Wall -std=c++11
CXXFLAGS=-O3 -Wall -std=c++11
LDFLAGS=-L/usr/local/depot/cuda-8.0/lib64/ -lcudart
NVCC=nvcc
NVCCFLAGSDEBUG=-g -G -m64 --gpu-architecture compute_35
NVCCFLAGS=-O3 -m64 --gpu-architecture compute_35


OBJS=$(OBJDIR)/main.o  $(OBJDIR)/jpeg.o $(OBJDIR)/jpeg-seq.o $(OBJDIR)/$(PNGDIR)/lodepng.o $(OBJDIR)/dct.o $(OBJDIR)/image.o $(OBJDIR)/quantize.o $(OBJDIR)/rle.o $(OBJDIR)/dpcm.o


.PHONY: default dirs clean lodepng

default: $(EXECUTABLE)

dirs:
		mkdir -p $(OBJDIR)
		mkdir -p $(OBJDIR)/$(PNGDIR)
		mkdir -p $(IMGDIR)

clean:
		rm -rf $(OBJDIR) $(IMGDIR) *~ $(EXECUTABLE) $(LOGS)

$(EXECUTABLE): dirs $(OBJS)
		$(CXX) $(CXXFLAGSDEBUG) -o $@ $(OBJS) $(LDFLAGS)

$(OBJDIR)/%.o: src/%.cpp
		$(CXX) $< $(CXXFLAGSDEBUG) -c -o $@

$(OBJDIR)/$(PNGDIR)/%.o: src/$(PNGDIR)/%.cpp
		$(CXX) $< $(CXXFLAGSDEBUG) -c -o $@

$(OBJDIR)/%.o: src/%.cu
		$(NVCC) $< $(NVCCFLAGSDEBUG) -c -o $@

