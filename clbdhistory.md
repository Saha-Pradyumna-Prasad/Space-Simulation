# Celestial Body Histories

This file collects the history and description of every celestial body in **Space Simulator v3.5**.

Each body has its own title. Under that title you will find:

- **In-game history** — the exact text the simulator stores for the body. The selected-body HUD wraps it and shows up to 10 lines.
- **Profile** — real-world and simulation data taken from the project's seed tables.
- **Role in the simulator** — what the body does in the current scene.

The default scene contains **26 active bodies**: 1 star, 8 planets, 13 moons, and 4 major asteroids. The simulator can also create seven runtime body types and several generated classes of body; those are covered at the end.

> The real-world numbers are educational reference values. The simulation itself uses a compressed scale so that the whole solar system fits inside a scene roughly 2.24 simulation units across. Distances and radii in the simulation are not the real distances and radii.

---

## Contents

- [The Sun](#the-sun)
- [Planets](#planets)
  - [Mercury](#mercury) · [Venus](#venus) · [Earth](#earth) · [Mars](#mars) · [Jupiter](#jupiter) · [Saturn](#saturn) · [Uranus](#uranus) · [Neptune](#neptune)
- [Moons](#moons)
  - [Moon (Luna)](#moon-luna) · [Phobos](#phobos) · [Deimos](#deimos) · [Io](#io) · [Europa](#europa) · [Ganymede](#ganymede) · [Callisto](#callisto) · [Enceladus](#enceladus) · [Rhea](#rhea) · [Titan](#titan) · [Titania](#titania) · [Oberon](#oberon) · [Triton](#triton)
- [Major Asteroids](#major-asteroids)
  - [Ceres](#ceres) · [Vesta](#vesta) · [Pallas](#pallas) · [Hygiea](#hygiea)
- [Spawnable Body Types](#spawnable-body-types)
  - [Star](#star-spawn-type) · [Planet](#planet-spawn-type) · [Moon](#moon-spawn-type) · [Asteroid](#asteroid-spawn-type) · [Meteor](#meteor-spawn-type) · [Comet](#comet-spawn-type) · [Black Hole](#black-hole-spawn-type)
- [Generated Bodies and Histories](#generated-bodies-and-histories)
- [Source Notes](#source-notes)

---

## The Sun

### Sun

**In-game history**

> The Sun contains 99.86 percent of the mass of the solar system. Its gravity holds every planet on its orbit. The core fuses about 600 million tonnes of hydrogen into helium every second, and light takes about eight minutes to reach Earth. The visible surface is roughly 5778 K, while the corona is far hotter; the Sun rotates once every 609 hours at this simulation's scale.

**Profile**

| Property | Value |
|---|---|
| Type | Star |
| Simulation mass | 1.00 |
| Volume | 1,300,000 Earth volumes |
| Visual radius | 0.060 simulation units |
| Real radius | About 696,340 km |
| Rotation period | 609.12 hours (about 25.4 days) |
| Density | 1,408 kg/m³ |
| Surface temperature | 5,778 K |
| Axial tilt | 7.25° |
| Surface | Procedural granulation, sunspots, faculae, corona, prominences, solar wind |
| Composition | Stellar |

**Role in the simulator**

The Sun is the default scene's central star and its dominant gravitational anchor. It is the light source for the system. It remains emissive and never receives planet-style diffuse shading. If a star crosses the collapse mass threshold of **2.5 simulation mass units**, it becomes a black hole.

**At a glance:** the Sun owns most of the system's mass, so every planet, moon, asteroid, and spawned body is in free fall around it.

---

## Planets

The eight planets are seeded with circular orbital speeds around the Sun. Their orbits are compressed: Mercury sits at `0.18` simulation units and Neptune at `1.12`, rather than at their real proportional distances.

### Mercury

**In-game history**

> Mercury is the smallest planet and the closest to the Sun. Its airless surface swings from about 430 C in daylight to -180 C at night, and its slow 1407.6-hour rotation is longer than its 88-day orbit. It is a heavily cratered world with a large iron core and a 3:2 spin-orbit resonance.

**Profile**

| Property | Value |
|---|---|
| Real radius | 2,439.7 km |
| Real orbital radius | 57.9 million km |
| Orbital period | 88.0 Earth days |
| Mass | 0.055 Earth masses |
| Volume | 0.056 Earth volumes |
| Rotation period | 1,407.60 hours |
| Density | 5,427 kg/m³ |
| Albedo | 0.088 |
| Axial tilt | 0.03° |
| Greenhouse offset | 0 K |
| Simulation orbit radius | 0.18 simulation units |
| Visual radius | 0.010 simulation units |

**Role in the simulator:** the innermost planet and the smallest seeded planet. Its proximity to the Sun and low albedo make it a hot, dark, rocky world.

### Venus

**In-game history**

> Venus is Earth's near twin in size, but a runaway greenhouse effect keeps its surface near 465 C under a crushing carbon-dioxide atmosphere. It rotates backwards once every 5832.5 hours, so a Venusian day is longer than its 224.7-day year. Its sulfuric-acid clouds hide a volcanic landscape.

**Profile**

| Property | Value |
|---|---|
| Real radius | 6,051.8 km |
| Real orbital radius | 108.2 million km |
| Orbital period | 224.7 Earth days |
| Mass | 0.815 Earth masses |
| Volume | 0.857 Earth volumes |
| Rotation period | −5,832.50 hours (retrograde) |
| Density | 5,243 kg/m³ |
| Albedo | 0.76 |
| Axial tilt | 177.4° |
| Greenhouse offset | 500 K |
| Simulation orbit radius | 0.28 simulation units |
| Visual radius | 0.014 simulation units |

**Role in the simulator:** the strongest greenhouse example in the default scene. Its high albedo and 500 K greenhouse offset make it far hotter than a simple equilibrium-temperature calculation would suggest.

### Earth

**In-game history**

> Earth is the third planet from the Sun and the only world known to support life. About 71 percent of its surface is covered by water, and its 23.4-degree axial tilt gives the seasons. Its 23.9-hour rotation and 365.2-day orbit define the units we use for time; the Moon stabilises its tilt.

**Profile**

| Property | Value |
|---|---|
| Real radius | 6,371.0 km |
| Real orbital radius | 149.6 million km |
| Orbital period | 365.2 Earth days |
| Mass | 1.000 Earth mass |
| Volume | 1.000 Earth volume |
| Rotation period | 23.93 hours |
| Density | 5,514 kg/m³ |
| Albedo | 0.306 |
| Axial tilt | 23.44° |
| Greenhouse offset | 33 K |
| Simulation orbit radius | 0.38 simulation units |
| Visual radius | 0.017 simulation units |

**Role in the simulator:** the reference world for mass, volume, density, and temperature. Earth is calibrated to a roughly Earth-like surface temperature, and it is the parent of the Moon. Its procedural surface includes oceans, land, clouds, and ice.

### Mars

**In-game history**

> Mars is a cold desert world with a thin carbon-dioxide atmosphere. Iron oxide dust gives it its red colour, and it hosts Olympus Mons, the tallest volcano in the solar system, and the Valles Marineris canyon system. Its 24.6-hour day is close to Earth's, while its orbit takes 687 Earth days.

**Profile**

| Property | Value |
|---|---|
| Real radius | 3,389.5 km |
| Real orbital radius | 227.9 million km |
| Orbital period | 687.0 Earth days |
| Mass | 0.107 Earth masses |
| Volume | 0.151 Earth volumes |
| Rotation period | 24.62 hours |
| Density | 3,933 kg/m³ |
| Albedo | 0.250 |
| Axial tilt | 25.19° |
| Greenhouse offset | 5 K |
| Simulation orbit radius | 0.48 simulation units |
| Visual radius | 0.013 simulation units |

**Role in the simulator:** a cold rocky world and the parent of Phobos and Deimos. Its thin atmosphere is represented by a small greenhouse offset.

### Jupiter

**In-game history**

> Jupiter is the largest planet, more massive than all the other planets combined. Its rapid 9.9-hour rotation drives powerful storms, including the Great Red Spot, and its strong magnetic field traps intense radiation belts. It has dozens of moons, including the four large Galilean worlds.

**Profile**

| Property | Value |
|---|---|
| Real radius | 69,911.0 km |
| Real orbital radius | 778.5 million km |
| Orbital period | 4,331.0 Earth days |
| Mass | 317.80 Earth masses |
| Volume | 1,321.30 Earth volumes |
| Rotation period | 9.93 hours |
| Density | 1,326 kg/m³ |
| Albedo | 0.343 |
| Axial tilt | 3.13° |
| Greenhouse offset | 0 K |
| Simulation orbit radius | 0.64 simulation units |
| Visual radius | 0.036 simulation units |

**Role in the simulator:** the most massive planet and the parent of Io, Europa, Ganymede, and Callisto. Its procedural bands and Great Red Spot are visible up close. Its strong gravity makes it a useful target for moon spawning and impact experiments.

### Saturn

**In-game history**

> Saturn is famous for its bright ring system, made of ice and rock particles. It is the least dense planet, with a 10.7-hour rotation and a 10747-day orbit. Its atmosphere hosts a hexagonal polar jet stream, and its moons Titan and Enceladus are among the most scientifically interesting worlds in the solar system.

**Profile**

| Property | Value |
|---|---|
| Real radius | 58,232.0 km |
| Real orbital radius | 1,432.0 million km |
| Orbital period | 10,747.0 Earth days |
| Mass | 95.20 Earth masses |
| Volume | 763.60 Earth volumes |
| Rotation period | 10.66 hours |
| Density | 687 kg/m³ |
| Albedo | 0.342 |
| Axial tilt | 26.73° |
| Greenhouse offset | 0 K |
| Simulation orbit radius | 0.80 simulation units |
| Visual radius | 0.031 simulation units |

**Role in the simulator:** the ringed planet and the parent of Enceladus, Rhea, and Titan. The renderer draws a high-resolution procedural ring system with D/C/B/A/F structure, fine ringlets, Cassini/Encke/Keeler gaps, axial-tilt orientation, and two-way shadows. The rings are cosmetic and are not collision bodies.

### Uranus

**In-game history**

> Uranus is an ice giant with a pale blue-green colour caused by methane. It rotates on its side once every 17.2 hours and takes 30589 Earth days to orbit the Sun, likely after a giant impact. It has faint dark rings and was the first planet discovered with a telescope.

**Profile**

| Property | Value |
|---|---|
| Real radius | 25,362.0 km |
| Real orbital radius | 2,867.0 million km |
| Orbital period | 30,589.0 Earth days |
| Mass | 14.50 Earth masses |
| Volume | 63.10 Earth volumes |
| Rotation period | −17.24 hours (retrograde) |
| Density | 1,271 kg/m³ |
| Albedo | 0.300 |
| Axial tilt | 97.77° |
| Greenhouse offset | 0 K |
| Simulation orbit radius | 0.96 simulation units |
| Visual radius | 0.023 simulation units |

**Role in the simulator:** an ice giant with an extreme axial tilt and the parent of Titania and Oberon. Its pale blue-green surface is rendered procedurally.

### Neptune

**In-game history**

> Neptune is the most distant planet and the windiest, with storms exceeding 2000 km/h. It completes one orbit every 59800 Earth days. It was predicted mathematically before it was observed, and its large moon Triton orbits backwards, suggesting it was captured from the Kuiper Belt.

**Profile**

| Property | Value |
|---|---|
| Real radius | 24,622.0 km |
| Real orbital radius | 4,515.0 million km |
| Orbital period | 59,800.0 Earth days |
| Mass | 17.10 Earth masses |
| Volume | 57.70 Earth volumes |
| Rotation period | 16.11 hours |
| Density | 1,638 kg/m³ |
| Albedo | 0.290 |
| Axial tilt | 28.32° |
| Greenhouse offset | 0 K |
| Simulation orbit radius | 1.12 simulation units |
| Visual radius | 0.022 simulation units |

**Role in the simulator:** the outermost planet and the parent of Triton, whose retrograde orbit is seeded with a negative direction. Neptune receives very little stellar flux, so it is one of the coldest default worlds.

---

## Moons

All 13 moons are true N-body bodies. Their physical orbits are placed inside a fraction of their parent planet's Hill sphere, while their rendered orbits are scaled outward for visibility.

### Moon (Luna)

**In-game history**

> The Moon is Earth's only natural satellite and is tidally locked, so the same face always points toward Earth. Its real orbital radius is about 384400 km and its period is 27.32 days. It stabilises Earth's axial tilt, drives the ocean tides, and was visited by Apollo astronauts.

**Profile**

| Property | Value |
|---|---|
| Parent | Earth |
| Real radius | 1,737.4 km |
| Real orbital radius | 384,400 km |
| Orbital period | 27.3217 days |
| Mass | 0.01230 Earth masses |
| Volume | 0.0203 Earth volumes |
| Rotation period | 655.7 hours |
| Density | 3,344 kg/m³ |
| Albedo | 0.12 |
| Inclination | 5.145° |
| Direction | Prograde |
| Visual orbit scale | 2.80 planet radii |

**Role in the simulator:** Earth's only moon and a tidally locked satellite. Its physical orbit is kept inside Earth's Hill sphere, and its rendered orbit is visually enlarged so the Moon remains easy to see and click.

### Phobos

**In-game history**

> Phobos is the larger and closer of Mars's two moons. It orbits at about 9376 km from Mars's centre and is slowly spiralling inward; in tens of millions of years it may break up or impact the planet. Its real period is 0.319 days.

**Profile**

| Property | Value |
|---|---|
| Parent | Mars |
| Real radius | 11.27 km |
| Real orbital radius | 9,376 km |
| Orbital period | 0.31891 days |
| Mass | 1.79 × 10⁻⁹ Earth masses |
| Volume | 5.6 × 10⁻⁹ Earth volumes |
| Rotation period | 7.65 hours |
| Density | 1,876 kg/m³ |
| Albedo | 0.07 |
| Inclination | 1.08° |
| Direction | Prograde |
| Visual orbit scale | 1.70 planet radii |

**Role in the simulator:** the inner moon of Mars. Its very small real mass makes it a useful example of a low-gravity, collision-sensitive satellite.

### Deimos

**In-game history**

> Deimos is the smaller, outer moon of Mars. Its surface is covered with dusty regolith over a dark, cratered body. It takes 1.263 days to complete one orbit around Mars and may be a captured asteroid.

**Profile**

| Property | Value |
|---|---|
| Parent | Mars |
| Real radius | 6.20 km |
| Real orbital radius | 23,463 km |
| Orbital period | 1.26244 days |
| Mass | 2.47 × 10⁻¹⁰ Earth masses |
| Volume | 9.4 × 10⁻¹⁰ Earth volumes |
| Rotation period | 30.3 hours |
| Density | 1,750 kg/m³ |
| Albedo | 0.07 |
| Inclination | 1.79° |
| Direction | Prograde |
| Visual orbit scale | 2.30 planet radii |

**Role in the simulator:** the outer moon of Mars. It is one of the smallest seeded moons and is rendered at a minimum visual radius so it remains visible.

### Io

**In-game history**

> Io is the most volcanically active body in the solar system. Tidal heating from Jupiter drives hundreds of volcanoes and sulfur-rich plumes that repaint its surface. Its real orbital period is 1.77 days.

**Profile**

| Property | Value |
|---|---|
| Parent | Jupiter |
| Real radius | 1,821.6 km |
| Real orbital radius | 421,700 km |
| Orbital period | 1.769 days |
| Mass | 0.0150 Earth masses |
| Volume | 0.0234 Earth volumes |
| Rotation period | 42.5 hours |
| Density | 3,528 kg/m³ |
| Albedo | 0.63 |
| Inclination | 0.036° |
| Direction | Prograde |
| Visual orbit scale | 1.50 planet radii |

**Role in the simulator:** a strong tidal-heating example. Its close orbit around Jupiter produces a visible tidal response and a higher tidal temperature contribution than the Moon.

### Europa

**In-game history**

> Europa has a smooth ice shell covering a global salt-water ocean. Tidal flexing may keep that ocean liquid, and it is one of the best candidates for extraterrestrial life. Its real orbital period is 3.55 days.

**Profile**

| Property | Value |
|---|---|
| Parent | Jupiter |
| Real radius | 1,560.8 km |
| Real orbital radius | 671,100 km |
| Orbital period | 3.551 days |
| Mass | 0.0080 Earth masses |
| Volume | 0.0147 Earth volumes |
| Rotation period | 85.2 hours |
| Density | 3,013 kg/m³ |
| Albedo | 0.67 |
| Inclination | 0.466° |
| Direction | Prograde |
| Visual orbit scale | 1.90 planet radii |

**Role in the simulator:** an icy Galilean moon with a bright surface. Its high albedo makes it one of the most reflective moons in the scene.

### Ganymede

**In-game history**

> Ganymede is the largest moon in the solar system, larger than Mercury. It has its own magnetic field, a differentiated interior, and a real orbital period of 7.15 days. Its surface mixes ancient dark terrain with younger grooved ice.

**Profile**

| Property | Value |
|---|---|
| Parent | Jupiter |
| Real radius | 2,634.1 km |
| Real orbital radius | 1,070,400 km |
| Orbital period | 7.155 days |
| Mass | 0.0248 Earth masses |
| Volume | 0.0704 Earth volumes |
| Rotation period | 171.7 hours |
| Density | 1,936 kg/m³ |
| Albedo | 0.43 |
| Inclination | 0.177° |
| Direction | Prograde |
| Visual orbit scale | 2.30 planet radii |

**Role in the simulator:** the largest moon in the scene and the largest moon in the solar system. Its size makes it a useful comparison with Mercury.

### Callisto

**In-game history**

> Callisto is the most heavily cratered object known. Its ancient surface has changed little in billions of years, and it may contain a deep subsurface ocean. Its real orbital period is 16.69 days.

**Profile**

| Property | Value |
|---|---|
| Parent | Jupiter |
| Real radius | 2,410.3 km |
| Real orbital radius | 1,882,700 km |
| Orbital period | 16.689 days |
| Mass | 0.0180 Earth masses |
| Volume | 0.0540 Earth volumes |
| Rotation period | 400.5 hours |
| Density | 1,834 kg/m³ |
| Albedo | 0.22 |
| Inclination | 0.192° |
| Direction | Prograde |
| Visual orbit scale | 2.80 planet radii |

**Role in the simulator:** the outermost Galilean moon and a dark, ancient, heavily cratered world.

### Enceladus

**In-game history**

> Enceladus vents water vapour and ice from a subsurface ocean through cracks near its south pole. The escaping material feeds Saturn's E ring; its real orbital period is 1.37 days. It is one of the most promising places to search for life.

**Profile**

| Property | Value |
|---|---|
| Parent | Saturn |
| Real radius | 252.1 km |
| Real orbital radius | 238,000 km |
| Orbital period | 1.370 days |
| Mass | 1.8 × 10⁻⁵ Earth masses |
| Volume | 6.2 × 10⁻⁵ Earth volumes |
| Rotation period | 32.9 hours |
| Density | 1,609 kg/m³ |
| Albedo | 0.81 |
| Inclination | 0.009° |
| Direction | Prograde |
| Visual orbit scale | 1.70 planet radii |

**Role in the simulator:** a small, bright, icy moon of Saturn. Its high albedo and icy composition make it stand out against Saturn's rings.

### Rhea

**In-game history**

> Rhea is Saturn's second-largest moon. It is an icy body with a heavily cratered surface and a real orbital period of 4.52 days. Its low density suggests it contains a large amount of water ice.

**Profile**

| Property | Value |
|---|---|
| Parent | Saturn |
| Real radius | 763.8 km |
| Real orbital radius | 527,000 km |
| Orbital period | 4.518 days |
| Mass | 3.9 × 10⁻⁴ Earth masses |
| Volume | 0.0017 Earth volumes |
| Rotation period | 108.4 hours |
| Density | 1,236 kg/m³ |
| Albedo | 0.60 |
| Inclination | 0.345° |
| Direction | Prograde |
| Visual orbit scale | 2.10 planet radii |

**Role in the simulator:** Saturn's second-largest moon and an icy, cratered satellite.

### Titan

**In-game history**

> Titan is Saturn's largest moon and the only moon with a dense atmosphere. Lakes of liquid methane and ethane dot its surface, and the Huygens probe landed there in 2005. Its real period is 15.95 days.

**Profile**

| Property | Value |
|---|---|
| Parent | Saturn |
| Real radius | 2,574.7 km |
| Real orbital radius | 1,221,870 km |
| Orbital period | 15.945 days |
| Mass | 0.0225 Earth masses |
| Volume | 0.0660 Earth volumes |
| Rotation period | 382.7 hours |
| Density | 1,880 kg/m³ |
| Albedo | 0.22 |
| Inclination | 0.348° |
| Direction | Prograde |
| Greenhouse offset | 10 K |
| Visual orbit scale | 2.70 planet radii |

**Role in the simulator:** Saturn's largest moon and the only moon in the scene with a nonzero greenhouse offset. Its orange atmosphere colour distinguishes it from the icy moons.

### Titania

**In-game history**

> Titania is the largest moon of Uranus. It is an icy world with long canyons and a real orbital period of 8.71 days. Its surface shows evidence of past geological activity.

**Profile**

| Property | Value |
|---|---|
| Parent | Uranus |
| Real radius | 788.9 km |
| Real orbital radius | 435,910 km |
| Orbital period | 8.706 days |
| Mass | 5.9 × 10⁻⁴ Earth masses |
| Volume | 0.0019 Earth volumes |
| Rotation period | 208.9 hours |
| Density | 1,711 kg/m³ |
| Albedo | 0.35 |
| Inclination | 0.34° |
| Direction | Prograde |
| Visual orbit scale | 1.90 planet radii |

**Role in the simulator:** the largest moon of Uranus, with an icy surface and a relatively large visual orbit.

### Oberon

**In-game history**

> Oberon is the outermost major moon of Uranus. Its dark, cratered surface is one of the oldest in the Uranian system. Its real period is 13.46 days.

**Profile**

| Property | Value |
|---|---|
| Parent | Uranus |
| Real radius | 761.4 km |
| Real orbital radius | 583,520 km |
| Orbital period | 13.463 days |
| Mass | 5.0 × 10⁻⁴ Earth masses |
| Volume | 0.0017 Earth volumes |
| Rotation period | 323.1 hours |
| Density | 1,630 kg/m³ |
| Albedo | 0.31 |
| Inclination | 0.058° |
| Direction | Prograde |
| Visual orbit scale | 2.50 planet radii |

**Role in the simulator:** the outermost major moon of Uranus and a dark, ancient icy world.

### Triton

**In-game history**

> Triton is Neptune's largest moon and orbits backwards compared with Neptune's rotation. It is probably a captured Kuiper Belt object and has nitrogen geysers. Its real period is 5.877 days.

**Profile**

| Property | Value |
|---|---|
| Parent | Neptune |
| Real radius | 1,353.4 km |
| Real orbital radius | 354,759 km |
| Orbital period | 5.877 days |
| Mass | 0.00359 Earth masses |
| Volume | 0.0096 Earth volumes |
| Rotation period | 141.0 hours |
| Density | 2,061 kg/m³ |
| Albedo | 0.76 |
| Inclination | 20.0° |
| Direction | Retrograde |
| Visual orbit scale | 2.20 planet radii |

**Role in the simulator:** Neptune's largest moon and the only seeded moon with a retrograde orbit. Its high inclination and negative direction make it a useful example of a captured satellite.

---

## Major Asteroids

Four physical asteroids are seeded inside the visual asteroid belt. They follow the same N-body gravity as planets and moons and can merge, fragment, or be absorbed. Their density, albedo, and spin metadata are assigned by the seeding code rather than by the real-world seed tables.

### Ceres

**In-game history**

> Ceres is the largest object in the asteroid belt and is classified as a dwarf planet. It contains a large amount of water ice and may have a subsurface brine reservoir; NASA's Dawn mission orbited it. Its surface is a mixture of dark rock and bright carbonate deposits.

**Profile**

| Property | Value |
|---|---|
| Real radius | 473.0 km |
| Seed orbital radius | 0.550 simulation units |
| Start angle | 0.70 radians |
| Mass | 0.000150 Earth masses |
| Visual radius | 0.0025 simulation units |
| Assigned density | 3,000 kg/m³ |
| Assigned albedo | 0.10 |
| Assigned rotation | 9.0 hours |
| Assigned spin rate | 0.8 degrees per time unit |

**Role in the simulator:** the largest physical asteroid and a dwarf-planet example in the belt.

### Vesta

**In-game history**

> Vesta is the second-largest asteroid and the brightest one visible from Earth. Its south pole bears the huge Rheasilvia impact basin, and it has a differentiated rocky interior. Vesta is the source of many of the meteorites found on Earth.

**Profile**

| Property | Value |
|---|---|
| Real radius | 262.0 km |
| Seed orbital radius | 0.535 simulation units |
| Start angle | 2.40 radians |
| Mass | 0.000043 Earth masses |
| Visual radius | 0.0022 simulation units |
| Assigned density | 3,000 kg/m³ |
| Assigned albedo | 0.10 |
| Assigned rotation | 11.0 hours |
| Assigned spin rate | 1.0 degrees per time unit |

**Role in the simulator:** the second-largest physical asteroid and a differentiated rocky body.

### Pallas

**In-game history**

> Pallas is the third-largest asteroid. It follows a highly inclined orbit and its surface is covered with impact craters. It is a dark, primitive B-type body and its orbit is strongly perturbed by Jupiter.

**Profile**

| Property | Value |
|---|---|
| Real radius | 256.0 km |
| Seed orbital radius | 0.565 simulation units |
| Start angle | 2.90 radians |
| Mass | 0.000034 Earth masses |
| Visual radius | 0.0022 simulation units |
| Assigned density | 3,000 kg/m³ |
| Assigned albedo | 0.10 |
| Assigned rotation | 13.0 hours |
| Assigned spin rate | 1.2 degrees per time unit |

**Role in the simulator:** a dark, primitive asteroid with a highly inclined real orbit.

### Hygiea

**In-game history**

> Hygiea is the fourth-largest asteroid and is classified as a dwarf planet candidate. Its dark, carbon-rich surface is one of the oldest in the belt; recent observations suggest it is nearly spherical, which would support dwarf-planet status.

**Profile**

| Property | Value |
|---|---|
| Real radius | 217.0 km |
| Seed orbital radius | 0.585 simulation units |
| Start angle | 5.40 radians |
| Mass | 0.000013 Earth masses |
| Visual radius | 0.0020 simulation units |
| Assigned density | 3,000 kg/m³ |
| Assigned albedo | 0.10 |
| Assigned rotation | 15.0 hours |
| Assigned spin rate | 1.4 degrees per time unit |

**Role in the simulator:** the smallest of the four physical belt asteroids and a dwarf-planet candidate.

---

## Spawnable Body Types

Seven types can be created at runtime with the `1`–`7` keys and the `A` key. Each type has a default history that is stored in the new body. Mass and radius can be scaled at spawn time; the numbers below are the unscaled defaults.

**Type defaults**

| Key | Type | Default mass (sim) | Default radius (sim) | Density (kg/m³) | Albedo | Greenhouse | Visual treatment |
|---|---|---:|---:|---:|---:|---:|---|
| `1` | Star | 0.50 | 0.050 | 1,408 | 0.00 | 0 K | Emissive sphere plus layered glow |
| `2` | Planet | 3.003 × 10⁻⁶ | 0.012 | 5,514 | 0.30 | 0 K | Lit sphere with planet colour |
| `3` | Moon | 3.700 × 10⁻⁸ | 0.006 | 3,344 | 0.12 | 0 K | Lit sphere; parented to a nearby planet when possible |
| `4` | Asteroid | 1.000 × 10⁻⁹ | 0.004 | 3,000 | 0.12 | 0 K | Low-detail lit sphere |
| `5` | Meteor | 1.000 × 10⁻¹⁰ | 0.003 | 3,000 | 0.10 | 0 K | Low-detail sphere plus a velocity streak |
| `6` | Comet | 1.000 × 10⁻¹⁰ | 0.004 | 600 | 0.04 | 0 K | Low-detail sphere plus coma and tails |
| `7` | Black Hole | 2.00 | 0.015 (mass-derived horizon) | 1.0 × 10⁹ | 0.00 | 0 K | Black sphere with an emissive purple torus |

### Star (spawn type)

**In-game history**

> A self-luminous star created in the simulation. Its mass sets the strength of its gravity and the brightness of the light it casts across the system. A star that grows past the collapse threshold will become a black hole.

**Behaviour:** a runtime star is emissive and lights the scene. Its temperature is initialized from its mass. It merges with other stars instead of fragmenting, and it collapses into a black hole when its mass reaches **2.5** simulation units.

### Planet (spawn type)

**In-game history**

> A world created in the simulation. Its mass, visual radius, surface, atmosphere, and orbit can be changed in the Body Editor. Its gravity perturbs every other active body, so small changes can produce very different long-term paths.

**Behaviour:** a runtime planet is a general-purpose world. It can be spawned in orbit around a star, planet, or black hole, or placed on an impact course. Its default mass is exactly one Earth mass in simulation units.

### Moon (spawn type)

**In-game history**

> A natural satellite created in the simulation. Moons are true N-body bodies: they feel the Sun, their parent planet, and every other active body. A moon spawned close to a planet is attached in a Hill-safe physical orbit and rendered at a larger visual scale.

**Behaviour:** if the spawn cursor is within `0.12` simulation units of a planet, the new moon is attached to that planet and placed inside its Hill sphere. Otherwise it follows the normal non-moon spawn path. The HUD reports its parent and real orbital radius when available.

### Asteroid (spawn type)

**In-game history**

> A small, rocky body representing the asteroid belt population. Asteroids follow the same N-body gravity as planets and can graze, fragment, merge, or be absorbed by a larger world.

**Behaviour:** asteroids are small rocky bodies. They use low-detail rendering and are often used as impactors or debris. Collision fragments are created as asteroids named `Debris N`.

### Meteor (spawn type)

**In-game history**

> A tiny, fast-moving fragment. Meteors are useful for testing close encounters, swept collision detection, and high-speed impacts; because they are small and quick, they often need adaptive sub-stepping to avoid tunnelling.

**Behaviour:** meteors are the smallest spawnable type. Their high speed relative to their radius is the main reason the simulation uses adaptive sub-stepping and swept collision detection. Promoted dust particles become meteors.

### Comet (spawn type)

**In-game history**

> An icy body that develops a coma and tails when it approaches a star. Its activity is driven by surface temperature and stellar flux, and its ion and dust tails are shaped by the local radiation field rather than a single scripted direction.

**Behaviour:** comet activity rises with surface temperature. The renderer draws a coma, a blue ion tail, and a curved warm dust tail. Comet spawning uses an eccentric orbit instead of a circular one, with a perihelion near the star and an apoapsis around `2.20` simulation units.

### Black Hole (spawn type)

**In-game history**

> A compact object whose gravity traps light. Its event horizon grows when it absorbs matter, and nearby orbits bend sharply around it. The bright photon ring and accretion disk are visual guides to the region where spacetime curvature becomes extreme.

**Behaviour:** the default black hole has mass `2.00`, which is heavier than the default Sun. Its radius is derived from its mass as an event horizon, clamped between `0.007` and `0.035` simulation units. It absorbs any colliding body, grows in mass, and expands its horizon. It never receives thermal or tidal surface values.

---

## Generated Bodies and Histories

The simulator also creates bodies at runtime. Their history text depends on how they were made.

### Custom bodies from the Body Editor

When you edit a selected body and press `S` to spawn a copy, the new body receives this history:

> A custom [type] created from an edited template in the Body Editor. Its mass, visual radius, surface, atmosphere, and starting kinematics are user-defined; it then evolves under the same N-body gravity as every other body.

The type name is inserted automatically, for example `A custom Planet created from...`.

### Moons spawned around a planet

A moon spawned close to a planet receives this history, with the parent's name inserted:

> A moon created in the simulation around [planet]. It follows the same N-body gravity as every other body, and its physical orbit is kept inside the planet's Hill sphere while the rendered orbit is scaled outward for visibility.

### Collapsed stars

A star that crosses the collapse threshold keeps its name but changes type to black hole. Its history is replaced with:

> This star crossed the collapse threshold and became a black hole. Its event horizon traps light, while the remnant's gravity continues to shape every nearby orbit.

### Promoted dust

A dust particle promoted with `O` becomes a **Meteor** named `Dust N`. It receives the default Meteor history.

### Collision fragments

Collision fragments are **Asteroids** named `Debris N`. They receive the default Asteroid history. A fragmenting impact creates up to 10 debris bodies, limited by the 256-body pool.

### Merged bodies

A merge keeps the heavier body. The surviving body keeps its existing history; the absorbed body's history is discarded with its slot.

### Runtime bodies in general

Any runtime-created Star, Planet, Moon, Asteroid, Meteor, Comet, or Black Hole that does not fall into the special cases above receives its type's default history from the list in [Spawnable Body Types](#spawnable-body-types).

---

## Source Notes

The histories in this file are reproduced from:

- `src/core/universe.cpp` — the Sun's history and the solar-system seeding code.
- `src/core/solar_system_data.cpp` — planet, moon, and asteroid histories and real-world seed values.
- `src/core/body.cpp` — default histories for the seven runtime body types.
- `src/input/spawn_service.cpp` — the spawned-moon history template.
- `src/input/input_controller.cpp` — the custom-body history template.
- `src/physics/collision_solver.cpp` — the collapse history and debris/fragment behaviour.

The simulator stores at most **511 characters** of history per body in a 512-byte buffer (`HIST_STR_LEN`). The HUD wraps the text and shows up to 10 lines in the selected-body panel.

For controls and workflows, see [instructions.md](instructions.md). For the complete feature catalogue, see [functionalities.md](functionalities.md).
