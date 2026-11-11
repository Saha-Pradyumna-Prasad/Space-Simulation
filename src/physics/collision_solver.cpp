#include "physics/collision_solver.h"

#include "core/config.h"
#include "core/events.h"
#include "core/math_utils.h"
#include "core/physics_metrics.h"
#include "core/state.h"
#include "core/universe.h"
#include "ui/status.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace spacesim {

namespace {

void update_black_hole_geometry(CelestialBody &body) {
    body.horizon_radius = clamp_double(
        0.009 + 0.006 * std::cbrt(body.mass / 2.0), 0.007, 0.035);
    body.radius = body.horizon_radius;
    body.collision_radius = body.radius * 0.55;
    body.density_kg_m3 = 1.0e9;
    body.volume_ratio = 0.0;
    body.temperature_k = 0.0;
    body.equilibrium_temperature_k = 0.0;
    body.greenhouse_k = 0.0;
    body.albedo = 0.0;
    body.tidal_heating_k = 0.0;
    body.tidal_ratio = 0.0;
    body.roche_limit = 0.0;
    body.axial_tilt_deg = 23.0;
    body.surface_kind = SurfaceKind::BlackHole;
    body.color[0] = 0.005f;
    body.color[1] = 0.003f;
    body.color[2] = 0.010f;
    body.spin_rate = 2.5;
}

void add_impact_event(UiState &ui, const ImpactResult &impact,
                      float r, float g, float b) {
    FxEvent event;
    event.type = FxEventType::Impact;
    event.x = impact.contact_x;
    event.y = impact.contact_y;
    event.z = impact.contact_z;
    event.nx = impact.normal[0];
    event.ny = impact.normal[1];
    event.nz = impact.normal[2];
    event.energy = 1.0 + impact.energy_ratio;
    event.color[0] = r;
    event.color[1] = g;
    event.color[2] = b;
    ui.fx.push(event);
}

} // namespace

void CollisionSolver::resolve(Universe &universe, UiState &ui,
                              SelectionState &selection, CollisionMode mode,
                              bool physical_collisions) {
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe.isActive(i)) continue;

        for (int j = i + 1; j < MAX_OBJECTS; ++j) {
            if (!universe.isActive(j)) continue;

            CelestialBody &a = universe.body(i);
            CelestialBody &b = universe.body(j);
            if (a.collision_cooldown > 0.0 || b.collision_cooldown > 0.0) {
                continue;
            }

            // Moons are rendered at exaggerated distances while their
            // physical orbits are much smaller. Using visual radii for a
            // moon would make every moon collide with its parent. Use the
            // physical radii for any pair involving a parented moon.
            bool moon_pair = (universe.moon(i).parent_index >= 0 ||
                              universe.moon(j).parent_index >= 0);
            bool pair_physical = physical_collisions || moon_pair ||
                                 a.radius < 0.008 || b.radius < 0.008;

            double radii = effective_collision_radius(a, pair_physical) +
                           effective_collision_radius(b, pair_physical);
            if (radii <= 0.0) radii = a.radius + b.radius;

            double dx = b.x - a.x;
            double dy = b.y - a.y;
            double dz = b.z - a.z;
            double distance_sq = dx * dx + dy * dy + dz * dz;
            bool hit = (distance_sq < radii * radii);

            if (!hit) {
                double p0x = universe.prevX(j) - universe.prevX(i);
                double p0y = universe.prevY(j) - universe.prevY(i);
                double p0z = universe.prevZ(j) - universe.prevZ(i);
                double sx = dx - p0x;
                double sy = dy - p0y;
                double sz = dz - p0z;
                double ss = sx * sx + sy * sy + sz * sz;

                double t = 0.0;
                if (ss > 1.0e-18) {
                    t = -(p0x * sx + p0y * sy + p0z * sz) / ss;
                    t = clamp_double(t, 0.0, 1.0);
                }
                double cx = p0x + t * sx;
                double cy = p0y + t * sy;
                double cz = p0z + t * sz;
                hit = (cx * cx + cy * cy + cz * cz < radii * radii);
            }

            if (!hit) continue;

            ImpactResult impact = classify_impact(a, b, pair_physical);
            add_impact_event(ui, impact,
                             impact.outcome == ImpactOutcome::Fragment ? 1.0f : 1.0f,
                             impact.outcome == ImpactOutcome::Fragment ? 0.55f : 0.85f,
                             0.35f);

            if (a.type == BodyType::BlackHole || b.type == BodyType::BlackHole) {
                bool keep_a = (a.type == BodyType::BlackHole &&
                               b.type != BodyType::BlackHole);
                if (a.type == BodyType::BlackHole &&
                    b.type == BodyType::BlackHole) {
                    keep_a = (a.mass >= b.mass);
                }
                if (keep_a) {
                    mergeBodies(universe, ui, selection, i, j, impact);
                    continue;
                }
                mergeBodies(universe, ui, selection, j, i, impact);
                break;
            }

            if (mode == CollisionMode::Classic) {
                if (a.mass >= b.mass) {
                    mergeBodies(universe, ui, selection, i, j, impact);
                } else {
                    mergeBodies(universe, ui, selection, j, i, impact);
                    break;
                }
                continue;
            }

            switch (impact.outcome) {
                case ImpactOutcome::Merge: {
                    if (a.mass >= b.mass) {
                        mergeBodies(universe, ui, selection, i, j, impact);
                    } else {
                        mergeBodies(universe, ui, selection, j, i, impact);
                        break;
                    }
                    break;
                }
                case ImpactOutcome::Bounce:
                case ImpactOutcome::Graze:
                    bounceBodies(universe, ui, i, j, impact, pair_physical);
                    break;
                case ImpactOutcome::Fragment:
                case ImpactOutcome::Disrupt: {
                    bool disruptive = (impact.outcome == ImpactOutcome::Disrupt);
                    if (a.mass >= b.mass) {
                        fragmentBody(universe, ui, selection, i, j, impact, disruptive);
                    } else {
                        fragmentBody(universe, ui, selection, j, i, impact, disruptive);
                        break;
                    }
                    break;
                }
            }
        }
    }
}

