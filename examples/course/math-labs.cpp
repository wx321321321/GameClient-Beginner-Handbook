#ifdef NDEBUG
#error "Teaching labs require assertions; compile without NDEBUG."
#endif

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

constexpr double pi = 3.14159265358979323846;
bool Near(double a, double b) { return std::abs(a - b) < 1e-9; }
struct Vec3 { double x, y, z; };
Vec3 operator+(Vec3 a, Vec3 b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
Vec3 operator*(Vec3 v, double s) { return {v.x*s, v.y*s, v.z*s}; }
Vec3 Cross(Vec3 a, Vec3 b) {
    return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
}
struct Quat { double x, y, z, w; };
Quat Normalize(Quat q) {
    const double length = std::sqrt(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
    assert(length > 0.0);
    return {q.x/length, q.y/length, q.z/length, q.w/length};
}
Vec3 Rotate(Quat q, Vec3 v) { // q must be unit length
    const Vec3 xyz{q.x, q.y, q.z};
    const Vec3 t = Cross(xyz, v) * 2.0;
    return v + t * q.w + Cross(xyz, t);
}
Quat Nlerp(Quat a, Quat b, double t) {
    assert(t >= 0.0 && t <= 1.0);
    if (a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w < 0.0)
        b = {-b.x, -b.y, -b.z, -b.w};
    return Normalize({(1-t)*a.x+t*b.x, (1-t)*a.y+t*b.y,
                      (1-t)*a.z+t*b.z, (1-t)*a.w+t*b.w});
}
double ShortestAngleDelta(double fromDegrees, double toDegrees) {
    return std::remainder(toDegrees - fromDegrees, 360.0);
}
bool RayBox(const std::array<double,3>& origin,
            const std::array<double,3>& direction,
            const std::array<double,3>& low,
            const std::array<double,3>& high) {
    double enter = -std::numeric_limits<double>::infinity();
    double exit = std::numeric_limits<double>::infinity();
    for (std::size_t axis = 0; axis < 3; ++axis) {
        if (direction[axis] == 0.0) {
            if (origin[axis] < low[axis] || origin[axis] > high[axis]) return false;
            continue;
        }
        double a = (low[axis] - origin[axis]) / direction[axis];
        double b = (high[axis] - origin[axis]) / direction[axis];
        if (a > b) std::swap(a, b);
        enter = std::max(enter, a);
        exit = std::min(exit, b);
    }
    return exit >= std::max(enter, 0.0);
}
int main() {
    assert(Near(ShortestAngleDelta(179, -179), 2));
    const Quat z90{0, 0, std::sin(pi/4), std::cos(pi/4)};
    const Vec3 turned = Rotate(z90, {1, 0, 0});
    assert(Near(turned.x, 0) && Near(turned.y, 1));
    const Quat neg{-z90.x, -z90.y, -z90.z, -z90.w};
    const Vec3 halfway = Rotate(Nlerp(z90, neg, 0.5), {1,0,0});
    assert(Near(halfway.x, 0) && Near(halfway.y, 1));
    const Vec3 transformed = Rotate(z90, Vec3{1,0,0} * 2.0) + Vec3{5,0,0};
    assert(Near(transformed.x, 5) && Near(transformed.y, 2));
    const double skinned = 0.25 * ((11.0 - 10.0) + 12.0) + 0.75 * 11.0;
    assert(Near(skinned, 11.5));
    const double u = (0.5*0.0/1.0 + 0.5*1.0/2.0) / (0.5/1.0 + 0.5/2.0);
    assert(Near(u, 1.0/3.0));
    assert(RayBox({0,0,-2}, {0,0,1}, {-1,-1,-1}, {1,1,1}));
    assert(!RayBox({2,0,-2}, {0,0,1}, {-1,-1,-1}, {1,1,1}));
    assert(RayBox({0,0,0}, {0,0,1}, {-1,-1,-1}, {1,1,1}));
    assert(!RayBox({0,0,2}, {0,0,1}, {-1,-1,-1}, {1,1,1}));
    std::cout << "angle=2 rotated=(0,1,0) point=(5,2,0) skin=11.5 u=1/3\n";
}
