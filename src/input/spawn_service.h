#ifndef SPACESIM_INPUT_SPAWN_SERVICE_H
#define SPACESIM_INPUT_SPAWN_SERVICE_H

namespace spacesim {

class Universe;
struct UiState;
struct SelectionState;

class SpawnService {
public:
    void deleteSelected(Universe &universe, SelectionState &selection,
                        UiState &ui) const;
    void spawnAtCursor(Universe &universe, UiState &ui,
                       SelectionState &selection) const;
};

} // namespace spacesim

#endif
