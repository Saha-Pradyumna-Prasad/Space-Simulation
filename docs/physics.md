# Physics

Space Simulator uses a compact Newtonian model tuned for interactive use. The numbers are not astronomical units; they are simulation units chosen so the full solar system fits in a scene roughly 2.24 units across while remaining stable at interactive speeds.

## Constants and scale

| Constant | Value | Meaning |
|---|---:|---|
| `G_CONSTANT` | 0.15 | Gravitational constant in simulation units |
| `SOFTENING` | 0.01 | General close-encounter softening length |
| `MOON_SOFTENING` | 0.0001 | Softening for a moon and its parent |
| `BASE_DT` | 0.0025 | Base simulation time step per 16 ms frame |
| `SUBSTEPS_MAX` | 32 | Maximum adaptive sub-steps per frame |
| `EARTH_MASS_SIM` | 3.003e-6 | Earth mass in simulation units |
| `EARTH_RADIUS_SIM` | 0.013 | Earth radius used for volume comparisons |
| `EARTH_VISUAL_RADIUS` | 0.017 | Earth's rendered radius |
| `MAX_OBJECTS` | 256 | Body-pool capacity |
| `MIN_SPEED_MULT` | 0.10 | Slowest simulation speed |
| `MAX_SPEED_MULT` | 10.00 | Fastest simulation speed |

The Sun has mass `1.0` and visual radius `0.060`. Planet masses are stored as Earth-mass multiples multiplied by `EARTH_MASS_SIM`. The scene is compressed: Neptune orbits at `1.12` simulation units, not at its real scaled distance.

## Gravity

For every unordered pair of active bodies `i` and `j`:

```text
d  = position_j - position_i
r  = |d|
eps = 0.0001 if i and j are a moon/parent pair, else 0.01
s  = r + eps

factor = G / (s^2 * r)

a_i += factor * mass_j * d
a_j -= factor * mass_i * d
```

This is equivalent to:

```text
a_i = G * mass_j * d / (r * (r + eps)^2)
```

The extra `r` in the denominator comes from applying the softened distance to the force magnitude while using the true direction vector. The softening length prevents singular accelerations during extremely close encounters and makes the seeded circular-orbit speeds consistent with the force law.

The loop uses Newton's third law, so each pair is evaluated once. The cost is O(N²): with the 256-body cap, the worst case is about 32,640 pair evaluations per integration step, multiplied by the active sub-step count.

The acceleration of every active body is reset to zero at the start of each gravity pass. Inactive slots are ignored entirely.

## Integration

The integrator is semi-implicit Euler:

```text
save previous positions
compute accelerations
for each active body:
    velocity += acceleration * dt
    position += velocity * dt
```

Updating velocity before position makes the scheme first-order symplectic and substantially more stable than explicit Euler for orbital motion. It is still a first-order method; long runs accumulate phase and energy error, and close encounters are only approximately resolved.

Previous positions are stored for the swept collision test, not for trajectory trails.

## Adaptive sub-stepping

Before each frame step, the engine estimates how far the fastest body would travel relative to the smallest visual radius:

```text
max_speed  = max |velocity|
min_radius = min visual radius
travel     = max_speed * |dt|
steps      = floor(travel / (0.5 * min_radius)) + 1
steps      = clamp(steps, 1, 32)
sub_dt     = dt / steps
```

Each sub-step runs gravity, integration, and collision resolution. This keeps fast meteors and comets from tunnelling through bodies without paying the full cost when the system is calm.

The simulation speed multiplier scales `dt = BASE_DT * time_multiplier * frame_ratio`, where `frame_ratio` is the measured real frame interval divided by the nominal 16 ms tick. Higher speeds therefore increase the required sub-step count and can reduce accuracy at the cap, but they do not change the base `0.0025` unit scale.

## Temperature model

Every non-star body receives flux from all active stars:

```text
flux = Σ (L_star / L_sun) / (distance / EARTH_ORBIT_SIM)^2
T_eq = 255 K * ((1 - albedo) * flux / (1 - 0.30))^(1/4)
T_surface = T_eq + greenhouse_k + tidal_heating_k + impact_heating_k
```

- `L_star` scales as `mass^3.5` in simulation units.
- Earth is calibrated to about 287 K at its seeded orbit.
- Venus has a large greenhouse term; Neptune receives little flux.
- Impact heating decays exponentially.
- Hot bodies emit a visible thermal glow in the renderer.
- Temperature also drives comet activity.

Thermal inertia makes temperature changes gradual rather than instantaneous. The model is a calibrated educational approximation, not a radiative-transfer climate simulation.

## Tidal model

For each body, the environment finds the dominant tidal primary. The physical tidal ratio uses the compressed collision radius:

```text
tidal_ratio = 2 * M_primary * R_body^3 / (M_body * d^3)
Roche_limit ≈ R_primary * cbrt(2 * density_primary / density_body)
```

