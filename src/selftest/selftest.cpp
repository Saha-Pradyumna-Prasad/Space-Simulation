#include "selftest/selftest.h"

#include "core/config.h"
#include "core/math_utils.h"
#include "core/physics_metrics.h"
#include "core/solar_system_data.h"
#include "core/state.h"
#include "core/universe.h"
#include "fx/dust_field.h"
#include "fx/effects.h"
#include "input/spawn_service.h"
#include "physics/collision_solver.h"
#include "physics/environment.h"
#include "physics/moon_system.h"
#include "physics/physics_engine.h"
#include "render/camera.h"
#include "render/cosmetic_scene.h"
#include "ui/body_editor.h"

#include <cmath>
#include <cstdio>

namespace spacesim {

namespace {

bool check(bool condition, const char *label) {
    std::printf("[%s] %s\n", condition ? "PASS" : "FAIL", label);
    return condition;
}

struct TestWorld {
    Universe universe;
    PhysicsEngine physics;
    CollisionSolver collisions;
    MoonSystem moons;
    UiState ui;
    SimulationState simulation;
    SelectionState selection;

    void reset() {
        universe.reset();
        universe.seedSolarSystem();
        universe.seedMoons();
        selection.selected_body_index = -1;
        selection.follow_selected = false;
        simulation.time_multiplier = 1.0;
        simulation.paused = false;
        simulation.sim_time = 0.0;
        ui.status_frames = 0;
        ui.status_message[0] = '\0';
    }

    void stepPhysicsOnly(double dt) {
        physics.integrate(universe, dt, ui);
        collisions.resolve(universe, ui, selection, simulation.collision_mode,
                           simulation.physical_collisions);
        moons.updateVisuals(universe);
    }
};

} // namespace

int run_selftest() {
    bool ok = true;

    std::printf("Space Simulator v3.5 self-test\n");
    std::printf("sizeof(CelestialBody) = %d bytes\n",
                static_cast<int>(sizeof(CelestialBody)));
    ok = check(sizeof(CelestialBody) >= 700 && sizeof(CelestialBody) <= 1024,
               "CelestialBody fits the pool budget") && ok;

    double ray_origin[3] = {0.0, 0.0, -5.0};
    double ray_direction[3] = {0.0, 0.0, 1.0};
    double sphere_center[3] = {0.0, 0.0, 0.0};
    double hit_t = 0.0;
    ok = check(ray_sphere_hit(ray_origin, ray_direction, sphere_center, 1.0, hit_t) &&
               std::fabs(hit_t - 4.0) < 1.0e-6,
               "ray-sphere hit returns the near intersection") && ok;
    double miss_origin[3] = {0.0, 5.0, -5.0};
    ok = check(!ray_sphere_hit(miss_origin, ray_direction, sphere_center, 1.0, hit_t),
               "ray-sphere miss is rejected") && ok;

    TestWorld world;
    world.reset();
    world.universe.printBodies();
    ok = check(world.universe.count() == 26, "initial layout has 26 active bodies") && ok;
    ok = check(world.universe.findByName("Sun") >= 0, "Sun exists") && ok;
    ok = check(world.universe.findByName("Earth") >= 0, "Earth exists") && ok;
    ok = check(world.universe.findByName("Ceres") >= 0 &&
               world.universe.findByName("Vesta") >= 0 &&
               world.universe.findByName("Pallas") >= 0 &&
               world.universe.findByName("Hygiea") >= 0,
               "major belt asteroids exist") && ok;
    int history_count = 0;
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (world.universe.isActive(i) && world.universe.body(i).history[0] != '\0') {
            ++history_count;
        }
    }
    ok = check(history_count == world.universe.count(),
               "every initial body has a history") && ok;
    int earth_surface = world.universe.findByName("Earth");
    if (earth_surface >= 0) {
        const CelestialBody &earth_body = world.universe.body(earth_surface);
        ok = check(earth_body.surface_kind == SurfaceKind::Earth &&
                   earth_body.axial_tilt_deg > 20.0,
                   "Earth has an Earth surface and axial tilt") && ok;
    }

    CosmeticScene cosmetics;
    cosmetics.init();
    bool belt_valid = (cosmetics.beltCount() == 5000);
    int gap_count = 0;
    for (int i = 0; i < cosmetics.beltCount(); ++i) {
        const CosmeticScene::BeltParticle &particle = cosmetics.belt()[i];
        if (particle.a < 0.49 || particle.a > 0.64) belt_valid = false;
        if (particle.e <= 0.0 || std::fabs(particle.inclination) < 1.0e-6) {
            belt_valid = false;
        }
        if (std::fabs(particle.a - 0.5433) < 0.0036) ++gap_count;
    }
    ok = check(belt_valid, "belt has eccentric, inclined particles in range") && ok;
    ok = check(gap_count == 0, "Kirkwood gaps contain no belt particles") && ok;

