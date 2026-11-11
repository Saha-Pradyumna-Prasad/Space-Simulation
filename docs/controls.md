# Controls

## Keyboard

| Key | Action |
|---|---|
| `Space` or `P` | Pause or resume the simulation |
| `+` or `=` | Increase simulation speed by 1.5×, up to 10.00× |
| `-` or `_` | Decrease simulation speed by 1.5×, down to 0.10× |
| `0` | Reset simulation speed to 1.00× |
| `Up` | Increase simulation speed (special key) |
| `Down` | Decrease simulation speed (special key) |
| `1` | Select Star as the spawn type |
| `2` | Select Planet |
| `3` | Select Moon |
| `4` | Select Asteroid |
| `5` | Select Meteor |
| `6` | Select Comet |
| `7` | Select Black Hole |
| `A` | Add the selected body type at the spawn cursor |
| `D` | Delete the selected body |
| `V` | Cycle spawn velocity: orbital → retrograde → co-moving zero |
| `C` | Toggle realistic vs classic merge-only collisions |
| `X` | Toggle effective collision spheres |
| `U` | Toggle gravity/collision dust |
| `O` | Promote nearest dust particle to a physical body |
| `F1` / `H` | Open Tutorial & Controls |
| `G` | Toggle the gravity grid |
| `T` | Toggle orbit trails |
| `Y` | Toggle selected-body trajectory prediction |
| `J` | Toggle thermal colours and glow |
| `K` | Toggle tidal deformation |
| `E` | Open/close the Equation Lab |
| `B` | Open/close the Body Editor |
| `[` | Decrease spawn mass scale by 25% (minimum 25%) |
| `]` | Increase spawn mass scale by 25% (maximum 400%) |
| `,` | Decrease spawn radius scale by 10% (minimum 50%) |
| `.` | Increase spawn radius scale by 10% (maximum 200%) |
| `F` | Toggle camera follow for the selected body |
| `Home` | Reset the camera |
| `PageUp` | Zoom in |
| `PageDown` | Zoom out |
| `L` | Toggle body labels |
| `R` | Reset the universe, camera, speed, pause state, and UI |
| `Q` or `Esc` | Quit the application |

`R` restores the built-in 26-body scene. Any spawned bodies, deletions, merges, and camera changes are discarded.

## Equation Lab

Open with `E`.

| Input | Action |
|---|---|
| `E` / `Esc` | Close the lab |
| `Tab` / Left / Right | Change tab |
| Up / Down | Scroll |
| `N` | Cycle the second body B |
| `Shift + left-click` | Set body B |
| `Q` | Quit |

The lab shows live pair gravity, orbit, thermal, tidal, impact, black-hole, and body metrics.

## Body Editor

Open with `B`.

| Input | Action |
|---|---|
| `B` / `Esc` | Close the editor |
| Up / Down | Select field |
| Left / Right | Adjust value |
| `PageUp` / `PageDown` | Coarse adjustment |
| `Enter` | Start typing a value / apply typed value |
| Digits, `.`, `-`, `Backspace` | Numeric entry |
| `S` | Spawn a custom body from the current values |
| `R` | Reset selected body appearance to type defaults |

## Mouse

| Input | Action |
|---|---|
| Move | Update the spawn cursor position and highlight the body under the pointer |
| Left click | Select the body under the pointer; clear selection on empty space |
| Left-drag | Rotate the camera |
| Right-drag | Pan the camera; cancels follow mode |
| Wheel up | Zoom in |
| Wheel down | Zoom out |

The simulation viewport ends where the right-side HUD panel begins. Clicks inside the HUD panel are ignored for picking and camera dragging.

## Camera

| Property | Default | Limits |
|---|---:|---:|
| Azimuth | 45° | Unbounded |
| Elevation | 35° | 5°–85° |
| Distance | 3.05 | 0.05–8.00 |
| Field of view | 45° | Fixed |
| Near plane | 0.01 | Fixed |
| Far plane | 100.0 | Fixed |

Left-drag rotates at `0.35°` per pixel. The wheel and zoom keys scale distance by a factor of `1.12` per step. Panning moves the camera target in the plane perpendicular to the view direction and turns off follow mode.

When follow mode is enabled, the camera target tracks the selected body's rendered position. This is especially useful for scaled moon orbits.

## Spawn cursor

The cursor is the intersection of the mouse ray with the `y = 0` plane. It is clamped to the scene bounds:

```text
x ∈ [-1.20, +1.20]
z ∈ [-1.20, +1.20]
y = 0
```

A small ring drawn on that plane shows the cursor. The cursor is updated when the pointer is inside the simulation viewport and the ray intersects the plane; otherwise it stays at its last valid position.

## Spawning rules

- The new body's base mass and radius come from its type defaults, then are multiplied by the current mass and radius scales.
- The spawn position is pushed out of overlap with all rendered bodies for up to eight passes.
- Moons look for the nearest planet within `0.12` units of the cursor. If one is found, the moon receives a physical orbit inside that planet's Hill sphere, inheriting the planet's velocity.
- If no planet is close enough, a moon uses the non-moon spawn path.
- Non-moon bodies use the heaviest active body as their orbital primary.
- Orbital velocity is tangential around the dominant local primary. Retrograde reverses the angular momentum. Zero makes the body co-moving with the primary, so it starts at rest relative to that body rather than the whole scene.
- A nearby planet or black hole is chosen when the cursor is inside its local influence radius; otherwise the dominant star is used.
- Comet spawns use an eccentric orbit instead of a circular one.
- Names are serialized per type: `Planet 1`, `Planet 2`, `Asteroid 1`, and so on.
- The new body becomes selected.
- If the pool is full, the spawn is rejected with a status message.

