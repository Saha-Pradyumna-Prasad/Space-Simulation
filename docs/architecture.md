# Architecture

## System context

Space Simulator is a single-process desktop application. freeGLUT owns the window, input callbacks, and the frame timer; OpenGL performs all rendering. The application has no external data files, network access, or persistence layer. The entire initial scene is defined in source.

```text
+-------------------------------------------------------------+
|                         Application                         |
|  GLUT callbacks -> InputController / timer / display        |
+---------------------------+---------------------------------+
                            |
        +-------------------+-------------------+
        |                                       |
+-------v--------+                     +--------v---------+
|   Simulation   |                     |   Presentation   |
| Universe       |                     | OrbitCamera      |
| PhysicsEngine  |                     | CosmeticScene    |
| CollisionSolver|                     | SceneRenderer    |
| MoonSystem     |                     | Hud / TextRenderer|
+-------+--------+                     +--------+---------+
        |                                       |
        +-------------------+-------------------+
                            |
                    +-------v--------+
                    |  Shared state  |
                    | UiState        |
                    | SimulationState|
                    | SelectionState |
                    | InputState     |
                    +----------------+
```

## Module map

| Directory | Responsibility |
|---|---|
| `src/app/` | Application lifecycle, GLUT registration, CLI parsing, screenshot capture |
| `src/core/` | Constants, body types, math helpers, shared state, universe storage, solar-system data |
| `src/physics/` | Gravity integration, collision/impact resolution, temperature, tides, comets, moon visual positioning |
| `src/fx/` | Particle, flash, shockwave, and camera-shake effects |
| `src/render/` | Modern OpenGL renderer, shaders, VBO meshes, camera, font/UI, cosmetics |
| `src/ui/` | HUD, layout calculations, status messages |
| `src/input/` | Keyboard/mouse handling, ray picking, spawning and deletion |
| `src/selftest/` | Headless verification suite |

### Files

| File | Purpose |
|---|---|
| `main.cpp` | Parses CLI options, runs the self-test or constructs the application |
| `app/application.*` | Window setup, GLUT callbacks, frame timer, simulation reset |
| `app/cli.*` | `--selftest`, `--screenshot`, `--select`, `--camera` parsing |
| `app/screenshot.*` | Back-buffer readback and PPM writer |
| `core/config.h` | Compile-time constants for physics, rendering, UI, and limits |
| `core/body.h/.cpp` | `BodyType`, `CelestialBody`, `MoonOrbit`, type names/defaults |
| `core/state.h` | `SimulationState`, `SelectionState`, `SpawnState`, `UiState`, `InputState` |
| `core/events.h` | Impact/spawn event queue and event log |
| `core/physics_metrics.*` | Shared pair, orbit, body, tidal, and black-hole calculations |
| `core/math3d.h` | Vec3/Mat4 helpers, perspective, look-at, transforms |
| `core/universe.h/.cpp` | Fixed body pool, seeding, lookup, removal, previous positions |
| `core/solar_system_data.*` | Planet and moon seed tables with real-world metadata |
| `core/math_utils.*` | Clamping, volume conversion, RNG, ray-sphere tests, safe copies |
| `physics/physics_engine.*` | Pairwise gravity, semi-implicit integration, sub-step count |
| `physics/collision_solver.*` | Overlap and swept collision tests, merge/bounce/graze/fragment/collapse |
| `physics/impact_model.*` | Impact classification from speed, impact parameter, and mass ratio |
| `physics/environment.*` | Temperature, greenhouse, tidal stress/heating, comet activity |
| `physics/moon_system.*` | Rendered moon positions and visual-radius updates |
| `fx/effects.*` | Pooled particles, shockwaves, flashes, camera-shake state |
| `render/camera.*` | Orbit camera, pan, zoom, follow target, projection, picking rays |
| `render/cosmetic_scene.*` | Starfield, asteroid belt data and animation |
| `render/scene_renderer.*` | Modern GL scene pass: lighting, bodies, grid, trails, effects, labels |
| `render/gl_api.*` | Minimal loader for OpenGL 3.3 core functions |
| `render/gl_shader.*` | Shader compilation, linking, uniforms |
| `render/gl_text.*` | Font-atlas text and 2D UI rectangles |
| `render/font_atlas.h` | Generated DejaVu Sans glyph atlas and metrics |
| `render/labels.h` | Screen-space label record |
| `ui/hud.*` | 2D overlay, event log, selected-body panel, Equation Lab, Body Editor |
| `ui/body_editor.*` | Editable field definitions, clamping, composition presets, derived values |
| `ui/layout.*` | HUD/simulation viewport width calculations |
| `ui/status.*` | Timed status message and event-log helper |
| `input/input_controller.*` | Keyboard/mouse bindings and camera interaction |
| `input/picking.*` | Screen ray construction, body picking, ground-plane intersection |
| `input/spawn_service.*` | Spawn placement, moon parenting, body deletion |
| `selftest/selftest.cpp` | Headless test suite |

