#include "BossPatternController.hpp"

#include "BossConfig.hpp"
#include "BossLaser.hpp"
#include "BossLaserManager.hpp"
#include "LaserStar.hpp"
#include "Player.hpp"

#include <cmath>
#include <numeric>

namespace
{
    double NormalizeAngleDifference(double diff)
    {
        while (diff <= -PI)
        {
            diff += 2.0 * PI;
        }

        while (diff > PI)
        {
            diff -= 2.0 * PI;
        }

        return diff;
    }
}

namespace Boss
{
    BossPatternController::BossPatternController(Player* in_player, BossLaserManager* in_laserManager) : player(in_player), laserManager(in_laserManager), rng(std::random_device{}())
    {
    }

    void BossPatternController::Update(double dt)
    {
        if (!enabled || laserManager == nullptr)
        {
            return;
        }

        if (patternActive)
        {
            patternTimer += dt;
            UpdateActivePattern(dt);

            if (patternTimer >= patternDuration)
            {
                EndCurrentPattern();
            }

            return;
        }

        if (cooldownTimer > 0.0)
        {
            cooldownTimer -= dt;

            if (cooldownTimer < 0.0)
            {
                cooldownTimer = 0.0;
            }

            return;
        }

        StartRandomPattern();
    }

    void BossPatternController::Clear()
    {
        EndCurrentPattern();
        cooldownTimer = 0.0;
    }

    void BossPatternController::SetEnabled(bool in_enabled)
    {
        enabled = in_enabled;

        if (!enabled)
        {
            Clear();
        }
    }

    bool BossPatternController::IsEnabled() const
    {
        return enabled;
    }

    void BossPatternController::SetMainStar(LaserStar* in_mainStar)
    {
        mainStar = in_mainStar;
    }

    bool BossPatternController::IsPatternActive() const
    {
        return patternActive;
    }

    BossPatternType BossPatternController::GetCurrentPatternType() const
    {
        return currentPattern;
    }

    std::string BossPatternController::GetCurrentPatternName() const
    {
        switch (currentPattern)
        {
            case BossPatternType::FourWayRotatingYellowLaser: return "FourWayRotatingYellowLaser";

            case BossPatternType::CrossWallYellowLaser: return "CrossWallYellowLaser";

            case BossPatternType::OrangeLaserRain: return "OrangeLaserRain";

            case BossPatternType::RedTrackingLaser: return "RedTrackingLaser";

            case BossPatternType::TripleShortParryLaser: return "TripleShortParryLaser";
        }

        return "Unknown";
    }

    void BossPatternController::StartRandomPattern()
    {
        StartPattern(PickWeightedPattern());
    }

    void BossPatternController::StartPattern(BossPatternType pattern)
    {
        currentPattern = pattern;

        patternActive = true;
        patternTimer  = 0.0;
        activePatternLasers.clear();

        switch (currentPattern)
        {
            case BossPatternType::FourWayRotatingYellowLaser: StartFourWayRotatingYellowLaser(); break;

            case BossPatternType::CrossWallYellowLaser: StartCrossWallYellowLaser(); break;

            case BossPatternType::OrangeLaserRain: StartOrangeLaserRain(); break;

            case BossPatternType::RedTrackingLaser: StartRedTrackingLaser(); break;

            case BossPatternType::TripleShortParryLaser: StartTripleShortParryLaser(); break;
        }
    }

    void BossPatternController::StartFourWayRotatingYellowLaser()
    {
        const Math::vec2 center = { static_cast<double>(Config::ArenaCenterX), static_cast<double>(Config::ArenaCenterY) };

        fourWayBaseAngle    = RandomDouble(0.0, PI * 2.0);
        fourWayRotationSign = RandomInt(0, 1) == 0 ? -1.0 : 1.0;

        patternDuration = Config::BossLaserWarningTime + Config::FourWayLaserActiveTime;

        for (int i = 0; i < 4; ++i)
        {
            const double     angle     = fourWayBaseAngle + static_cast<double>(i) * (PI * 0.5);
            const Math::vec2 direction = { std::cos(angle), std::sin(angle) };

            BossLaser* laser =
                laserManager->SpawnLaser(center, direction, LaserType::Yellow, LaserSource::Ophiuchus, Config::FourWayLaserLength, Config::BossLaserWarningTime, Config::FourWayLaserActiveTime);

            activePatternLasers.push_back(laser);
        }
    }

