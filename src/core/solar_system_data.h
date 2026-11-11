#ifndef SPACESIM_CORE_SOLAR_SYSTEM_DATA_H
#define SPACESIM_CORE_SOLAR_SYSTEM_DATA_H

#include "core/body.h"

namespace spacesim {

struct PlanetSeed {
    const char *name;
    double orbit_radius;
    double visual_radius;
    double real_radius_km;
    double real_orbit_radius_km;
    double mass_earth;
    double volume_earth;
    double rotation_hours;
    double period_days;
    double start_angle;
    float color[3];
    double density_kg_m3;
    double albedo;
    double greenhouse_k;
    double axial_tilt_deg;
    double flattening;
    SurfaceKind surface_kind;
    float atmosphere_color[3];
    double cloud_amount;
    double procedural_seed;
    const char *history;
};

struct AsteroidSeed {
    const char *name;
    double orbit_radius;
    double start_angle;
    double mass_earth;
    double visual_radius;
    double real_radius_km;
    float color[3];
    const char *history;
};

struct MoonSeed {
    const char *name;
    const char *parent;
    double visual_radius_scale;
    double inclination_deg;
    int direction;
    double start_angle_deg;
    double real_radius_km;
    double real_orbit_radius_km;
    double real_period_days;
    double real_mass_earth;
    double real_volume_earth;
    double rotation_hours;
    float color[3];
    double density_kg_m3;
    double albedo;
    double greenhouse_k;
    const char *history;
};

extern const PlanetSeed PLANET_SEEDS[8];
extern const AsteroidSeed ASTEROID_SEEDS[4];
extern const MoonSeed MOON_SEEDS[13];

} // namespace spacesim

#endif
