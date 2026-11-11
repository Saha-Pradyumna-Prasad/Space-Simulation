// ============================================================================
//  N-Body Space Simulator — 3D
//  Interactive 3D gravity sandbox built with C++ and freeGLUT / OpenGL.
//
//  The simulation keeps a fixed pool of celestial bodies and updates every
//  body from the mutual gravitational attraction of every other body.  Orbits
//  are therefore emergent: nothing is precomputed or animated by hand.
//
//  Coordinate system: y is up; the initial planetary orbits lie in the x-z
//  plane.  The camera is a spherical orbit camera.
//
//  Build (Ubuntu):   make
//  Run:              ./nbody
//  Physics check:    ./nbody --selftest
// ============================================================================

#include <GL/glut.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// ============================================================================
//  1. Constants
// ============================================================================

constexpr double PI = 3.14159265358979323846;

constexpr int MAX_OBJECTS = 256;   // fixed object pool
constexpr int NAME_STR_LEN = 64;   // max body name length
constexpr int HIST_STR_LEN = 512;  // max history text length

constexpr double G_CONSTANT = 0.15;  // gravitational constant (simulation units)
constexpr double SOFTENING = 0.01;   // general distance softening
constexpr double MOON_SOFTENING = 0.0001;  // moon-parent pairs are much closer

constexpr double BASE_DT = 0.0025;   // simulation time advanced per frame at 1x
constexpr int SUBSTEPS_MAX = 32;     // safety cap for adaptive sub-stepping

constexpr double MIN_SPEED_MULT = 0.10;
constexpr double MAX_SPEED_MULT = 10.00;

// Earth is the reference for the HUD.  The Sun stays at mass 1.0 in the
// simulation, so Earth's simulation mass is chosen to keep the real
// Sun/Earth mass ratio readable in the HUD.
constexpr double EARTH_MASS_SIM = 3.003e-6;
constexpr double EARTH_RADIUS_SIM = 0.013;
constexpr double EARTH_VISUAL_RADIUS = 0.017;
constexpr double MIN_MOON_VISUAL_RADIUS = 0.0025;

// At 1x, one real second advances about this much simulation time
// (BASE_DT per frame at roughly 60 FPS).  Used to convert the visual
// moon periods in the seed table into angular speeds.
constexpr int BELT_COUNT = 320;
constexpr int STAR_COUNT = 600;

constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 800;
constexpr int HUD_MARGIN = 20;
constexpr int HUD_PANEL_MAX_WIDTH = 480;

constexpr double CAMERA_FOV = 45.0;
constexpr double CAMERA_NEAR = 0.01;
constexpr double CAMERA_FAR = 100.0;
constexpr double CAMERA_DEFAULT_AZIMUTH = 45.0;
constexpr double CAMERA_DEFAULT_ELEVATION = 35.0;
constexpr double CAMERA_DEFAULT_DISTANCE = 3.05;
constexpr double CAMERA_MIN_DISTANCE = 0.70;
constexpr double CAMERA_MAX_DISTANCE = 8.00;

// ============================================================================
//  2. Body types and data model
// ============================================================================

enum class BodyType {
    Star,
    Planet,
    Moon,
    Asteroid,
    Meteor,
    Comet,
    BlackHole,
    Count  // always last: number of types (7)
};
constexpr int BODY_TYPE_COUNT = static_cast<int>(BodyType::Count);

struct CelestialBody {
    BodyType type = BodyType::Star;
    bool active = false;

    double x = 0.0, y = 0.0, z = 0.0;
    double vx = 0.0, vy = 0.0, vz = 0.0;
    double ax = 0.0, ay = 0.0, az = 0.0;

    double mass = 0.0;
    double radius = 0.0;          // visual radius
    double collision_radius = 0.0; // physical collision radius

    char name[NAME_STR_LEN] = {};

    double self_rotation_time = 0.0;  // hours per rotation
    double orbital_period = 0.0;      // days per orbit
    double volume_ratio = 0.0;        // volume compared with Earth

    char history[HIST_STR_LEN] = {};

    float color[3] = {1.0f, 1.0f, 1.0f};
};

// Parented moon orbit.  Moons are pool bodies, but their position is
// computed relative to their parent planet so the compressed solar system
// can show stable, visible satellites.
struct MoonOrbit {
    int parent_index = -1;
    double physical_radius = 0.0;   // Hill-safe physical orbit radius
    double visual_radius = 0.0;     // exaggerated radius used for drawing
    double visual_scale = 1.0;      // visual_radius / physical_radius
    double inclination = 0.0;       // used for the visual orbit ring
    double real_orbit_radius_km = 0.0;
    double visual_x = 0.0;          // render-only position
    double visual_y = 0.0;
    double visual_z = 0.0;
};

// The whole simulation lives in this fixed pool.  No per-body heap allocation.
static std::array<CelestialBody, MAX_OBJECTS> universe_pool;
static std::array<MoonOrbit, MAX_OBJECTS> moon_orbits;
static std::array<double, MAX_OBJECTS> prev_x;
static std::array<double, MAX_OBJECTS> prev_y;
static std::array<double, MAX_OBJECTS> prev_z;
static int active_object_count = 0;
static int selected_body_index = -1;
static double time_multiplier = 1.0;
static bool paused = false;
static double sim_time = 0.0;

// Window / view state.
static int winWidth = WINDOW_WIDTH;
static int winHeight = WINDOW_HEIGHT;
static int sim_width = WINDOW_WIDTH;

// Spherical orbit camera.  y is up.
static double cam_azimuth = CAMERA_DEFAULT_AZIMUTH;
static double cam_elevation = CAMERA_DEFAULT_ELEVATION;
static double cam_distance = CAMERA_DEFAULT_DISTANCE;
static double cam_target_x = 0.0;
static double cam_target_y = 0.0;
static double cam_target_z = 0.0;
static bool follow_selected = false;

// Mouse drag state.
static int drag_mode = 0;  // 0 = none, 1 = rotate, 2 = pan
static int last_mouse_x = 0;
static int last_mouse_y = 0;

// Spawn cursor on the y = 0 orbital plane.
static double spawn_x = 0.0;
static double spawn_z = 0.0;
static bool spawn_cursor_valid = false;

// Spawn configuration.
static BodyType spawn_type = BodyType::Planet;
static int spawn_mass_percent = 100;    // 25 .. 400
static int spawn_radius_percent = 100;  // 50 .. 200
static int spawn_velocity_mode = 0;     // 0 = orbital, 1 = retrograde, 2 = zero
static int spawn_counter[BODY_TYPE_COUNT] = {};

// HUD / diagnostics.
static int fps = 0;
static int frame_count = 0;
static int fps_last_time = 0;
static bool show_labels = true;
static char status_message[160] = {};
static int status_frames = 0;

// Optional one-frame screenshot mode (used for documentation checks).
static bool capture_armed = false;
static bool capture_done = false;
static char screenshot_path[512] = "nbody.ppm";
static char initial_selection[NAME_STR_LEN] = {};

// Projected screen-space labels.
struct ScreenLabel {
    int x = 0;
    int y = 0;
    char text[NAME_STR_LEN] = {};
    float color[3] = {1.0f, 1.0f, 1.0f};
};
static ScreenLabel screen_labels[MAX_OBJECTS];
static int screen_label_count = 0;

// Shared sphere geometry.
static GLUquadric *sphere_quadric = nullptr;

// ============================================================================
//  3. Type defaults
// ============================================================================

struct TypeDefaults {
    const char *label;
    double mass;
    double radius;
    float color[3];
};

static const TypeDefaults TYPE_DEFAULTS[BODY_TYPE_COUNT] = {
    {"Star",      0.50,        0.050, {1.00f, 0.85f, 0.25f}},
    {"Planet",    3.003e-6,    0.012, {0.35f, 0.62f, 1.00f}},
    {"Moon",      3.700e-8,    0.006, {0.72f, 0.72f, 0.75f}},
    {"Asteroid",  1.000e-9,    0.004, {0.62f, 0.57f, 0.50f}},
    {"Meteor",    1.000e-10,   0.003, {1.00f, 0.90f, 0.60f}},
    {"Comet",     1.000e-10,   0.004, {0.65f, 0.92f, 1.00f}},
    {"BlackHole", 2.00,        0.018, {0.06f, 0.02f, 0.10f}},
};

static const char *type_name(BodyType t) {
    switch (t) {
        case BodyType::Star:      return "Star";
        case BodyType::Planet:    return "Planet";
        case BodyType::Moon:      return "Moon";
        case BodyType::Asteroid:  return "Asteroid";
        case BodyType::Meteor:    return "Meteor";
        case BodyType::Comet:     return "Comet";
        case BodyType::BlackHole: return "Black Hole";
        default:                  return "Unknown";
    }
}

static const TypeDefaults &type_defaults(BodyType t) {
    int index = static_cast<int>(t);
    if (index < 0 || index >= BODY_TYPE_COUNT) {
        index = 0;
    }
    return TYPE_DEFAULTS[index];
}

// ============================================================================
//  4. Small helpers
// ============================================================================

static double clamp_double(double value, double low, double high) {
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

static int clamp_int(int value, int low, int high) {
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

static void copy_name(char *dst, const char *src) {
    int i = 0;
    if (src != nullptr) {
        for (; i < NAME_STR_LEN - 1 && src[i] != '\0'; ++i) {
            dst[i] = src[i];
        }
    }
    dst[i] = '\0';
}

static void copy_history(char *dst, const char *src) {
    int i = 0;
    if (src != nullptr) {
        for (; i < HIST_STR_LEN - 1 && src[i] != '\0'; ++i) {
            dst[i] = src[i];
        }
    }
    dst[i] = '\0';
}

static void copy_path(char *dst, int size, const char *src) {
    int i = 0;
    if (src != nullptr) {
        for (; i < size - 1 && src[i] != '\0'; ++i) {
            dst[i] = src[i];
        }
    }
    dst[i] = '\0';
}

static double volume_from_radius(double radius) {
    double ratio = radius / EARTH_RADIUS_SIM;
    return ratio * ratio * ratio;
}

static void set_status(const char *format, ...) {
    va_list args;
    va_start(args, format);
    std::vsnprintf(status_message, sizeof(status_message), format, args);
    va_end(args);
    status_frames = 180;  // about 3 seconds at 60 FPS
}

static bool body_is_finite(const CelestialBody &body) {
    return std::isfinite(body.x) && std::isfinite(body.y) && std::isfinite(body.z) &&
           std::isfinite(body.vx) && std::isfinite(body.vy) && std::isfinite(body.vz) &&
           std::isfinite(body.ax) && std::isfinite(body.ay) && std::isfinite(body.az);
}

static void body_visual_position(int index, double out[3]);

// Ray–sphere intersection.  Returns the nearest positive hit distance.
static bool ray_sphere_hit(const double origin[3], const double direction[3],
                           const double center[3], double radius, double &hit_t) {
    double oc_x = origin[0] - center[0];
    double oc_y = origin[1] - center[1];
    double oc_z = origin[2] - center[2];

    double a = direction[0] * direction[0] +
               direction[1] * direction[1] +
               direction[2] * direction[2];
    if (a < 1.0e-15) return false;

    double b = 2.0 * (oc_x * direction[0] +
                      oc_y * direction[1] +
                      oc_z * direction[2]);
    double c = oc_x * oc_x + oc_y * oc_y + oc_z * oc_z - radius * radius;
    double discriminant = b * b - 4.0 * a * c;
    if (discriminant < 0.0) return false;

    double root = std::sqrt(discriminant);
    double t0 = (-b - root) / (2.0 * a);
    double t1 = (-b + root) / (2.0 * a);
    if (t0 > 1.0e-6) {
        hit_t = t0;
        return true;
    }
    if (t1 > 1.0e-6) {
        hit_t = t1;
        return true;
    }
    return false;
}

// ============================================================================
//  5. Object pool operations
// ============================================================================

static int find_free_slot() {
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe_pool[i].active) {
            return i;
        }
    }
    return -1;
}

static int add_body(BodyType type,
                    const char *name,
                    double x, double y, double z,
                    double vx, double vy, double vz,
                    double mass, double radius) {
    int slot = find_free_slot();
    if (slot < 0) {
        return -1;
    }

    const TypeDefaults &defaults = type_defaults(type);
    if (mass <= 0.0) mass = defaults.mass;
    if (radius <= 0.0) radius = defaults.radius;

    CelestialBody body;
    body.type = type;
    body.active = true;
    body.x = x;
    body.y = y;
    body.z = z;
    body.vx = vx;
    body.vy = vy;
    body.vz = vz;
    body.ax = 0.0;
    body.ay = 0.0;
    body.az = 0.0;
    body.mass = mass;
    body.radius = radius;
    body.collision_radius = radius * 0.001;  // small physical default
    copy_name(body.name, (name != nullptr ? name : type_name(type)));
    body.self_rotation_time = 0.0;
    body.orbital_period = 0.0;
    body.volume_ratio = volume_from_radius(radius);
    copy_history(body.history, "Runtime body created by the user. Watch how its gravity perturbs the other orbits.");
    for (int i = 0; i < 3; ++i) {
        body.color[i] = defaults.color[i];
    }

    universe_pool[slot] = body;
    moon_orbits[slot] = MoonOrbit{};
    prev_x[slot] = x;
    prev_y[slot] = y;
    prev_z[slot] = z;
    ++active_object_count;
    return slot;
}

// A disappearing planet no longer deletes its moons: they are detached and
// remain in the N-body simulation as heliocentric bodies.
static void detach_moon_children(int parent_index) {
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe_pool[i].active) continue;
        if (moon_orbits[i].parent_index != parent_index) continue;
        moon_orbits[i] = MoonOrbit{};
    }
}

