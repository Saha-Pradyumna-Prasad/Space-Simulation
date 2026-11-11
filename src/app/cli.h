#ifndef SPACESIM_APP_CLI_H
#define SPACESIM_APP_CLI_H

#include "core/config.h"

namespace spacesim {

struct CliOptions {
    bool help = false;
    bool selftest = false;
    bool screenshot_mode = false;
    bool has_window_size = false;
    int window_width = WINDOW_WIDTH;
    int window_height = WINDOW_HEIGHT;
    char screenshot_path[512] = "space_simulator.ppm";
    char initial_selection[NAME_STR_LEN] = {};
    bool has_camera = false;
    double camera_azimuth = 45.0;
    double camera_elevation = 35.0;
    double camera_distance = 3.05;
    bool show_grid = false;
    bool show_trails = true;
    bool show_prediction = true;
    bool classic_collisions = false;
    bool impact_demo = false;
    bool collapse_demo = false;
    bool comet_demo = false;
    bool follow_selected = false;
    bool show_lab = false;
    bool show_editor = false;
    bool physical_collisions = false;
    bool collision_debug = false;
    bool tutorial = false;
    int lab_tab = 0;
    bool guided_tour = false;
};

CliOptions parse_cli(int argc, char **argv);
void print_cli_usage(const char *program_name);

} // namespace spacesim

#endif
