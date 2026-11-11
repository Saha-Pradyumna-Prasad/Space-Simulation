#ifndef SPACESIM_UI_HUD_H
#define SPACESIM_UI_HUD_H

#include "render/gl_text.h"

namespace spacesim {

class Universe;
class SceneRenderer;
struct UiState;
struct SimulationState;
struct SelectionState;

class Hud {
public:
    bool init();
    void draw(const Universe &universe, const UiState &ui,
              const SimulationState &simulation,
              const SelectionState &selection,
              const SceneRenderer &renderer);

private:
    void drawProjectedLabels(const SceneRenderer &renderer, const UiState &ui);
    void drawTutorial(const Universe &universe, const UiState &ui);
    void drawEquationLab(const Universe &universe, const UiState &ui,
                         const SimulationState &simulation,
                         const SelectionState &selection);
    void drawBodyEditor(const Universe &universe, const UiState &ui,
                        const SelectionState &selection);
    float drawWrapped(float x, float y, float max_width, const char *text,
                      float scale, float line_height, int max_lines,
                      float r, float g, float b);

    GlText text_;
};

} // namespace spacesim

#endif
