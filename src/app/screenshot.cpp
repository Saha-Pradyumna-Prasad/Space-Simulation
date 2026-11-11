#include "app/screenshot.h"

#include "core/math_utils.h"

#include <GL/glut.h>
#include <cstdio>

namespace spacesim {

Screenshot::Screenshot() : armed_(false), done_(false) {
    copy_path(path_, sizeof(path_), "space_simulator.ppm");
}

void Screenshot::arm(const char *path) {
    copy_path(path_, sizeof(path_), path);
    armed_ = true;
    done_ = false;
}

bool Screenshot::shouldCapture() const {
    return armed_ && !done_;
}

bool Screenshot::done() const {
    return done_;
}

void Screenshot::save(int width, int height) {
    if (width < 1 || height < 1) return;

    unsigned char *pixels = new unsigned char[width * height * 3];
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    std::FILE *file = std::fopen(path_, "wb");
    if (file != nullptr) {
        std::fprintf(file, "P6\n%d %d\n255\n", width, height);
        for (int y = height - 1; y >= 0; --y) {
            std::fwrite(pixels + y * width * 3, 1, width * 3, file);
        }
        std::fclose(file);
        std::printf("Screenshot written to %s\n", path_);
    } else {
        std::printf("Could not open screenshot file %s\n", path_);
    }

    delete[] pixels;
    done_ = true;
}

} // namespace spacesim
