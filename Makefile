EXECUTABLE=compress-bin
OBJDIR=objs
IMGDIR=images
COMPDIR=compressed
PNGDIR=lodepng
CXX=mpic++ -m64
CXXFLAGSDEBUG=-ggdb -O0 -Wall -std=c++11
CXXFLAGS=-O3 -Wall -std=c++11

OBJS=$(OBJDIR)/main.o $(OBJDIR)/$(PNGDIR)/lodepng.o $(OBJDIR)/dct.o $(OBJDIR)/image.o $(OBJDIR)/quantize.o $(OBJDIR)/rle.o $(OBJDIR)/dpcm.o


.PHONY: default dirs clean

default: $(EXECUTABLE)

dirs:
		mkdir -p $(OBJDIR)
		mkdir -p $(OBJDIR)/$(PNGDIR)
		mkdir -p $(IMGDIR)
		mkdir -p $(COMPDIR)

clean:
		rm -rf $(OBJDIR) $(IMGDIR) $(COMPDIR) *~ $(EXECUTABLE) $(LOGS)

$(EXECUTABLE): dirs $(OBJS)
		# $(CXX) $(CXXFLAGSDEBUG) -o $@ $(OBJS) $(LDFLAGS)
		$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

$(OBJDIR)/%.o: src/%.cpp
		# $(CXX) $< $(CXXFLAGSDEBUG) -c -o $@
		$(CXX) $< $(CXXFLAGS) -c -o $@

$(OBJDIR)/$(PNGDIR)/%.o: src/$(PNGDIR)/%.cpp
		# $(CXX) $< $(CXXFLAGSDEBUG) -c -o $@
		$(CXX) $< $(CXXFLAGS) -c -o $@

