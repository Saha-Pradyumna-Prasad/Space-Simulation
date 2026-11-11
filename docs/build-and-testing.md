# Build and Testing

## Toolchain

Space Simulator is a C++17 application built with `g++`. It uses freeGLUT for windowing and input and an OpenGL 3.3 core context for rendering. The renderer uses GLSL shaders, VBO meshes, GPU point sprites, and a generated font-atlas HUD; it does not use GLU or fixed-function matrix stacks. A small internal loader resolves the OpenGL 3.3 function set on Linux and Windows.

- Language level: C++17.
- Compatibility target: the same sources also compile as C++11 for older MinGW installations.
- Build systems: a `Makefile` and VS Code tasks.
- No CMake, package manager, or generated build files are required.

## Linux

### Dependencies

Ubuntu/Debian:

```bash
sudo apt update
sudo apt install build-essential freeglut3-dev
```

The linker libraries are:

```text
-lglut -lGL -ldl -lm
```

The OpenGL 3.3 core functions are loaded at runtime by `src/render/gl_api.cpp`; no GLFW/GLEW/GLAD package is required.

### Build and run

```bash
make
./space_simulator
```

`make` compiles every `src/*.cpp` and `src/*/*.cpp` file into `space_simulator`.

Build an optimised binary:

```bash
make release
```

### Clean

```bash
make clean
```

This removes `space_simulator`, `space_simulator.exe`, and the temporary `space_simulator_cxx11` test binary.

## Windows

### Native MSYS2/MinGW build

Install the compiler and freeGLUT:

```bash
pacman -S mingw-w64-ucrt-x86_64-gcc \
          mingw-w64-ucrt-x86_64-freeglut \
          mingw-w64-ucrt-x86_64-make
```

Build and run from the project directory:

```bash
make
make run
```

The Makefile detects `Windows_NT`, produces `space_simulator.exe`, and links
`-lfreeglut -lopengl32`. `make` only builds; `make run` builds and launches the
executable. If the tree is already up to date, plain `make` prints
`Nothing to be done for 'all'.` and does not start the simulator.

The equivalent manual command is:

```bash
g++ -g -Wall -Wextra -std=c++17 -Isrc \
    src/main.cpp src/app/*.cpp src/core/*.cpp src/fx/*.cpp src/input/*.cpp \
    src/physics/*.cpp src/render/*.cpp src/selftest/*.cpp src/ui/*.cpp \
    -o space_simulator.exe -lfreeglut -lopengl32
```

The VS Code `build` task contains an explicit Windows source list and the same include path and libraries, so it does not rely on shell glob expansion. The `run` task always rebuilds and then launches the executable.

If freeGLUT is linked dynamically, make sure the matching `freeglut.dll` is available beside the executable or on `PATH`.

### Cross-compile from Ubuntu

The optional cross-build uses `x86_64-w64-mingw32-g++` and a static MinGW freeGLUT distribution.

1. Install the cross-compiler:

   ```bash
   sudo apt install mingw-w64
   ```

2. Place a MinGW freeGLUT package in a directory, for example `$HOME/freeglut-mingw`, with:

   ```text
   $HOME/freeglut-mingw/include/GL/...
   $HOME/freeglut-mingw/lib/libfreeglut_static.a
   ```

3. Build:

   ```bash
   make windows
   ```

The target links `freeglut_static`, `opengl32`, `winmm`, and `gdi32`, and statically links the GCC runtime. It produces `space_simulator.exe`.

Run the cross-built executable through Wine (builds it first if needed):

```bash
make windows-run
```

Run the self-test through Wine if available:

```bash
wine ./space_simulator.exe --selftest
```

If `FREEGLUT_WIN` is not set, the Makefile defaults to `$HOME/freeglut-mingw`. Override it when needed:

```bash
make windows FREEGLUT_WIN=/path/to/freeglut-mingw
```

## VS Code

The workspace includes:

- `.vscode/tasks.json`
  - `build` — `make` on Linux; a complete explicit `g++` command on Windows.
  - `run` — builds and launches the simulator.
  - `check` — runs `make check`.
  - `build windows (cross-compile from Ubuntu)` — runs `make windows`.
- `.vscode/launch.json`
  - `Debug Space Simulator (Linux)`
  - `Debug Space Simulator (Windows MinGW)`
- `.vscode/c_cpp_properties.json`
  - Linux and Windows MinGW IntelliSense configurations with C++17.

The build task includes `-Isrc` so module includes such as `"core/universe.h"` resolve correctly.

## Self-tests

```bash
make check
```

This performs three steps:

1. Builds `space_simulator_cxx11` with `-std=c++11`, runs `--selftest`, and removes the temporary binary.
2. Runs `--selftest` on the normal C++17 `space_simulator` build.
3. Fails the make target if either test exits non-zero.

### Coverage

The self-test checks:

