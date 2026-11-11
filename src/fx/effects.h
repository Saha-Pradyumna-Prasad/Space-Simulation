#ifndef SPACESIM_FX_EFFECTS_H
#define SPACESIM_FX_EFFECTS_H

#include "core/config.h"
#include "core/events.h"

namespace spacesim {

struct UiState;

struct EffectParticle {
    double x = 0.0, y = 0.0, z = 0.0;
    double vx = 0.0, vy = 0.0, vz = 0.0;
    double life = 0.0;
    double max_life = 1.0;
    float size = 2.0f;
    float color[3] = {1.0f, 1.0f, 1.0f};
};

struct EffectShockwave {
    double x = 0.0, y = 0.0, z = 0.0;
    double nx = 0.0, ny = 1.0, nz = 0.0;
    double radius = 0.0;
    double speed = 0.0;
    double life = 0.0;
    double max_life = 1.0;
    float color[3] = {1.0f, 0.8f, 0.4f};
};

class Effects {
public:
    Effects();

    void reset();
    void consume(UiState &ui);
    void update(double dt);
    void clear();

    int particle_count() const { return particle_count_; }
    const EffectParticle *particles() const { return particles_; }
    int shockwave_count() const { return shockwave_count_; }
    const EffectShockwave *shockwaves() const { return shockwaves_; }
    double shake_amount() const { return shake_; }

private:
    void spawn_particles(const FxEvent &event, int count, double speed,
                         float size, float r, float g, float b,
                         double life_scale);
    void spawn_shockwave(const FxEvent &event, double speed, double life,
                         float r, float g, float b);
    void add_particle(const EffectParticle &particle);
    void add_shockwave(const EffectShockwave &shockwave);

    EffectParticle particles_[2048];
    int particle_count_;
    EffectShockwave shockwaves_[64];
    int shockwave_count_;
    double shake_;
};

} // namespace spacesim

#endif
