#include "ui/body_editor.h"

#include "core/config.h"
#include "core/math_utils.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace spacesim {

namespace {

void apply_type(BodyType type, CelestialBody &body) {
    body.type = type;
    switch (type) {
        case BodyType::Star:
            body.surface_kind = SurfaceKind::Sun;
            body.composition = Composition::Stellar;
            body.color[0] = 1.0f;
            body.color[1] = 0.85f;
            body.color[2] = 0.25f;
            body.atmosphere_color[0] = 1.0f;
            body.atmosphere_color[1] = 0.8f;
            body.atmosphere_color[2] = 0.4f;
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
            body.color[0] = 0.005f;
            body.color[1] = 0.003f;
            body.color[2] = 0.010f;
            body.horizon_radius = clamp_double(
                0.009 + 0.006 * std::cbrt(body.mass / 2.0), 0.007, 0.035);
            body.radius = body.horizon_radius;
            body.collision_radius = body.horizon_radius * 0.55;
            break;
        default:
            break;
    }
}

void normalize_non_black_hole(CelestialBody &body) {
    body.mass = std::max(body.mass, 1.0e-15);
    body.radius = clamp_double(body.radius, 1.0e-5, 0.5);
    body.density_kg_m3 = clamp_double(body.density_kg_m3, 1.0, 1.0e12);
    body.albedo = clamp_double(body.albedo, 0.0, 1.0);
    body.cloud_amount = clamp_double(body.cloud_amount, 0.0, 1.0);
    body.flattening = clamp_double(body.flattening, 0.0, 0.6);
    sync_body_volume(body);
}

} // namespace

const char *editor_field_name(int field) {
    switch (field) {
        case EDITOR_MASS: return "Mass";
        case EDITOR_RADIUS: return "Visual radius";
        case EDITOR_VOLUME: return "Volume";
        case EDITOR_DENSITY: return "Density";
        case EDITOR_COLLISION_RADIUS: return "Collision radius";
        case EDITOR_COLLISION_SCALE: return "Collision scale";
        case EDITOR_POS_X: return "Position X";
        case EDITOR_POS_Y: return "Position Y";
        case EDITOR_POS_Z: return "Position Z";
        case EDITOR_VEL_X: return "Velocity X";
        case EDITOR_VEL_Y: return "Velocity Y";
        case EDITOR_VEL_Z: return "Velocity Z";
        case EDITOR_ALBEDO: return "Albedo";
        case EDITOR_GREENHOUSE: return "Greenhouse";
        case EDITOR_TILT: return "Axial tilt";
        case EDITOR_FLATTENING: return "Flattening";
        case EDITOR_CLOUD: return "Cloud amount";
        case EDITOR_SPIN: return "Spin rate";
        case EDITOR_TYPE: return "Body type";
        case EDITOR_COMPOSITION: return "Composition";
        case EDITOR_ATM_R: return "Atmosphere R";
        case EDITOR_ATM_G: return "Atmosphere G";
        case EDITOR_ATM_B: return "Atmosphere B";
        case EDITOR_INTERNAL_HEAT: return "Internal heat";
        case EDITOR_THERMAL_INERTIA: return "Thermal inertia";
        case EDITOR_TIDAL_LOCK: return "Tidal lock";
        default: return "";
    }
}

const char *editor_field_unit(int field) {
    switch (field) {
        case EDITOR_MASS: return "Earth masses";
        case EDITOR_RADIUS: return "sim units";
        case EDITOR_VOLUME: return "Earth volumes";
        case EDITOR_DENSITY: return "kg/m3";
        case EDITOR_COLLISION_RADIUS: return "sim units";
        case EDITOR_COLLISION_SCALE: return "0-1";
        case EDITOR_POS_X:
        case EDITOR_POS_Y:
        case EDITOR_POS_Z: return "sim units";
        case EDITOR_VEL_X:
        case EDITOR_VEL_Y:
        case EDITOR_VEL_Z: return "sim units/time";
        case EDITOR_ALBEDO: return "0-1";
        case EDITOR_GREENHOUSE: return "K";
        case EDITOR_TILT: return "degrees";
        case EDITOR_FLATTENING: return "0-1";
        case EDITOR_CLOUD: return "0-1";
        case EDITOR_SPIN: return "deg/time";
        case EDITOR_ATM_R:
        case EDITOR_ATM_G:
        case EDITOR_ATM_B: return "0-1";
        case EDITOR_INTERNAL_HEAT: return "K";
        case EDITOR_THERMAL_INERTIA: return "time units";
        case EDITOR_TIDAL_LOCK: return "0/1";
        default: return "";
    }
}

