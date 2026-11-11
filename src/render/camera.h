#ifndef SPACESIM_RENDER_CAMERA_H
#define SPACESIM_RENDER_CAMERA_H

#include "core/math3d.h"

namespace spacesim {

class Universe;
struct SelectionState;
struct UiState;

class OrbitCamera {
public:
    OrbitCamera();

    void reset(SelectionState &selection);
    void update(const Universe &universe, const SelectionState &selection,
                double real_dt);
    void rotate(double dx, double dy);
    void zoom(double steps);
    void pan(double dx, double dy, SelectionState &selection);
    void syncTarget(const Universe &universe, const SelectionState &selection);
    void setView(double azimuth, double elevation, double distance);

    Vec3 eye(const Universe &universe, const SelectionState &selection) const;
    Vec3 target(const Universe &universe, const SelectionState &selection) const;
    Mat4 view(const Universe &universe, const SelectionState &selection) const;
    Mat4 projection(double aspect) const;

    bool project(const Universe &universe, const SelectionState &selection,
                 const double world[3], const UiState &ui,
                 double &screen_x, double &screen_y, double &depth) const;

    void screen_ray(const Universe &universe, const SelectionState &selection,
                    const UiState &ui, int screen_x, int screen_y,
                    double origin[3], double direction[3]) const;

    double azimuth() const;
    double elevation() const;
    double distance() const;

private:
    Vec3 target_impl(const Universe &universe, const SelectionState &selection) const;
    double effective_distance(const Universe &universe,
                              const SelectionState &selection) const;

    double azimuth_;
    double elevation_;
    double distance_;
    double target_x_;
    double target_y_;
    double target_z_;
    Vec3 follow_target_;
    bool follow_initialized_ = false;
};

} // namespace spacesim

#endif
