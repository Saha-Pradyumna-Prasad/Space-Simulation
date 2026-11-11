#ifndef SPACESIM_PHYSICS_COLLISION_SOLVER_H
#define SPACESIM_PHYSICS_COLLISION_SOLVER_H

#include "core/state.h"
#include "physics/impact_model.h"

namespace spacesim {

class Universe;
struct SelectionState;
struct UiState;

class CollisionSolver {
public:
    void resolve(Universe &universe, UiState &ui, SelectionState &selection,
                 CollisionMode mode, bool physical_collisions);

private:
    void mergeBodies(Universe &universe, UiState &ui,
                     SelectionState &selection, int keep, int gone,
                     const ImpactResult &impact);
    void bounceBodies(Universe &universe, UiState &ui,
                      int a, int b, const ImpactResult &impact,
                      bool physical_collisions);
    void fragmentBody(Universe &universe, UiState &ui,
                      SelectionState &selection, int keep, int gone,
                      const ImpactResult &impact, bool disruptive);
    void maybeCollapse(Universe &universe, UiState &ui, int index,
                       const ImpactResult &impact);
};

} // namespace spacesim

#endif
