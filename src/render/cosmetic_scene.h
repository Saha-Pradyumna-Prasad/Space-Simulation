#ifndef SPACESIM_RENDER_COSMETIC_SCENE_H
#define SPACESIM_RENDER_COSMETIC_SCENE_H

#include "core/config.h"

#include <vector>

namespace spacesim {

class CosmeticScene {
public:
    struct StarPoint {
        float x, y, z, brightness;
    };

    struct BeltParticle {
        double a;
        double e;
        double inclination;
        double node;
        double arg_peri;
        double mean_anomaly;
        double mean_motion;
        double x, y, z;
        float size;
        float brightness;
    };

    CosmeticScene();

    void init();
    void update(double dt);

    int starCount() const { return STAR_COUNT; }
    const StarPoint *stars() const { return starfield_; }
    int beltCount() const { return static_cast<int>(belt_.size()); }
    const BeltParticle *belt() const { return belt_.data(); }

private:
    double randomUnit();
    void setParticlePosition(BeltParticle &particle);

    StarPoint starfield_[STAR_COUNT];
    std::vector<BeltParticle> belt_;
    unsigned int rng_state_;
};

} // namespace spacesim

#endif
