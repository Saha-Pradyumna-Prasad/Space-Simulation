#include "ui/layout.h"

#include "core/config.h"

#include <algorithm>

namespace spacesim {

namespace {

int scaled_margin(const UiState &ui) {
    int margin = static_cast<int>(HUD_MARGIN * ui.ui_scale);
    return margin > 0 ? margin : 1;
}

} // namespace

int hud_panel_width(const UiState &ui) {
    // Scale the HUD with the window so a maximized 1366x768 session keeps the
    // same proportions as the 1600x900 design layout.
    int available = ui.win_width - 2 * scaled_margin(ui) -
                    static_cast<int>(200.0f * ui.ui_scale);
    int width = std::min(static_cast<int>(HUD_PANEL_MAX_WIDTH * ui.ui_scale),
                         available);
    if (width < 200) width = 200;
    return width;
}

int simulation_width(const UiState &ui) {
    int width = ui.win_width - hud_panel_width(ui) - 2 * scaled_margin(ui);
    if (width < 200) width = 200;
    return width;
}

void update_sim_width(UiState &ui) {
    if (ui.win_width < 1) ui.win_width = 1;
    if (ui.win_height < 1) ui.win_height = 1;

    const float scale_x = static_cast<float>(ui.win_width) /
                          static_cast<float>(WINDOW_WIDTH);
    const float scale_y = static_cast<float>(ui.win_height) /
                          static_cast<float>(WINDOW_HEIGHT);
    ui.ui_scale = std::min(scale_x, scale_y);
    if (ui.ui_scale < 0.70f) ui.ui_scale = 0.70f;
    if (ui.ui_scale > 1.0f) ui.ui_scale = 1.0f;

    ui.sim_width = simulation_width(ui);
    if (ui.sim_width < 1) ui.sim_width = 1;
}

} // namespace spacesim
