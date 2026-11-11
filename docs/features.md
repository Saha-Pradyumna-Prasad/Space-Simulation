# Features

This document catalogues what Space Simulator v3.5 actually does. For implementation details, see [architecture.md](architecture.md) and [physics.md](physics.md).

## 1. Simulation

- Real-time 3D N-body gravity over every active body pair.
- Base simulation step of `0.0025` time units, scaled by the speed multiplier and the measured real frame interval so wall-clock speed stays consistent.
- Adaptive sub-stepping from 1 to 32 steps per frame based on the fastest body and the smallest radius.
- Pause and resume without losing simulation state.
- Speed control from 0.10× to 10.00× in 1.5× steps, with a one-key reset to 1.00×.
- Simulation-time counter and frames-per-second display.
- Automatic removal of bodies whose position or velocity becomes non-finite.
- Fixed 256-slot object pool with free-slot reuse and a hard capacity limit.

## 2. Default solar system

The initial scene contains **26 active bodies**:

- 1 star: the Sun.
- 8 planets: Mercury, Venus, Earth, Mars, Jupiter, Saturn, Uranus, Neptune.
- 13 moons: Moon, Phobos, Deimos, Io, Europa, Ganymede, Callisto, Enceladus, Rhea, Titan, Titania, Oberon, Triton.
- 4 major belt asteroids: Ceres, Vesta, Pallas, and Hygiea.

| Planet | Orbit radius (sim) | Mass (Earth = 1) | Visual radius (sim) |
|---|---:|---:|---:|
| Mercury | 0.18 | 0.055 | 0.010 |
| Venus | 0.28 | 0.815 | 0.014 |
| Earth | 0.38 | 1.000 | 0.017 |
| Mars | 0.48 | 0.107 | 0.013 |
| Jupiter | 0.64 | 317.80 | 0.036 |
| Saturn | 0.80 | 95.20 | 0.031 |
| Uranus | 0.96 | 14.50 | 0.023 |
| Neptune | 1.12 | 17.10 | 0.022 |

| Moon | Parent | Real orbital period |
|---|---|---:|
| Moon | Earth | 27.32 days |
| Phobos | Mars | 0.319 days |
| Deimos | Mars | 1.263 days |
| Io | Jupiter | 1.769 days |
| Europa | Jupiter | 3.551 days |
| Ganymede | Jupiter | 7.155 days |
| Callisto | Jupiter | 16.689 days |
| Enceladus | Saturn | 1.370 days |
| Rhea | Saturn | 4.518 days |
| Titan | Saturn | 15.945 days |
| Titania | Uranus | 8.706 days |
| Oberon | Uranus | 13.463 days |
| Triton | Neptune | 5.877 days (retrograde) |

Each body also stores real-world rotation periods, orbital periods, volume ratios, colours, and a multi-sentence factual history for the HUD. Runtime bodies receive type-specific histories describing their physics and behaviour.

## 3. Body types

Seven types can be spawned at runtime:

| Type | Default mass (sim) | Default radius (sim) | Visual treatment |
|---|---:|---:|---|
| Star | 0.50 | 0.050 | Emissive sphere plus layered glow |
| Planet | 3.003e-6 | 0.012 | Lit sphere with planet colour |
| Moon | 3.700e-8 | 0.006 | Lit sphere; parented to a nearby planet when possible |
| Asteroid | 1.000e-9 | 0.004 | Low-detail lit sphere |
| Meteor | 1.000e-10 | 0.003 | Low-detail sphere plus a velocity streak |
| Comet | 1.000e-10 | 0.004 | Low-detail sphere plus a tail pointing away from the nearest star |
| Black Hole | 2.00 | 0.018 | Black sphere with an emissive purple torus |

Mass and radius can be scaled at spawn time. The most luminous star provides scene lighting; if no star exists, scene lighting is disabled.

## 4. Runtime mutation

### Spawning

- `1`–`7` selects the body type.
- `A` creates the selected type at the mouse cursor.
- Spawn cursor is the intersection of the mouse ray with the `y = 0` plane, clamped to ±1.20 in X and Z.
- New bodies are pushed out of overlap with existing bodies for up to eight passes.
- New bodies receive the mass and radius of their type multiplied by the current scale factors.
- Names use a per-type serial: `Planet 1`, `Planet 2`, `Moon 1`, and so on.
- The newly created body becomes the current selection.

