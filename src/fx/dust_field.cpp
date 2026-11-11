#include "fx/dust_field.h"

#include "core/events.h"
#include "core/math_utils.h"
#include "core/state.h"
#include "core/universe.h"

#include <cmath>

namespace spacesim {

namespace {

void seed_particle(DustParticle &p) {
    double radius = 0.48 + 0.17 * random_unit();
    double angle = 2.0 * PI * random_unit();
    p.x = radius * std::cos(angle);
    p.y = (random_unit() - 0.5) * 0.10;
    p.z = radius * std::sin(angle);
    double speed = std::sqrt(G_CONSTANT * radius) / (radius + SOFTENING);
    p.vx = -speed * std::sin(angle);
    p.vy = 0.0;
    p.vz = speed * std::cos(angle);
    p.size = 1.0f + 1.8f * static_cast<float>(random_unit());
    p.brightness = 0.25f + 0.55f * static_cast<float>(random_unit());
    p.active = true;
    p.respawn_delay = 0.0;
}

} // namespace

DustField::DustField() {
    reset();
}

void DustField::reset() {
    for (int i = 0; i < DUST_COUNT; ++i) {
        seed_particle(particles_[i]);
    }
}

bool DustField::deactivate(int index) {
    if (index < 0 || index >= DUST_COUNT) return false;
    if (!particles_[index].active) return false;
    particles_[index].active = false;
    particles_[index].respawn_delay = -1.0;
    return true;
}

void DustField::update(Universe &universe, UiState &ui, double dt) {
    for (int i = 0; i < DUST_COUNT; ++i) {
        DustParticle &p = particles_[i];
        if (!p.active) {
            if (p.respawn_delay < 0.0) continue;
            p.respawn_delay -= dt;
            if (p.respawn_delay <= 0.0) seed_particle(p);
            continue;
        }

        double ax = 0.0;
        double ay = 0.0;
        double az = 0.0;
        bool collided = false;
        for (int j = 0; j < MAX_OBJECTS; ++j) {
            if (!universe.isActive(j)) continue;
            const CelestialBody &body = universe.body(j);
            if (body.mass <= 0.0) continue;
            double dx = body.x - p.x;
            double dy = body.y - p.y;
            double dz = body.z - p.z;
            double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (distance < 1.0e-9) continue;

            if (distance < body.radius * 0.92) {
                p.active = false;
                p.respawn_delay = 3.0 + 4.0 * random_unit();
                collided = true;
                FxEvent event;
                event.type = FxEventType::Impact;
                event.x = p.x;
                event.y = p.y;
                event.z = p.z;
                event.energy = 0.18;
                event.color[0] = 0.8f;
                event.color[1] = 0.7f;
                event.color[2] = 0.5f;
                ui.fx.push(event);
                break;
            }
            double softened = distance + 0.01;
            double factor = G_CONSTANT * body.mass /
                            (softened * softened * distance);
            ax += factor * dx;
            ay += factor * dy;
            az += factor * dz;
        }
        if (collided) continue;

        p.vx += ax * dt;
        p.vy += ay * dt;
        p.vz += az * dt;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.z += p.vz * dt;
        if (p.x * p.x + p.y * p.y + p.z * p.z > 9.0) {
            p.active = false;
            p.respawn_delay = 2.0 + 3.0 * random_unit();
        }
    }
}

} // namespace spacesim
