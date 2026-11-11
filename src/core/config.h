#ifndef SPACESIM_CORE_CONFIG_H
#define SPACESIM_CORE_CONFIG_H

namespace spacesim {

constexpr double PI = 3.14159265358979323846;

constexpr int MAX_OBJECTS = 256;
constexpr int NAME_STR_LEN = 64;
constexpr int HIST_STR_LEN = 512;

constexpr double G_CONSTANT = 0.15;
constexpr double SOFTENING = 0.01;
constexpr double MOON_SOFTENING = 0.0001;

constexpr double BASE_DT = 0.0025;
constexpr int SUBSTEPS_MAX = 32;

constexpr double MIN_SPEED_MULT = 0.10;
constexpr double MAX_SPEED_MULT = 10.00;

constexpr double EARTH_MASS_SIM = 3.003e-6;
constexpr double EARTH_RADIUS_SIM = 0.013;
constexpr double EARTH_VISUAL_RADIUS = 0.017;
constexpr double MIN_MOON_VISUAL_RADIUS = 0.0025;

constexpr int BELT_COUNT = 5000;
constexpr int DUST_COUNT = 1024;
constexpr int STAR_COUNT = 600;

constexpr int WINDOW_WIDTH = 1600;
constexpr int WINDOW_HEIGHT = 900;
constexpr int HUD_MARGIN = 24;
constexpr int HUD_PANEL_MAX_WIDTH = 620;
constexpr double UI_FONT_SCALE = 1.0;

constexpr double CAMERA_FOV = 45.0;
constexpr double CAMERA_NEAR = 0.01;
constexpr double CAMERA_FAR = 100.0;
constexpr double CAMERA_DEFAULT_AZIMUTH = 45.0;
constexpr double CAMERA_DEFAULT_ELEVATION = 35.0;
constexpr double CAMERA_DEFAULT_DISTANCE = 3.05;
constexpr double CAMERA_MIN_DISTANCE = 0.05;
constexpr double CAMERA_MAX_DISTANCE = 8.00;

constexpr int EVENT_LOG_SIZE = 16;
constexpr int FX_EVENT_QUEUE_SIZE = 64;

constexpr double EARTH_ORBIT_SIM = 0.38;
constexpr double EARTH_TEMPERATURE_K = 288.0;
constexpr double STAR_COLLAPSE_MASS = 2.5;
constexpr int FRAGMENT_MAX = 10;
constexpr double COLLISION_RESTITUTION = 0.35;
constexpr double COLLISION_VISUAL_SCALE = 0.90;
constexpr double COLLISION_COOLDOWN_TIME = 0.08;
constexpr double EARTH_DENSITY_KG_M3 = 5514.0;

} // namespace spacesim

#endif
