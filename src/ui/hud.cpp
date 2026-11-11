#include "ui/hud.h"

#include "core/config.h"
#include "core/math_utils.h"
#include "core/physics_metrics.h"
#include "core/state.h"
#include "core/universe.h"
#include "physics/impact_model.h"
#include "render/scene_renderer.h"
#include "ui/body_editor.h"
#include "ui/layout.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace spacesim {

namespace {

void format_number(double value, char *out, int size) {
    double magnitude = std::fabs(value);
    if (magnitude < 1.0e-12) {
        std::snprintf(out, size, "0.000");
    } else if (magnitude >= 1.0e9) {
        std::snprintf(out, size, "%.3e", value);
    } else if (magnitude >= 1000.0) {
        std::snprintf(out, size, "%.0f", value);
    } else if (magnitude >= 10.0) {
        std::snprintf(out, size, "%.1f", value);
    } else if (magnitude >= 0.001) {
        std::snprintf(out, size, "%.3f", value);
    } else {
        std::snprintf(out, size, "%.2e", value);
    }
}

} // namespace

bool Hud::init() {
    return text_.init();
}

void Hud::drawProjectedLabels(const SceneRenderer &renderer, const UiState &ui) {
    struct LabelRect {
        float x, y, w, h;
    };
    LabelRect drawn[MAX_OBJECTS];
    int drawn_count = 0;
    const float scale_base = ui.ui_scale;

    // Selected labels always win; planet/moon labels come next, and the Sun
    // label is allowed to disappear when it would cover an inner planet.
    for (int pass = 2; pass >= 0; --pass) {
        for (int i = 0; i < renderer.labelCount(); ++i) {
            const ScreenLabel &label = renderer.labels()[i];
            if (label.priority != pass) continue;

            float scale = (label.selected ? 1.0f : 0.88f) * scale_base;
            float width = text_.text_width(label.text, scale);
            if (label.x + width + 8.0f * scale_base > ui.sim_width) continue;

            float label_height = 44.0f * scale + 2.0f * scale_base;
            LabelRect rect = {static_cast<float>(label.x) - 2.0f * scale_base,
                              static_cast<float>(label.y) - 2.0f * scale_base,
                              width + 16.0f * scale_base,
                              label_height + 4.0f * scale_base};
            bool overlaps = false;
            for (int r = 0; r < drawn_count; ++r) {
                if (rect.x < drawn[r].x + drawn[r].w &&
                    rect.x + rect.w > drawn[r].x &&
                    rect.y < drawn[r].y + drawn[r].h &&
                    rect.y + rect.h > drawn[r].y) {
                    overlaps = true;
                    break;
                }
            }
            if (overlaps) continue;
            if (drawn_count < MAX_OBJECTS) drawn[drawn_count++] = rect;

            text_.draw_rect(static_cast<float>(label.x), static_cast<float>(label.y),
                            width + 12.0f * scale_base, label_height,
                            0.01f, 0.02f, 0.05f, 0.58f);
            text_.draw_text(static_cast<float>(label.x) + 4.0f * scale_base,
                            static_cast<float>(label.y) - 1.0f * scale_base,
                            label.text, scale, label.color[0], label.color[1],
                            label.color[2], 1.0f);
        }
    }
}

float Hud::drawWrapped(float x, float y, float max_width, const char *text,
                       float scale, float line_height, int max_lines,
                       float r, float g, float b) {
    if (text == nullptr || max_lines <= 0) return y;
    char line[256];
    int line_len = 0;
    float line_width = 0.0f;
    int lines = 0;
    const char *p = text;

    while (*p != '\0' && lines < max_lines) {
        while (*p == ' ') ++p;
        if (*p == '\0') break;

        const char *word_start = p;
        while (*p != '\0' && *p != ' ') ++p;
        int word_len = static_cast<int>(p - word_start);
        char word[128];
        int copy_len = word_len < 127 ? word_len : 127;
        std::memcpy(word, word_start, static_cast<size_t>(copy_len));
        word[copy_len] = '\0';
        float word_width = text_.text_width(word, scale);
        float space_width = line_len > 0 ? text_.text_width(" ", scale) : 0.0f;

        if (line_len > 0 && line_width + space_width + word_width > max_width) {
            line[line_len] = '\0';
            text_.draw_text(x, y, line, scale, r, g, b, 1.0f);
            y -= line_height;
            ++lines;
            line_len = 0;
            line_width = 0.0f;
            if (lines >= max_lines) break;
        }
        if (line_len > 0 && line_len < 255) {
            line[line_len++] = ' ';
            line_width += space_width;
        }
        for (int i = 0; i < copy_len && line_len < 255; ++i) {
            line[line_len++] = word[i];
        }
        line_width += word_width;
    }
    if (line_len > 0 && lines < max_lines) {
        line[line_len] = '\0';
        text_.draw_text(x, y, line, scale, r, g, b, 1.0f);
        y -= line_height;
    }
    return y;
}

