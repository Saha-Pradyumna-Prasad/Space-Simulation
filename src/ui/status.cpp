#include "ui/status.h"

#include <cstdarg>
#include <cstdio>

namespace spacesim {

void set_status(UiState &ui, const char *format, ...) {
    va_list args;
    va_start(args, format);
    std::vsnprintf(ui.status_message, sizeof(ui.status_message), format, args);
    va_end(args);
    ui.status_frames = 180;
}

void log_event(UiState &ui, float r, float g, float b, const char *format, ...) {
    char text[160];
    va_list args;
    va_start(args, format);
    std::vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    ui.events.add(ui.sim_time, text, r, g, b);
}

} // namespace spacesim
