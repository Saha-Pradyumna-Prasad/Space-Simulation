#include "input/input_controller.h"

#include "core/config.h"
#include "core/math_utils.h"
#include "core/state.h"
#include "core/universe.h"
#include "input/picking.h"
#include "fx/dust_field.h"
#include "input/spawn_service.h"
#include "render/camera.h"
#include "ui/body_editor.h"
#include "ui/layout.h"
#include "ui/status.h"

#include <GL/glut.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>

namespace spacesim {

InputController::InputController(Universe &universe, UiState &ui,
                                 SimulationState &simulation,
                                 SelectionState &selection, InputState &input,
                                 OrbitCamera &camera, SpawnService &spawn,
                                 DustField &dust,
                                 std::function<void()> reset_universe)
    : universe_(universe), ui_(ui), simulation_(simulation),
      selection_(selection), input_(input), camera_(camera), spawn_(spawn),
      dust_(dust), reset_universe_(reset_universe) {}

void InputController::speed_up() {
    simulation_.time_multiplier *= 1.5;
    if (simulation_.time_multiplier > MAX_SPEED_MULT) {
        simulation_.time_multiplier = MAX_SPEED_MULT;
    }
    set_status(ui_, "Speed: %.2fx", simulation_.time_multiplier);
}

void InputController::speed_down() {
    simulation_.time_multiplier /= 1.5;
    if (simulation_.time_multiplier < MIN_SPEED_MULT) {
        simulation_.time_multiplier = MIN_SPEED_MULT;
    }
    set_status(ui_, "Speed: %.2fx", simulation_.time_multiplier);
}

void InputController::track_mouse(int x, int y) {
    update_sim_width(ui_);
    if (x < 0 || x >= ui_.sim_width) {
        ui_.hover_body_index = -1;
        return;
    }
    if (ui_.show_tutorial || ui_.show_equations || ui_.show_editor) {
        ui_.hover_body_index = -1;
        return;
    }

    double origin[3];
    double direction[3];
    screen_ray(camera_, universe_, selection_, ui_, x, y, origin, direction);

    double px = 0.0;
    double pz = 0.0;
    if (ray_plane_y0(origin, direction, px, pz)) {
        ui_.spawn.cursor_x = clamp_double(px, -1.20, 1.20);
        ui_.spawn.cursor_z = clamp_double(pz, -1.20, 1.20);
        ui_.spawn.cursor_valid = true;
    }
    ui_.hover_body_index = pick_body(universe_, camera_, selection_, ui_, x, y);
}

void InputController::mouse(int button, int state, int x, int y) {
    update_sim_width(ui_);
    if (x >= ui_.sim_width) return;

    if (button == 3 && state == GLUT_DOWN) {
        camera_.zoom(1.0);
        return;
    }
    if (button == 4 && state == GLUT_DOWN) {
        camera_.zoom(-1.0);
        return;
    }

    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            track_mouse(x, y);
            if (ui_.show_equations &&
                (glutGetModifiers() & GLUT_ACTIVE_SHIFT) != 0) {
                int picked = pick_body(universe_, camera_, selection_, ui_, x, y);
                if (picked >= 0 &&
                    picked != selection_.selected_body_index) {
                    ui_.equation_body_b = picked;
                    set_status(ui_, "Equation target: %s",
                               universe_.body(picked).name);
                }
                return;
            }
            selection_.selected_body_index =
                pick_body(universe_, camera_, selection_, ui_, x, y);
            if (selection_.selected_body_index < 0) {
                selection_.follow_selected = false;
                set_status(ui_, "Selection cleared");
            }
            input_.drag_mode = 1;
            input_.last_mouse_x = x;
            input_.last_mouse_y = y;
        } else if (state == GLUT_UP && input_.drag_mode == 1) {
            input_.drag_mode = 0;
        }
    } else if (button == GLUT_RIGHT_BUTTON) {
        if (state == GLUT_DOWN) {
            input_.drag_mode = 2;
            input_.last_mouse_x = x;
            input_.last_mouse_y = y;
        } else if (state == GLUT_UP && input_.drag_mode == 2) {
            input_.drag_mode = 0;
        }
    }
}