void Hud::drawTutorial(const Universe &universe, const UiState &ui) {
    (void)universe;
    const float s = ui.ui_scale;
    const float px = 45.0f * s;
    const float py = 30.0f * s;
    const float pw = static_cast<float>(ui.win_width) - 90.0f * s;
    const float ph = static_cast<float>(ui.win_height) - 60.0f * s;
    text_.draw_rect(px, py, pw, ph, 0.015f, 0.025f, 0.055f, 0.97f);
    text_.draw_text(px + 30.0f * s, py + ph - 58.0f * s, "TUTORIAL & CONTROLS",
                    1.0f * s, 1.00f, 0.85f, 0.35f);

    const char *sections[8] = {
        "GETTING STARTED", "CAMERA", "TIME", "SPAWNING",
        "BODY EDITOR", "EQUATION LAB", "TIDAL / THERMAL", "DUST / KEYS"
    };
    float column_w = (pw - 56.0f * s) / 4.0f;
    float tab_w = std::max(100.0f * s, column_w - 12.0f * s);
    float tab_scale = (tab_w < 200.0f * s) ? 0.78f :
                      ((tab_w < 300.0f * s) ? 0.88f : 1.0f);
    tab_scale *= s;
    for (int i = 0; i < 8; ++i) {
        int col = i % 4;
        int row = i / 4;
        float tx = px + 28.0f * s + static_cast<float>(col) * column_w;
        float ty = py + ph - 122.0f * s - static_cast<float>(row) * 44.0f * s;
        bool active = (ui.tutorial_section == i);
        text_.draw_rect(tx, ty, tab_w, 40.0f * s,
                        active ? 0.16f : 0.05f,
                        active ? 0.22f : 0.07f,
                        active ? 0.38f : 0.12f, 0.95f);
        text_.draw_text(tx + 8.0f * s, ty + 7.0f * s, sections[i], tab_scale,
                        active ? 1.0f : 0.62f,
                        active ? 0.92f : 0.70f,
                        active ? 0.55f : 0.80f);
    }

    std::vector<std::string> lines;
    switch (ui.tutorial_section) {
        case 0:
            lines.push_back("Space Simulator is a real-time N-body gravity sandbox.");
            lines.push_back("Gravity is computed between every pair of active bodies.");
            lines.push_back("Orbits are not animations; they emerge from the physics.");
            lines.push_back("1. Rotate with left-drag and zoom with the wheel.");
            lines.push_back("2. Click a body to inspect it.");
            lines.push_back("3. Press E for equations and B to edit the selected body.");
            lines.push_back("4. Press H or F1 whenever you need this page.");
            break;
        case 1:
            lines.push_back("Left-drag: rotate the camera.");
            lines.push_back("Right-drag: pan the camera.");
            lines.push_back("Wheel, PageUp, PageDown: zoom in and out.");
            lines.push_back("F: follow the selected body.");
            lines.push_back("Home: reset the camera.");
            lines.push_back("L: toggle body labels.");
            lines.push_back("X: show effective collision spheres.");
            lines.push_back("Click empty space to clear the selection.");
            break;
        case 2:
            lines.push_back("Space / P: pause or resume.");
            lines.push_back("+ / - or Up / Down: change simulation speed.");
            lines.push_back("0: reset speed to 1.00x.");
            lines.push_back("R: reset the entire universe and camera.");
            lines.push_back("C: switch realistic and classic collision modes.");
            lines.push_back("The HUD shows body count, FPS, speed, and simulation time.");
            break;
        case 3:
            lines.push_back("1-7: select Star, Planet, Moon, Asteroid, Meteor, Comet, Black Hole.");
            lines.push_back("A: add the selected type at the mouse cursor.");
            lines.push_back("D: delete the selected body.");
            lines.push_back("V: cycle orbital, retrograde, and co-moving zero velocity.");
            lines.push_back("[ / ]: adjust spawn mass scale.");
            lines.push_back(", / .: adjust spawn radius scale.");
            lines.push_back("Spawn placement avoids visual overlaps automatically.");
            break;
        case 4:
            lines.push_back("B: open the Body Editor for the selected body.");
            lines.push_back("Up / Down: select a field.");
            lines.push_back("Left / Right: adjust; PageUp/PageDown: coarse.");
            lines.push_back("Enter: type a numeric value; Enter again applies it.");
            lines.push_back("S: spawn a custom body from the edited values.");
            lines.push_back("R: reset the selected body's appearance to type defaults.");
            lines.push_back("Visual radius is separate from physical collision radius.");
            lines.push_back("Volume is derived from mass and density.");
            break;
        case 5:
            lines.push_back("E: open or close the Equation Lab.");
            lines.push_back("A is the selected body; B is the comparison body.");
            lines.push_back("N or Shift+left-click: choose body B.");
            lines.push_back("Tab or Left/Right: change equations tab.");
            lines.push_back("Up/Down: scroll the equations.");
            lines.push_back("Tabs include Pair, Orbit, Thermal, Tidal, Impact, Black Hole, Body.");
            lines.push_back("The EVERYTHING tab lists every interaction on the selected body.");
            break;
        case 6:
            lines.push_back("Temperature uses stellar flux, albedo, greenhouse, tidal heating, and impacts.");
            lines.push_back("Thermal inertia makes heating and cooling gradual.");
            lines.push_back("Tidal stress, tidal heating, Roche limits, and Hill radii are computed per body.");
            lines.push_back("Tidal locking drives moon spin toward synchronous rotation.");
            lines.push_back("Hot bodies glow; cold bodies can receive an icy tint.");
            lines.push_back("Use the Thermal and Tidal tabs for live values.");
            break;
        case 7:
            lines.push_back("The Keplerian belt contains 5,000 visual particles.");
            lines.push_back("Press U to toggle dust; dust responds to gravity and planets.");
            lines.push_back("Dust collisions create impact effects and event-log entries.");
            lines.push_back("Press O to promote the nearest dust particle to a physical body.");
            lines.push_back("F1 or H: tutorial/help.     E: equations.     B: body editor.");
            lines.push_back("Space: pause.  R: reset.  Q/Esc: quit.");
            lines.push_back("CLI: --tutorial --lab --editor --follow --collision-debug --lab-tab N");
            lines.push_back("CLI: --physical-collisions --grid --classic --screenshot [path]");
            break;
        default:
            break;
    }

    float content_top = py + ph - 205.0f * s;
    float content_bottom = py + (ui.guided_tour ? 226.0f : 108.0f) * s;
    int max_lines = 14;
    while (max_lines > 1 &&
           content_top - static_cast<float>(max_lines - 1) * 34.0f * s < content_bottom) {
        --max_lines;
    }
    int max_scroll = static_cast<int>(lines.size()) - max_lines;
    if (max_scroll < 0) max_scroll = 0;
    int start = clamp_int(ui.tutorial_scroll, 0, max_scroll);
    float content_scale = (pw < 1000.0f * s) ? 0.82f : 1.0f;
    content_scale *= s;
    float line_y = content_top;
    for (int i = start; i < static_cast<int>(lines.size()) && i < start + max_lines; ++i) {
        text_.draw_text(px + 34.0f * s, line_y, lines[i].c_str(), content_scale,
                        0.84f, 0.88f, 0.96f);
        line_y -= 34.0f * s;
    }

    if (ui.guided_tour) {
        const char *tour[12] = {
            "Rotate the camera with left-drag.",
            "Select a body with left-click.",
            "Pause with Space and change speed with +/-.",
            "Press E to open the Equation Lab.",
            "Shift-click a second body to choose B.",
            "Press B to open the Body Editor.",
            "Change mass, radius, or volume with Left/Right.",
            "Press S to spawn the edited custom body.",
            "Press X to show collision spheres.",
            "Press O to promote the nearest dust particle.",
            "Open the EVERYTHING tab in the Equation Lab.",
            "Press F1 or H to close the tutorial."
        };
        int step = ui.tour_step % 12;
        if (step < 0) step += 12;
        float box_y = py + 40.0f * s;
        text_.draw_rect(px + 30.0f * s, box_y, pw - 60.0f * s, 140.0f * s,
                        0.08f, 0.12f, 0.22f, 0.95f);
        char header[128];
        std::snprintf(header, sizeof(header), "GUIDED TOUR  -  Step %d of 12",
                      step + 1);
        text_.draw_text(px + 48.0f * s, box_y + 96.0f * s, header, 1.0f * s,
                        1.0f, 0.80f, 0.35f);
        text_.draw_text(px + 48.0f * s, box_y + 54.0f * s, tour[step], 1.0f * s,
                        0.90f, 0.93f, 1.0f);
        text_.draw_text(px + 48.0f * s, box_y + 10.0f * s,
                        "Enter/Space next   Backspace previous   T exit tour",
                        1.0f * s, 0.65f, 0.72f, 0.82f);
    } else {
        text_.draw_text(px + 30.0f * s, py + 54.0f * s,
                        "F1/Esc close   Left/Right section   Up/Down scroll   T guided tour",
                        1.0f * s, 0.62f, 0.68f, 0.78f);
    }
}

