#ifndef SPACESIM_CORE_PHYSICS_METRICS_H
#define SPACESIM_CORE_PHYSICS_METRICS_H

#include "core/body.h"

namespace spacesim {

struct PairMetrics {
    double distance = 0.0;
    double force = 0.0;
    double acceleration_a = 0.0;
    double acceleration_b = 0.0;
    double potential = 0.0;
    double escape_velocity = 0.0;
    double relative_speed = 0.0;
    double direction[3] = {1.0, 0.0, 0.0};
};

struct OrbitMetrics {
    bool valid = false;
    double semi_major_axis = 0.0;
    double eccentricity = 0.0;
    double periapsis = 0.0;
    double apoapsis = 0.0;
    double period = 0.0;
    double specific_energy = 0.0;
    double angular_momentum = 0.0;
};

struct BodyMetrics {
    double density = 0.0;
    double surface_gravity = 0.0;
    double escape_velocity = 0.0;
};

struct TidalMetrics {
    double acceleration = 0.0;
    double ratio = 0.0;
    double roche_limit = 0.0;
    double hill_radius = 0.0;
};

struct BlackHoleMetrics {
    double horizon = 0.0;
    double photon_sphere = 0.0;
    double isco = 0.0;
    double time_dilation = 0.0;
};

double effective_collision_radius(const CelestialBody &body,
                                  bool physical_collisions = false);
PairMetrics compute_pair_metrics(const CelestialBody &a, const CelestialBody &b);
OrbitMetrics compute_orbit_metrics(const CelestialBody &body,
                                   const CelestialBody &primary);
BodyMetrics compute_body_metrics(const CelestialBody &body);
TidalMetrics compute_tidal_metrics(const CelestialBody &body,
                                   const CelestialBody &primary,
                                   double star_mass,
                                   double body_orbit_radius);
BlackHoleMetrics compute_black_hole_metrics(const CelestialBody &body);

} // namespace spacesim

#endif
