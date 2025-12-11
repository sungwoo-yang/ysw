#include "Reflection.hpp"
#include "Engine/Vec2.hpp"
#include <cmath>
#include <limits>

namespace
{
    inline Math::vec2 perpendicular(const Math::vec2& v)
    {
        return Math::vec2{ -v.y, v.x };
    }
}

namespace Physics
{
    bool RaySegmentIntersection(Math::vec2 rayOrigin, Math::vec2 rayDir, Math::vec2 segStart, Math::vec2 segEnd, Math::vec2& outIntersection, double& outT)
    {
        const Math::vec2 p = rayOrigin;
        const Math::vec2 r = rayDir;
        const Math::vec2 q = segStart;
        const Math::vec2 s = segEnd - segStart;

        const double     rxs   = r.x * s.y - r.y * s.x;
        const Math::vec2 qmp   = q - p;
        const double     qmpxr = qmp.x * r.y - qmp.y * r.x;
        const double     qmpxs = qmp.x * s.y - qmp.y * s.x;

        if (std::abs(rxs) < std::numeric_limits<double>::epsilon())
        {
            return false;
        }

        const double t = qmpxs / rxs;
        const double u = qmpxr / rxs;

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
        Math::vec2 norm        = normal.Normalize();
        double     dot_product = dot(incident, norm);
        Math::vec2 reflection  = incident - 2.0 * dot_product * norm;
        return reflection.Normalize();
    }

    std::vector<std::pair<Math::vec2, Math::vec2>>
        CalculateLaserPath(Math::vec2 startPos, Math::vec2 initialDir, const std::vector<std::pair<Math::vec2, Math::vec2>>& segments, int maxBounces, double maxLength)
    {
        std::vector<std::pair<Math::vec2, Math::vec2>> path;
        Math::vec2                                     currentPos        = startPos;
        Math::vec2                                     currentDir        = initialDir.Normalize();
        const double                                   verySmallDistance = 1e-6;

        for (int bounce = 0; bounce <= maxBounces; ++bounce)
        {
            double     closestT = std::numeric_limits<double>::infinity();
            Math::vec2 closestIntersection;
            Math::vec2 surfaceNormal;
            int        intersectedSegmentIndex = -1;

            for (size_t i = 0; i < segments.size(); ++i)
            {
                Math::vec2 intersectionPoint;
                double     t;
                if (RaySegmentIntersection(currentPos, currentDir, segments[i].first, segments[i].second, intersectionPoint, t))
                {
                    if (t > verySmallDistance && t < closestT)
                    {
                        closestT                = t;
                        closestIntersection     = intersectionPoint;
                        intersectedSegmentIndex = static_cast<int>(i);

                        Math::vec2 segmentVec = segments[i].second - segments[i].first;
                        Math::vec2 normal     = perpendicular(segmentVec).Normalize();

                        if (dot(normal, -currentDir) < 0)
                        {
                            normal = -normal;
                        }
                        surfaceNormal = normal;
                    }
                }
            }

            if (intersectedSegmentIndex != -1)
            {
                path.push_back({ currentPos, closestIntersection });
                currentPos = closestIntersection + surfaceNormal * verySmallDistance;
                currentDir = CalculateReflection(currentDir, surfaceNormal);
            }
            else
            {
                Math::vec2 endPoint = currentPos + currentDir * maxLength;
                path.push_back({ currentPos, endPoint });
                break;
            }
        }
        return path;
    }
}