void Hud::drawEquationLab(const Universe &universe, const UiState &ui,
                          const SimulationState &simulation,
                          const SelectionState &selection) {
    const float s = ui.ui_scale;
    const float px = 45.0f * s;
    const float py = 30.0f * s;
    const float pw = static_cast<float>(ui.win_width) - 90.0f * s;
    const float ph = static_cast<float>(ui.win_height) - 60.0f * s;
    text_.draw_rect(px, py, pw, ph, 0.015f, 0.025f, 0.055f, 0.96f);
    text_.draw_rect(px, py + ph - 3.0f * s, pw, 3.0f * s,
                    0.35f, 0.45f, 0.7f, 0.9f);

    text_.draw_text(px + 30.0f * s, py + ph - 55.0f * s, "EQUATION LAB", 1.05f * s,
                    1.00f, 0.85f, 0.35f);

    const char *tabs[8] = {"PAIR", "ORBIT", "THERMAL", "TIDAL",
                           "IMPACT", "BLACK HOLE", "BODY", "EVERYTHING"};
    float tab_gap = 8.0f * s;
    float tab_w = (pw - 56.0f * s - 7.0f * tab_gap) / 8.0f;
    if (tab_w > 172.0f * s) tab_w = 172.0f * s;
    if (tab_w < 80.0f * s) tab_w = 80.0f * s;
    float tab_scale = (tab_w < 120.0f * s) ? 0.70f :
                      ((tab_w < 150.0f * s) ? 0.82f : 1.0f);
    tab_scale *= s;
    for (int i = 0; i < 8; ++i) {
        float tx = px + 28.0f * s + static_cast<float>(i) * (tab_w + tab_gap);
        float ty = py + ph - 108.0f * s;
        bool active = (ui.equation_tab == i);
        text_.draw_rect(tx, ty, tab_w, 42.0f * s,
                        active ? 0.16f : 0.05f,
                        active ? 0.22f : 0.07f,
                        active ? 0.38f : 0.12f, 0.95f);
        text_.draw_text(tx + 8.0f * s, ty + 8.0f * s, tabs[i], tab_scale,
                        active ? 1.0f : 0.62f,
                        active ? 0.92f : 0.70f,
                        active ? 0.55f : 0.80f);
    }

    int a = selection.selected_body_index;
    if (a < 0 || a >= MAX_OBJECTS || !universe.isActive(a)) {
        text_.draw_text(px + 30.0f * s, py + ph - 165.0f * s,
                        "Select a body first.", 1.0f * s, 1.0f, 0.5f, 0.4f);
        text_.draw_text(px + 30.0f * s, py + 50.0f * s,
                        "E close   Left/Right tab   Up/Down scroll", 1.0f * s,
                        0.62f, 0.68f, 0.78f);
        return;
    }
    int b = ui.equation_body_b;
    if (b < 0 || b >= MAX_OBJECTS || !universe.isActive(b) || b == a) {
        b = -1;
        for (int i = 0; i < MAX_OBJECTS; ++i) {
            if (i != a && universe.isActive(i)) { b = i; break; }
        }
    }
    const CelestialBody &A = universe.body(a);
    const CelestialBody &B = universe.body(b >= 0 ? b : a);

    text_.draw_text(px + 30.0f * s, py + ph - 158.0f * s,
                    "A", 1.0f * s, 0.75f, 0.88f, 1.0f);
    text_.draw_text(px + 60.0f * s, py + ph - 158.0f * s, A.name, 1.0f * s,
                    0.96f, 0.96f, 1.0f);
    text_.draw_text(px + 360.0f * s, py + ph - 158.0f * s, "B", 1.0f * s,
                    0.75f, 0.88f, 1.0f);
    text_.draw_text(px + 390.0f * s, py + ph - 158.0f * s,
                    b >= 0 ? B.name : "none", 1.0f * s, 0.96f, 0.96f, 1.0f);

    std::vector<std::string> lines;
    char buffer[256];
    const char *outcome_names[6] = {"Merge", "Bounce", "Graze",
                                    "Fragment", "Disrupt", "Absorb"};

    if (ui.equation_tab == 0) {
        PairMetrics p = compute_pair_metrics(A, B);
        std::snprintf(buffer, sizeof(buffer), "F = G m1 m2 / (r + eps)^2");
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "  = %.6g sim force units", p.force);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "a(A) = G m(B) / (r + eps)^2 = %.6g", p.acceleration_a);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "a(B) = G m(A) / (r + eps)^2 = %.6g", p.acceleration_b);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "distance r = %.6g sim units", p.distance);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "potential U = -G m1 m2/(r+eps) = %.6g", p.potential);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "escape v = sqrt(2G(m1+m2)/(r+eps)) = %.6g", p.escape_velocity);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "relative speed = %.6g sim units/time", p.relative_speed);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "direction A->B = (%.3f, %.3f, %.3f)",
                      p.direction[0], p.direction[1], p.direction[2]);
        lines.push_back(buffer);
    } else if (ui.equation_tab == 1) {
        OrbitMetrics o = compute_orbit_metrics(A, B);
        lines.push_back("mu = G(m1 + m2)");
        if (o.valid) {
            std::snprintf(buffer, sizeof(buffer), "specific energy = %.6g", o.specific_energy);
            lines.push_back(buffer);
            std::snprintf(buffer, sizeof(buffer), "angular momentum h = %.6g", o.angular_momentum);
            lines.push_back(buffer);
            std::snprintf(buffer, sizeof(buffer), "eccentricity e = %.6g", o.eccentricity);
            lines.push_back(buffer);
            std::snprintf(buffer, sizeof(buffer), "semi-major axis a = %.6g", o.semi_major_axis);
            lines.push_back(buffer);
            std::snprintf(buffer, sizeof(buffer), "periapsis = %.6g   apoapsis = %.6g",
                          o.periapsis, o.apoapsis);
            lines.push_back(buffer);
            std::snprintf(buffer, sizeof(buffer), "period T = %.6g time units", o.period);
            lines.push_back(buffer);
            lines.push_back("v^2 = mu(2/r - 1/a)");
        } else {
            lines.push_back("No bound orbit for this pair.");
        }
    } else if (ui.equation_tab == 2) {
        std::snprintf(buffer, sizeof(buffer), "A temperature = %.1f K", A.temperature_k);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "A equilibrium = %.1f K", A.equilibrium_temperature_k);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "A greenhouse = %.1f K   tidal = %.1f K   impact = %.1f K",
                      A.greenhouse_k, A.tidal_heating_k, A.impact_heating_k);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "B temperature = %.1f K", B.temperature_k);
        lines.push_back(buffer);
        if (B.type == BodyType::Star) {
            std::snprintf(buffer, sizeof(buffer), "B luminosity ~ M^3.5 = %.6g",
                          std::pow(B.mass, 3.5));
            lines.push_back(buffer);
        } else {
            lines.push_back("B is not a star; it provides no luminous flux.");
        }
        lines.push_back("L = 4 pi R^2 sigma T^4");
        lines.push_back("T_eq = [flux(1 - albedo)/(4 sigma)]^(1/4)");
    } else if (ui.equation_tab == 3) {
        int heaviest = universe.heaviestBody();
        double star_mass = heaviest >= 0 ? universe.body(heaviest).mass : 1.0;
        double orbit_radius = std::sqrt(A.x * A.x + A.y * A.y + A.z * A.z);
        TidalMetrics t = compute_tidal_metrics(A, B, star_mass, orbit_radius);
        std::snprintf(buffer, sizeof(buffer), "tidal acceleration = 2GMR/d^3 = %.6e",
                      t.acceleration);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "tidal ratio = %.6e", t.ratio);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "Roche limit = %.6g sim units", t.roche_limit);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "Hill radius = %.6g sim units", t.hill_radius);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "tidal heating = %.1f K", A.tidal_heating_k);
        lines.push_back(buffer);
        lines.push_back("R_Roche = R_p (2 M_p / M_b)^(1/3)");
        lines.push_back("R_Hill = a (m / (3M))^(1/3)");
    } else if (ui.equation_tab == 4) {
        bool moon_pair = (universe.moon(a).parent_index >= 0 ||
                          (b >= 0 && universe.moon(b).parent_index >= 0));
        bool pair_physical = simulation.physical_collisions || moon_pair ||
                             A.radius < 0.008 || B.radius < 0.008;
        ImpactResult im = classify_impact(A, B, pair_physical);
        std::snprintf(buffer, sizeof(buffer), "collision mode: %s",
                      pair_physical ? "physical radii" : "visual-surface");
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "effective radii A/B = %.6g / %.6g",
                      effective_collision_radius(A, pair_physical),
                      effective_collision_radius(B, pair_physical));
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "collision scales A/B = %.2f / %.2f",
                      A.collision_scale, B.collision_scale);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "relative speed = %.6g", im.relative_speed);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "impact parameter b/(r1+r2) = %.6g",
                      im.impact_parameter);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "specific impact energy Q = %.6g",
                      im.energy_ratio);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "predicted outcome: %s",
                      outcome_names[static_cast<int>(im.outcome)]);
        lines.push_back(buffer);
        lines.push_back("b = |r x v| / |v|");
        lines.push_back("Q = 0.5 mu v^2 / (m1 + m2)");
    } else if (ui.equation_tab == 5) {
        const CelestialBody &bh = (A.type == BodyType::BlackHole) ? A : B;
        if (bh.type != BodyType::BlackHole) {
            lines.push_back("Neither A nor B is a black hole.");
        } else {
            BlackHoleMetrics m = compute_black_hole_metrics(bh);
            std::snprintf(buffer, sizeof(buffer), "horizon radius = %.6g sim units", m.horizon);
            lines.push_back(buffer);
            std::snprintf(buffer, sizeof(buffer), "photon sphere = 1.5 r_s = %.6g", m.photon_sphere);
            lines.push_back(buffer);
            std::snprintf(buffer, sizeof(buffer), "ISCO = 3 r_s = %.6g", m.isco);
            lines.push_back(buffer);
            std::snprintf(buffer, sizeof(buffer), "time dilation at 2 r_s = %.6g", m.time_dilation);
            lines.push_back(buffer);
            lines.push_back("r_s = 2GM/c^2 (simulation-equivalent scale)");
        }
    } else if (ui.equation_tab == 6) {
        BodyMetrics m = compute_body_metrics(A);
        std::snprintf(buffer, sizeof(buffer), "mass = %.6g Earth masses", A.mass / EARTH_MASS_SIM);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "radius = %.6g sim units", A.radius);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "density = %.6g kg/m3", A.density_kg_m3);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "surface gravity = %.6g", m.surface_gravity);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "escape velocity = %.6g", m.escape_velocity);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "axial tilt = %.2f deg   flattening = %.3f",
                      A.axial_tilt_deg, A.flattening);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "composition = %s",
                      composition_name(A.composition));
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "collision scale = %.2f   effective radius = %.6g",
                      A.collision_scale,
                      effective_collision_radius(A, simulation.physical_collisions));
        lines.push_back(buffer);
    } else {
        struct Contribution {
            int index;
            PairMetrics pair;
            double tidal;
        };
        std::vector<Contribution> contributions;
        double net_force[3] = {0.0, 0.0, 0.0};
        double net_accel[3] = {0.0, 0.0, 0.0};
        double net_tidal[3] = {0.0, 0.0, 0.0};
        double kinetic = 0.0;
        double potential = 0.0;
        double momentum[3] = {0.0, 0.0, 0.0};
        for (int i = 0; i < MAX_OBJECTS; ++i) {
            if (!universe.isActive(i)) continue;
            const CelestialBody &body = universe.body(i);
            double speed_sq = body.vx * body.vx + body.vy * body.vy +
                              body.vz * body.vz;
            kinetic += 0.5 * body.mass * speed_sq;
            momentum[0] += body.mass * body.vx;
            momentum[1] += body.mass * body.vy;
            momentum[2] += body.mass * body.vz;
            for (int j = i + 1; j < MAX_OBJECTS; ++j) {
                if (!universe.isActive(j)) continue;
                const CelestialBody &other = universe.body(j);
                double dx = other.x - body.x;
                double dy = other.y - body.y;
                double dz = other.z - body.z;
                double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
                potential -= G_CONSTANT * body.mass * other.mass /
                             (distance + SOFTENING);
            }
        }
        for (int i = 0; i < MAX_OBJECTS; ++i) {
            if (!universe.isActive(i) || i == a) continue;
            const CelestialBody &other = universe.body(i);
            PairMetrics p = compute_pair_metrics(A, other);
            double tidal = 2.0 * G_CONSTANT * other.mass * A.radius /
                           (p.distance * p.distance * p.distance + 1.0e-18);
            contributions.push_back(Contribution{i, p, tidal});
            net_force[0] += p.direction[0] * p.force;
            net_force[1] += p.direction[1] * p.force;
            net_force[2] += p.direction[2] * p.force;
            net_accel[0] += p.direction[0] * p.acceleration_a;
            net_accel[1] += p.direction[1] * p.acceleration_a;
            net_accel[2] += p.direction[2] * p.acceleration_a;
            net_tidal[0] += p.direction[0] * tidal;
            net_tidal[1] += p.direction[1] * tidal;
            net_tidal[2] += p.direction[2] * tidal;
        }
        std::sort(contributions.begin(), contributions.end(),
                  [](const Contribution &lhs, const Contribution &rhs) {
                      return lhs.pair.force > rhs.pair.force;
                  });
        lines.push_back("EVERYTHING: selected body interactions");
        std::snprintf(buffer, sizeof(buffer), "net force = (%.5g, %.5g, %.5g)",
                      net_force[0], net_force[1], net_force[2]);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "net acceleration = (%.5g, %.5g, %.5g)",
                      net_accel[0], net_accel[1], net_accel[2]);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer), "net tidal acceleration = (%.5g, %.5g, %.5g)",
                      net_tidal[0], net_tidal[1], net_tidal[2]);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer),
                      "system energy: K=%.5g U=%.5g E=%.5g",
                      kinetic, potential, kinetic + potential);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer),
                      "system momentum = (%.5g, %.5g, %.5g)",
                      momentum[0], momentum[1], momentum[2]);
        lines.push_back(buffer);
        lines.push_back("All gravitational contributors (sorted by force):");
        for (int i = 0; i < static_cast<int>(contributions.size()); ++i) {
            const CelestialBody &other = universe.body(contributions[i].index);
            const PairMetrics &p = contributions[i].pair;
            std::snprintf(buffer, sizeof(buffer),
                          "  %-10s d=%.4g F=%.4g a=%.4g T=%.4g",
                          other.name, p.distance, p.force, p.acceleration_a,
                          contributions[i].tidal);
            lines.push_back(buffer);
        }
        OrbitMetrics o = compute_orbit_metrics(A, B);
        if (o.valid) {
            std::snprintf(buffer, sizeof(buffer),
                          "orbit: a=%.4g e=%.4g energy=%.4g %s",
                          o.semi_major_axis, o.eccentricity, o.specific_energy,
                          o.specific_energy >= 0.0 ? "UNBOUND" : "bound");
        } else {
            std::snprintf(buffer, sizeof(buffer), "orbit: no bound conic with B");
        }
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer),
                      "temperature: surface %.1f K  eq %.1f K  greenhouse %.1f K",
                      A.temperature_k, A.equilibrium_temperature_k, A.greenhouse_k);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer),
                      "tidal heating %.1f K  impact heating %.1f K",
                      A.tidal_heating_k, A.impact_heating_k);
        lines.push_back(buffer);
        int heaviest = universe.heaviestBody();
        double star_mass = heaviest >= 0 ? universe.body(heaviest).mass : 1.0;
        double orbit_radius = std::sqrt(A.x * A.x + A.y * A.y + A.z * A.z);
        TidalMetrics tide = compute_tidal_metrics(A, B, star_mass, orbit_radius);
        std::snprintf(buffer, sizeof(buffer),
                      "tide: a=%.4e ratio=%.4e Roche=%.4g Hill=%.4g",
                      tide.acceleration, tide.ratio, tide.roche_limit,
                      tide.hill_radius);
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer),
                      "rotation: spin %.3g  tilt %.2f deg  flattening %.3f  tidal lock %s",
                      A.spin_rate, A.axial_tilt_deg, A.flattening,
                      A.tidal_lock ? "yes" : "no");
        lines.push_back(buffer);
        std::snprintf(buffer, sizeof(buffer),
                      "collision: effective r %.4g  relative v %.4g",
                      effective_collision_radius(A, simulation.physical_collisions),
                      compute_pair_metrics(A, B).relative_speed);
        lines.push_back(buffer);
        if (A.type == BodyType::BlackHole) {
            BlackHoleMetrics bhm = compute_black_hole_metrics(A);
            std::snprintf(buffer, sizeof(buffer),
                          "black hole: horizon %.4g photon %.4g ISCO %.4g",
                          bhm.horizon, bhm.photon_sphere, bhm.isco);
            lines.push_back(buffer);
        }
    }

    int max_lines = 13;
    float content_top = py + ph - 205.0f * s;
    float content_bottom = py + 110.0f * s;
    while (max_lines > 1 &&
           content_top - static_cast<float>(max_lines - 1) * 34.0f * s < content_bottom) {
        --max_lines;
    }
    int max_scroll = static_cast<int>(lines.size()) - max_lines;
    if (max_scroll < 0) max_scroll = 0;
    int start = clamp_int(ui.equation_scroll, 0, max_scroll);
    float content_scale = (pw < 1000.0f * s) ? 0.82f : 1.0f;
    content_scale *= s;
    float line_y = content_top;
    for (int i = start; i < static_cast<int>(lines.size()) && i < start + max_lines; ++i) {
        text_.draw_text(px + 34.0f * s, line_y, lines[i].c_str(), content_scale,
                        0.84f, 0.88f, 0.96f);
        line_y -= 34.0f * s;
    }

    text_.draw_text(px + 30.0f * s, py + 48.0f * s,
                    "E close   Left/Right tab   Up/Down scroll   N cycle B   Shift+Click set B",
                    1.0f * s, 0.62f, 0.68f, 0.78f);
}