const char *composition_name(Composition composition) {
    switch (composition) {
        case Composition::Rocky: return "Rocky";
        case Composition::Icy: return "Icy";
        case Composition::Metallic: return "Metallic";
        case Composition::GasGiant: return "Gas Giant";
        case Composition::Stellar: return "Stellar";
        case Composition::Exotic: return "Exotic";
        default: return "Unknown";
    }
}

double editor_field_value(const CelestialBody &body, int field) {
    switch (field) {
        case EDITOR_MASS: return body.mass / EARTH_MASS_SIM;
        case EDITOR_RADIUS: return body.radius;
        case EDITOR_VOLUME: return body.volume_ratio;
        case EDITOR_DENSITY: return body.density_kg_m3;
        case EDITOR_COLLISION_RADIUS: return body.collision_radius;
        case EDITOR_COLLISION_SCALE: return body.collision_scale;
        case EDITOR_POS_X: return body.x;
        case EDITOR_POS_Y: return body.y;
        case EDITOR_POS_Z: return body.z;
        case EDITOR_VEL_X: return body.vx;
        case EDITOR_VEL_Y: return body.vy;
        case EDITOR_VEL_Z: return body.vz;
        case EDITOR_ALBEDO: return body.albedo;
        case EDITOR_GREENHOUSE: return body.greenhouse_k;
        case EDITOR_TILT: return body.axial_tilt_deg;
        case EDITOR_FLATTENING: return body.flattening;
        case EDITOR_CLOUD: return body.cloud_amount;
        case EDITOR_SPIN: return body.spin_rate;
        case EDITOR_ATM_R: return body.atmosphere_color[0];
        case EDITOR_ATM_G: return body.atmosphere_color[1];
        case EDITOR_ATM_B: return body.atmosphere_color[2];
        case EDITOR_INTERNAL_HEAT: return body.internal_heat_k;
        case EDITOR_THERMAL_INERTIA: return body.thermal_inertia;
        case EDITOR_TIDAL_LOCK: return body.tidal_lock ? 1.0 : 0.0;
        default: return 0.0;
    }
}

void set_body_type_defaults(CelestialBody &body, BodyType type) {
    apply_type(type, body);
    normalize_body_after_edit(body);
}

void apply_composition(CelestialBody &body, Composition composition) {
    body.composition = composition;
    switch (composition) {
        case Composition::Rocky:
            body.density_kg_m3 = 3000.0;
            body.albedo = 0.12;
            body.surface_kind = SurfaceKind::Rocky;
            body.cloud_amount = 0.0;
            break;
        case Composition::Icy:
            body.density_kg_m3 = 1000.0;
            body.albedo = 0.60;
            body.surface_kind = SurfaceKind::Icy;
            body.cloud_amount = 0.10;
            body.atmosphere_color[0] = 0.70f;
            body.atmosphere_color[1] = 0.82f;
            body.atmosphere_color[2] = 1.00f;
            break;
        case Composition::Metallic:
            body.density_kg_m3 = 7000.0;
            body.albedo = 0.08;
            body.surface_kind = SurfaceKind::Rocky;
            body.cloud_amount = 0.0;
            break;
        case Composition::GasGiant:
            body.density_kg_m3 = 1000.0;
            body.albedo = 0.35;
            body.surface_kind = SurfaceKind::GasGiant;
            body.cloud_amount = 0.30;
            body.atmosphere_color[0] = 0.82f;
            body.atmosphere_color[1] = 0.76f;
            body.atmosphere_color[2] = 0.60f;
            break;
        case Composition::Stellar:
            body.density_kg_m3 = 1408.0;
            body.albedo = 0.0;
            body.surface_kind = SurfaceKind::Sun;
            body.cloud_amount = 0.0;
            body.atmosphere_color[0] = 1.0f;
            body.atmosphere_color[1] = 0.8f;
            body.atmosphere_color[2] = 0.4f;
            break;
        case Composition::Exotic:
            body.density_kg_m3 = 1.0e6;
            body.albedo = 0.0;
            body.surface_kind = SurfaceKind::Rocky;
            body.cloud_amount = 0.0;
            break;
        default:
            break;
    }
}

