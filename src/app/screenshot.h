#ifndef SPACESIM_APP_SCREENSHOT_H
#define SPACESIM_APP_SCREENSHOT_H

#include "core/config.h"

namespace spacesim {

class Screenshot {
public:
    Screenshot();

    void arm(const char *path);
    bool shouldCapture() const;
    bool done() const;
    void save(int width, int height);

private:
    bool armed_;
    bool done_;
    char path_[512];
};

} // namespace spacesim

#endif
