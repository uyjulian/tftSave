
CC = i686-w64-mingw32-gcc
CXX = i686-w64-mingw32-g++
GIT_TAG := $(shell git rev-parse --short HEAD)
CFLAGS += -Ofast -march=ivybridge -mfpmath=sse -flto
CFLAGS += -Wall -Wno-unused-value -Wno-format -fpermissive -I. -I.. -I../ncbind -DGIT_TAG=L\"$(GIT_TAG)\" -DNDEBUG -DWIN32 -D_WIN32 -D_WINDOWS 
CFLAGS += -D_USRDLL -DMINGW_HAS_SECURE_API -DUNICODE -D_UNICODE -DNO_STRICT
LDFLAGS += -static -static-libstdc++ -static-libgcc -shared -Wl,--kill-at
LDLIBS += -lodbc32 -lodbccp32 -lgdi32 -lcomctl32 -lcomdlg32 -lole32 -loleaut32 -luuid

%.o: %.c
	@printf '\t%s %s\n' CC $<
	$(CC) -c $(CFLAGS) -o $@ $<

%.o: %.cpp
	@printf '\t%s %s\n' CXX $<
	$(CXX) -c $(CFLAGS) -o $@ $<

SOURCES := ../tp_stub.cpp ../ncbind/ncbind.cpp main.cpp
OBJECTS := $(SOURCES:.c=.o)
OBJECTS := $(OBJECTS:.cpp=.o)

BINARY ?= tftSave.dll
ARCHIVE ?= tftSave.$(GIT_TAG).7z

all: $(BINARY)

archive: $(ARCHIVE)

clean:
	rm -f $(OBJECTS) $(BINARY) $(ARCHIVE)

$(ARCHIVE): $(BINARY) 
	rm -f $(ARCHIVE)
	7z a $@ $^

$(BINARY): $(OBJECTS) 
	@printf '\t%s %s\n' LNK $@
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)