void normalize_body_after_edit(CelestialBody &body) {
    body.mass = std::max(body.mass, 1.0e-15);
    body.radius = clamp_double(body.radius, 1.0e-5, 0.5);
    body.albedo = clamp_double(body.albedo, 0.0, 1.0);
    body.cloud_amount = clamp_double(body.cloud_amount, 0.0, 1.0);
    body.flattening = clamp_double(body.flattening, 0.0, 0.6);
    body.collision_radius = clamp_double(body.collision_radius, 0.0, 1.0);
    body.collision_scale = clamp_double(body.collision_scale, 0.0, 1.5);
    body.axial_tilt_deg = clamp_double(body.axial_tilt_deg, -360.0, 360.0);
    body.greenhouse_k = clamp_double(body.greenhouse_k, 0.0, 5000.0);
    body.internal_heat_k = clamp_double(body.internal_heat_k, 0.0, 10000.0);
    body.thermal_inertia = clamp_double(body.thermal_inertia, 0.0, 100.0);
    body.atmosphere_color[0] = clamp_double(body.atmosphere_color[0], 0.0, 1.0);
    body.atmosphere_color[1] = clamp_double(body.atmosphere_color[1], 0.0, 1.0);
    body.atmosphere_color[2] = clamp_double(body.atmosphere_color[2], 0.0, 1.0);
    if (body.type == BodyType::BlackHole) {
        body.horizon_radius = clamp_double(
            0.009 + 0.006 * std::cbrt(body.mass / 2.0), 0.007, 0.035);
        body.radius = body.horizon_radius;
        body.collision_radius = body.horizon_radius * 0.55;
        body.volume_ratio = 0.0;
        body.surface_kind = SurfaceKind::BlackHole;
        body.composition = Composition::Exotic;
        body.temperature_k = 0.0;
        body.albedo = 0.0;
    } else {
        normalize_non_black_hole(body);
    }
}

void adjust_body_field(CelestialBody &body, int field, int direction,
                       bool coarse) {
    if (direction == 0) return;
    double factor = coarse ? 2.0 : 1.15;
    switch (field) {
        case EDITOR_MASS:
            body.mass *= (direction > 0 ? factor : 1.0 / factor);
            break;
        case EDITOR_RADIUS:
            body.radius *= (direction > 0 ? 1.08 : 1.0 / 1.08);
            break;
        case EDITOR_VOLUME:
            // Volume is physical metadata derived from mass and density.
            // Changing it at constant density scales the mass.
            body.mass *= (direction > 0 ? factor : 1.0 / factor);
            break;
        case EDITOR_DENSITY:
            body.density_kg_m3 *= (direction > 0 ? factor : 1.0 / factor);
            break;
        case EDITOR_COLLISION_RADIUS:
            body.collision_radius *= (direction > 0 ? 1.08 : 1.0 / 1.08);
            break;
        case EDITOR_COLLISION_SCALE:
            body.collision_scale += direction * 0.05;
            break;
        case EDITOR_POS_X: body.x += direction * (coarse ? 0.10 : 0.01); break;
        case EDITOR_POS_Y: body.y += direction * (coarse ? 0.10 : 0.01); break;
        case EDITOR_POS_Z: body.z += direction * (coarse ? 0.10 : 0.01); break;
        case EDITOR_VEL_X: body.vx += direction * (coarse ? 0.10 : 0.01); break;
        case EDITOR_VEL_Y: body.vy += direction * (coarse ? 0.10 : 0.01); break;
        case EDITOR_VEL_Z: body.vz += direction * (coarse ? 0.10 : 0.01); break;
        case EDITOR_ALBEDO: body.albedo += direction * 0.05; break;
        case EDITOR_GREENHOUSE: body.greenhouse_k += direction * (coarse ? 20.0 : 5.0); break;
        case EDITOR_TILT: body.axial_tilt_deg += direction * (coarse ? 15.0 : 5.0); break;
        case EDITOR_FLATTENING: body.flattening += direction * 0.01; break;
        case EDITOR_CLOUD: body.cloud_amount += direction * 0.05; break;
        case EDITOR_SPIN: body.spin_rate += direction * (coarse ? 5.0 : 1.0); break;
        case EDITOR_TYPE: {
            int type = (static_cast<int>(body.type) + direction) % BODY_TYPE_COUNT;
            if (type < 0) type += BODY_TYPE_COUNT;
            apply_type(static_cast<BodyType>(type), body);
            break;
        }
        case EDITOR_COMPOSITION: {
            int c = (static_cast<int>(body.composition) + direction) % 6;
            if (c < 0) c += 6;
            apply_composition(body, static_cast<Composition>(c));
            break;
        }
        case EDITOR_ATM_R: body.atmosphere_color[0] += direction * 0.05f; break;
        case EDITOR_ATM_G: body.atmosphere_color[1] += direction * 0.05f; break;
        case EDITOR_ATM_B: body.atmosphere_color[2] += direction * 0.05f; break;
        case EDITOR_INTERNAL_HEAT: body.internal_heat_k += direction * 5.0; break;
        case EDITOR_THERMAL_INERTIA: body.thermal_inertia += direction * 0.25; break;
        case EDITOR_TIDAL_LOCK: body.tidal_lock = !body.tidal_lock; break;
        default: break;
    }
    normalize_body_after_edit(body);
}