void CollisionSolver::mergeBodies(Universe &universe, UiState &ui,
                                  SelectionState &selection, int keep, int gone,
                                  const ImpactResult &impact) {
    if (keep < 0 || keep >= MAX_OBJECTS || gone < 0 || gone >= MAX_OBJECTS) return;
    if (keep == gone) return;
    if (!universe.isActive(keep) || !universe.isActive(gone)) return;

    CelestialBody &a = universe.body(keep);
    CelestialBody &b = universe.body(gone);

    char absorbed_name[NAME_STR_LEN];
    copy_name(absorbed_name, b.name);

    double mass_a = a.mass;
    double mass_b = b.mass;
    double total_mass = mass_a + mass_b;
    if (total_mass <= 0.0) {
        universe.removeBody(gone);
        return;
    }

    a.vx = (mass_a * a.vx + mass_b * b.vx) / total_mass;
    a.vy = (mass_a * a.vy + mass_b * b.vy) / total_mass;
    a.vz = (mass_a * a.vz + mass_b * b.vz) / total_mass;
    a.x = (mass_a * a.x + mass_b * b.x) / total_mass;
    a.y = (mass_a * a.y + mass_b * b.y) / total_mass;
    a.z = (mass_a * a.z + mass_b * b.z) / total_mass;
    a.mass = total_mass;

    bool keep_is_black_hole = (a.type == BodyType::BlackHole);
    if (keep_is_black_hole) {
        update_black_hole_geometry(a);
    } else {
        if (mass_a > 0.0 && a.radius > 0.0) {
            a.radius *= std::cbrt(total_mass / mass_a);
        } else if (a.radius <= 0.0) {
            a.radius = b.radius;
        }
        a.volume_ratio += b.volume_ratio;
        // Mass and volume are conserved independently; derive the resulting
        // physical density from them instead of leaving the heavier body's
        // old density attached to a different volume.
        if (a.volume_ratio > 0.0) {
            a.density_kg_m3 = (a.mass / EARTH_MASS_SIM) *
                              (EARTH_DENSITY_KG_M3 / a.volume_ratio);
        }
        a.impact_heating_k = std::min(1800.0,
            a.impact_heating_k + 120.0 + 220.0 * impact.energy_ratio);
    }

    if (selection.selected_body_index == gone) {
        selection.selected_body_index = keep;
    }

    universe.removeBody(gone);
    if (keep_is_black_hole) {
        set_status(ui, "Black hole absorbed %s", absorbed_name);
        log_event(ui, 0.82f, 0.55f, 1.0f, "Black hole absorbed %s", absorbed_name);
        FxEvent event;
        event.type = FxEventType::Collapse;
        event.x = a.x;
        event.y = a.y;
        event.z = a.z;
        event.energy = 2.0 + impact.energy_ratio;
        event.color[0] = 0.80f;
        event.color[1] = 0.50f;
        event.color[2] = 1.00f;
        ui.fx.push(event);
    } else {
        set_status(ui, "%s absorbed %s (impact energy %.2f)",
                   a.name, absorbed_name, impact.energy_ratio);
        log_event(ui, 1.0f, 0.78f, 0.35f, "%s absorbed %s",
                  a.name, absorbed_name);
        maybeCollapse(universe, ui, keep, impact);
    }
}