    int sun_metrics = world.universe.findByName("Sun");
    int earth_metrics = world.universe.findByName("Earth");
    if (sun_metrics >= 0 && earth_metrics >= 0) {
        const CelestialBody &sun_body = world.universe.body(sun_metrics);
        const CelestialBody &earth_body = world.universe.body(earth_metrics);
        PairMetrics pair = compute_pair_metrics(sun_body, earth_body);
        OrbitMetrics orbit = compute_orbit_metrics(earth_body, sun_body);
        BodyMetrics body_metrics = compute_body_metrics(earth_body);
        ok = check(pair.force > 0.0 && pair.escape_velocity > 0.0,
                   "pair metrics produce positive force and escape velocity") && ok;
        ok = check(orbit.valid && orbit.semi_major_axis > 0.30 &&
                   orbit.semi_major_axis < 0.45 && orbit.period > 0.0,
                   "orbit metrics produce a valid Earth-Sun orbit") && ok;
        ok = check(body_metrics.surface_gravity > 0.0 &&
                   body_metrics.escape_velocity > 0.0,
                   "body metrics produce gravity and escape velocity") && ok;

        CelestialBody editable = earth_body;
        ok = check(apply_body_field_text(editable, EDITOR_MASS, "2.5") &&
                   std::fabs(editable.mass / EARTH_MASS_SIM - 2.5) < 0.01,
                   "body editor applies custom mass") && ok;
        ok = check(apply_body_field_text(editable, EDITOR_TILT, "45") &&
                   std::fabs(editable.axial_tilt_deg - 45.0) < 1.0e-9,
                   "body editor applies custom axial tilt") && ok;
        ok = check(apply_body_field_text(editable, EDITOR_TYPE, "6") &&
                   editable.type == BodyType::BlackHole &&
                   editable.horizon_radius > 0.0,
                   "body editor applies black-hole type and horizon") && ok;

        CelestialBody editable2 = earth_body;
        double collision_before = editable2.collision_radius;
        adjust_body_field(editable2, EDITOR_ALBEDO, 1, false);
        ok = check(std::fabs(editable2.collision_radius - collision_before) < 1.0e-15,
                   "body appearance edits preserve collision radius") && ok;
        adjust_body_field(editable2, EDITOR_MASS, 1, false);
        ok = check(editable2.mass > earth_body.mass,
                   "body editor arrow adjustment changes mass") && ok;

        CelestialBody editable3 = earth_body;
        double earth_volume = editable3.volume_ratio;
        double earth_density = editable3.density_kg_m3;
        double earth_collision = editable3.collision_radius;
        adjust_body_field(editable3, EDITOR_ALBEDO, 1, false);
        ok = check(std::fabs(editable3.volume_ratio - earth_volume) < 1.0e-12 &&
                   std::fabs(editable3.density_kg_m3 - earth_density) < 1.0e-12 &&
                   std::fabs(editable3.collision_radius - earth_collision) < 1.0e-15,
                   "appearance edits preserve physical properties") && ok;

        ok = check(apply_body_field_text(editable3, EDITOR_VOLUME, "2.0"),
                   "body editor accepts custom volume") && ok;
        ok = check(std::fabs(editable3.volume_ratio - 2.0) < 1.0e-9 &&
                   std::fabs(editable3.density_kg_m3 - earth_density) < 1.0e-9,
                   "volume edit keeps density and updates volume") && ok;

        double mass_before_density = editable3.mass;
        ok = check(apply_body_field_text(editable3, EDITOR_DENSITY, "1000"),
                   "body editor accepts custom density") && ok;
        ok = check(std::fabs(editable3.mass - mass_before_density) < 1.0e-15 &&
                   std::fabs(editable3.volume_ratio -
                             earth_volume_from_mass_density(editable3.mass,
                                                            editable3.density_kg_m3)) < 1.0e-9,
                   "density edit updates volume consistently") && ok;

        double collision_before_radius = editable3.collision_radius;
        ok = check(apply_body_field_text(editable3, EDITOR_RADIUS, "0.020"),
                   "body editor accepts visual radius") && ok;
        ok = check(std::fabs(editable3.collision_radius - collision_before_radius) < 1.0e-15,
                   "visual radius edits preserve collision radius") && ok;

        ok = check(apply_body_field_text(editable3, EDITOR_COLLISION_SCALE, "0.50") &&
                   std::fabs(editable3.collision_scale - 0.50) < 1.0e-9,
                   "body editor accepts collision scale") && ok;
    }

