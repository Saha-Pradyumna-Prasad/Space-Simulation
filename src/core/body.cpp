#include "core/body.h"

namespace spacesim {

static const TypeDefaults TYPE_DEFAULTS[BODY_TYPE_COUNT] = {
    {"Star",      0.50,        0.050, {1.00f, 0.85f, 0.25f},   1408.0,  0.00,   0.0},
    {"Planet",    3.003e-6,    0.012, {0.35f, 0.62f, 1.00f},   5514.0,  0.30,   0.0},
    {"Moon",      3.700e-8,    0.006, {0.72f, 0.72f, 0.75f},   3344.0,  0.12,   0.0},
    {"Asteroid",  1.000e-9,    0.004, {0.62f, 0.57f, 0.50f},   3000.0,  0.12,   0.0},
    {"Meteor",    1.000e-10,   0.003, {1.00f, 0.90f, 0.60f},   3000.0,  0.10,   0.0},
    {"Comet",     1.000e-10,   0.004, {0.65f, 0.92f, 1.00f},    600.0,  0.04,   0.0},
    {"BlackHole", 2.00,        0.018, {0.06f, 0.02f, 0.10f}, 1.0e9,    0.00,   0.0},
};

const char *type_name(BodyType type) {
    switch (type) {
        case BodyType::Star:      return "Star";
        case BodyType::Planet:    return "Planet";
        case BodyType::Moon:      return "Moon";
        case BodyType::Asteroid:  return "Asteroid";
        case BodyType::Meteor:    return "Meteor";
        case BodyType::Comet:     return "Comet";
        case BodyType::BlackHole: return "Black Hole";
        default:                  return "Unknown";
    }
}

const char *default_history(BodyType type) {
    switch (type) {
        case BodyType::Star:
            return "A self-luminous star created in the simulation. Its mass sets the "
                   "strength of its gravity and the brightness of the light it casts "
                   "across the system. A star that grows past the collapse threshold "
                   "will become a black hole.";
        case BodyType::Planet:
            return "A world created in the simulation. Its mass, visual radius, "
                   "surface, atmosphere, and orbit can be changed in the Body Editor. "
                   "Its gravity perturbs every other active body, so small changes can "
                   "produce very different long-term paths.";
        case BodyType::Moon:
            return "A natural satellite created in the simulation. Moons are true "
                   "N-body bodies: they feel the Sun, their parent planet, and every "
                   "other active body. A moon spawned close to a planet is attached "
                   "in a Hill-safe physical orbit and rendered at a larger visual scale.";
        case BodyType::Asteroid:
            return "A small, rocky body representing the asteroid belt population. "
                   "Asteroids follow the same N-body gravity as planets and can graze, "
                   "fragment, merge, or be absorbed by a larger world.";
        case BodyType::Meteor:
            return "A tiny, fast-moving fragment. Meteors are useful for testing close "
                   "encounters, swept collision detection, and high-speed impacts; "
                   "because they are small and quick, they often need adaptive "
                   "sub-stepping to avoid tunnelling.";
        case BodyType::Comet:
            return "An icy body that develops a coma and tails when it approaches a "
                   "star. Its activity is driven by surface temperature and stellar "
                   "flux, and its ion and dust tails are shaped by the local radiation "
                   "field rather than a single scripted direction.";
        case BodyType::BlackHole:
            return "A compact object whose gravity traps light. Its event horizon grows "
                   "when it absorbs matter, and nearby orbits bend sharply around it. "
                   "The bright photon ring and accretion disk are visual guides to the "
                   "region where spacetime curvature becomes extreme.";
        default:
            return "A runtime body.";
    }
}

const TypeDefaults &type_defaults(BodyType type) {
    int index = static_cast<int>(type);
    if (index < 0 || index >= BODY_TYPE_COUNT) index = 0;
    return TYPE_DEFAULTS[index];
}

} // namespace spacesim
