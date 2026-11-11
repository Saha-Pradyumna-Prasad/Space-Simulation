#include "core/physics_metrics.h"

#include "core/config.h"

#include <algorithm>
#include <cmath>

namespace spacesim {

namespace {

double vec_length(double x, double y, double z) {
    return std::sqrt(x * x + y * y + z * z);
}

} // namespace

double effective_collision_radius(const CelestialBody &body,
                                  bool physical_collisions) {
    if (physical_collisions) return body.collision_radius;
    // Small bodies and moons use the physical scale: their rendered positions
    // are intentionally exaggerated, so visual-scale collisions would create
    // false impacts with their parent or neighbours.
    if (body.radius < 0.008) return body.collision_radius;
    return std::max(body.collision_radius,
                    body.radius * body.collision_scale);
}

PairMetrics compute_pair_metrics(const CelestialBody &a, const CelestialBody &b) {
    PairMetrics result;
    double dx = b.x - a.x;
    double dy = b.y - a.y;
    double dz = b.z - a.z;
    double distance = vec_length(dx, dy, dz);
    if (distance < 1.0e-12) distance = 1.0e-12;
    result.distance = distance;
    result.direction[0] = dx / distance;
    result.direction[1] = dy / distance;
    result.direction[2] = dz / distance;

    double softened = distance + SOFTENING;
    result.force = G_CONSTANT * a.mass * b.mass /
                   (softened * softened);
    result.acceleration_a = G_CONSTANT * b.mass /
                            (softened * softened);
    result.acceleration_b = G_CONSTANT * a.mass /
                            (softened * softened);
    result.potential = -G_CONSTANT * a.mass * b.mass / softened;
    result.escape_velocity = std::sqrt(
        2.0 * G_CONSTANT * (a.mass + b.mass) / softened);

    double dvx = b.vx - a.vx;
    double dvy = b.vy - a.vy;
    double dvz = b.vz - a.vz;
    result.relative_speed = vec_length(dvx, dvy, dvz);
    return result;
}

OrbitMetrics compute_orbit_metrics(const CelestialBody &body,
                                   const CelestialBody &primary) {
    OrbitMetrics result;
    double rx = body.x - primary.x;
    double ry = body.y - primary.y;
    double rz = body.z - primary.z;
    double vx = body.vx - primary.vx;
    double vy = body.vy - primary.vy;
    double vz = body.vz - primary.vz;

    double r = vec_length(rx, ry, rz);
    double v2 = vx * vx + vy * vy + vz * vz;
    double mu = G_CONSTANT * (body.mass + primary.mass);
    if (r < 1.0e-12 || mu <= 0.0) return result;

    double hx = ry * vz - rz * vy;
    double hy = rz * vx - rx * vz;
    double hz = rx * vy - ry * vx;
    double h = vec_length(hx, hy, hz);
    double energy = 0.5 * v2 - mu / r;

    double ex = (vy * hz - vz * hy) / mu - rx / r;
    double ey = (vz * hx - vx * hz) / mu - ry / r;
    double ez = (vx * hy - vy * hx) / mu - rz / r;
    double eccentricity = vec_length(ex, ey, ez);

    result.valid = true;
    result.specific_energy = energy;
    result.angular_momentum = h;
    result.eccentricity = eccentricity;
    if (energy < 0.0) {
        double a = -mu / (2.0 * energy);
        result.semi_major_axis = a;
        result.periapsis = a * (1.0 - eccentricity);
        result.apoapsis = a * (1.0 + eccentricity);
        result.period = 2.0 * PI * std::sqrt(a * a * a / mu);
    }
    return result;
}

BodyMetrics compute_body_metrics(const CelestialBody &body) {
    BodyMetrics result;
    double radius = body.radius > 0.0 ? body.radius : body.collision_radius;
    if (radius <= 0.0) return result;
    result.density = body.density_kg_m3;
    result.surface_gravity = G_CONSTANT * body.mass / (radius * radius);
    result.escape_velocity = std::sqrt(2.0 * G_CONSTANT * body.mass / radius);
    return result;
}

TidalMetrics compute_tidal_metrics(const CelestialBody &body,
                                   const CelestialBody &primary,
                                   double star_mass,
                                   double body_orbit_radius) {
    TidalMetrics result;
    double dx = primary.x - body.x;
    double dy = primary.y - body.y;
    double dz = primary.z - body.z;
    double distance = vec_length(dx, dy, dz);
    if (distance < 1.0e-12) return result;
    double radius = body.collision_radius > 0.0 ? body.collision_radius : body.radius;
    result.acceleration = 2.0 * G_CONSTANT * primary.mass * radius /
                          (distance * distance * distance);
    double surface_gravity = G_CONSTANT * body.mass / (radius * radius);
    if (surface_gravity > 0.0) result.ratio = result.acceleration / surface_gravity;
    result.roche_limit = primary.radius *
        std::cbrt(std::max(0.001, 2.0 * primary.density_kg_m3 /
                            std::max(1.0, body.density_kg_m3)));
    if (star_mass > 0.0 && body_orbit_radius > 0.0) {
        result.hill_radius = body_orbit_radius *
            std::cbrt(body.mass / (3.0 * star_mass));
    }
    return result;
}

BlackHoleMetrics compute_black_hole_metrics(const CelestialBody &body) {
    BlackHoleMetrics result;
    result.horizon = body.horizon_radius > 0.0
        ? body.horizon_radius : body.radius;
    result.photon_sphere = result.horizon * 1.5;
    result.isco = result.horizon * 3.0;
    double r = result.horizon * 2.0;
    if (r > 0.0 && result.horizon < r) {
        result.time_dilation = std::sqrt(1.0 - result.horizon / r);
    }
    return result;
}

} // namespace spacesim