    int sun = world.universe.findByName("Sun");
    int earth = world.universe.findByName("Earth");

    int planet_indices[8];
    double planet_initial_radii[8];
    int planet_total = 0;
    for (int i = 0; i < 8; ++i) {
        int index = world.universe.findByName(PLANET_SEEDS[i].name);
        if (index < 0 || sun < 0) continue;
        double dx = world.universe.body(index).x - world.universe.body(sun).x;
        double dy = world.universe.body(index).y - world.universe.body(sun).y;
        double dz = world.universe.body(index).z - world.universe.body(sun).z;
        planet_indices[planet_total] = index;
        planet_initial_radii[planet_total] = std::sqrt(dx * dx + dy * dy + dz * dz);
        ++planet_total;
    }

    double initial_radius = 0.0;
    if (sun >= 0 && earth >= 0) {
        double dx = world.universe.body(earth).x - world.universe.body(sun).x;
        double dy = world.universe.body(earth).y - world.universe.body(sun).y;
        double dz = world.universe.body(earth).z - world.universe.body(sun).z;
        initial_radius = std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    for (int i = 0; i < 20000; ++i) {
        world.stepPhysicsOnly(BASE_DT / 4.0);
    }

    double final_radius = 0.0;
    if (sun >= 0 && earth >= 0 &&
        world.universe.isActive(sun) && world.universe.isActive(earth)) {
        double dx = world.universe.body(earth).x - world.universe.body(sun).x;
        double dy = world.universe.body(earth).y - world.universe.body(sun).y;
        double dz = world.universe.body(earth).z - world.universe.body(sun).z;
        final_radius = std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    if (initial_radius > 0.0) {
        std::printf("Earth radius: initial %.4f, final %.4f (drift %.2f%%)\n",
                    initial_radius, final_radius,
                    100.0 * std::fabs(final_radius - initial_radius) / initial_radius);
    }
    ok = check(std::isfinite(final_radius), "Earth position stays finite") && ok;
    ok = check(initial_radius > 0.0 &&
               std::fabs(final_radius - initial_radius) / initial_radius < 0.05,
               "Earth orbit radius drift is under 5 percent") && ok;

    bool all_planets_stable = (planet_total == 8 && sun >= 0);
    for (int i = 0; i < planet_total; ++i) {
        int index = planet_indices[i];
        if (!world.universe.isActive(index) || !world.universe.isActive(sun)) {
            all_planets_stable = false;
            break;
        }
        double dx = world.universe.body(index).x - world.universe.body(sun).x;
        double dy = world.universe.body(index).y - world.universe.body(sun).y;
        double dz = world.universe.body(index).z - world.universe.body(sun).z;
        double radius = std::sqrt(dx * dx + dy * dy + dz * dz);
        double drift = std::fabs(radius - planet_initial_radii[i]) /
                       planet_initial_radii[i];
        std::printf("  %-8s orbit drift %.2f%%\n",
                    world.universe.body(index).name, 100.0 * drift);
        if (drift >= 0.05) {
            all_planets_stable = false;
            break;
        }
    }
    ok = check(all_planets_stable,
               "all eight planets stay within 5% of their initial orbits") && ok;

    int moon_index = world.universe.findByName("Moon");
    if (moon_index >= 0 && earth >= 0) {
        double dx = world.universe.body(moon_index).x - world.universe.body(earth).x;
        double dy = world.universe.body(moon_index).y - world.universe.body(earth).y;
        double dz = world.universe.body(moon_index).z - world.universe.body(earth).z;
        double moon_distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        double expected_distance = world.universe.moon(moon_index).physical_radius;
        if (expected_distance > 0.0) {
            std::printf("Moon physical radius: initial %.6f, final %.6f (drift %.2f%%)\n",
                        expected_distance, moon_distance,
                        100.0 * std::fabs(moon_distance - expected_distance) / expected_distance);
        }
        ok = check(world.universe.moon(moon_index).parent_index == earth,
                   "Moon is attached to Earth in the N-body model") && ok;
        ok = check(expected_distance > 0.0 &&
                   std::fabs(moon_distance - expected_distance) / expected_distance < 0.20,
                   "Moon stays in its Hill-safe physical orbit") && ok;

        double visual_dx = world.universe.moon(moon_index).visual_x - world.universe.body(earth).x;
        double visual_dy = world.universe.moon(moon_index).visual_y - world.universe.body(earth).y;
        double visual_dz = world.universe.moon(moon_index).visual_z - world.universe.body(earth).z;
        double visual_distance = std::sqrt(visual_dx * visual_dx +
                                           visual_dy * visual_dy +
                                           visual_dz * visual_dz);
        double expected_visual = moon_distance * world.universe.moon(moon_index).visual_scale;
        ok = check(std::fabs(visual_distance - expected_visual) < 1.0e-6,
                   "Moon visual position follows its physical orbit") && ok;
    }

    bool all_moons_bound = true;
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!world.universe.isActive(i)) continue;
        const MoonOrbit &orbit = world.universe.moon(i);
        if (orbit.parent_index < 0) continue;
        if (orbit.parent_index >= MAX_OBJECTS ||
            !world.universe.isActive(orbit.parent_index)) {
            all_moons_bound = false;
            break;
        }
        double dx = world.universe.body(i).x - world.universe.body(orbit.parent_index).x;
        double dy = world.universe.body(i).y - world.universe.body(orbit.parent_index).y;
        double dz = world.universe.body(i).z - world.universe.body(orbit.parent_index).z;
        double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (orbit.physical_radius <= 0.0 ||
            distance < orbit.physical_radius * 0.20 ||
            distance > orbit.physical_radius * 2.00) {
            all_moons_bound = false;
            break;
        }
    }
    ok = check(all_moons_bound, "all moons remain bound to their parents") && ok;
    ok = check(world.universe.count() == 26,
               "stable layout has no accidental collisions") && ok;

