#ifndef SPACESIM_FX_DUST_FIELD_H
#define SPACESIM_FX_DUST_FIELD_H

#include "core/config.h"

#include <array>

namespace spacesim {

class Universe;
struct UiState;

struct DustParticle {
    double x = 0.0, y = 0.0, z = 0.0;
    double vx = 0.0, vy = 0.0, vz = 0.0;
    float size = 1.5f;
    float brightness = 0.5f;
    bool active = false;
    double respawn_delay = 0.0; // negative means promoted/off permanently
};

class DustField {
public:
    DustField();

    void reset();
    void update(Universe &universe, UiState &ui, double dt);
    int count() const { return DUST_COUNT; }
    const DustParticle *particles() const { return particles_.data(); }
    bool deactivate(int index);

private:
    std::array<DustParticle, DUST_COUNT> particles_;
};

} // namespace spacesim

#endif
