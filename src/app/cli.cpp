#include "app/cli.h"

#include "core/math_utils.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace spacesim {

CliOptions parse_cli(int argc, char **argv) {
    CliOptions options;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--help") == 0 ||
            std::strcmp(argv[i], "-h") == 0) {
            options.help = true;
        } else if (std::strcmp(argv[i], "--selftest") == 0) {
            options.selftest = true;
        } else if (std::strcmp(argv[i], "--screenshot") == 0) {
            options.screenshot_mode = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                copy_path(options.screenshot_path, sizeof(options.screenshot_path), argv[i + 1]);
                ++i;
            }
        } else if (std::strcmp(argv[i], "--size") == 0) {
            if (i + 1 < argc) {
                int width = 0;
                int height = 0;
                if (std::sscanf(argv[i + 1], "%dx%d", &width, &height) == 2 &&
                    width >= 640 && height >= 480) {
                    options.has_window_size = true;
                    options.window_width = width;
                    options.window_height = height;
                } else {
                    std::fprintf(stderr,
                                 "Invalid --size value '%s' (expected WxH, minimum 640x480)\n",
                                 argv[i + 1]);
                }
                ++i;
            }
        } else if (std::strcmp(argv[i], "--select") == 0) {
            if (i + 1 < argc) {
                copy_name(options.initial_selection, argv[i + 1]);
                ++i;
            }
        } else if (std::strcmp(argv[i], "--camera") == 0) {
            if (i + 3 < argc) {
                options.has_camera = true;
                options.camera_azimuth = std::atof(argv[i + 1]);
                options.camera_elevation = std::atof(argv[i + 2]);
                options.camera_distance = std::atof(argv[i + 3]);
                i += 3;
            }
        } else if (std::strcmp(argv[i], "--grid") == 0) {
            options.show_grid = true;
        } else if (std::strcmp(argv[i], "--no-trails") == 0) {
            options.show_trails = false;
        } else if (std::strcmp(argv[i], "--no-prediction") == 0) {
            options.show_prediction = false;
        } else if (std::strcmp(argv[i], "--classic") == 0) {
            options.classic_collisions = true;
        } else if (std::strcmp(argv[i], "--impact-demo") == 0) {
            options.impact_demo = true;
        } else if (std::strcmp(argv[i], "--collapse-demo") == 0) {
            options.collapse_demo = true;
        } else if (std::strcmp(argv[i], "--comet-demo") == 0) {
            options.comet_demo = true;
        } else if (std::strcmp(argv[i], "--follow") == 0) {
            options.follow_selected = true;
        } else if (std::strcmp(argv[i], "--lab") == 0) {
            options.show_lab = true;
        } else if (std::strcmp(argv[i], "--editor") == 0) {
            options.show_editor = true;
        } else if (std::strcmp(argv[i], "--physical-collisions") == 0) {
            options.physical_collisions = true;
        } else if (std::strcmp(argv[i], "--collision-debug") == 0) {
            options.collision_debug = true;
        } else if (std::strcmp(argv[i], "--tutorial") == 0) {
            options.tutorial = true;
        } else if (std::strcmp(argv[i], "--tour") == 0) {
            options.tutorial = true;
            options.guided_tour = true;
        } else if (std::strcmp(argv[i], "--lab-tab") == 0) {
            if (i + 1 < argc) {
                options.lab_tab = std::atoi(argv[i + 1]);
                if (options.lab_tab < 0) options.lab_tab = 0;
                if (options.lab_tab > 7) options.lab_tab = 7;
                ++i;
            }
        } else {
            std::fprintf(stderr, "Unknown option: %s (use --help)\n", argv[i]);
        }
    }
    return options;
}

void print_cli_usage(const char *program_name) {
    const char *name = (program_name != nullptr) ? program_name : "space_simulator";
    std::printf(
        "Space Simulator v3.5\n"
        "Usage: %s [options]\n\n"
        "  --selftest                 Run headless tests and exit\n"
        "  --screenshot [path]        Capture a PPM after warm-up and exit\n"
        "  --size WxH                 Set the initial window size (e.g. 1366x738)\n"
        "  --select NAME              Select a body at startup\n"
        "  --camera AZ EL DIST        Set initial camera angles and distance\n"
        "  --follow                   Follow the selected body\n"
        "  --grid                     Enable the gravity grid\n"
        "  --no-trails                Disable orbit trails\n"
        "  --no-prediction            Disable trajectory prediction\n"
        "  --classic                  Use merge-only collisions\n"
        "  --physical-collisions      Use physical collision radii\n"
        "  --collision-debug          Show collision spheres\n"
        "  --impact-demo              Spawn a planetary impact demo\n"
        "  --collapse-demo            Spawn a collapsing star demo\n"
        "  --comet-demo               Spawn an active comet demo\n"
        "  --lab                      Open the Equation Lab\n"
        "  --lab-tab N                Open the Equation Lab on tab N (0-7)\n"
        "  --editor                   Open the Body Editor\n"
        "  --tutorial                 Open Tutorial & Controls\n"
        "  --tour                     Open the guided tour\n"
        "  --help, -h                 Show this help and exit\n",
        name);
}

} // namespace spacesim
