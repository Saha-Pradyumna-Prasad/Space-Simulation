#include "app/application.h"

#include "core/config.h"
#include "core/math_utils.h"
#include "core/universe.h"
#include "ui/layout.h"
#include "ui/status.h"

#include "render/gl_api.h"

#include <GL/glut.h>
#include <GL/freeglut_ext.h>
#include <cstdlib>
#include <cstdio>

namespace spacesim {

Application *Application::instance_ = nullptr;

Application::Application(const CliOptions &options)
    : input_(universe_, ui_, simulation_, selection_, input_state_, camera_,
             spawn_, dust_, [this]() { resetSimulation(); }),
      options_(options) {}

void Application::resetSimulation() {
    universe_.reset();
    universe_.seedSolarSystem();
    universe_.seedMoons();

    selection_.selected_body_index = -1;
    selection_.follow_selected = false;

    simulation_.time_multiplier = 1.0;
    simulation_.paused = false;
    simulation_.sim_time = 0.0;
    simulation_.collision_mode = CollisionMode::Realistic;

    ui_.fps = 0;
    ui_.frame_count = 0;
    ui_.fps_last_time = 0;
    ui_.show_labels = true;
    ui_.show_trails = true;
    ui_.show_prediction = true;
    ui_.show_gravity_grid = false;
    ui_.show_tidal = true;
    ui_.show_thermal = true;
    ui_.show_dust = true;
    ui_.show_collision_debug = false;
    ui_.status_frames = 0;
    ui_.status_message[0] = '\0';
    ui_.sim_time = 0.0;
    ui_.events.clear();
    ui_.fx.clear();
    effects_.reset();
    ui_.hover_body_index = -1;
    ui_.spawn.cursor_valid = false;
    for (int i = 0; i < BODY_TYPE_COUNT; ++i) ui_.spawn.counter[i] = 0;

    camera_.reset(selection_);
    renderer_.reset_trails();
    dust_.reset();
    environment_.update(universe_, 0.0);
    set_status(ui_, "Solar system seeded with %d bodies", universe_.count());
}

void Application::step(double dt) {
    int substeps = physics_.computeSubsteps(universe_, dt);
    double sub_dt = dt / static_cast<double>(substeps);
    for (int s = 0; s < substeps; ++s) {
        physics_.integrate(universe_, sub_dt, ui_);
        collisions_.resolve(universe_, ui_, selection_, simulation_.collision_mode,
                            simulation_.physical_collisions);
    }

    moons_.updateVisuals(universe_);
    environment_.update(universe_, dt);
    if (ui_.show_dust) dust_.update(universe_, ui_, dt);
    cosmetics_.update(dt);
    simulation_.sim_time += dt;
    ui_.sim_time = simulation_.sim_time;
}

void Application::display() {
    if (!gl_ready_) {
        // No usable OpenGL 3.3 context/function set: present a plain error
        // screen instead of calling into unloaded GL entry points.
        glClearColor(0.12f, 0.02f, 0.02f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glutSwapBuffers();
        return;
    }

    renderer_.render(universe_, camera_, cosmetics_, dust_, effects_, selection_,
                     ui_, simulation_.physical_collisions);
    hud_.draw(universe_, ui_, simulation_, selection_, renderer_);

    if (screenshot_.shouldCapture()) {
        screenshot_.save(ui_.win_width, ui_.win_height);
    }

    glutSwapBuffers();

    if (screenshot_.done()) {
        std::exit(0);
    }
}

void Application::reshape(int width, int height) {
    ui_.win_width = (width > 0 ? width : 1);
    ui_.win_height = (height > 0 ? height : 1);
    update_sim_width(ui_);
    glViewport(0, 0, ui_.win_width, ui_.win_height);
    glutPostRedisplay();
}

void Application::timer(int value) {
    (void)value;

    ++ui_.frame_count;
    int now = glutGet(GLUT_ELAPSED_TIME);
    if (ui_.fps_last_time == 0) ui_.fps_last_time = now;
    if (now - ui_.fps_last_time >= 500) {
        ui_.fps = static_cast<int>(ui_.frame_count * 1000.0 /
                                   static_cast<double>(now - ui_.fps_last_time));
        ui_.frame_count = 0;
        ui_.fps_last_time = now;
    }

    double now_seconds = static_cast<double>(now) * 0.001;
    double real_dt = (last_timer_seconds_ < 0.0)
        ? 0.016 : now_seconds - last_timer_seconds_;
    last_timer_seconds_ = now_seconds;
    real_dt = clamp_double(real_dt, 0.0, 0.05);

    if (!simulation_.paused) {
        // Keep simulation speed tied to wall-clock time instead of the
        // nominal 16 ms timer. Adaptive sub-stepping still protects accuracy.
        double frame_ratio = real_dt / 0.016;
        double dt = BASE_DT * simulation_.time_multiplier * frame_ratio;
        if (dt > 0.0) step(dt);
    }

    camera_.update(universe_, selection_, real_dt);

    // Effects keep animating while the simulation is paused, so a body spawned
    // during a pause still produces its flash and shockwave.
    effects_.consume(ui_);
    effects_.update(real_dt);

    if (ui_.status_frames > 0) --ui_.status_frames;

    glutPostRedisplay();
    glutTimerFunc(16, timerCallback, 0);
}

bool Application::initGL() {
    if (!glapi::load()) {
        std::fprintf(stderr,
                     "Could not load the OpenGL 3.3 function set. The GPU or "
                     "driver must support OpenGL 3.3 core.\n");
        glutSetWindowTitle("Space Simulator v3.5 - OpenGL 3.3 required");
        return false;
    }
    const GLubyte *vendor = glGetString(GL_VENDOR);
    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *version = glGetString(GL_VERSION);
    const GLubyte *glsl = glGetString(GL_SHADING_LANGUAGE_VERSION);
    if (vendor != nullptr) {
        std::printf("OpenGL vendor: %s\n", reinterpret_cast<const char *>(vendor));
    }
    if (renderer != nullptr) {
        std::printf("OpenGL renderer: %s\n",
                    reinterpret_cast<const char *>(renderer));
    }
    if (version != nullptr) {
        std::printf("OpenGL: %s\n", reinterpret_cast<const char *>(version));
    }
    if (glsl != nullptr) {
        std::printf("GLSL: %s\n", reinterpret_cast<const char *>(glsl));
    }
    GLint profile = 0;
#ifdef GL_CONTEXT_PROFILE_MASK
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
#endif
    const char *profile_name = "unknown";
#ifdef GL_CONTEXT_CORE_PROFILE_BIT
    if ((profile & GL_CONTEXT_CORE_PROFILE_BIT) != 0) profile_name = "core";
#endif
#ifdef GL_CONTEXT_COMPATIBILITY_PROFILE_BIT
    if ((profile & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) != 0) {
        profile_name = "compatibility";
    }
#endif
    std::printf("Profile: %s\n", profile_name);
    std::printf("MSAA samples: %d\n", glutGet(GLUT_WINDOW_NUM_SAMPLES));
    glClearColor(0.005f, 0.008f, 0.020f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);
    return true;
}

int Application::run(int argc, char **argv) {
    instance_ = this;

    glutInit(&argc, argv);
    if (!options_.has_window_size) {
        // Avoid creating a 1600x900 window on a smaller desktop (common on
        // Windows laptops); the HUD scales down with the resulting size.
        const int screen_width = glutGet(GLUT_SCREEN_WIDTH);
        const int screen_height = glutGet(GLUT_SCREEN_HEIGHT);
        if (screen_width > 0 && screen_height > 0) {
            if (ui_.win_width > screen_width - 40) {
                ui_.win_width = screen_width - 40;
            }
            if (ui_.win_height > screen_height - 80) {
                ui_.win_height = screen_height - 80;
            }
            if (ui_.win_width < 640) ui_.win_width = 640;
            if (ui_.win_height < 480) ui_.win_height = 480;
        }
    }
    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE);
    if (options_.has_window_size) {
        ui_.win_width = options_.window_width;
        ui_.win_height = options_.window_height;
    }
    update_sim_width(ui_);
    glutInitWindowSize(ui_.win_width, ui_.win_height);
    glutCreateWindow("Space Simulator v3.5");

    gl_ready_ = initGL();
    if (gl_ready_) {
        cosmetics_.init();
        renderer_.init();
        if (!hud_.init()) {
            std::fprintf(stderr, "Could not initialize the HUD text renderer.\n");
        }
    }
    resetSimulation();

    ui_.show_gravity_grid = options_.show_grid;
    ui_.show_trails = options_.show_trails;
    ui_.show_prediction = options_.show_prediction;
    if (options_.classic_collisions) {
        simulation_.collision_mode = CollisionMode::Classic;
    }

    if (options_.impact_demo) {
        // Place the two bodies apart and give them closing velocities so the
        // demo shows an approach before the impact.
        universe_.addBody(BodyType::Planet, "Impact Demo A",
                          0.82, 0.0, 0.90, 1.6, 0.0, 0.0,
                          3.003e-6, 0.020);
        universe_.addBody(BodyType::Planet, "Impact Demo B",
                          1.06, 0.0, 0.90, -1.6, 0.0, 0.0,
                          3.003e-6, 0.020);
        selection_.selected_body_index = universe_.findByName("Impact Demo A");
    } else if (options_.collapse_demo) {
        universe_.addBody(BodyType::Star, "Collapse Demo",
                          0.90, 0.0, 0.90, 0.0, 0.0, 0.0,
                          3.0, 0.050);
        universe_.addBody(BodyType::Asteroid, "Collapse Trigger",
                          0.90, 0.0, 0.90, 0.0, 0.0, 0.0,
                          1.0e-9, 0.004);
        selection_.selected_body_index = universe_.findByName("Collapse Demo");
    } else if (options_.comet_demo) {
        universe_.addBody(BodyType::Comet, "Comet Demo",
                          0.34, 0.0, 0.0, 0.0, 0.0, 0.78,
                          1.0e-10, 0.004);
        selection_.selected_body_index = universe_.findByName("Comet Demo");
    }

    if (options_.initial_selection[0] != '\0') {
        int selected = universe_.findByName(options_.initial_selection);
        if (selected >= 0) selection_.selected_body_index = selected;
    }
    if (options_.follow_selected &&
        selection_.selected_body_index >= 0) {
        selection_.follow_selected = true;
    }
    if (options_.show_lab) {
        ui_.show_equations = true;
        ui_.equation_tab = options_.lab_tab;
    }
    if (options_.show_editor) ui_.show_editor = true;
    if (options_.physical_collisions) simulation_.physical_collisions = true;
    if (options_.collision_debug) ui_.show_collision_debug = true;
    if (options_.tutorial) {
        ui_.show_tutorial = true;
        ui_.guided_tour = options_.guided_tour;
    }
    if (options_.has_camera) {
        camera_.setView(options_.camera_azimuth, options_.camera_elevation,
                        options_.camera_distance);
    }

    glutDisplayFunc(displayCallback);
    glutReshapeFunc(reshapeCallback);
    glutKeyboardFunc(keyboardCallback);
    glutSpecialFunc(specialCallback);
    glutMouseFunc(mouseCallback);
    glutMotionFunc(motionCallback);
    glutPassiveMotionFunc(passiveMotionCallback);
    glutTimerFunc(16, timerCallback, 0);

    if (options_.screenshot_mode) {
        glutTimerFunc(1500, armScreenshotCallback, 0);
    }

    glutMainLoop();
    return 0;
}

void Application::displayCallback() {
    if (instance_ != nullptr) instance_->display();
}

void Application::reshapeCallback(int width, int height) {
    if (instance_ != nullptr) instance_->reshape(width, height);
}

void Application::timerCallback(int value) {
    if (instance_ != nullptr) instance_->timer(value);
}

void Application::keyboardCallback(unsigned char key, int x, int y) {
    (void)x;
    (void)y;
    if (instance_ != nullptr) instance_->input_.keyboard(key);
}

void Application::specialCallback(int key, int x, int y) {
    (void)x;
    (void)y;
    if (instance_ != nullptr) instance_->input_.special(key);
}

void Application::mouseCallback(int button, int state, int x, int y) {
    if (instance_ != nullptr) instance_->input_.mouse(button, state, x, y);
}

void Application::motionCallback(int x, int y) {
    if (instance_ != nullptr) instance_->input_.motion(x, y);
}

void Application::passiveMotionCallback(int x, int y) {
    if (instance_ != nullptr) instance_->input_.passiveMotion(x, y);
}

void Application::armScreenshotCallback(int value) {
    (void)value;
    if (instance_ == nullptr) return;
    instance_->screenshot_.arm(instance_->options_.screenshot_path);
    glutPostRedisplay();
}

} // namespace spacesim