void InputController::motion(int x, int y) {
    int dx = x - input_.last_mouse_x;
    int dy = y - input_.last_mouse_y;
    input_.last_mouse_x = x;
    input_.last_mouse_y = y;

    if (input_.drag_mode == 1) {
        camera_.rotate(static_cast<double>(dx), static_cast<double>(dy));
    } else if (input_.drag_mode == 2) {
        camera_.syncTarget(universe_, selection_);
        camera_.pan(static_cast<double>(dx), static_cast<double>(dy), selection_);
    }

    if (x >= 0 && x < ui_.sim_width) {
        track_mouse(x, y);
    }
}

void InputController::passiveMotion(int x, int y) {
    track_mouse(x, y);
}

void InputController::open_tutorial() {
    ui_.show_tutorial = true;
    ui_.show_equations = false;
    ui_.show_editor = false;
    ui_.tutorial_section = 0;
    ui_.tutorial_scroll = 0;
    set_status(ui_, "Tutorial & Controls");
}

void InputController::handle_tutorial_key(unsigned char key) {
    if (key == 27 || key == 'h' || key == 'H') {
        ui_.show_tutorial = false;
        ui_.guided_tour = false;
        set_status(ui_, "Tutorial closed");
        return;
    }
    if (key == 'q' || key == 'Q') std::exit(0);
    if (key == 9) {
        ui_.tutorial_section = (ui_.tutorial_section + 1) % 8;
        ui_.tutorial_scroll = 0;
    } else if (key == 't' || key == 'T') {
        ui_.guided_tour = !ui_.guided_tour;
        ui_.tour_step = 0;
    } else if (key == 13 || key == ' ') {
        if (ui_.guided_tour) ++ui_.tour_step;
    } else if (key == 8) {
        if (ui_.guided_tour && ui_.tour_step > 0) --ui_.tour_step;
    }
}

void InputController::open_equations() {
    ui_.show_equations = true;
    ui_.show_editor = false;
    ui_.equation_tab = 0;
    ui_.equation_scroll = 0;
    if (selection_.selected_body_index < 0 ||
        selection_.selected_body_index >= MAX_OBJECTS ||
        !universe_.isActive(selection_.selected_body_index)) {
        selection_.selected_body_index = universe_.heaviestBody();
    }
    int a = selection_.selected_body_index;
    if (a < 0 || a >= MAX_OBJECTS || !universe_.isActive(a)) {
        set_status(ui_, "Equation Lab needs an active body");
        ui_.show_equations = false;
        return;
    }
    ui_.equation_body_b = -1;
    double best_distance = 1.0e30;
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe_.isActive(i) || i == a) continue;
        const CelestialBody &body = universe_.body(i);
        double dx = body.x - universe_.body(a).x;
        double dy = body.y - universe_.body(a).y;
        double dz = body.z - universe_.body(a).z;
        double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (distance < best_distance) {
            best_distance = distance;
            ui_.equation_body_b = i;
        }
    }
    set_status(ui_, "Equation Lab: A=%s  B=%s",
               a >= 0 ? universe_.body(a).name : "none",
               ui_.equation_body_b >= 0 ? universe_.body(ui_.equation_body_b).name : "none");
}

void InputController::open_editor() {
    ui_.show_editor = true;
    ui_.show_equations = false;
    ui_.editor_field = 0;
    ui_.editor_typing = false;
    ui_.editor_buffer[0] = '\0';
    if (selection_.selected_body_index < 0 ||
        selection_.selected_body_index >= MAX_OBJECTS ||
        !universe_.isActive(selection_.selected_body_index)) {
        selection_.selected_body_index = universe_.heaviestBody();
    }
    set_status(ui_, "Body Editor: %s",
               selection_.selected_body_index >= 0
                   ? universe_.body(selection_.selected_body_index).name
                   : "select a body");
}