static bool remove_body(int index) {
    if (index < 0 || index >= MAX_OBJECTS) {
        return false;
    }
    if (!universe_pool[index].active) {
        return false;
    }

    universe_pool[index] = CelestialBody{};
    moon_orbits[index] = MoonOrbit{};
    --active_object_count;
    if (active_object_count < 0) {
        active_object_count = 0;
    }
    if (selected_body_index == index) {
        selected_body_index = -1;
    }

    detach_moon_children(index);
    return true;
}

static int find_body_index_by_name(const char *name) {
    if (name == nullptr) {
        return -1;
    }
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (universe_pool[i].active && std::strcmp(universe_pool[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static int heaviest_body() {
    int best = -1;
    double best_mass = -1.0;
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe_pool[i].active) continue;
        if (universe_pool[i].mass > best_mass) {
            best_mass = universe_pool[i].mass;
            best = i;
        }
    }
    return best;
}

static int nearest_star_index(int body_index) {
    int best = -1;
    double best_distance = 1.0e30;
    if (body_index < 0 || body_index >= MAX_OBJECTS) {
        return -1;
    }
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe_pool[i].active) continue;
        if (universe_pool[i].type != BodyType::Star) continue;
        double dx = universe_pool[i].x - universe_pool[body_index].x;
        double dy = universe_pool[i].y - universe_pool[body_index].y;
        double dz = universe_pool[i].z - universe_pool[body_index].z;
        double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (distance < best_distance) {
            best_distance = distance;
            best = i;
        }
    }
    return best;
}

// Returns the distance to the nearest star, or -1 if there is no star.
static double distance_to_nearest_star(int body_index, double &star_x,
                                       double &star_y, double &star_z) {
    int star = nearest_star_index(body_index);
    if (star < 0) {
        return -1.0;
    }
    star_x = universe_pool[star].x;
    star_y = universe_pool[star].y;
    star_z = universe_pool[star].z;
    double dx = universe_pool[body_index].x - star_x;
    double dy = universe_pool[body_index].y - star_y;
    double dz = universe_pool[body_index].z - star_z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

static void print_bodies() {
    std::printf("Active bodies: %d\n", active_object_count);
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe_pool[i].active) continue;
        const CelestialBody &b = universe_pool[i];
        std::printf("  [%3d] %-14s %-10s x=%7.4f y=%7.4f z=%7.4f mass=%10.4g r=%7.4f\n",
                    i, b.name, type_name(b.type), b.x, b.y, b.z, b.mass, b.radius);
    }
}

// ============================================================================
//  6. Initial solar system
// ============================================================================

struct PlanetSeed {
    const char *name;
    double orbit_radius;
    double visual_radius;
    double real_radius_km;
    double real_orbit_radius_km;
    double mass_earth;
    double volume_earth;
    double rotation_hours;
    double period_days;
    double start_angle;
    float color[3];
    const char *history;
};

static const PlanetSeed PLANET_SEEDS[8] = {
    {"Mercury", 0.18, 0.010, 2439.7, 57.9e6, 0.055, 0.056, 1407.60, 88.0, 0.00,
     {0.62f, 0.62f, 0.62f},
     "Mercury is the smallest planet and the closest to the Sun. It has no atmosphere to retain heat, so its surface swings from about 430 C in daylight to -180 C at night. Its slow 1407.6-hour rotation is longer than its 88-day orbit."},
    {"Venus", 0.28, 0.014, 6051.8, 108.2e6, 0.815, 0.857, -5832.50, 224.7, 1.10,
     {0.90f, 0.80f, 0.45f},
     "Venus is Earth's near twin in size, but a runaway greenhouse effect keeps its surface near 465 C. It rotates backwards once every 5832.5 hours, so a Venusian day is longer than its 224.7-day year."},
    {"Earth", 0.38, 0.017, 6371.0, 149.6e6, 1.000, 1.000, 23.93, 365.2, 2.20,
     {0.20f, 0.45f, 0.95f},
     "Earth is the third planet from the Sun and the only world known to support life. Its 23.9-hour rotation and 365.2-day orbit define the units we use for time. About 71 percent of the surface is covered by water."},
    {"Mars", 0.48, 0.013, 3389.5, 227.9e6, 0.107, 0.151, 24.62, 687.0, 3.30,
     {0.85f, 0.30f, 0.20f},
     "Mars is a cold desert world with a thin carbon-dioxide atmosphere. Iron oxide dust gives it its red colour. Its 24.6-hour day is close to Earth's, while its orbit takes 687 Earth days."},
    {"Jupiter", 0.64, 0.036, 69911.0, 778.5e6, 317.80, 1321.30, 9.93, 4331.0, 4.30,
     {0.85f, 0.65f, 0.40f},
     "Jupiter is the largest planet, more massive than all the other planets combined. Its rapid 9.9-hour rotation drives powerful storms, including the Great Red Spot. One orbit takes about 4331 Earth days."},
    {"Saturn", 0.80, 0.031, 58232.0, 1432.0e6, 95.20, 763.60, 10.66, 10747.0, 5.40,
     {0.90f, 0.80f, 0.50f},
     "Saturn is famous for its bright ring system, made of ice and rock particles. It is the least dense planet, with a 10.7-hour rotation and a 10747-day orbit. The rings are drawn here as cosmetic geometry outside the physics pool."},
    {"Uranus", 0.96, 0.023, 25362.0, 2867.0e6, 14.50, 63.10, -17.24, 30589.0, 0.70,
     {0.45f, 0.85f, 0.90f},
     "Uranus is an ice giant with a pale blue-green colour caused by methane. It rotates on its side once every 17.2 hours and takes 30589 Earth days to orbit the Sun. It was the first planet discovered with a telescope."},
    {"Neptune", 1.12, 0.022, 24622.0, 4515.0e6, 17.10, 57.70, 16.11, 59800.0, 2.90,
     {0.25f, 0.35f, 0.95f},
     "Neptune is the most distant planet and the windiest, with storms exceeding 2000 km/h. It completes one orbit every 59800 Earth days. It was predicted mathematically before it was observed."},
};

struct MoonSeed {
    const char *name;
    const char *parent;
    double visual_radius_scale;
    double inclination_deg;
    int direction;
    double start_angle_deg;
    double real_radius_km;
    double real_orbit_radius_km;
    double real_period_days;
    double real_mass_earth;
    double real_volume_earth;
    double rotation_hours;
    float color[3];
    const char *history;
};

static const MoonSeed MOON_SEEDS[13] = {
    {"Moon", "Earth", 2.80, 5.145, 1, 0.0,
     1737.4, 384400.0, 27.3217, 0.01230, 0.0203, 655.7,
     {0.78f, 0.78f, 0.75f},
     "The Moon is Earth's only natural satellite. It is tidally locked, so the same face always points toward Earth. Its real orbital radius is about 384400 km and its period is 27.32 days."},
    {"Phobos", "Mars", 1.70, 1.08, 1, 30.0,
     11.27, 9376.0, 0.31891, 1.79e-9, 5.6e-9, 7.65,
     {0.55f, 0.50f, 0.45f},
     "Phobos is the larger and closer of Mars's two moons. It orbits at about 9376 km from Mars's centre and is slowly spiralling inward. Its real period is 0.319 days."},
    {"Deimos", "Mars", 2.30, 1.79, 1, 210.0,
     6.20, 23463.0, 1.26244, 2.47e-10, 9.4e-10, 30.3,
     {0.50f, 0.46f, 0.42f},
     "Deimos is the smaller, outer moon of Mars. Its surface is covered with dusty regolith. It takes 1.263 days to complete one orbit around Mars."},
    {"Io", "Jupiter", 1.50, 0.036, 1, 0.0,
     1821.6, 421700.0, 1.769, 0.0150, 0.0234, 42.5,
     {0.95f, 0.85f, 0.35f},
     "Io is the most volcanically active body in the solar system. Tidal heating from Jupiter drives hundreds of volcanoes. Its real orbital period is 1.77 days."},
    {"Europa", "Jupiter", 1.90, 0.466, 1, 90.0,
     1560.8, 671100.0, 3.551, 0.0080, 0.0147, 85.2,
     {0.92f, 0.90f, 0.84f},
     "Europa has a smooth ice shell covering a global salt-water ocean. It is one of the best candidates for extraterrestrial life. Its real orbital period is 3.55 days."},
    {"Ganymede", "Jupiter", 2.30, 0.177, 1, 180.0,
     2634.1, 1070400.0, 7.155, 0.0248, 0.0704, 171.7,
     {0.62f, 0.55f, 0.48f},
     "Ganymede is the largest moon in the solar system, larger than Mercury. It has its own magnetic field and a real orbital period of 7.15 days."},
    {"Callisto", "Jupiter", 2.80, 0.192, 1, 270.0,
     2410.3, 1882700.0, 16.689, 0.0180, 0.0540, 400.5,
     {0.40f, 0.36f, 0.32f},
     "Callisto is the most heavily cratered object known. Its ancient surface has changed little in billions of years. Its real orbital period is 16.69 days."},
    {"Enceladus", "Saturn", 1.70, 0.009, 1, 20.0,
     252.1, 238000.0, 1.370, 1.8e-5, 6.2e-5, 32.9,
     {0.94f, 0.96f, 0.98f},
     "Enceladus vents water vapour and ice from a subsurface ocean through cracks near its south pole. Its real orbital period is 1.37 days."},
    {"Rhea", "Saturn", 2.10, 0.345, 1, 140.0,
     763.8, 527000.0, 4.518, 3.9e-4, 0.0017, 108.4,
     {0.78f, 0.76f, 0.72f},
     "Rhea is Saturn's second-largest moon. It is an icy body with a heavily cratered surface and a real orbital period of 4.52 days."},
    {"Titan", "Saturn", 2.70, 0.348, 1, 260.0,
     2574.7, 1221870.0, 15.945, 0.0225, 0.0660, 382.7,
     {0.85f, 0.65f, 0.35f},
     "Titan is Saturn's largest moon and the only moon with a dense atmosphere. Lakes of liquid methane and ethane dot its surface. Its real orbital period is 15.95 days."},
    {"Titania", "Uranus", 1.90, 0.34, 1, 40.0,
     788.9, 435910.0, 8.706, 5.9e-4, 0.0019, 208.9,
     {0.70f, 0.68f, 0.65f},
     "Titania is the largest moon of Uranus. It is an icy world with long canyons and a real orbital period of 8.71 days."},
    {"Oberon", "Uranus", 2.50, 0.058, 1, 220.0,
     761.4, 583520.0, 13.463, 5.0e-4, 0.0017, 323.1,
     {0.62f, 0.58f, 0.55f},
     "Oberon is the outermost major moon of Uranus. Its dark, cratered surface is one of the oldest in the Uranian system. Its real period is 13.46 days."},
    {"Triton", "Neptune", 2.20, 20.0, -1, 110.0,
     1353.4, 354759.0, 5.877, 0.00359, 0.0096, 141.0,
     {0.85f, 0.75f, 0.72f},
     "Triton is Neptune's largest moon and orbits backwards compared with Neptune's rotation. It is probably a captured Kuiper Belt object. Its real period is 5.88 days."},
};

static void seed_moons() {
    int sun = find_body_index_by_name("Sun");
    double sun_mass = (sun >= 0 ? universe_pool[sun].mass : 1.0);
    if (sun_mass <= 0.0) sun_mass = 1.0;

    for (int i = 0; i < 13; ++i) {
        const MoonSeed &seed = MOON_SEEDS[i];
        int parent = find_body_index_by_name(seed.parent);
        if (parent < 0 || !universe_pool[parent].active) continue;

        const CelestialBody &p = universe_pool[parent];
        double parent_orbit = 0.0;
        if (sun >= 0) {
            double sx = p.x - universe_pool[sun].x;
            double sy = p.y - universe_pool[sun].y;
            double sz = p.z - universe_pool[sun].z;
            parent_orbit = std::sqrt(sx * sx + sy * sy + sz * sz);
        } else {
            parent_orbit = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
        }
        if (parent_orbit < 1.0e-6) parent_orbit = 0.10;

        // Hill-safe physical orbit.  The visible orbit is scaled up for
        // rendering so the compressed solar system still shows moons.
        double hill = parent_orbit * std::cbrt(p.mass / (3.0 * sun_mass));
        if (hill < 1.0e-6) hill = parent_orbit * 0.01;

        // Keep the moons in distinct, conservative Hill-safe orbits:
        // 0.20..0.40 r_H gives a good stability margin.
        double physical_fraction = 0.15 +
            0.25 * (seed.visual_radius_scale - 1.5) / 1.3;
        physical_fraction = clamp_double(physical_fraction, 0.20, 0.40);
        double physical_orbit = hill * physical_fraction;
        if (physical_orbit < 1.0e-6) physical_orbit = 1.0e-4;

        double visual_orbit = p.radius * seed.visual_radius_scale;
        double visual_scale = visual_orbit / physical_orbit;

        double angle = seed.start_angle_deg * PI / 180.0;
        double inclination = seed.inclination_deg * PI / 180.0;
        double direction = static_cast<double>(seed.direction);

        double x_rel = physical_orbit * std::cos(angle);
        double z0 = physical_orbit * std::sin(angle);
        double y_rel = z0 * std::sin(inclination);
        double z_rel = z0 * std::cos(inclination);

        double physical_speed = std::sqrt(G_CONSTANT * p.mass * physical_orbit) /
                                (physical_orbit + MOON_SOFTENING);
        double vx_rel = -physical_speed * std::sin(angle) * direction;
        double vz0 = physical_speed * std::cos(angle) * direction;
        double vy_rel = vz0 * std::sin(inclination);
        double vz_rel = vz0 * std::cos(inclination);

        double body_radius = (seed.real_radius_km / 6371.0) * EARTH_VISUAL_RADIUS;
        if (body_radius < MIN_MOON_VISUAL_RADIUS) {
            body_radius = MIN_MOON_VISUAL_RADIUS;
        }
        double mass = seed.real_mass_earth * EARTH_MASS_SIM;

        int slot = add_body(BodyType::Moon, seed.name,
                            p.x + x_rel, p.y + y_rel, p.z + z_rel,
                            p.vx + vx_rel, p.vy + vy_rel, p.vz + vz_rel,
                            mass, body_radius);
        if (slot < 0) continue;

        CelestialBody &m = universe_pool[slot];
        m.volume_ratio = seed.real_volume_earth;
        m.self_rotation_time = seed.rotation_hours;
        m.orbital_period = seed.real_period_days;
        m.collision_radius = physical_orbit *
                             seed.real_radius_km / seed.real_orbit_radius_km;
        for (int c = 0; c < 3; ++c) {
            m.color[c] = seed.color[c];
        }
        copy_history(m.history, seed.history);

        MoonOrbit &orbit = moon_orbits[slot];
        orbit.parent_index = parent;
        orbit.physical_radius = physical_orbit;
        orbit.visual_radius = visual_orbit;
        orbit.visual_scale = visual_scale;
        orbit.inclination = inclination;
        orbit.real_orbit_radius_km = seed.real_orbit_radius_km;
        orbit.visual_x = p.x + x_rel * visual_scale;
        orbit.visual_y = p.y + y_rel * visual_scale;
        orbit.visual_z = p.z + z_rel * visual_scale;
    }
}

