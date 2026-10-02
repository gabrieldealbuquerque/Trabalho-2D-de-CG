#include "math.hpp"

namespace cg {
namespace {
constexpr double contactTolerance = 1e-10;
constexpr double directionTolerance = 1e-14;
constexpr double zeroLengthTolerance = 1e-20;
}

bool circleRect(Vec2 center, double radius, const Rect& rect) {
    const Vec2 closest{
        std::clamp(center.x, rect.center.x - rect.width / 2, rect.center.x + rect.width / 2),
        std::clamp(center.y, rect.center.y - rect.height / 2, rect.center.y + rect.height / 2)};
    const Vec2 distance = center - closest;
    return dot(distance, distance) <= radius * radius + contactTolerance;
}

double segmentCircle(Vec2 from, Vec2 to, Vec2 center, double radius) {
    const Vec2 offset = from - center;
    const Vec2 delta = to - from;
    const double c = dot(offset, offset) - radius * radius;
    if (c <= contactTolerance) return 0;
    const double a = dot(delta, delta);
    if (a < zeroLengthTolerance) return noHit;
    const double b = dot(offset, delta);
    const double discriminant = b * b - a * c;
    if (discriminant < 0) return noHit;
    const double hitTime = (-b - std::sqrt(discriminant)) / a;
    if (hitTime >= 0 && hitTime <= 1) {
        return hitTime;
    }
    return noHit;
}

double segmentRect(Vec2 from, Vec2 to, const Rect& rect) {
    double entryTime = 0;
    double exitTime = 1;
    const Vec2 local = from - rect.center;
    const Vec2 delta = to - from;
    const double origins[] = {local.x, local.y};
    const double directions[] = {delta.x, delta.y};
    const double extents[] = {rect.width / 2, rect.height / 2};
    for (int axis = 0; axis < 2; ++axis) {
        if (std::abs(directions[axis]) < directionTolerance) {
            if (std::abs(origins[axis]) > extents[axis]) {
                return noHit;
            }
        } else {
            double nearTime = (-extents[axis] - origins[axis]) / directions[axis];
            double farTime = (extents[axis] - origins[axis]) / directions[axis];
            if (nearTime > farTime) {
                std::swap(nearTime, farTime);
            }
            entryTime = std::max(entryTime, nearTime);
            exitTime = std::min(exitTime, farTime);
            if (entryTime > exitTime) return noHit;
        }
    }
    return entryTime;
}

double sweepCircleRect(Vec2 from, Vec2 to, double radius, const Rect& rect) {
    if (circleRect(from, radius, rect)) return 0;
    const Rect horizontalStrip{rect.center, rect.width + 2 * radius, rect.height};
    const Rect verticalStrip{rect.center, rect.width, rect.height + 2 * radius};
    double hit = std::min(segmentRect(from, to, horizontalStrip), segmentRect(from, to, verticalStrip));
    for (double x : {-1.0, 1.0}) {
        for (double y : {-1.0, 1.0}) {
            const Vec2 corner = rect.center + Vec2{x * rect.width / 2, y * rect.height / 2};
            hit = std::min(hit, segmentCircle(from, to, corner, radius));
        }
    }
    return hit;
}
}
