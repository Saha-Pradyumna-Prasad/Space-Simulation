#include "render/cosmetic_scene.h"

#include "core/config.h"

#include <cmath>

namespace spacesim {

namespace {

double solve_kepler(double mean_anomaly, double eccentricity) {
    double e = mean_anomaly;
    for (int i = 0; i < 4; ++i) {
        double f = e - eccentricity * std::sin(e) - mean_anomaly;
        double fp = 1.0 - eccentricity * std::cos(e);
        e -= f / fp;
    }
    return e;
}

bool in_kirkwood_gap(double a) {
    const double gaps[4] = {0.5433, 0.5780, 0.5920, 0.6270};
    for (int i = 0; i < 4; ++i) {
        if (std::fabs(a - gaps[i]) < 0.0036) return true;
    }
    return false;
}

} // namespace

CosmeticScene::CosmeticScene() : rng_state_(0x12345678u) {
    for (int i = 0; i < STAR_COUNT; ++i) {
        starfield_[i] = StarPoint{0.0f, 0.0f, 0.0f, 0.0f};
    }
    belt_.assign(BELT_COUNT, BeltParticle{});
}

double CosmeticScene::randomUnit() {
    rng_state_ = rng_state_ * 1664525u + 1013904223u;
    return static_cast<double>(rng_state_ & 0x00FFFFFFu) / 16777216.0;
}

void CosmeticScene::setParticlePosition(BeltParticle &particle) {
    double e_anomaly = solve_kepler(particle.mean_anomaly, particle.e);
    double radius = particle.a * (1.0 - particle.e * std::cos(e_anomaly));
    double true_anomaly = std::atan2(
        std::sqrt(1.0 - particle.e * particle.e) * std::sin(e_anomaly),
        std::cos(e_anomaly) - particle.e);
    double u = true_anomaly + particle.arg_peri;
    double x0 = radius * std::cos(u);
    double z0 = radius * std::sin(u);
    double y1 = z0 * std::sin(particle.inclination);
    double z1 = z0 * std::cos(particle.inclination);
    double c = std::cos(particle.node);
    double s = std::sin(particle.node);
    particle.x = x0 * c - z1 * s;
    particle.y = y1;
    particle.z = x0 * s + z1 * c;
}

void CosmeticScene::init() {
    rng_state_ = 0x12345678u;

    for (int i = 0; i < STAR_COUNT; ++i) {
        double u = 2.0 * randomUnit() - 1.0;
        double theta = std::acos(u);
        double phi = 2.0 * PI * randomUnit();
        double radius = 18.0 + 4.0 * randomUnit();
        double sin_theta = std::sin(theta);
        starfield_[i].x = static_cast<float>(radius * sin_theta * std::cos(phi));
        starfield_[i].y = static_cast<float>(radius * std::cos(theta));
        starfield_[i].z = static_cast<float>(radius * sin_theta * std::sin(phi));
        starfield_[i].brightness = 0.25f + static_cast<float>(0.70 * randomUnit());
    }

    for (int i = 0; i < BELT_COUNT; ++i) {
        double a = 0.0;
        for (int tries = 0; tries < 64; ++tries) {
            a = 0.500 + 0.130 * randomUnit();
            if (!in_kirkwood_gap(a)) break;
        }
        BeltParticle &particle = belt_[i];
        particle.a = a;
        particle.e = 0.20 * randomUnit();
        particle.inclination = (randomUnit() - 0.5) * 2.0 * (10.0 * PI / 180.0);
        particle.node = 2.0 * PI * randomUnit();
        particle.arg_peri = 2.0 * PI * randomUnit();
        particle.mean_anomaly = 2.0 * PI * randomUnit();
        particle.mean_motion = std::sqrt(G_CONSTANT /
                                         (a * a * a));
        particle.size = 1.1f + 2.4f * static_cast<float>(randomUnit());
        particle.brightness = 0.30f + 0.65f * static_cast<float>(randomUnit());
        setParticlePosition(particle);
    }
}

void CosmeticScene::update(double dt) {
    for (int i = 0; i < BELT_COUNT; ++i) {
        BeltParticle &particle = belt_[i];
        particle.mean_anomaly += particle.mean_motion * dt;
        if (particle.mean_anomaly > 2.0 * PI) {
            particle.mean_anomaly -= 2.0 * PI;
        }
        setParticlePosition(particle);
    }
}

} // namespace spacesim