## Core data model

### `CelestialBody`

A plain, copyable value stored in the fixed pool:

- `BodyType type` and `bool active`.
- Position `(x, y, z)`, velocity `(vx, vy, vz)`, acceleration `(ax, ay, az)`.
- `mass`, visual `radius`, and physical `collision_radius`.
- Fixed `name[64]` and `history[512]` strings.
- Rotation time, orbital period, physical `density_kg_m3`, and `volume_ratio` relative to Earth. Volume is derived from mass and physical density; the visual radius is a separate render property and does not imply density.
- RGB colour.

A default-constructed slot has `type = Star` and `active = false`; every loop must check `active` before reading other fields.

### `MoonOrbit`

Rendering metadata attached to a moon slot:

- `parent_index`.
- `physical_radius`, `visual_radius`, and `visual_scale`.
- `inclination`.
- Real orbit radius in kilometres.
- Cached `visual_x`, `visual_y`, `visual_z`.

A slot with `parent_index < 0` is not a parented moon.

### Shared state structs

| Struct | Contents |
|---|---|
| `SimulationState` | Speed multiplier, paused flag, simulation time |
| `SelectionState` | Selected body index and follow flag |
| `SpawnState` | Cursor position, validity, type, mass/radius percentages, velocity mode, per-type counters |
| `UiState` | Window/simulation size, FPS counters, labels flag, status message, spawn state |
| `InputState` | Drag mode and last mouse position |

These structs are passed by reference to the components that need them. There is no global mutable universe.

## Components

### `Application`

Owns every subsystem and the shared state. It:

- Initializes OpenGL state, cosmetics, renderer, and the universe.
- Registers GLUT display, reshape, keyboard, special-key, mouse, motion, passive-motion, and timer callbacks.
- Runs the 16 ms timer loop.
- Advances physics in adaptive sub-steps, resolves collisions, updates moon visuals and cosmetic animation, and increments simulation time.
- Renders the 3D scene, draws the HUD, optionally saves a screenshot, and swaps buffers.
- Applies CLI-provided selection and camera settings.
- Resets the universe to the built-in scene.

GLUT requires plain function pointers, so the application stores a static `Application *instance_` and forwards callbacks through it.

### `Universe`

Owns three fixed arrays:

- `bodies_[MAX_OBJECTS]`
- `moons_[MAX_OBJECTS]`
- `prev_x_/prev_y_/prev_z_[MAX_OBJECTS]`

It provides O(1) slot access, first-free-slot allocation, named lookup, heaviest-body search, nearest-star search, moon detachment, and previous-position storage for swept collisions. `seedSolarSystem()` and `seedMoons()` build the default scene; `reset()` clears every slot.

### `PhysicsEngine`

`computeGravity()` zeroes accelerations and walks all unordered body pairs. `integrate()` saves previous positions, computes gravity, applies semi-implicit Euler, and removes non-finite bodies. `computeSubsteps()` derives an integer sub-step count from the fastest body's travel per frame relative to the smallest radius.

### `CollisionSolver`

`resolve()` walks all active pairs and tests current overlap, then a swept segment based on previous positions. On a hit, `mergeBodies()` keeps the heavier body and updates momentum, position, mass, radius, volume, selection, and the status message.

