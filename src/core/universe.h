#ifndef SPACESIM_CORE_UNIVERSE_H
#define SPACESIM_CORE_UNIVERSE_H

#include "core/body.h"

#include <array>

namespace spacesim {

class Universe {
public:
    Universe();

    void reset();
    int count() const;
    bool isActive(int index) const;

    CelestialBody &body(int index);
    const CelestialBody &body(int index) const;
    MoonOrbit &moon(int index);
    const MoonOrbit &moon(int index) const;

    std::array<CelestialBody, MAX_OBJECTS> &bodies();
    const std::array<CelestialBody, MAX_OBJECTS> &bodies() const;
    std::array<MoonOrbit, MAX_OBJECTS> &moons();
    const std::array<MoonOrbit, MAX_OBJECTS> &moons() const;

    int addBody(BodyType type, const char *name,
                double x, double y, double z,
                double vx, double vy, double vz,
                double mass, double radius);
    bool removeBody(int index);
    int findByName(const char *name) const;

    int heaviestBody() const;
    int nearestStarIndex(int body_index) const;
    double distanceToNearestStar(int body_index,
                                 double &star_x, double &star_y, double &star_z) const;

    void detachMoonChildren(int parent_index);
    void savePreviousPositions();
    double prevX(int index) const;
    double prevY(int index) const;
    double prevZ(int index) const;

    void seedSolarSystem();
    void seedMoons();
    void printBodies() const;

private:
    int findFreeSlot() const;

    std::array<CelestialBody, MAX_OBJECTS> bodies_;
    std::array<MoonOrbit, MAX_OBJECTS> moons_;
    std::array<double, MAX_OBJECTS> prev_x_;
    std::array<double, MAX_OBJECTS> prev_y_;
    std::array<double, MAX_OBJECTS> prev_z_;
    int active_count_;
};

} // namespace spacesim

#endif
