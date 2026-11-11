#ifndef SPACESIM_UI_LAYOUT_H
#define SPACESIM_UI_LAYOUT_H

#include "core/state.h"

namespace spacesim {

int hud_panel_width(const UiState &ui);
int simulation_width(const UiState &ui);
void update_sim_width(UiState &ui);

} // namespace spacesim

#endif
