#ifndef SPACESIM_UI_STATUS_H
#define SPACESIM_UI_STATUS_H

#include "core/state.h"

namespace spacesim {

void set_status(UiState &ui, const char *format, ...);
void log_event(UiState &ui, float r, float g, float b, const char *format, ...);

} // namespace spacesim

#endif
