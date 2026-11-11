#include "core/universe.h"

#include "core/math_utils.h"
#include "core/solar_system_data.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace spacesim {

Universe::Universe() : active_count_(0) {
    reset();
}

void Universe::reset() {
    bodies_.fill(CelestialBody{});
    moons_.fill(MoonOrbit{});
    prev_x_.fill(0.0);
    prev_y_.fill(0.0);
    prev_z_.fill(0.0);
    active_count_ = 0;
}

int Universe::count() const {
    return active_count_;
}

bool Universe::isActive(int index) const {
    if (index < 0 || index >= MAX_OBJECTS) return false;
    return bodies_[index].active;
}

CelestialBody &Universe::body(int index) {
    return bodies_[index];
}

const CelestialBody &Universe::body(int index) const {
    return bodies_[index];
}

MoonOrbit &Universe::moon(int index) {
    return moons_[index];
}

const MoonOrbit &Universe::moon(int index) const {
    return moons_[index];
}

std::array<CelestialBody, MAX_OBJECTS> &Universe::bodies() {
    return bodies_;
}

const std::array<CelestialBody, MAX_OBJECTS> &Universe::bodies() const {
    return bodies_;
}

std::array<MoonOrbit, MAX_OBJECTS> &Universe::moons() {
    return moons_;
}

const std::array<MoonOrbit, MAX_OBJECTS> &Universe::moons() const {
    return moons_;
}

int Universe::findFreeSlot() const {
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!bodies_[i].active) return i;
    }
    return -1;
}

int Universe::addBody(BodyType type, const char *name,
                      double x, double y, double z,
                      double vx, double vy, double vz,
                      double mass, double radius) {
    int slot = findFreeSlot();
    if (slot < 0) return -1;

    const TypeDefaults &defaults = type_defaults(type);
    if (mass <= 0.0) mass = defaults.mass;
    if (radius <= 0.0) radius = defaults.radius;

    CelestialBody body;
    body.type = type;
    body.active = true;
    body.x = x;
    body.y = y;
    body.z = z;
    body.vx = vx;
    body.vy = vy;
    body.vz = vz;
    body.mass = mass;
    body.radius = radius;
    body.collision_radius = radius * 0.001;
    if (type == BodyType::BlackHole) {
        body.horizon_radius = clamp_double(
            0.009 + 0.006 * std::cbrt(body.mass / 2.0), 0.007, 0.035);
        body.radius = body.horizon_radius;
        body.collision_radius = body.horizon_radius * 0.55;
        body.axial_tilt_deg = 23.0;
        body.spin_rate = 2.5;
        body.color[0] = 0.005f;
        body.color[1] = 0.003f;
        body.color[2] = 0.010f;
    }
    copy_name(body.name, (name != nullptr ? name : type_name(type)));
    body.density_kg_m3 = defaults.density_kg_m3;
    sync_body_volume(body);
    body.albedo = defaults.albedo;
    body.greenhouse_k = defaults.greenhouse_k;
    body.temperature_k = (type == BodyType::Star)
        ? 2500.0 + 3200.0 * std::sqrt(body.mass)
        : 3.0;
    switch (type) {
        case BodyType::Star:
            body.surface_kind = SurfaceKind::Sun;
            body.composition = Composition::Stellar;
            break;
        case BodyType::Planet:
            body.surface_kind = SurfaceKind::Rocky;
            body.composition = Composition::Rocky;
            break;
        case BodyType::Moon:
            body.surface_kind = SurfaceKind::Icy;
            body.composition = Composition::Icy;
            break;
        case BodyType::Asteroid:
        case BodyType::Meteor:
            body.surface_kind = SurfaceKind::Rocky;
            body.composition = Composition::Rocky;
            break;
        case BodyType::Comet:
            body.surface_kind = SurfaceKind::Comet;
            body.composition = Composition::Icy;
            break;
        case BodyType::BlackHole:
            body.surface_kind = SurfaceKind::BlackHole;
            body.composition = Composition::Exotic;
            break;
        default: break;
    }
    body.procedural_seed = static_cast<double>(slot) * 1.37 + 1.0;
    body.spin_rate = (body.self_rotation_time != 0.0)
        ? 360.0 / body.self_rotation_time : 0.35;
    body.thermal_inertia = (type == BodyType::Star || type == BodyType::BlackHole)
        ? 0.0 : 1.5;
    body.internal_heat_k = (type == BodyType::Planet) ? 5.0 : 0.0;
    copy_history(body.history, default_history(type));
    for (int c = 0; c < 3; ++c) body.color[c] = defaults.color[c];

    bodies_[slot] = body;
    moons_[slot] = MoonOrbit{};
    prev_x_[slot] = x;
    prev_y_[slot] = y;
    prev_z_[slot] = z;
    ++active_count_;
    return slot;
}