void InputController::handle_equation_key(unsigned char key) {
    if (key == 27 || key == 'e' || key == 'E') {
        ui_.show_equations = false;
        set_status(ui_, "Equation Lab closed");
        return;
    }
    if (key == 'q' || key == 'Q') std::exit(0);
    if (key == 9) {
        ui_.equation_tab = (ui_.equation_tab + 1) % 8;
        ui_.equation_scroll = 0;
    } else if (key == 'n' || key == 'N') {
        int a = selection_.selected_body_index;
        int start = ui_.equation_body_b;
        for (int step = 1; step <= MAX_OBJECTS; ++step) {
            int candidate = (start + step) % MAX_OBJECTS;
            if (candidate != a && universe_.isActive(candidate)) {
                ui_.equation_body_b = candidate;
                set_status(ui_, "Equation target: %s", universe_.body(candidate).name);
                break;
            }
        }
    }
}

void InputController::handle_editor_key(unsigned char key) {
    if (key == 27) {
        if (ui_.editor_typing) {
            ui_.editor_typing = false;
            ui_.editor_buffer[0] = '\0';
        } else {
            ui_.show_editor = false;
            set_status(ui_, "Body Editor closed");
        }
        return;
    }

    bool numeric = (key >= '0' && key <= '9') || key == '.' || key == '-';

    if (ui_.editor_typing) {
        // While entering a value, only editing keys are meaningful. This
        // prevents Q/B/S/R from quitting, closing, spawning, or resetting.
        if (key == 13) {
            apply_editor_buffer();
        } else if (key == 8) {
            size_t len = std::strlen(ui_.editor_buffer);
            if (len > 0) ui_.editor_buffer[len - 1] = '\0';
        } else if (numeric) {
            size_t len = std::strlen(ui_.editor_buffer);
            if (len + 1 < sizeof(ui_.editor_buffer)) {
                ui_.editor_buffer[len] = static_cast<char>(key);
                ui_.editor_buffer[len + 1] = '\0';
            }
        }
        return;
    }

    if (key == 'q' || key == 'Q') std::exit(0);
    if (key == 'b' || key == 'B') {
        ui_.show_editor = false;
        set_status(ui_, "Body Editor closed");
        return;
    }
    if (key == 's' || key == 'S') {
        spawn_custom_body();
        return;
    }
    if (key == 'r' || key == 'R') {
        if (selection_.selected_body_index >= 0 &&
            universe_.isActive(selection_.selected_body_index)) {
            CelestialBody &body = universe_.body(selection_.selected_body_index);
            set_body_type_defaults(body, body.type);
            set_status(ui_, "Reset %s appearance to type defaults", body.name);
        }
        return;
    }
    if (key == 13) {
        if (selection_.selected_body_index >= 0 &&
            universe_.isActive(selection_.selected_body_index)) {
            ui_.editor_buffer[0] = '\0';
            ui_.editor_typing = true;
        }
        return;
    }
    if (numeric) {
        ui_.editor_buffer[0] = '\0';
        ui_.editor_typing = true;
        ui_.editor_buffer[0] = static_cast<char>(key);
        ui_.editor_buffer[1] = '\0';
    }
}

void InputController::adjust_editor_field(int direction, bool coarse) {
    if (selection_.selected_body_index < 0 ||
        !universe_.isActive(selection_.selected_body_index)) {
        set_status(ui_, "Select a body to edit");
        return;
    }
    CelestialBody &body = universe_.body(selection_.selected_body_index);
    adjust_body_field(body, ui_.editor_field, direction, coarse);
}

void InputController::apply_editor_buffer() {
    if (selection_.selected_body_index < 0 ||
        !universe_.isActive(selection_.selected_body_index)) return;
    CelestialBody &body = universe_.body(selection_.selected_body_index);
    if (apply_body_field_text(body, ui_.editor_field, ui_.editor_buffer)) {
        set_status(ui_, "%s applied", editor_field_name(ui_.editor_field));
    } else {
        set_status(ui_, "Invalid value for %s", editor_field_name(ui_.editor_field));
    }
    ui_.editor_typing = false;
    ui_.editor_buffer[0] = '\0';
}

