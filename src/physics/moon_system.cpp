#include "physics/moon_system.h"

#include "core/config.h"
#include "core/universe.h"

#include <cmath>

namespace spacesim {

void MoonSystem::visualPosition(const Universe &universe, int index, double out[3]) {
    out[0] = 0.0;
    out[1] = 0.0;
    out[2] = 0.0;
    if (index < 0 || index >= MAX_OBJECTS) return;
    if (!universe.isActive(index)) return;

    const MoonOrbit &orbit = universe.moon(index);
    if (orbit.parent_index >= 0 && orbit.parent_index < MAX_OBJECTS &&
        universe.isActive(orbit.parent_index)) {
        out[0] = orbit.visual_x;
        out[1] = orbit.visual_y;
        out[2] = orbit.visual_z;
    } else {
        const CelestialBody &body = universe.body(index);
        out[0] = body.x;
        out[1] = body.y;
        out[2] = body.z;
    }
}

void MoonSystem::updateVisuals(Universe &universe) const {
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe.isActive(i)) continue;
        MoonOrbit &orbit = universe.moon(i);
        if (orbit.parent_index < 0) continue;
        if (orbit.parent_index >= MAX_OBJECTS) continue;
        if (!universe.isActive(orbit.parent_index)) continue;

        const CelestialBody &parent = universe.body(orbit.parent_index);
        const CelestialBody &moon = universe.body(i);
        double scale = orbit.visual_scale;
        if (scale <= 0.0) scale = 1.0;

        double dx = moon.x - parent.x;
        double dy = moon.y - parent.y;
        double dz = moon.z - parent.z;
        double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (distance > 1.0e-12) orbit.visual_radius = distance * scale;

        orbit.visual_x = parent.x + dx * scale;
        orbit.visual_y = parent.y + dy * scale;
        orbit.visual_z = parent.z + dz * scale;
    }
}

} // namespace spacesim
