##############################################################################

TARGET = KINGVON.EXE
CXXFLAGS = -D_UNICODE -DUNICODE -O2 -s
LDFLAGS = -mwindows -municode -s -lmfplat -lmf -lmfuuid -lstrmiids -lshlwapi -lole32
CXX = g++
WINDRES = windres
OBJS = $(patsubst %.cc,%.o,$(wildcard *.cc)) resource.o

##############################################################################

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

resource.o: resource.rc
	$(WINDRES) $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

##############################################################################
