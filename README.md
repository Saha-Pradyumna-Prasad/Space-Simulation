# Space Simulator

**Space Simulator v3.5** is an interactive 3D N-body gravity sandbox written in C++17. It combines real-time Newtonian physics, physical collisions, thermal and tidal models, corrected orbital spawning, and a modern OpenGL 3.3 core renderer with shaders, VBOs, glow, particles, and a custom HUD text atlas.

Orbits are not pre-computed. Every active body pulls on every other body, and planets, moons, comets, debris, and anything you spawn follow whatever paths the physics produces.

## What is new in v3.5

- Procedural planet surfaces with axial tilt, spin, oblateness, atmospheres, clouds, and a Great Red Spot.
- Ceres, Vesta, Pallas, and Hygiea as real simulated bodies in the asteroid belt.
- Rebuilt Saturn ring system with a high-resolution procedural profile, D/C/B/A/F structure, Cassini/Encke/Keeler gaps, fine ringlets, soft planet shadow, axial-tilt orientation, and two-way shadows.
- 5,000-particle Keplerian asteroid belt with eccentricity, inclination, density families, and Kirkwood gaps.
- Black holes with an event horizon, photon ring, accretion disk, growth on absorption, and collapse effects.
- Polished Sun with multi-scale animated granulation, sunspots and faculae, a soft billboard corona, prominences, and solar wind.
- Equation Lab with live pair/gravity/orbit/thermal/tidal/impact/black-hole/body formulas.
- Body Editor for custom mass, radius, volume, density, collision scale, kinematics, thermal properties, type, and composition.
- Visible-surface collision handling, including retrograde co-orbital impacts, type-aware outcomes, and collision debug spheres.
- Modern OpenGL 3.3 core renderer with GLSL shaders and VBO meshes.
- Gravity grid, orbit trails, and selected-body trajectory prediction.
- Particle-based collision, fragmentation, collapse, and spawn effects.
- Realistic collision outcomes: merge, bounce, graze, fragment, or collapse.
- Star collapse into a black hole after a mass threshold is crossed.
- Planet temperature model with equilibrium temperature, greenhouse warming, impact heating, and thermal glow.
- Tidal model with tidal stress, Roche limits, tidal heating, and visible tidal deformation.
- Corrected comet model: temperature-driven activity, coma, ion tail, and curved dust tail.
- Corrected spawning: dominant-primary selection, 3D orbit planes, prograde/retrograde/co-moving modes, and eccentric comet orbits.
- Event log, expanded body inspector, detailed real-world histories for seeded bodies, type-specific histories for runtime bodies, and demo modes.
- Dedicated Tutorial & Controls page with a guided tour (`F1` / `H`).
- 1600×900 default window with native 24 px text, proportional HUD scaling for smaller windows, and a compact layout below the design size.
- Gravity- and collision-aware space dust with physical promotion (`U` / `O`).
- Gradual thermal inertia and tidal locking.
- Everything tab listing all interactions on the selected body.
- Automatic safe-renderer fallback and `gl_errors.log` diagnostics when a GPU driver rejects the procedural shader.
- Proportional HUD/label scaling below the 1600×900 design size, with the initial window clamped to fit smaller desktops.

## Quick start

On Ubuntu/Debian:

```bash
sudo apt install build-essential freeglut3-dev
make
./space_simulator
```

On Windows (MSYS2/MinGW):

```bash
pacman -S mingw-w64-ucrt-x86_64-gcc \
          mingw-w64-ucrt-x86_64-freeglut \
          mingw-w64-ucrt-x86_64-make
make run
```

This produces and launches `space_simulator.exe`. The same `.exe` can be
double-clicked from Explorer; it only needs the standard Windows system DLLs
and a GPU/driver with OpenGL 3.3 (a built-in fallback shader is used if the
procedural body shader is rejected).

Cross-compiling the `.exe` from Linux (MinGW-w64 + static freeGLUT):

```bash
make windows        # produces space_simulator.exe
make windows-run    # runs that .exe through Wine
```

`make` only builds the executable. Use `make run` (or the VS Code **run**
task) to build and launch it. If the tree is already up to date, plain `make`
prints `Nothing to be done for 'all'.` and does not start the simulator.

Run the self-tests:

```bash
make check
```

Build a release binary:

```bash
make release
```

## Controls at a glance