void InputController::promote_dust() {
    if (!ui_.spawn.cursor_valid) {
        set_status(ui_, "Move the cursor over the scene first");
        return;
    }
    int best = -1;
    double best_distance = 0.20;
    for (int i = 0; i < dust_.count(); ++i) {
        const DustParticle &p = dust_.particles()[i];
        if (!p.active) continue;
        double dx = p.x - ui_.spawn.cursor_x;
        double dz = p.z - ui_.spawn.cursor_z;
        double distance = std::sqrt(dx * dx + dz * dz);
        if (distance < best_distance) {
            best_distance = distance;
            best = i;
        }
    }
    if (best < 0) {
        set_status(ui_, "No dust near cursor");
        return;
    }
    const DustParticle &p = dust_.particles()[best];
    int type_index = static_cast<int>(BodyType::Meteor);
    ui_.spawn.counter[type_index] += 1;
    char name[NAME_STR_LEN];
    std::snprintf(name, sizeof(name), "Dust %d", ui_.spawn.counter[type_index]);
    int slot = universe_.addBody(BodyType::Meteor, name, p.x, p.y, p.z,
                                 p.vx, p.vy, p.vz,
                                 1.0e-12, 0.0020);
    if (slot < 0) {
        set_status(ui_, "Cannot promote dust: pool is full");
        return;
    }
    dust_.deactivate(best);
    selection_.selected_body_index = slot;
    set_status(ui_, "Promoted %s to a physical body", name);
}

void InputController::spawn_custom_body() {
    if (selection_.selected_body_index < 0 ||
        !universe_.isActive(selection_.selected_body_index)) {
        set_status(ui_, "Select a template body first");
        return;
    }
    if (!ui_.spawn.cursor_valid) {
        set_status(ui_, "Move the cursor over the scene first");
        return;
    }
    CelestialBody spec = universe_.body(selection_.selected_body_index);
    int type_index = static_cast<int>(spec.type);
    ui_.spawn.counter[type_index] += 1;
    char name[NAME_STR_LEN];
    std::snprintf(name, sizeof(name), "Custom %s %d", type_name(spec.type),
                  ui_.spawn.counter[type_index]);
    double x = clamp_double(ui_.spawn.cursor_x, -1.20, 1.20);
    double z = clamp_double(ui_.spawn.cursor_z, -1.20, 1.20);
    int slot = universe_.addBody(spec.type, name, x, 0.0, z,
                                 spec.vx, spec.vy, spec.vz,
                                 spec.mass, spec.radius);
    if (slot < 0) {
        set_status(ui_, "Cannot add: pool is full");
        return;
    }
    CelestialBody copy = spec;
    copy_name(copy.name, name);
    copy.active = true;
    copy.x = x;
    copy.y = 0.0;
    copy.z = z;
    copy.vx = spec.vx;
    copy.vy = spec.vy;
    copy.vz = spec.vz;
    std::snprintf(copy.history, sizeof(copy.history),
                  "A custom %s created from an edited template in the Body Editor. "
                  "Its mass, visual radius, surface, atmosphere, and starting "
                  "kinematics are user-defined; it then evolves under the same "
                  "N-body gravity as every other body.",
                  type_name(copy.type));
    universe_.body(slot) = copy;
    normalize_body_after_edit(universe_.body(slot));
    selection_.selected_body_index = slot;
    set_status(ui_, "Spawned custom body %s", name);
}