- The tidal axis points toward the primary.
- The renderer stretches the body sphere along that axis by a clamped visual amount.
- Tidal heating is a bounded, stylised temperature contribution designed to distinguish Io-like moons from cold moons.
- The rendered tidal bulge follows the net tidal vector from all bodies.
- Tidal locking drives moon spin rates toward their orbital periods.
- Tidal orbital evolution, despinning, and disruption streams are deferred to the full-realism roadmap.

## Comet model

Comet activity is derived from surface temperature:

```text
activity = clamp((T_surface - 160 K) / 260 K, 0, 1)
coma_radius = nucleus_radius * (1.5 + 3.5 * activity)
tail_length = 0.06 + 0.55 * activity
```

- The ion tail points along the net anti-stellar radiation vector.
- The dust tail is curved and lags behind the orbital motion.
- The coma is an additive glow around the nucleus.
- Comet spawns use an eccentric orbit with a perihelion near the star.

## Collision outcomes

The impact model computes relative speed, impact parameter, and a simplified specific-energy ratio. The outcome depends on energy and mass ratio:

| Outcome | Meaning |
|---|---|
| Merge | Low-energy accretion |
| Bounce | Inelastic rebound with restitution |
| Graze | Off-centre momentum exchange |
| Fragment | Debris creation from the smaller body |
| Disrupt | Catastrophic debris creation |
| Absorb | Black-hole collision |

Fragment bodies conserve total mass and momentum, use asteroid visuals, and are limited by the 256-slot pool. The classic always-merge behaviour remains available.

| Pair | Rule |
|---|---|
| Star + anything | Star merges/absorbs; stars never fragment |
| Star + black hole | Black hole absorbs the star |
| Black hole + anything | Black hole absorbs and grows |
| Planet + planet | Merge / bounce / graze / fragment / disrupt |
| Moon + moon | Same realistic classifier |
| Asteroid/comet + planet | Merge, graze, fragment, or crater |

A bounce or graze starts a short collision cooldown to prevent repeated contacts on consecutive frames.

When a star's merged mass exceeds `STAR_COLLAPSE_MASS`, it becomes a black hole and emits a collapse event. Black holes absorb any colliding body, increase their mass, and grow their horizon radius using a stylised mass-horizon relation. They never receive thermal or tidal surface values. The full realistic destruction tier (tidal streams, material strength, spin transfer, neutron stars) is documented in [roadmap.md](roadmap.md).

The visual asteroid belt uses 5,000 Keplerian particles with eccentricity, inclination, nodes, perihelia, mean anomalies, and Kirkwood gaps. Ceres, Vesta, Pallas, and Hygiea are separate physical bodies. Saturn's ring system is a set of D/C/B/A/F bands with axial-tilt orientation; Saturn shadows the rings analytically, and the ring opacity texture is sampled in the planet shader to shadow Saturn's surface.

## Spawn/orbit correction

The spawn solver now:

- Chooses a local planet/black hole when the cursor is inside its visual influence radius, otherwise the dominant star.
- Builds the orbit in the primary's angular-momentum plane.
- Uses the softened circular speed for prograde/retrograde modes.
- Makes zero-velocity mode co-moving with the primary.
- Uses an eccentric vis-viva orbit for comets.
- Adds a spawn flash and shockwave.

## Collision detection

### Physical versus visual radii

Every body has:

- `radius` — the rendered sphere size.
- `collision_radius` — the physical collision size.
- `collision_scale` — how much of the visual radius participates in visible collisions.

Default visible-surface collision:

```text
effective_radius = max(collision_radius, radius * collision_scale)
collision_scale  = 0.90
```

This is used for overlap, swept tests, impact parameter, escape velocity, and impact classification. It makes two mutually visible planets collide instead of passing through each other.

Small bodies and parented moons use their physical radii for collision tests because their rendered positions are visually exaggerated. Retrograde co-orbital planets use the visual-surface radius and collide correctly.

Physical-only mode can be selected with `--physical-collisions`.

### Overlap test

For each active pair:

```text
radii = effective_radius_i + effective_radius_j
hit   = |position_j - position_i|^2 < radii^2
```

If both effective radii are non-positive, the visual radii are used instead.

### Swept test

If the current positions do not overlap, the solver tests the relative motion during the last integration step:

```text
p0 = previous_position_j - previous_position_i
d  = current_position_j  - current_position_i
s  = d - p0

t = clamp(-dot(p0, s) / dot(s, s), 0, 1)
closest = p0 + t * s
hit = |closest|^2 < radii^2
```

This is a linear-motion approximation for the pair over one sub-step. It catches fast bodies that would otherwise pass through a target between frames.

## Merging

When a collision is detected, the heavier body is kept. Let `keep` have mass `m1`, velocity `v1`, and position `p1`, and let `gone` have `m2`, `v2`, `p2`.

```text
total = m1 + m2

velocity = (m1 * v1 + m2 * v2) / total          # momentum conservation
position = (m1 * p1 + m2 * p2) / total          # centre of mass
mass     = total
radius   = keep.radius * cbrt(total / m1)       # constant-density sphere
volume   = keep.volume + gone.volume            # volume ratios add
```