    void BossPatternController::StartCrossWallYellowLaser()
    {
        const Math::vec2 targetPosition = player != nullptr ? player->GetPosition() : Math::vec2{ static_cast<double>(Config::ArenaCenterX), static_cast<double>(Config::GroundY) };

        const double minY = static_cast<double>(Config::GroundY) + 180.0;
        const double maxY = static_cast<double>(Config::ArenaTopY);

        const Math::vec2 leftStart = { static_cast<double>(Config::ArenaLeftX), RandomDouble(minY, maxY) };

        const Math::vec2 rightStart = { static_cast<double>(Config::ArenaRightX), RandomDouble(minY, maxY) };

        const Math::vec2 leftDirection  = (targetPosition - leftStart).Normalize();
        const Math::vec2 rightDirection = (targetPosition - rightStart).Normalize();

        patternDuration = Config::BossLaserWarningTime + Config::CrossWallLaserActiveTime;

        activePatternLasers.push_back(laserManager->SpawnLaser(
            leftStart, leftDirection, LaserType::Yellow, LaserSource::Ophiuchus, Config::CrossWallLaserLength, Config::BossLaserWarningTime, Config::CrossWallLaserActiveTime));

        activePatternLasers.push_back(laserManager->SpawnLaser(
            rightStart, rightDirection, LaserType::Yellow, LaserSource::Ophiuchus, Config::CrossWallLaserLength, Config::BossLaserWarningTime, Config::CrossWallLaserActiveTime));
    }

    void BossPatternController::StartOrangeLaserRain()
    {
        patternDuration = Config::BossLaserWarningTime + Config::OrangeRainLaserActiveTime;

        for (int i = 0; i < Config::OrangeRainLaserCount; ++i)
        {
            const double x = RandomDouble(static_cast<double>(Config::ArenaLeftX), static_cast<double>(Config::ArenaRightX));

            const Math::vec2 start = { x, static_cast<double>(Config::ArenaTopY) };

            const double     xDrift    = RandomDouble(-0.25, 0.25);
            const Math::vec2 direction = Math::vec2{ xDrift, -1.0 }.Normalize();

            activePatternLasers.push_back(
                laserManager->SpawnLaser(start, direction, LaserType::Orange, LaserSource::Ophiuchus, Config::OrangeRainLaserLength, Config::BossLaserWarningTime, Config::OrangeRainLaserActiveTime));
        }
    }

    void BossPatternController::StartRedTrackingLaser()
    {
        const Math::vec2 start = GetMainStarPosition();

        const Math::vec2 target = player != nullptr ? player->GetPosition() : Math::vec2{ static_cast<double>(Config::ArenaCenterX), static_cast<double>(Config::GroundY) };

        const Math::vec2 direction = (target - start).Normalize();

        patternDuration = Config::BossLaserWarningTime + Config::RedTrackingLaserActiveTime;

        activePatternLasers.push_back(laserManager->SpawnLaser(
            start, direction, LaserType::RedTracking, LaserSource::MainConstellation, Config::RedTrackingLaserLength, Config::BossLaserWarningTime, Config::RedTrackingLaserActiveTime));
    }

    void BossPatternController::StartTripleShortParryLaser()
    {
        tripleShortFiredCount = 0;
        tripleShortTimer      = 0.0;

        patternDuration = Config::TripleShortLaserInterval * static_cast<double>(Config::TripleShortLaserCount - 1) + Config::ShortParryLaserActiveTime + 0.2;
        
        SpawnShortParryLaserAtPlayer();
    }

    void BossPatternController::UpdateActivePattern(double dt)
    {
        switch (currentPattern)
        {
            case BossPatternType::FourWayRotatingYellowLaser: UpdateFourWayRotatingYellowLaser(); break;

            case BossPatternType::RedTrackingLaser: UpdateRedTrackingLaser(dt); break;

            case BossPatternType::TripleShortParryLaser: UpdateTripleShortParryLaser(dt); break;

            case BossPatternType::CrossWallYellowLaser:
            case BossPatternType::OrangeLaserRain: break;
        }
    }

    void BossPatternController::UpdateFourWayRotatingYellowLaser()
    {
        const Math::vec2 center = { static_cast<double>(Config::ArenaCenterX), static_cast<double>(Config::ArenaCenterY) };

        const double angleOffset = fourWayRotationSign * DegreesToRadians(static_cast<double>(Config::FourWayRotateSpeed)) * patternTimer;

        for (size_t i = 0; i < activePatternLasers.size(); ++i)
        {
            BossLaser* laser = activePatternLasers[i];

            if (laser == nullptr || laser->IsExpired())
            {
                continue;
            }

            const double     angle     = fourWayBaseAngle + static_cast<double>(i) * (PI * 0.5) + angleOffset;
            const Math::vec2 direction = { std::cos(angle), std::sin(angle) };

            laser->SetStartPosition(center);
            laser->SetDirection(direction);
        }
    }