## HUD layout

| Region | Contents |
|---|---|
| Top strip | Title, subtitle, body count, FPS, speed, pause state, simulation time, spawn configuration, status messages |
| Right panel | Selected-body data, or a no-selection control reminder |
| Bottom strip | Keyboard and mouse legend |
| Scene labels | Body names projected above their rendered positions |

Selecting a body shows:

- Name and type.
- Mass in Earth masses.
- Volume in Earth volumes.
- Rotation period (retrograde is marked).
- Orbital period.
- Surface temperature and equilibrium temperature.
- Density.
- Tidal stress and Roche limit.
- Effective collision radius and collision scale.
- Parent and real orbital radius for moons.
- Visual orbit size in planet radii for moons.
- Comet activity and tail length.
- Velocity vector, speed, and acceleration.
- Distance from the nearest star.
- Wrapped history text.

Labels are shown for stars, planets, comets, and black holes by default. Moon, asteroid, and meteor labels appear only when selected. Overlapping labels are decluttered in screen space, selected labels take priority, and close follow views show only the focused body's label. Labels are clipped away from the HUD strips.

## Tutorial & Controls

Open with `F1` or `H`.

- Left/Right: change section.
- Up/Down: scroll.
- `T`: toggle the guided tour.
- Enter/Space: next tour step.
- Backspace: previous tour step.
- `Esc` / `F1`: close.

The page includes reference sections and a guided tour; its full control table is the authoritative legend. The main HUD keeps a compact legend.

## Command line

```text
space_simulator [options]
```

| Option | Description |
|---|---|
| `--selftest` | Run the headless self-test suite and exit |
| `--screenshot [path]` | Capture a PPM screenshot after a 1.5 s warm-up and exit |
| `--size WxH` | Set the initial window size, for example `1366x738` |
| `--select NAME` | Select the named body at startup (case-sensitive, exact match) |
| `--camera AZ EL DIST` | Set initial azimuth, elevation, and distance |
| `--follow` | Follow the selected body at startup |
| `--lab` | Open the Equation Lab at startup |
| `--editor` | Open the Body Editor at startup |
| `--physical-collisions` | Use physical collision radii instead of visible surfaces |
| `--collision-debug` | Show effective collision spheres at startup |
| `--tutorial` | Open Tutorial & Controls at startup |
| `--tour` | Open Tutorial & Controls with the guided tour active |
| `--lab-tab N` | Open the Equation Lab on tab N |
| `--grid` | Enable the gravity grid at startup |
| `--no-trails` | Disable orbit trails |
| `--no-prediction` | Disable trajectory prediction |
| `--classic` | Use classic merge-only collisions |
| `--impact-demo` | Spawn two planets on a high-energy collision course |
| `--collapse-demo` | Spawn a massive star that collapses into a black hole |
| `--comet-demo` | Spawn an active comet |
| `--help`, `-h` | Print usage and exit; unknown options print a warning |

`--screenshot` uses `space_simulator.ppm` when no path is supplied. If the next argument begins with `-`, it is treated as another option.

### Examples

```bash
# Normal interactive run
./space_simulator

# Run the test suite
./space_simulator --selftest

# Open with Earth selected and a closer camera
./space_simulator --select Earth --camera 60 25 2.2

# Save a screenshot and exit
./space_simulator --screenshot solar_system.ppm

# Combine selection, camera, and capture
./space_simulator --select Saturn --camera 90 20 1.8 --screenshot saturn.ppm

# Show the gravity grid
./space_simulator --grid --camera 55 30 2.6

# Collision, collapse, and comet demos
./space_simulator --impact-demo
./space_simulator --collapse-demo
./space_simulator --comet-demo
```

Screenshots are binary PPM (`P6`) files. They can be converted with ImageMagick:

```bash
convert solar_system.ppm solar_system.png
```

Screenshot mode requires a display server. On a headless machine, use Xvfb:

```bash
xvfb-run -a ./space_simulator --screenshot solar_system.ppm
```

## Status messages

Actions and errors appear in the top strip for 180 timer ticks, roughly three seconds at 60 FPS:

- Speed changes.
- Pause/resume.
- Spawn type, velocity mode, mass scale, and radius scale changes.
- Add and delete results.
- Merge results, such as `Jupiter absorbed Io`.
- Unstable-body removal.
- Pool-full and no-selection errors.

## Tips

- Pause before spawning if you want to place several bodies carefully.
- Use zero velocity and place a body near a planet to create a direct impact.
- Use orbital velocity from a distance to create a stable satellite.
- Spawn a black hole (mass 2.0) to see strong perturbations; it is heavier than the default Sun.
- Select Ceres, Vesta, Pallas, or Hygiea to inspect the major asteroids.
- Follow Saturn and zoom in to inspect its ring bands, gaps, and shadows.
- Follow a moon to watch the scaled visual orbit track its physical motion.
- Press `G` to see the gravitational potential grid, and `Y` to inspect a selected body's predicted path.
- Press `C` to compare realistic fragmentation with classic merge-only collisions.
- `R` is the fastest way to recover after an experiment destabilizes the scene.