bool Universe::removeBody(int index) {
    if (index < 0 || index >= MAX_OBJECTS) return false;
    if (!bodies_[index].active) return false;

    bodies_[index] = CelestialBody{};
    moons_[index] = MoonOrbit{};
    prev_x_[index] = 0.0;
    prev_y_[index] = 0.0;
    prev_z_[index] = 0.0;
    --active_count_;
    if (active_count_ < 0) active_count_ = 0;

    detachMoonChildren(index);
    return true;
}

void Universe::detachMoonChildren(int parent_index) {
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!bodies_[i].active) continue;
        if (moons_[i].parent_index == parent_index) {
            moons_[i] = MoonOrbit{};
        }
    }
}

int Universe::findByName(const char *name) const {
    if (name == nullptr) return -1;
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (bodies_[i].active && std::strcmp(bodies_[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

int Universe::heaviestBody() const {
    int best = -1;
    double best_mass = -1.0;
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!bodies_[i].active) continue;
        if (bodies_[i].mass > best_mass) {
            best_mass = bodies_[i].mass;
            best = i;
        }
    }
    return best;
}

int Universe::nearestStarIndex(int body_index) const {
    if (body_index < 0 || body_index >= MAX_OBJECTS) return -1;
    if (!bodies_[body_index].active) return -1;

    const CelestialBody &body = bodies_[body_index];
    int best = -1;
    double best_distance = 1.0e30;
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!bodies_[i].active || i == body_index) continue;
        if (bodies_[i].type != BodyType::Star) continue;
        double dx = bodies_[i].x - body.x;
        double dy = bodies_[i].y - body.y;
        double dz = bodies_[i].z - body.z;
        double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (distance < best_distance) {
            best_distance = distance;
            best = i;
        }
    }
    return best;
}

double Universe::distanceToNearestStar(int body_index,
                                       double &star_x, double &star_y, double &star_z) const {
    star_x = 0.0;
    star_y = 0.0;
    star_z = 0.0;
    int star = nearestStarIndex(body_index);
    if (star < 0) return -1.0;
    const CelestialBody &s = bodies_[star];
    const CelestialBody &b = bodies_[body_index];
    star_x = s.x;
    star_y = s.y;
    star_z = s.z;
    double dx = b.x - s.x;
    double dy = b.y - s.y;
    double dz = b.z - s.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

void Universe::savePreviousPositions() {
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        prev_x_[i] = bodies_[i].x;
        prev_y_[i] = bodies_[i].y;
        prev_z_[i] = bodies_[i].z;
    }
}

double Universe::prevX(int index) const {
    return prev_x_[index];
}

double Universe::prevY(int index) const {
    return prev_y_[index];
}

double Universe::prevZ(int index) const {
    return prev_z_[index];
}

