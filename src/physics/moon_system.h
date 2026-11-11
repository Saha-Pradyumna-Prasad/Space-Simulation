#ifndef SPACESIM_PHYSICS_MOON_SYSTEM_H
#define SPACESIM_PHYSICS_MOON_SYSTEM_H

namespace spacesim {

class Universe;

class MoonSystem {
public:
    static void visualPosition(const Universe &universe, int index, double out[3]);
    void updateVisuals(Universe &universe) const;
};

} // namespace spacesim

#endif