### Spawn velocity modes

`V` cycles through three modes:

1. **Orbital** — tangential velocity around the most massive body; for moons, around the chosen parent planet.
2. **Retrograde** — the same speed in the opposite direction.
3. **Zero** — no initial velocity; the body falls under gravity.

### Spawn scales

- Mass: 25% to 400% in 25% increments with `[` and `]`.
- Radius: 50% to 200% in 10% increments with `,` and `.`.

### Deletion

- `D` deletes the selected body.
- Deleting a planet detaches its moons rather than deleting them.
- Deleting a body clears the selection.
- The pool slot is immediately reusable.

### Reset

- `R` restores the full solar system, camera defaults, speed, pause state, labels, status, spawn state, and selection.

## 4b. Thermal model

- Every non-star body receives stellar flux from all active stars.
- Star luminosity scales with mass; distance uses the compressed Earth orbit as the calibration point.
- Equilibrium temperature uses albedo and distance; greenhouse offsets warm Venus-like worlds.
- Impact heating raises temperature temporarily and decays.
- Tidal heating adds a temperature contribution.
- Hot bodies receive an emissive thermal glow; the HUD shows surface and equilibrium temperature.
- Temperature drives comet activity.
- Thermal inertia makes temperature changes gradual.
- Tidal locking synchronizes moon rotation with its orbital period.

## 4c. Tidal model

- Computes the dominant primary for each body by tidal influence.
- Tracks the tidal axis, tidal stress ratio, and Roche limit.
- Adds tidal heating to the temperature model.
- Renders a visible tidal bulge by deforming the body sphere along the primary direction.
- Strong tides produce a visible warning/glow; tidal disruption is deferred to the full-realism roadmap.

## 4d. Comet model

- Activity is driven by surface temperature and stellar flux.
- The HUD reports activity percentage and tail length.
- A coma glow and two tails are rendered: a blue ion tail and a curved warm dust tail.
- The tails use the net stellar radiation field, not only the nearest star.
- Comet spawning uses an eccentric orbit rather than a circular planetary orbit.

## 4e. Sun polish

- Multi-scale animated procedural granulation.
- View-dependent limb darkening and chromosphere rim.
- Sparse sunspots with penumbra/umbra structure and bright faculae.
- Four-layer soft billboard corona with smooth radial falloff.
- Animated prominence arcs.
- Soft solar-wind particles.
- Temperature-based stellar colour and lighting.
- The Sun remains emissive and never receives planet-style diffuse shading.

## 4f. Equation Lab

Open with `E`.

- Pair gravity: force, acceleration, potential, escape velocity, relative speed, direction.
- Orbit: semi-major axis, eccentricity, periapsis, apoapsis, period, specific energy, angular momentum.
- Thermal: stellar flux, luminosity, equilibrium/surface temperature, greenhouse, tidal/impact heating.
- Tidal: tidal acceleration, ratio, Roche limit, Hill radius, heating.
- Impact: relative speed, impact parameter, specific impact energy, predicted outcome.
- Black hole: horizon, photon sphere, ISCO, time dilation.
- Body: mass, radius, density, surface gravity, escape velocity, tilt, flattening, composition.
- Everything: net force/acceleration/tidal vectors, system kinetic/potential/total energy, system momentum, all contributors sorted by force, orbit, thermal, tidal, rotation, collision, and black-hole metrics.
- A is the selected body; B is chosen with `N` or `Shift + left-click`.
- `Tab` or Left/Right changes tabs; Up/Down scrolls.

## 4g. Body Editor

Open with `B`.

- Edit the selected body or use it as a template for a custom spawn.
- Fields: mass, visual radius, derived volume, physical density, collision radius, collision scale, position, velocity, albedo, greenhouse, axial tilt, flattening, cloud amount, spin rate, body type, composition, atmosphere RGB.
- Mass and physical density are canonical; volume is derived from them, while the visual radius remains a separate render property.
- Visual radius edits, appearance edits, and composition edits do not silently change the physical collision radius.
- Effective visual collision radius is displayed.
- Direct numeric entry with Enter.
- Composition presets: Rocky, Icy, Metallic, Gas Giant, Stellar, Exotic.
- Derived values update automatically: volume, collision radius, horizon, density.
- `S` spawns a custom copy at the cursor.
- `R` resets the selected body's type appearance and composition defaults.
- Black holes get a mass-derived event horizon.