    world.reset();
    Environment environment;
    environment.update(world.universe, 0.10);
    int earth_temp = world.universe.findByName("Earth");
    int venus_temp = world.universe.findByName("Venus");
    int neptune_temp = world.universe.findByName("Neptune");
    int moon_temp = world.universe.findByName("Moon");
    int io_temp = world.universe.findByName("Io");
    if (earth_temp >= 0 && venus_temp >= 0 && neptune_temp >= 0) {
        const CelestialBody &earth_body = world.universe.body(earth_temp);
        const CelestialBody &venus_body = world.universe.body(venus_temp);
        const CelestialBody &neptune_body = world.universe.body(neptune_temp);
        std::printf("Earth temperature: %.0f K, Venus: %.0f K, Neptune: %.0f K\n",
                    earth_body.temperature_k, venus_body.temperature_k,
                    neptune_body.temperature_k);
        ok = check(earth_body.temperature_k > 260.0 &&
                   earth_body.temperature_k < 320.0,
                   "Earth surface temperature is Earth-like") && ok;
        ok = check(venus_body.temperature_k > earth_body.temperature_k,
                   "Venus is hotter than Earth") && ok;
        ok = check(neptune_body.temperature_k < earth_body.temperature_k,
                   "Neptune is colder than Earth") && ok;
    }
    if (moon_temp >= 0 && io_temp >= 0) {
        ok = check(world.universe.body(io_temp).tidal_ratio >
                   world.universe.body(moon_temp).tidal_ratio,
                   "Io experiences a stronger tide than the Moon") && ok;
        ok = check(world.universe.body(io_temp).tidal_heating_k >
                   world.universe.body(moon_temp).tidal_heating_k,
                   "Io receives more tidal heating than the Moon") && ok;
    }