void CollisionSolver::bounceBodies(Universe &universe, UiState &ui,
                                   int a_index, int b_index,
                                   const ImpactResult &impact,
                                   bool physical_collisions) {
    if (!universe.isActive(a_index) || !universe.isActive(b_index)) return;
    CelestialBody &a = universe.body(a_index);
    CelestialBody &b = universe.body(b_index);

    double nx = impact.normal[0];
    double ny = impact.normal[1];
    double nz = impact.normal[2];
    double rvx = b.vx - a.vx;
    double rvy = b.vy - a.vy;
    double rvz = b.vz - a.vz;
    double vn = rvx * nx + rvy * ny + rvz * nz;

    if (vn < 0.0 && a.mass > 0.0 && b.mass > 0.0) {
        double inv_a = 1.0 / a.mass;
        double inv_b = 1.0 / b.mass;
        double impulse = -(1.0 + COLLISION_RESTITUTION) * vn / (inv_a + inv_b);
        a.vx -= impulse * inv_a * nx;
        a.vy -= impulse * inv_a * ny;
        a.vz -= impulse * inv_a * nz;
        b.vx += impulse * inv_b * nx;
        b.vy += impulse * inv_b * ny;
        b.vz += impulse * inv_b * nz;
    }

    double dx = b.x - a.x;
    double dy = b.y - a.y;
    double dz = b.z - a.z;
    double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    double radii = effective_collision_radius(a, physical_collisions) +
                   effective_collision_radius(b, physical_collisions);
    if (radii <= 0.0) radii = a.radius + b.radius;
    if (distance > 1.0e-9 && distance < radii) {
        double correction = (radii - distance) * 0.5;
        a.x -= nx * correction;
        a.y -= ny * correction;
        a.z -= nz * correction;
        b.x += nx * correction;
        b.y += ny * correction;
        b.z += nz * correction;
    }

    a.impact_heating_k = std::min(900.0, a.impact_heating_k + 60.0 * impact.energy_ratio);
    b.impact_heating_k = std::min(900.0, b.impact_heating_k + 60.0 * impact.energy_ratio);
    a.collision_cooldown = COLLISION_COOLDOWN_TIME;
    b.collision_cooldown = COLLISION_COOLDOWN_TIME;
    set_status(ui, "%s and %s survived a %s impact",
               a.name, b.name,
               impact.outcome == ImpactOutcome::Graze ? "grazing" : "bouncing");
    log_event(ui, 0.55f, 0.82f, 1.0f, "%s and %s: %s impact",
              a.name, b.name,
              impact.outcome == ImpactOutcome::Graze ? "grazing" : "bouncing");
}

