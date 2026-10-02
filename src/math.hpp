#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

namespace cg {
constexpr double pi = 3.14159265358979323846;
constexpr double noHit = std::numeric_limits<double>::infinity();
struct Vec2 {
    double x = 0;
    double y = 0;
    Vec2 operator+(Vec2 b) const { return {x + b.x, y + b.y}; }
    Vec2 operator-(Vec2 b) const { return {x - b.x, y - b.y}; }
    Vec2 operator*(double s) const { return {x * s, y * s}; }
};
inline double dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
inline double length(Vec2 a) { return std::sqrt(dot(a, a)); }
inline double radians(double degrees) { return degrees * pi / 180; }
inline double wrapAngle(double degrees) { return std::remainder(degrees, 360.0); }
inline Vec2 forward(double degrees) {
    const double a = radians(degrees);
    return {-std::sin(a), std::cos(a)};
}
inline double angleTo(Vec2 origin, Vec2 target) {
    const Vec2 direction = target - origin;
    return std::atan2(-direction.x, direction.y) * 180 / pi;
}
inline Vec2 rotate(Vec2 p, double degrees) {
    const double a = radians(degrees);
    return {p.x * std::cos(a) - p.y * std::sin(a),
            p.x * std::sin(a) + p.y * std::cos(a)};
}
struct Rect {
    Vec2 center;
    double width;
    double height;
};
bool circleRect(Vec2 center, double radius, const Rect& rect);
double segmentCircle(Vec2 from, Vec2 to, Vec2 center, double radius);
double segmentRect(Vec2 from, Vec2 to, const Rect& rect);
double sweepCircleRect(Vec2 from, Vec2 to, double radius, const Rect& rect);
}