void Hud::drawBodyEditor(const Universe &universe, const UiState &ui,
                         const SelectionState &selection) {
    const float s = ui.ui_scale;
    const float px = 45.0f * s;
    const float py = 30.0f * s;
    const float pw = static_cast<float>(ui.win_width) - 90.0f * s;
    const float ph = static_cast<float>(ui.win_height) - 60.0f * s;
    text_.draw_rect(px, py, pw, ph, 0.015f, 0.025f, 0.055f, 0.96f);
    text_.draw_text(px + 30.0f * s, py + ph - 55.0f * s, "BODY EDITOR", 1.0f * s,
                    1.00f, 0.85f, 0.35f);
    if (selection.selected_body_index < 0 ||
        selection.selected_body_index >= MAX_OBJECTS ||
        !universe.isActive(selection.selected_body_index)) {
        text_.draw_text(px + 30.0f * s, py + ph - 130.0f * s,
                        "Select a body to edit, or press Esc.", 1.0f * s,
                        1.0f, 0.5f, 0.4f);
        text_.draw_text(px + 30.0f * s, py + 48.0f * s,
                        "B/Esc close", 1.0f * s, 0.62f, 0.68f, 0.78f);
        return;
    }
    const CelestialBody &body = universe.body(selection.selected_body_index);
    text_.draw_text(px + 30.0f * s, py + ph - 110.0f * s, body.name, 1.0f * s,
                    0.96f, 0.96f, 1.0f);
    char sub[192];
    std::snprintf(sub, sizeof(sub), "%s  |  %s  |  effective collision radius %.4g",
                  type_name(body.type), composition_name(body.composition),
                  effective_collision_radius(body, false));
    text_.draw_text(px + 30.0f * s, py + ph - 155.0f * s, sub, 1.0f * s,
                    0.62f, 0.76f, 1.0f);

    const float line_height = 30.0f * s;
    int max_visible = 14;
    int fit_fields = static_cast<int>((ph - 280.0f * s) / line_height);
    if (fit_fields < max_visible) max_visible = fit_fields;
    if (max_visible < 6) max_visible = 6;
    int first_field = 0;
    if (ui.editor_field >= max_visible) {
        first_field = ui.editor_field - max_visible + 1;
    }
    int last_field = std::min(static_cast<int>(EDITOR_FIELD_COUNT),
                              first_field + max_visible);
    float y = py + ph - 205.0f * s;
    for (int field = first_field; field < last_field; ++field) {
        bool selected = (field == ui.editor_field);
        if (selected) {
            text_.draw_rect(px + 22.0f * s, y - 4.0f * s, pw - 44.0f * s,
                            line_height, 0.12f, 0.18f, 0.34f, 0.8f);
        }
        char value[96];
        if (field == EDITOR_TYPE) {
            std::snprintf(value, sizeof(value), "%s", type_name(body.type));
        } else if (field == EDITOR_COMPOSITION) {
            std::snprintf(value, sizeof(value), "%s", composition_name(body.composition));
        } else {
            std::snprintf(value, sizeof(value), "%.6g", editor_field_value(body, field));
        }
        char line[192];
        std::snprintf(line, sizeof(line), "%-18s %-16s %s",
                      editor_field_name(field), value,
                      editor_field_unit(field));
        text_.draw_text(px + 34.0f * s, y, line, 1.0f * s,
                        selected ? 1.0f : 0.82f,
                        selected ? 0.92f : 0.86f,
                        selected ? 0.55f : 0.94f);
        y -= line_height;
    }

    float footer = py + 50.0f * s;
    if (ui.editor_typing) {
        std::snprintf(sub, sizeof(sub), "Typing: %s    Enter apply   Esc cancel",
                      ui.editor_buffer);
        text_.draw_text(px + 30.0f * s, footer + 30.0f * s, sub, 1.0f * s,
                        1.0f, 0.78f, 0.35f);
    } else {
        text_.draw_text(px + 30.0f * s, footer + 30.0f * s,
                        "Up/Down field   Left/Right adjust   Enter type value",
                        1.0f * s, 0.62f, 0.68f, 0.78f);
    }
    std::snprintf(sub, sizeof(sub), "Fields %d-%d of %d",
                  first_field + 1, last_field, static_cast<int>(EDITOR_FIELD_COUNT));
    text_.draw_text(px + pw - 260.0f * s, footer + 30.0f * s, sub, 1.0f * s,
                    0.55f, 0.65f, 0.78f);
    text_.draw_text(px + 30.0f * s, footer,
                    "S spawn custom body   R reset appearance   B/Esc close",
                    1.0f * s, 0.62f, 0.68f, 0.78f);
}

