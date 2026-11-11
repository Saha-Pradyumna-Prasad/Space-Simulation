#ifndef SPACESIM_CORE_BODY_H
#define SPACESIM_CORE_BODY_H

#include "core/config.h"

namespace spacesim {

enum class BodyType {
    Star,
    Planet,
    Moon,
    Asteroid,
    Meteor,
    Comet,
    BlackHole,
    Count
};

enum class SurfaceKind {
    Rocky,
    Earth,
    GasGiant,
    IceGiant,
    Icy,
    Sun,
    Comet,
    BlackHole
};

enum class Composition {
    Rocky,
    Icy,
    Metallic,
    GasGiant,
    Stellar,
    Exotic
};

constexpr int BODY_TYPE_COUNT = static_cast<int>(BodyType::Count);

struct CelestialBody {
    BodyType type = BodyType::Star;
    bool active = false;

    double x = 0.0, y = 0.0, z = 0.0;
    double vx = 0.0, vy = 0.0, vz = 0.0;
    double ax = 0.0, ay = 0.0, az = 0.0;

    double mass = 0.0;
    double radius = 0.0;
    double collision_radius = 0.0;
    double collision_scale = 0.90;
    double collision_cooldown = 0.0;

    char name[NAME_STR_LEN] = {};

    double self_rotation_time = 0.0;
    double orbital_period = 0.0;
    double volume_ratio = 0.0;
    double density_kg_m3 = 1000.0;

    // Thermal model.
    double albedo = 0.30;
    double greenhouse_k = 0.0;
    double temperature_k = 0.0;
    double equilibrium_temperature_k = 0.0;
    double tidal_heating_k = 0.0;
    double impact_heating_k = 0.0;
    double internal_heat_k = 0.0;
    double thermal_inertia = 1.0;
    bool tidal_lock = false;

    // Tidal model.
    double tidal_ratio = 0.0;
    double tidal_axis[3] = {1.0, 0.0, 0.0};
    double roche_limit = 0.0;

    // Visual rotation and shape.
    SurfaceKind surface_kind = SurfaceKind::Rocky;
    Composition composition = Composition::Rocky;
    double spin_angle = 0.0;
    double spin_rate = 0.0;
    double axial_tilt_deg = 0.0;
    double flattening = 0.0;
    double procedural_seed = 0.0;
    double cloud_amount = 0.0;
    float atmosphere_color[3] = {0.0f, 0.0f, 0.0f};

    // Black-hole horizon (0 for non-black-hole bodies).
    double horizon_radius = 0.0;

    // Comet model.
    double comet_activity = 0.0;
    double coma_radius = 0.0;
    double tail_length = 0.0;
    double tail_dir[3] = {1.0, 0.0, 0.0};

    // Collapse/impact visual state.
    double collapse_flash = 0.0;

    char history[HIST_STR_LEN] = {};

    float color[3] = {1.0f, 1.0f, 1.0f};
};

struct MoonOrbit {
    int parent_index = -1;
    double physical_radius = 0.0;
    double visual_radius = 0.0;
    double visual_scale = 1.0;
    double inclination = 0.0;
    double real_orbit_radius_km = 0.0;
    double visual_x = 0.0;
    double visual_y = 0.0;
    double visual_z = 0.0;
};

struct TypeDefaults {
    const char *label;
    double mass;
    double radius;
    float color[3];
    double density_kg_m3;
    double albedo;
    double greenhouse_k;
};

const char *type_name(BodyType type);
const char *default_history(BodyType type);
const TypeDefaults &type_defaults(BodyType type);

} // namespace spacesim

#endif
