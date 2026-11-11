#ifndef SPACESIM_RENDER_LABELS_H
#define SPACESIM_RENDER_LABELS_H

#include "core/config.h"

namespace spacesim {

struct ScreenLabel {
    int x = 0;
    int y = 0;
    bool selected = false;
    int priority = 1;
    char text[NAME_STR_LEN] = {};
    float color[3] = {1.0f, 1.0f, 1.0f};
};

} // namespace spacesim

#endif