static void seed_solar_system() {
    const double sun_mass = 1.0;
    const double sun_radius = 0.060;

    int sun = add_body(BodyType::Star, "Sun",
                       0.0, 0.0, 0.0,
                       0.0, 0.0, 0.0,
                       sun_mass, sun_radius);
    if (sun >= 0) {
        CelestialBody &b = universe_pool[sun];
        b.volume_ratio = 1300000.0;
        b.self_rotation_time = 609.12;
        b.orbital_period = 0.0;
        b.collision_radius = 0.38 * (696340.0 / 149.6e6);
        copy_history(b.history,
                     "The Sun contains 99.86 percent of the mass of the solar system. "
                     "Its gravity holds every planet on its orbit. The core fuses about "
                     "600 million tonnes of hydrogen into helium every second.");
    }

    for (int i = 0; i < 8; ++i) {
        const PlanetSeed &seed = PLANET_SEEDS[i];
        double angle = seed.start_angle;
        double x = seed.orbit_radius * std::cos(angle);
        double z = seed.orbit_radius * std::sin(angle);

        // Circular-orbit speed for the softened force:
        // v = sqrt(G * M * r) / (r + epsilon), tangent in the x-z plane.
        double speed = std::sqrt(G_CONSTANT * sun_mass * seed.orbit_radius) /
                       (seed.orbit_radius + SOFTENING);
        double vx = -speed * std::sin(angle);
        double vz = speed * std::cos(angle);

        double mass = seed.mass_earth * EARTH_MASS_SIM;
        int slot = add_body(BodyType::Planet, seed.name,
                            x, 0.0, z,
                            vx, 0.0, vz,
                            mass, seed.visual_radius);
        if (slot < 0) continue;

        CelestialBody &b = universe_pool[slot];
        b.volume_ratio = seed.volume_earth;
        b.self_rotation_time = seed.rotation_hours;
        b.orbital_period = seed.period_days;
        b.collision_radius = seed.orbit_radius *
                             seed.real_radius_km / seed.real_orbit_radius_km;
        for (int c = 0; c < 3; ++c) {
            b.color[c] = seed.color[c];
        }
        copy_history(b.history, seed.history);
    }

    // Put the centre of mass at rest in 3D.
    if (sun >= 0 && universe_pool[sun].mass > 0.0) {
        double total_px = 0.0;
        double total_py = 0.0;
        double total_pz = 0.0;
        for (int i = 0; i < MAX_OBJECTS; ++i) {
            if (!universe_pool[i].active || i == sun) continue;
            total_px += universe_pool[i].mass * universe_pool[i].vx;
            total_py += universe_pool[i].mass * universe_pool[i].vy;
            total_pz += universe_pool[i].mass * universe_pool[i].vz;
        }
        universe_pool[sun].vx = -total_px / universe_pool[sun].mass;
        universe_pool[sun].vy = -total_py / universe_pool[sun].mass;
        universe_pool[sun].vz = -total_pz / universe_pool[sun].mass;
    }
}

static void reset_universe() {
    universe_pool.fill(CelestialBody{});
    moon_orbits.fill(MoonOrbit{});
    active_object_count = 0;
    selected_body_index = -1;
    time_multiplier = 1.0;
    paused = false;
    sim_time = 0.0;
    for (int i = 0; i < BODY_TYPE_COUNT; ++i) {
        spawn_counter[i] = 0;
    }
    seed_solar_system();
    seed_moons();
    set_status("Solar system seeded with %d bodies", active_object_count);
}

// ============================================================================
//  7. Physics
// ============================================================================

static void compute_gravity() {
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe_pool[i].active) continue;
        universe_pool[i].ax = 0.0;
        universe_pool[i].ay = 0.0;
        universe_pool[i].az = 0.0;
    }

    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe_pool[i].active) continue;
        CelestialBody &a = universe_pool[i];

        for (int j = i + 1; j < MAX_OBJECTS; ++j) {
            if (!universe_pool[j].active) continue;
            CelestialBody &b = universe_pool[j];

            double dx = b.x - a.x;
            double dy = b.y - a.y;
            double dz = b.z - a.z;
            double distance = std::sqrt(dx * dx + dy * dy + dz * dz);

            double epsilon = SOFTENING;
            if (moon_orbits[i].parent_index == j ||
                moon_orbits[j].parent_index == i) {
                epsilon = MOON_SOFTENING;
            }
            double softened = distance + epsilon;

            if (distance > 1.0e-12) {
                // Unit direction is (dx,dy,dz)/distance; the force magnitude
                // is G*m/(distance + epsilon)^2.
                double factor = G_CONSTANT / (softened * softened * distance);
                a.ax += factor * b.mass * dx;
                a.ay += factor * b.mass * dy;
                a.az += factor * b.mass * dz;
                b.ax -= factor * a.mass * dx;
                b.ay -= factor * a.mass * dy;
                b.az -= factor * a.mass * dz;
            }
        }
    }
}

// The heavier body keeps its identity; the lighter one is removed.
// Mass and momentum are conserved, and the result moves to the centre of mass.
static void merge_bodies(int keep, int gone) {
    if (keep < 0 || keep >= MAX_OBJECTS || gone < 0 || gone >= MAX_OBJECTS) return;
    if (keep == gone) return;
    if (!universe_pool[keep].active || !universe_pool[gone].active) return;

    CelestialBody &a = universe_pool[keep];
    CelestialBody &b = universe_pool[gone];

    char absorbed_name[NAME_STR_LEN];
    copy_name(absorbed_name, b.name);

    double mass_a = a.mass;
    double mass_b = b.mass;
    double total_mass = mass_a + mass_b;
    if (total_mass <= 0.0) {
        remove_body(gone);
        return;
    }

    a.vx = (mass_a * a.vx + mass_b * b.vx) / total_mass;
    a.vy = (mass_a * a.vy + mass_b * b.vy) / total_mass;
    a.vz = (mass_a * a.vz + mass_b * b.vz) / total_mass;
    a.x = (mass_a * a.x + mass_b * b.x) / total_mass;
    a.y = (mass_a * a.y + mass_b * b.y) / total_mass;
    a.z = (mass_a * a.z + mass_b * b.z) / total_mass;
    a.mass = total_mass;

    // In 3D the radius grows with the cube root of mass.
    if (mass_a > 0.0 && a.radius > 0.0) {
        a.radius *= std::cbrt(total_mass / mass_a);
    } else if (a.radius <= 0.0) {
        a.radius = b.radius;
    }

    a.volume_ratio += b.volume_ratio;

    if (selected_body_index == gone) {
        selected_body_index = keep;
    }

    universe_pool[gone] = CelestialBody{};
    moon_orbits[gone] = MoonOrbit{};
    --active_object_count;
    if (active_object_count < 0) {
        active_object_count = 0;
    }
    detach_moon_children(gone);
    set_status("%s absorbed %s", a.name, absorbed_name);
}

static void resolve_collisions() {
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe_pool[i].active) continue;

        for (int j = i + 1; j < MAX_OBJECTS; ++j) {
            if (!universe_pool[j].active) continue;

            CelestialBody &a = universe_pool[i];
            CelestialBody &b = universe_pool[j];

            double radii = a.collision_radius + b.collision_radius;
            if (radii <= 0.0) radii = a.radius + b.radius;

            // Current separation.
            double dx = b.x - a.x;
            double dy = b.y - a.y;
            double dz = b.z - a.z;
            double distance_sq = dx * dx + dy * dy + dz * dz;

            bool hit = (distance_sq < radii * radii);

            // Swept test: catch fast bodies that crossed between frames.
            if (!hit) {
                double p0x = prev_x[j] - prev_x[i];
                double p0y = prev_y[j] - prev_y[i];
                double p0z = prev_z[j] - prev_z[i];
                double sx = dx - p0x;
                double sy = dy - p0y;
                double sz = dz - p0z;
                double ss = sx * sx + sy * sy + sz * sz;

                double t = 0.0;
                if (ss > 1.0e-18) {
                    t = -(p0x * sx + p0y * sy + p0z * sz) / ss;
                    t = clamp_double(t, 0.0, 1.0);
                }
                double cx = p0x + t * sx;
                double cy = p0y + t * sy;
                double cz = p0z + t * sz;
                hit = (cx * cx + cy * cy + cz * cz < radii * radii);
            }

            if (hit) {
                if (a.mass >= b.mass) {
                    merge_bodies(i, j);
                } else {
                    merge_bodies(j, i);
                    break;  // body i was absorbed
                }
            }
        }
    }
}

static void step_physics(double dt) {
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe_pool[i].active) continue;
        prev_x[i] = universe_pool[i].x;
        prev_y[i] = universe_pool[i].y;
        prev_z[i] = universe_pool[i].z;
    }

    compute_gravity();

    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe_pool[i].active) continue;
        CelestialBody &b = universe_pool[i];

        // Semi-implicit Euler: update velocity first, then position.
        b.vx += b.ax * dt;
        b.vy += b.ay * dt;
        b.vz += b.az * dt;
        b.x += b.vx * dt;
        b.y += b.vy * dt;
        b.z += b.vz * dt;

        if (!body_is_finite(b)) {
            char bad_name[NAME_STR_LEN];
            copy_name(bad_name, b.name);
            remove_body(i);
            set_status("Removed unstable body %s", bad_name);
        }
    }

    resolve_collisions();
    sim_time += dt;
}

// Choose enough small steps that even a fast meteor cannot jump through a
// planet in one frame.  The count is based on the fastest body and the
// smallest radius currently active.
static int compute_substeps(double dt) {
    double max_speed = 0.0;
    double min_radius = 1.0e30;

    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe_pool[i].active) continue;
        const CelestialBody &b = universe_pool[i];
        double speed = std::sqrt(b.vx * b.vx + b.vy * b.vy + b.vz * b.vz);
        if (speed > max_speed) max_speed = speed;
        if (b.radius > 0.0 && b.radius < min_radius) min_radius = b.radius;
    }

    if (min_radius > 1.0e29) return 1;

    double travel = max_speed * std::fabs(dt);
    int steps = static_cast<int>(travel / (0.5 * min_radius)) + 1;
    return clamp_int(steps, 1, SUBSTEPS_MAX);
}

// Place parented moons on their compressed circular orbits.  This is a
// visual/orbital model: the parent's N-body motion carries the moon, while
// the moon's relative orbit uses real order, inclination and HUD data.
// The moon's physical position is used by gravity.  The renderer uses an
// exaggerated visual position so the compressed solar system can show moons.
static void update_moon_visuals() {
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe_pool[i].active) continue;
        MoonOrbit &orbit = moon_orbits[i];
        if (orbit.parent_index < 0) continue;
        if (orbit.parent_index >= MAX_OBJECTS) continue;
        if (!universe_pool[orbit.parent_index].active) continue;

        const CelestialBody &parent = universe_pool[orbit.parent_index];
        const CelestialBody &moon = universe_pool[i];
        double scale = orbit.visual_scale;
        if (scale <= 0.0) scale = 1.0;

        double dx = moon.x - parent.x;
        double dy = moon.y - parent.y;
        double dz = moon.z - parent.z;
        double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (distance > 1.0e-12) {
            orbit.visual_radius = distance * scale;
        }

        orbit.visual_x = parent.x + dx * scale;
        orbit.visual_y = parent.y + dy * scale;
        orbit.visual_z = parent.z + dz * scale;
    }
}

static void body_visual_position(int index, double out[3]) {
    out[0] = 0.0;
    out[1] = 0.0;
    out[2] = 0.0;
    if (index < 0 || index >= MAX_OBJECTS) return;
    if (!universe_pool[index].active) return;

    const MoonOrbit &orbit = moon_orbits[index];
    if (orbit.parent_index >= 0) {
        out[0] = orbit.visual_x;
        out[1] = orbit.visual_y;
        out[2] = orbit.visual_z;
    } else {
        out[0] = universe_pool[index].x;
        out[1] = universe_pool[index].y;
        out[2] = universe_pool[index].z;
    }
}

