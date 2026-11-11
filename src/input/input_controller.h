#ifndef SPACESIM_INPUT_INPUT_CONTROLLER_H
#define SPACESIM_INPUT_INPUT_CONTROLLER_H

#include <functional>

namespace spacesim {

class Universe;
class OrbitCamera;
class SpawnService;
class DustField;
struct UiState;
struct SimulationState;
struct SelectionState;
struct InputState;

class InputController {
public:
    InputController(Universe &universe, UiState &ui, SimulationState &simulation,
                    SelectionState &selection, InputState &input,
                    OrbitCamera &camera, SpawnService &spawn, DustField &dust,
                    std::function<void()> reset_universe);

    void keyboard(unsigned char key);
    void special(int key);
    void mouse(int button, int state, int x, int y);
    void motion(int x, int y);
    void passiveMotion(int x, int y);

private:
    void track_mouse(int x, int y);
    void speed_up();
    void speed_down();
    void open_tutorial();
    void handle_tutorial_key(unsigned char key);
    void open_equations();
    void open_editor();
    void handle_equation_key(unsigned char key);
    void handle_editor_key(unsigned char key);
    void adjust_editor_field(int direction, bool coarse);
    void apply_editor_buffer();
    void spawn_custom_body();
    void promote_dust();

    Universe &universe_;
    UiState &ui_;
    SimulationState &simulation_;
    SelectionState &selection_;
    InputState &input_;
    OrbitCamera &camera_;
    SpawnService &spawn_;
    DustField &dust_;
    std::function<void()> reset_universe_;
};

} // namespace spacesim

#endif