void Hud::draw(const Universe &universe, const UiState &ui,
               const SimulationState &simulation,
               const SelectionState &selection,
               const SceneRenderer &renderer) {
    glViewport(0, 0, ui.win_width, ui.win_height);
    text_.begin(ui.win_width, ui.win_height);
    glDisable(GL_DEPTH_TEST);
    if (ui.show_tutorial) {
        drawTutorial(universe, ui);
        text_.end();
        glEnable(GL_DEPTH_TEST);
        return;
    }
    if (ui.show_equations) {
        drawEquationLab(universe, ui, simulation, selection);
        text_.end();
        glEnable(GL_DEPTH_TEST);
        return;
    }
    if (ui.show_editor) {
        drawBodyEditor(universe, ui, selection);
        text_.end();
        glEnable(GL_DEPTH_TEST);
        return;
    }

    const float s = ui.ui_scale;
    const float margin = static_cast<float>(HUD_MARGIN) * s;
    const bool compact = (ui.win_height < 820);
    const bool narrow = (ui.sim_width < 760);
    const float title_scale = s;
    const float body_scale = narrow ? s * 0.95f : s;
    const float small_scale = narrow ? s * 0.90f : s;
    const float panel_scale = s;
    const float history_scale = s;
    const float line_step = 34.0f * s;
    char buffer[256];
    char number[48];

    text_.draw_rect(0.0f, static_cast<float>(ui.win_height) - 220.0f * s,
                    static_cast<float>(ui.sim_width), 220.0f * s,
                    0.02f, 0.03f, 0.07f, 0.40f);
    text_.draw_rect(0.0f, 0.0f, static_cast<float>(ui.win_width), 160.0f * s,
                    0.02f, 0.03f, 0.07f, 0.32f);

    drawProjectedLabels(renderer, ui);

    text_.draw_text(margin, static_cast<float>(ui.win_height) - 52.0f * s,
                    "SPACE SIMULATOR  |  V3.5", title_scale,
                    1.00f, 0.85f, 0.35f);
    if (!narrow) {
        text_.draw_text(margin, static_cast<float>(ui.win_height) - 90.0f * s,
                        "Gravity sandbox - click a body to inspect it", body_scale,
                        0.55f, 0.75f, 1.00f);
        std::snprintf(buffer, sizeof(buffer),
                      "Bodies: %d   FPS: %d   Speed: %.2fx%s   Sim time: %.1f   collisions: %s",
                      universe.count(), ui.fps, simulation.time_multiplier,
                      simulation.paused ? "   [PAUSED]" : "", simulation.sim_time,
                      simulation.collision_mode == CollisionMode::Realistic
                          ? "realistic" : "classic");
        text_.draw_text(margin, static_cast<float>(ui.win_height) - 126.0f * s,
                        buffer, body_scale, 0.70f, 0.85f, 0.75f);
    } else {
        std::snprintf(buffer, sizeof(buffer),
                      "Bodies: %d   FPS: %d   Speed: %.2fx%s",
                      universe.count(), ui.fps, simulation.time_multiplier,
                      simulation.paused ? " [PAUSED]" : "");
        text_.draw_text(margin, static_cast<float>(ui.win_height) - 90.0f * s,
                        buffer, body_scale, 0.70f, 0.85f, 0.75f);
        std::snprintf(buffer, sizeof(buffer), "Sim time: %.1f   collisions: %s",
                      simulation.sim_time,
                      simulation.collision_mode == CollisionMode::Realistic
                          ? "realistic" : "classic");
        text_.draw_text(margin, static_cast<float>(ui.win_height) - 126.0f * s,
                        buffer, body_scale, 0.70f, 0.85f, 0.75f);
    }

    const char *velocity_label = "orbital";
    if (ui.spawn.velocity_mode == 1) velocity_label = "retrograde";
    else if (ui.spawn.velocity_mode == 2) velocity_label = "zero / co-moving";
    std::snprintf(buffer, sizeof(buffer),
                  "Spawn: %s   mass x%.2f   radius x%.2f   velocity: %s",
                  type_name(ui.spawn.type),
                  static_cast<double>(ui.spawn.mass_percent) / 100.0,
                  static_cast<double>(ui.spawn.radius_percent) / 100.0,
                  velocity_label);
    text_.draw_text(margin, static_cast<float>(ui.win_height) - 160.0f * s, buffer,
                    body_scale, 0.70f, 0.70f, 0.85f);

    if (ui.status_frames > 0) {
        text_.draw_text(margin, static_cast<float>(ui.win_height) - 194.0f * s,
                        ui.status_message, body_scale, 1.00f, 0.75f, 0.30f);
    }

    if (!narrow) {
        text_.draw_text(margin, 124.0f * s,
                        "Space pause | +/- speed | 1-7 type | A add | D delete",
                        small_scale, 0.62f, 0.68f, 0.78f);
        text_.draw_text(margin, 90.0f * s,
                        "V velocity | R reset | E equations | B editor | F1 tutorial",
                        small_scale, 0.62f, 0.68f, 0.78f);
        text_.draw_text(margin, 56.0f * s,
                        "F follow | G grid | T trails | Y prediction | C collisions | X spheres",
                        small_scale, 0.62f, 0.68f, 0.78f);
        text_.draw_text(margin, 22.0f * s,
                        "U dust | O promote | Left-drag rotate | Wheel zoom | Right-drag pan",
                        small_scale, 0.62f, 0.68f, 0.78f);
    } else {
        float right_column = std::max(margin + 280.0f * s,
                                      static_cast<float>(ui.sim_width) * 0.56f);
        text_.draw_text(margin, 124.0f * s, "Space pause | +/- speed",
                        small_scale, 0.62f, 0.68f, 0.78f);
        text_.draw_text(right_column, 124.0f * s, "1-7 type | A add | D delete",
                        small_scale, 0.62f, 0.68f, 0.78f);
        text_.draw_text(margin, 90.0f * s, "R reset | E equations | B editor",
                        small_scale, 0.62f, 0.68f, 0.78f);
        text_.draw_text(right_column, 90.0f * s, "F1 tutorial | F follow | G grid",
                        small_scale, 0.62f, 0.68f, 0.78f);
        text_.draw_text(margin, 56.0f * s, "T trails | Y prediction | C collisions",
                        small_scale, 0.62f, 0.68f, 0.78f);
        text_.draw_text(right_column, 56.0f * s, "X spheres | U dust | O promote",
                        small_scale, 0.62f, 0.68f, 0.78f);
        text_.draw_text(margin, 22.0f * s, "Left-drag rotate | Wheel zoom",
                        small_scale, 0.62f, 0.68f, 0.78f);
        text_.draw_text(right_column, 22.0f * s, "Right-drag pan | V velocity",
                        small_scale, 0.62f, 0.68f, 0.78f);
    }

    const bool has_selection = (selection.selected_body_index >= 0 &&
                                selection.selected_body_index < MAX_OBJECTS &&
                                universe.isActive(selection.selected_body_index));
    const int panel_width = hud_panel_width(ui);
    int panel_height = static_cast<int>((has_selection ? 800.0f : 360.0f) * s);
    if (has_selection) {
        const CelestialBody &sel_body = universe.body(selection.selected_body_index);
        const MoonOrbit &sel_moon = universe.moon(selection.selected_body_index);
        if (sel_moon.parent_index >= 0) {
            panel_height = static_cast<int>(852.0f * s);
        } else if (sel_body.type == BodyType::BlackHole) {
            panel_height = static_cast<int>(820.0f * s);
        } else if (sel_body.type == BodyType::Comet) {
            panel_height = static_cast<int>(800.0f * s);
        }
    }
    const int panel_margin = static_cast<int>(HUD_MARGIN * s);
    panel_height = std::min(panel_height, ui.win_height - 2 * panel_margin);
    const int panel_x = ui.win_width - panel_width - panel_margin;
    const int panel_y = ui.win_height - panel_height - panel_margin;
    text_.draw_rect(static_cast<float>(panel_x), static_cast<float>(panel_y),
                    static_cast<float>(panel_width), static_cast<float>(panel_height),
                    0.03f, 0.05f, 0.10f, 0.86f);
    text_.draw_rect(static_cast<float>(panel_x), static_cast<float>(panel_y),
                    2.0f * s, static_cast<float>(panel_height),
                    0.25f, 0.35f, 0.55f, 0.9f);

    float x = static_cast<float>(panel_x) + 18.0f * s;
    float y = static_cast<float>(panel_y + panel_height) - 52.0f * s;

    if (has_selection) {
        const int index = selection.selected_body_index;
        const CelestialBody &b = universe.body(index);
        const MoonOrbit &moon_orbit = universe.moon(index);

        text_.draw_text(x, y, "SELECTED BODY", title_scale, 1.00f, 0.85f, 0.35f);
        y -= 48.0f * s;
        text_.draw_text(x, y, b.name, title_scale, 0.96f, 0.96f, 1.00f);
        y -= 40.0f * s;
        text_.draw_text(x, y, type_name(b.type), panel_scale, 0.60f, 0.75f, 0.95f);
        y -= line_step;

        format_number(b.mass / EARTH_MASS_SIM, number, sizeof(number));
        std::snprintf(buffer, sizeof(buffer), "Mass: %s Earth masses", number);
        text_.draw_text(x, y, buffer, panel_scale, 0.86f, 0.89f, 0.96f);
        y -= line_step;

        if (b.type == BodyType::BlackHole) {
            std::snprintf(buffer, sizeof(buffer), "Event horizon: %.3f sim units",
                          b.horizon_radius);
            text_.draw_text(x, y, buffer, panel_scale, 0.80f, 0.62f, 1.00f);
            y -= line_step;
            text_.draw_text(x, y, "Accretion disk: active", panel_scale,
                            1.00f, 0.55f, 0.25f);
            y -= line_step;
        } else {
            format_number(b.volume_ratio, number, sizeof(number));
            std::snprintf(buffer, sizeof(buffer), "Volume: %s Earths", number);
            text_.draw_text(x, y, buffer, panel_scale, 0.86f, 0.89f, 0.96f);
            y -= line_step;

            if (b.self_rotation_time > 0.0) {
                std::snprintf(buffer, sizeof(buffer), "Rotation: %.2f hours", b.self_rotation_time);
            } else if (b.self_rotation_time < 0.0) {
                std::snprintf(buffer, sizeof(buffer), "Rotation: %.2f hours (retrograde)",
                              -b.self_rotation_time);
            } else {
                std::snprintf(buffer, sizeof(buffer), "Rotation: n/a");
            }
            text_.draw_text(x, y, buffer, panel_scale, 0.86f, 0.89f, 0.96f);
            y -= line_step;

            if (b.orbital_period > 0.0) {
                std::snprintf(buffer, sizeof(buffer), "Orbital period: %.1f days", b.orbital_period);
            } else {
                std::snprintf(buffer, sizeof(buffer), "Orbital period: n/a");
            }
            text_.draw_text(x, y, buffer, panel_scale, 0.86f, 0.89f, 0.96f);
            y -= line_step;

            std::snprintf(buffer, sizeof(buffer), "Temperature: %.0f K  (eq %.0f K)",
                          b.temperature_k, b.equilibrium_temperature_k);
            text_.draw_text(x, y, buffer, panel_scale, 1.00f, 0.72f, 0.40f);
            y -= line_step;

            std::snprintf(buffer, sizeof(buffer), "Density: %.0f kg/m3", b.density_kg_m3);
            text_.draw_text(x, y, buffer, panel_scale, 0.86f, 0.89f, 0.96f);
            y -= line_step;

            std::snprintf(buffer, sizeof(buffer), "Tidal stress: %.2e   Roche: %.4f",
                          b.tidal_ratio, b.roche_limit);
            text_.draw_text(x, y, buffer, panel_scale, 0.78f, 0.82f, 0.92f);
            y -= line_step;
        }

        if (moon_orbit.parent_index >= 0 &&
            moon_orbit.parent_index < MAX_OBJECTS &&
            universe.isActive(moon_orbit.parent_index)) {
            std::snprintf(buffer, sizeof(buffer), "Parent: %s",
                          universe.body(moon_orbit.parent_index).name);
            text_.draw_text(x, y, buffer, panel_scale, 0.72f, 0.84f, 1.00f);
            y -= line_step;
            if (moon_orbit.real_orbit_radius_km > 0.0) {
                std::snprintf(buffer, sizeof(buffer), "Real orbit: %.0f km",
                              moon_orbit.real_orbit_radius_km);
            } else {
                std::snprintf(buffer, sizeof(buffer), "Real orbit: n/a");
            }
            text_.draw_text(x, y, buffer, panel_scale, 0.72f, 0.84f, 1.00f);
            y -= line_step;
            double parent_radius = universe.body(moon_orbit.parent_index).radius;
            if (parent_radius > 0.0) {
                std::snprintf(buffer, sizeof(buffer),
                              "Visual orbit: %.2f planet radii (scaled)",
                              moon_orbit.visual_radius / parent_radius);
            } else {
                std::snprintf(buffer, sizeof(buffer), "Visual orbit: scaled");
            }
            text_.draw_text(x, y, buffer, panel_scale, 0.72f, 0.84f, 1.00f);
            y -= line_step;
        }

        if (b.type == BodyType::Comet) {
            std::snprintf(buffer, sizeof(buffer), "Comet activity: %.0f%%   tail: %.3f",
                          b.comet_activity * 100.0, b.tail_length);
            text_.draw_text(x, y, buffer, panel_scale, 0.65f, 0.92f, 1.00f);
            y -= line_step;
        }

        std::snprintf(buffer, sizeof(buffer), "Velocity: (%.2f, %.2f, %.2f)",
                      b.vx, b.vy, b.vz);
        text_.draw_text(x, y, buffer, panel_scale, 0.86f, 0.89f, 0.96f);
        y -= line_step;

        double speed = std::sqrt(b.vx * b.vx + b.vy * b.vy + b.vz * b.vz);
        std::snprintf(buffer, sizeof(buffer), "Speed: %.3f sim units", speed);
        text_.draw_text(x, y, buffer, panel_scale, 0.86f, 0.89f, 0.96f);
        y -= line_step;

        double acceleration = std::sqrt(b.ax * b.ax + b.ay * b.ay + b.az * b.az);
        std::snprintf(buffer, sizeof(buffer), "Acceleration: %.4f sim units", acceleration);
        text_.draw_text(x, y, buffer, panel_scale, 0.86f, 0.89f, 0.96f);
        y -= line_step;

        double star_x = 0.0, star_y = 0.0, star_z = 0.0;
        double star_distance = universe.distanceToNearestStar(index, star_x, star_y, star_z);
        if (star_distance >= 0.0) {
            std::snprintf(buffer, sizeof(buffer),
                          "Distance from Sun: %.3f sim units", star_distance);
        } else {
            std::snprintf(buffer, sizeof(buffer), "Distance from Sun: n/a");
        }
        text_.draw_text(x, y, buffer, panel_scale, 0.86f, 0.89f, 0.96f);
        y -= line_step;

        text_.draw_text(x, y, "History:", panel_scale, 0.70f, 0.85f, 0.75f);
        y -= line_step;
        float history_step = (compact ? 25.0f :
            ((moon_orbit.parent_index >= 0) ? 26.0f : 28.0f)) * s;
        float history_bottom = static_cast<float>(panel_y) + 4.0f * s;
        int history_lines = 1 +
            static_cast<int>((y - history_bottom) / history_step);
        if (history_lines < 1) history_lines = 1;
        if (history_lines > 10) history_lines = 10;
        drawWrapped(x, y, static_cast<float>(panel_width) - 36.0f * s, b.history,
                    history_scale, history_step, history_lines,
                    0.78f, 0.81f, 0.88f);
    } else {
        text_.draw_text(x, y, "NO BODY SELECTED", title_scale, 1.00f, 0.85f, 0.35f);
        y -= 40.0f * s;
        text_.draw_text(x, y, "Click any body to read its data.", panel_scale,
                        0.78f, 0.83f, 0.92f);
        y -= line_step;
        text_.draw_text(x, y, "Scale is compressed; facts are real.", panel_scale,
                        0.62f, 0.72f, 0.82f);
        y -= line_step;
        text_.draw_text(x, y, "G  gravity grid", panel_scale, 0.78f, 0.83f, 0.92f);
        y -= line_step;
        text_.draw_text(x, y, "T  orbit trails", panel_scale, 0.78f, 0.83f, 0.92f);
        y -= line_step;
        text_.draw_text(x, y, "Y  trajectory prediction", panel_scale, 0.78f, 0.83f, 0.92f);
        y -= line_step;
        text_.draw_text(x, y, "C  collision mode", panel_scale, 0.78f, 0.83f, 0.92f);
        y -= line_step;
        text_.draw_text(x, y, "A/D spawn or delete bodies", panel_scale,
                        0.78f, 0.83f, 0.92f);
    }

    // Recent event feed, drawn above the bottom control strip. Status messages
    // are transient; only physics events enter this list.
    int event_count = ui.events.count < 4 ? ui.events.count : 4;
    if (event_count > 0) {
        const float event_step = 38.0f * s;
        char event_lines[4][224];
        float max_width = 0.0f;
        for (int i = 0; i < event_count; ++i) {
            int index = (ui.events.next - 1 - i + EVENT_LOG_SIZE) % EVENT_LOG_SIZE;
            const SimEvent &event = ui.events.entries[index];
            std::snprintf(event_lines[i], sizeof(event_lines[i]), "%7.1f  %s",
                          event.time, event.text);
            float width = text_.text_width(event_lines[i], small_scale);
            if (width > max_width) max_width = width;
        }
        text_.draw_rect(margin - 5.0f * s, 188.0f * s, max_width + 12.0f * s,
                        static_cast<float>(event_count) * event_step + 8.0f * s,
                        0.02f, 0.03f, 0.07f, 0.62f);
        float event_y = 196.0f * s +
                        static_cast<float>(event_count - 1) * event_step;
        for (int i = 0; i < event_count; ++i) {
            int index = (ui.events.next - 1 - i + EVENT_LOG_SIZE) % EVENT_LOG_SIZE;
            const SimEvent &event = ui.events.entries[index];
            text_.draw_text(margin, event_y, event_lines[i], small_scale,
                            event.color[0], event.color[1], event.color[2], 0.82f);
            event_y -= event_step;
        }
    }

    const char *renderer_status = renderer.rendererStatus();
    if (renderer_status != nullptr && renderer_status[0] != '\0') {
        const float warn_scale = small_scale;
        const float warn_width = text_.text_width(renderer_status, warn_scale);
        const float warn_x = margin;
        const float warn_y = static_cast<float>(ui.win_height) -
                             220.0f * s - 36.0f * s;
        text_.draw_rect(warn_x, warn_y, warn_width + 24.0f * s, 34.0f * s,
                        0.30f, 0.06f, 0.05f, 0.85f);
        text_.draw_text(warn_x + 12.0f * s, warn_y + 9.0f * s,
                        renderer_status, warn_scale, 1.00f, 0.88f, 0.60f);
    }

    if (simulation.paused) {
        text_.draw_text(static_cast<float>(ui.sim_width / 2) - 60.0f * s,
                        static_cast<float>(ui.win_height) - 52.0f * s,
                        "PAUSED", title_scale, 1.00f, 0.30f, 0.20f);
    }

    text_.end();
    glEnable(GL_DEPTH_TEST);
}

} // namespace spacesim