// ============================================================================
//  8. Cosmetic particles (not part of the physics pool)
// ============================================================================

struct StarPoint {
    float x;
    float y;
    float z;
    float brightness;
};

struct BeltParticle {
    double angle;
    double radius;
    double y;
    double omega;
};

static StarPoint starfield[STAR_COUNT];
static BeltParticle belt[BELT_COUNT];
static unsigned int rng_state = 0x12345678u;

static double random_unit() {
    rng_state = rng_state * 1664525u + 1013904223u;
    return static_cast<double>(rng_state & 0x00FFFFFFu) / 16777216.0;
}

static void init_cosmetic() {
    rng_state = 0x12345678u;

    // Stars on a large sphere around the scene.
    for (int i = 0; i < STAR_COUNT; ++i) {
        double u = 2.0 * random_unit() - 1.0;
        double theta = std::acos(u);
        double phi = 2.0 * PI * random_unit();
        double radius = 18.0 + 4.0 * random_unit();
        double sin_theta = std::sin(theta);
        starfield[i].x = static_cast<float>(radius * sin_theta * std::cos(phi));
        starfield[i].y = static_cast<float>(radius * std::cos(theta));
        starfield[i].z = static_cast<float>(radius * sin_theta * std::sin(phi));
        starfield[i].brightness = 0.25f + static_cast<float>(0.70 * random_unit());
    }

    // Asteroid belt between Mars and Jupiter, with a little vertical spread.
    for (int i = 0; i < BELT_COUNT; ++i) {
        belt[i].angle = 2.0 * PI * random_unit();
        belt[i].radius = 0.520 + 0.080 * random_unit();
        belt[i].y = (random_unit() - 0.5) * 0.030;
        belt[i].omega = std::sqrt(G_CONSTANT /
                                  (belt[i].radius * belt[i].radius * belt[i].radius));
    }
}

static void update_cosmetic(double dt) {
    for (int i = 0; i < BELT_COUNT; ++i) {
        belt[i].angle += belt[i].omega * dt;
        if (belt[i].angle > 2.0 * PI) {
            belt[i].angle -= 2.0 * PI;
        }
    }
}

// ============================================================================
//  9. Camera and rendering
// ============================================================================

static int hud_panel_width() {
    int width = std::min(HUD_PANEL_MAX_WIDTH, winWidth - 2 * HUD_MARGIN);
    if (width < 200) width = 200;
    return width;
}

static int simulation_width() {
    int width = winWidth - hud_panel_width() - 2 * HUD_MARGIN;
    if (width < 200) width = winWidth;  // very narrow window: overlay fallback
    return width;
}

static void update_sim_width() {
    if (winWidth < 1) winWidth = 1;
    if (winHeight < 1) winHeight = 1;
    sim_width = simulation_width();
    if (sim_width < 1) sim_width = 1;
}

static void camera_reset() {
    cam_azimuth = CAMERA_DEFAULT_AZIMUTH;
    cam_elevation = CAMERA_DEFAULT_ELEVATION;
    cam_distance = CAMERA_DEFAULT_DISTANCE;
    cam_target_x = 0.0;
    cam_target_y = 0.0;
    cam_target_z = 0.0;
    follow_selected = false;
}

static void camera_rotate(double dx, double dy) {
    cam_azimuth += dx * 0.35;
    cam_elevation += dy * 0.35;
    cam_elevation = clamp_double(cam_elevation, 5.0, 85.0);
}

static void camera_zoom(double steps) {
    cam_distance *= std::pow(1.12, -steps);
    cam_distance = clamp_double(cam_distance,
                                CAMERA_MIN_DISTANCE,
                                CAMERA_MAX_DISTANCE);
}

static void camera_pan(double dx, double dy) {
    double azimuth = cam_azimuth * PI / 180.0;
    double scale = cam_distance * 0.0016;
    double right_x = std::sin(azimuth);
    double right_z = -std::cos(azimuth);

    cam_target_x -= right_x * dx * scale;
    cam_target_z -= right_z * dx * scale;
    cam_target_y += dy * scale;
    follow_selected = false;
}

static void camera_update_target() {
    if (!follow_selected) return;
    if (selected_body_index < 0 || selected_body_index >= MAX_OBJECTS ||
        !universe_pool[selected_body_index].active) {
        follow_selected = false;
        cam_target_x = 0.0;
        cam_target_y = 0.0;
        cam_target_z = 0.0;
        return;
    }
    double position[3];
    body_visual_position(selected_body_index, position);
    cam_target_x = position[0];
    cam_target_y = position[1];
    cam_target_z = position[2];
}

static void camera_apply() {
    camera_update_target();

    double azimuth = cam_azimuth * PI / 180.0;
    double elevation = cam_elevation * PI / 180.0;
    double horizontal = cam_distance * std::cos(elevation);

    double eye_x = cam_target_x + horizontal * std::cos(azimuth);
    double eye_y = cam_target_y + cam_distance * std::sin(elevation);
    double eye_z = cam_target_z + horizontal * std::sin(azimuth);

    gluLookAt(eye_x, eye_y, eye_z,
              cam_target_x, cam_target_y, cam_target_z,
              0.0, 1.0, 0.0);
}

static int find_light_source() {
    int sun = find_body_index_by_name("Sun");
    if (sun >= 0 && universe_pool[sun].active) return sun;

    int best = -1;
    double best_mass = -1.0;
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe_pool[i].active) continue;
        if (universe_pool[i].type != BodyType::Star) continue;
        if (universe_pool[i].mass > best_mass) {
            best_mass = universe_pool[i].mass;
            best = i;
        }
    }
    return best;
}

static void set_perspective() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(CAMERA_FOV,
                   static_cast<double>(sim_width) / static_cast<double>(winHeight),
                   CAMERA_NEAR, CAMERA_FAR);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void set_light() {
    int light = find_light_source();
    if (light < 0) {
        glDisable(GL_LIGHT0);
        return;
    }

    const CelestialBody &b = universe_pool[light];
    GLfloat position[4] = {static_cast<GLfloat>(b.x),
                           static_cast<GLfloat>(b.y),
                           static_cast<GLfloat>(b.z),
                           1.0f};
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
}

static void draw_starfield() {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glPointSize(1.6f);
    glBegin(GL_POINTS);
    for (int i = 0; i < STAR_COUNT; ++i) {
        float b = starfield[i].brightness;
        glColor3f(b, b, b);
        glVertex3f(starfield[i].x, starfield[i].y, starfield[i].z);
    }
    glEnd();
    glPointSize(1.0f);
    glEnable(GL_DEPTH_TEST);
}

static void draw_orbit(int index) {
    const CelestialBody &b = universe_pool[index];
    if (!b.active || b.type != BodyType::Planet) return;

    int sun = find_light_source();
    double center_x = 0.0;
    double center_y = 0.0;
    double center_z = 0.0;
    if (sun >= 0) {
        center_x = universe_pool[sun].x;
        center_y = universe_pool[sun].y;
        center_z = universe_pool[sun].z;
    }

    double dx = b.x - center_x;
    double dy = b.y - center_y;
    double dz = b.z - center_z;
    double radius = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (radius < 0.01) return;

    if (index == selected_body_index) {
        glColor4f(0.78f, 0.88f, 1.00f, 0.80f);
        glLineWidth(1.5f);
    } else {
        glColor4f(0.38f, 0.48f, 0.68f, 0.52f);
        glLineWidth(1.0f);
    }

    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 180; ++i) {
        double angle = 2.0 * PI * static_cast<double>(i) / 180.0;
        glVertex3d(center_x + radius * std::cos(angle),
                   center_y,
                   center_z + radius * std::sin(angle));
    }
    glEnd();
    glLineWidth(1.0f);
}

static void draw_moon_orbit(int index) {
    const MoonOrbit &orbit = moon_orbits[index];
    if (orbit.parent_index < 0 || orbit.parent_index >= MAX_OBJECTS) return;
    if (!universe_pool[orbit.parent_index].active) return;

    const CelestialBody &parent = universe_pool[orbit.parent_index];

    glPushMatrix();
    glTranslatef(static_cast<GLfloat>(parent.x),
                 static_cast<GLfloat>(parent.y),
                 static_cast<GLfloat>(parent.z));
    glRotatef(static_cast<GLfloat>(orbit.inclination * 180.0 / PI),
              1.0f, 0.0f, 0.0f);

    if (index == selected_body_index) {
        glColor4f(0.80f, 0.90f, 1.00f, 0.70f);
        glLineWidth(1.4f);
    } else {
        glColor4f(0.48f, 0.55f, 0.65f, 0.35f);
        glLineWidth(1.0f);
    }

    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 96; ++i) {
        double angle = 2.0 * PI * static_cast<double>(i) / 96.0;
        glVertex3d(orbit.visual_radius * std::cos(angle),
                   0.0,
                   orbit.visual_radius * std::sin(angle));
    }
    glEnd();
    glLineWidth(1.0f);
    glPopMatrix();
}

static void draw_belt() {
    glDisable(GL_LIGHTING);
    glPointSize(1.7f);
    glBegin(GL_POINTS);
    for (int i = 0; i < BELT_COUNT; ++i) {
        double x = belt[i].radius * std::cos(belt[i].angle);
        double z = belt[i].radius * std::sin(belt[i].angle);
        float shade = 0.42f + static_cast<float>((i % 5) * 0.035);
        glColor3f(shade, shade * 0.92f, shade * 0.82f);
        glVertex3d(x, belt[i].y, z);
    }
    glEnd();
    glPointSize(1.0f);
}

static void draw_saturn_ring() {
    int saturn = find_body_index_by_name("Saturn");
    if (saturn < 0 || !universe_pool[saturn].active) return;

    const CelestialBody &s = universe_pool[saturn];
    double inner = s.radius * 1.60;
    double outer = s.radius * 2.45;
    const int segments = 96;

    glPushMatrix();
    glTranslatef(static_cast<GLfloat>(s.x),
                 static_cast<GLfloat>(s.y),
                 static_cast<GLfloat>(s.z));
    glRotatef(26.7f, 1.0f, 0.0f, 0.0f);

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glColor4f(0.95f, 0.88f, 0.70f, 0.68f);
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= segments; ++i) {
        double angle = 2.0 * PI * static_cast<double>(i) / segments;
        double ca = std::cos(angle);
        double sa = std::sin(angle);
        glVertex3d(inner * ca, 0.0, inner * sa);
        glVertex3d(outer * ca, 0.0, outer * sa);
    }
    glEnd();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

static void draw_comet_tail(int index) {
    int star = nearest_star_index(index);
    if (star < 0) return;

    const CelestialBody &b = universe_pool[index];
    const CelestialBody &s = universe_pool[star];

    double dx = b.x - s.x;
    double dy = b.y - s.y;
    double dz = b.z - s.z;
    double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (distance < 1.0e-6) return;

    double length = 0.040 + 0.040 / (distance + 0.10);
    length = clamp_double(length, 0.030, 0.100);

    double ux = dx / distance;
    double uy = dy / distance;
    double uz = dz / distance;

    glDisable(GL_LIGHTING);
    glLineWidth(2.0f);
    glColor4f(0.65f, 0.92f, 1.00f, 0.75f);
    glBegin(GL_LINES);
    glVertex3d(b.x, b.y, b.z);
    glVertex3d(b.x + ux * length, b.y + uy * length, b.z + uz * length);
    glEnd();
    glLineWidth(1.0f);
}

static void draw_meteor_streak(int index) {
    const CelestialBody &b = universe_pool[index];
    double speed = std::sqrt(b.vx * b.vx + b.vy * b.vy + b.vz * b.vz);
    if (speed < 1.0e-6) return;

    double length = std::min(0.060, speed * 0.012);
    double ux = b.vx / speed;
    double uy = b.vy / speed;
    double uz = b.vz / speed;

    glDisable(GL_LIGHTING);
    glLineWidth(2.0f);
    glColor4f(1.00f, 0.90f, 0.60f, 0.60f);
    glBegin(GL_LINES);
    glVertex3d(b.x, b.y, b.z);
    glVertex3d(b.x - ux * length, b.y - uy * length, b.z - uz * length);
    glEnd();
    glLineWidth(1.0f);
}

