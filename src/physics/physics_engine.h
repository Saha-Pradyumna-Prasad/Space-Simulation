#ifndef SPACESIM_PHYSICS_PHYSICS_ENGINE_H
#define SPACESIM_PHYSICS_PHYSICS_ENGINE_H

namespace spacesim {

class Universe;
struct UiState;

class PhysicsEngine {
public:
    void integrate(Universe &universe, double dt, UiState &ui);
    void computeGravity(Universe &universe);
    int computeSubsteps(const Universe &universe, double dt) const;
};

} // namespace spacesim

#endif
