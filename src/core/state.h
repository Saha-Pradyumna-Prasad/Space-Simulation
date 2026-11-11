#ifndef SPACESIM_CORE_STATE_H
#define SPACESIM_CORE_STATE_H

#include "core/body.h"
#include "core/config.h"
#include "core/events.h"

namespace spacesim {

enum class CollisionMode {
    Classic,
    Realistic
};

struct SimulationState {
    double time_multiplier = 1.0;
    bool paused = false;
    double sim_time = 0.0;
    CollisionMode collision_mode = CollisionMode::Realistic;
    bool physical_collisions = false;
};

struct SelectionState {
    int selected_body_index = -1;
    bool follow_selected = false;
};

struct SpawnState {
    double cursor_x = 0.0;
    double cursor_z = 0.0;
    bool cursor_valid = false;
    BodyType type = BodyType::Planet;
    int mass_percent = 100;
    int radius_percent = 100;
    int velocity_mode = 0;
    int counter[BODY_TYPE_COUNT] = {};
};

struct UiState {
    int win_width = WINDOW_WIDTH;
    int win_height = WINDOW_HEIGHT;
    int sim_width = WINDOW_WIDTH;
    float ui_scale = 1.0f;
    int fps = 0;
    int frame_count = 0;
    int fps_last_time = 0;
    bool show_labels = true;
    bool show_trails = true;
    bool show_prediction = true;
    bool show_gravity_grid = false;
    bool show_tidal = true;
    bool show_thermal = true;
    bool show_dust = true;
    bool show_collision_debug = false;
    bool show_tutorial = false;
    bool guided_tour = false;
    int tutorial_section = 0;
    int tutorial_scroll = 0;
    int tour_step = 0;
    bool show_equations = false;
    int equation_tab = 0;
    int equation_scroll = 0;
    int equation_body_b = -1;
    bool show_editor = false;
    int hover_body_index = -1;
    int editor_field = 0;
    bool editor_typing = false;
    char editor_buffer[64] = {};
    char status_message[160] = {};
    int status_frames = 0;
    double sim_time = 0.0;
    SpawnState spawn;
    FxQueue fx;
    EventLog events;
};

struct InputState {
    int drag_mode = 0;
    int last_mouse_x = 0;
    int last_mouse_y = 0;
};

} // namespace spacesim

#endif