static void draw_body(int index) {
    const CelestialBody &b = universe_pool[index];
    if (!b.active) return;

    double position[3];
    body_visual_position(index, position);

    glPushMatrix();
    glTranslatef(static_cast<GLfloat>(position[0]),
                 static_cast<GLfloat>(position[1]),
                 static_cast<GLfloat>(position[2]));

    GLfloat diffuse[4] = {b.color[0], b.color[1], b.color[2], 1.0f};
    GLfloat specular[4] = {0.40f, 0.40f, 0.40f, 1.0f};
    GLfloat black_material[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    GLfloat zero_emission[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    if (b.type == BodyType::Star) {
        GLfloat emission[4] = {b.color[0], b.color[1], b.color[2], 1.0f};
        GLfloat sun_diffuse[4] = {b.color[0] * 0.35f,
                                  b.color[1] * 0.35f,
                                  b.color[2] * 0.35f, 1.0f};
        glMaterialfv(GL_FRONT, GL_EMISSION, emission);
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, sun_diffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR, black_material);
        glMaterialf(GL_FRONT, GL_SHININESS, 0.0f);
        gluSphere(sphere_quadric, b.radius, 32, 24);
        glMaterialfv(GL_FRONT, GL_EMISSION, zero_emission);
    } else if (b.type == BodyType::BlackHole) {
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, black_material);
        glMaterialfv(GL_FRONT, GL_SPECULAR, black_material);
        glMaterialf(GL_FRONT, GL_SHININESS, 0.0f);
        gluSphere(sphere_quadric, b.radius, 24, 16);

        GLfloat ring_emission[4] = {0.75f, 0.40f, 1.00f, 1.0f};
        glMaterialfv(GL_FRONT, GL_EMISSION, ring_emission);
        glPushMatrix();
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        glutSolidTorus(b.radius * 1.35, b.radius * 1.85, 12, 32);
        glPopMatrix();
        glMaterialfv(GL_FRONT, GL_EMISSION, zero_emission);
    } else {
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, diffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
        glMaterialf(GL_FRONT, GL_SHININESS, 24.0f);

        int slices = 24;
        int stacks = 16;
        if (b.type == BodyType::Moon || b.type == BodyType::Asteroid ||
            b.type == BodyType::Meteor || b.type == BodyType::Comet) {
            slices = 12;
            stacks = 8;
        }
        gluSphere(sphere_quadric, b.radius, slices, stacks);
    }

    glPopMatrix();
}

static void draw_effects() {
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe_pool[i].active) continue;
        if (universe_pool[i].type == BodyType::Comet) {
            draw_comet_tail(i);
        } else if (universe_pool[i].type == BodyType::Meteor) {
            draw_meteor_streak(i);
        }
    }
}

static void draw_bodies() {
    glEnable(GL_LIGHTING);
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (universe_pool[i].active) {
            draw_body(i);
        }
    }
}

static void draw_sun_glow() {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe_pool[i].active) continue;
        if (universe_pool[i].type != BodyType::Star) continue;
        const CelestialBody &b = universe_pool[i];
        glPushMatrix();
        glTranslatef(static_cast<GLfloat>(b.x),
                     static_cast<GLfloat>(b.y),
                     static_cast<GLfloat>(b.z));
        glColor4f(b.color[0], b.color[1], b.color[2], 0.11f);
        gluSphere(sphere_quadric, b.radius * 1.50, 16, 12);
        glColor4f(b.color[0], b.color[1], b.color[2], 0.045f);
        gluSphere(sphere_quadric, b.radius * 2.10, 16, 12);
        glPopMatrix();
    }

    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

static void draw_selection() {
    if (selected_body_index < 0 || selected_body_index >= MAX_OBJECTS) return;
    const CelestialBody &b = universe_pool[selected_body_index];
    if (!b.active) return;

    double position[3];
    body_visual_position(selected_body_index, position);

    glDisable(GL_LIGHTING);
    glColor4f(1.0f, 1.0f, 1.0f, 0.80f);
    glLineWidth(1.0f);
    glPushMatrix();
    glTranslatef(static_cast<GLfloat>(position[0]),
                 static_cast<GLfloat>(position[1]),
                 static_cast<GLfloat>(position[2]));
    glutWireSphere(b.radius * 1.25 + 0.002, 12, 6);
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

static void draw_spawn_marker() {
    if (!spawn_cursor_valid) return;

    glDisable(GL_LIGHTING);
    glColor4f(0.80f, 0.90f, 1.00f, 0.75f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 32; ++i) {
        double angle = 2.0 * PI * static_cast<double>(i) / 32.0;
        glVertex3d(spawn_x + 0.012 * std::cos(angle),
                   0.0,
                   spawn_z + 0.012 * std::sin(angle));
    }
    glEnd();
    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
}

static void collect_labels() {
    screen_label_count = 0;

    GLdouble model[16];
    GLdouble projection[16];
    GLint viewport[4] = {0, 0, sim_width, winHeight};
    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);

    int light = find_light_source();
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe_pool[i].active) continue;
        const CelestialBody &b = universe_pool[i];

        if (b.type == BodyType::Asteroid ||
            b.type == BodyType::Meteor ||
            b.type == BodyType::Moon) {
            if (i != selected_body_index) continue;
        }

        // The inner planets project too close together from the default
        // camera; label them only when selected.
        if (b.type == BodyType::Planet && i != selected_body_index && light >= 0) {
            double dx = b.x - universe_pool[light].x;
            double dz = b.z - universe_pool[light].z;
            double orbit_radius = std::sqrt(dx * dx + dz * dz);
            if (orbit_radius < 0.45) continue;
        }

        double position[3];
        body_visual_position(i, position);

        GLdouble sx = 0.0;
        GLdouble sy = 0.0;
        GLdouble sz = 0.0;
        if (gluProject(position[0], position[1] + b.radius + 0.020, position[2],
                       model, projection, viewport, &sx, &sy, &sz) != GL_TRUE) {
            continue;
        }
        if (sz < 0.0 || sz > 1.0) continue;
        if (sx < 0.0 || sx >= sim_width || sy < 0.0 || sy >= winHeight) continue;
        // Keep labels out of the top HUD band and the bottom controls band.
        if (sy < 120.0 || sy > static_cast<double>(winHeight - 190)) continue;

        ScreenLabel &label = screen_labels[screen_label_count];
        label.x = static_cast<int>(sx);
        label.y = static_cast<int>(sy);
        copy_name(label.text, b.name);
        if (i == selected_body_index) {
            label.color[0] = 1.00f;
            label.color[1] = 0.95f;
            label.color[2] = 0.60f;
        } else {
            label.color[0] = 0.62f;
            label.color[1] = 0.68f;
            label.color[2] = 0.80f;
        }
        ++screen_label_count;
        if (screen_label_count >= MAX_OBJECTS) break;
    }
}

static void render_scene() {
    update_sim_width();
    glViewport(0, 0, sim_width, winHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    set_perspective();
    camera_apply();
    set_light();

    glEnable(GL_DEPTH_TEST);

    draw_starfield();

    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (universe_pool[i].active) {
            draw_orbit(i);
            draw_moon_orbit(i);
        }
    }
    draw_belt();
    draw_effects();
    draw_bodies();
    draw_saturn_ring();
    draw_sun_glow();
    draw_selection();
    draw_spawn_marker();

    collect_labels();
}

// ============================================================================
//  10. HUD
// ============================================================================

static void setup_hud_projection() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0.0, static_cast<double>(winWidth), 0.0, static_cast<double>(winHeight));

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static void restore_hud_projection() {
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}

static void draw_text(int x, int y, const char *text, void *font) {
    if (text == nullptr) return;
    glRasterPos2i(x, y);
    for (const char *p = text; *p != '\0'; ++p) {
        glutBitmapCharacter(font, static_cast<int>(static_cast<unsigned char>(*p)));
    }
}

static int text_width_n(const char *text, int count, void *font) {
    int width = 0;
    for (int i = 0; i < count && text[i] != '\0'; ++i) {
        width += glutBitmapWidth(font, static_cast<int>(static_cast<unsigned char>(text[i])));
    }
    return width;
}

