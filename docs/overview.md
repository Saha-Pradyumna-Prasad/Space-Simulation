# Overview

## What Space Simulator is

Space Simulator is an interactive, real-time 3D gravity sandbox. It starts with a compressed model of the Sun, its eight planets, and thirteen major moons, then advances every body using Newtonian gravity. Orbits are not keyframed or scripted; they are the result of the simulated forces.

The application is written in C++17 and uses freeGLUT for the window, input, and timer loop. Rendering uses a modern OpenGL 3.3 core pipeline with GLSL shaders, VBO meshes, GPU point sprites, additive glow, and a custom font-atlas HUD. It runs on Linux natively and can be built for Windows with MinGW.

## Why it exists

The project was created as a university graphics project and an educational tool. Its purpose is to make orbital mechanics observable and manipulable:

- Show that stable orbits emerge from mutual gravitational attraction.
- Let a user change the system at runtime and watch the consequences.
- Connect live simulation data with real astronomical facts through an in-scene HUD.
- Demonstrate 3D graphics techniques such as camera control, lighting, picking, text overlays, transparency, and particle effects in a single coherent application.

Space Simulator is a teaching and experimentation tool, not a precision ephemeris or a replacement for professional astrophysics software.

## Who it is for

- **Students** studying gravity, orbital mechanics, or real-time graphics.
- **Instructors** who want a visual, interactive demonstration of N-body behaviour.
- **Graphics learners** interested in an OpenGL/freeGLUT application with a modular C++ structure.
- **Anyone curious** about what happens when planets, moons, or black holes are added to a live gravity system.

## What a user can do

- Observe the default solar system in 3D.
- Rotate, pan, and zoom an orbit camera around the scene.
- Click any body to inspect its mass, volume, rotation, orbital period, temperature, density, tidal stress, Roche limit, velocity, acceleration, distance from the Sun, and a detailed body history.
- Follow a selected body with the camera.
- Pause the simulation and change its speed between 0.10× and 10.00×.
- Spawn stars, planets, moons, asteroids, meteors, comets, or black holes.
- Control spawn mass, radius, and velocity mode (orbital, retrograde, or co-moving zero).
- Delete bodies and watch moons detach when their parent disappears.
- Watch collisions merge, bounce, graze, fragment, or collapse into a black hole.
- Toggle a gravity grid, orbit trails, trajectory prediction, thermal colours, and tidal deformation.
- Open the Equation Lab to inspect live gravitational, orbital, thermal, tidal, impact, and black-hole formulas.
- Open the Body Editor to change mass, radius, density, position, velocity, temperature, composition, and appearance, or spawn a custom body.
- Inspect the Sun’s granulation, sunspots, corona, prominences, and solar wind close up.
- Reset the universe and camera at any time.
- Capture a screenshot non-interactively with a command-line flag.
- Run impact, collapse, and comet demonstrations from the command line.

## Educational model

The HUD is part of the teaching design. Selecting a body exposes both its live simulation values and real-world facts:

- Simulation values: 3D velocity, speed, acceleration, current distance from the Sun.
- Physical comparisons: mass in Earth masses, volume in Earth volumes, and density.
- Thermal values: surface temperature, equilibrium temperature, and tidal heating.
- Tidal values: tidal stress and the Roche limit.
- Real astronomical values: rotation period, orbital period, and for moons the real orbital radius.
- History: a multi-sentence description of the body, including real-world facts for seeded planets, moons, asteroids, and the Sun, plus type-specific context for runtime bodies.
- Moon-specific values: parent planet, real orbit radius, and the scaled visual orbit in planet radii.
- Comet-specific values: activity percentage and tail length.

The default scene is deliberately compressed so that the whole planetary system fits on screen. Real values are retained in the data model and shown in the HUD. Moon orbits are physically plausible but rendered at larger visual distances so they remain visible beside their planets.

## Design principles

1. **Physics first.** Gravity and collisions drive the scene. Visual effects never move simulated bodies.
2. **Runtime mutation.** The universe is not a fixed demo; bodies can be added and removed while the simulation runs.
3. **Separate physical and visual scales.** Moon orbits use a Hill-safe physical radius and a separately scaled visual radius.
4. **Fixed memory.** A 256-slot object pool provides constant-time access and predictable memory use.
5. **Layered rendering.** Physics, scene rendering, and the 2D HUD are independent layers.
6. **Graceful failure.** Non-finite bodies are removed, full pools are rejected, and invalid selections show status messages instead of crashing.

## Scope of v3.5

v3.5 is the interactive core simulator. It includes:

- The 26-body solar-system seed, including Ceres, Vesta, Pallas, and Hygiea.
- Real N-body gravity and physical collisions with realistic outcomes, fragmentation, and collapse.
- The 3D camera, picking, HUD, labels, event log, and spawn/delete controls.
- Thermal and tidal models with visible effects.
- Corrected comet and spawn models.
- Gravity grid, orbit trails, trajectory prediction, and particle effects.
- Procedural planet surfaces, axial tilt, spin, oblateness, and atmospheres.
- A 5,000-particle Keplerian asteroid belt with Kirkwood gaps.
- Rebuilt Saturn rings with bands, gaps, and two-way shadows.
- Cosmetic starfield, sun glow, comet tails, and meteor streaks.
- Modern OpenGL 3.3 core rendering, self-tests, screenshot mode, and Linux/Windows build support.

It intentionally excludes the 14 event scenarios, event log, scenario menu, event particles, live capture/escape detection, Io glow, and red-giant Sun from the later v4 branch.

## Limitations

- **Compressed scale.** Distances, masses, and visual radii are tuned for the screen, not for astronomical accuracy.
- **Two-body-derived seeds.** Initial orbits are circularised against the dominant body, then perturbed by the full N-body system. They drift slowly over long runs.
- **Scaled visual moon orbits.** Moon positions are scaled for rendering; the physics uses Hill-safe physical orbits.
- **Cosmetic objects remain non-physical.** The belt particles, starfield, Saturn ring, comet tails, sun glow, and meteor streaks do not participate in gravity or collisions. The four major belt asteroids are real bodies.
- **First-order integration.** The fixed-step semi-implicit Euler integrator is simple and stable for this scale, but it is not a high-precision symplectic or high-order scheme.
- **Stylised temperature and tides.** Temperatures are calibrated to Earth and use greenhouse offsets, not a climate model. Tidal heating is a visual/thermal approximation and does not evolve orbits.
- **Collision outcomes are simplified.** High-energy impacts fragment into a bounded number of debris bodies. Full tidal disruption, material strength, and continuous debris streams are deferred.
- **No persistence.** There is no save/load system; `R` restores the built-in scene.
- **No Barnes-Hut or GPU gravity.** The gravity loop is O(N²); the 256-body ceiling keeps this practical.
- **No relativistic, drag, or radiation-pressure physics.**
