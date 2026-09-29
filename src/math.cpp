#include "math.hpp"

namespace cg {
bool circleRect(Vec2 center, double radius, const Rect& rect) {
    const Vec2 closest{
        std::clamp(center.x, rect.center.x - rect.width / 2, rect.center.x + rect.width / 2),
        std::clamp(center.y, rect.center.y - rect.height / 2, rect.center.y + rect.height / 2)};
    return dot(center - closest, center - closest) <= radius * radius + 1e-10;
}

double segmentCircle(Vec2 from, Vec2 to, Vec2 center, double radius) {
    const Vec2 offset = from - center, delta = to - from;
    const double c = dot(offset, offset) - radius * radius;
    if (c <= 1e-10) return 0;
    const double a = dot(delta, delta);
    if (a < 1e-20) return noHit;
    const double b = dot(offset, delta);
    const double discriminant = b * b - a * c;
    if (discriminant < 0) return noHit;
    const double t = (-b - std::sqrt(discriminant)) / a;
    return t >= 0 && t <= 1 ? t : noHit;
}

double segmentRect(Vec2 from, Vec2 to, const Rect& rect) {
    double enter = 0, leave = 1;
    const Vec2 local = from - rect.center, delta = to - from;
    const double origins[] = {local.x, local.y};
    const double directions[] = {delta.x, delta.y};
    const double extents[] = {rect.width / 2, rect.height / 2};
    for (int axis = 0; axis < 2; ++axis) {
        if (std::abs(directions[axis]) < 1e-14) {
            if (std::abs(origins[axis]) > extents[axis]) return noHit;
        } else {
            double a = (-extents[axis] - origins[axis]) / directions[axis];
            double b = (extents[axis] - origins[axis]) / directions[axis];
            if (a > b) std::swap(a, b);
            enter = std::max(enter, a);
            leave = std::min(leave, b);
            if (enter > leave) return noHit;
        }
    }
    return enter;
}

double sweepCircleRect(Vec2 from, Vec2 to, double radius, const Rect& rect) {
    if (circleRect(from, radius, rect)) return 0;
    double hit = std::min(
        segmentRect(from, to, {rect.center, rect.width + 2 * radius, rect.height}),
        segmentRect(from, to, {rect.center, rect.width, rect.height + 2 * radius}));
    for (double x : {-1.0, 1.0})
        for (double y : {-1.0, 1.0})
            hit = std::min(hit, segmentCircle(from, to,
                rect.center + Vec2{x * rect.width / 2, y * rect.height / 2}, radius));
    return hit;
}
}