    world.reset();
    int jupiter_spawn = world.universe.findByName("Jupiter");
    double prograde_vx = 0.0, prograde_vy = 0.0, prograde_vz = 0.0;
    if (jupiter_spawn >= 0) {
        const CelestialBody &jupiter = world.universe.body(jupiter_spawn);
        double jupiter_vx = jupiter.vx;
        double jupiter_vy = jupiter.vy;
        double jupiter_vz = jupiter.vz;
        SpawnService spawn;

        world.ui.spawn.type = BodyType::Asteroid;
        world.ui.spawn.velocity_mode = 2;
        world.ui.spawn.cursor_x = jupiter.x + 0.10;
        world.ui.spawn.cursor_z = jupiter.z;
        world.ui.spawn.cursor_valid = true;
        spawn.spawnAtCursor(world.universe, world.ui, world.selection);
        if (world.selection.selected_body_index >= 0 &&
            world.universe.isActive(world.selection.selected_body_index)) {
            const CelestialBody &body = world.universe.body(world.selection.selected_body_index);
            double rvx = body.vx - jupiter_vx;
            double rvy = body.vy - jupiter_vy;
            double rvz = body.vz - jupiter_vz;
            ok = check(std::sqrt(rvx * rvx + rvy * rvy + rvz * rvz) < 0.02,
                       "zero-velocity spawn is co-moving with Jupiter") && ok;
        } else {
            ok = check(false, "zero-velocity spawn near Jupiter is selected") && ok;
        }

        world.reset();
        jupiter_spawn = world.universe.findByName("Jupiter");
        const CelestialBody &jupiter2 = world.universe.body(jupiter_spawn);
        double j2vx = jupiter2.vx;
        double j2vy = jupiter2.vy;
        double j2vz = jupiter2.vz;
        world.ui.spawn.type = BodyType::Asteroid;
        world.ui.spawn.velocity_mode = 0;
        world.ui.spawn.cursor_x = jupiter2.x + 0.10;
        world.ui.spawn.cursor_z = jupiter2.z;
        world.ui.spawn.cursor_valid = true;
        spawn.spawnAtCursor(world.universe, world.ui, world.selection);
        if (world.selection.selected_body_index >= 0 &&
            world.universe.isActive(world.selection.selected_body_index)) {
            const CelestialBody &body = world.universe.body(world.selection.selected_body_index);
            double rvx = body.vx - j2vx;
            double rvy = body.vy - j2vy;
            double rvz = body.vz - j2vz;
            prograde_vx = rvx;
            prograde_vy = rvy;
            prograde_vz = rvz;
            ok = check(std::sqrt(rvx * rvx + rvy * rvy + rvz * rvz) > 0.01,
                       "prograde spawn near Jupiter has local orbital velocity") && ok;
        }

        world.reset();
        jupiter_spawn = world.universe.findByName("Jupiter");
        const CelestialBody &jupiter3 = world.universe.body(jupiter_spawn);
        double j3vx = jupiter3.vx;
        double j3vy = jupiter3.vy;
        double j3vz = jupiter3.vz;
        world.ui.spawn.type = BodyType::Asteroid;
        world.ui.spawn.velocity_mode = 1;
        world.ui.spawn.cursor_x = jupiter3.x + 0.10;
        world.ui.spawn.cursor_z = jupiter3.z;
        world.ui.spawn.cursor_valid = true;
        spawn.spawnAtCursor(world.universe, world.ui, world.selection);
        if (world.selection.selected_body_index >= 0 &&
            world.universe.isActive(world.selection.selected_body_index)) {
            const CelestialBody &body = world.universe.body(world.selection.selected_body_index);
            double rvx = body.vx - j3vx;
            double rvy = body.vy - j3vy;
            double rvz = body.vz - j3vz;
            double dot = rvx * prograde_vx + rvy * prograde_vy + rvz * prograde_vz;
            ok = check(dot < 0.0,
                       "retrograde spawn reverses the orbital direction") && ok;
        }
    }

    world.reset();
    int earth_remove = world.universe.findByName("Earth");
    int moon_remove = world.universe.findByName("Moon");
    if (earth_remove >= 0 && moon_remove >= 0) {
        ok = check(world.universe.removeBody(earth_remove), "Earth can be removed") && ok;
        ok = check(world.universe.isActive(moon_remove),
                   "Earth's Moon detaches instead of disappearing") && ok;
        ok = check(world.universe.moon(moon_remove).parent_index == -1,
                   "detached Moon is unparented") && ok;
        ok = check(world.universe.count() == 25,
                   "planet removal leaves its moon in the simulation") && ok;
    }

    world.reset();
    int a = world.universe.addBody(BodyType::Asteroid, "Test A",
                                   0.90, 0.0, 0.90, 0.0, 0.0, 0.0, 1.0e-9, 0.004);
    int b = world.universe.addBody(BodyType::Asteroid, "Test B",
                                   0.92, 0.0, 0.90, 0.0, 0.0, 0.0, 1.0e-9, 0.004);
    int c = world.universe.addBody(BodyType::Asteroid, "Test C",
                                   0.94, 0.0, 0.90, 0.0, 0.0, 0.0, 1.0e-9, 0.004);
    ok = check(a >= 0 && b >= 0 && c >= 0, "three bodies can be added") && ok;
    ok = check(world.universe.count() == 29, "active count rises to 29") && ok;
    ok = check(world.universe.removeBody(b), "middle body can be removed") && ok;
    int reused = world.universe.addBody(BodyType::Asteroid, "Test D",
                                        0.96, 0.0, 0.90, 0.0, 0.0, 0.0, 1.0e-9, 0.004);
    ok = check(reused == b, "removed slot is reused") && ok;
    ok = check(!world.universe.removeBody(999),
               "out-of-range removal is rejected") && ok;

