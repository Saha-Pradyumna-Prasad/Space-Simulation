#include "physics/impact_model.h"

#include "core/body.h"
#include "core/config.h"
#include "core/physics_metrics.h"

#include <cmath>

namespace spacesim {

namespace {

double clamp01(double value) {
    if (value < 0.0) return 0.0;
    if (value > 1.0) return 1.0;
    return value;
}

} // namespace

ImpactResult classify_impact(const CelestialBody &a, const CelestialBody &b,
                             bool physical_collisions) {
    ImpactResult result;

    double rx = b.x - a.x;
    double ry = b.y - a.y;
    double rz = b.z - a.z;
    double distance = std::sqrt(rx * rx + ry * ry + rz * rz);
    if (distance < 1.0e-12) distance = 1.0e-12;

    double vx = b.vx - a.vx;
    double vy = b.vy - a.vy;
    double vz = b.vz - a.vz;
    double speed = std::sqrt(vx * vx + vy * vy + vz * vz);
    if (speed < 1.0e-12) speed = 1.0e-12;

    double radii = effective_collision_radius(a, physical_collisions) +
                   effective_collision_radius(b, physical_collisions);
    if (radii <= 0.0) radii = a.radius + b.radius;

    double cross_x = ry * vz - rz * vy;
    double cross_y = rz * vx - rx * vz;
    double cross_z = rx * vy - ry * vx;
    double impact_parameter = std::sqrt(cross_x * cross_x +
                                        cross_y * cross_y +
                                        cross_z * cross_z) / speed;
    double normalized_parameter = impact_parameter / (radii > 0.0 ? radii : 1.0);

    double min_mass = (a.mass < b.mass ? a.mass : b.mass);
    double max_mass = (a.mass > b.mass ? a.mass : b.mass);
    double total_mass = a.mass + b.mass;
    double reduced_mass = (total_mass > 0.0)
        ? (a.mass * b.mass / total_mass) : 0.0;
    double specific_energy = (total_mass > 0.0)
        ? 0.5 * reduced_mass * speed * speed / total_mass : 0.0;
    double escape_speed = (radii > 0.0)
        ? std::sqrt(2.0 * G_CONSTANT * total_mass / radii) : 1.0e-9;
    double escape_energy = 0.5 * escape_speed * escape_speed;
    double energy_ratio = specific_energy / (escape_energy > 0.0 ? escape_energy : 1.0);

    result.relative_speed = speed;
    result.impact_parameter = normalized_parameter;
    result.energy_ratio = energy_ratio;
    result.normal[0] = rx / distance;
    result.normal[1] = ry / distance;
    result.normal[2] = rz / distance;
    result.contact_x = (a.x + b.x) * 0.5;
    result.contact_y = (a.y + b.y) * 0.5;
    result.contact_z = (a.z + b.z) * 0.5;

    if (a.type == BodyType::BlackHole || b.type == BodyType::BlackHole) {
        result.outcome = ImpactOutcome::Merge;
        return result;
    }
    if (a.type == BodyType::Star || b.type == BodyType::Star) {
        // Stars absorb/merge rather than fragmenting into debris. A merged
        // star may still collapse through maybeCollapse().
        result.outcome = ImpactOutcome::Merge;
        return result;
    }

    double mass_ratio = (max_mass > 0.0) ? min_mass / max_mass : 0.0;
    double grazing = clamp01((normalized_parameter - 0.55) / 0.45);

    if (energy_ratio > 4.0 && mass_ratio > 0.08) {
        result.outcome = ImpactOutcome::Disrupt;
    } else if (energy_ratio > 1.25 && mass_ratio > 0.04) {
        result.outcome = ImpactOutcome::Fragment;
    } else if (grazing > 0.55 && energy_ratio > 0.05) {
        result.outcome = ImpactOutcome::Graze;
    } else if (energy_ratio > 0.55) {
        result.outcome = ImpactOutcome::Bounce;
    } else {
        result.outcome = ImpactOutcome::Merge;
    }
    return result;
}

} // namespace spacesim
