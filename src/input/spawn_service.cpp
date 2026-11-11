#include "input/spawn_service.h"

#include "core/body.h"
#include "core/config.h"
#include "core/math_utils.h"
#include "core/state.h"
#include "core/universe.h"
#include "physics/moon_system.h"
#include "ui/status.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace spacesim {

namespace {

int dominant_primary(const Universe &universe, double x, double y, double z,
                     BodyType spawn_type) {
    if (spawn_type == BodyType::Comet) {
        int best_star = -1;
        double best_distance = 1.0e30;
        for (int i = 0; i < MAX_OBJECTS; ++i) {
            if (!universe.isActive(i)) continue;
            const CelestialBody &body = universe.body(i);
            if (body.type != BodyType::Star) continue;
            double dx = body.x - x;
            double dy = body.y - y;
            double dz = body.z - z;
            double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (distance < best_distance) {
                best_distance = distance;
                best_star = i;
            }
        }
        if (best_star >= 0) return best_star;
    }

    // Prefer a nearby planet or black hole when its local visual sphere
    // contains the cursor. This makes "spawn near Jupiter" actually orbit
    // Jupiter instead of the Sun.
    int local = -1;
    double local_distance = 1.0e30;
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe.isActive(i)) continue;
        const CelestialBody &body = universe.body(i);
        if (body.type != BodyType::Planet && body.type != BodyType::BlackHole) continue;
        double dx = body.x - x;
        double dy = body.y - y;
        double dz = body.z - z;
        double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        double local_radius = std::max(0.08, body.radius * 6.0);
        if (distance > body.radius * 1.15 && distance < local_radius &&
            distance < local_distance) {
            local = i;
            local_distance = distance;
        }
    }
    if (local >= 0) return local;

    int best = -1;
    double best_acceleration = -1.0;
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe.isActive(i)) continue;
        const CelestialBody &body = universe.body(i);
        if (body.mass <= 0.0) continue;
        if (body.type != BodyType::Star && body.type != BodyType::BlackHole) continue;
        double dx = body.x - x;
        double dy = body.y - y;
        double dz = body.z - z;
        double distance_sq = dx * dx + dy * dy + dz * dz;
        if (distance_sq < 1.0e-12) continue;
        double acceleration = G_CONSTANT * body.mass / distance_sq;
        if (acceleration > best_acceleration) {
            best_acceleration = acceleration;
            best = i;
        }
    }
    return best;
}

void orbit_normal(const Universe &universe, int primary, double out[3]) {
    out[0] = 0.0;
    out[1] = 1.0;
    out[2] = 0.0;
    if (primary < 0 || primary >= MAX_OBJECTS) return;

    int reference = universe.heaviestBody();
    if (reference == primary) reference = -1;
    const CelestialBody &p = universe.body(primary);
    double rx = p.x, ry = p.y, rz = p.z;
    double vx = p.vx, vy = p.vy, vz = p.vz;
    if (reference >= 0) {
        const CelestialBody &r = universe.body(reference);
        rx -= r.x;
        ry -= r.y;
        rz -= r.z;
        vx -= r.vx;
        vy -= r.vy;
        vz -= r.vz;
    }
    double hx = ry * vz - rz * vy;
    double hy = rz * vx - rx * vz;
    double hz = rx * vy - ry * vx;
    double length = std::sqrt(hx * hx + hy * hy + hz * hz);
    if (length < 1.0e-9) return;
    out[0] = hx / length;
    out[1] = hy / length;
    out[2] = hz / length;
}

void tangential_direction(const double relative[3], const double normal[3],
                          double out[3]) {
    double length = std::sqrt(relative[0] * relative[0] +
                              relative[1] * relative[1] +
                              relative[2] * relative[2]);
    if (length < 1.0e-12) {
        out[0] = 1.0;
        out[1] = 0.0;
        out[2] = 0.0;
        return;
    }
    double rx = relative[0] / length;
    double ry = relative[1] / length;
    double rz = relative[2] / length;
    out[0] = ry * normal[2] - rz * normal[1];
    out[1] = rz * normal[0] - rx * normal[2];
    out[2] = rx * normal[1] - ry * normal[0];
    double tangent_length = std::sqrt(out[0] * out[0] +
                                      out[1] * out[1] +
                                      out[2] * out[2]);
    if (tangent_length < 1.0e-12) {
        out[0] = 1.0;
        out[1] = 0.0;
        out[2] = 0.0;
    } else {
        out[0] /= tangent_length;
        out[1] /= tangent_length;
        out[2] /= tangent_length;
    }
}

} // namespace

void SpawnService::deleteSelected(Universe &universe, SelectionState &selection,
                                  UiState &ui) const {
    if (selection.selected_body_index < 0 ||
        selection.selected_body_index >= MAX_OBJECTS ||
        !universe.isActive(selection.selected_body_index)) {
        set_status(ui, "No body selected to delete");
        return;
    }

    char deleted_name[NAME_STR_LEN];
    copy_name(deleted_name, universe.body(selection.selected_body_index).name);
    int index = selection.selected_body_index;
    universe.removeBody(index);
    selection.selected_body_index = -1;
    set_status(ui, "Deleted %s", deleted_name);
}