    world.reset();
    int earth_index = world.universe.findByName("Earth");
    if (earth_index >= 0) {
        double earth_vx = world.universe.body(earth_index).vx;
        double earth_vy = world.universe.body(earth_index).vy;
        double earth_vz = world.universe.body(earth_index).vz;

        world.ui.spawn.type = BodyType::Moon;
        world.ui.spawn.mass_percent = 100;
        world.ui.spawn.radius_percent = 100;
        world.ui.spawn.velocity_mode = 0;
        world.ui.spawn.cursor_x = world.universe.body(earth_index).x;
        world.ui.spawn.cursor_z = world.universe.body(earth_index).z;

        int before_spawn = world.universe.count();
        SpawnService spawn;
        spawn.spawnAtCursor(world.universe, world.ui, world.selection);

        ok = check(world.universe.count() == before_spawn + 1,
                   "moon can be spawned at the cursor") && ok;
        if (world.selection.selected_body_index >= 0 &&
            world.universe.isActive(world.selection.selected_body_index)) {
            const CelestialBody &moon = world.universe.body(world.selection.selected_body_index);
            double relative_vx = moon.vx - earth_vx;
            double relative_vy = moon.vy - earth_vy;
            double relative_vz = moon.vz - earth_vz;
            double relative_speed = std::sqrt(relative_vx * relative_vx +
                                              relative_vy * relative_vy +
                                              relative_vz * relative_vz);
            ok = check(moon.type == BodyType::Moon, "spawned body is a moon") && ok;
            ok = check(relative_speed > 0.0 && relative_speed < 0.10,
                       "spawned moon has a physical orbital velocity") && ok;
            ok = check(world.universe.moon(world.selection.selected_body_index).parent_index == earth_index,
                       "spawned moon is parented to Earth") && ok;
        } else {
            ok = check(false, "spawned moon is selected") && ok;
        }
    }

    world.reset();
    int m1 = world.universe.addBody(BodyType::Planet, "Merge A",
                                    0.90, 0.0, 0.90, 1.0, 0.0, -0.5, 2.0, 0.02);
    int m2 = world.universe.addBody(BodyType::Asteroid, "Merge B",
                                    0.900001, 0.0, 0.90, -2.0, 1.0, 0.5, 1.0, 0.01);
    if (m1 >= 0 && m2 >= 0) {
        double expected_mass = world.universe.body(m1).mass + world.universe.body(m2).mass;
        double expected_vx = (world.universe.body(m1).mass * world.universe.body(m1).vx +
                              world.universe.body(m2).mass * world.universe.body(m2).vx) / expected_mass;
        double expected_vy = (world.universe.body(m1).mass * world.universe.body(m1).vy +
                              world.universe.body(m2).mass * world.universe.body(m2).vy) / expected_mass;
        double expected_vz = (world.universe.body(m1).mass * world.universe.body(m1).vz +
                              world.universe.body(m2).mass * world.universe.body(m2).vz) / expected_mass;
        double expected_volume = world.universe.body(m1).volume_ratio +
                                 world.universe.body(m2).volume_ratio;

        world.simulation.collision_mode = CollisionMode::Classic;
        world.collisions.resolve(world.universe, world.ui, world.selection,
                                 world.simulation.collision_mode,
                                 world.simulation.physical_collisions);

        ok = check(std::fabs(world.universe.body(m1).mass - expected_mass) < 1.0e-9,
                   "merge conserves mass") && ok;
        ok = check(std::fabs(world.universe.body(m1).vx - expected_vx) < 1.0e-9 &&
                   std::fabs(world.universe.body(m1).vy - expected_vy) < 1.0e-9 &&
                   std::fabs(world.universe.body(m1).vz - expected_vz) < 1.0e-9,
                   "merge conserves 3D momentum") && ok;
        ok = check(std::fabs(world.universe.body(m1).volume_ratio - expected_volume) < 1.0e-9,
                   "merge adds volume ratios") && ok;
        ok = check(!world.universe.isActive(m2), "absorbed body is inactive") && ok;
    } else {
        ok = check(false, "merge test bodies could be created") && ok;
    }