### `MoonSystem`

`updateVisuals()` converts each moon's physical displacement from its parent into the scaled visual displacement. `visualPosition()` returns the render position for any body: the cached visual moon position when parented, otherwise the physical position.

### `OrbitCamera`

Stores azimuth, elevation, distance, and a look-at target. It builds view and projection matrices with the internal `math3d` helpers, projects world points to screen coordinates for labels, and constructs picking rays directly from the camera basis. Follow mode copies the selected body's rendered position into the target. Panning disables follow.

### `CosmeticScene`

Owns a deterministic starfield and a 5,000-particle Keplerian asteroid belt generated with a linear congruential generator. The belt animates independently of the physics pool. Saturn's ring profile, ring shader, soft star halos, and sun surface are rendered by the scene renderer. These cosmetic elements do not participate in collisions.

### `SceneRenderer`

Owns the modern OpenGL pipeline:

- A unit sphere VBO/VAO mesh.
- Dynamic line and point VBOs.
- Lit, color, and point shaders.
- Orbit, trail, prediction, gravity-grid, comet, meteor, selection, spawn, and effect drawing.
- Saturn ring profile texture (RGBA radial density/colour), a dedicated ring shader, soft planet-shadow calculations, and ring-shadow uniforms.
- Black-hole event horizon, photon ring, and accretion-disk rendering.
- A screen-label array filled by camera projection.

It renders directly into the OpenGL 3.3 core framebuffer. Point sprites provide star/belt/particle rendering; additive blending provides glow; shaders handle per-body lighting, thermal emission, and tidal deformation.

### `GlText` and `Hud`

`GlText` uploads the generated DejaVu Sans atlas, draws textured glyph quads, and draws colored UI panels. `Hud` uses it for the title, status strip, right-side inspector, event feed, labels, and control legend. There are no GLUT bitmap-font calls in the active path.

### `Effects`

A fixed-capacity particle/shockwave system. Collision, fragmentation, collapse, and spawn code push `FxEvent` records into `UiState::fx`; `Effects::consume` turns them into particles and rings; `Effects::update` advances and expires them. The renderer draws particles as additive point sprites and shockwaves as camera-facing rings.

### `InputController`

Maps keyboard and mouse events to state changes, camera actions, and spawn/delete calls. Left-click picks a body; left-drag rotates; right-drag pans; wheel and navigation keys zoom. It also tracks the spawn cursor.

### `SpawnService`

Creates and deletes bodies. Placement is clamped to the scene and pushed out of visual overlaps. Moons try to attach to the nearest planet within 0.12 units; other bodies use the heaviest body as their orbital primary. The service builds names, applies mass/radius scales, sets velocity, and initializes `MoonOrbit` metadata.

### `Screenshot`

Arms a capture after a delay, reads the back buffer with `glReadPixels`, writes a binary PPM, and causes the application to exit.

## Runtime flow

### Startup

```text
main()
  parse CLI
  if --selftest: run tests and exit
  Application app(options)
    glutInit / create window / init GL
    cosmetic scene init
    renderer init
    reset simulation:
      universe reset + seed planets + seed moons
      reset selection, simulation, UI, spawn counters
      camera reset
  apply --select and --camera
  register callbacks
  optionally arm screenshot after 1.5 s
  glutMainLoop()
```

### Simulation tick

The timer fires approximately every 16 ms:

```text
if not paused:
    frame_ratio = clamp(real_elapsed / 0.016, 0, 3.125)
    dt = BASE_DT * time_multiplier * frame_ratio
    substeps = clamp(ceil(max_speed * |dt| / (0.5 * min_radius)), 1, 32)
    for each substep:
        integrate(gravity + semi-implicit Euler)
        resolve collisions
    update moon visual positions
    update belt animation
    sim_time += dt
schedule next timer
```

The speed multiplier changes how much simulated time passes per real second. The measured frame interval keeps that rate stable when the timer drifts, while adaptive sub-stepping protects accuracy; effects use the same real interval.

### Render pass

