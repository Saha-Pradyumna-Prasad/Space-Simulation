# Space Simulator v3.5 — Functionalities and Features

This file is the complete functionality catalogue for **Space Simulator v3.5**. It describes what the project does, how each feature is used, the important rules behind it, and the limits that are intentionally left out.

For button-by-button operation, see [instructions.md](instructions.md). For body histories and real-world profiles, see [clbdhistory.md](clbdhistory.md).

---

## Contents

- [1. Real-Time N-Body Simulation](#1-real-time-n-body-simulation)
- [2. Default 26-Body Solar System](#2-default-26-body-solar-system)
- [3. Body Types and Runtime Spawning](#3-body-types-and-runtime-spawning)
- [4. Orbital Placement and the Spawn Solver](#4-orbital-placement-and-the-spawn-solver)
- [5. Moon System and Visual Scaling](#5-moon-system-and-visual-scaling)
- [6. Collision Detection and Impact Outcomes](#6-collision-detection-and-impact-outcomes)
- [7. Thermal Model](#7-thermal-model)
- [8. Tidal Model](#8-tidal-model)
- [9. Comet Model](#9-comet-model)
- [10. Black Holes and Stellar Collapse](#10-black-holes-and-stellar-collapse)
- [11. Gravity Grid, Orbit Trails, and Trajectory Prediction](#11-gravity-grid-orbit-trails-and-trajectory-prediction)
- [12. Interactive Dust and Physical Promotion](#12-interactive-dust-and-physical-promotion)
- [13. Camera and Navigation](#13-camera-and-navigation)
- [14. Picking, Selection, and Labels](#14-picking-selection-and-labels)
- [15. HUD and Educational Data](#15-hud-and-educational-data)
- [16. Equation Lab](#16-equation-lab)
- [17. Body Editor](#17-body-editor)
- [18. Tutorial and Controls with Guided Tour](#18-tutorial-and-controls-with-guided-tour)
- [19. Rendering and Visual Scene](#19-rendering-and-visual-scene)
- [20. Time Controls and Simulation State](#20-time-controls-and-simulation-state)
- [21. Event Log and Status System](#21-event-log-and-status-system)
- [22. Command-Line Interface and Demos](#22-command-line-interface-and-demos)
- [23. Self-Tests and Diagnostics](#23-self-tests-and-diagnostics)
- [24. Platform and Build Support](#24-platform-and-build-support)
- [25. Source Architecture](#25-source-architecture)
- [26. Known Limitations and Deferred Work](#26-known-limitations-and-deferred-work)
- [27. Verification Summary](#27-verification-summary)

---

## 1. Real-Time N-Body Simulation

**What it does**

Every active body gravitationally attracts every other active body. Orbits are not pre-computed animations; planets, moons, comets, debris, and anything you spawn follow the paths produced by the current forces.

**How it works**

- Pairwise Newtonian gravity is evaluated for every unordered pair.
- Newton's third law is applied, so each pair is calculated once.
- The integration scheme is **semi-implicit Euler**: velocity is updated first, then position.
- The calculation is `O(N²)`. At the 256-body cap, that is 32,640 pair evaluations per integration step.
- Two softening lengths are used:
  - `0.01` for general pairs;
  - `0.0001` for a moon and its parent.
- The base step is `0.0025` simulation-time units.
- Adaptive sub-stepping chooses **1 to 32** sub-steps per frame based on the fastest body and the smallest radius.
- Bodies whose position or velocity becomes non-finite are removed automatically.
- The object pool has **256** slots, with free-slot reuse.

**Why it feels stable**

Softening prevents singular accelerations during extremely close encounters. The semi-implicit update is first-order symplectic, which is substantially more stable for orbital motion than explicit Euler. Adaptive sub-stepping protects fast meteors and comets from tunnelling through small bodies.

**Limits**

- No Barnes-Hut tree, octree, or GPU acceleration.
- Close encounters below the softening length are approximated rather than fully resolved.
- The system is chaotic over long spans; small changes can produce very different outcomes.

---

## 2. Default 26-Body Solar System

**What it does**

The startup scene contains a complete, compressed solar system with **26 active bodies**:

| Category | Count | Bodies |
|---|---:|---|
| Star | 1 | Sun |
| Planets | 8 | Mercury, Venus, Earth, Mars, Jupiter, Saturn, Uranus, Neptune |
| Moons | 13 | Moon, Phobos, Deimos, Io, Europa, Ganymede, Callisto, Enceladus, Rhea, Titan, Titania, Oberon, Triton |
| Major asteroids | 4 | Ceres, Vesta, Pallas, Hygiea |

**What each body carries**

- Name and type.
- Simulation mass and visual radius.
- Real-world radius, orbital radius, orbital period, rotation period, mass ratio, volume ratio, density, albedo, axial tilt, and greenhouse offset where applicable.
- A multi-sentence history used by the HUD and documented in [clbdhistory.md](clbdhistory.md).
- Surface kind, composition, colour, atmosphere colour, cloud amount, and procedural seed.
- Moon parent and visual orbit scaling where applicable.

**Scale**

The scene is compressed: Mercury orbits at `0.18` simulation units and Neptune at `1.12`, not at their real proportional distances. The Sun has mass `1.0` and visual radius `0.060`. The whole system fits inside a scene roughly 2.24 units across.

**Seed orbits**

- Planets are seeded with softened circular speeds around the Sun.
- The Sun's velocity is adjusted so the total momentum of the seeded system is zero.
- Moons are seeded inside 20%–40% of their parent planet's Hill sphere.
- Triton is seeded retrograde.
- The four major asteroids are separate physical bodies inside the visual asteroid belt.

**Reset**

Press `R` in the main view to restore the 26-body scene, camera, speed, pause state, collision mode, visual toggles, spawn counters, and event log.

---

## 3. Body Types and Runtime Spawning

**What it does**

Seven body types can be created at runtime. Each has its own default mass, radius, density, albedo, greenhouse offset, and visual treatment.

| Key | Type | Default mass (sim) | Default radius (sim) | Density (kg/m³) | Visual treatment |
|---|---|---:|---:|---:|---|
| `1` | Star | 0.50 | 0.050 | 1,408 | Emissive sphere plus layered glow |
| `2` | Planet | 3.003 × 10⁻⁶ | 0.012 | 5,514 | Lit sphere with planet colour |
| `3` | Moon | 3.700 × 10⁻⁸ | 0.006 | 3,344 | Lit sphere; parented to a nearby planet when possible |
| `4` | Asteroid | 1.000 × 10⁻⁹ | 0.004 | 3,000 | Low-detail lit sphere |
| `5` | Meteor | 1.000 × 10⁻¹⁰ | 0.003 | 3,000 | Low-detail sphere plus a velocity streak |
| `6` | Comet | 1.000 × 10⁻¹⁰ | 0.004 | 600 | Low-detail sphere plus coma and tails |
| `7` | Black Hole | 2.00 | 0.015 (mass-derived horizon) | 1.0 × 10⁹ | Black sphere with an emissive purple torus |

**How to spawn**

1. Press `1`–`7` to choose a type.
2. Move the mouse into the viewport to set the spawn cursor.
3. Optionally press `V` to choose orbital, retrograde, or zero/co-moving velocity.
4. Optionally adjust mass with `[` / `]` and radius with `,` / `.`.
5. Press `A`.

**Spawn rules**

- The cursor is the intersection of the mouse ray with the `y = 0` plane, clamped to `x ∈ [−1.20, +1.20]` and `z ∈ [−1.20, +1.20]`.
- The new body is pushed out of overlap with existing rendered bodies for up to eight passes.
- Mass and radius are the type defaults multiplied by the current spawn scales.
- Mass scale range: 25%–400% in 25% steps.
- Radius scale range: 50%–200% in 10% steps.
- Names are serialized per type: `Planet 1`, `Planet 2`, `Moon 1`, and so on.
- The new body becomes selected.
- A spawn flash and expanding ring are emitted.
- If the 256-slot pool is full, the spawn is rejected.

**Deletion**

- `D` deletes the selected body.
- Deleting a planet detaches its moons instead of deleting them.
- The selection is cleared.
- The slot is immediately reusable.

**Reset**

- `R` restores the default scene and most simulation/UI state. Spawn type, scales, and velocity mode are retained.

---

## 4. Orbital Placement and the Spawn Solver

**What it does**

The spawn solver gives a new body a physically sensible starting velocity instead of dropping it at rest.

**Primary selection**

For non-comet spawns:

1. A nearby **planet or black hole** is preferred when the cursor is inside its local influence sphere.
   - The influence radius is `max(0.08, body.radius × 6)`.
   - The cursor must be outside `body.radius × 1.15`.
2. Otherwise, the star or black hole with the strongest gravitational acceleration at the cursor is used.
3. If no candidate exists, the heaviest active body is used.

Comets always prefer the nearest star.

**Orbit plane**

The orbit is built in the primary's angular-momentum plane. The normal is derived from the primary's position and velocity relative to the heaviest body. If the angular momentum is negligible, the solver falls back to the `y` axis.

**Velocity modes**

| Mode | Behaviour |
|---|---|
| **Orbital** | Softened circular speed in the tangential direction around the primary |
| **Retrograde** | The same speed in the opposite direction |
| **Zero / co-moving** | The body inherits the primary's velocity and starts at rest relative to it |

**Comet orbits**

Comets use an eccentric vis-viva orbit:

- perihelion: `max(0.08, star_radius × 3.0)`;
- apoapsis: `2.20`;
- speed: `sqrt(G × M × (2/r − 1/a))`.

**Moon spawning**

- If a moon is spawned within `0.12` units of a planet, that planet becomes its parent.
- The physical orbit is 20%–40% of the planet's Hill radius.
- The rendered orbit is scaled outward for visibility.
- The moon inherits the planet's velocity and adds its own orbital velocity.
- If no planet is close enough, the normal non-moon spawn path is used.

**Limits**

- The orbit solver is a two-body approximation. N-body perturbations begin immediately after spawning.
- The spawn position is clamped to a flat `y = 0` plane.
- Very close spawns can still produce unstable or colliding orbits.

---

## 5. Moon System and Visual Scaling

**What it does**

Moons are true N-body bodies, but their real orbits are tiny at the simulator's compressed scale. The moon system separates the **physical orbit** from the **rendered orbit**.

**Physical orbit**

```text
r_Hill = parent_orbit × cbrt(parent_mass / (3 × sun_mass))
fraction = clamp(0.15 + 0.25 × (visual_radius_scale − 1.5) / 1.3, 0.20, 0.40)
physical_orbit = r_Hill × fraction
```

Every seeded moon is placed between 20% and 40% of its parent's Hill radius. The initial speed is the softened circular speed:

```text
v = sqrt(G × parent_mass × physical_orbit) / (physical_orbit + MOON_SOFTENING)
```

**Visual orbit**

```text
visual_orbit = parent.radius × visual_radius_scale
visual_scale = visual_orbit / physical_orbit
render_position = parent.position + (moon.position − parent.position) × visual_scale
```

The physics still uses the physical orbit, so gravity and collisions remain Hill-safe. The HUD reports both the real orbital radius and the visual orbit size in planet radii.

**Tidal locking**

Seeded moons are marked as tidally locked. The environment drives their spin toward synchronous rotation. This is a visual and thermal behaviour, not a full angular-momentum model.

**Detachment**

When a parent body is removed or absorbed, its moons are detached rather than deleted. They stay active as independent bodies at their current physical positions. Their rendered positions switch back to the physical positions, so they may appear to move inward after detachment.

**Limits**

- No tidal orbital evolution, despinning beyond the stylised lock, or moon formation.
- No full Roche-limit disruption of moons.

---

## 6. Collision Detection and Impact Outcomes

**What it does**

The collision system detects close encounters, chooses an outcome based on energy and geometry, and conserves mass and momentum.

**Detection**

Each body has:

- `radius` — rendered sphere size.
- `collision_radius` — physical collision size.
- `collision_scale` — how much of the visual radius participates in visible collisions.

Default effective radius:

```text
effective_radius = max(collision_radius, radius × 0.90)
```

Small bodies and parented moons use their physical radii because their rendered positions are visually exaggerated.

**Swept test**

If two bodies do not overlap at their current positions, the solver tests their relative motion during the last integration step:

```text
p0 = previous_position_j − previous_position_i
d  = current_position_j  − current_position_i
s  = d − p0
t  = clamp(−dot(p0, s) / dot(s, s), 0, 1)
closest = p0 + t × s
hit = |closest|² < (r_i + r_j)²
```

This catches fast bodies that would otherwise pass through a target between frames.

**Outcomes**

| Outcome | Meaning |
|---|---|
| **Merge** | Low-energy accretion; the heavier body survives |
| **Bounce** | Low-energy rebound with restitution |
| **Graze** | Off-centre momentum exchange |
| **Fragment** | Debris creation from the smaller body |
| **Disrupt** | Catastrophic debris creation |
| **Absorb** | Black-hole collision |

**Type rules**

- Star + anything: the star merges/absorbs; stars never fragment.
- Star + black hole: the black hole absorbs the star.
- Black hole + anything: absorption and growth.
- Planet + planet: merge, bounce, graze, fragment, or disrupt.
- Moon + moon: the same realistic classifier.
- Asteroid or comet + planet: merge, graze, fragment, or crater.

**Merging**

The heavier body is kept. Mass and 3D momentum are conserved, the result moves to the centre of mass, the radius grows by the cube root of the mass ratio, and volume ratios add:

```text
total = m1 + m2
velocity = (m1v1 + m2v2) / total
position = (m1p1 + m2p2) / total
radius = keep.radius × cbrt(total / m1)
volume = keep.volume + gone.volume
```

**Fragmentation**

- A fragmenting impact creates 5 debris bodies, or 8 for a disruptive impact.
- The cap is **10** fragments per event.
- Debris are Asteroids named `Debris N`.
- Total mass and momentum are conserved.
- Fragmentation is limited by the 256-body pool; if fewer than two slots are free, the impact merges instead.

**Cooldown**

A bounce or graze starts a `0.08`-time-unit collision cooldown so the pair does not re-trigger on consecutive frames.

**Modes**

- **Realistic** — the full outcome classifier. Default.
- **Classic** — every collision merges into the heavier body. Toggle with `C` or `--classic`.
- **Visible-surface** — default collision radii, with `max(physical, visual × 0.90)`.
- **Physical-only** — `--physical-collisions` uses physical radii for everything.

**Debugging**

Press `X` to show effective collision spheres for every body.

**Limits**

- No material strength, spin transfer, cratering, or full angular-momentum collision model.
- No full tidal disruption streams.
- No white dwarf or neutron star remnants.

---

## 7. Thermal Model

**What it does**

Every non-star body receives stellar flux and develops a surface temperature. Temperature drives thermal glow and comet activity.

**Model**

```text
flux = Σ (L_star / L_sun) / (distance / EARTH_ORBIT_SIM)²
T_eq = 255 K × ((1 − albedo) × flux / (1 − 0.30))^(1/4)
T_surface = T_eq + greenhouse_k + tidal_heating_k + impact_heating_k
```

- Star luminosity scales as `mass^3.5`.
- Earth is calibrated to roughly `287 K` at its seeded orbit.
- Venus has a large greenhouse term.
- Neptune receives very little flux.
- Impact heating decays exponentially.
- Thermal inertia makes heating and cooling gradual.
- Hot bodies emit a visible thermal glow; cold bodies can receive an icy tint.
- Tidal heating and internal heat add temperature contributions.
- Comet activity is derived from surface temperature.

**Where to see it**

- The selected-body HUD shows surface temperature and equilibrium temperature.
- The Equation Lab **THERMAL** tab shows surface temperature, equilibrium temperature, stellar luminosity, greenhouse, tidal heating, and impact heating.
- Press `J` to toggle thermal colours and glow.

**Limits**

- This is a calibrated educational approximation, not a radiative-transfer climate model.
- No atmospheric circulation, albedo feedback, or seasonal energy transport.

---

## 8. Tidal Model

**What it does**

The simulator computes tidal stress, tidal heating, Roche limits, Hill radii, and a visible tidal bulge.

**Dominant primary**

For each body, the environment finds the strongest tidal primary. The tidal axis points toward that primary. The renderer stretches the body along the net tidal vector from all bodies.

**Formulas**

```text
tidal_ratio = 2 × M_primary × R_body³ / (M_body × d³)
Roche_limit ≈ R_primary × cbrt(2 × density_primary / density_body)
Hill_radius ≈ a × cbrt(m / (3M))
```

- Tidal acceleration is `2GMR/d³`.
- Tidal heating is a bounded, stylised temperature contribution.
- Strong tides produce a visible warning/glow.
- Tidal locking drives moon spin rates toward synchronous rotation.

**Where to see it**

- The HUD shows tidal stress and Roche limit.
- The Equation Lab **TIDAL** tab shows tidal acceleration, ratio, Roche limit, Hill radius, and heating.
- The **EVERYTHING** tab shows the net tidal vector and all contributors.
- Press `K` to toggle tidal deformation.

**Limits**

- Tidal disruption and Roche-limit fragmentation are deferred.
- Tidal orbital evolution and despinning are simplified.
- The bulge is a visual deformation, not a fluid-dynamics simulation.

---

## 9. Comet Model

**What it does**

Comets develop a coma and two tails as they approach a star.

**Model**

```text
activity = clamp((T_surface − 160 K) / 260 K, 0, 1)
coma_radius = nucleus_radius × (1.5 + 3.5 × activity)
tail_length = 0.06 + 0.55 × activity
```

- Activity is driven by surface temperature and stellar flux.
- The ion tail points along the net anti-stellar radiation vector.
- The dust tail is curved and lags behind the orbital motion.
- The coma is an additive glow around the nucleus.
- The HUD reports activity percentage and tail length.
- Comet spawns use an eccentric orbit with a perihelion near the star and an apoapsis around `2.20`.

**Where to see it**

- Select a comet and read the HUD.
- Try `./space_simulator --comet-demo`.
- Spawn a comet with `6`, then `A`.

**Limits**

- The tail is a visual model, not a gas-dynamics simulation.
- No nucleus rotation, fragmentation, or outbursts.

---

## 10. Black Holes and Stellar Collapse

**What it does**

Black holes are massive compact bodies that absorb matter and grow. Massive stars can collapse into them.

**Black-hole properties**

- Default spawn mass: `2.00` simulation units, heavier than the default Sun.
- Event horizon:

```text
horizon_radius = clamp(0.009 + 0.006 × cbrt(mass / 2.0), 0.007, 0.035)
```

- Collision radius: `0.55 × horizon_radius`.
- Photon sphere: `1.5 × horizon_radius`.
- ISCO: `3 × horizon_radius`.
- Time dilation at `2 × horizon_radius` is reported in the Equation Lab.

**Stellar collapse**

- A star whose merged mass reaches **2.5** simulation units becomes a black hole.
- The body keeps its name but changes type.
- Its history is replaced with the collapse history.
- A large flash, shockwave, purple ejecta, and black-hole ring are emitted.
- The status strip and event log report the collapse.

**Absorption**

- A black hole absorbs any colliding body.
- Its mass increases.
- Its horizon radius grows.
- It never receives thermal or tidal surface values.

**Where to see it**

- Spawn with `7`, then `A`.
- Try `./space_simulator --collapse-demo`.
- Open the Equation Lab **BLACK HOLE** tab.
- Select a black hole to see its event horizon and accretion-disk status in the HUD.

**Limits**

- No gravitational lensing.
- The photon ring and accretion disk are visual guides, not radiative-transfer simulations.
- No Hawking radiation or charge/spin metrics.

---

## 11. Gravity Grid, Orbit Trails, and Trajectory Prediction

**Gravity grid (`G`)**

- A toggleable spacetime mesh depressed by gravitational potential.
- Purely visual; it does not affect physics.
- Default: off.

**Orbit trails (`T`)**

- Fading history lines behind bodies.
- Stored by the renderer, not by the physics engine.
- Default: on.
- Reset clears the trails.

**Trajectory prediction (`Y`)**

- Forecasts the selected body's future path.
- Uses a cached, softened two-body forecast refreshed periodically.
- Uses the same softened gravity as the simulation.
- Moon predictions are mapped into the scaled visual orbit.
- Default: on.

**Limits**

- Prediction is a two-body approximation, not a full N-body forecast.
- It does not account for future collisions.
- Long forecasts drift from the true path in a chaotic system.

---

## 12. Interactive Dust and Physical Promotion

**What it does**

A field of **1,024** dust particles responds to gravity and planet encounters.

**Behaviour**

- Dust feels gravity from all bodies.
- Dust collisions create impact effects and event-log entries.
- Dust is rendered as small glowing points.
- Press `U` to show or hide dust. While hidden, dust physics updates stop.
- Press `O` to promote the nearest dust particle within `0.20` units of the cursor.

**Promotion**

- The particle becomes a physical **Meteor** named `Dust N`.
- It keeps the dust particle's position and velocity.
- It receives a small mass and a `0.0020` radius.
- It is selected and becomes a normal N-body body.
- The dust particle is deactivated permanently.

**Limits**

- Dust is a hybrid system: 1,024 visual/dynamic particles, while promoted bodies use the shared 256-body pool.
- The 5,000-particle asteroid belt is cosmetic and Keplerian; `O` applies to dust, not to every belt point.
- Dust is not a fluid or collisional gas.

---

## 13. Camera and Navigation

**What it does**

A 3D orbit camera lets you inspect the whole system or a single body.

**Defaults**

| Property | Default | Limits |
|---|---:|---:|
| Azimuth | 45° | Unbounded |
| Elevation | 35° | 5°–85° |
| Distance | 3.05 | 0.05–8.00 |
| Field of view | 45° | Fixed |
| Near plane | 0.01 | Fixed |
| Far plane | 100.0 | Fixed |

**Controls**

- Left-drag rotates at `0.35°` per pixel.
- Right-drag pans in the plane perpendicular to the view; panning cancels follow mode.
- Wheel, `PageUp`, and `PageDown` zoom by `1.12×` per step.
- `Home` resets the camera and follow mode.
- `F` toggles follow-selected.

**Follow mode**

- Tracks the selected body's rendered position.
- Uses smooth exponential blending.
- Enforces a close-focus minimum distance based on the body's rendered radius.
- Uses a larger close-focus distance for black holes.
- Turns off when the selection is cleared or the camera is panned.

**Limits**

- No free-flight camera, roll, or field-of-view control.
- No collision between the camera and bodies beyond the follow-distance clamp.

---

## 14. Picking, Selection, and Labels

**Picking**

- A mouse click is converted into a world-space ray using the camera basis and projection parameters.
- Bodies are tested as spheres.
- The nearest ray-sphere hit wins.
- Picking uses the rendered position, so visual moon scaling determines clickability.

**Selection**

- The selected body gets a white wire sphere.
- Its label is highlighted and drawn first.
- The right panel is populated.
- Hovering draws a subtle ring around the pickable body.
- Clicking empty space clears the selection and follow mode.
- Clicks inside the right HUD panel are ignored.

**Labels**

- Stars, planets, comets, and black holes are labelled by default.
- Moons, asteroids, and meteors appear only when selected.
- Priority:
  1. selected label (priority 2);
  2. non-star bodies (priority 1);
  3. stars (priority 0).
- Overlapping labels are skipped in screen space.
- The selected label wins.
- Very close follow views show only the focused body's label.
- Labels are clipped away from the top and bottom HUD strips.

**Limit**

- Picking is sphere-based; there is no pixel-perfect mesh picking.

---

## 15. HUD and Educational Data

**What it does**

The HUD turns the simulation into an inspectable educational tool.

**Top strip**

- Title and version.
- Body count.
- FPS.
- Speed multiplier.
- Pause state.
- Simulation time.
- Collision mode.
- Spawn type, mass scale, radius scale, and velocity mode.
- Temporary status message.

**Selected-body panel**

- Name and type.
- Mass in Earth masses.
- Volume in Earth volumes.
- Rotation period, with retrograde marked.
- Orbital period.
- Surface temperature and equilibrium temperature.
- Density.
- Tidal stress and Roche limit.
- Parent and real orbital radius for moons.
- Visual orbit size in planet radii for moons.
- Comet activity and tail length.
- Velocity vector, speed, and acceleration.
- Distance from the nearest star (shown in the HUD as `Distance from Sun`).
- Wrapped history, up to 10 lines.

**Special cases**

- Black holes show the event horizon and accretion-disk status instead of the volume, rotation, orbital-period, temperature, density, and tidal-stress block.
- Moons show parent and orbit data.
- Comets show activity and tail length.

**No-selection panel**

Shows a short reminder for gravity grid, trails, prediction, collision mode, and spawning/deleting.

**Event log**

- The last four physics events are drawn above the bottom control legend.
- Events include impacts, merges, fragmentation, collapse, and unstable-body removal.
- Each event carries a simulation timestamp.
- The internal log stores 16 entries.

**Status messages**

- Shown in the top strip for roughly three seconds.
- Cover speed changes, pause/resume, spawn changes, add/delete results, merge results, pool-full errors, and no-selection errors.

**Responsive layout**

- Default window: 1600 × 900.
- Right panel width is capped at 620 px and shrinks to preserve a usable viewport.
- The HUD switches to compact spacing and scaling below 820 px height and narrower text below 760 px simulation width.
- The right panel is excluded from picking and camera drag.

---

## 16. Equation Lab

**What it does**

The Equation Lab shows live physics formulas and values for a selected body **A** and a comparison body **B**.

**Open and navigate**

- `E` opens or closes.
- `Esc` closes.
- `Tab` or `Left` / `Right` changes tab.
- `Up` / `Down` scrolls.
- `Home` resets scroll.
- `N` cycles B.
- `Shift + left-click` sets B.
- `Q` quits.

**A and B**

- A is the selected body.
- B defaults to the nearest other active body.
- If no body is selected, the lab selects the heaviest active body.
- If no active body exists, the lab reports an error and closes.

**Tabs**

| Tab | Contents |
|---|---|
| **PAIR** | Force, acceleration of A and B, distance, potential, escape velocity, relative speed, direction |
| **ORBIT** | Specific energy, angular momentum, eccentricity, semi-major axis, periapsis, apoapsis, period |
| **THERMAL** | Surface and equilibrium temperature, greenhouse, tidal and impact heating, stellar luminosity |
| **TIDAL** | Tidal acceleration, ratio, Roche limit, Hill radius, heating |
| **IMPACT** | Collision mode, effective radii, collision scales, relative speed, impact parameter, specific impact energy, predicted outcome |
| **BLACK HOLE** | Event horizon, photon sphere, ISCO, time dilation |
| **BODY** | Mass, radius, density, surface gravity, escape velocity, axial tilt, flattening, composition, effective collision radius |
| **EVERYTHING** | Net vectors, system energy and momentum, every gravitational contributor sorted by force, orbit, thermal, tidal, rotation, collision, and black-hole metrics |

**Formulas shown**

- Pair gravity: `F = G m1 m2 / (r + eps)^2`.
- Potential: `U = −G m1 m2 / (r + eps)`.
- Escape velocity: `sqrt(2G(m1 + m2)/(r + eps))`.
- Vis-viva: `v² = μ(2/r − 1/a)`.
- Luminosity: `L = 4πR²σT⁴`.
- Equilibrium temperature: `T_eq = [flux(1 − albedo)/(4σ)]^(1/4)`.
- Tidal acceleration: `2GMR/d³`.
- Roche limit: `R_p (2 M_p / M_b)^(1/3)`.
- Hill radius: `a (m / (3M))^(1/3)`.
- Impact parameter: `b = |r × v| / |v|`.
- Specific impact energy: `Q = 0.5 μ v² / (m1 + m2)`.
- Black hole: `r_s = 2GM/c²` in a simulation-equivalent scale.

**Limits**

- Values are live snapshots; they update as the simulation runs.
- The lab is read-only; editing is done in the Body Editor.
- The two-body orbit tab is an osculating approximation, not a full N-body forecast.

---

## 17. Body Editor

**What it does**

The Body Editor edits the selected body or uses it as a template for a custom spawn.

**Open and close**

- `B` opens or closes.
- `Esc` cancels typing first, then closes.
- `Q` quits.
- If no body is selected, the editor falls back to the heaviest active body.

**26 fields**

The editor exposes:

- physical: mass, visual radius, volume, density, collision radius, collision scale, body type, composition;
- kinematic: position X/Y/Z, velocity X/Y/Z, spin rate;
- thermal: albedo, greenhouse, internal heat, thermal inertia, tidal lock;
- appearance: axial tilt, flattening, cloud amount, atmosphere R/G/B.

**Editing**

- `Up` / `Down` selects a field.
- `Left` / `Right` adjusts.
- `PageUp` / `PageDown` adjusts coarsely.
- `Enter` starts typing; `Enter` again applies.
- Digits, `.`, `-`, and `Backspace` are accepted while typing.
- `Esc` cancels typing.
- `S` spawns a custom body from the current values at the cursor.
- `R` resets the selected body's appearance to type defaults.

**Invariants**

- Mass and density are canonical.
- Volume is derived from mass and density.
- Visual radius is a render property; it does not silently change the physical collision radius.
- Collision radius and collision scale are separate from visual radius.
- Black-hole mass edits recompute the event horizon and collision radius.
- Values are clamped to safe ranges after editing.
- Invalid typed values are rejected without changing the body.

**Composition presets**

Rocky, Icy, Metallic, Gas Giant, Stellar, Exotic. Each preset sets density, albedo, surface kind, cloud amount, and atmosphere tint.

**Custom spawn**

- Requires a selected template and a valid cursor.
- Copies the template's current values.
- Places the copy at the cursor with `y = 0`.
- Inherits the template's velocity.
- Names it `Custom <Type> N`.
- Selects the new body.
- Adds the body immediately. Unlike `A`, the custom-spawn command does not push a spawn flash event.

**Limits**

- 26 fields are shown in one scrolling list, not separate pages.
- No undo/redo.
- `R` resets appearance, not the original seeded mass or history.
- The editor edits one body at a time.

---

## 18. Tutorial and Controls with Guided Tour

**What it does**

A dedicated reference page explains the controls and a 12-step guided tour introduces the main workflow.

**Open and close**

- `F1` or `H` opens.
- `Esc`, `F1`, or `H` closes.

**Eight sections**

1. Getting Started.
2. Camera.
3. Time.
4. Spawning.
5. Body Editor.
6. Equation Lab.
7. Tidal / Thermal.
8. Dust / Keys.

**Navigation**

- `Left` / `Right` changes section.
- `Tab` advances to the next section.
- `Up` / `Down` scrolls.
- `Home` resets scroll.
- `T` toggles the guided tour.
- `Enter` / `Space` advances the tour.
- `Backspace` goes back.
- `Q` quits.

**Guided tour**

The tour covers camera rotation, selection, pause/speed, the Equation Lab, B selection, the Body Editor, field adjustment, custom spawning, collision spheres, dust promotion, the Everything tab, and closing the tutorial.

**Limit**

- The tour is instruction-only; it does not move the camera or highlight widgets automatically.

---

## 19. Rendering and Visual Scene

**Renderer**

- Modern **OpenGL 3.3 core** pipeline.
- GLSL shaders.
- VBO sphere, line, and point meshes.
- Additive glow and particles.
- Custom font-atlas HUD.
- Default window 1600 × 900.
- Native 24 px text throughout the HUD, labels, menus, inspector, editor, and tutorial.

**Planets**

- Procedural surfaces.
- Mercury/Mars/Moon rocky surfaces.
- Earth oceans, land, clouds, and ice.
- Venus clouds.
- Jupiter bands and Great Red Spot.
- Saturn bands.
- Uranus/Neptune ice-giant bands.
- Axial tilt, visible spin, gas-giant flattening.
- Atmosphere rim, thermal glow, icy tint.

**Sun**

- Multi-scale animated procedural granulation.
- View-dependent limb darkening.
- Chromosphere rim.
- Sparse sunspots with penumbra/umbra structure and bright faculae.
- Four-layer soft billboard corona.
- Animated prominence arcs.
- Soft solar-wind particles.
- Temperature-based stellar colour and lighting.

**Saturn rings**

- 2,048-sample procedural profile.
- D/C/B/A/F structure.
- Fine ringlets.
- Cassini, Encke, and Keeler gaps.
- Optical-depth alpha and soft edges.
- View-dependent illumination.
- Soft planet shadow.
- Axial-tilt orientation.
- Ring shadow on Saturn.
- Cosmetic, not a collision body.

**Black holes**

- Event horizon.
- Photon ring.
- Animated accretion disk.
- Absorption flare and growth.
- Collapse effects.

**Other scene elements**

- 600-point procedural starfield at radius 18–22 units.
- 5,000-particle Keplerian asteroid belt with eccentricity, inclination, density families, and Kirkwood gaps.
- Bound-body orbit lines from osculating elements.
- Moon orbit conics scaled into the visual moon space.
- Comet coma and two tails.
- Meteor velocity streaks.
- Selection wire sphere.
- Spawn cursor ring.
- Collision debug spheres.
- Smooth lighting, specular materials, emissive stars, dark space background.

**Effects**

- Impact sparks, dust, shockwave rings, flashes, and camera-shake data.
- Collapse flash, shockwave, purple ejecta, and black-hole ring.
- Spawn materialisation flash and expanding ring.

**HUD text**

- Generated DejaVu Sans font atlas at 24 px.
- No GLUT bitmap text.
- Responsive scaling for smaller windows.

---

## 20. Time Controls and Simulation State

**Simulation state**

- Time multiplier: `0.10×–10.00×`.
- Pause flag.
- Accumulated simulation time.
- Collision mode: realistic or classic.
- Physical-collision flag, available from the CLI.

**Speed control**

- `+` / `=` multiplies by 1.5.
- `-` / `_` divides by 1.5.
- `0` resets to 1.00×.
- `Up` / `Down` also change speed in the main view.

**Frame-time coupling**

```text
dt = BASE_DT × time_multiplier × (real_frame_time / 0.016)
```

The real frame interval is clamped to 0–0.05 seconds. Higher speeds demand more sub-steps and can reduce accuracy at the 32-sub-step cap.

**Pause**

- `Space` or `P`.
- Physics and simulation time freeze.
- Camera, effects, and status messages continue.
- Spawning, editing, and toggles still work.

**Reset**

- `R` restores the default universe and most UI state. Spawn type, scales, and velocity mode are retained.

**Unstable-body safety**

- If a position or velocity becomes non-finite, the body is removed.
- The removal is logged as a physics event.

---

## 21. Event Log and Status System

**Event log**

- Stores the most recent **16** physics events.
- Draws the most recent **4** above the bottom control legend.
- Each entry includes a simulation timestamp.
- Events include impacts, merges, fragmentation, collapse, and unstable-body removal.
- Events use colour coding for different categories.
- The log is cleared by reset.

**Status messages**

- Shown in the top strip.
- Last for **180 timer ticks**, roughly three seconds at 60 FPS.
- Cover speed, pause, spawn type/velocity/scales, add/delete, merges, pool-full errors, no-selection errors, editor results, and overlay open/close events.
- Status messages are transient; the event log is the persistent physics history.

---

## 22. Command-Line Interface and Demos

**Options**

| Option | Effect |
|---|---|
| `--selftest` | Run the headless self-test suite and exit |
| `--screenshot [path]` | Render for 1.5 s, save a PPM, and exit |
| `--select NAME` | Select an exact body name at startup |
| `--camera AZ EL DIST` | Set initial camera azimuth, elevation, and distance |
| `--follow` | Follow the selected body at startup |
| `--lab` | Open the Equation Lab at startup |
| `--lab-tab N` | Open the lab on tab N (0–7) |
| `--editor` | Open the Body Editor at startup |
| `--tutorial` | Open Tutorial & Controls at startup |
| `--tour` | Open the tutorial with the guided tour active |
| `--grid` | Enable the gravity grid |
| `--no-trails` | Disable orbit trails |
| `--no-prediction` | Disable trajectory prediction |
| `--classic` | Use classic merge-only collisions |
| `--physical-collisions` | Use physical collision radii |
| `--collision-debug` | Show collision spheres at startup |
| `--impact-demo` | Spawn two planets on a collision course |
| `--collapse-demo` | Spawn a massive collapsing star |
| `--comet-demo` | Spawn an active comet |
| `--help`, `-h` | Print usage and exit |

**Demos**

- `--impact-demo`: two planets at `(0.82, 0, 0.90)` and `(1.06, 0, 0.90)`, moving toward each other at `±1.6` units/time.
- `--collapse-demo`: a star of mass `3.0` at `(0.90, 0, 0.90)` plus an asteroid trigger at the same position. The star crosses the collapse threshold and becomes a black hole.
- `--comet-demo`: a comet at `(0.34, 0, 0)` with velocity `(0, 0, 0.78)`.

**Screenshots**

- Binary PPM (`P6`).
- Default path: `space_simulator.ppm`.
- 1.5-second warm-up.
- Requires a display server; use `xvfb-run` on headless machines.

**Limits**

- Unknown options print a warning and are ignored.
- `--screenshot` treats a following argument beginning with `-` as another option, not a path.

---

## 23. Self-Tests and Diagnostics

**What it does**

`--selftest` runs without a window and checks the core behaviour.

**Coverage**

- `CelestialBody` fits the pool memory budget.
- Ray-sphere hit and miss behaviour.
- 26-body initial layout and expected named bodies.
- Earth's orbit drift stays under 5% over 20,000 quarter-steps.
- All eight planets stay within 5% of their initial orbits.
- The Moon stays bound to Earth in its Hill-safe physical orbit.
- Moon visual position matches the physical orbit and visual scale.
- All moons remain bound to their parents.
- No accidental collisions in the stable layout.
- Planet removal detaches rather than destroys moons.
- Slot allocation, removal, and reuse.
- Out-of-range removal rejection.
- Runtime moon spawning, parent assignment, and orbital velocity.
- Merge conservation of mass, 3D momentum, and volume ratio.
- Pool filling to 256 bodies and rejection of overflow.
- Temperature behaviour: Earth-like Earth, hotter Venus, colder Neptune.
- Tidal heating: Io stronger than the Moon.
- Corrected Jupiter-relative spawning.
- Fragmentation and collapse.
- Comet activity, tail growth, and anti-sunward direction.
- Effect generation.
- Physics-metric calculations.
- Keplerian belt data.
- Body Editor value application and invariants.

**Commands**

```bash
make check
./space_simulator --selftest
```

On Windows/Wine:

```bash
wine ./space_simulator.exe --selftest
```

**Diagnostics**

- The application prints the OpenGL version and MSAA sample count at startup.
- Screenshot mode prints the output path.
- Headless tests report pass/fail for each check.

---

## 24. Platform and Build Support

**Linux**

- Native build with `g++`, freeGLUT, and OpenGL.
- `make`, `make release`, `make check`.

**Windows**

- MinGW build.
- The VS Code task links freeGLUT and OpenGL.
- Optional cross-compilation from Ubuntu with a static MinGW freeGLUT build.
- `make windows`.

**Tooling**

- VS Code build, run, check, and cross-compile tasks.
- Linux and Windows debug configurations.
- `space_simulator.exe` for Windows.

**Rendering context**

- OpenGL 3.3 core context created through freeGLUT.
- GLFW migration is deferred because its development package is not available in the target environment.
- The HUD uses a generated DejaVu Sans font atlas, so no GLUT bitmap text is needed.

---

## 25. Source Architecture

```text
src/
├── main.cpp
├── app/          Application lifecycle, CLI, screenshot
├── core/         Body data, config, math, state, universe, events, seed data
├── physics/      Gravity, collisions, impact model, environment, moon system
├── fx/           Particles, shockwaves, dust field
├── render/       OpenGL renderer, shaders, camera, font atlas, labels, cosmetics
├── ui/           HUD, layout, status messages, body editor
├── input/        Input controller, picking, spawn service
└── selftest/     Headless test suite
```

**Frame loop**

1. Read input and update UI state.
2. Choose adaptive sub-steps.
3. Integrate gravity.
4. Resolve collisions.
5. Update moon visual positions.
6. Update the thermal/tidal/comet environment.
7. Update dust.
8. Update cosmetics and effects.
9. Render the scene and HUD.
10. Swap buffers.

**Data flow**

- `Universe` owns the body pool, moon metadata, and previous positions.
- `PhysicsEngine` computes accelerations and integrates.
- `CollisionSolver` resolves impacts and mutates the pool.
- `Environment` updates temperature, tides, and comet state.
- `MoonSystem` maps physical moon positions into visual positions.
- `SceneRenderer` draws bodies, orbits, labels, effects, and cosmetics.
- `Hud` draws the text UI and inspector.
- `InputController` maps mouse and keyboard events to the other systems.

**Legacy**

- The pre-refactor single-file version is archived in `legacy/v3_monolith/main.cpp` and is not built.

---

## 26. Known Limitations and Deferred Work

**Physics**

- Full tidal disruption streams and Roche-limit fragmentation are not implemented.
- No material-strength, spin-transfer, or full angular-momentum collision model.
- No white dwarf or neutron star remnants.
- No relativistic precession, drag, radiation pressure, or solar-wind forces.
- No collisions with the cosmetic asteroid belt or Saturn rings.
- No body rotation dynamics; rotation periods are HUD and visual metadata.
- No merger mass loss, cratering, or detailed impact ejecta physics.
- No Barnes-Hut tree, octree, or GPU gravity.
- Close encounters below the softening length are approximated.
- The system is chaotic over long time spans.

**Visuals**

- No black-hole gravitational lensing; the photon ring and accretion disk are present.
- Saturn's rings and the asteroid belt are cosmetic.
- No audio.

**Tools**

- No save/load or scenario presets.
- No clickable menu widgets or interactive tutorial highlighting.
- The Body Editor uses a scrolling list rather than separate pages.
- No undo/redo.
- No reserved physical dust slots; promotion uses the shared 256-body pool.

**Platform**

- GLFW migration remains deferred.
- The bundled `Progress_Report_v3.docx` documents an earlier milestone; the v3.5 feature set is documented in the README and `docs/`.

---

## 27. Verification Summary

The project includes a headless self-test suite and build targets for Linux and Windows.

```bash
make check
make release
make windows
./space_simulator --selftest
```

The self-tests cover:

- stable 26-body layout;
- orbital drift bounds;
- moon binding and visual scaling;
- spawn solver and parent assignment;
- merge and fragmentation conservation;
- pool limits;
- temperature and tidal behaviour;
- comet activity and tails;
- collision effects;
- physics metrics;
- Body Editor invariants;
- Keplerian belt data.

**Documentation map**

| Document | Purpose |
|---|---|
| [README.md](README.md) | Project overview and quick start |
| [PROGRESS.md](PROGRESS.md) | Progress, handoff notes, and verification history |
| [clbdhistory.md](clbdhistory.md) | Every body's history and profile |
| [instructions.md](instructions.md) | Button-by-button operation and workflows |
| [functionalities.md](functionalities.md) | This feature catalogue |
| [docs/overview.md](docs/overview.md) | Purpose, audience, scope, limitations |
| [docs/features.md](docs/features.md) | Original feature catalogue |
| [docs/architecture.md](docs/architecture.md) | Modules, renderer, data flow |
| [docs/physics.md](docs/physics.md) | Gravity, collisions, temperature, tides, comets |
| [docs/controls.md](docs/controls.md) | Original control reference |
| [docs/build-and-testing.md](docs/build-and-testing.md) | Build targets and verification |
| [docs/roadmap.md](docs/roadmap.md) | Deferred full-realism work |