bool apply_body_field_text(CelestialBody &body, int field, const char *text) {
    if (text == nullptr || text[0] == '\0') return false;
    char *end = nullptr;
    double value = std::strtod(text, &end);
    if (end == text || !std::isfinite(value)) return false;
    while (end != nullptr && (*end == ' ' || *end == '\t')) ++end;
    if (end != nullptr && *end != '\0') return false;

    switch (field) {
        case EDITOR_MASS:
            body.mass = value * EARTH_MASS_SIM;
            break;
        case EDITOR_RADIUS:
            // This is the rendered radius. Physical scale is edited through
            // density, volume, and collision radius.
            body.radius = value;
            break;
        case EDITOR_VOLUME:
            // Volume is derived from mass and density. At constant density,
            // changing the volume changes the mass.
            if (body.density_kg_m3 > 0.0) {
                body.mass = std::max(0.0, value) * body.density_kg_m3 /
                            EARTH_DENSITY_KG_M3 * EARTH_MASS_SIM;
            }
            break;
        case EDITOR_DENSITY:
            body.density_kg_m3 = value;
            break;
        case EDITOR_COLLISION_RADIUS: body.collision_radius = value; break;
        case EDITOR_COLLISION_SCALE: body.collision_scale = value; break;
        case EDITOR_POS_X: body.x = value; break;
        case EDITOR_POS_Y: body.y = value; break;
        case EDITOR_POS_Z: body.z = value; break;
        case EDITOR_VEL_X: body.vx = value; break;
        case EDITOR_VEL_Y: body.vy = value; break;
        case EDITOR_VEL_Z: body.vz = value; break;
        case EDITOR_ALBEDO: body.albedo = value; break;
        case EDITOR_GREENHOUSE: body.greenhouse_k = value; break;
        case EDITOR_TILT: body.axial_tilt_deg = value; break;
        case EDITOR_FLATTENING: body.flattening = value; break;
        case EDITOR_CLOUD: body.cloud_amount = value; break;
        case EDITOR_SPIN: body.spin_rate = value; break;
        case EDITOR_ATM_R: body.atmosphere_color[0] = static_cast<float>(value); break;
        case EDITOR_ATM_G: body.atmosphere_color[1] = static_cast<float>(value); break;
        case EDITOR_ATM_B: body.atmosphere_color[2] = static_cast<float>(value); break;
        case EDITOR_INTERNAL_HEAT: body.internal_heat_k = value; break;
        case EDITOR_THERMAL_INERTIA: body.thermal_inertia = value; break;
        case EDITOR_TIDAL_LOCK: body.tidal_lock = (value != 0.0); break;
        case EDITOR_TYPE: {
            int type = static_cast<int>(value);
            if (type < 0 || type >= BODY_TYPE_COUNT) return false;
            apply_type(static_cast<BodyType>(type), body);
            break;
        }
        case EDITOR_COMPOSITION: {
            int c = static_cast<int>(value);
            if (c < 0 || c >= 6) return false;
            apply_composition(body, static_cast<Composition>(c));
            break;
        }
        default: return false;
    }
    normalize_body_after_edit(body);
    return true;
}

} // namespace spacesim
