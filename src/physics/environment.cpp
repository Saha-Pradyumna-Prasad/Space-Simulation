#include "physics/environment.h"

#include "core/config.h"
#include "core/math_utils.h"
#include "core/universe.h"

#include <algorithm>
#include <cmath>

namespace spacesim {

namespace {

double star_luminosity(const CelestialBody &body) {
    if (body.type != BodyType::Star || body.mass <= 0.0) return 0.0;
    return std::pow(body.mass, 3.5);
}

double star_temperature(const CelestialBody &body) {
    if (body.type != BodyType::Star) return 0.0;
    return clamp_double(2500.0 + 3200.0 * std::sqrt(body.mass), 2400.0, 40000.0);
}

double distance_between(const CelestialBody &a, const CelestialBody &b) {
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    double dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

} // namespace

void Environment::update(Universe &universe, double dt) {
    if (dt < 0.0) dt = 0.0;

    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe.isActive(i)) continue;
        CelestialBody &body = universe.body(i);

        body.collapse_flash *= std::exp(-dt * 2.8);
        body.impact_heating_k *= std::exp(-dt / 9.0);
        if (body.collision_cooldown > 0.0) {
            body.collision_cooldown -= dt;
            if (body.collision_cooldown < 0.0) body.collision_cooldown = 0.0;
        }
        if (body.impact_heating_k < 0.01) body.impact_heating_k = 0.0;

        if (body.type == BodyType::Star) {
            body.temperature_k = star_temperature(body);
            body.equilibrium_temperature_k = body.temperature_k;
            body.tidal_heating_k = 0.0;
            body.tidal_ratio = 0.0;
            body.comet_activity = 0.0;
            body.coma_radius = 0.0;
            body.tail_length = 0.0;
            body.spin_angle = std::fmod(body.spin_angle +
                                        body.spin_rate * dt * 8.0, 360.0);
            if (body.spin_angle < 0.0) body.spin_angle += 360.0;
            continue;
        }

        if (body.type == BodyType::BlackHole) {
            body.temperature_k = 0.0;
            body.equilibrium_temperature_k = 0.0;
            body.tidal_heating_k = 0.0;
            body.tidal_ratio = 0.0;
            body.comet_activity = 0.0;
            body.coma_radius = 0.0;
            body.tail_length = 0.0;
            body.spin_angle = std::fmod(body.spin_angle +
                                        body.spin_rate * dt * 8.0, 360.0);
            if (body.spin_angle < 0.0) body.spin_angle += 360.0;
            continue;
        }

        double flux = 0.0;
        double tail_x = 0.0;
        double tail_y = 0.0;
        double tail_z = 0.0;

        for (int s = 0; s < MAX_OBJECTS; ++s) {
            if (!universe.isActive(s) || s == i) continue;
            const CelestialBody &star = universe.body(s);
            double luminosity = star_luminosity(star);
            if (luminosity <= 0.0) continue;

            double dx = body.x - star.x;
            double dy = body.y - star.y;
            double dz = body.z - star.z;
            double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (distance < 1.0e-5) distance = 1.0e-5;

            double normalized = distance / EARTH_ORBIT_SIM;
            flux += luminosity / (normalized * normalized);

            double weight = luminosity / (distance * distance);
            tail_x += (dx / distance) * weight;
            tail_y += (dy / distance) * weight;
            tail_z += (dz / distance) * weight;
        }

        double absorbed = (1.0 - clamp_double(body.albedo, 0.0, 1.0)) / (1.0 - 0.30);
        double thermal_flux = flux * absorbed;
        if (thermal_flux < 0.0) thermal_flux = 0.0;
        body.equilibrium_temperature_k = 255.0 * std::pow(thermal_flux, 0.25);

        // Physical tidal ratio uses the compressed collision radius, which is
        // the scale at which collisions and orbital mechanics are consistent.
        int primary = -1;
        double best_tide = -1.0;
        double primary_distance = 0.0;
        for (int p = 0; p < MAX_OBJECTS; ++p) {
            if (!universe.isActive(p) || p == i) continue;
            const CelestialBody &candidate = universe.body(p);
            if (candidate.mass <= 0.0) continue;
            double distance = distance_between(body, candidate);
            if (distance < 1.0e-6) continue;
            double tide = candidate.mass / (distance * distance * distance);
            if (tide > best_tide) {
                best_tide = tide;
                primary = p;
                primary_distance = distance;
            }
        }

        body.tidal_ratio = 0.0;
        body.roche_limit = 0.0;
        body.tidal_heating_k = 0.0;
        body.tidal_axis[0] = 1.0;
        body.tidal_axis[1] = 0.0;
        body.tidal_axis[2] = 0.0;