    world.reset();
    world.simulation.collision_mode = CollisionMode::Realistic;
    int frag_a = world.universe.addBody(BodyType::Planet, "Frag A",
                                        0.90, 0.0, 0.90,
                                        1.5, 0.0, 0.0,
                                        3.003e-6, 0.020);
    int frag_b = world.universe.addBody(BodyType::Planet, "Frag B",
                                        0.90, 0.0, 0.90,
                                        -1.5, 0.0, 0.0,
                                        3.003e-6, 0.020);
    if (frag_a >= 0 && frag_b >= 0) {
        int before = world.universe.count();
        world.collisions.resolve(world.universe, world.ui, world.selection,
                                 CollisionMode::Realistic, world.simulation.physical_collisions);
        ok = check(world.universe.count() > before,
                   "high-energy impact produces debris fragments") && ok;
    } else {
        ok = check(false, "fragmentation test bodies could be created") && ok;
    }

    world.reset();
    world.simulation.physical_collisions = false;
    int coorbital_a = world.universe.addBody(BodyType::Planet, "Co-orbital A",
                                              0.90, 0.0, 0.90,
                                              1.0, 0.0, 0.0,
                                              3.003e-6, 0.020);
    int coorbital_b = world.universe.addBody(BodyType::Planet, "Co-orbital B",
                                              0.938, 0.0, 0.90,
                                              -1.0, 0.0, 0.0,
                                              3.003e-6, 0.020);
    if (coorbital_a >= 0 && coorbital_b >= 0) {
        int before = world.universe.count();
        world.physics.integrate(world.universe, BASE_DT, world.ui);
        world.collisions.resolve(world.universe, world.ui, world.selection,
                                 CollisionMode::Realistic,
                                 world.simulation.physical_collisions);
        ok = check(world.universe.count() != before ||
                   !world.universe.isActive(coorbital_b),
                   "co-orbital retrograde planets collide instead of passing through") && ok;
    } else {
        ok = check(false, "co-orbital collision test bodies could be created") && ok;
    }

    world.reset();
    int physical_a = world.universe.addBody(BodyType::Planet, "Physical A",
                                             0.90, 0.0, 0.90,
                                             0.0, 0.0, 0.0,
                                             3.003e-6, 0.020);
    int physical_b = world.universe.addBody(BodyType::Planet, "Physical B",
                                             0.920, 0.0, 0.90,
                                             0.0, 0.0, 0.0,
                                             3.003e-6, 0.020);
    if (physical_a >= 0 && physical_b >= 0) {
        int before = world.universe.count();
        world.simulation.physical_collisions = true;
        world.collisions.resolve(world.universe, world.ui, world.selection,
                                 CollisionMode::Realistic, true);
        ok = check(world.universe.count() == before,
                   "physical-only mode preserves pass-through") && ok;
    } else {
        ok = check(false, "physical collision test bodies could be created") && ok;
    }

    world.reset();
    int star_merge = world.universe.addBody(BodyType::Star, "Star Merge",
                                            0.90, 0.0, 0.90,
                                            0.0, 0.0, 0.0,
                                            0.5, 0.050);
    int star_snack = world.universe.addBody(BodyType::Asteroid, "Star Snack",
                                             0.90, 0.0, 0.90,
                                             0.0, 0.0, 0.0,
                                             1.0e-9, 0.004);
    if (star_merge >= 0 && star_snack >= 0) {
        world.simulation.physical_collisions = false;
        world.collisions.resolve(world.universe, world.ui, world.selection,
                                 CollisionMode::Realistic, false);
        ok = check(world.universe.body(star_merge).type == BodyType::Star &&
                   !world.universe.isActive(star_snack),
                   "stars merge instead of fragmenting") && ok;
    } else {
        ok = check(false, "star merge test bodies could be created") && ok;
    }

    world.reset();
    int collapse_star = world.universe.addBody(BodyType::Star, "Collapse Star",
                                               0.90, 0.0, 0.90,
                                               0.0, 0.0, 0.0,
                                               3.0, 0.050);
    int collapse_trigger = world.universe.addBody(BodyType::Asteroid, "Collapse Trigger",
                                                   0.90, 0.0, 0.90,
                                                   0.0, 0.0, 0.0,
                                                   1.0e-9, 0.004);
    if (collapse_star >= 0 && collapse_trigger >= 0) {
        world.simulation.collision_mode = CollisionMode::Classic;
        world.collisions.resolve(world.universe, world.ui, world.selection,
                                 CollisionMode::Classic, world.simulation.physical_collisions);
        ok = check(world.universe.body(collapse_star).type == BodyType::BlackHole,
                   "massive star collapses into a black hole") && ok;
        ok = check(world.universe.body(collapse_star).horizon_radius > 0.0 &&
                   world.universe.body(collapse_star).temperature_k == 0.0,
                   "collapsed black hole has a horizon and no thermal surface") && ok;
    } else {
        ok = check(false, "collapse test bodies could be created") && ok;
    }

