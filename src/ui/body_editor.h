#ifndef SPACESIM_UI_BODY_EDITOR_H
#define SPACESIM_UI_BODY_EDITOR_H

#include "core/body.h"

namespace spacesim {

enum EditorField {
    EDITOR_MASS = 0,
    EDITOR_RADIUS,
    EDITOR_VOLUME,
    EDITOR_DENSITY,
    EDITOR_COLLISION_RADIUS,
    EDITOR_COLLISION_SCALE,
    EDITOR_POS_X,
    EDITOR_POS_Y,
    EDITOR_POS_Z,
    EDITOR_VEL_X,
    EDITOR_VEL_Y,
    EDITOR_VEL_Z,
    EDITOR_ALBEDO,
    EDITOR_GREENHOUSE,
    EDITOR_TILT,
    EDITOR_FLATTENING,
    EDITOR_CLOUD,
    EDITOR_SPIN,
    EDITOR_TYPE,
    EDITOR_COMPOSITION,
    EDITOR_ATM_R,
    EDITOR_ATM_G,
    EDITOR_ATM_B,
    EDITOR_INTERNAL_HEAT,
    EDITOR_THERMAL_INERTIA,
    EDITOR_TIDAL_LOCK,
    EDITOR_FIELD_COUNT
};

const char *editor_field_name(int field);
const char *editor_field_unit(int field);
const char *composition_name(Composition composition);
double editor_field_value(const CelestialBody &body, int field);
void adjust_body_field(CelestialBody &body, int field, int direction, bool coarse);
bool apply_body_field_text(CelestialBody &body, int field, const char *text);
void set_body_type_defaults(CelestialBody &body, BodyType type);
void apply_composition(CelestialBody &body, Composition composition);
void normalize_body_after_edit(CelestialBody &body);

} // namespace spacesim

#endif