| Input | Action |
|---|---|
| Left click | Select a body |
| Left-drag | Rotate the camera |
| Right-drag | Pan the camera |
| Wheel / `PageUp` / `PageDown` | Zoom |
| `Space` / `P` | Pause or resume |
| `+` / `-` or `Up` / `Down` | Change simulation speed |
| `1`–`7` | Choose a spawn type |
| `A` | Add the chosen body at the cursor |
| `D` | Delete the selected body |
| `V` | Cycle spawn velocity: orbital / retrograde / zero (co-moving) |
| `C` | Toggle realistic vs classic collision mode |
| `X` | Toggle effective collision spheres |
| `U` | Toggle gravity/collision dust |
| `O` | Promote the nearest dust particle to a physical body |
| `F1` / `H` | Open the Tutorial & Controls page |
| `E` | Open/close the Equation Lab |
| `G` | Toggle the gravity grid |
| `T` | Toggle orbit trails |
| `Y` | Toggle trajectory prediction |
| `J` | Toggle thermal colours |
| `K` | Toggle tidal deformation |
| `B` | Open/close the Body Editor |
| `F` | Follow the selected body |
| `R` | Reset the universe |
| `Q` / `Esc` | Quit |

The camera can zoom much closer now, so following a planet lets you inspect its surface, rings, and oblateness.

The full reference is in [docs/controls.md](docs/controls.md).

## Command-line demos

```bash
# Gravity grid on
./space_simulator --grid

# Two planets collide and fragment
./space_simulator --impact-demo

# A massive star collapses into a black hole
./space_simulator --collapse-demo

# A comet with coma and two tails
./space_simulator --comet-demo

# Classic merge-only collisions
./space_simulator --classic

# Screenshot and exit
./space_simulator --grid --select Earth --camera 55 30 2.6 --screenshot earth.ppm

# Follow a planet and zoom in on its surface or rings
./space_simulator --select Saturn --follow --camera 20 10 0.14
```

Other CLI options:

| Option | Effect |
|---|---|
| `--selftest` | Run the headless test suite and exit |
| `--screenshot [path]` | Render, save a PPM, and exit |
| `--size WxH` | Set the initial window size (for example `1366x738`) |
| `--select NAME` | Select a body by exact name |
| `--camera AZ EL DIST` | Set the initial camera |
| `--grid` | Enable the gravity grid at startup |
| `--no-trails` | Disable orbit trails |
| `--no-prediction` | Disable trajectory prediction |
| `--classic` | Use merge-only collisions |
| `--impact-demo` | Spawn a high-energy planetary impact |
| `--collapse-demo` | Spawn a collapsing massive star |
| `--comet-demo` | Spawn an active comet |
| `--follow` | Follow the selected body at startup |
| `--lab` | Open the Equation Lab at startup |
| `--editor` | Open the Body Editor at startup |
| `--physical-collisions` | Use physical collision radii instead of visible surfaces |
| `--collision-debug` | Show effective collision spheres at startup |
| `--tutorial` | Open the Tutorial & Controls page at startup |
| `--tour` | Open the tutorial with the guided tour active |
| `--lab-tab N` | Open the Equation Lab on tab N |
| `--help`, `-h` | Print the command-line usage and exit |

## Documentation

| Document | Description |
|---|---|
| [docs/overview.md](docs/overview.md) | Purpose, audience, scope, and limitations |
| [docs/features.md](docs/features.md) | Complete functionality catalogue |
| [docs/architecture.md](docs/architecture.md) | Modules, renderer, data flow, and frame loop |
| [docs/physics.md](docs/physics.md) | Gravity, collisions, temperature, tides, comets, and spawning |
| [docs/controls.md](docs/controls.md) | Keyboard, mouse, and CLI reference |
| [docs/build-and-testing.md](docs/build-and-testing.md) | Build targets, Windows setup, and verification |
| [docs/roadmap.md](docs/roadmap.md) | Deferred full-realism destruction and future work |

## Source layout

```text
src/
├── main.cpp
├── app/          Application lifecycle, CLI, screenshots
├── core/         Body data, constants, math, state, universe, event log
├── physics/      Gravity, collisions, impact model, temperature, tides, comets
├── fx/           Particles and shockwave effects
├── render/       Modern OpenGL renderer, shaders, camera, font/UI, cosmetics
├── ui/           HUD, layout, status messages
├── input/        Camera/input handling, picking, spawning
└── selftest/     Headless test suite
```

The pre-refactor single-file version is archived in `legacy/v3_monolith/main.cpp`.

## Verification

```bash
make check
```

The test suite covers the original 26-body stability checks plus temperature, tidal heating, corrected Jupiter-relative spawning, fragmentation, collapse, comet activity/tails, effect generation, physics-metric calculations, the Keplerian belt, and Body Editor value application.

The accompanying [Progress_Report_v3.docx](Progress_Report_v3.docx) documents the earlier v3 interactive-core milestone (22 bodies, fixed-function rendering). v3.5 supersedes that report; the current feature set is documented in this README and `docs/`.