void CollisionSolver::fragmentBody(Universe &universe, UiState &ui,
                                   SelectionState &selection, int keep, int gone,
                                   const ImpactResult &impact, bool disruptive) {
    if (!universe.isActive(keep) || !universe.isActive(gone)) return;

    CelestialBody &a = universe.body(keep);
    CelestialBody &b = universe.body(gone);
    char gone_name[NAME_STR_LEN];
    copy_name(gone_name, b.name);

    int free_slots = MAX_OBJECTS - universe.count();
    int desired = disruptive ? 8 : 5;
    if (desired > FRAGMENT_MAX) desired = FRAGMENT_MAX;
    int count = std::min(desired, free_slots);
    if (count < 2) {
        mergeBodies(universe, ui, selection, keep, gone, impact);
        return;
    }

    double total_mass = b.mass;
    double fragment_density = b.density_kg_m3;
    double fragment_mass = total_mass / static_cast<double>(count);
    double visual_radius = b.radius * std::cbrt(1.0 / static_cast<double>(count));
    if (visual_radius < MIN_MOON_VISUAL_RADIUS) visual_radius = MIN_MOON_VISUAL_RADIUS;
    double spread = std::max(b.radius * 2.0, impact.relative_speed * 0.02 + 0.002);

    double base_x = b.x;
    double base_y = b.y;
    double base_z = b.z;
    double base_vx = b.vx;
    double base_vy = b.vy;
    double base_vz = b.vz;
    double ejected = impact.relative_speed * (disruptive ? 0.30 : 0.18);

    int type_index = static_cast<int>(BodyType::Asteroid);
    universe.removeBody(gone);
    bool check_collapse = (a.type == BodyType::Star);
    int created[FRAGMENT_MAX];
    int created_count = 0;

    for (int n = 0; n < count; ++n) {
        double angle = 2.0 * PI * static_cast<double>(n) / static_cast<double>(count);
        double wobble = (random_unit() - 0.5) * 0.7;
        double ox = std::cos(angle) * spread;
        double oy = std::sin(angle * 1.7) * spread * 0.35;
        double oz = std::sin(angle) * spread;
        double vx = base_vx + std::cos(angle + wobble) * ejected;
        double vy = base_vy + std::sin(angle * 1.7) * ejected * 0.30;
        double vz = base_vz + std::sin(angle + wobble) * ejected;

        ui.spawn.counter[type_index] += 1;
        char name[NAME_STR_LEN];
        std::snprintf(name, sizeof(name), "Debris %d", ui.spawn.counter[type_index]);
        int slot = universe.addBody(BodyType::Asteroid, name,
                                    base_x + ox, base_y + oy, base_z + oz,
                                    vx, vy, vz, fragment_mass, visual_radius);
        if (slot >= 0 && created_count < FRAGMENT_MAX) {
            CelestialBody &fragment = universe.body(slot);
            fragment.density_kg_m3 = fragment_density;
            sync_body_volume(fragment);
            created[created_count++] = slot;
        }
    }

    // Remove the systemic drift introduced by the random fragment directions.
    double px = 0.0, py = 0.0, pz = 0.0, mass_sum = 0.0;
    for (int n = 0; n < created_count; ++n) {
        const CelestialBody &frag = universe.body(created[n]);
        px += frag.mass * frag.vx;
        py += frag.mass * frag.vy;
        pz += frag.mass * frag.vz;
        mass_sum += frag.mass;
    }
    if (mass_sum > 0.0) {
        double cvx = px / mass_sum;
        double cvy = py / mass_sum;
        double cvz = pz / mass_sum;
        for (int n = 0; n < created_count; ++n) {
            CelestialBody &frag = universe.body(created[n]);
            frag.vx += base_vx - cvx;
            frag.vy += base_vy - cvy;
            frag.vz += base_vz - cvz;
        }
    }

    if (selection.selected_body_index == gone) {
        selection.selected_body_index = keep;
    }

    FxEvent event;
    event.type = FxEventType::Fragment;
    event.x = base_x;
    event.y = base_y;
    event.z = base_z;
    event.energy = 2.0 + impact.energy_ratio;
    event.color[0] = 1.0f;
    event.color[1] = 0.65f;
    event.color[2] = 0.25f;
    ui.fx.push(event);

    set_status(ui, "%s fragmented into %d debris bodies", gone_name, count);
    log_event(ui, 1.0f, 0.62f, 0.28f, "%s fragmented into %d debris",
              gone_name, count);
    if (check_collapse) maybeCollapse(universe, ui, keep, impact);
}

void CollisionSolver::maybeCollapse(Universe &universe, UiState &ui, int index,
                                    const ImpactResult &impact) {
    if (index < 0 || index >= MAX_OBJECTS || !universe.isActive(index)) return;
    CelestialBody &body = universe.body(index);
    if (body.type != BodyType::Star) return;
    if (body.mass < STAR_COLLAPSE_MASS) return;

    body.type = BodyType::BlackHole;
    update_black_hole_geometry(body);
    body.collapse_flash = 1.0;
    copy_history(body.history,
                 "This star crossed the collapse threshold and became a black "
                 "hole. Its event horizon traps light, while the remnant's gravity "
                 "continues to shape every nearby orbit.");

    FxEvent event;
    event.type = FxEventType::Collapse;
    event.x = body.x;
    event.y = body.y;
    event.z = body.z;
    event.energy = 4.0 + impact.energy_ratio;
    event.color[0] = 0.85f;
    event.color[1] = 0.55f;
    event.color[2] = 1.0f;
    ui.fx.push(event);

    set_status(ui, "%s collapsed into a black hole", body.name);
    log_event(ui, 0.85f, 0.55f, 1.0f, "%s collapsed into a black hole", body.name);
}

} // namespace spacesim
