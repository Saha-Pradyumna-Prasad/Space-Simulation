#ifndef SPACESIM_CORE_MATH3D_H
#define SPACESIM_CORE_MATH3D_H

#include <cmath>

namespace spacesim {

struct Vec3 {
    double x = 0.0, y = 0.0, z = 0.0;
    Vec3() = default;
    Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
};

inline Vec3 operator+(const Vec3 &a, const Vec3 &b) {
    return Vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}
inline Vec3 operator-(const Vec3 &a, const Vec3 &b) {
    return Vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}
inline Vec3 operator*(const Vec3 &a, double s) {
    return Vec3(a.x * s, a.y * s, a.z * s);
}
inline Vec3 operator/(const Vec3 &a, double s) {
    return Vec3(a.x / s, a.y / s, a.z / s);
}
inline double dot(const Vec3 &a, const Vec3 &b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
inline Vec3 cross(const Vec3 &a, const Vec3 &b) {
    return Vec3(a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x);
}
inline double length(const Vec3 &a) {
    return std::sqrt(dot(a, a));
}
inline Vec3 normalize(const Vec3 &a) {
    double l = length(a);
    return (l > 1.0e-12) ? (a / l) : Vec3(1.0, 0.0, 0.0);
}

struct Mat4 {
    float m[16] = {1, 0, 0, 0,
                   0, 1, 0, 0,
                   0, 0, 1, 0,
                   0, 0, 0, 1};

    static Mat4 identity() {
        return Mat4();
    }

    static Mat4 translation(double x, double y, double z) {
        Mat4 r;
        r.m[12] = static_cast<float>(x);
        r.m[13] = static_cast<float>(y);
        r.m[14] = static_cast<float>(z);
        return r;
    }

    static Mat4 scale(double x, double y, double z) {
        Mat4 r;
        r.m[0] = static_cast<float>(x);
        r.m[5] = static_cast<float>(y);
        r.m[10] = static_cast<float>(z);
        return r;
    }

    static Mat4 rotation_x(double degrees) {
        double radians = degrees * 3.14159265358979323846 / 180.0;
        double c = std::cos(radians);
        double s = std::sin(radians);
        Mat4 r;
        r.m[5] = static_cast<float>(c);
        r.m[6] = static_cast<float>(s);
        r.m[9] = static_cast<float>(-s);
        r.m[10] = static_cast<float>(c);
        return r;
    }

    static Mat4 rotation_y(double degrees) {
        double radians = degrees * 3.14159265358979323846 / 180.0;
        double c = std::cos(radians);
        double s = std::sin(radians);
        Mat4 r;
        r.m[0] = static_cast<float>(c);
        r.m[2] = static_cast<float>(-s);
        r.m[8] = static_cast<float>(s);
        r.m[10] = static_cast<float>(c);
        return r;
    }

    static Mat4 perspective(double fov_degrees, double aspect,
                            double near_plane, double far_plane) {
        Mat4 r;
        for (int i = 0; i < 16; ++i) r.m[i] = 0.0f;
        double radians = fov_degrees * 3.14159265358979323846 / 180.0;
        double f = 1.0 / std::tan(radians * 0.5);
        r.m[0] = static_cast<float>(f / aspect);
        r.m[5] = static_cast<float>(f);
        r.m[10] = static_cast<float>((far_plane + near_plane) /
                                     (near_plane - far_plane));
        r.m[11] = -1.0f;
        r.m[14] = static_cast<float>((2.0 * far_plane * near_plane) /
                                     (near_plane - far_plane));
        return r;
    }

    static Mat4 look_at(const Vec3 &eye, const Vec3 &target, const Vec3 &up) {
        Vec3 f = normalize(target - eye);
        Vec3 s = normalize(cross(f, up));
        Vec3 u = cross(s, f);
        Mat4 r;
        r.m[0] = static_cast<float>(s.x);
        r.m[4] = static_cast<float>(s.y);
        r.m[8] = static_cast<float>(s.z);
        r.m[1] = static_cast<float>(u.x);
        r.m[5] = static_cast<float>(u.y);
        r.m[9] = static_cast<float>(u.z);
        r.m[2] = static_cast<float>(-f.x);
        r.m[6] = static_cast<float>(-f.y);
        r.m[10] = static_cast<float>(-f.z);
        r.m[12] = static_cast<float>(-dot(s, eye));
        r.m[13] = static_cast<float>(-dot(u, eye));
        r.m[14] = static_cast<float>(dot(f, eye));
        return r;
    }
};

inline Mat4 operator*(const Mat4 &a, const Mat4 &b) {
    Mat4 r;
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                sum += a.m[k * 4 + row] * b.m[col * 4 + k];
            }
            r.m[col * 4 + row] = sum;
        }
    }
    return r;
}

inline Vec3 transform_point(const Mat4 &matrix, const Vec3 &point, double *w_out = nullptr) {
    double x = matrix.m[0] * point.x + matrix.m[4] * point.y +
               matrix.m[8] * point.z + matrix.m[12];
    double y = matrix.m[1] * point.x + matrix.m[5] * point.y +
               matrix.m[9] * point.z + matrix.m[13];
    double z = matrix.m[2] * point.x + matrix.m[6] * point.y +
               matrix.m[10] * point.z + matrix.m[14];
    double w = matrix.m[3] * point.x + matrix.m[7] * point.y +
               matrix.m[11] * point.z + matrix.m[15];
    if (w_out != nullptr) *w_out = w;
    return Vec3(x, y, z);
}

} // namespace spacesim

#endif
