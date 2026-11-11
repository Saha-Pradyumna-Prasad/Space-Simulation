#ifndef SPACESIM_INPUT_PICKING_H
#define SPACESIM_INPUT_PICKING_H

namespace spacesim {

struct UiState;
struct SelectionState;
class Universe;
class OrbitCamera;

void screen_ray(const OrbitCamera &camera, const Universe &universe,
                const SelectionState &selection, const UiState &ui,
                int screen_x, int screen_y,
                double origin[3], double direction[3]);
int pick_body(const Universe &universe, const OrbitCamera &camera,
              const SelectionState &selection, const UiState &ui,
              int screen_x, int screen_y);
bool ray_plane_y0(const double origin[3], const double direction[3],
                  double &x, double &z);

} // namespace spacesim

#endif
