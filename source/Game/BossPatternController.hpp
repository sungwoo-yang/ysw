#pragma once

#include "BossTypes.hpp"
#include "Engine/Vec2.hpp"

#include <random>
#include <string>
#include <vector>

class Player;
class LaserStar;

namespace Boss
{
    class BossLaser;
    class BossLaserManager;

    class BossPatternController
    {
    public:
        BossPatternController(Player* in_player, BossLaserManager* in_laserManager);

        void Update(double dt);
        void Clear();

        void SetEnabled(bool enabled);
        bool IsEnabled() const;

        void SetMainStar(LaserStar* in_mainStar);

        bool            IsPatternActive() const;
        BossPatternType GetCurrentPatternType() const;
        std::string     GetCurrentPatternName() const;

    private:
        void StartRandomPattern();
        void StartPattern(BossPatternType pattern);

        void StartFourWayRotatingYellowLaser();
        void StartCrossWallYellowLaser();
        void StartOrangeLaserRain();
        void StartRedTrackingLaser();
        void StartTripleShortParryLaser();

        void UpdateActivePattern(double dt);
        void UpdateFourWayRotatingYellowLaser();
        void UpdateRedTrackingLaser(double dt);
        void UpdateTripleShortParryLaser(double dt);

        void EndCurrentPattern();

        void SpawnShortParryLaserAtPlayer();

        BossPatternType PickWeightedPattern();

        int        GetTotalPatternWeight() const;
        Math::vec2 GetMainStarPosition() const;

        double RandomDouble(double minValue, double maxValue);
        int    RandomInt(int minValue, int maxValue);

        static double DegreesToRadians(double degrees);

        Player*    player   = nullptr;
        LaserStar* mainStar = nullptr;
        BossLaserManager* laserManager = nullptr;

        bool enabled       = true;
        bool patternActive = false;

        BossPatternType currentPattern = BossPatternType::CrossWallYellowLaser;

        double cooldownTimer   = 0.0;
        double patternTimer    = 0.0;
        double patternDuration = 0.0;

        std::vector<BossLaser*> activePatternLasers;

        // Four-way rotating laser state
        double fourWayBaseAngle    = 0.0;
        double fourWayRotationSign = 1.0;

        // Triple short parry laser state
        int    tripleShortFiredCount = 0;
        double tripleShortTimer      = 0.0;

        std::mt19937 rng;
    };
}