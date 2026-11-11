#include "render/camera.h"

#include "core/config.h"
#include "core/math_utils.h"
#include "core/state.h"
#include "core/universe.h"
#include "physics/moon_system.h"

#include <algorithm>
#include <cmath>

namespace spacesim {

OrbitCamera::OrbitCamera()
    : azimuth_(CAMERA_DEFAULT_AZIMUTH),
      elevation_(CAMERA_DEFAULT_ELEVATION),
      distance_(CAMERA_DEFAULT_DISTANCE),
      target_x_(0.0), target_y_(0.0), target_z_(0.0) {}

void OrbitCamera::reset(SelectionState &selection) {
    azimuth_ = CAMERA_DEFAULT_AZIMUTH;
    elevation_ = CAMERA_DEFAULT_ELEVATION;
    distance_ = CAMERA_DEFAULT_DISTANCE;
    target_x_ = 0.0;
    target_y_ = 0.0;
    target_z_ = 0.0;
    follow_target_ = Vec3(0.0, 0.0, 0.0);
    follow_initialized_ = false;
    selection.follow_selected = false;
}

void OrbitCamera::update(const Universe &universe,
                         const SelectionState &selection, double real_dt) {
    if (!selection.follow_selected ||
        selection.selected_body_index < 0 ||
        selection.selected_body_index >= MAX_OBJECTS ||
        !universe.isActive(selection.selected_body_index)) {
        follow_initialized_ = false;
        return;
    }

    double position[3];
    MoonSystem::visualPosition(universe, selection.selected_body_index, position);
    Vec3 target_position(position[0], position[1], position[2]);

    if (!follow_initialized_) {
        // Start from the current pan target and glide to the body.
        follow_target_ = Vec3(target_x_, target_y_, target_z_);
        follow_initialized_ = true;
    }

    double clamped_dt = clamp_double(real_dt, 0.0, 0.08);
    double blend = 1.0 - std::exp(-clamped_dt * 10.0);
    follow_target_ = follow_target_ +
                     (target_position - follow_target_) * blend;
}

void OrbitCamera::syncTarget(const Universe &universe,
                             const SelectionState &selection) {
    if (!selection.follow_selected ||
        selection.selected_body_index < 0 ||
        selection.selected_body_index >= MAX_OBJECTS ||
        !universe.isActive(selection.selected_body_index)) {
        return;
    }
    double position[3];
    MoonSystem::visualPosition(universe, selection.selected_body_index, position);
    target_x_ = position[0];
    target_y_ = position[1];
    target_z_ = position[2];
    follow_target_ = Vec3(position[0], position[1], position[2]);
    follow_initialized_ = true;
}

void OrbitCamera::rotate(double dx, double dy) {
    azimuth_ += dx * 0.35;
    elevation_ += dy * 0.35;
    elevation_ = clamp_double(elevation_, 5.0, 85.0);
}

void OrbitCamera::zoom(double steps) {
    distance_ *= std::pow(1.12, -steps);
    distance_ = clamp_double(distance_, CAMERA_MIN_DISTANCE, CAMERA_MAX_DISTANCE);
}

void OrbitCamera::setView(double azimuth, double elevation, double distance) {
    azimuth_ = azimuth;
    elevation_ = clamp_double(elevation, 5.0, 85.0);
    distance_ = clamp_double(distance, CAMERA_MIN_DISTANCE, CAMERA_MAX_DISTANCE);
}

void OrbitCamera::pan(double dx, double dy, SelectionState &selection) {
    double az = azimuth_ * PI / 180.0;
    double scale = distance_ * 0.0016;
    double right_x = std::sin(az);
    double right_z = -std::cos(az);

    target_x_ -= right_x * dx * scale;
    target_z_ -= right_z * dx * scale;
    target_y_ += dy * scale;
    selection.follow_selected = false;
}

Vec3 OrbitCamera::target_impl(const Universe &universe,
                              const SelectionState &selection) const {
    double tx = target_x_;
    double ty = target_y_;
    double tz = target_z_;
    if (selection.follow_selected &&
        selection.selected_body_index >= 0 &&
        selection.selected_body_index < MAX_OBJECTS &&
        universe.isActive(selection.selected_body_index)) {
        if (follow_initialized_) {
            return follow_target_;
        }
        double position[3];
        MoonSystem::visualPosition(universe, selection.selected_body_index, position);
        tx = position[0];
        ty = position[1];
        tz = position[2];
    }
    return Vec3(tx, ty, tz);
}

Vec3 OrbitCamera::target(const Universe &universe,
                         const SelectionState &selection) const {
    return target_impl(universe, selection);
}

double OrbitCamera::effective_distance(const Universe &universe,
                                       const SelectionState &selection) const {
    double distance = distance_;
    if (selection.follow_selected &&
        selection.selected_body_index >= 0 &&
        selection.selected_body_index < MAX_OBJECTS &&
        universe.isActive(selection.selected_body_index)) {
        const CelestialBody &body = universe.body(selection.selected_body_index);
        double scale = (body.type == BodyType::BlackHole) ? 7.0 : 2.6;
        double radius = (body.type == BodyType::BlackHole && body.horizon_radius > 0.0)
            ? body.horizon_radius : body.radius;
        distance = std::max(distance, radius * scale + 0.006);
    }
    return distance;
}

Vec3 OrbitCamera::eye(const Universe &universe,
                      const SelectionState &selection) const {
    Vec3 center = target_impl(universe, selection);
    double distance = effective_distance(universe, selection);
    double az = azimuth_ * PI / 180.0;
    double el = elevation_ * PI / 180.0;
    double horizontal = distance * std::cos(el);
    return Vec3(center.x + horizontal * std::cos(az),
                center.y + distance * std::sin(el),
                center.z + horizontal * std::sin(az));
}

Mat4 OrbitCamera::view(const Universe &universe,
                       const SelectionState &selection) const {
    return Mat4::look_at(eye(universe, selection),
                         target_impl(universe, selection),
                         Vec3(0.0, 1.0, 0.0));
}

Mat4 OrbitCamera::projection(double aspect) const {
    return Mat4::perspective(CAMERA_FOV, aspect, CAMERA_NEAR, CAMERA_FAR);
}

bool OrbitCamera::project(const Universe &universe,
                          const SelectionState &selection,
                          const double world[3], const UiState &ui,
                          double &screen_x, double &screen_y,
                          double &depth) const {
    Mat4 vp = projection(static_cast<double>(ui.sim_width) / ui.win_height) *
              view(universe, selection);
    double w = 0.0;
    Vec3 clip = transform_point(vp, Vec3(world[0], world[1], world[2]), &w);
    if (w <= 1.0e-9) return false;
    double ndc_x = clip.x / w;
    double ndc_y = clip.y / w;
    double ndc_z = clip.z / w;
    if (ndc_z < -1.0 || ndc_z > 1.0) return false;
    screen_x = (ndc_x * 0.5 + 0.5) * ui.sim_width;
    screen_y = (ndc_y * 0.5 + 0.5) * ui.win_height;
    depth = ndc_z;
    return true;
}

void OrbitCamera::screen_ray(const Universe &universe,
                             const SelectionState &selection,
                             const UiState &ui, int screen_x, int screen_y,
                             double origin[3], double direction[3]) const {
    Vec3 eye_pos = eye(universe, selection);
    Vec3 center = target_impl(universe, selection);
    Vec3 forward = normalize(center - eye_pos);
    Vec3 right = normalize(cross(forward, Vec3(0.0, 1.0, 0.0)));
    Vec3 up = cross(right, forward);
    double aspect = static_cast<double>(ui.sim_width) /
                    static_cast<double>(ui.win_height > 0 ? ui.win_height : 1);
    double ndc_x = 2.0 * static_cast<double>(screen_x) /
                   static_cast<double>(ui.sim_width > 0 ? ui.sim_width : 1) - 1.0;
    double ndc_y = 1.0 - 2.0 * static_cast<double>(screen_y) /
                   static_cast<double>(ui.win_height > 0 ? ui.win_height : 1);
    double tan_half = std::tan(CAMERA_FOV * PI / 360.0);
    Vec3 dir = normalize(forward + right * (ndc_x * tan_half * aspect) +
                         up * (ndc_y * tan_half));
    origin[0] = eye_pos.x;
    origin[1] = eye_pos.y;
    origin[2] = eye_pos.z;
    direction[0] = dir.x;
    direction[1] = dir.y;
    direction[2] = dir.z;
}

double OrbitCamera::azimuth() const { return azimuth_; }
double OrbitCamera::elevation() const { return elevation_; }
double OrbitCamera::distance() const { return distance_; }

} // namespace spacesim
