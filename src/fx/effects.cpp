#include "fx/effects.h"

#include "core/math_utils.h"
#include "core/state.h"

#include <algorithm>
#include <cmath>

namespace spacesim {

Effects::Effects() : particle_count_(0), shockwave_count_(0), shake_(0.0) {}

void Effects::reset() {
    clear();
}

void Effects::clear() {
    particle_count_ = 0;
    shockwave_count_ = 0;
    shake_ = 0.0;
}

void Effects::add_particle(const EffectParticle &particle) {
    if (particle_count_ >= 2048) return;
    particles_[particle_count_++] = particle;
}

void Effects::add_shockwave(const EffectShockwave &shockwave) {
    if (shockwave_count_ >= 64) return;
    shockwaves_[shockwave_count_++] = shockwave;
}

void Effects::spawn_particles(const FxEvent &event, int count, double speed,
                              float size, float r, float g, float b,
                              double life_scale) {
    for (int i = 0; i < count; ++i) {
        double theta = 2.0 * PI * random_unit();
        double u = 2.0 * random_unit() - 1.0;
        double s = std::sqrt(1.0 - u * u);
        double magnitude = speed * (0.35 + 0.65 * random_unit());
        EffectParticle particle;
        particle.x = event.x;
        particle.y = event.y;
        particle.z = event.z;
        particle.vx = std::cos(theta) * s * magnitude + event.nx * speed * 0.2;
        particle.vy = u * magnitude + event.ny * speed * 0.2;
        particle.vz = std::sin(theta) * s * magnitude + event.nz * speed * 0.2;
        particle.max_life = life_scale * (0.45 + 0.9 * random_unit());
        particle.life = particle.max_life;
        particle.size = size * static_cast<float>(0.55 + random_unit());
        particle.color[0] = r;
        particle.color[1] = g;
        particle.color[2] = b;
        add_particle(particle);
    }
}

void Effects::spawn_shockwave(const FxEvent &event, double speed, double life,
                              float r, float g, float b) {
    EffectShockwave wave;
    wave.x = event.x;
    wave.y = event.y;
    wave.z = event.z;
    wave.nx = event.nx;
    wave.ny = event.ny;
    wave.nz = event.nz;
    wave.radius = 0.005;
    wave.speed = speed;
    wave.max_life = life;
    wave.life = life;
    wave.color[0] = r;
    wave.color[1] = g;
    wave.color[2] = b;
    add_shockwave(wave);
}

void Effects::consume(UiState &ui) {
    for (int i = 0; i < ui.fx.count; ++i) {
        const FxEvent &event = ui.fx.events[i];
        switch (event.type) {
            case FxEventType::Spawn:
                spawn_particles(event, 20, 0.10, 2.2f,
                                event.color[0], event.color[1], event.color[2], 0.7);
                spawn_shockwave(event, 0.22, 0.45,
                                event.color[0], event.color[1], event.color[2]);
                break;
            case FxEventType::Impact:
                spawn_particles(event, static_cast<int>(28 + event.energy * 20),
                                0.16 + event.energy * 0.04, 3.0f,
                                1.0f, 0.72f, 0.30f, 1.0);
                spawn_shockwave(event, 0.55 + event.energy * 0.10, 0.7,
                                1.0f, 0.72f, 0.30f);
                shake_ = std::min(0.06, shake_ + 0.008 * event.energy);
                break;
            case FxEventType::Fragment:
                spawn_particles(event, 80, 0.28, 2.4f,
                                1.0f, 0.55f, 0.20f, 1.5);
                spawn_shockwave(event, 0.8, 0.9, 1.0f, 0.55f, 0.20f);
                shake_ = std::min(0.08, shake_ + 0.015);
                break;
            case FxEventType::Collapse:
                spawn_particles(event, 180, 0.42, 3.8f,
                                0.85f, 0.55f, 1.0f, 1.8);
                spawn_shockwave(event, 1.2, 1.4, 0.85f, 0.55f, 1.0f);
                shake_ = std::min(0.12, shake_ + 0.04);
                break;
            case FxEventType::Shockwave:
                spawn_shockwave(event, 0.9, 1.0,
                                event.color[0], event.color[1], event.color[2]);
                break;
        }
    }
    ui.fx.clear();
}

void Effects::update(double dt) {
    if (dt < 0.0) dt = 0.0;

    int write = 0;
    for (int i = 0; i < particle_count_; ++i) {
        EffectParticle &particle = particles_[i];
        particle.life -= dt;
        if (particle.life <= 0.0) continue;
        particle.x += particle.vx * dt;
        particle.y += particle.vy * dt;
        particle.z += particle.vz * dt;
        double drag = std::exp(-dt * 1.4);
        particle.vx *= drag;
        particle.vy *= drag;
        particle.vz *= drag;
        particles_[write++] = particle;
    }
    particle_count_ = write;

    write = 0;
    for (int i = 0; i < shockwave_count_; ++i) {
        EffectShockwave &wave = shockwaves_[i];
        wave.life -= dt;
        if (wave.life <= 0.0) continue;
        wave.radius += wave.speed * dt;
        shockwaves_[write++] = wave;
    }
    shockwave_count_ = write;

    shake_ *= std::exp(-dt * 3.0);
    if (shake_ < 0.0005) shake_ = 0.0;
}

} // namespace spacesim