    world.reset();
    int test_bh = world.universe.addBody(BodyType::BlackHole, "Test Black Hole",
                                          0.90, 0.0, 0.90,
                                          0.0, 0.0, 0.0,
                                          2.0, 0.020);
    int bh_snack = world.universe.addBody(BodyType::Asteroid, "BH Snack",
                                           0.90, 0.0, 0.90,
                                           0.0, 0.0, 0.0,
                                           1.0e-9, 0.004);
    if (test_bh >= 0 && bh_snack >= 0) {
        double mass_before = world.universe.body(test_bh).mass;
        double horizon_before = world.universe.body(test_bh).horizon_radius;
        world.simulation.collision_mode = CollisionMode::Classic;
        world.collisions.resolve(world.universe, world.ui, world.selection,
                                 CollisionMode::Classic, world.simulation.physical_collisions);
        const CelestialBody &bh = world.universe.body(test_bh);
        ok = check(bh.mass > mass_before &&
                   bh.horizon_radius >= horizon_before &&
                   bh.type == BodyType::BlackHole,
                   "black hole absorbs matter and grows its horizon") && ok;
    } else {
        ok = check(false, "black hole absorption test bodies could be created") && ok;
    }

    world.reset();
    DustField dust;
    int dust_active = 0;
    for (int i = 0; i < dust.count(); ++i) {
        if (dust.particles()[i].active) ++dust_active;
    }
    dust.update(world.universe, world.ui, 0.01);
    int dust_after = 0;
    for (int i = 0; i < dust.count(); ++i) {
        if (dust.particles()[i].active) ++dust_after;
    }
    ok = check(dust_active > 1000 && dust_after <= dust_active,
               "dust field initializes and updates") && ok;

    UiState fx_ui;
    FxEvent fx_event;
    fx_event.type = FxEventType::Impact;
    fx_event.energy = 2.0;
    fx_ui.fx.push(fx_event);
    Effects effects;
    effects.consume(fx_ui);
    ok = check(effects.particle_count() > 0 && effects.shockwave_count() > 0,
               "impact event produces particles and a shockwave") && ok;
    effects.update(0.1);
    ok = check(effects.particle_count() > 0,
               "effect particles survive a short update") && ok;

    world.reset();
    int comet_index = world.universe.addBody(BodyType::Comet, "Test Comet",
                                             0.40, 0.0, 0.0,
                                             0.0, 0.0, 0.35,
                                             1.0e-10, 0.004);
    if (comet_index >= 0) {
        Environment comet_environment;
        comet_environment.update(world.universe, 0.10);
        const CelestialBody &comet = world.universe.body(comet_index);
        ok = check(comet.comet_activity > 0.05,
                   "comet activity rises near the Sun") && ok;
        ok = check(comet.tail_length > 0.06,
                   "comet tail grows with activity") && ok;
        ok = check(comet.tail_dir[0] > 0.5,
                   "comet tail points anti-sunward") && ok;
    } else {
        ok = check(false, "comet test body could be created") && ok;
    }

    world.reset();
    int fill = 0;
    while (world.universe.count() < MAX_OBJECTS) {
        char fill_name[NAME_STR_LEN];
        ++fill;
        std::snprintf(fill_name, sizeof(fill_name), "Fill %d", fill);
        if (world.universe.addBody(BodyType::Asteroid, fill_name,
                                   0.90, 0.0, 0.90, 0.0, 0.0, 0.0,
                                   1.0e-9, 0.004) < 0) {
            break;
        }
    }
    ok = check(world.universe.count() == MAX_OBJECTS, "pool fills to 256 bodies") && ok;
    ok = check(world.universe.addBody(BodyType::Asteroid, "Overflow",
                                      0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                      1.0e-9, 0.004) < 0,
               "adding to a full pool is rejected") && ok;


    std::printf("%s\n", ok ? "SELF-TEST PASSED" : "SELF-TEST FAILED");
    return ok ? 0 : 1;
}

} // namespace spacesim