        if (primary >= 0) {
            const CelestialBody &p = universe.body(primary);
            double rb = body.collision_radius > 0.0 ? body.collision_radius : body.radius;
            if (primary_distance > 1.0e-9 && body.mass > 0.0 && rb > 0.0) {
                double ratio = 2.0 * p.mass * rb * rb * rb /
                               (body.mass * primary_distance * primary_distance * primary_distance);
                body.tidal_ratio = clamp_double(ratio, 0.0, 10.0);

                double dx = p.x - body.x;
                double dy = p.y - body.y;
                double dz = p.z - body.z;
                double inv = 1.0 / primary_distance;
                body.tidal_axis[0] = dx * inv;
                body.tidal_axis[1] = dy * inv;
                body.tidal_axis[2] = dz * inv;

                double rp = p.collision_radius > 0.0 ? p.collision_radius : p.radius;
                double density_ratio = (body.density_kg_m3 > 0.0)
                    ? (2.0 * p.density_kg_m3 / body.density_kg_m3) : 2.0;
                body.roche_limit = rp * std::cbrt(std::max(0.01, density_ratio));

                // A deliberately amplified thermal response: the physical
                // ratio is tiny at solar-system scale, but the visual result
                // should distinguish Io-like moons from cold moons.
                body.tidal_heating_k = clamp_double(50000.0 * body.tidal_ratio,
                                                    0.0, 900.0);
            }

            // Use the net tidal vector for the rendered bulge, even though
            // the heating/ratio above is based on the dominant primary.
            double net_x = 0.0, net_y = 0.0, net_z = 0.0;
            double body_radius = body.collision_radius > 0.0
                ? body.collision_radius : body.radius;
            for (int q = 0; q < MAX_OBJECTS; ++q) {
                if (!universe.isActive(q) || q == i) continue;
                const CelestialBody &other = universe.body(q);
                if (other.mass <= 0.0) continue;
                double dx = other.x - body.x;
                double dy = other.y - body.y;
                double dz = other.z - body.z;
                double d = std::sqrt(dx * dx + dy * dy + dz * dz);
                if (d < 1.0e-9) continue;
                double a = 2.0 * G_CONSTANT * other.mass * body_radius /
                           (d * d * d);
                net_x += dx / d * a;
                net_y += dy / d * a;
                net_z += dz / d * a;
            }
            double net_length = std::sqrt(net_x * net_x + net_y * net_y +
                                          net_z * net_z);
            if (net_length > 1.0e-18) {
                body.tidal_axis[0] = net_x / net_length;
                body.tidal_axis[1] = net_y / net_length;
                body.tidal_axis[2] = net_z / net_length;
            }
        }

        double target_temperature = body.equilibrium_temperature_k +
                                    body.greenhouse_k +
                                    body.tidal_heating_k +
                                    body.impact_heating_k +
                                    body.internal_heat_k;
        if (target_temperature < 3.0) target_temperature = 3.0;
        if (body.temperature_k <= 3.0 || body.thermal_inertia <= 0.0 ||
            dt <= 0.0) {
            body.temperature_k = target_temperature;
        } else {
            double response = clamp_double(dt / body.thermal_inertia, 0.0, 1.0);
            body.temperature_k += (target_temperature - body.temperature_k) * response;
        }

        if (body.type == BodyType::Comet) {
            double activity = clamp_double((body.temperature_k - 160.0) / 260.0,
                                           0.0, 1.0);
            body.comet_activity = activity;
            body.coma_radius = body.radius * (1.5 + 3.5 * activity);
            body.tail_length = 0.06 + 0.55 * activity;
            if (tail_x * tail_x + tail_y * tail_y + tail_z * tail_z > 1.0e-18) {
                double inv = 1.0 / std::sqrt(tail_x * tail_x +
                                             tail_y * tail_y +
                                             tail_z * tail_z);
                body.tail_dir[0] = tail_x * inv;
                body.tail_dir[1] = tail_y * inv;
                body.tail_dir[2] = tail_z * inv;
            }
        } else {
            body.comet_activity = 0.0;
            body.coma_radius = 0.0;
            body.tail_length = 0.0;
        }

        if (body.tidal_lock && universe.moon(i).parent_index >= 0 &&
            body.orbital_period > 0.0) {
            body.spin_rate = 30.0 / body.orbital_period;
        }
        body.spin_angle = std::fmod(body.spin_angle +
                                    body.spin_rate * dt * 8.0, 360.0);
        if (body.spin_angle < 0.0) body.spin_angle += 360.0;
    }
}

} // namespace spacesim
