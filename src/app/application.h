#ifndef SPACESIM_APP_APPLICATION_H
#define SPACESIM_APP_APPLICATION_H

#include "app/cli.h"
#include "app/screenshot.h"
#include "core/state.h"
#include "core/universe.h"
#include "fx/dust_field.h"
#include "fx/effects.h"
#include "input/input_controller.h"
#include "input/spawn_service.h"
#include "physics/collision_solver.h"
#include "physics/environment.h"
#include "physics/moon_system.h"
#include "physics/physics_engine.h"
#include "render/camera.h"
#include "render/cosmetic_scene.h"
#include "render/scene_renderer.h"
#include "ui/hud.h"

namespace spacesim {

class Application {
public:
    explicit Application(const CliOptions &options);
    int run(int argc, char **argv);

    void display();
    void reshape(int width, int height);
    void timer(int value);

private:
    bool initGL();
    void resetSimulation();
    void step(double dt);

    static void displayCallback();
    static void reshapeCallback(int width, int height);
    static void timerCallback(int value);
    static void keyboardCallback(unsigned char key, int x, int y);
    static void specialCallback(int key, int x, int y);
    static void mouseCallback(int button, int state, int x, int y);
    static void motionCallback(int x, int y);
    static void passiveMotionCallback(int x, int y);
    static void armScreenshotCallback(int value);

    static Application *instance_;

    Universe universe_;
    PhysicsEngine physics_;
    CollisionSolver collisions_;
    Environment environment_;
    MoonSystem moons_;
    Effects effects_;
    DustField dust_;
    OrbitCamera camera_;
    CosmeticScene cosmetics_;
    SceneRenderer renderer_;
    Hud hud_;
    SpawnService spawn_;
    UiState ui_;
    SimulationState simulation_;
    SelectionState selection_;
    InputState input_state_;
    InputController input_;
    Screenshot screenshot_;
    CliOptions options_;
    double last_timer_seconds_ = -1.0;
    // Set to false when the OpenGL 3.3 function set cannot be loaded. The main
    // loop then presents a plain error screen instead of calling null pointers.
    bool gl_ready_ = true;
};

} // namespace spacesim

#endif