- `CelestialBody` fits the fixed pool memory budget.
- Ray-sphere intersection returns the near hit and rejects misses.
- The initial scene has exactly 26 bodies and includes the Sun, Earth, and the four major belt asteroids.
- Earth stays finite and its orbital radius drift is under 5% after 20,000 quarter-steps.
- All eight planets stay within 5% of their initial orbital radii.
- The Moon stays attached to Earth and in its Hill-safe orbit.
- Rendered moon positions match the physical orbit and visual scale.
- All moons remain bound to their parents.
- The stable layout produces no accidental collisions.
- Earth's thermal model is Earth-like; Venus is hotter; Neptune is colder.
- Io experiences stronger tidal stress and heating than the Moon.
- Zero-velocity spawns near Jupiter are co-moving; prograde spawns have local orbital velocity.
- High-energy impacts produce debris fragments.
- Massive stars collapse into black holes.
- Comet activity, tail growth, and anti-sunward direction behave as expected.
- The belt contains Keplerian particles and Kirkwood gaps.
- The four major asteroids exist.
- Black holes absorb matter and grow their horizon.
- Pair, orbit, and body metrics match direct calculations.
- Co-orbital retrograde planets collide instead of passing through.
- Physical-only mode preserves visible pass-through.
- Stars merge instead of fragmenting.
- Body Editor mass, volume, density, visual-radius, and collision-radius edits preserve physical invariants.
- The Body Editor applies custom mass, tilt, type, and black-hole horizon values.
- Impact events create particles and shockwaves.
- Removing Earth detaches the Moon instead of deleting it.
- Slot addition, removal, and reuse work; out-of-range removal is rejected.
- A moon can be spawned, selected, parented, and given orbital velocity.
- Merging conserves mass, 3D momentum, and volume ratio, and deactivates the absorbed body.
- The pool fills to 256 bodies and rejects overflow.

Expected summary:

```text
Space Simulator v3.5 self-test
sizeof(CelestialBody) = 992 bytes
...
SELF-TEST PASSED
```

The exact struct size can differ on other architectures or compilers; the test checks against a range, not a hard-coded size.

## Screenshot mode

```bash
./space_simulator --screenshot
./space_simulator --screenshot output.ppm
./space_simulator --size 1366x738 --screenshot small_window.ppm
./space_simulator --select Earth --camera 60 25 2.2 --screenshot earth.ppm
./space_simulator --grid --select Earth --camera 55 30 2.6 --screenshot grid.ppm
./space_simulator --impact-demo --camera 45 30 3.0 --screenshot impact.ppm
./space_simulator --collapse-demo --camera 45 30 3.0 --screenshot collapse.ppm
./space_simulator --comet-demo --camera 45 25 2.6 --screenshot comet.ppm
```

The application waits 1.5 s, renders a frame, reads the back buffer, writes a binary PPM, and exits. The default file is `space_simulator.ppm`.

On a headless machine:

```bash
xvfb-run -a ./space_simulator --screenshot output.ppm
```

## Optional sanitizer run

To check for memory and undefined-behaviour errors in the headless path:

```bash
g++ -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer \
    -std=c++17 -Isrc $(find src -name '*.cpp') \
    -o /tmp/space_simulator_san -lglut -lGL -ldl -lm
ASAN_OPTIONS=detect_leaks=1 /tmp/space_simulator_san --selftest
```

## Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| `GL/glut.h: No such file or directory` | freeGLUT headers missing | Install `freeglut3-dev` |
| `cannot find -lglut` | freeGLUT linker library missing | Install the development package or adjust the library path |
| Window fails to open on a server | No display server | Use a desktop session or `xvfb-run` for screenshots |
| `make windows` says to set `FREEGLUT_WIN` | MinGW freeGLUT folder not found | Set `FREEGLUT_WIN` to the folder containing `include/` and `lib/` |
| Windows executable cannot start | Dynamic freeGLUT DLL missing | Use `make windows` (static freeGLUT) or place `freeglut.dll` beside the `.exe` |
| Windows window shows a dark red screen | OpenGL 3.3 function set unavailable | Update the GPU driver; the title bar reports the requirement |
| Build task only compiles `main.cpp` | Old task configuration | Use the updated task, which builds all sources |
| Screenshot is black | Capture before the first frame | Keep the built-in 1.5 s delay or use `--screenshot` normally |
| Simulation becomes unstable after spawning | Extreme mass or zero-velocity impact | Press `R` to reset, or reduce the spawn mass scale |
| `make` says `Nothing to be done for 'all'.` | The executable is newer than the sources; `make` only builds | Use `make run`, the VS Code `run` task, or launch the executable directly |
| Planets are hidden behind a giant white disc | The body shader failed on the GPU driver and fixed-function drew a unit sphere | Check `gl_errors.log`; the app now switches to a simplified shader and shows a warning banner |
| HUD text looks too large on a small window | The window is smaller than the 1600x900 design size | The HUD now scales proportionally; use `--size 1600x900` for the original layout |

### GPU diagnostics

At startup the renderer writes `gl_errors.log` (or
`%TEMP%/space_simulator_gl.log` if the working directory is read-only) with:

- GL vendor, renderer, version, GLSL version, and context profile.
- The compile/link result for every shader.

If the procedural body shader fails, the renderer automatically builds a small
fallback shader so planets and the Sun remain visible, and the HUD shows a
warning banner. To verify the fallback path on a machine where the main shader
works, run:

```bash
SPACESIM_FORCE_LIT_FALLBACK=1 ./space_simulator
```

## Verification checklist

```bash
make clean
make
make check
./space_simulator --selftest
```

A correct build is warning-free under `-Wall -Wextra` and ends with `SELF-TEST PASSED` for both the C++11 and C++17 binaries. The GUI smoke test can be run with `--screenshot` on a machine with a display or under Xvfb.