void Universe::seedSolarSystem() {
    const double sun_mass = 1.0;
    const double sun_radius = 0.060;

    int sun = addBody(BodyType::Star, "Sun",
                      0.0, 0.0, 0.0,
                      0.0, 0.0, 0.0,
                      sun_mass, sun_radius);
    if (sun >= 0) {
        CelestialBody &b = bodies_[sun];
        b.volume_ratio = 1300000.0;
        b.self_rotation_time = 609.12;
        b.orbital_period = 0.0;
        b.density_kg_m3 = 1408.0;
        b.albedo = 0.0;
        b.greenhouse_k = 0.0;
        b.temperature_k = 5778.0;
        b.surface_kind = SurfaceKind::Sun;
        b.axial_tilt_deg = 7.25;
        b.procedural_seed = 0.0;
        b.atmosphere_color[0] = 1.0f;
        b.atmosphere_color[1] = 0.80f;
        b.atmosphere_color[2] = 0.40f;
        b.spin_rate = 600.0 / 609.12;
        b.collision_radius = 0.38 * (696340.0 / 149.6e6);
        copy_history(b.history,
                     "The Sun contains 99.86 percent of the mass of the solar system. "
                     "Its gravity holds every planet on its orbit. The core fuses about "
                     "600 million tonnes of hydrogen into helium every second, and light "
                     "takes about eight minutes to reach Earth. The visible surface is "
                     "roughly 5778 K, while the corona is far hotter; the Sun rotates "
                     "once every 609 hours at this simulation's scale.");
    }

    for (int i = 0; i < 8; ++i) {
        const PlanetSeed &seed = PLANET_SEEDS[i];
        double angle = seed.start_angle;
        double x = seed.orbit_radius * std::cos(angle);
        double z = seed.orbit_radius * std::sin(angle);
        double speed = std::sqrt(G_CONSTANT * sun_mass * seed.orbit_radius) /
                       (seed.orbit_radius + SOFTENING);
        double vx = -speed * std::sin(angle);
        double vz = speed * std::cos(angle);

        double mass = seed.mass_earth * EARTH_MASS_SIM;
        int slot = addBody(BodyType::Planet, seed.name,
                           x, 0.0, z,
                           vx, 0.0, vz,
                           mass, seed.visual_radius);
        if (slot < 0) continue;

        CelestialBody &b = bodies_[slot];
        b.volume_ratio = seed.volume_earth;
        b.self_rotation_time = seed.rotation_hours;
        b.orbital_period = seed.period_days;
        b.density_kg_m3 = seed.density_kg_m3;
        b.albedo = seed.albedo;
        b.greenhouse_k = seed.greenhouse_k;
        b.axial_tilt_deg = seed.axial_tilt_deg;
        b.flattening = seed.flattening;
        b.surface_kind = seed.surface_kind;
        if (seed.surface_kind == SurfaceKind::GasGiant) b.composition = Composition::GasGiant;
        else if (seed.surface_kind == SurfaceKind::IceGiant) b.composition = Composition::Icy;
        else if (seed.surface_kind == SurfaceKind::Earth) b.composition = Composition::Rocky;
        b.cloud_amount = seed.cloud_amount;
        b.procedural_seed = seed.procedural_seed;
        for (int c = 0; c < 3; ++c) b.atmosphere_color[c] = seed.atmosphere_color[c];
        b.spin_rate = (seed.rotation_hours < 0.0 ? -1.0 : 1.0) *
                      600.0 / std::max(1.0, std::fabs(seed.rotation_hours));
        b.collision_radius = seed.orbit_radius *
                             seed.real_radius_km / seed.real_orbit_radius_km;
        for (int c = 0; c < 3; ++c) b.color[c] = seed.color[c];
        copy_history(b.history, seed.history);
    }

    for (int i = 0; i < 4; ++i) {
        const AsteroidSeed &seed = ASTEROID_SEEDS[i];
        double angle = seed.start_angle;
        double x = seed.orbit_radius * std::cos(angle);
        double z = seed.orbit_radius * std::sin(angle);
        double speed = std::sqrt(G_CONSTANT * sun_mass * seed.orbit_radius) /
                       (seed.orbit_radius + SOFTENING);
        double vx = -speed * std::sin(angle);
        double vz = speed * std::cos(angle);
        double mass = seed.mass_earth * EARTH_MASS_SIM;
        int slot = addBody(BodyType::Asteroid, seed.name,
                           x, 0.0, z, vx, 0.0, vz,
                           mass, seed.visual_radius);
        if (slot < 0) continue;
        CelestialBody &body = bodies_[slot];
        body.density_kg_m3 = 3000.0;
        body.albedo = 0.10;
        body.surface_kind = SurfaceKind::Rocky;
        body.composition = Composition::Rocky;
        body.procedural_seed = 20.0 + static_cast<double>(i);
        body.spin_rate = 0.8 + 0.2 * static_cast<double>(i);
        body.self_rotation_time = 9.0 + 2.0 * static_cast<double>(i);
        for (int c = 0; c < 3; ++c) body.color[c] = seed.color[c];
        copy_history(body.history, seed.history);
    }

    if (sun >= 0 && bodies_[sun].mass > 0.0) {
        double total_px = 0.0;
        double total_py = 0.0;
        double total_pz = 0.0;
        for (int i = 0; i < MAX_OBJECTS; ++i) {
            if (!bodies_[i].active || i == sun) continue;
            total_px += bodies_[i].mass * bodies_[i].vx;
            total_py += bodies_[i].mass * bodies_[i].vy;
            total_pz += bodies_[i].mass * bodies_[i].vz;
        }
        bodies_[sun].vx = -total_px / bodies_[sun].mass;
        bodies_[sun].vy = -total_py / bodies_[sun].mass;
        bodies_[sun].vz = -total_pz / bodies_[sun].mass;
    }
}

