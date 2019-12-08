EXECUTABLE=compress-bin
OMPEXEC=compress-bin-omp
OBJDIR=objs
OBJOMPDIR=objs-omp
IMGDIR=images
COMPDIR=compressed
PNGDIR=lodepng
CXX=mpic++ -m64
CXXOMP=g++ -m64 -fopenmp
CXXFLAGS=-O3 -Wall -std=c++11

OBJSMPISEQ=$(OBJDIR)/main.o $(OBJDIR)/$(PNGDIR)/lodepng.o $(OBJDIR)/dct.o $(OBJDIR)/image.o $(OBJDIR)/quantize.o $(OBJDIR)/rle.o $(OBJDIR)/dpcm.o
OBJSOMP=$(OBJOMPDIR)/main2.o $(OBJOMPDIR)/$(PNGDIR)/lodepng.o $(OBJOMPDIR)/dct.o $(OBJOMPDIR)/image.o $(OBJOMPDIR)/quantize.o $(OBJOMPDIR)/rle.o $(OBJOMPDIR)/dpcm.o


.PHONY: default dirs clean

default: dirs $(EXECUTABLE) $(OMPEXEC)

dirs:
		mkdir -p $(OBJDIR)
		mkdir -p $(OBJDIR)/$(PNGDIR)
		mkdir -p $(OBJOMPDIR)
		mkdir -p $(OBJOMPDIR)/$(PNGDIR)
		mkdir -p $(IMGDIR)
		mkdir -p $(COMPDIR)

clean:
		rm -rf $(OBJDIR) $(OBJOMPDIR) $(IMGDIR) $(COMPDIR) *~ $(EXECUTABLE) $(OMPEXEC) $(LOGS)

$(EXECUTABLE): $(OBJSMPISEQ)
		$(CXX) $(CXXFLAGS) -o $@ $(OBJSMPISEQ) $(OBJS) $(LDFLAGS)

$(OMPEXEC): $(OBJSOMP)
		$(CXXOMP) $(CXXFLAGS) -o $@ $(OBJSOMP) $(OBJS) $(LDFLAGS)

$(OBJDIR)/%.o: src/%.cpp
		$(CXX) $< $(CXXFLAGS) -c -o $@

$(OBJDIR)/$(PNGDIR)/%.o: src/$(PNGDIR)/%.cpp
		$(CXX) $< $(CXXFLAGS) -c -o $@

$(OBJOMPDIR)/%.o: src/%.cpp
		$(CXXOMP) $< $(CXXFLAGS) -c -o $@

$(OBJOMPDIR)/$(PNGDIR)/%.o: src/$(PNGDIR)/%.cpp
		$(CXXOMP) $< $(CXXFLAGS) -c -o $@