static void draw_panel(int x, int y, int width, int height) {
    glColor4f(0.03f, 0.05f, 0.10f, 0.86f);
    glBegin(GL_QUADS);
    glVertex2i(x, y);
    glVertex2i(x + width, y);
    glVertex2i(x + width, y + height);
    glVertex2i(x, y + height);
    glEnd();

    glColor4f(0.25f, 0.35f, 0.55f, 0.90f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2i(x, y);
    glVertex2i(x + width, y);
    glVertex2i(x + width, y + height);
    glVertex2i(x, y + height);
    glEnd();
}

static void draw_strip(int x, int y, int width, int height, float alpha) {
    glColor4f(0.02f, 0.03f, 0.07f, alpha);
    glBegin(GL_QUADS);
    glVertex2i(x, y);
    glVertex2i(x + width, y);
    glVertex2i(x + width, y + height);
    glVertex2i(x, y + height);
    glEnd();
}

static int draw_wrapped_text(int x, int y, int max_width, const char *text,
                             void *font, int line_height, int max_lines) {
    if (text == nullptr || max_lines <= 0) return 0;

    char line[256];
    int line_len = 0;
    int line_width = 0;
    int lines = 0;
    const char *p = text;

    while (*p != '\0' && lines < max_lines) {
        while (*p == ' ') ++p;
        if (*p == '\0') break;

        const char *word_start = p;
        while (*p != '\0' && *p != ' ') ++p;
        int word_len = static_cast<int>(p - word_start);
        int word_width = text_width_n(word_start, word_len, font);

        int space_width = (line_len > 0) ? glutBitmapWidth(font, ' ') : 0;
        if (line_len > 0 && line_width + space_width + word_width > max_width) {
            line[line_len] = '\0';
            draw_text(x, y, line, font);
            y -= line_height;
            ++lines;
            line_len = 0;
            line_width = 0;
            if (lines >= max_lines) break;
        }

        if (line_len > 0) {
            line[line_len++] = ' ';
            line_width += space_width;
        }

        for (int i = 0; i < word_len && line_len < 255; ++i) {
            line[line_len++] = word_start[i];
        }
        line_width += word_width;
    }

    if (line_len > 0 && lines < max_lines) {
        line[line_len] = '\0';
        draw_text(x, y, line, font);
        ++lines;
    }

    return lines;
}

static void format_number(double value, char *out, int size) {
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

static void draw_projected_labels() {
    for (int i = 0; i < screen_label_count; ++i) {
        const ScreenLabel &label = screen_labels[i];
        int length = static_cast<int>(std::strlen(label.text));
        int label_width = text_width_n(label.text, length, GLUT_BITMAP_HELVETICA_18);
        if (label.x + label_width + 8 > sim_width) continue;

        glColor4f(0.01f, 0.02f, 0.05f, 0.58f);
        glBegin(GL_QUADS);
        glVertex2i(label.x, label.y - 2);
        glVertex2i(label.x + label_width + 8, label.y - 2);
        glVertex2i(label.x + label_width + 8, label.y + 22);
        glVertex2i(label.x, label.y + 22);
        glEnd();

        glColor3f(label.color[0], label.color[1], label.color[2]);
        draw_text(label.x + 4, label.y + 4, label.text, GLUT_BITMAP_HELVETICA_18);
    }
}

static void draw_hud() {
    glViewport(0, 0, winWidth, winHeight);
    setup_hud_projection();

    const int margin = HUD_MARGIN;
    char buffer[256];
    char number[40];

    // Readability strips behind the top and bottom HUD text.
    draw_strip(0, winHeight - 190, sim_width, 190, 0.40f);
    draw_strip(0, 0, winWidth, 120, 0.32f);

    draw_projected_labels();

    glColor3f(1.00f, 0.85f, 0.35f);
    draw_text(margin, winHeight - 40, "N-BODY SPACE SIMULATOR - 3D - V3",
              GLUT_BITMAP_TIMES_ROMAN_24);
    glColor3f(0.55f, 0.75f, 1.00f);
    draw_text(margin, winHeight - 70, "Gravity sandbox - click a body to inspect it",
              GLUT_BITMAP_HELVETICA_18);

    glColor3f(0.70f, 0.85f, 0.75f);
    std::snprintf(buffer, sizeof(buffer),
                  "Bodies: %d   FPS: %d   Speed: %.2fx%s   Sim time: %.1f",
                  active_object_count, fps, time_multiplier,
                  paused ? "   [PAUSED]" : "", sim_time);
    draw_text(margin, winHeight - 104, buffer, GLUT_BITMAP_HELVETICA_18);

    const char *velocity_label = "orbital";
    if (spawn_velocity_mode == 1) velocity_label = "retrograde";
    else if (spawn_velocity_mode == 2) velocity_label = "zero";
    glColor3f(0.70f, 0.70f, 0.85f);
    std::snprintf(buffer, sizeof(buffer),
                  "Spawn: %s   mass x%.2f   radius x%.2f   velocity: %s",
                  type_name(spawn_type),
                  static_cast<double>(spawn_mass_percent) / 100.0,
                  static_cast<double>(spawn_radius_percent) / 100.0,
                  velocity_label);
    draw_text(margin, winHeight - 134, buffer, GLUT_BITMAP_HELVETICA_18);

    if (status_frames > 0) {
        glColor3f(1.00f, 0.75f, 0.30f);
        draw_text(margin, winHeight - 164, status_message, GLUT_BITMAP_HELVETICA_18);
    }

    glColor3f(0.62f, 0.68f, 0.78f);
    draw_text(margin, 92,
              "Space pause | +/- speed | 1-7 type | A add | D delete | V velocity | R reset | Q quit",
              GLUT_BITMAP_HELVETICA_18);
    draw_text(margin, 64,
              "Left-drag rotate | Wheel zoom | Right-drag pan | F follow | Home camera | L labels",
              GLUT_BITMAP_HELVETICA_18);
    draw_text(margin, 36,
              "[ ] mass scale | , . radius scale | Click a body to select | Esc quit",
              GLUT_BITMAP_HELVETICA_18);

    const bool has_selection = (selected_body_index >= 0 &&
                                selected_body_index < MAX_OBJECTS &&
                                universe_pool[selected_body_index].active);
    const int panel_width = hud_panel_width();
    int panel_height = has_selection ? 640 : 320;
    panel_height = std::min(panel_height, winHeight - 2 * margin);
    const int panel_x = winWidth - panel_width - margin;
    const int panel_y = winHeight - panel_height - margin;

    draw_panel(panel_x, panel_y, panel_width, panel_height);

    int x = panel_x + 18;
    int y = panel_y + panel_height - 38;

    if (has_selection) {
        const CelestialBody &b = universe_pool[selected_body_index];
        const MoonOrbit &moon_orbit = moon_orbits[selected_body_index];

        glColor3f(1.00f, 0.85f, 0.35f);
        draw_text(x, y, "SELECTED BODY", GLUT_BITMAP_TIMES_ROMAN_24);
        y -= 32;

        glColor3f(0.96f, 0.96f, 1.00f);
        draw_text(x, y, b.name, GLUT_BITMAP_TIMES_ROMAN_24);
        y -= 30;

        glColor3f(0.60f, 0.75f, 0.95f);
        draw_text(x, y, type_name(b.type), GLUT_BITMAP_HELVETICA_18);
        y -= 30;

        glColor3f(0.86f, 0.89f, 0.96f);

        format_number(b.mass / EARTH_MASS_SIM, number, sizeof(number));
        std::snprintf(buffer, sizeof(buffer), "Mass: %s Earth masses", number);
        draw_text(x, y, buffer, GLUT_BITMAP_HELVETICA_18);
        y -= 22;

        format_number(b.volume_ratio, number, sizeof(number));
        std::snprintf(buffer, sizeof(buffer), "Volume: %s Earths", number);
        draw_text(x, y, buffer, GLUT_BITMAP_HELVETICA_18);
        y -= 22;

        if (b.self_rotation_time > 0.0) {
            std::snprintf(buffer, sizeof(buffer), "Rotation: %.2f hours", b.self_rotation_time);
        } else if (b.self_rotation_time < 0.0) {
            std::snprintf(buffer, sizeof(buffer), "Rotation: %.2f hours (retrograde)",
                          -b.self_rotation_time);
        } else {
            std::snprintf(buffer, sizeof(buffer), "Rotation: n/a");
        }
        draw_text(x, y, buffer, GLUT_BITMAP_HELVETICA_18);
        y -= 22;

        if (b.orbital_period > 0.0) {
            std::snprintf(buffer, sizeof(buffer), "Orbital period: %.1f days", b.orbital_period);
        } else {
            std::snprintf(buffer, sizeof(buffer), "Orbital period: n/a");
        }
        draw_text(x, y, buffer, GLUT_BITMAP_HELVETICA_18);
        y -= 22;

        if (moon_orbit.parent_index >= 0 &&
            moon_orbit.parent_index < MAX_OBJECTS &&
            universe_pool[moon_orbit.parent_index].active) {
            glColor3f(0.72f, 0.84f, 1.00f);
            std::snprintf(buffer, sizeof(buffer), "Parent: %s",
                          universe_pool[moon_orbit.parent_index].name);
            draw_text(x, y, buffer, GLUT_BITMAP_HELVETICA_18);
            y -= 22;

            if (moon_orbit.real_orbit_radius_km > 0.0) {
                std::snprintf(buffer, sizeof(buffer), "Real orbit: %.0f km",
                              moon_orbit.real_orbit_radius_km);
            } else {
                std::snprintf(buffer, sizeof(buffer), "Real orbit: n/a");
            }
            draw_text(x, y, buffer, GLUT_BITMAP_HELVETICA_18);
            y -= 22;

            double parent_radius = universe_pool[moon_orbit.parent_index].radius;
            if (parent_radius > 0.0) {
                std::snprintf(buffer, sizeof(buffer),
                              "Visual orbit: %.2f planet radii (exaggerated)",
                              moon_orbit.visual_radius / parent_radius);
            } else {
                std::snprintf(buffer, sizeof(buffer), "Visual orbit: exaggerated");
            }
            draw_text(x, y, buffer, GLUT_BITMAP_HELVETICA_18);
            y -= 22;
            glColor3f(0.86f, 0.89f, 0.96f);
        }

        std::snprintf(buffer, sizeof(buffer),
                      "Velocity: (%.2f, %.2f, %.2f)", b.vx, b.vy, b.vz);
        draw_text(x, y, buffer, GLUT_BITMAP_HELVETICA_18);
        y -= 22;

        double speed = std::sqrt(b.vx * b.vx + b.vy * b.vy + b.vz * b.vz);
        if (moon_orbit.parent_index >= 0) {
            std::snprintf(buffer, sizeof(buffer), "Speed: %.3f sim units (physical)", speed);
        } else {
            std::snprintf(buffer, sizeof(buffer), "Speed: %.3f sim units", speed);
        }
        draw_text(x, y, buffer, GLUT_BITMAP_HELVETICA_18);
        y -= 22;

        double acceleration = std::sqrt(b.ax * b.ax + b.ay * b.ay + b.az * b.az);
        std::snprintf(buffer, sizeof(buffer), "Acceleration: %.4f sim units", acceleration);
        draw_text(x, y, buffer, GLUT_BITMAP_HELVETICA_18);
        y -= 22;

        double star_x = 0.0;
        double star_y = 0.0;
        double star_z = 0.0;
        double star_distance = distance_to_nearest_star(selected_body_index,
                                                        star_x, star_y, star_z);
        if (star_distance >= 0.0) {
            std::snprintf(buffer, sizeof(buffer),
                          "Distance from Sun: %.3f sim units", star_distance);
        } else {
            std::snprintf(buffer, sizeof(buffer), "Distance from Sun: n/a");
        }
        draw_text(x, y, buffer, GLUT_BITMAP_HELVETICA_18);
        y -= 28;

        glColor3f(0.70f, 0.85f, 0.75f);
        draw_text(x, y, "History:", GLUT_BITMAP_HELVETICA_18);
        y -= 24;

        glColor3f(0.78f, 0.81f, 0.88f);
        int max_lines = (y - (panel_y + 12)) / 22;
        if (max_lines < 1) max_lines = 1;
        draw_wrapped_text(x, y, panel_width - 36, b.history,
                          GLUT_BITMAP_HELVETICA_18, 22, max_lines);
    } else {
        glColor3f(1.00f, 0.85f, 0.35f);
        draw_text(x, y, "NO BODY SELECTED", GLUT_BITMAP_TIMES_ROMAN_24);
        y -= 36;

        glColor3f(0.78f, 0.83f, 0.92f);
        draw_text(x, y, "Click any body to read its data.", GLUT_BITMAP_HELVETICA_18);
        y -= 26;
        draw_text(x, y, "1-7  choose a body type", GLUT_BITMAP_HELVETICA_18);
        y -= 24;
        draw_text(x, y, "A    add it at the cursor", GLUT_BITMAP_HELVETICA_18);
        y -= 24;
        draw_text(x, y, "D    delete the selected body", GLUT_BITMAP_HELVETICA_18);
        y -= 24;
        draw_text(x, y, "Left-drag rotate   Wheel zoom", GLUT_BITMAP_HELVETICA_18);
        y -= 24;
        draw_text(x, y, "F follow   Home reset camera", GLUT_BITMAP_HELVETICA_18);
    }

    if (paused) {
        glColor4f(1.00f, 0.30f, 0.20f, 0.95f);
        draw_text(sim_width / 2 - 60, winHeight - 40, "PAUSED",
                  GLUT_BITMAP_TIMES_ROMAN_24);
    }

    restore_hud_projection();
}

// ============================================================================
//  11. Input, camera controls and picking
// ============================================================================

static void screen_ray(int screen_x, int screen_y,
                       double origin[3], double direction[3]) {
    update_sim_width();

    GLdouble model[16];
    GLdouble projection[16];
    GLint viewport[4] = {0, 0, sim_width, winHeight};

    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);

    GLdouble near_x = 0.0, near_y = 0.0, near_z = 0.0;
    GLdouble far_x = 0.0, far_y = 0.0, far_z = 0.0;
    double window_y = static_cast<double>(winHeight - 1 - screen_y);

    gluUnProject(static_cast<GLdouble>(screen_x), window_y, 0.0,
                 model, projection, viewport,
                 &near_x, &near_y, &near_z);
    gluUnProject(static_cast<GLdouble>(screen_x), window_y, 1.0,
                 model, projection, viewport,
                 &far_x, &far_y, &far_z);

    origin[0] = near_x;
    origin[1] = near_y;
    origin[2] = near_z;
    direction[0] = far_x - near_x;
    direction[1] = far_y - near_y;
    direction[2] = far_z - near_z;
}

static int pick_body(int screen_x, int screen_y) {
    double origin[3];
    double direction[3];
    screen_ray(screen_x, screen_y, origin, direction);

    int best = -1;
    double best_t = 1.0e30;

    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe_pool[i].active) continue;
        const CelestialBody &b = universe_pool[i];
        double center[3];
        body_visual_position(i, center);
        double hit_t = 0.0;
        if (ray_sphere_hit(origin, direction, center, b.radius + 0.010, hit_t)) {
            if (hit_t < best_t) {
                best_t = hit_t;
                best = i;
            }
        }
    }
    return best;
}

static bool ray_plane_y0(const double origin[3], const double direction[3],
                         double &x, double &z) {
    if (std::fabs(direction[1]) < 1.0e-9) return false;
    double t = -origin[1] / direction[1];
    if (t <= 0.0) return false;
    x = origin[0] + t * direction[0];
    z = origin[2] + t * direction[2];
    return true;
}

static void track_mouse(int x, int y) {
    update_sim_width();
    if (x < 0 || x >= sim_width) return;

    double origin[3];
    double direction[3];
    screen_ray(x, y, origin, direction);

    double px = 0.0;
    double pz = 0.0;
    if (ray_plane_y0(origin, direction, px, pz)) {
        spawn_x = clamp_double(px, -1.20, 1.20);
        spawn_z = clamp_double(pz, -1.20, 1.20);
        spawn_cursor_valid = true;
    }
}

static void mouse(int button, int state, int x, int y) {
    update_sim_width();
    if (x >= sim_width) return;

    if (button == 3 && state == GLUT_DOWN) {
        camera_zoom(1.0);
        return;
    }
    if (button == 4 && state == GLUT_DOWN) {
        camera_zoom(-1.0);
        return;
    }

    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            track_mouse(x, y);
            selected_body_index = pick_body(x, y);
            if (selected_body_index < 0) {
                set_status("Selection cleared");
            }
            drag_mode = 1;
            last_mouse_x = x;
            last_mouse_y = y;
        } else if (state == GLUT_UP && drag_mode == 1) {
            drag_mode = 0;
        }
    } else if (button == GLUT_RIGHT_BUTTON) {
        if (state == GLUT_DOWN) {
            drag_mode = 2;
            last_mouse_x = x;
            last_mouse_y = y;
        } else if (state == GLUT_UP && drag_mode == 2) {
            drag_mode = 0;
        }
    }
}

static void motion(int x, int y) {
    int dx = x - last_mouse_x;
    int dy = y - last_mouse_y;
    last_mouse_x = x;
    last_mouse_y = y;

    if (drag_mode == 1) {
        camera_rotate(static_cast<double>(dx), static_cast<double>(dy));
    } else if (drag_mode == 2) {
        camera_pan(static_cast<double>(dx), static_cast<double>(dy));
    }

    if (x >= 0 && x < sim_width) {
        track_mouse(x, y);
    }
}

static void speed_up() {
    time_multiplier *= 1.5;
    if (time_multiplier > MAX_SPEED_MULT) {
        time_multiplier = MAX_SPEED_MULT;
    }
    set_status("Speed: %.2fx", time_multiplier);
}

static void speed_down() {
    time_multiplier /= 1.5;
    if (time_multiplier < MIN_SPEED_MULT) {
        time_multiplier = MIN_SPEED_MULT;
    }
    set_status("Speed: %.2fx", time_multiplier);
}

static void delete_selected() {
    if (selected_body_index < 0 || selected_body_index >= MAX_OBJECTS ||
        !universe_pool[selected_body_index].active) {
        set_status("No body selected to delete");
        return;
    }

    char deleted_name[NAME_STR_LEN];
    copy_name(deleted_name, universe_pool[selected_body_index].name);
    int index = selected_body_index;
    remove_body(index);
    selected_body_index = -1;
    set_status("Deleted %s", deleted_name);
}