void InputController::keyboard(unsigned char key) {
    if (ui_.show_tutorial) {
        handle_tutorial_key(key);
        return;
    }
    if (ui_.show_equations) {
        handle_equation_key(key);
        return;
    }
    if (ui_.show_editor) {
        handle_editor_key(key);
        return;
    }
    switch (key) {
        case 27:
        case 'q':
        case 'Q':
            std::exit(0);
            break;

        case 'h':
        case 'H':
            open_tutorial();
            break;

        case 'e':
        case 'E':
            open_equations();
            break;

        case 'b':
        case 'B':
            open_editor();
            break;

        case ' ':
        case 'p':
        case 'P':
            simulation_.paused = !simulation_.paused;
            set_status(ui_, simulation_.paused ? "Simulation paused" : "Simulation resumed");
            break;

        case '+':
        case '=':
            speed_up();
            break;

        case '-':
        case '_':
            speed_down();
            break;

        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
            ui_.spawn.type = static_cast<BodyType>(key - '1');
            set_status(ui_, "Spawn type: %s", type_name(ui_.spawn.type));
            break;

        case 'a':
        case 'A':
            spawn_.spawnAtCursor(universe_, ui_, selection_);
            break;

        case 'd':
        case 'D':
            spawn_.deleteSelected(universe_, selection_, ui_);
            break;

        case 'r':
        case 'R':
            reset_universe_();
            break;

        case 'f':
        case 'F':
            if (selection_.follow_selected) {
                camera_.syncTarget(universe_, selection_);
            }
            selection_.follow_selected = !selection_.follow_selected;
            if (selection_.follow_selected &&
                (selection_.selected_body_index < 0 ||
                 selection_.selected_body_index >= MAX_OBJECTS ||
                 !universe_.isActive(selection_.selected_body_index))) {
                selection_.follow_selected = false;
                set_status(ui_, "Select a body before following it");
            } else {
                set_status(ui_, selection_.follow_selected ? "Camera following selected body"
                                                           : "Camera follow off");
            }
            break;

        case 'v':
        case 'V':
            ui_.spawn.velocity_mode = (ui_.spawn.velocity_mode + 1) % 3;
            if (ui_.spawn.velocity_mode == 0) set_status(ui_, "Spawn velocity: orbital");
            else if (ui_.spawn.velocity_mode == 1) set_status(ui_, "Spawn velocity: retrograde");
            else set_status(ui_, "Spawn velocity: zero");
            break;

        case '[':
            ui_.spawn.mass_percent = clamp_int(ui_.spawn.mass_percent - 25, 25, 400);
            set_status(ui_, "Spawn mass scale: %.2fx", ui_.spawn.mass_percent / 100.0);
            break;

        case ']':
            ui_.spawn.mass_percent = clamp_int(ui_.spawn.mass_percent + 25, 25, 400);
            set_status(ui_, "Spawn mass scale: %.2fx", ui_.spawn.mass_percent / 100.0);
            break;

        case ',':
            ui_.spawn.radius_percent = clamp_int(ui_.spawn.radius_percent - 10, 50, 200);
            set_status(ui_, "Spawn radius scale: %.2fx", ui_.spawn.radius_percent / 100.0);
            break;

        case '.':
            ui_.spawn.radius_percent = clamp_int(ui_.spawn.radius_percent + 10, 50, 200);
            set_status(ui_, "Spawn radius scale: %.2fx", ui_.spawn.radius_percent / 100.0);
            break;

        case 'l':
        case 'L':
            ui_.show_labels = !ui_.show_labels;
            set_status(ui_, ui_.show_labels ? "Labels on" : "Labels off");
            break;

        case 'g':
        case 'G':
            ui_.show_gravity_grid = !ui_.show_gravity_grid;
            set_status(ui_, ui_.show_gravity_grid
                              ? "Gravity grid on" : "Gravity grid off");
            break;

        case 't':
        case 'T':
            ui_.show_trails = !ui_.show_trails;
            set_status(ui_, ui_.show_trails ? "Orbit trails on" : "Orbit trails off");
            break;

        case 'y':
        case 'Y':
            ui_.show_prediction = !ui_.show_prediction;
            set_status(ui_, ui_.show_prediction
                              ? "Trajectory prediction on" : "Trajectory prediction off");
            break;

        case 'j':
        case 'J':
            ui_.show_thermal = !ui_.show_thermal;
            set_status(ui_, ui_.show_thermal ? "Thermal colours on" : "Thermal colours off");
            break;

        case 'k':
        case 'K':
            ui_.show_tidal = !ui_.show_tidal;
            set_status(ui_, ui_.show_tidal ? "Tidal deformation on" : "Tidal deformation off");
            break;

        case 'u':
        case 'U':
            ui_.show_dust = !ui_.show_dust;
            set_status(ui_, ui_.show_dust ? "Dust on" : "Dust off");
            break;

        case 'o':
        case 'O':
            promote_dust();
            break;

        case 'x':
        case 'X':
            ui_.show_collision_debug = !ui_.show_collision_debug;
            set_status(ui_, ui_.show_collision_debug
                              ? "Collision spheres on"
                              : "Collision spheres off");
            break;

        case 'c':
        case 'C':
            simulation_.collision_mode =
                (simulation_.collision_mode == CollisionMode::Realistic)
                    ? CollisionMode::Classic : CollisionMode::Realistic;
            set_status(ui_, simulation_.collision_mode == CollisionMode::Realistic
                              ? "Collision mode: realistic" : "Collision mode: classic merge");
            break;

        case '0':
            simulation_.time_multiplier = 1.0;
            set_status(ui_, "Speed reset to 1.00x");
            break;

        default:
            break;
    }
}

