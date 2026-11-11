#include "core/math_utils.h"

#include <cmath>
#include <cstring>

namespace spacesim {

static unsigned int rng_state = 0x12345678u;

double clamp_double(double value, double low, double high) {
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

int clamp_int(int value, int low, int high) {
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

double volume_from_radius(double radius) {
    if (radius <= 0.0) return 0.0;
    double earth = EARTH_RADIUS_SIM;
    if (earth <= 0.0) return 0.0;
    return (radius * radius * radius) / (earth * earth * earth);
}

double earth_volume_from_mass_density(double mass, double density_kg_m3) {
    if (mass <= 0.0 || density_kg_m3 <= 0.0) return 0.0;
    // The seeded volume ratios satisfy this identity exactly:
    // volume(Earths) = mass(Earths) * EarthDensity / density.
    return (mass / EARTH_MASS_SIM) *
           (EARTH_DENSITY_KG_M3 / density_kg_m3);
}

void sync_body_volume(CelestialBody &body) {
    if (body.type == BodyType::BlackHole) {
        body.volume_ratio = 0.0;
        return;
    }
    body.volume_ratio = earth_volume_from_mass_density(body.mass,
                                                       body.density_kg_m3);
}

double random_unit() {
    rng_state = rng_state * 1664525u + 1013904223u;
    return static_cast<double>(rng_state) / 4294967296.0;
}

bool ray_sphere_hit(const double origin[3], const double direction[3],
                    const double center[3], double radius, double &hit_t) {
    double ox = origin[0] - center[0];
    double oy = origin[1] - center[1];
    double oz = origin[2] - center[2];
    double a = direction[0] * direction[0] +
               direction[1] * direction[1] +
               direction[2] * direction[2];
    if (a < 1.0e-18) return false;
    double b = 2.0 * (ox * direction[0] + oy * direction[1] + oz * direction[2]);
    double c = ox * ox + oy * oy + oz * oz - radius * radius;
    double discriminant = b * b - 4.0 * a * c;
    if (discriminant < 0.0) return false;
    double root = std::sqrt(discriminant);
    double t0 = (-b - root) / (2.0 * a);
    double t1 = (-b + root) / (2.0 * a);
    if (t0 > 0.0) {
        hit_t = t0;
        return true;
    }
    if (t1 > 0.0) {
        hit_t = t1;
        return true;
    }
    return false;
}

bool body_is_finite(const CelestialBody &body) {
    return std::isfinite(body.x) && std::isfinite(body.y) && std::isfinite(body.z) &&
           std::isfinite(body.vx) && std::isfinite(body.vy) && std::isfinite(body.vz) &&
           std::isfinite(body.ax) && std::isfinite(body.ay) && std::isfinite(body.az);
}

void copy_name(char *dst, const char *src) {
    int i = 0;
    if (src != nullptr) {
        for (; i < NAME_STR_LEN - 1 && src[i] != '\0'; ++i) {
            dst[i] = src[i];
        }
    }
    dst[i] = '\0';
}

void copy_history(char *dst, const char *src) {
    int i = 0;
    if (src != nullptr) {
        for (; i < HIST_STR_LEN - 1 && src[i] != '\0'; ++i) {
            dst[i] = src[i];
        }
    }
    dst[i] = '\0';
}

void copy_path(char *dst, int size, const char *src) {
    int i = 0;
    if (src != nullptr && size > 0) {
        for (; i < size - 1 && src[i] != '\0'; ++i) {
            dst[i] = src[i];
        }
    }
    if (size > 0) dst[i] = '\0';
}

} // namespace spacesim
