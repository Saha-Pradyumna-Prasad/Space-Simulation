# Space Simulator v3.5 — Instructions

This is the complete operating guide for **Space Simulator v3.5**. It explains what every mouse action and key does, what happens after you press it, and how to combine the controls into useful workflows.

For the body-by-body reference, see [clbdhistory.md](clbdhistory.md). For a feature catalogue, see [functionalities.md](functionalities.md).

---

## Contents

- [1. Before You Start](#1-before-you-start)
- [2. Screen Layout](#2-screen-layout)
- [3. Mouse Controls](#3-mouse-controls)
- [4. Keyboard Quick Reference](#4-keyboard-quick-reference)
- [5. Time and Simulation Speed](#5-time-and-simulation-speed)
- [6. Camera and Selection](#6-camera-and-selection)
- [7. Choosing and Spawning Bodies](#7-choosing-and-spawning-bodies)
- [8. Deleting, Resetting, and Quitting](#8-deleting-resetting-and-quitting)
- [9. Collisions, Dust, and Diagnostics](#9-collisions-dust-and-diagnostics)
- [10. Visualisation Toggles](#10-visualisation-toggles)
- [11. The Three Overlay Pages](#11-the-three-overlay-pages)
- [12. Spawn Workflows](#12-spawn-workflows)
- [13. Body Editor Reference](#13-body-editor-reference)
- [14. Equation Lab Reference](#14-equation-lab-reference)
- [15. Tutorial and Controls Page](#15-tutorial-and-controls-page)
- [16. Command-Line Options](#16-command-line-options)
- [17. Status Messages and Troubleshooting](#17-status-messages-and-troubleshooting)
- [18. Tips and Known Limits](#18-tips-and-known-limits)

---

## 1. Before You Start

### Requirements

- Linux or Windows.
- A GPU/driver that supports **OpenGL 3.3 core**.
- **freeGLUT** and a C++17 compiler.

On Ubuntu/Debian:

```bash
sudo apt install build-essential freeglut3-dev
make
./space_simulator
```

Other useful commands:

```bash
make release    # optimised build
make check      # headless self-tests
make windows    # MinGW cross-build
./space_simulator --help
```

### What starts up

The default scene contains **26 bodies**:

- the Sun;
- Mercury, Venus, Earth, Mars, Jupiter, Saturn, Uranus, and Neptune;
- the Moon, Phobos, Deimos, Io, Europa, Ganymede, Callisto, Enceladus, Rhea, Titan, Titania, Oberon, and Triton;
- Ceres, Vesta, Pallas, and Hygiea.

The window opens at **1600 × 900**. Orbits are not pre-baked animations: every body pulls on every other body, and the paths you see emerge from the gravity calculation.

### Your first minute

1. **Left-click Earth** to select it and read its HUD data.
2. **Left-drag** to rotate the camera, and use the **mouse wheel** to zoom.
3. Press **F** to follow Earth, then **Home** to reset the camera.
4. Press **E** to open the Equation Lab, **B** to open the Body Editor, and **F1** or **H** for the built-in tutorial.
5. Press **Space** to pause before placing a new body.

---

## 2. Screen Layout

The screen is divided into four visual regions.

| Region | What it shows |
|---|---|
| **Top strip** | Title, body count, FPS, simulation speed, pause state, simulation time, collision mode, spawn configuration, and temporary status messages |
| **Main viewport** | The 3D scene, labels, selection marker, spawn cursor, orbit trails, gravity grid, and effects |
| **Right panel** | Selected-body data or a no-selection reminder |
| **Bottom strip** | Compact keyboard and mouse legend, plus the physics event log |

### Top strip

The top strip reports:

```text
Bodies: 26   FPS: 60   Speed: 1.00x   Sim time: 12.4   collisions: realistic
Spawn: Planet   mass x1.00   radius x1.00   velocity: orbital
```

- **Bodies** — number of active bodies, up to 256.
- **FPS** — measured frames per second.
- **Speed** — current simulation-speed multiplier, from `0.10x` to `10.00x`.
- **PAUSED** — appears when the simulation is paused.
- **Sim time** — accumulated simulation time, not wall-clock time.
- **collisions** — `realistic` or `classic` collision mode.
- **Spawn** — current type, mass scale, radius scale, and velocity mode.
- **Status message** — the latest action or error, shown for roughly three seconds.

### Right panel

When a body is selected, the right panel shows:

- Name and type.
- Mass in Earth masses.
- Volume in Earth volumes (except black holes, which show the event horizon).
- Rotation period, with retrograde marked.
- Orbital period.
- Surface temperature and equilibrium temperature.
- Density.
- Tidal stress and Roche limit.
- Parent planet and real orbital radius for moons.
- Visual orbit size in planet radii for moons.
- Comet activity and tail length for comets.
- Velocity vector, speed, and acceleration.
- Distance from the nearest star (shown in the HUD as `Distance from Sun`).
- Wrapped history text, up to 10 lines.

When nothing is selected, the panel shows a short control reminder instead.

### Bottom strip and event log

- The bottom strip contains a compact control legend.
- Above it, the **event log** shows the last four physics events with simulation timestamps. Events include impacts, merges, fragmentation, collapse, and unstable-body removal.
- The full event log stores 16 entries internally; only the four most recent are drawn.

### Label rules
- Moons, asteroids, and meteors are labelled only when selected.
- The selected label wins over all others.
- Non-star bodies have higher label priority than stars, so the Sun's label may disappear when it would overlap an inner planet.
- Overlapping labels are skipped in screen space.
- Labels are clipped away from the top and bottom HUD strips.
- When you follow a body very closely, other labels are hidden so only the focused body remains.

### The viewport ends at the right panel

The right panel is **not** part of the simulation viewport. Clicks, drags, and the spawn cursor are ignored there. This prevents HUD clicks from selecting bodies or rotating the camera.

---

## 3. Mouse Controls

| Input | Action | What happens next |
|---|---|---|
| **Move** | Updates the spawn cursor and hover highlight | The cursor follows the intersection of the mouse ray with the `y = 0` plane. The body under the pointer receives a subtle hover ring. |
| **Left click** | Selects the body under the pointer | The selected body gets a white wire sphere, a highlighted label, and a populated right panel. Clicking empty space clears the selection and disables follow mode. |
| **Left-drag** | Rotates the camera | Horizontal drag changes azimuth; vertical drag changes elevation. Rotation is `0.35°` per pixel. Elevation is clamped to `5°–85°`. |
| **Right-drag** | Pans the camera | Moves the camera target in the plane perpendicular to the view. Panning cancels follow mode. |
| **Wheel up** | Zooms in | Multiplies camera distance by `1/1.12` per step, down to `0.05`. |
| **Wheel down** | Zooms out | Multiplies camera distance by `1.12` per step, up to `8.00`. |

### Selecting a body

A left click casts a ray from the camera through the pointer. Every body is tested as a sphere, and the nearest ray-sphere hit wins. The pick uses the body's **rendered** position, so the visually enlarged moon orbits are also easy to click.

After selection:

- The right panel changes from the no-selection reminder to the body inspector.
- The body gets a white wire sphere.
- Its label becomes brighter and is drawn first.
- If the body is a moon, its parent and real orbital radius are shown.
- If the body is a black hole, the panel shows the event horizon and accretion-disk status instead of the volume, rotation, orbital-period, temperature, density, and tidal-stress block.

Clicking empty space:

- Clears the selection.
- Clears the right panel.
- Turns off follow mode.
- Shows `Selection cleared` in the status strip.

### Rotating and panning

- **Left-drag** rotates around the camera target. This does not change the selected body.
- **Right-drag** pans the target. It also sets `follow_selected = false`, because a manually panned camera should not snap back to the selected body.
- **Home** resets azimuth, elevation, distance, target, and follow mode.

### Zooming

The mouse wheel and the `PageUp` / `PageDown` keys all use the same zoom function:

```text
distance = distance × 1.12^(−steps)
distance = clamp(distance, 0.05, 8.00)
```

Zooming in is useful for inspecting surfaces, Saturn's rings, black-hole accretion disks, and close moon orbits. When following a body, the camera also enforces a close-focus minimum distance so it does not clip inside the body.

### The spawn cursor

- The spawn cursor is the intersection of the mouse ray with the `y = 0` plane.
- It is clamped to `x ∈ [−1.20, +1.20]` and `z ∈ [−1.20, +1.20]`.
- A small ring is drawn on that plane.
- The cursor stays at its last valid position when the mouse leaves the viewport or the ray does not intersect the plane.
- `S` and `O` require a valid spawn cursor. `A` uses the last cursor position; if the mouse has never entered the viewport, that position is the origin `(0, 0)`.

---

## 4. Keyboard Quick Reference

| Key | Action |
|---|---|
| `Space` or `P` | Pause or resume |
| `+` or `=` | Increase speed by 1.5× |
| `-` or `_` | Decrease speed by 1.5× |
| `Up` / `Down` | Increase / decrease speed in the main view; scroll in overlays |
| `0` | Reset speed to 1.00× |
| `1` – `7` | Select spawn type: Star, Planet, Moon, Asteroid, Meteor, Comet, Black Hole |
| `A` | Add the selected type at the spawn cursor |
| `D` | Delete the selected body |
| `V` | Cycle spawn velocity: orbital → retrograde → zero/co-moving |
| `[` / `]` | Decrease / increase spawn mass scale by 25% |
| `,` / `.` | Decrease / increase spawn radius scale by 10% |
| `C` | Toggle realistic vs classic merge-only collisions |
| `X` | Toggle effective collision spheres |
| `U` | Toggle interactive dust |
| `O` | Promote the nearest dust particle to a physical body |
| `F1` or `H` | Open Tutorial & Controls |
| `E` | Open or close the Equation Lab |
| `B` | Open or close the Body Editor |
| `G` | Toggle the gravity grid |
| `T` | Toggle orbit trails |
| `Y` | Toggle selected-body trajectory prediction |
| `J` | Toggle thermal colours and glow |
| `K` | Toggle tidal deformation |
| `L` | Toggle body labels |
| `F` | Toggle camera follow for the selected body |
| `Home` | Reset the camera |
| `PageUp` / `PageDown` | Zoom in / out |
| `R` | Reset the universe in the main view; reset appearance in the Body Editor |
| `Q` or `Esc` | Quit in the main view; close/cancel in overlays |

> **Important:** several keys change meaning while an overlay is open. For example, `T` toggles orbit trails in the main view but toggles the guided tour inside the tutorial. `R` resets the universe in the main view but resets appearance in the Body Editor. `Up` / `Down` change speed in the main view but scroll or change fields in overlays.

---

## 5. Time and Simulation Speed

### Pause and resume

Press **`Space`** or **`P`** to pause or resume.

- Pausing freezes physics integration and simulation time.
- The camera, effects, and status messages keep updating.
- A large `PAUSED` indicator appears in the top strip.
- Spawning, deleting, editing, and toggling still work while paused. This is the safest way to place several bodies precisely.
- The status strip shows `Simulation paused` or `Simulation resumed`.

### Change speed

Press **`+`** or **`=`** to multiply the speed by `1.5`. Press **`-`** or **`_`** to divide it by `1.5`.

- Minimum: `0.10×`.
- Maximum: `10.00×`.
- The current value is shown in the top strip.
- The status message confirms each change, for example `Speed: 2.25x`.

The **`Up`** and **`Down`** arrow keys do the same thing in the main view. Inside an overlay they scroll instead.

### Reset speed

Press **`0`** to return to `1.00×` immediately.

### Reset the simulation

Press **`R`** in the main view to restore the built-in 26-body scene. This also resets:

- camera position and follow mode;
- simulation speed and pause state;
- collision mode;
- labels, trails, prediction, gravity grid, thermal, tidal, dust, and collision-debug toggles;
- spawn cursor validity and spawn counters;
- status messages and the event log;
- selection.

The spawn **type**, mass scale, radius scale, and velocity mode are retained across a reset. Set them again before your next spawn if you need a different configuration.

A reset is the fastest way to recover after an experiment destabilises the scene.

### Why speed and frame rate are linked

The simulation advances at `BASE_DT × speed × frame_ratio`, where `frame_ratio` is the measured real frame interval divided by 16 ms. This keeps wall-clock speed consistent when the frame rate changes. The engine then chooses 1–32 adaptive sub-steps so fast bodies do not tunnel through small ones. Very high speeds can still reduce accuracy at the 32-sub-step cap.

---

## 6. Camera and Selection

### Follow the selected body

Press **`F`** to toggle follow mode.

- Follow mode requires an active selection.
- The camera glides toward the selected body's **rendered** position, including the visual scaling used for moons.
- If you pan with right-drag, follow mode turns off.
- If you clear the selection, follow mode turns off.
- When following a body, close zoom is limited by the body's rendered size so the camera does not enter the sphere.

Typical use: select Saturn, press `F`, zoom in, and inspect the rings. Select a moon, press `F`, and watch its scaled visual orbit track its physical motion.

### Reset the camera

Press **`Home`** to restore:

- azimuth `45°`;
- elevation `35°`;
- distance `3.05`;
- target at the origin;
- follow mode off.

The status strip shows `Camera reset`.

### Zoom with the keyboard

- **`PageUp`** zooms in.
- **`PageDown`** zooms out.

Both use the same 1.12× step and the same `0.05–8.00` distance limits as the wheel.

### Labels (`L`)

- Default: on.
- The status strip shows `Labels on` or `Labels off`.
- Selected labels always take priority.
- Moons, asteroids, and meteors only appear when selected.
- Very close follow views hide other labels.

### Selection behaviour

- Selection is by rendered position, so visual moon scaling affects clickability.
- Hovering draws a subtle ring around the body under the pointer.
- Clicking empty space clears selection and follow mode.
- The Equation Lab and Body Editor can open without a prior selection; they fall back to the heaviest active body. If no body is active, they report an error.

---

## 7. Choosing and Spawning Bodies

### Choose a type

Press **`1`** through **`7`**:

| Key | Type |
|---|---|
| `1` | Star |
| `2` | Planet |
| `3` | Moon |
| `4` | Asteroid |
| `5` | Meteor |
| `6` | Comet |
| `7` | Black Hole |

The top strip updates immediately, and the status strip confirms, for example `Spawn type: Planet`.

### Add the body

1. Move the mouse into the simulation viewport so the spawn cursor ring appears.
2. Optionally pause with `Space`.
3. Optionally set the velocity mode with `V`.
4. Optionally adjust mass with `[` / `]` and radius with `,` / `.`.
5. Press **`A`**.

`A` uses the last cursor position. If the mouse has never entered the viewport, the body is placed at the origin `(0, 0)` and then pushed out of overlap with existing bodies.

What happens:

- The new body uses its type defaults multiplied by the current mass and radius scales.
- It is placed at the spawn cursor, then pushed out of overlap with existing bodies for up to eight passes.
- Comets use an eccentric orbit.
- Moons look for a nearby planet within `0.12` units and attach to it if found.
- Non-moon bodies choose a dominant local primary and receive orbital, retrograde, or co-moving velocity.
- The body receives a per-type serial name such as `Planet 1`, `Moon 2`, or `Asteroid 3`.
- The new body becomes selected.
- A spawn flash and expanding ring are emitted.
- The status strip reports `Added X orbiting Y`, or just `Added X` if no primary was found.

If the 256-slot pool is full, the spawn is rejected with `Cannot add: pool is full (256 bodies)`.

### Velocity modes

Press **`V`** to cycle:

1. **Orbital** — tangential velocity around the dominant local primary. For a moon near a planet, this means around that planet.
2. **Retrograde** — the same speed in the opposite direction.
3. **Zero / co-moving** — the body starts at rest relative to its primary, so it falls under gravity instead of orbiting immediately.

Use orbital for a stable satellite, retrograde to test counter-rotating orbits, and zero for impacts or free-fall experiments.

### Mass and radius scales

- **`[`** decreases spawn mass by 25%, to a minimum of 25%.
- **`]`** increases spawn mass by 25%, to a maximum of 400%.
- **`,`** decreases spawn radius by 10%, to a minimum of 50%.
- **`.`** increases spawn radius by 10%, to a maximum of 200%.

Scales apply only to newly spawned bodies, not to bodies already in the scene. The top strip shows the current multipliers.

### How the primary is chosen

For non-comet spawns:

1. A **planet or black hole** whose local influence sphere contains the cursor is preferred. The influence radius is `max(0.08, body.radius × 6)`, and the cursor must be outside `body.radius × 1.15`.
2. Otherwise, the star or black hole with the strongest gravitational acceleration at the cursor position is chosen.
3. If no candidate exists, the heaviest active body is used.

Comets always prefer the nearest star.

### How a moon is attached

If a moon is spawned within `0.12` units of a planet:

- The nearest planet becomes its parent.
- The physical orbit is placed at 20%–40% of the planet's Hill radius.
- The rendered orbit is scaled outward for visibility.
- The moon inherits the planet's velocity and adds its own orbital velocity.
- Its HUD shows the parent and real orbital radius.

If no planet is close enough, the moon uses the normal non-moon spawn path and is not attached.

### Naming

Names are serialized per type:

```text
Star 1, Star 2, ...
Planet 1, Planet 2, ...
Moon 1, Moon 2, ...
Asteroid 1, ...
Meteor 1, ...
Comet 1, ...
Black Hole 1, ...
```

Promoted dust uses `Dust N`, collision debris uses `Debris N`, and custom editor spawns use `Custom <Type> N`.

### Spawn limits

- The pool holds **256** active bodies.
- Free slots are reused.
- If the pool is full, spawning is rejected and a status message explains why.
- Deleting a body frees its slot immediately.

---

## 8. Deleting, Resetting, and Quitting

### Delete the selected body

Press **`D`**.

- The selected body is removed.
- If it was a planet, its moons are **detached**, not deleted. They stay active as independent bodies at their current physical positions.
- The selection is cleared.
- The slot becomes immediately reusable.
- The status strip reports `Deleted <name>`.

If nothing valid is selected, the status strip says `No body selected to delete`.

### Reset the universe

Press **`R`** in the main view.

This is a full reset, not an undo. Spawned bodies, deletions, merges, camera changes, and event history are discarded.

### Quit

Press **`Q`** or **`Esc`** in the main view to exit.

Inside overlays, the same keys close or cancel the overlay instead. See the overlay sections for the exact behaviour.

---

## 9. Collisions, Dust, and Diagnostics

### Toggle collision mode

Press **`C`**.

- **Realistic** — impacts can merge, bounce, graze, fragment, or disrupt depending on energy, impact parameter, and body type.
- **Classic** — every collision merges into the heavier body.

The top strip reports `collisions: realistic` or `collisions: classic`. The status strip confirms the switch.

Realistic mode is the default. Stars merge rather than fragment. Black holes absorb any body. A star that grows past the collapse threshold becomes a black hole.

### Show effective collision spheres

Press **`X`**.

- Draws a debug sphere for each body's effective collision radius.
- Default: off.
- Useful for understanding why two bodies did or did not collide.
- The status strip shows `Collision spheres on` or `Collision spheres off`.

The effective radius is normally:

```text
effective_radius = max(physical_collision_radius, visual_radius × 0.90)
```

Small bodies and parented moons use their physical radii because their visual positions are exaggerated. The `--physical-collisions` CLI flag switches the whole simulation to physical-only collision radii.

### Toggle dust

Press **`U`**.

- Default: on.
- Shows or hides the 1,024 interactive dust particles.
- Dust responds to gravity from all bodies.
- Dust collisions can create impact effects and event-log entries.
- While hidden, dust physics updates stop.
- Status: `Dust on` or `Dust off`.

### Promote dust to a body

Press **`O`**.

- Finds the nearest active dust particle within `0.20` units of the spawn cursor.
- Converts it into a **Meteor** named `Dust N`.
- Gives it a very small mass and a `0.0020` radius.
- Selects the new body.
- Emits a status message.

Failure cases:

- `Move the cursor over the scene first` — the spawn cursor is not valid.
- `No dust near cursor` — no dust particle is within range.
- `Cannot promote dust: pool is full` — all 256 body slots are occupied.

Promotion is useful for turning a visual particle into a real N-body body without opening the editor.

---

## 10. Visualisation Toggles

| Key | Toggle | Default | What it shows |
|---|---|---|---|
| `G` | Gravity grid | Off | A spacetime mesh depressed by gravitational potential |
| `T` | Orbit trails | On | Fading history lines behind bodies |
| `Y` | Trajectory prediction | On | The selected body's forecast path |
| `J` | Thermal colours and glow | On | Temperature-driven surface tint and emissive glow |
| `K` | Tidal deformation | On | Visible tidal bulge along the net tidal axis |
| `L` | Body labels | On | Projected names above bodies |
| `X` | Collision spheres | Off | Effective collision-radius debug spheres |
| `U` | Dust | On | Interactive dust particles |

Each toggle reports its new state in the status strip.

### Gravity grid (`G`)

The grid is a visual potential well. It does not affect physics. It is useful for showing how the Sun and massive planets dominate the local gravitational field.

### Orbit trails (`T`)

Trails show where bodies have been. They are renderer history, not physics data. Resetting the universe clears them.

### Trajectory prediction (`Y`)

Prediction forecasts the selected body's future path using a cached, softened two-body model refreshed periodically. It uses the same softened gravity as the simulation. Moon predictions are mapped into the scaled visual orbit. Prediction is an approximation, not a full N-body forecast.

### Thermal colours (`J`)

Hot bodies receive an emissive thermal glow; cold bodies can receive an icy tint. The HUD temperature values are independent of this visual toggle.

### Tidal deformation (`K`)

The renderer stretches the body sphere along the net tidal axis. Strong tides produce a visible warning glow. The effect is visual; full tidal disruption is not implemented.

### Label visibility

Turns projected names on or off. Selection still works when labels are hidden.

---

## 11. The Three Overlay Pages

The simulator has three full-screen pages. Each captures the keyboard while it is open.

| Page | Open | Close | Purpose |
|---|---|---|---|
| **Tutorial & Controls** | `F1` or `H` | `Esc`, `F1`, or `H` | Reference sections and a guided tour |
| **Equation Lab** | `E` | `E` or `Esc` | Live formulas and metrics for a selected body and a comparison body |
| **Body Editor** | `B` | `B` or `Esc` | Edit the selected body or spawn a custom copy |

When an overlay is open, the main-view shortcuts are suspended. For example, `T` does not toggle trails inside the tutorial; it toggles the guided tour. `R` does not reset the universe inside the Body Editor.

---

## 12. Spawn Workflows

### 12.1 Spawn a stable planet around the Sun

1. Press `Space` to pause.
2. Press `2` to choose Planet.
3. Press `V` until the status says `Spawn velocity: orbital`.
4. Move the cursor to a spot away from existing planets.
5. Press `A`.
6. Press `Space` to resume.

The planet receives a tangential velocity around the dominant primary and begins orbiting.

### 12.2 Spawn a moon around a chosen planet

1. Select the planet and press `F` to follow it, or zoom in on it.
2. Press `Space` to pause.
3. Press `3` to choose Moon.
4. Place the cursor within `0.12` simulation units of the planet.
5. Press `A`.

The moon is attached to the planet, placed inside its Hill sphere, and selected. Its HUD shows the parent and, for seeded moons, the real orbital radius.

### 12.3 Spawn a retrograde satellite

1. Choose the body type, for example `2` for Planet.
2. Press `V` until the status says `Spawn velocity: retrograde`.
3. Place the cursor away from the primary and press `A`.

The body receives the same speed as the orbital mode but in the opposite direction. This is useful for counter-rotating experiments and collision tests.

### 12.4 Create a direct impact

1. Press `Space` to pause.
2. Press `2` for Planet.
3. Press `V` until the status says `Spawn velocity: zero`.
4. Place the cursor just outside the target body.
5. Optionally increase mass with `]`.
6. Press `A`.
7. Press `Space` to resume.

With zero velocity, the new body is co-moving with its primary and falls under gravity. If you place it close enough, it will impact. Press `X` to see collision spheres and `C` to compare realistic and classic outcomes.

### 12.5 Spawn a comet

1. Press `6` to choose Comet.
2. Place the cursor away from the Sun.
3. Press `A`.

Comets use an eccentric orbit with a perihelion near the star and an apoapsis around `2.20` units. When the comet approaches the star, its activity and tails grow. Select it to read the activity percentage and tail length in the HUD.

### 12.6 Spawn a black hole

1. Press `7` to choose Black Hole.
2. Place the cursor away from existing bodies.
3. Press `A`.

The default black hole has mass `2.00`, heavier than the default Sun. Its event horizon and accretion disk are visible. Nearby orbits bend sharply, and any body that collides with it is absorbed and increases its mass.

### 12.7 Spawn a custom body from the Body Editor

1. Select a body to use as a template.
2. Press `B` to open the Body Editor.
3. Edit mass, radius, velocity, type, composition, or any other field.
4. Press `S` to spawn a copy at the cursor.

The copy is named `Custom <Type> N` and receives the custom-body history. The editor does **not** require the template to be spawned; it copies the current values of the selected body.

### 12.8 Promote a dust particle

1. Make sure `U` dust is on.
2. Move the cursor near a visible dust particle.
3. Press `O`.

The nearest dust particle within `0.20` units becomes a physical Meteor. This is a quick way to create a small impactor without using the spawn menu.

---

## 13. Body Editor Reference

Open with **`B`**. Close with **`B`** or **`Esc`**. Press **`Q`** to quit the application. If no body is selected, the editor falls back to the heaviest active body.

### Editor controls

| Input | Action |
|---|---|
| `Up` / `Down` | Select the previous / next field |
| `Home` | Jump to the first field |
| `Left` / `Right` | Decrease / increase the value |
| `PageUp` / `PageDown` | Coarse decrease / increase |
| `Enter` | Start typing a value; press again to apply |
| Digits, `.`, `-` | Enter a numeric value |
| `Backspace` | Delete the last typed character |
| `S` | Spawn a custom body from the current values at the cursor |
| `R` | Reset the selected body's appearance to type defaults |
| `B` | Close the editor |
| `Esc` | Cancel typing; if not typing, close the editor |
| `Q` | Quit the application |

While you are typing a value, only digits, `.`, `-`, `Backspace`, and `Enter` are accepted. This prevents `Q`, `B`, `S`, and `R` from quitting, closing, spawning, or resetting while you type.

### All 26 fields

Fields are shown in this order. The unit column matches the HUD.

| # | Field | Unit | What it controls |
|---:|---|---|---|
| 1 | Mass | Earth masses | The body's gravitational mass. Changing it changes gravity and collision momentum. |
| 2 | Visual radius | simulation units | The rendered sphere size. It is separate from the physical collision radius. |
| 3 | Volume | Earth volumes | Derived physical volume. Changing it at constant density scales the mass. |
| 4 | Density | kg/m³ | Physical density. Combined with mass, it determines the derived volume. |
| 5 | Collision radius | simulation units | The physical collision radius used when visible-surface collisions are not used. |
| 6 | Collision scale | 0–1.5 (HUD label says 0–1) | How much of the visual radius participates in visible-surface collisions. |
| 7 | Position X | simulation units | World-space X position. |
| 8 | Position Y | simulation units | World-space Y position. |
| 9 | Position Z | simulation units | World-space Z position. |
| 10 | Velocity X | simulation units/time | World-space X velocity. |
| 11 | Velocity Y | simulation units/time | World-space Y velocity. |
| 12 | Velocity Z | simulation units/time | World-space Z velocity. |
| 13 | Albedo | 0–1 | Reflectivity. Higher albedo lowers the equilibrium temperature. |
| 14 | Greenhouse | K | Added surface warming, used for Venus-like atmospheres. |
| 15 | Axial tilt | degrees | Visual axial tilt, from −360° to 360°. |
| 16 | Flattening | 0–0.6 | Oblateness of the rendered sphere. |
| 17 | Cloud amount | 0–1 | Procedural cloud coverage. |
| 18 | Spin rate | degrees/time | Visual rotation rate. Negative values rotate backwards. |
| 19 | Body type | enum | Star, Planet, Moon, Asteroid, Meteor, Comet, or Black Hole. |
| 20 | Composition | enum | Rocky, Icy, Metallic, Gas Giant, Stellar, or Exotic. |
| 21 | Atmosphere R | 0–1 | Red component of the atmosphere tint. |
| 22 | Atmosphere G | 0–1 | Green component of the atmosphere tint. |
| 23 | Atmosphere B | 0–1 | Blue component of the atmosphere tint. |
| 24 | Internal heat | K | Internal temperature contribution. |
| 25 | Thermal inertia | time units | How slowly the body heats and cools. |
| 26 | Tidal lock | 0/1 | Whether the body is tidally locked. |

### Adjustment rates

Small adjustments use `Left` / `Right`; coarse adjustments use `PageUp` / `PageDown`.

| Field group | Small step | Coarse step |
|---|---|---|
| Mass, volume, density | ×1.15 | ×2.0 |
| Visual radius | ×1.08 | ×1.08 |
| Collision radius | ×1.08 | ×1.08 |
| Collision scale | ±0.05 | ±0.05 |
| Position and velocity | ±0.01 | ±0.10 |
| Albedo | ±0.05 | ±0.05 |
| Greenhouse | ±5 K | ±20 K |
| Axial tilt | ±5° | ±15° |
| Flattening | ±0.01 | ±0.01 |
| Cloud amount | ±0.05 | ±0.05 |
| Spin rate | ±1 | ±5 |
| Atmosphere RGB | ±0.05 | ±0.05 |
| Internal heat | ±5 K | ±5 K |
| Thermal inertia | ±0.25 | ±0.25 |
| Tidal lock | toggles | toggles |
| Body type / composition | cycles | cycles |

### Typed values

Press `Enter` on a field to begin typing. Type the value and press `Enter` again.

- The editor validates the whole string.
- An invalid value leaves the body unchanged and shows `Invalid value for <field>`.
- Values are clamped after application where appropriate.
- Black-hole mass edits automatically recompute the event horizon and collision radius.

### Derived values and invariants

- **Mass** and **density** are the canonical physical values.
- **Volume** is derived from mass and density.
- **Visual radius** is a render property. Changing it does **not** silently change the physical collision radius.
- **Collision radius** and **collision scale** are displayed separately from the visual radius.
- For black holes, the horizon is derived from mass and clamped to `0.007–0.035` simulation units.
- Negative or zero masses are clamped to a tiny positive value.
- Radius is clamped to `1 × 10⁻⁵ – 0.5`.
- Albedo and cloud amount are clamped to `0–1`.
- Flattening is clamped to `0–0.6`.
- Axial tilt is clamped to `−360°–360°`.
- Greenhouse is clamped to `0–5,000 K`.
- Internal heat is clamped to `0–10,000 K`.
- Thermal inertia is clamped to `0–100`.
- Collision scale is clamped to `0–1.5`.

### Composition presets

Press `Left` / `Right` on the Composition field to cycle:

| Preset | Density | Albedo | Surface | Cloud |
|---|---:|---:|---|---:|
| Rocky | 3,000 kg/m³ | 0.12 | Rocky | 0.00 |
| Icy | 1,000 kg/m³ | 0.60 | Icy | 0.10 |
| Metallic | 7,000 kg/m³ | 0.08 | Rocky | 0.00 |
| Gas Giant | 1,000 kg/m³ | 0.35 | Gas giant | 0.30 |
| Stellar | 1,408 kg/m³ | 0.00 | Sun | 0.00 |
| Exotic | 1,000,000 kg/m³ | 0.00 | Rocky | 0.00 |

### Spawn a custom body (`S`)

- Requires a valid selected body and a valid spawn cursor.
- Copies the selected body's current values.
- Places it at the cursor with `y = 0`.
- Inherits the template's velocity.
- Names it `Custom <Type> N`.
- Normalizes the copied values.
- Selects the new body.

Failure messages:

- `Select a template body first`
- `Move the cursor over the scene first`
- `Cannot add: pool is full`

### Reset appearance (`R`)

Applies the selected body type's default surface, composition, colour, and atmosphere. It does **not** restore the original seeded mass, velocity, or history. For black holes it also recomputes the horizon geometry.

### Close the editor (`B` or `Esc`)

- `B` closes immediately.
- `Esc` cancels typing first; pressing it again closes the editor.

---

## 14. Equation Lab Reference

Open with **`E`**. Close with **`E`** or **`Esc`**.

### A and B bodies

- **A** is the currently selected body.
- **B** is the comparison body.
- When the lab opens, B defaults to the nearest other active body.
- Press **`N`** to cycle B through active bodies.
- **Shift + left-click** a body to set B, as long as it is not A.
- A normal left click in the lab selects a new A body.

If no body is selected when the lab opens, it selects the heaviest active body. If there is no active body, it closes with an error.

### Navigation

| Input | Action |
|---|---|
| `E` or `Esc` | Close the lab |
| `Tab` or `Left` / `Right` | Change tab |
| `Up` / `Down` | Scroll |
| `Home` | Reset scroll to the top |
| `N` | Cycle B |
| `Shift + left-click` | Set B |
| `Q` | Quit the application |

### The eight tabs

| # | Tab | What it reports |
|---:|---|---|
| 0 | **PAIR** | Pair force, acceleration of each body, distance, potential, escape velocity, relative speed, and direction |
| 1 | **ORBIT** | Specific orbital energy, angular momentum, eccentricity, semi-major axis, periapsis, apoapsis, and period |
| 2 | **THERMAL** | Surface temperature, equilibrium temperature, greenhouse/tidal/impact heating, and stellar luminosity |
| 3 | **TIDAL** | Tidal acceleration, tidal ratio, Roche limit, Hill radius, and tidal heating |
| 4 | **IMPACT** | Collision mode, effective radii, collision scales, relative speed, impact parameter, specific impact energy, and predicted outcome |
| 5 | **BLACK HOLE** | Event horizon, photon sphere, ISCO, and time dilation at 2 event-horizon radii |
| 6 | **BODY** | Mass, radius, density, surface gravity, escape velocity, axial tilt, flattening, composition, and effective collision radius |
| 7 | **EVERYTHING** | Net force/acceleration/tidal vectors, system kinetic/potential/total energy, system momentum, every gravitational contributor sorted by force, orbit, thermal, tidal, rotation, collision, and black-hole metrics |

### Reading the PAIR tab

```text
F = G m1 m2 / (r + eps)^2
a(A) = G m(B) / (r + eps)^2
a(B) = G m(A) / (r + eps)^2
U = -G m1 m2 / (r + eps)
v_escape = sqrt(2G(m1 + m2)/(r + eps))
```

- A negative potential means the pair is gravitationally bound.
- The relative speed tells you how fast the two bodies are approaching or separating.
- The direction is the unit vector from A to B.

### Reading the ORBIT tab

- **Specific energy** below zero means a bound orbit. Zero is parabolic; positive is unbound.
- **Eccentricity** below 1 is elliptical; 1 is parabolic; above 1 is hyperbolic.
- **Semi-major axis** describes the size of the orbit.
- **Periapsis** and **apoapsis** are the closest and farthest points.
- **Period** is the time for one full orbit under the two-body approximation.

The tab uses the standard vis-viva relation:

```text
v² = μ(2/r − 1/a)
```

### Reading the THERMAL tab

```text
L = 4πR²σT⁴
T_eq = [flux(1 − albedo)/(4σ)]^(1/4)
T_surface = T_eq + greenhouse + tidal heating + impact heating
```

- Surface temperature is the current temperature.
- Equilibrium temperature is the baseline from stellar flux and albedo.
- Greenhouse, tidal, and impact terms are added on top.
- A star's luminosity is estimated from its mass.
- A non-star B provides no luminous flux.

### Reading the TIDAL tab

```text
tidal acceleration = 2GMR/d³
Roche limit        ≈ R_primary × cbrt(2 × density_primary / density_body)
Hill radius        ≈ a × cbrt(m / (3M))
```

- A larger tidal ratio means stronger tidal stress.
- The Roche limit is the distance inside which a fluid body could be disrupted; disruption itself is not implemented.
- The Hill radius is the region where the body's gravity dominates over the primary.
- Tidal heating is a bounded, stylised temperature contribution.

### Reading the IMPACT tab

- **Collision mode** — physical radii or visual-surface radii.
- **Effective radii** — the radii actually used for the collision test.
- **Impact parameter** `b/(r1+r2)` — 0 is a head-on impact; 1 is a grazing edge.
- **Specific impact energy Q** — higher values favour fragmentation or disruption.
- **Predicted outcome** — Merge, Bounce, Graze, Fragment, Disrupt, or Absorb.

The prediction is an educational approximation based on the current pair state, not a guarantee of the next frame's result.

### Reading the BLACK HOLE tab

- **Horizon radius** — the event horizon used by the simulation.
- **Photon sphere** — `1.5 × horizon`.
- **ISCO** — the innermost stable circular orbit, `3 × horizon`.
- **Time dilation at 2 horizons** — a stylised relativistic value.

If neither A nor B is a black hole, the tab reports that.

### Reading the EVERYTHING tab

This is the full interaction report for A:

- Net force, net acceleration, and net tidal acceleration vectors.
- System kinetic, potential, and total energy.
- System momentum.
- Every gravitational contributor sorted by force, with distance, force, acceleration, and tidal contribution.
- The A–B orbit and whether it is bound.
- Surface and equilibrium temperature, greenhouse, tidal heating, and impact heating.
- Tidal acceleration, ratio, Roche limit, and Hill radius.
- Rotation, axial tilt, flattening, and tidal-lock state.
- Collision effective radius and relative speed.
- Black-hole metrics if A is a black hole.

Use this tab when you want to know *why* a body is moving the way it is. The force-sorted list shows which other bodies dominate its motion.

---

## 15. Tutorial and Controls Page

Open with **`F1`** or **`H`**. Close with **`Esc`**, **`F1`**, or **`H`**.

### Eight reference sections

1. **Getting Started** — the basic loop of rotate, select, inspect, and open tools.
2. **Camera** — mouse and keyboard camera controls.
3. **Time** — pause, speed, reset, collision modes, and HUD status.
4. **Spawning** — body types, adding, deleting, velocity modes, and scales.
5. **Body Editor** — field navigation, typing, spawning, and reset.
6. **Equation Lab** — A/B selection, tabs, scrolling, and the Everything tab.
7. **Tidal / Thermal** — temperature sources, thermal inertia, tides, Roche limits, and locking.
8. **Dust / Keys** — dust, promotion, key summary, and CLI flags.

### Tutorial controls

| Input | Action |
|---|---|
| `Left` / `Right` | Previous / next section |
| `Tab` | Next section |
| `Up` / `Down` | Scroll |
| `Home` | Scroll to top |
| `T` | Toggle the guided tour |
| `Enter` or `Space` | Next tour step |
| `Backspace` | Previous tour step |
| `Esc`, `F1`, or `H` | Close the tutorial |
| `Q` | Quit the application |

### Guided tour

Press **`T`** to start or stop the tour. The tour has 12 steps:

1. Rotate the camera with left-drag.
2. Select a body with left-click.
3. Pause with Space and change speed with `+` / `-`.
4. Press `E` to open the Equation Lab.
5. Shift-click a second body to choose B.
6. Press `B` to open the Body Editor.
7. Change mass, radius, or volume with Left/Right.
8. Press `S` to spawn the edited custom body.
9. Press `X` to show collision spheres.
10. Press `O` to promote the nearest dust particle.
11. Open the EVERYTHING tab in the Equation Lab.
12. Press `F1` or `H` to close the tutorial.

The tour is instruction-only; it does not move the camera or highlight widgets automatically.

---

## 16. Command-Line Options

```text
space_simulator [options]
```

| Option | Description |
|---|---|
| `--selftest` | Run the headless self-test suite and exit |
| `--screenshot [path]` | Render for 1.5 s, save a PPM screenshot, and exit |
| `--select NAME` | Select the exact body name at startup (case-sensitive) |
| `--camera AZ EL DIST` | Set initial azimuth, elevation, and distance |
| `--follow` | Follow the selected body at startup |
| `--lab` | Open the Equation Lab at startup |
| `--lab-tab N` | Open the lab on tab N (0–7) |
| `--editor` | Open the Body Editor at startup |
| `--tutorial` | Open Tutorial & Controls at startup |
| `--tour` | Open the tutorial with the guided tour active |
| `--grid` | Enable the gravity grid at startup |
| `--no-trails` | Disable orbit trails |
| `--no-prediction` | Disable trajectory prediction |
| `--classic` | Use classic merge-only collisions |
| `--physical-collisions` | Use physical collision radii instead of visible surfaces |
| `--collision-debug` | Show effective collision spheres at startup |
| `--impact-demo` | Spawn two planets on a high-energy collision course |
| `--collapse-demo` | Spawn a massive star that collapses into a black hole |
| `--comet-demo` | Spawn an active comet |
| `--help`, `-h` | Print usage and exit |

Options compose. Unknown options print a warning and are ignored.

### Examples

```bash
# Normal interactive run
./space_simulator

# Run the test suite
./space_simulator --selftest

# Select Earth and use a closer camera
./space_simulator --select Earth --camera 60 25 2.2

# Follow Saturn and inspect its rings
./space_simulator --select Saturn --follow --camera 25 8 0.085

# Open the Equation Lab on the Everything tab
./space_simulator --select Earth --lab --lab-tab 7

# Open the Body Editor at startup
./space_simulator --editor

# Show the gravity grid and capture a screenshot
./space_simulator --grid --select Earth --camera 55 30 2.6 --screenshot earth.ppm

# Collision, collapse, and comet demos
./space_simulator --impact-demo
./space_simulator --collapse-demo
./space_simulator --comet-demo
```

### Screenshots

`--screenshot` writes a binary PPM (`P6`) file. The default path is `space_simulator.ppm` when no path is supplied. Convert it with ImageMagick:

```bash
convert solar_system.ppm solar_system.png
```

Screenshot mode needs a display. On a headless machine:

```bash
xvfb-run -a ./space_simulator --screenshot solar_system.ppm
```

If the argument after `--screenshot` begins with `-`, it is treated as another option, so the default path is used.

### Camera CLI values

- Azimuth is unbounded and measured in degrees.
- Elevation is clamped to `5°–85°`.
- Distance is clamped to `0.05–8.00`.

---

## 17. Status Messages and Troubleshooting

### Status messages

Status messages appear in the top strip for **180 timer ticks**, roughly three seconds at 60 FPS. They are transient and separate from the physics event log.

| Message | Meaning | What to do |
|---|---|---|
| `Simulation paused` / `Simulation resumed` | Pause state changed | None |
| `Speed: 2.25x` | Speed changed | None |
| `Speed reset to 1.00x` | Speed returned to default | None |
| `Spawn type: Planet` | Spawn type changed | None |
| `Spawn velocity: orbital` | Velocity mode changed | None |
| `Spawn mass scale: 1.25x` | Mass scale changed | None |
| `Spawn radius scale: 1.10x` | Radius scale changed | None |
| `Added Planet 3 orbiting Sun` | Spawn succeeded | The new body is selected |
| `Cannot add: pool is full (256 bodies)` | No free body slot for `A` | Delete a body or reset |
| `Cannot add: pool is full` | No free body slot for a custom editor spawn | Delete a body or reset |
| `Cannot promote dust: pool is full` | No free body slot for dust promotion | Delete a body or reset |
| `Deleted Earth` | Delete succeeded | Selection cleared |
| `No body selected to delete` | `D` pressed with no valid selection | Select a body first |
| `Selection cleared` | Empty-space click | None |
| `Labels on` / `Labels off` | Label toggle | None |
| `Gravity grid on` / `Gravity grid off` | Grid toggle | None |
| `Orbit trails on` / `Orbit trails off` | Trail toggle | None |
| `Trajectory prediction on` / `off` | Prediction toggle | None |
| `Thermal colours on` / `off` | Thermal toggle | None |
| `Tidal deformation on` / `off` | Tidal toggle | None |
| `Dust on` / `Dust off` | Dust toggle | None |
| `Collision spheres on` / `off` | Debug-sphere toggle | None |
| `Collision mode: realistic` / `classic merge` | Collision mode changed | None |
| `Camera following selected body` | Follow enabled | None |
| `Camera follow off` | Follow disabled | None |
| `Select a body before following it` | `F` pressed without a selection | Select a body |
| `Camera reset` | `Home` reset the camera | None |
| `Promoted Dust 7 to a physical body` | Dust promotion succeeded | The new meteor is selected |
| `No dust near cursor` | No dust within `0.20` units | Move the cursor closer |
| `Move the cursor over the scene first` | Spawn cursor is invalid | Move the mouse into the viewport |
| `Body Editor: Earth` | Editor opened | None |
| `Reset Earth appearance to type defaults` | `R` in the editor | None |
| `Spawned custom body Custom Planet 1` | Custom spawn succeeded | The new body is selected |
| `Select a template body first` | `S` without a selection | Select a template |
| `Invalid value for Mass` | Typed value rejected | Enter a valid number |
| `Equation Lab: A=Earth B=Moon` | Lab opened | None |
| `Equation target: Jupiter` | B changed in the lab | None |
| `Equation Lab needs an active body` | No active body available | Reset with `R` |
| `Tutorial & Controls` | Tutorial opened | None |
| `Tutorial closed` | Tutorial closed | None |
| `Equation Lab closed` | Lab closed | None |
| `Body Editor closed` | Editor closed | None |
| `Solar system seeded with 26 bodies` | Reset completed | None |

### Common problems

| Problem | Cause | Solution |
|---|---|---|
| Nothing spawns when pressing `A` | The pool is full, or the body was placed at the last cursor position and is not obvious | Check the status message; move the mouse into the viewport and try again |
| A moon does not attach to a planet | The cursor was farther than `0.12` units from the planet | Zoom in and place the cursor closer |
| A body passes through another | Collision mode uses physical radii or the bodies are too small | Press `X` to inspect spheres; try `--physical-collisions` comparison |
| The scene looks empty after an experiment | Bodies merged, scattered, or were removed as unstable | Press `R` to reset |
| The camera is stuck following a body | Follow mode is on | Press `F`, or pan with right-drag |
| `Esc` does not quit | An overlay is open, or the editor is typing | Close the overlay first; press `Esc` twice in the editor |
| `Q` does nothing in the editor | You are typing a numeric value | Press `Enter` or `Esc` first |
| Typed value is rejected | The entire string must be a finite number | Use digits, `.`, and `-` only |
| Window is black on a server | No display server or no OpenGL 3.3 context | Use `xvfb-run`; check the OpenGL version printed at startup |
| FPS is low | Large body count, high speed, dust, grid, trails, and prediction all enabled | Lower speed, hide dust/grid/trails, or delete bodies |

### Unstable-body removal

If a body's position or velocity becomes non-finite, the simulation removes it automatically and logs an unstable-body event. This is a safety net, not a normal outcome. Reset with `R` if it happens after an extreme experiment.

---

## 18. Tips and Known Limits

### Practical tips

- **Pause before placing several bodies.** Physics is frozen, so your placements do not drift between spawns.
- **Use zero velocity for impacts.** Place the new body near the target and let gravity pull it in.
- **Use orbital velocity for stable satellites.** Place the body at a reasonable distance and spawn.
- **Use `[` / `]` to make a collision more dramatic.** A heavier impactor produces more debris.
- **Follow Saturn and zoom in.** The ring bands, gaps, and shadows are best seen up close.
- **Follow a moon.** The HUD shows both the real orbital radius and the scaled visual orbit.
- **Press `G` and `Y` for insight.** The gravity grid and prediction line make the physics visible.
- **Compare `C` modes.** Realistic mode fragments and bounces; classic mode always merges.
- **Use `O` for a quick impactor.** A promoted dust particle is a real meteor with the dust's velocity.
- **Use `R` freely.** It restores the full 26-body scene, camera, and state.

### Known limits

- No save/load or scenario presets.
- No full tidal disruption streams or Roche-limit fragmentation.
- No material-strength, spin-transfer, or full angular-momentum collision model.
- No white dwarf or neutron star remnants.
- No black-hole gravitational lensing; the photon ring and accretion disk are visual guides.
- No Barnes-Hut or octree gravity; the body pool is capped at 256.
- Planet orbits remain circular and coplanar by design; axial tilt and spin are visual.
- The asteroid belt is cosmetic and Keplerian; only the four seeded major asteroids are physical.
- Saturn's rings are cosmetic and not collision bodies.
- Dust is a hybrid system: 1,024 visual/dynamic particles, while promotion uses the shared 256-body pool.
- The Body Editor uses a scrolling 26-field list rather than separate pages.
- No audio, clickable menu widgets, or automatic tutorial highlighting.
- The simulation is chaotic over long time spans; small changes can produce very different outcomes.

For the full body histories, see [clbdhistory.md](clbdhistory.md). For the feature catalogue, see [functionalities.md](functionalities.md). For the original reference docs, see [docs/controls.md](docs/controls.md) and [README.md](README.md).