## 5. Gravity, collisions, and moons

- O(N²) pairwise gravity with Newton's third law.
- Two softening lengths: `0.01` for general pairs and `0.0001` for a moon and its parent.
- Swept collision test between previous and current positions to catch fast bodies.
- Visible-surface collision mode: effective radius = max(physical radius, visual radius × 0.90).
- Physical-only collision mode is available with `--physical-collisions`.
- Type-aware impact rules: stars merge, black holes absorb, planets/moons use realistic outcomes.
- Short collision cooldown after bounce/graze.
- Realistic impact classification:
  - **Merge** for low-energy, low-impact-parameter collisions.
  - **Bounce** for low-energy, strong bodies.
  - **Graze** for off-centre impacts.
  - **Fragment** for high specific impact energy.
  - **Disrupt** for extreme impacts.
  - **Absorb** for black-hole collisions.
- Classic merge-only mode is available with `C` or `--classic`.
- Collision merge keeps the heavier body, conserves mass and 3D momentum, moves the result to the centre of mass, grows the radius by the cube root of the mass ratio, adds volume ratios, and remaps the selection if the absorbed body was selected.
- Fragmentation creates real debris bodies with conserved mass and momentum, capped by the 256-body pool.
- Stars that cross the collapse threshold become black holes.
- Impact, fragmentation, collapse, and spawn events feed the particle/effects system.
- Moons are seeded inside a fraction of their parent's Hill sphere (20%–40%) and rendered at a scaled visual orbit.
- Moons remain true N-body bodies; the visual scaling affects only their rendered position.
- A moon detaches to an independent body if its parent is removed.
- Merge events and unstable-body removals appear as temporary HUD status messages.

## 6. Camera

- 3D orbit camera with azimuth/elevation rotation and distance zoom.
- Default view: azimuth 45°, elevation 35°, distance 3.05.
- Elevation is limited to 5°–85°.
- Distance is limited to 0.05–8.00, allowing close planet inspection.
- Left-drag rotates; right-drag pans; wheel, `PageUp`, and `PageDown` zoom.
- `Home` resets the camera.
- `F` toggles follow-selected. Panning cancels follow mode.
- Follow mode tracks the selected body's rendered position.

## 7. Picking and selection

- Mouse clicks are converted to a world-space ray from the camera basis and projection parameters.
- Bodies are tested as spheres; the nearest ray-sphere hit wins.
- Picking uses the rendered position, so the visual scaling also determines how easy a moon is to click.
- The selection is shown with a white wire sphere and a highlighted label; hovering draws a subtle ring around the pickable body.
- Clicking empty space clears the selection and follow mode.
- The camera viewport excludes the HUD panel, so HUD clicks do not select bodies.

## 8. HUD and educational data

- Title, subtitle, and a status strip with body count, FPS, speed, pause state, and simulation time.
- Live spawn configuration display.
- Temporary status messages for actions and errors.
- Selected-body panel:
  - Name and type.
  - Mass in Earth masses.
  - Volume in Earth volumes.
  - Rotation period and retrograde indicator.
  - Orbital period.
  - Parent planet and real orbital radius for moons.
  - Visual orbit size in planet radii for moons.
  - 3D velocity vector, speed, and acceleration.
  - Distance from the nearest star.
  - Wrapped history text.
- No-selection panel with a short control reminder.
- Projected labels above bodies; labels for moons, asteroids, and meteors appear only when selected.
- Screen-space label decluttering: the selected label wins, planet labels come next, and overlapping lower-priority labels are skipped; close follow views show only the focused body's label.
- Bottom control legend and a large `PAUSED` indicator.
- Responsive HUD width capped at 620 pixels, with compact spacing and type scaling on smaller windows.

## 8b. Gravity visualization and effects

- **Gravity grid**: toggleable spacetime mesh depressed by gravitational potential.
- **Orbit trails**: fading history lines behind bodies.
- **Trajectory prediction**: selected body's future path using a cached, softened two-body forecast refreshed periodically; moon paths are mapped into the scaled visual orbit.
- **Impact effects**: sparks, dust, shockwave rings, flashes, and camera-shake data.
- **Collapse effects**: large flash, shockwave, purple ejecta, and black-hole ring.
- **Spawn effects**: materialisation flash and expanding ring.
- **Event log**: the last four physics events (impacts, merges, fragmentation, collapse, unstable-body removal) are shown with simulation timestamps above the control legend.