    void BossPatternController::UpdateRedTrackingLaser(double dt)
    {
        if (player == nullptr || activePatternLasers.empty())
        {
            return;
        }

        BossLaser* laser = activePatternLasers.front();

        if (laser == nullptr || laser->IsExpired())
        {
            return;
        }

        const Math::vec2 start = GetMainStarPosition();
        laser->SetStartPosition(start);

        const Math::vec2 targetDirection  = (player->GetPosition() - start).Normalize();
        const Math::vec2 currentDirection = laser->GetDirection();

        double currentAngle = std::atan2(currentDirection.y, currentDirection.x);
        double targetAngle  = std::atan2(targetDirection.y, targetDirection.x);

        const double diff = NormalizeAngleDifference(targetAngle - currentAngle);

        const double maxRotate = DegreesToRadians(static_cast<double>(Config::RedTrackingRotateSpeed)) * dt;

        if (std::abs(diff) <= maxRotate)
        {
            currentAngle = targetAngle;
        }
        else
        {
            currentAngle += diff > 0.0 ? maxRotate : -maxRotate;
        }

        const Math::vec2 newDirection = { std::cos(currentAngle), std::sin(currentAngle) };

        laser->SetDirection(newDirection);
    }

    void BossPatternController::UpdateTripleShortParryLaser(double dt)
    {
        for (BossLaser* laser : activePatternLasers)
        {
            if (laser == nullptr || laser->IsExpired())
            {
                continue;
            }

            if (laser->GetLaserType() != LaserType::ShortParry)
            {
                continue;
            }

            const Math::vec2 newStart = laser->GetStartPosition() + laser->GetDirection() * (Config::ShortParryLaserSpeed * dt);

            laser->SetStartPosition(newStart);
        }

        if (tripleShortFiredCount >= Config::TripleShortLaserCount)
        {
            return;
        }

        tripleShortTimer += dt;

        if (tripleShortTimer >= Config::TripleShortLaserInterval)
        {
            tripleShortTimer = 0.0;
            SpawnShortParryLaserAtPlayer();
        }
    }

    void BossPatternController::EndCurrentPattern()
    {
        for (BossLaser* laser : activePatternLasers)
        {
            if (laser != nullptr && !laser->IsExpired())
            {
                laser->Expire();
            }
        }

        activePatternLasers.clear();

        patternActive   = false;
        patternTimer    = 0.0;
        patternDuration = 0.0;
        cooldownTimer   = Config::PatternInterval;

        tripleShortFiredCount = 0;
        tripleShortTimer      = 0.0;
    }

    void BossPatternController::SpawnShortParryLaserAtPlayer()
    {
        if (laserManager == nullptr)
        {
            return;
        }

        const Math::vec2 start = GetMainStarPosition();

        const Math::vec2 target = player != nullptr ? player->GetPosition() : Math::vec2{ static_cast<double>(Config::ArenaCenterX), static_cast<double>(Config::GroundY) };

        const Math::vec2 direction = (target - start).Normalize();

        activePatternLasers.push_back(
            laserManager->SpawnLaser(start, direction, LaserType::ShortParry, LaserSource::MainConstellation, Config::ShortParryLaserLength, 0.0, Config::TripleShortLaserDuration));

        ++tripleShortFiredCount;
    }

    BossPatternType BossPatternController::PickWeightedPattern()
    {
        const int totalWeight = GetTotalPatternWeight();

        if (totalWeight <= 0)
        {
            return BossPatternType::CrossWallYellowLaser;
        }

        int roll = RandomInt(1, totalWeight);

        roll -= Config::FourWayRotatingYellowLaserWeight;
        if (roll <= 0)
        {
            return BossPatternType::FourWayRotatingYellowLaser;
        }

        roll -= Config::CrossWallYellowLaserWeight;
        if (roll <= 0)
        {
            return BossPatternType::CrossWallYellowLaser;
        }

        roll -= Config::OrangeLaserRainWeight;
        if (roll <= 0)
        {
            return BossPatternType::OrangeLaserRain;
        }

        roll -= Config::RedTrackingLaserWeight;
        if (roll <= 0)
        {
            return BossPatternType::RedTrackingLaser;
        }

        return BossPatternType::TripleShortParryLaser;
    }

    int BossPatternController::GetTotalPatternWeight() const
    {
        return Config::FourWayRotatingYellowLaserWeight + Config::CrossWallYellowLaserWeight + Config::OrangeLaserRainWeight + Config::RedTrackingLaserWeight + Config::TripleShortParryLaserWeight;
    }

    Math::vec2 BossPatternController::GetMainStarPosition() const
    {
        if (mainStar != nullptr)
        {
            return mainStar->GetPosition();
        }

        return { static_cast<double>(Config::ArenaCenterX), static_cast<double>(Config::ArenaTopY) };
    }

    double BossPatternController::RandomDouble(double minValue, double maxValue)
    {
        std::uniform_real_distribution<double> distribution(minValue, maxValue);
        return distribution(rng);
    }

    int BossPatternController::RandomInt(int minValue, int maxValue)
    {
        std::uniform_int_distribution<int> distribution(minValue, maxValue);
        return distribution(rng);
    }

    double BossPatternController::DegreesToRadians(double degrees)
    {
        return degrees * PI / 180.0;
    }
}