void Universe::seedMoons() {
    int sun = findByName("Sun");
    double sun_mass = (sun >= 0 ? bodies_[sun].mass : 1.0);
    if (sun_mass <= 0.0) sun_mass = 1.0;

    for (int i = 0; i < 13; ++i) {
        const MoonSeed &seed = MOON_SEEDS[i];
        int parent = findByName(seed.parent);
        if (parent < 0 || !bodies_[parent].active) continue;

        const CelestialBody &p = bodies_[parent];
        double parent_orbit = 0.0;
        if (sun >= 0) {
            double sx = p.x - bodies_[sun].x;
            double sy = p.y - bodies_[sun].y;
            double sz = p.z - bodies_[sun].z;
            parent_orbit = std::sqrt(sx * sx + sy * sy + sz * sz);
        } else {
            parent_orbit = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
        }
        if (parent_orbit < 1.0e-6) parent_orbit = 0.10;

        double hill = parent_orbit * std::cbrt(p.mass / (3.0 * sun_mass));
        if (hill < 1.0e-6) hill = parent_orbit * 0.01;

        double physical_fraction = 0.15 +
            0.25 * (seed.visual_radius_scale - 1.5) / 1.3;
        physical_fraction = clamp_double(physical_fraction, 0.20, 0.40);
        double physical_orbit = hill * physical_fraction;
        if (physical_orbit < 1.0e-6) physical_orbit = 1.0e-4;

        double visual_orbit = p.radius * seed.visual_radius_scale;
        double visual_scale = visual_orbit / physical_orbit;

        double angle = seed.start_angle_deg * PI / 180.0;
        double inclination = seed.inclination_deg * PI / 180.0;
        double direction = static_cast<double>(seed.direction);

        double x_rel = physical_orbit * std::cos(angle);
        double z0 = physical_orbit * std::sin(angle);
        double y_rel = z0 * std::sin(inclination);
        double z_rel = z0 * std::cos(inclination);

        double physical_speed = std::sqrt(G_CONSTANT * p.mass * physical_orbit) /
                                (physical_orbit + MOON_SOFTENING);
        double vx_rel = -physical_speed * std::sin(angle) * direction;
        double vz0 = physical_speed * std::cos(angle) * direction;
        double vy_rel = vz0 * std::sin(inclination);
        double vz_rel = vz0 * std::cos(inclination);

        double body_radius = (seed.real_radius_km / 6371.0) * EARTH_VISUAL_RADIUS;
        if (body_radius < MIN_MOON_VISUAL_RADIUS) {
            body_radius = MIN_MOON_VISUAL_RADIUS;
        }
        double mass = seed.real_mass_earth * EARTH_MASS_SIM;

        int slot = addBody(BodyType::Moon, seed.name,
                           p.x + x_rel, p.y + y_rel, p.z + z_rel,
                           p.vx + vx_rel, p.vy + vy_rel, p.vz + vz_rel,
                           mass, body_radius);
        if (slot < 0) continue;

        CelestialBody &m = bodies_[slot];
        m.volume_ratio = seed.real_volume_earth;
        m.self_rotation_time = seed.rotation_hours;
        m.orbital_period = seed.real_period_days;
        m.density_kg_m3 = seed.density_kg_m3;
        m.albedo = seed.albedo;
        m.greenhouse_k = seed.greenhouse_k;
        m.surface_kind = SurfaceKind::Icy;
        m.composition = Composition::Icy;
        m.procedural_seed = 30.0 + static_cast<double>(i);
        m.spin_rate = (seed.direction < 0 ? -1.0 : 1.0) *
                      30.0 / std::max(1.0, seed.real_period_days);
        m.tidal_lock = true;
        m.thermal_inertia = 2.0;
        m.collision_radius = physical_orbit *
                             seed.real_radius_km / seed.real_orbit_radius_km;
        for (int c = 0; c < 3; ++c) m.color[c] = seed.color[c];
        copy_history(m.history, seed.history);

        MoonOrbit &orbit = moons_[slot];
        orbit.parent_index = parent;
        orbit.physical_radius = physical_orbit;
        orbit.visual_radius = visual_orbit;
        orbit.visual_scale = visual_scale;
        orbit.inclination = inclination;
        orbit.real_orbit_radius_km = seed.real_orbit_radius_km;
        orbit.visual_x = p.x + x_rel * visual_scale;
        orbit.visual_y = p.y + y_rel * visual_scale;
        orbit.visual_z = p.z + z_rel * visual_scale;
    }
}

void Universe::printBodies() const {
    std::printf("Active bodies: %d\n", active_count_);
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!bodies_[i].active) continue;
        const CelestialBody &b = bodies_[i];
        std::printf("  [%3d] %-14s %-10s x=%7.4f y=%7.4f z=%7.4f mass=%10.4g r=%7.4f\n",
                    i, b.name, type_name(b.type), b.x, b.y, b.z, b.mass, b.radius);
    }
}

} // namespace spacesim
