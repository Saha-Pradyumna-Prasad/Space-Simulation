# Roadmap

## Delivered in v3.5

- Modern OpenGL 3.3 core renderer using shaders, VBO meshes, GPU point sprites, blending, and a custom font-atlas HUD.
- Spacetime gravity grid, orbit trails, and selected-body trajectory prediction.
- Impact particle system with sparks, dust, shockwaves, flashes, and camera shake data.
- Collision outcomes: merge, bounce, graze, fragment, and black-hole absorption.
- Debris fragmentation with mass and momentum conservation and pool-limit handling.
- Stellar collapse into a black hole when the mass threshold is crossed.
- Temperature model with equilibrium temperature, greenhouse warming, impact heating, and thermal glow.
- Tidal model with tidal stress, Roche limits, tidal heating, and tidal deformation.
- Comet model with activity, coma, ion tail, curved dust tail, and temperature-driven behavior.
- Corrected spawn primary selection, 3D orbital planes, prograde/retrograde/co-moving modes, and eccentric comet spawning.
- Event log, expanded inspector, and impact/collapse/comet demo flags.
- Procedural planet surfaces, axial tilt, spin, oblateness, clouds, and atmosphere rims.
- Ceres, Vesta, Pallas, and Hygiea as real belt bodies.
- 5,000-particle Keplerian asteroid belt with Kirkwood gaps.
- Saturn procedural ring profile, fine ringlets, Cassini/Encke/Keeler gaps, soft planet shadow, axial-tilt orientation, and two-way shadows.
- Black-hole event horizon, photon ring, accretion disk, and mass growth on absorption.
- Sun multi-scale granulation, limb darkening, sunspots/faculae, soft billboard corona, prominences, and solar wind.
- Live Equation Lab with pair/orbit/thermal/tidal/impact/black-hole/body metrics.
- Body Editor with custom mass, radius, volume, density, collision scale, kinematics, thermal values, type, composition, and custom spawning.
- Visible-surface collision handling with effective radii, type-aware outcomes, collision cooldown, and collision debug spheres.
- 1600×900 native 24 px UI, Tutorial & Controls page, guided tour, and compact HUD legend.
- Gradual thermal inertia and tidal locking.
- Gravity/collision dust with physical promotion and an Everything interactions tab.

## Deferred: full realistic destruction (the “option 3” tier)

These are intentionally not part of the current collision model:

- Tidal disruption streams and Roche-limit fragmentation into a continuous debris stream.
- Material strength, fracture thresholds, and regolith behavior.
- Full angular-momentum transfer, spin evolution, and tidal despinning.
- White-dwarf and neutron-star remnants with distinct physics and visuals.
- Relativistic lensing and physically modelled accretion disks.
- Long-term orbital circularisation and tidal migration.
- Gas dynamics, atmospheres, and ablation during atmospheric entry.

## Deferred: renderer and scale

- HDR framebuffer bloom and screen-space black-hole lensing.
- Instanced body rendering and GPU-computed gravity grid.
- Procedural planet textures and atmospheric scattering.
- Shadow mapping/eclipses with multiple star lights.
- Barnes-Hut or spatial-grid gravity for 1,000+ bodies.
- GLFW/GLAD windowing migration. The current build uses freeGLUT to create an OpenGL 3.3 core context because GLFW development packages are not available in the target environment; the rendering pipeline itself is modern OpenGL.

## Deferred: content and tooling

- Save/load and scenario presets.
- Binary-star, black-hole-flyby, and asteroid-impact scenarios.
- Physical asteroid-belt mode.
- Audio cues for impacts and collapse.
- Frame-sequence recording.