static void spawn_body_at_cursor() {
    const TypeDefaults &defaults = type_defaults(spawn_type);
    double mass = defaults.mass * (static_cast<double>(spawn_mass_percent) / 100.0);
    double radius = defaults.radius * (static_cast<double>(spawn_radius_percent) / 100.0);

    double visual_x = clamp_double(spawn_x, -1.20, 1.20);
    double visual_z = clamp_double(spawn_z, -1.20, 1.20);
    double visual_y = 0.0;

    // Push the spawn point out of any body using its visible position.
    for (int pass = 0; pass < 8; ++pass) {
        bool moved = false;
        for (int i = 0; i < MAX_OBJECTS; ++i) {
            if (!universe_pool[i].active) continue;
            const CelestialBody &b = universe_pool[i];
            double position[3];
            body_visual_position(i, position);

            double dx = visual_x - position[0];
            double dy = visual_y - position[1];
            double dz = visual_z - position[2];
            double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
            double minimum = b.radius + radius + 0.012;
            if (distance < minimum) {
                double horizontal = std::sqrt(dx * dx + dz * dz);
                if (horizontal < 1.0e-9) {
                    visual_x = position[0] + minimum;
                    visual_z = position[2];
                } else {
                    visual_x = position[0] + (dx / horizontal) * minimum;
                    visual_z = position[2] + (dz / horizontal) * minimum;
                }
                moved = true;
            }
        }
        if (!moved) break;
    }

    visual_x = clamp_double(visual_x, -1.20, 1.20);
    visual_z = clamp_double(visual_z, -1.20, 1.20);

    // A moon looks for a nearby planet first; every other type orbits the
    // most massive body (normally the Sun).
    int parent_planet = -1;
    if (spawn_type == BodyType::Moon) {
        double best_distance = 0.12;
        for (int i = 0; i < MAX_OBJECTS; ++i) {
            if (!universe_pool[i].active) continue;
            const CelestialBody &candidate = universe_pool[i];
            if (candidate.type != BodyType::Planet) continue;
            double position[3];
            body_visual_position(i, position);
            double dx = visual_x - position[0];
            double dy = visual_y - position[1];
            double dz = visual_z - position[2];
            double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (distance < best_distance) {
                best_distance = distance;
                parent_planet = i;
            }
        }
    }

    int primary = parent_planet;
    if (primary < 0) {
        primary = heaviest_body();
    }

    double body_x = visual_x;
    double body_y = visual_y;
    double body_z = visual_z;
    double vx = 0.0;
    double vy = 0.0;
    double vz = 0.0;

    int moon_parent = -1;
    double moon_physical_radius = 0.0;
    double moon_visual_radius = 0.0;
    double moon_visual_scale = 1.0;
    double moon_inclination = 0.0;
    double moon_visual_y = visual_y;

    if (spawn_type == BodyType::Moon && parent_planet >= 0) {
        const CelestialBody &parent = universe_pool[parent_planet];
        int sun = find_body_index_by_name("Sun");
        double sun_mass = (sun >= 0 ? universe_pool[sun].mass : 1.0);
        if (sun_mass <= 0.0) sun_mass = 1.0;

        double parent_orbit = 0.0;
        if (sun >= 0) {
            double sx = parent.x - universe_pool[sun].x;
            double sy = parent.y - universe_pool[sun].y;
            double sz = parent.z - universe_pool[sun].z;
            parent_orbit = std::sqrt(sx * sx + sy * sy + sz * sz);
        } else {
            parent_orbit = std::sqrt(parent.x * parent.x +
                                     parent.y * parent.y +
                                     parent.z * parent.z);
        }
        if (parent_orbit < 1.0e-6) parent_orbit = 0.10;

        double hill = parent_orbit * std::cbrt(parent.mass / (3.0 * sun_mass));
        if (hill < 1.0e-6) hill = parent_orbit * 0.01;

        double dx = visual_x - parent.x;
        double dz = visual_z - parent.z;
        double visual_radius = std::sqrt(dx * dx + dz * dz);
        if (visual_radius < 1.0e-6) {
            visual_radius = parent.radius + radius + 0.012;
            dx = visual_radius;
            dz = 0.0;
        }

        double parent_radius = (parent.radius > 0.0 ? parent.radius : 1.0);
        double visual_factor = visual_radius / parent_radius;
        double physical_fraction = 0.15 +
            0.25 * (visual_factor - 1.5) / 1.3;
        physical_fraction = clamp_double(physical_fraction, 0.20, 0.40);
        double physical_radius = hill * physical_fraction;
        if (physical_radius < 1.0e-6) physical_radius = 1.0e-4;

        double angle = std::atan2(dz, dx);
        double visual_scale = visual_radius / physical_radius;

        body_x = parent.x + physical_radius * std::cos(angle);
        body_y = parent.y;
        body_z = parent.z + physical_radius * std::sin(angle);
        moon_visual_y = parent.y;

        double physical_speed = std::sqrt(G_CONSTANT * parent.mass * physical_radius) /
                                (physical_radius + MOON_SOFTENING);
        double direction = (spawn_velocity_mode == 1) ? -1.0 : 1.0;
        if (spawn_velocity_mode == 2) physical_speed = 0.0;

        vx = parent.vx + direction * (-std::sin(angle)) * physical_speed;
        vy = parent.vy;
        vz = parent.vz + direction * (std::cos(angle)) * physical_speed;

        moon_parent = parent_planet;
        moon_physical_radius = physical_radius;
        moon_visual_radius = visual_radius;
        moon_visual_scale = visual_scale;
        moon_inclination = 0.0;
    } else if (spawn_velocity_mode != 2 && primary >= 0) {
        const CelestialBody &p = universe_pool[primary];
        double dx = body_x - p.x;
        double dz = body_z - p.z;
        double distance = std::sqrt(dx * dx + dz * dz);
        if (distance > 1.0e-6 && p.mass > 0.0) {
            double speed = std::sqrt(G_CONSTANT * p.mass * distance) /
                           (distance + SOFTENING);
            double direction = (spawn_velocity_mode == 1) ? -1.0 : 1.0;
            vx = p.vx + direction * (-dz / distance) * speed;
            vy = p.vy;
            vz = p.vz + direction * (dx / distance) * speed;
        }
    }

    int type_index = static_cast<int>(spawn_type);
    spawn_counter[type_index] += 1;
    int serial = spawn_counter[type_index];

    char name[NAME_STR_LEN];
    std::snprintf(name, sizeof(name), "%s %d", type_name(spawn_type), serial);

    int slot = add_body(spawn_type, name,
                        body_x, body_y, body_z,
                        vx, vy, vz,
                        mass, radius);
    if (slot < 0) {
        set_status("Cannot add: pool is full (%d bodies)", MAX_OBJECTS);
        return;
    }

    if (moon_parent >= 0) {
        MoonOrbit &orbit = moon_orbits[slot];
        orbit.parent_index = moon_parent;
        orbit.physical_radius = moon_physical_radius;
        orbit.visual_radius = moon_visual_radius;
        orbit.visual_scale = moon_visual_scale;
        orbit.inclination = moon_inclination;
        orbit.real_orbit_radius_km = 0.0;
        orbit.visual_x = visual_x;
        orbit.visual_y = moon_visual_y;
        orbit.visual_z = visual_z;
        universe_pool[slot].orbital_period = 0.0;
        copy_history(universe_pool[slot].history,
                     "Runtime moon created by the user. It is a true N-body satellite with a compressed visual orbit.");
    }

    selected_body_index = slot;
    set_status("Added %s (mass %.4g, radius %.4g)", name, mass, radius);
}

static void keyboard(unsigned char key, int x, int y) {
    (void)x;
    (void)y;

    switch (key) {
        case 27:
        case 'q':
        case 'Q':
            std::exit(0);
            break;

        case ' ':
        case 'p':
        case 'P':
            paused = !paused;
            set_status(paused ? "Simulation paused" : "Simulation resumed");
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
            spawn_type = static_cast<BodyType>(key - '1');
            set_status("Spawn type: %s", type_name(spawn_type));
            break;

        case 'a':
        case 'A':
            spawn_body_at_cursor();
            break;

        case 'd':
        case 'D':
            delete_selected();
            break;

        case 'r':
        case 'R':
            reset_universe();
            break;

        case 'f':
        case 'F':
            follow_selected = !follow_selected;
            if (follow_selected &&
                (selected_body_index < 0 || selected_body_index >= MAX_OBJECTS ||
                 !universe_pool[selected_body_index].active)) {
                follow_selected = false;
                set_status("Select a body before following it");
            } else {
                set_status(follow_selected ? "Camera following selected body"
                                           : "Camera follow off");
            }
            break;

        case 'v':
        case 'V':
            spawn_velocity_mode = (spawn_velocity_mode + 1) % 3;
            if (spawn_velocity_mode == 0) set_status("Spawn velocity: orbital");
            else if (spawn_velocity_mode == 1) set_status("Spawn velocity: retrograde");
            else set_status("Spawn velocity: zero");
            break;

        case '[':
            spawn_mass_percent = clamp_int(spawn_mass_percent - 25, 25, 400);
            set_status("Spawn mass scale: %.2fx", spawn_mass_percent / 100.0);
            break;

        case ']':
            spawn_mass_percent = clamp_int(spawn_mass_percent + 25, 25, 400);
            set_status("Spawn mass scale: %.2fx", spawn_mass_percent / 100.0);
            break;

        case ',':
            spawn_radius_percent = clamp_int(spawn_radius_percent - 10, 50, 200);
            set_status("Spawn radius scale: %.2fx", spawn_radius_percent / 100.0);
            break;

        case '.':
            spawn_radius_percent = clamp_int(spawn_radius_percent + 10, 50, 200);
            set_status("Spawn radius scale: %.2fx", spawn_radius_percent / 100.0);
            break;

        case 'l':
        case 'L':
            show_labels = !show_labels;
            set_status(show_labels ? "Labels on" : "Labels off");
            break;

        case '0':
            time_multiplier = 1.0;
            set_status("Speed reset to 1.00x");
            break;

        default:
            break;
    }
}

static void special_keys(int key, int x, int y) {
    (void)x;
    (void)y;

    if (key == GLUT_KEY_UP) {
        speed_up();
    } else if (key == GLUT_KEY_DOWN) {
        speed_down();
    } else if (key == GLUT_KEY_HOME) {
        camera_reset();
        set_status("Camera reset");
    } else if (key == GLUT_KEY_PAGE_UP) {
        camera_zoom(1.0);
    } else if (key == GLUT_KEY_PAGE_DOWN) {
        camera_zoom(-1.0);
    }
}

// ============================================================================
//  12. GLUT callbacks
// ============================================================================

static void save_screenshot(const char *path) {
    if (winWidth < 1 || winHeight < 1) return;

    int width = winWidth;
    int height = winHeight;
    unsigned char *pixels = new unsigned char[width * height * 3];

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    std::FILE *file = std::fopen(path, "wb");
    if (file != nullptr) {
        std::fprintf(file, "P6\n%d %d\n255\n", width, height);
        for (int y = height - 1; y >= 0; --y) {
            std::fwrite(pixels + y * width * 3, 1, width * 3, file);
        }
        std::fclose(file);
        std::printf("Screenshot written to %s\n", path);
    } else {
        std::printf("Could not open screenshot file %s\n", path);
    }

    delete[] pixels;
}

static void arm_screenshot(int value) {
    (void)value;
    capture_armed = true;
    glutPostRedisplay();
}

static void display() {
    render_scene();
    draw_hud();

    if (capture_armed && !capture_done) {
        capture_done = true;
        save_screenshot(screenshot_path);
    }

    glutSwapBuffers();

    if (capture_done) {
        std::exit(0);
    }
}

static void reshape(int width, int height) {
    winWidth = (width > 0 ? width : 1);
    winHeight = (height > 0 ? height : 1);
    glViewport(0, 0, winWidth, winHeight);
    update_sim_width();
    glutPostRedisplay();
}

static void timer(int value) {
    (void)value;

    ++frame_count;
    int now = glutGet(GLUT_ELAPSED_TIME);
    if (fps_last_time == 0) {
        fps_last_time = now;
    }
    if (now - fps_last_time >= 500) {
        fps = static_cast<int>(frame_count * 1000.0 /
                               static_cast<double>(now - fps_last_time));
        frame_count = 0;
        fps_last_time = now;
    }

    if (!paused) {
        double dt = BASE_DT * time_multiplier;
        int substeps = compute_substeps(dt);
        for (int s = 0; s < substeps; ++s) {
            step_physics(dt / static_cast<double>(substeps));
        }
        update_moon_visuals();
        update_cosmetic(dt);
    }

    if (status_frames > 0) {
        --status_frames;
    }

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

static void init_gl() {
    glClearColor(0.005f, 0.008f, 0.020f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_POINT_SMOOTH);

    GLfloat ambient_light[] = {0.20f, 0.20f, 0.22f, 1.0f};
    GLfloat diffuse_light[] = {1.00f, 0.97f, 0.90f, 1.0f};
    GLfloat specular_light[] = {0.55f, 0.55f, 0.55f, 1.0f};
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient_light);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse_light);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular_light);

    GLfloat global_ambient[] = {0.10f, 0.10f, 0.12f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);

    sphere_quadric = gluNewQuadric();
    gluQuadricNormals(sphere_quadric, GLU_SMOOTH);
}

// ============================================================================
//  13. Self-test (no window needed)
// ============================================================================

static bool check(bool condition, const char *label) {
    std::printf("[%s] %s\n", condition ? "PASS" : "FAIL", label);
    return condition;
}

