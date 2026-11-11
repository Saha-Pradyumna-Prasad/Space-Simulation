#include "physics/physics_engine.h"

#include "core/config.h"
#include "core/math_utils.h"
#include "core/state.h"
#include "core/universe.h"
#include "ui/status.h"

#include <cmath>

namespace spacesim {

void PhysicsEngine::computeGravity(Universe &universe) {
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe.isActive(i)) continue;
        CelestialBody &b = universe.body(i);
        b.ax = 0.0;
        b.ay = 0.0;
        b.az = 0.0;
    }

    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe.isActive(i)) continue;
        CelestialBody &a = universe.body(i);

        for (int j = i + 1; j < MAX_OBJECTS; ++j) {
            if (!universe.isActive(j)) continue;
            CelestialBody &b = universe.body(j);

            double dx = b.x - a.x;
            double dy = b.y - a.y;
            double dz = b.z - a.z;
            double distance = std::sqrt(dx * dx + dy * dy + dz * dz);

            double epsilon = SOFTENING;
            if (universe.moon(i).parent_index == j ||
                universe.moon(j).parent_index == i) {
                epsilon = MOON_SOFTENING;
            }
            double softened = distance + epsilon;

            if (distance > 1.0e-12) {
                double factor = G_CONSTANT / (softened * softened * distance);
                a.ax += factor * b.mass * dx;
                a.ay += factor * b.mass * dy;
                a.az += factor * b.mass * dz;
                b.ax -= factor * a.mass * dx;
                b.ay -= factor * a.mass * dy;
                b.az -= factor * a.mass * dz;
            }
        }
    }
}

void PhysicsEngine::integrate(Universe &universe, double dt, UiState &ui) {
    universe.savePreviousPositions();
    computeGravity(universe);

    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe.isActive(i)) continue;
        CelestialBody &b = universe.body(i);

        b.vx += b.ax * dt;
        b.vy += b.ay * dt;
        b.vz += b.az * dt;
        b.x += b.vx * dt;
        b.y += b.vy * dt;
        b.z += b.vz * dt;

        if (!body_is_finite(b)) {
            char bad_name[NAME_STR_LEN];
            copy_name(bad_name, b.name);
            universe.removeBody(i);
            set_status(ui, "Removed unstable body %s", bad_name);
            log_event(ui, 1.0f, 0.35f, 0.28f, "Removed unstable body %s", bad_name);
        }
    }
}

int PhysicsEngine::computeSubsteps(const Universe &universe, double dt) const {
    double max_speed = 0.0;
    double min_radius = 1.0e30;

    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe.isActive(i)) continue;
        const CelestialBody &b = universe.body(i);
        double speed = std::sqrt(b.vx * b.vx + b.vy * b.vy + b.vz * b.vz);
        if (speed > max_speed) max_speed = speed;
        if (b.radius > 0.0 && b.radius < min_radius) min_radius = b.radius;
    }

    if (min_radius > 1.0e29) return 1;

    double travel = max_speed * std::fabs(dt);
    int steps = static_cast<int>(travel / (0.5 * min_radius)) + 1;
    return clamp_int(steps, 1, SUBSTEPS_MAX);
}

} // namespace spacesim