The `gone` slot is cleared, its moons are detached, and the selection moves to the kept body if the absorbed body was selected. A status message reports the merge.

The cube-root radius update assumes constant density. Because both seeded and spawned bodies use a single radius per body, volume ratios add consistently with the radius change.

## Moon system

### Physical orbit placement

Moons are true N-body bodies. Their physical orbit radius is derived from the parent planet's Hill sphere:

```text
r_Hill = parent_orbit * cbrt(parent_mass / (3 * sun_mass))
```

`parent_orbit` is the distance from the parent planet to the Sun. The physical moon orbit is a fraction of that Hill radius:

```text
fraction = 0.15 + 0.25 * (visual_radius_scale - 1.5) / 1.3
fraction = clamp(fraction, 0.20, 0.40)

physical_orbit = r_Hill * fraction
```

This places every seeded moon between 20% and 40% of its parent's Hill radius, comfortably inside the region where the planet's gravity dominates. The values differ per moon because their visual radius scales differ.

The initial tangential speed is the softened circular speed:

```text
v = sqrt(G * parent_mass * physical_orbit) / (physical_orbit + MOON_SOFTENING)
```

The moon inherits its parent's velocity and adds the relative orbital velocity. Inclination rotates the relative position and velocity about the X axis; Triton is seeded with a negative direction to represent its retrograde orbit.

### Visual scaling

Real moon orbits are tiny compared with planetary orbits at this compressed scale. Rendering them at their physical size can make them nearly invisible or crowd the planet, so the renderer uses a separate visual transform:

```text
visual_orbit = parent.radius * visual_radius_scale
visual_scale = visual_orbit / physical_orbit

render_position = parent.position + (moon.position - parent.position) * visual_scale
```

`visual_scale` varies by moon. It is about 31 for Earth's Moon, about 6–8 for the large moons of Jupiter, Saturn, Uranus, and Neptune, and below 1 for Phobos and Deimos, whose Hill spheres are wide relative to their rendered planet radii. The physics still uses the physical orbit, so each moon's gravity and collisions remain Hill-safe. The HUD reports both the real orbital radius and the scaled visual orbit in planet radii.

Every frame, `MoonSystem::updateVisuals()` refreshes:

- `visual_radius = current_physical_separation * visual_scale`
- `visual_x/y/z = parent_position + relative_position * visual_scale`

This keeps the drawn orbit centred on the moving planet.

### Runtime-spawned moons

A moon spawned near a planet is attached to the nearest planet within `0.12` scene units. Its physical fraction is derived from the desired visual radius relative to the planet's radius:

```text
visual_factor = visual_distance / parent.radius
fraction = clamp(0.15 + 0.25 * (visual_factor - 1.5) / 1.3, 0.20, 0.40)
physical_orbit = r_Hill * fraction
visual_scale = visual_distance / physical_orbit
```

The resulting body is a normal N-body moon with an initialized `MoonOrbit`. A spawned moon far from any planet falls back to the non-moon spawn path.

### Detachment

When a parent body is removed or absorbed, `Universe::removeBody()` clears the parent reference on all of its moons. The moons stay active as independent bodies at their current physical positions. Their rendered positions switch to the physical positions, so they may appear to move inward after detachment.

## Stability and verification

The default scene seeds each planet with a softened circular speed against the Sun, then sets the Sun's velocity so that total momentum is zero. The full N-body system still perturbs those initial orbits.

The self-test runs 20,000 steps at `BASE_DT / 4` and checks:

- Earth's orbital radius drifts by less than 5%.
- All eight planets stay within 5% of their initial orbital radii.
- The Moon remains attached to Earth and stays within 20% of its seeded physical radius.
- All moons remain bound to their parents within 20%–200% of their seeded physical radius.
- No accidental collisions occur in the stable layout.
- Earth's temperature is Earth-like, Venus is hotter, Neptune is colder.
- Io experiences stronger tidal stress and heating than the Moon.
- Zero-velocity spawns near Jupiter are co-moving; prograde spawns have local orbital velocity.
- High-energy planet impacts produce debris; massive stars collapse to black holes.
- Comet activity, tail growth, and anti-sunward direction behave as expected.
- Impact events produce particles and shockwaves.

On the current build the Earth drift is about 1.3%, and Saturn is the largest at about 5.0%. That margin is intentionally close because the test is a regression guard against integration or softening mistakes, not a precision-orbit benchmark.

## Model limitations

- No relativistic precession, tidal evolution, drag, radiation pressure, or solar wind.
- No collisions with the cosmetic asteroid belt or Saturn ring.
- No body rotation dynamics; rotation periods are HUD metadata.
- No merger fragmentation, cratering, or mass loss.
- No adaptive mesh, Barnes-Hut tree, or GPU acceleration.
- Close encounters below the softening length are approximated rather than resolved.
- The system is chaotic over long time spans; small changes in spawn mass or velocity can produce very different outcomes.