static int run_selftest() {
    bool ok = true;

    std::printf("N-Body Space Simulator 3D v3 self-test\n");
    std::printf("sizeof(CelestialBody) = %d bytes\n",
                static_cast<int>(sizeof(CelestialBody)));
    ok = check(sizeof(CelestialBody) >= 700 && sizeof(CelestialBody) <= 1024,
               "CelestialBody fits the pool budget") && ok;

    // Ray–sphere maths used by picking.
    double ray_origin[3] = {0.0, 0.0, -5.0};
    double ray_direction[3] = {0.0, 0.0, 1.0};
    double sphere_center[3] = {0.0, 0.0, 0.0};
    double hit_t = 0.0;
    ok = check(ray_sphere_hit(ray_origin, ray_direction, sphere_center, 1.0, hit_t) &&
               std::fabs(hit_t - 4.0) < 1.0e-6,
               "ray-sphere hit returns the near intersection") && ok;
    double miss_origin[3] = {0.0, 5.0, -5.0};
    ok = check(!ray_sphere_hit(miss_origin, ray_direction, sphere_center, 1.0, hit_t),
               "ray-sphere miss is rejected") && ok;

    reset_universe();
    print_bodies();
    ok = check(active_object_count == 22, "initial layout has 22 active bodies") && ok;
    ok = check(find_body_index_by_name("Sun") >= 0, "Sun exists") && ok;
    ok = check(find_body_index_by_name("Earth") >= 0, "Earth exists") && ok;

    int sun = find_body_index_by_name("Sun");
    int earth = find_body_index_by_name("Earth");

    int planet_indices[8];
    double planet_initial_radii[8];
    int planet_total = 0;
    for (int i = 0; i < 8; ++i) {
        int index = find_body_index_by_name(PLANET_SEEDS[i].name);
        if (index < 0 || sun < 0) continue;
        double dx = universe_pool[index].x - universe_pool[sun].x;
        double dy = universe_pool[index].y - universe_pool[sun].y;
        double dz = universe_pool[index].z - universe_pool[sun].z;
        planet_indices[planet_total] = index;
        planet_initial_radii[planet_total] =
            std::sqrt(dx * dx + dy * dy + dz * dz);
        ++planet_total;
    }

    double initial_radius = 0.0;
    if (sun >= 0 && earth >= 0) {
        double dx = universe_pool[earth].x - universe_pool[sun].x;
        double dy = universe_pool[earth].y - universe_pool[sun].y;
        double dz = universe_pool[earth].z - universe_pool[sun].z;
        initial_radius = std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    // Run 20,000 small physics steps (about 12.5 simulation time units).
    for (int i = 0; i < 20000; ++i) {
        step_physics(BASE_DT / 4.0);
    }
    update_moon_visuals();

    double final_radius = 0.0;
    if (sun >= 0 && earth >= 0 &&
        universe_pool[sun].active && universe_pool[earth].active) {
        double dx = universe_pool[earth].x - universe_pool[sun].x;
        double dy = universe_pool[earth].y - universe_pool[sun].y;
        double dz = universe_pool[earth].z - universe_pool[sun].z;
        final_radius = std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    if (initial_radius > 0.0) {
        std::printf("Earth radius: initial %.4f, final %.4f (drift %.2f%%)\n",
                    initial_radius, final_radius,
                    100.0 * std::fabs(final_radius - initial_radius) / initial_radius);
    }
    ok = check(std::isfinite(final_radius), "Earth position stays finite") && ok;
    ok = check(initial_radius > 0.0 &&
               std::fabs(final_radius - initial_radius) / initial_radius < 0.05,
               "Earth orbit radius drift is under 5 percent") && ok;

    bool all_planets_stable = (planet_total == 8 && sun >= 0);
    for (int i = 0; i < planet_total; ++i) {
        int index = planet_indices[i];
        if (!universe_pool[index].active || !universe_pool[sun].active) {
            all_planets_stable = false;
            break;
        }
        double dx = universe_pool[index].x - universe_pool[sun].x;
        double dy = universe_pool[index].y - universe_pool[sun].y;
        double dz = universe_pool[index].z - universe_pool[sun].z;
        double radius = std::sqrt(dx * dx + dy * dy + dz * dz);
        double drift = std::fabs(radius - planet_initial_radii[i]) /
                       planet_initial_radii[i];
        std::printf("  %-8s orbit drift %.2f%%\n",
                    universe_pool[index].name, 100.0 * drift);
        if (drift >= 0.05) {
            all_planets_stable = false;
            break;
        }
    }
    ok = check(all_planets_stable,
               "all eight planets stay within 5% of their initial orbits") && ok;

    int moon_index = find_body_index_by_name("Moon");
    if (moon_index >= 0 && earth >= 0) {
        double dx = universe_pool[moon_index].x - universe_pool[earth].x;
        double dy = universe_pool[moon_index].y - universe_pool[earth].y;
        double dz = universe_pool[moon_index].z - universe_pool[earth].z;
        double moon_distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        double expected_distance = moon_orbits[moon_index].physical_radius;
        if (expected_distance > 0.0) {
            std::printf("Moon physical radius: initial %.6f, final %.6f (drift %.2f%%)\n",
                        expected_distance, moon_distance,
                        100.0 * std::fabs(moon_distance - expected_distance) / expected_distance);
        }
        ok = check(moon_orbits[moon_index].parent_index == earth,
                   "Moon is attached to Earth in the N-body model") && ok;
        ok = check(expected_distance > 0.0 &&
                   std::fabs(moon_distance - expected_distance) / expected_distance < 0.20,
                   "Moon stays in its Hill-safe physical orbit") && ok;

        double visual_dx = moon_orbits[moon_index].visual_x - universe_pool[earth].x;
        double visual_dy = moon_orbits[moon_index].visual_y - universe_pool[earth].y;
        double visual_dz = moon_orbits[moon_index].visual_z - universe_pool[earth].z;
        double visual_distance = std::sqrt(visual_dx * visual_dx +
                                           visual_dy * visual_dy +
                                           visual_dz * visual_dz);
        double expected_visual = moon_distance * moon_orbits[moon_index].visual_scale;
        ok = check(std::fabs(visual_distance - expected_visual) < 1.0e-6,
                   "Moon visual position follows its physical orbit") && ok;
    }

    bool all_moons_bound = true;
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe_pool[i].active) continue;
        const MoonOrbit &orbit = moon_orbits[i];
        if (orbit.parent_index < 0) continue;
        if (orbit.parent_index >= MAX_OBJECTS ||
            !universe_pool[orbit.parent_index].active) {
            all_moons_bound = false;
            break;
        }
        double dx = universe_pool[i].x - universe_pool[orbit.parent_index].x;
        double dy = universe_pool[i].y - universe_pool[orbit.parent_index].y;
        double dz = universe_pool[i].z - universe_pool[orbit.parent_index].z;
        double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (orbit.physical_radius <= 0.0 ||
            distance < orbit.physical_radius * 0.20 ||
            distance > orbit.physical_radius * 2.00) {
            all_moons_bound = false;
            break;
        }
    }
    ok = check(all_moons_bound, "all moons remain bound to their parents") && ok;
    ok = check(active_object_count == 22, "stable layout has no accidental collisions") && ok;

    // Removing a planet removes its parented moons.
    reset_universe();
    int earth_remove = find_body_index_by_name("Earth");
    int moon_remove = find_body_index_by_name("Moon");
    if (earth_remove >= 0 && moon_remove >= 0) {
        ok = check(remove_body(earth_remove), "Earth can be removed") && ok;
        ok = check(universe_pool[moon_remove].active,
                   "Earth's Moon detaches instead of disappearing") && ok;
        ok = check(moon_orbits[moon_remove].parent_index == -1,
                   "detached Moon is unparented") && ok;
        ok = check(active_object_count == 21,
                   "planet removal leaves its moon in the simulation") && ok;
    }

    // Add / remove / slot reuse.
    reset_universe();
    int a = add_body(BodyType::Asteroid, "Test A",
                     0.90, 0.0, 0.90, 0.0, 0.0, 0.0, 1.0e-9, 0.004);
    int b = add_body(BodyType::Asteroid, "Test B",
                     0.92, 0.0, 0.90, 0.0, 0.0, 0.0, 1.0e-9, 0.004);
    int c = add_body(BodyType::Asteroid, "Test C",
                     0.94, 0.0, 0.90, 0.0, 0.0, 0.0, 1.0e-9, 0.004);
    ok = check(a >= 0 && b >= 0 && c >= 0, "three bodies can be added") && ok;
    ok = check(active_object_count == 25, "active count rises to 25") && ok;
    ok = check(remove_body(b), "middle body can be removed") && ok;
    int reused = add_body(BodyType::Asteroid, "Test D",
                          0.96, 0.0, 0.90, 0.0, 0.0, 0.0, 1.0e-9, 0.004);
    ok = check(reused == b, "removed slot is reused") && ok;
    ok = check(!remove_body(999), "out-of-range removal is rejected") && ok;

    // A spawned moon should orbit a nearby planet, not the Sun.
    reset_universe();
    int earth_index = find_body_index_by_name("Earth");
    if (earth_index >= 0) {
        double earth_vx = universe_pool[earth_index].vx;
        double earth_vy = universe_pool[earth_index].vy;
        double earth_vz = universe_pool[earth_index].vz;

        spawn_type = BodyType::Moon;
        spawn_mass_percent = 100;
        spawn_radius_percent = 100;
        spawn_velocity_mode = 0;
        spawn_x = universe_pool[earth_index].x;
        spawn_z = universe_pool[earth_index].z;

        int before_spawn = active_object_count;
        spawn_body_at_cursor();

        ok = check(active_object_count == before_spawn + 1,
                   "moon can be spawned at the cursor") && ok;
        if (selected_body_index >= 0 &&
            universe_pool[selected_body_index].active) {
            const CelestialBody &moon = universe_pool[selected_body_index];
            double relative_vx = moon.vx - earth_vx;
            double relative_vy = moon.vy - earth_vy;
            double relative_vz = moon.vz - earth_vz;
            double relative_speed = std::sqrt(relative_vx * relative_vx +
                                              relative_vy * relative_vy +
                                              relative_vz * relative_vz);
            ok = check(moon.type == BodyType::Moon,
                       "spawned body is a moon") && ok;
            ok = check(relative_speed > 0.0 && relative_speed < 0.10,
                       "spawned moon has a physical orbital velocity") && ok;
            ok = check(moon_orbits[selected_body_index].parent_index == earth_index,
                       "spawned moon is parented to Earth") && ok;
        } else {
            ok = check(false, "spawned moon is selected") && ok;
        }
    }

    // Momentum-conserving 3D merge.
    reset_universe();
    int m1 = add_body(BodyType::Planet, "Merge A",
                      0.90, 0.0, 0.90, 1.0, 0.0, -0.5, 2.0, 0.02);
    int m2 = add_body(BodyType::Asteroid, "Merge B",
                      0.92, 0.0, 0.90, -2.0, 1.0, 0.5, 1.0, 0.01);
    if (m1 >= 0 && m2 >= 0) {
        double expected_mass = universe_pool[m1].mass + universe_pool[m2].mass;
        double expected_vx = (universe_pool[m1].mass * universe_pool[m1].vx +
                              universe_pool[m2].mass * universe_pool[m2].vx) / expected_mass;
        double expected_vy = (universe_pool[m1].mass * universe_pool[m1].vy +
                              universe_pool[m2].mass * universe_pool[m2].vy) / expected_mass;
        double expected_vz = (universe_pool[m1].mass * universe_pool[m1].vz +
                              universe_pool[m2].mass * universe_pool[m2].vz) / expected_mass;
        double expected_volume = universe_pool[m1].volume_ratio +
                                 universe_pool[m2].volume_ratio;

        merge_bodies(m1, m2);

        ok = check(std::fabs(universe_pool[m1].mass - expected_mass) < 1.0e-9,
                   "merge conserves mass") && ok;
        ok = check(std::fabs(universe_pool[m1].vx - expected_vx) < 1.0e-9 &&
                   std::fabs(universe_pool[m1].vy - expected_vy) < 1.0e-9 &&
                   std::fabs(universe_pool[m1].vz - expected_vz) < 1.0e-9,
                   "merge conserves 3D momentum") && ok;
        ok = check(std::fabs(universe_pool[m1].volume_ratio - expected_volume) < 1.0e-9,
                   "merge adds volume ratios") && ok;
        ok = check(!universe_pool[m2].active, "absorbed body is inactive") && ok;
    } else {
        ok = check(false, "merge test bodies could be created") && ok;
    }

    // Pool-full behaviour.
    reset_universe();
    int fill = 0;
    while (active_object_count < MAX_OBJECTS) {
        char fill_name[NAME_STR_LEN];
        ++fill;
        std::snprintf(fill_name, sizeof(fill_name), "Fill %d", fill);
        if (add_body(BodyType::Asteroid, fill_name,
                     0.90, 0.0, 0.90, 0.0, 0.0, 0.0,
                     1.0e-9, 0.004) < 0) {
            break;
        }
    }
    ok = check(active_object_count == MAX_OBJECTS, "pool fills to 256 bodies") && ok;
    ok = check(add_body(BodyType::Asteroid, "Overflow",
                        0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                        1.0e-9, 0.004) < 0,
               "adding to a full pool is rejected") && ok;


    reset_universe();

    std::printf("%s\n", ok ? "SELF-TEST PASSED" : "SELF-TEST FAILED");
    return ok ? 0 : 1;
}

// ============================================================================
//  14. Entry point
// ============================================================================

int main(int argc, char **argv) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--selftest") == 0) {
            return run_selftest();
        }
    }

    bool screenshot_mode = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--screenshot") == 0) {
            screenshot_mode = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                copy_path(screenshot_path, sizeof(screenshot_path), argv[i + 1]);
                ++i;
            }
        } else if (std::strcmp(argv[i], "--select") == 0) {
            if (i + 1 < argc) {
                copy_name(initial_selection, argv[i + 1]);
                ++i;
            }
        } else if (std::strcmp(argv[i], "--camera") == 0) {
            if (i + 3 < argc) {
                cam_azimuth = std::atof(argv[i + 1]);
                cam_elevation = clamp_double(std::atof(argv[i + 2]), 5.0, 85.0);
                cam_distance = clamp_double(std::atof(argv[i + 3]),
                                            CAMERA_MIN_DISTANCE,
                                            CAMERA_MAX_DISTANCE);
                i += 3;
            }
        }
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(winWidth, winHeight);
    glutCreateWindow("N-Body Space Simulator 3D - v3");

    init_gl();
    init_cosmetic();
    reset_universe();

    if (initial_selection[0] != '\0') {
        int selected = find_body_index_by_name(initial_selection);
        if (selected >= 0) {
            selected_body_index = selected;
        }
    }

    fps_last_time = glutGet(GLUT_ELAPSED_TIME);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special_keys);
    glutMouseFunc(mouse);
    glutPassiveMotionFunc(track_mouse);
    glutMotionFunc(motion);
    glutTimerFunc(16, timer, 0);

    if (screenshot_mode) {
        glutTimerFunc(700, arm_screenshot, 0);
    }

    glutMainLoop();
    return 0;
}