```text
update simulation viewport width
clear colour + depth
build view/projection matrices from the camera
position the light at the active star
draw starfield points
draw gravity grid (optional)
draw planet/moon orbit lines
draw orbit trails and selected-body prediction
draw asteroid-belt points
draw lit sphere instances with thermal/tidal shader parameters
draw Saturn ring, star glow, comet tails, meteor streaks
draw additive particles and shockwave rings
draw selection marker and spawn cursor
project labels to screen space
```

The HUD then renders in a separate 2D pass over the full window, including the right-side panel that is excluded from the simulation viewport. Text is drawn from the generated font atlas using the UI shader.

## State-changing flows

### Picking

```text
mouse click
  -> screen_ray from the camera basis and projection parameters
  -> ray_sphere_hit for every active body at its rendered position
  -> nearest hit becomes selected body
  -> empty hit clears selection
```

### Spawn

```text
A key
  -> resolve cursor on y = 0 plane
  -> clamp to scene bounds
  -> push out of overlap with rendered bodies
  -> if moon type, find nearest planet within 0.12
  -> compute mass/radius from type defaults and user scales
  -> choose orbital primary and velocity mode
  -> Universe::addBody allocates a free slot
  -> initialize MoonOrbit when parented
  -> select new body and report status
```

### Merge

```text
collision detected
  -> heavier body is kept
  -> momentum: v = (m1 v1 + m2 v2) / (m1 + m2)
  -> position: centre of mass
  -> mass: sum
  -> radius: keep.radius * cbrt(total_mass / old_mass)
  -> volume ratio: sum
  -> selection remapped if the absorbed body was selected
  -> Universe::removeBody clears the slot and detaches its moons
  -> status message
```

## Object pool and memory

- Capacity: 256 bodies, fixed at compile time.
- Allocation: scan for the first inactive slot.
- Access: indexed array access, O(1).
- Removal: overwrite the slot with a default value and decrement the active count.
- Reuse: the next spawn takes the lowest free slot.
- Body strings use fixed buffers, so no per-body heap allocation occurs.
- `sizeof(CelestialBody)` is 992 bytes on the current 64-bit build; the self-test checks that it remains within the pool budget.

## Thermal, tidal, and comet flow

```text
Application::step
  -> Environment::update
       star luminosity and flux
       equilibrium and surface temperature
       impact-heating decay
       dominant tide, tidal ratio, tidal heating, Roche limit
       comet activity, coma radius, tail length/direction
  -> timer consumes queued effects and advances them
  -> renderer uses the updated fields
```

The environment update is visual/thermal only. It does not apply tidal torques or orbital migration.

## Modern renderer bootstrap

```text
glutInitContextVersion(3, 3)
glutInitContextProfile(GLUT_CORE_PROFILE)
glutCreateWindow(...)
glapi::load()             # glCreateShader/VAO/VBO/FBO/etc.
SceneRenderer::init()     # compile shaders, build sphere VBO
GlText::init()            # upload font atlas, build UI VBO
```

The fallback path is intentionally simple: if the OpenGL function loader fails, the renderer cannot initialize, and the program reports the problem on stderr. The simulation and self-tests remain independent of the window system.

## Error handling

- Invalid selected indices are ignored by renderers and selection logic.
- Out-of-range removal returns `false`.
- Full-pool spawning returns `-1` and shows a status message.
- Non-finite positions or velocities cause the body to be removed with a status message.
- A missing star disables dynamic lighting and falls back to global ambient light.
- A missing parent detaches a moon and returns its physical position for rendering.
- Zero or negative masses are rejected by collision merging and guarded in the physics loop.

## Extension points

- Add a body type in `core/body.h`, `core/body.cpp`, and the input type-selection switch.
- Add new cosmetic effects in `CosmeticScene`.
- Add new HUD fields in `Hud::draw`.
- Change the seed scene through `PLANET_SEEDS` and `MOON_SEEDS`.
- Replace the integrator or add a higher-order scheme inside `PhysicsEngine`.
- Add new CLI options in `app/cli.*` and apply them in `Application::run`.
