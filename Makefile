CXX      = g++
CXXFLAGS = -g -Wall -Wextra -std=c++17 -Isrc
SRC      = $(wildcard src/*.cpp src/*/*.cpp)
HEADERS  = $(wildcard src/*.h src/*/*.h)

ifeq ($(OS),Windows_NT)
TARGET    = space_simulator.exe
LIBS      = -lfreeglut -lopengl32
CHECK_BIN = space_simulator_cxx11.exe
# cmd.exe does not understand "./name"; plain "name" works in cmd and POSIX shells.
RUN_CMD   = $(TARGET)
else
TARGET    = space_simulator
LIBS      = -lglut -lGL -ldl -lm
CHECK_BIN = space_simulator_cxx11
RUN_CMD   = ./$(TARGET)
endif

# Optional MinGW cross-compile from Ubuntu.
# Point FREEGLUT_WIN at your cross-compiled MinGW freeGLUT static folder.
WIN_CXX      = x86_64-w64-mingw32-g++
FREEGLUT_WIN ?= $(HOME)/freeglut-mingw

# 1. Added -DFREEGLUT_STATIC definition flag
WIN_CXXFLAGS = -O2 -Wall -Wextra -Wno-cast-function-type -std=c++17 -Isrc -DFREEGLUT_STATIC -I$(FREEGLUT_WIN)/include

# 2. Linked -lfreeglut_static instead of -lfreeglut
# 3. Appended system libraries -lwinmm -lgdi32 needed by static freeGLUT on Windows
FREEGLUT_LIBDIR ?= $(FREEGLUT_WIN)/lib
WIN_LIBS     = -L$(FREEGLUT_LIBDIR) -lfreeglut_static -lopengl32 -lwinmm -lgdi32 \
               -static-libgcc -static-libstdc++

all: $(TARGET)

$(TARGET): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LIBS)

run: $(TARGET)
	$(RUN_CMD)

release: $(SRC) $(HEADERS)
	$(CXX) -O2 -DNDEBUG -Wall -Wextra -std=c++17 -Isrc $(SRC) -o $(TARGET) $(LIBS)

# Build once with the stricter C++11 subset used by older MinGW installs,
# then run the physics self-test with both binaries (no display needed).
check: $(TARGET)
	$(CXX) -O2 -Wall -Wextra -std=c++11 -Isrc $(SRC) -o $(CHECK_BIN) $(LIBS)
	./$(CHECK_BIN) --selftest
	./$(TARGET) --selftest
	rm -f $(CHECK_BIN)

windows: space_simulator.exe

space_simulator.exe: $(SRC) $(HEADERS)
	@test -d "$(FREEGLUT_WIN)" || \
		(echo "Set FREEGLUT_WIN to a MinGW freeGLUT folder; see docs/build-and-testing.md."; exit 1)
	$(WIN_CXX) $(WIN_CXXFLAGS) $(SRC) -o space_simulator.exe $(WIN_LIBS)

# Run the Windows build locally through Wine (handy for testing a cross-build
# on Linux before copying space_simulator.exe to a Windows machine).
windows-run: space_simulator.exe
	wine space_simulator.exe

clean:
	rm -f space_simulator space_simulator.exe \
	      space_simulator_cxx11 space_simulator_cxx11.exe gl_errors.log

.PHONY: all run release check windows windows-run clean

