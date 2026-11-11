#include "input/picking.h"

#include "core/config.h"
#include "core/math_utils.h"
#include "core/state.h"
#include "core/universe.h"
#include "physics/moon_system.h"
#include "render/camera.h"

#include <cmath>

namespace spacesim {

void screen_ray(const OrbitCamera &camera, const Universe &universe,
                const SelectionState &selection, const UiState &ui,
                int screen_x, int screen_y,
                double origin[3], double direction[3]) {
    camera.screen_ray(universe, selection, ui, screen_x, screen_y,
                      origin, direction);
}

int pick_body(const Universe &universe, const OrbitCamera &camera,
              const SelectionState &selection, const UiState &ui,
              int screen_x, int screen_y) {
    double origin[3];
    double direction[3];
    screen_ray(camera, universe, selection, ui, screen_x, screen_y,
               origin, direction);

    int best = -1;
    double best_t = 1.0e30;
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe.isActive(i)) continue;
        const CelestialBody &body = universe.body(i);
        double center[3];
        MoonSystem::visualPosition(universe, i, center);
        double hit_t = 0.0;
        if (ray_sphere_hit(origin, direction, center, body.radius + 0.010, hit_t)) {
            if (hit_t < best_t) {
                best_t = hit_t;
                best = i;
            }
        }
    }
    return best;
}

bool ray_plane_y0(const double origin[3], const double direction[3],
                  double &x, double &z) {
    if (std::fabs(direction[1]) < 1.0e-9) return false;
    double t = -origin[1] / direction[1];
    if (t <= 0.0) return false;
    x = origin[0] + t * direction[0];
    z = origin[2] + t * direction[2];
    return true;
}

} // namespace spacesim
