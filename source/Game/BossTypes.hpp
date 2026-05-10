#pragma once

namespace Boss
{
    enum class LaserType
    {
        Yellow,          // Creates light orbs, normal damage
        Orange,          // No light orb, normal damage
        RedTracking,     // Strong tracking laser, not parryable
        ShortParry       // Short laser used for parry energy gain
    };

    enum class LaserSource
    {
        Ophiuchus,        // 13th constellation interference pattern
        MainConstellation // Main constellation direct attack pattern
    };

    enum class BossPatternType
    {
        FourWayRotatingYellowLaser,
        CrossWallYellowLaser,
        OrangeLaserRain,
        RedTrackingLaser,
        TripleShortParryLaser
    };

    enum class DamageLevel
    {
        None,
        Normal,
        Strong
    };

    enum class LaserState
    {
        Warning,
        Tracking,
        Active,
        Expired
    };

    enum class BossFightState
    {
        Intro,
        Playing,
        Clear
    };
}