## 9. Visual scene

- Modern OpenGL 3.3 core renderer with GLSL shaders and VBO meshes.
- 1600×900 default window; all data text uses the native 24 px font.
- Lit sphere rendering with thermal emission and tidal deformation.
- Procedural planet surfaces with axial tilt, spin, oblateness, atmosphere rims, clouds, and a Great Red Spot.
- Black holes render an event horizon, photon ring, and animated accretion disk.
- 600-point procedural starfield at a radius of 18–22 units.
- 5,000-particle Keplerian asteroid belt with eccentricity, inclination, density structure, and Kirkwood gaps.
- Saturn ring rendered from a 2,048-sample procedural profile with D/C/B/A/F structure, fine ringlets, Cassini/Encke/Keeler gaps, optical-depth alpha, view-dependent illumination, a soft planet shadow, axial-tilt orientation, and two-way shadows; it remains cosmetic, not a collision body.
- Sun glow rendered as four camera-facing additive billboard layers sized from the star's apparent screen radius.
- Bound-body orbit lines drawn from the current osculating orbital elements, including eccentricity and inclination.
- Moon orbit lines are conics scaled into the same visual space as the rendered moon positions.
- Trajectory prediction is cached and refreshed only when needed; it uses the same softened gravity as the simulation and maps moon paths through the visual scale.
- Comet tails oriented away from the nearest star.
- Meteor streaks oriented opposite the velocity vector.
- Selection wire sphere and spawn cursor ring.
- Smooth lighting, specular materials, emissive stars, and a dark space background.

## 10. Command-line interface

| Option | Effect |
|---|---|
| `--selftest` | Run the headless test suite and exit |
| `--screenshot [path]` | Render for 1.5 s, save a PPM screenshot, and exit |
| `--select NAME` | Select a body by exact name at startup |
| `--camera AZ EL DIST` | Set the initial camera azimuth, elevation, and distance |
| `--grid` | Enable the gravity grid at startup |
| `--no-trails` | Disable orbit trails |
| `--no-prediction` | Disable trajectory prediction |
| `--classic` | Use classic merge-only collisions |
| `--impact-demo` | Spawn a high-energy planetary impact |
| `--collapse-demo` | Spawn a collapsing massive star |
| `--comet-demo` | Spawn an active comet |
| `--help`, `-h` | Print command-line usage and exit |

Options compose. For example:

```bash
./space_simulator --select Earth --camera 60 25 2.2 --screenshot earth.ppm
```

The default screenshot path is `space_simulator.ppm`.

## 11. Self-tests and diagnostics

The `--selftest` mode runs without a window and checks:

- `CelestialBody` fits the pool memory budget.
- Ray-sphere hit and miss behaviour.
- 26-body initial layout and expected named bodies.
- Earth's orbit drift stays under 5% over 20,000 quarter-steps.
- All eight planets stay within 5% of their initial orbits.
- Moon stays bound to Earth in its Hill-safe physical orbit.
- Moon visual position matches the physical orbit and visual scale.
- All moons remain bound to their parents.
- No accidental collisions in the stable layout.
- Planet removal detaches rather than destroys moons.
- Slot allocation, removal, and reuse.
- Out-of-range removal rejection.
- Runtime moon spawning, parent assignment, and orbital velocity.
- Merge conservation of mass, 3D momentum, and volume ratio.
- Pool filling to 256 bodies and rejection of overflow.

## 12. Platform support

- Native Linux build with `g++`, freeGLUT, and OpenGL.
- Windows build with MinGW; the VS Code task links freeGLUT and OpenGL.
- Optional cross-compilation from Ubuntu with a static MinGW freeGLUT build.
- VS Code build, run, check, and cross-compile tasks, plus Linux and Windows debug configurations.
- The renderer uses an OpenGL 3.3 core context created through freeGLUT; GLFW is deferred because its development package is not available in the target environment.
- The HUD uses a generated DejaVu Sans font atlas, so no GLUT bitmap text is needed.