void SpawnService::spawnAtCursor(Universe &universe, UiState &ui,
                                 SelectionState &selection) const {
    const TypeDefaults &defaults = type_defaults(ui.spawn.type);
    double mass = defaults.mass * (static_cast<double>(ui.spawn.mass_percent) / 100.0);
    double radius = defaults.radius * (static_cast<double>(ui.spawn.radius_percent) / 100.0);

    double visual_x = clamp_double(ui.spawn.cursor_x, -1.20, 1.20);
    double visual_z = clamp_double(ui.spawn.cursor_z, -1.20, 1.20);
    double visual_y = 0.0;

    for (int pass = 0; pass < 8; ++pass) {
        bool moved = false;
        for (int i = 0; i < MAX_OBJECTS; ++i) {
            if (!universe.isActive(i)) continue;
            const CelestialBody &b = universe.body(i);
            double position[3];
            MoonSystem::visualPosition(universe, i, position);

            double dx = visual_x - position[0];
            double dy = visual_y - position[1];
            double dz = visual_z - position[2];
            double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
            double minimum = b.radius + radius + 0.012;
            if (distance < minimum) {
                double horizontal = std::sqrt(dx * dx + dz * dz);
                if (horizontal < 1.0e-9) {
                    visual_x = position[0] + minimum;
                    visual_z = position[2];
                } else {
                    visual_x = position[0] + (dx / horizontal) * minimum;
                    visual_z = position[2] + (dz / horizontal) * minimum;
                }
                moved = true;
            }
        }
        if (!moved) break;
    }

    visual_x = clamp_double(visual_x, -1.20, 1.20);
    visual_z = clamp_double(visual_z, -1.20, 1.20);

    int parent_planet = -1;
    if (ui.spawn.type == BodyType::Moon) {
        double best_distance = 0.12;
        for (int i = 0; i < MAX_OBJECTS; ++i) {
            if (!universe.isActive(i)) continue;
            const CelestialBody &candidate = universe.body(i);
            if (candidate.type != BodyType::Planet) continue;
            double position[3];
            MoonSystem::visualPosition(universe, i, position);
            double dx = visual_x - position[0];
            double dy = visual_y - position[1];
            double dz = visual_z - position[2];
            double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (distance < best_distance) {
                best_distance = distance;
                parent_planet = i;
            }
        }
    }

    int primary = parent_planet;
    if (primary < 0) {
        primary = dominant_primary(universe, visual_x, visual_y, visual_z,
                                   ui.spawn.type);
    }
    if (primary < 0) primary = universe.heaviestBody();

    double body_x = visual_x;
    double body_y = visual_y;
    double body_z = visual_z;
    double vx = 0.0;
    double vy = 0.0;
    double vz = 0.0;

    int moon_parent = -1;
    double moon_physical_radius = 0.0;
    double moon_visual_radius = 0.0;
    double moon_visual_scale = 1.0;
    double moon_inclination = 0.0;
    double moon_visual_y = visual_y;

    if (ui.spawn.type == BodyType::Moon && parent_planet >= 0) {
        const CelestialBody &parent = universe.body(parent_planet);
        int sun = universe.findByName("Sun");
        double sun_mass = (sun >= 0 ? universe.body(sun).mass : 1.0);
        if (sun_mass <= 0.0) sun_mass = 1.0;

        double parent_orbit = 0.0;
        if (sun >= 0) {
            double sx = parent.x - universe.body(sun).x;
            double sy = parent.y - universe.body(sun).y;
            double sz = parent.z - universe.body(sun).z;
            parent_orbit = std::sqrt(sx * sx + sy * sy + sz * sz);
        } else {
            parent_orbit = std::sqrt(parent.x * parent.x +
                                     parent.y * parent.y +
                                     parent.z * parent.z);
        }
        if (parent_orbit < 1.0e-6) parent_orbit = 0.10;

        double hill = parent_orbit * std::cbrt(parent.mass / (3.0 * sun_mass));
        if (hill < 1.0e-6) hill = parent_orbit * 0.01;

        double dx = visual_x - parent.x;
        double dz = visual_z - parent.z;
        double visual_radius = std::sqrt(dx * dx + dz * dz);
        if (visual_radius < 1.0e-6) {
            visual_radius = parent.radius + radius + 0.012;
            dx = visual_radius;
            dz = 0.0;
        }

        double parent_radius = (parent.radius > 0.0 ? parent.radius : 1.0);
        double visual_factor = visual_radius / parent_radius;
        double physical_fraction = 0.15 +
            0.25 * (visual_factor - 1.5) / 1.3;
        physical_fraction = clamp_double(physical_fraction, 0.20, 0.40);
        double physical_radius = hill * physical_fraction;
        if (physical_radius < 1.0e-6) physical_radius = 1.0e-4;

        double angle = std::atan2(dz, dx);
        double visual_scale = visual_radius / physical_radius;

        body_x = parent.x + physical_radius * std::cos(angle);
        body_y = parent.y;
        body_z = parent.z + physical_radius * std::sin(angle);
        moon_visual_y = parent.y;

        double physical_speed = std::sqrt(G_CONSTANT * parent.mass * physical_radius) /
                                (physical_radius + MOON_SOFTENING);
        double direction = (ui.spawn.velocity_mode == 1) ? -1.0 : 1.0;
        if (ui.spawn.velocity_mode == 2) physical_speed = 0.0;

        vx = parent.vx + direction * (-std::sin(angle)) * physical_speed;
        vy = parent.vy;
        vz = parent.vz + direction * (std::cos(angle)) * physical_speed;

        moon_parent = parent_planet;
        moon_physical_radius = physical_radius;
        moon_visual_radius = visual_radius;
        moon_visual_scale = visual_scale;
        moon_inclination = 0.0;
    } else if (primary >= 0) {
        const CelestialBody &p = universe.body(primary);
        double relative[3] = {body_x - p.x, body_y - p.y, body_z - p.z};
        double distance = std::sqrt(relative[0] * relative[0] +
                                    relative[1] * relative[1] +
                                    relative[2] * relative[2]);
        if (distance > 1.0e-6 && p.mass > 0.0) {
            double normal[3];
            orbit_normal(universe, primary, normal);
            double tangent[3];
            tangential_direction(relative, normal, tangent);
            double direction = (ui.spawn.velocity_mode == 1) ? -1.0 : 1.0;

            if (ui.spawn.velocity_mode == 2) {
                vx = p.vx;
                vy = p.vy;
                vz = p.vz;
            } else {
                double speed = std::sqrt(G_CONSTANT * p.mass * distance) /
                               (distance + SOFTENING);
                if (ui.spawn.type == BodyType::Comet) {
                    double perihelion = std::max(0.08, p.radius * 3.0);
                    double apoapsis = 2.20;
                    double semi_major = (perihelion + apoapsis) * 0.5;
                    double vis_viva = 2.0 / distance - 1.0 / semi_major;
                    if (vis_viva > 0.0) {
                        speed = std::sqrt(G_CONSTANT * p.mass * vis_viva);
                    }
                }
                vx = p.vx + tangent[0] * speed * direction;
                vy = p.vy + tangent[1] * speed * direction;
                vz = p.vz + tangent[2] * speed * direction;
            }
        }
    }

    int type_index = static_cast<int>(ui.spawn.type);
    ui.spawn.counter[type_index] += 1;
    int serial = ui.spawn.counter[type_index];

    char name[NAME_STR_LEN];
    std::snprintf(name, sizeof(name), "%s %d", type_name(ui.spawn.type), serial);

    int slot = universe.addBody(ui.spawn.type, name,
                                body_x, body_y, body_z,
                                vx, vy, vz,
                                mass, radius);
    if (slot < 0) {
        set_status(ui, "Cannot add: pool is full (%d bodies)", MAX_OBJECTS);
        return;
    }

    CelestialBody &spawned = universe.body(slot);
    spawned.density_kg_m3 = defaults.density_kg_m3;
    spawned.albedo = defaults.albedo;
    spawned.greenhouse_k = defaults.greenhouse_k;

    if (moon_parent >= 0) {
        MoonOrbit &orbit = universe.moon(slot);
        orbit.parent_index = moon_parent;
        orbit.physical_radius = moon_physical_radius;
        orbit.visual_radius = moon_visual_radius;
        orbit.visual_scale = moon_visual_scale;
        orbit.inclination = moon_inclination;
        orbit.real_orbit_radius_km = 0.0;
        orbit.visual_x = visual_x;
        orbit.visual_y = moon_visual_y;
        orbit.visual_z = visual_z;
        spawned.orbital_period = 0.0;
        std::snprintf(spawned.history, sizeof(spawned.history),
                      "A moon created in the simulation around %s. It follows the same "
                      "N-body gravity as every other body, and its physical orbit is "
                      "kept inside the planet's Hill sphere while the rendered orbit is "
                      "scaled outward for visibility.",
                      universe.body(moon_parent).name);
    }

    FxEvent event;
    event.type = FxEventType::Spawn;
    event.x = body_x;
    event.y = body_y;
    event.z = body_z;
    event.energy = 1.0;
    event.color[0] = defaults.color[0];
    event.color[1] = defaults.color[1];
    event.color[2] = defaults.color[2];
    ui.fx.push(event);

    selection.selected_body_index = slot;
    if (primary >= 0 && universe.isActive(primary)) {
        set_status(ui, "Added %s orbiting %s", name, universe.body(primary).name);
    } else {
        set_status(ui, "Added %s", name);
    }
}

} // namespace spacesim
