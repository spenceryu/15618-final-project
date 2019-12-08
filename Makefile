EXECUTABLE=compress-bin
OMPEXEC=compress-bin-omp
OBJDIR=objs
IMGDIR=images
COMPDIR=compressed
PNGDIR=lodepng
CXX=mpic++ -m64
CXXOMP=g++ -m64 -fopenmp
CXXFLAGSDEBUG=-ggdb -O0 -Wall -std=c++11
CXXFLAGS=-O3 -Wall -std=c++11

OBJSMPISEQ=$(OBJDIR)/main.o
OBJSOMP=$(OBJDIR)/main2.o
OBJS=$(OBJDIR)/$(PNGDIR)/lodepng.o $(OBJDIR)/dct.o $(OBJDIR)/image.o $(OBJDIR)/quantize.o $(OBJDIR)/rle.o $(OBJDIR)/dpcm.o


.PHONY: default dirs clean

default: $(EXECUTABLE) $(OMPEXEC)

dirs:
		mkdir -p $(OBJDIR)
		mkdir -p $(OBJDIR)/$(PNGDIR)
		mkdir -p $(IMGDIR)
		mkdir -p $(COMPDIR)

clean:
		rm -rf $(OBJSMPISEQ) $(OBJSOMP) $(OBJDIR) $(IMGDIR) $(COMPDIR) *~ $(EXECUTABLE) $(OMPEXEC) $(LOGS)

$(EXECUTABLE): dirs $(OBJSMPISEQ) $(OBJS)
		# $(CXX) $(CXXFLAGSDEBUG) -o $@ $(OBJS) $(LDFLAGS)
		$(CXX) $(CXXFLAGS) -o $@ $(OBJSMPISEQ) $(OBJS) $(LDFLAGS)

$(OMPEXEC): dirs $(OBJSOMP) $(OBJS)
		$(CXXOMP) $(CXXFLAGS) -o $@ $(OBJSOMP) $(OBJS) $(LDFLAGS)

$(OBJSMPISEQ): src/main.cpp
		$(CXX) $< $(CXXFLAGS) -c -o $@

$(OBJSOMP): src/main2.cpp
		$(CXXOMP) $< $(CXXFLAGS) -c -o $@

$(OBJDIR)/%.o: src/%.cpp
		# $(CXX) $< $(CXXFLAGSDEBUG) -c -o $@
		$(CXX) $< $(CXXFLAGS) -c -o $@

$(OBJDIR)/$(PNGDIR)/%.o: src/$(PNGDIR)/%.cpp
		# $(CXX) $< $(CXXFLAGSDEBUG) -c -o $@
		$(CXX) $< $(CXXFLAGS) -c -o $@

