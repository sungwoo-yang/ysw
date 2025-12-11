#pragma once
#include "Engine/Vec2.hpp"
#include <optional>
#include <vector>

namespace Physics
{
    bool       RaySegmentIntersection(Math::vec2 rayOrigin, Math::vec2 rayDir, Math::vec2 segStart, Math::vec2 segEnd, Math::vec2& outIntersection, double& outT);
    Math::vec2 CalculateReflection(Math::vec2 incident, Math::vec2 normal);
    std::vector<std::pair<Math::vec2, Math::vec2>>
        CalculateLaserPath(Math::vec2 startPos, Math::vec2 initialDir, const std::vector<std::pair<Math::vec2, Math::vec2>>& segments, int maxBounces = 5, double maxLength = 15000.0);
}
