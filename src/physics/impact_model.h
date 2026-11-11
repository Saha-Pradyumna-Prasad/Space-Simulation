#ifndef SPACESIM_PHYSICS_IMPACT_MODEL_H
#define SPACESIM_PHYSICS_IMPACT_MODEL_H

namespace spacesim {

struct CelestialBody;

enum class ImpactOutcome {
    Merge,
    Bounce,
    Graze,
    Fragment,
    Disrupt
};

struct ImpactResult {
    ImpactOutcome outcome = ImpactOutcome::Merge;
    double relative_speed = 0.0;
    double impact_parameter = 0.0;
    double energy_ratio = 0.0;
    double normal[3] = {1.0, 0.0, 0.0};
    double contact_x = 0.0;
    double contact_y = 0.0;
    double contact_z = 0.0;
};

ImpactResult classify_impact(const CelestialBody &a, const CelestialBody &b,
                             bool physical_collisions = false);

} // namespace spacesim

#endif
