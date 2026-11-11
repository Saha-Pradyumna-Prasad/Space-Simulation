#ifndef SPACESIM_CORE_MATH_UTILS_H
#define SPACESIM_CORE_MATH_UTILS_H

#include "core/body.h"

namespace spacesim {

double clamp_double(double value, double low, double high);
int clamp_int(int value, int low, int high);

double volume_from_radius(double radius);
double earth_volume_from_mass_density(double mass, double density_kg_m3);
void sync_body_volume(CelestialBody &body);
double random_unit();

bool ray_sphere_hit(const double origin[3], const double direction[3],
                    const double center[3], double radius, double &hit_t);
bool body_is_finite(const CelestialBody &body);

void copy_name(char *dst, const char *src);
void copy_history(char *dst, const char *src);
void copy_path(char *dst, int size, const char *src);

} // namespace spacesim

#endif
