#ifndef SPACESIM_PHYSICS_ENVIRONMENT_H
#define SPACESIM_PHYSICS_ENVIRONMENT_H

namespace spacesim {

class Universe;

class Environment {
public:
    void update(Universe &universe, double dt);
};

} // namespace spacesim

#endif
