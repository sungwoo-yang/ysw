#include "Reflection.hpp"
#include "Engine/Vec2.hpp"
#include <cmath>
#include <limits>

namespace
{
    // Get perpendicular vector
    inline Math::vec2 perpendicular(const Math::vec2& v)
    {
        return Math::vec2{ -v.y, v.x };
    }
}

namespace Physics
{
    bool RaySegmentIntersection(Math::vec2 rayOrigin, Math::vec2 rayDir, Math::vec2 segStart, Math::vec2 segEnd, Math::vec2& outIntersection, double& outT)
    {
        // Optimization: Use const for fixed math values
        const Math::vec2 p = rayOrigin;
        const Math::vec2 r = rayDir;
        const Math::vec2 q = segStart;
        const Math::vec2 s = segEnd - segStart;

        const double     rxs   = r.x * s.y - r.y * s.x;
        const Math::vec2 qmp   = q - p;
        const double     qmpxr = qmp.x * r.y - qmp.y * r.x;
        const double     qmpxs = qmp.x * s.y - qmp.y * s.x;

        // Check parallel
        if (std::abs(rxs) < std::numeric_limits<double>::epsilon())
        {
            return false;
        }

        const double t = qmpxs / rxs;
        const double u = qmpxr / rxs;

        // Check if intersection is within segment bounds
        if (t >= -std::numeric_limits<double>::epsilon() && u >= -std::numeric_limits<double>::epsilon() && u <= 1.0 + std::numeric_limits<double>::epsilon())
        {
            outT            = t;
            outIntersection = p + r * t;
            return true;
        }
        return false;
    }

    Math::vec2 CalculateReflection(Math::vec2 incident, Math::vec2 normal)
    {
        // Reflection logic
        Math::vec2 norm        = normal.Normalize();
        double     dot_product = dot(incident, norm);
        Math::vec2 reflection  = incident - 2.0 * dot_product * norm;
        return reflection.Normalize();
    }

    std::vector<std::pair<Math::vec2, Math::vec2>> CalculateLaserPath(Math::vec2 startPos, Math::vec2 initialDir, const std::vector<LineSegment>& segments, int maxBounces, double maxLength)
    {
        std::vector<std::pair<Math::vec2, Math::vec2>> path;

        // Optimization: Reserve vector memory
        path.reserve(static_cast<size_t>(maxBounces + 1));

        Math::vec2   currentPos = startPos;
        Math::vec2   currentDir = initialDir.Normalize();
        const double epsilon    = 0.1;

        double remainingLength = maxLength;

        for (int bounce = 0; bounce <= maxBounces; ++bounce)
        {
            if (remainingLength <= 0.0) break;

            double     closestT = std::numeric_limits<double>::infinity();
            Math::vec2 closestIntersection;
            Math::vec2 surfaceNormal;
            int        intersectedIndex = -1;

            // Check all segments
            for (size_t i = 0; i < segments.size(); ++i)
            {
                Math::vec2 intersectionPoint;
                double     t;
                if (RaySegmentIntersection(currentPos, currentDir, segments[i].p1, segments[i].p2, intersectionPoint, t))
                {
                    if (t > epsilon && t < closestT)
                    {
                        closestT            = t;
                        closestIntersection = intersectionPoint;
                        intersectedIndex    = static_cast<int>(i);

                        Math::vec2 segmentVec = segments[i].p2 - segments[i].p1;
                        Math::vec2 normal     = perpendicular(segmentVec).Normalize();

                        // Correct normal direction
                        if (dot(normal, -currentDir) < 0)
                            normal = -normal;
                        surfaceNormal = normal;
                    }
                }
            }

            if (intersectedIndex != -1 && closestT <= remainingLength)
            {
                path.emplace_back(currentPos, closestIntersection);
                
                remainingLength -= closestT;

                if (segments[static_cast<size_t>(intersectedIndex)].isReflective)
                {
                    // Apply reflection
                    currentPos = closestIntersection + surfaceNormal * epsilon;
                    currentDir = CalculateReflection(currentDir, surfaceNormal);
                }
                else
                {
                    // Stop if not reflective
                    break;
                }
            }
            else
            {
                // No hit
                Math::vec2 endPoint = currentPos + currentDir * remainingLength;
                path.push_back({ currentPos, endPoint });
                break;
            }
        }
        return path;
    }
}
