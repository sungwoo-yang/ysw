#pragma once

namespace Boss::Config
{
    // -------------------------------------------------------------------------
    // Shield Energy
    // -------------------------------------------------------------------------

    inline constexpr float MaxShieldEnergy = 100.0f;

    inline constexpr float LightOrbEnergyGain   = 15.0f;
    inline constexpr float ShortParryEnergyGain = 25.0f;

    // -------------------------------------------------------------------------
    // Light Orb
    // -------------------------------------------------------------------------

    inline constexpr int MaxLightOrbCount = 3;

    inline constexpr float LightOrbAttractRadius    = 120.0f;
    inline constexpr float LightOrbCollectRadius    = 24.0f;
    inline constexpr float LightOrbMoveSpeed        = 700.0f;
    inline constexpr float LightOrbSpawnCooldown    = 1.0f;
    inline constexpr float MinLightOrbSpawnDistance = 100.0f;

    // -------------------------------------------------------------------------
    // Damage
    // -------------------------------------------------------------------------

    inline constexpr double YellowLaserDamage      = 1.0;
    inline constexpr double OrangeLaserDamage      = 1.0;
    inline constexpr double RedTrackingLaserDamage = 2.0;
    inline constexpr double ShortParryLaserDamage  = 1.0;

    // -------------------------------------------------------------------------
    // Parry
    // -------------------------------------------------------------------------

    inline constexpr float ParryWindow   = 0.25f;
    inline constexpr float ParryCooldown = 0.5f;

    // -------------------------------------------------------------------------
    // Charge Shot
    // -------------------------------------------------------------------------

    inline constexpr float RequiredChargeEnergy = 100.0f;
    inline constexpr float ChargeHoldTime       = 0.8f;

    // -------------------------------------------------------------------------
    // Pattern Weights
    // Later, adjust only these values to rebalance pattern frequency.
    // -------------------------------------------------------------------------

    inline constexpr int FourWayRotatingYellowLaserWeight = 20;
    inline constexpr int CrossWallYellowLaserWeight       = 20;
    inline constexpr int OrangeLaserRainWeight            = 20;
    inline constexpr int RedTrackingLaserWeight           = 20;
    inline constexpr int TripleShortParryLaserWeight      = 20;

    // -------------------------------------------------------------------------
    // Pattern Timing
    // -------------------------------------------------------------------------

    inline constexpr float PatternInterval = 2.0f;
    inline constexpr float WarningTime     = 1.0f;

    // -------------------------------------------------------------------------
    // Four-way rotating yellow laser
    // -------------------------------------------------------------------------

    inline constexpr float FourWayRotateMinAngle = 90.0f;
    inline constexpr float FourWayRotateMaxAngle = 180.0f;
    inline constexpr float FourWayRotateSpeed    = 45.0f;

    // -------------------------------------------------------------------------
    // Triple short parry laser
    // -------------------------------------------------------------------------

    inline constexpr int TripleShortLaserCount = 3;

    inline constexpr float TripleShortLaserInterval = 0.4f;
    inline constexpr float TripleShortLaserDuration = 0.2f;

    // -------------------------------------------------------------------------
    // Arena
    // -------------------------------------------------------------------------

    inline constexpr float ArenaLeftX   = 0.0f;
    inline constexpr float ArenaRightX  = 3000.0f;
    inline constexpr float ArenaTopY    = 1440.0f;
    inline constexpr float ArenaCenterX = 1500.0f;
    inline constexpr float ArenaCenterY = 500.0f;
    inline constexpr float GroundY      = 0.0f;

    // -------------------------------------------------------------------------
    // Boss Laser Common
    // -------------------------------------------------------------------------

    inline constexpr double BossLaserDefaultLength = 3200.0;
    inline constexpr double BossLaserWarningTime   = 1.0;

    // -------------------------------------------------------------------------
    // Four-way rotating yellow laser
    // -------------------------------------------------------------------------

    inline constexpr double FourWayLaserLength     = 3200.0;
    inline constexpr double FourWayLaserActiveTime = 4.0;

    // -------------------------------------------------------------------------
    // Cross wall yellow laser
    // -------------------------------------------------------------------------

    inline constexpr double CrossWallLaserLength     = 3200.0;
    inline constexpr double CrossWallLaserActiveTime = 1.2;

    // -------------------------------------------------------------------------
    // Orange laser rain
    // -------------------------------------------------------------------------

    inline constexpr int OrangeRainLaserCount = 8;

    inline constexpr double OrangeRainLaserLength     = 1800.0;
    inline constexpr double OrangeRainLaserActiveTime = 0.8;

    // -------------------------------------------------------------------------
    // Red tracking laser
    // -------------------------------------------------------------------------

    inline constexpr double RedTrackingLaserLength     = 3200.0;
    inline constexpr double RedTrackingLaserActiveTime = 2.5;

    // -------------------------------------------------------------------------
    // Triple short parry laser
    // -------------------------------------------------------------------------

    inline constexpr double ShortParryLaserLength = 1200.0;

    // TripleShortLaserCount, TripleShortLaserInterval, TripleShortLaserDuration가
    // 이미 있으면 기존 값 유지.
}