void InputController::special(int key) {
    if (ui_.show_tutorial) {
        if (key == GLUT_KEY_F1) {
            ui_.show_tutorial = false;
            ui_.guided_tour = false;
        } else if (key == GLUT_KEY_LEFT) {
            ui_.tutorial_section = (ui_.tutorial_section + 7) % 8;
            ui_.tutorial_scroll = 0;
        } else if (key == GLUT_KEY_RIGHT) {
            ui_.tutorial_section = (ui_.tutorial_section + 1) % 8;
            ui_.tutorial_scroll = 0;
        } else if (key == GLUT_KEY_UP) {
            if (ui_.tutorial_scroll > 0) --ui_.tutorial_scroll;
        } else if (key == GLUT_KEY_DOWN) {
            ++ui_.tutorial_scroll;
        } else if (key == GLUT_KEY_HOME) {
            ui_.tutorial_scroll = 0;
        }
        return;
    }
    if (key == GLUT_KEY_F1) {
        open_tutorial();
        return;
    }
    if (ui_.show_equations) {
        if (key == GLUT_KEY_LEFT) {
            ui_.equation_tab = (ui_.equation_tab + 7) % 8;
            ui_.equation_scroll = 0;
        } else if (key == GLUT_KEY_RIGHT) {
            ui_.equation_tab = (ui_.equation_tab + 1) % 8;
            ui_.equation_scroll = 0;
        } else if (key == GLUT_KEY_UP) {
            if (ui_.equation_scroll > 0) --ui_.equation_scroll;
        } else if (key == GLUT_KEY_DOWN) {
            ++ui_.equation_scroll;
        } else if (key == GLUT_KEY_HOME) {
            ui_.equation_scroll = 0;
        }
        return;
    }
    if (ui_.show_editor) {
        if (key == GLUT_KEY_UP) {
            ui_.editor_field = (ui_.editor_field + EDITOR_FIELD_COUNT - 1) %
                               EDITOR_FIELD_COUNT;
            ui_.editor_typing = false;
            ui_.editor_buffer[0] = '\0';
        } else if (key == GLUT_KEY_DOWN) {
            ui_.editor_field = (ui_.editor_field + 1) % EDITOR_FIELD_COUNT;
            ui_.editor_typing = false;
            ui_.editor_buffer[0] = '\0';
        } else if (key == GLUT_KEY_LEFT) {
            adjust_editor_field(-1, false);
        } else if (key == GLUT_KEY_RIGHT) {
            adjust_editor_field(1, false);
        } else if (key == GLUT_KEY_PAGE_UP) {
            adjust_editor_field(-1, true);
        } else if (key == GLUT_KEY_PAGE_DOWN) {
            adjust_editor_field(1, true);
        } else if (key == GLUT_KEY_HOME) {
            ui_.editor_field = 0;
        }
        return;
    }
    if (key == GLUT_KEY_UP) speed_up();
    else if (key == GLUT_KEY_DOWN) speed_down();
    else if (key == GLUT_KEY_HOME) {
        camera_.reset(selection_);
        set_status(ui_, "Camera reset");
    } else if (key == GLUT_KEY_PAGE_UP) {
        camera_.zoom(1.0);
    } else if (key == GLUT_KEY_PAGE_DOWN) {
        camera_.zoom(-1.0);
    }
}

} // namespace spacesim
