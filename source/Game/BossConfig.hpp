#pragma once

namespace Boss::Config
{
    // -------------------------------------------------------------------------
    // Arena
    // -------------------------------------------------------------------------

    inline constexpr float ArenaLeftX   = 0.0f;
    inline constexpr float ArenaRightX  = 3000.0f;
    inline constexpr float ArenaTopY    = 1440.0f;
    inline constexpr float ArenaCenterX = 1500.0f;
    inline constexpr float ArenaCenterY = 250.0f;

    // Ground line used for yellow laser/light-orb intersection.
    // Adjust this later to match the actual boss arena floor.
    inline constexpr float GroundY = 0.0f;

    // -------------------------------------------------------------------------
    // Shield Energy
    // -------------------------------------------------------------------------

    inline constexpr float MaxShieldEnergy      = 100.0f;
    inline constexpr float RequiredChargeEnergy = 100.0f;

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

    inline constexpr float ChargeHoldTime = 0.8f;

    // -------------------------------------------------------------------------
    // Pattern Weights
    // Adjust these values later to rebalance random pattern frequency.
    // -------------------------------------------------------------------------

    inline constexpr int FourWayRotatingYellowLaserWeight = 20;
    inline constexpr int CrossWallYellowLaserWeight       = 20;
    inline constexpr int OrangeLaserRainWeight            = 20;
    inline constexpr int RedTrackingLaserWeight           = 20;
    inline constexpr int TripleShortParryLaserWeight      = 20;

    // -------------------------------------------------------------------------
    // Pattern Timing
    // -------------------------------------------------------------------------

    inline constexpr float PatternInterval = 3.0f;

    // This is the warning telegraph time currently used by BossPatternController.
    inline constexpr double BossLaserWarningTime = 2.0;

    // Keep this alias only if older code still references Config::WarningTime.
    inline constexpr float WarningTime = static_cast<float>(BossLaserWarningTime);

    // -------------------------------------------------------------------------
    // Boss Laser Common
    // -------------------------------------------------------------------------

    inline constexpr double BossLaserDefaultLength = 3200.0;

    // -------------------------------------------------------------------------
    // Four-way Rotating Yellow Laser
    // -------------------------------------------------------------------------

    inline constexpr double FourWayLaserLength     = 3200.0;
    inline constexpr double FourWayLaserActiveTime = 4.0;

    // Unit: degrees per second.
    inline constexpr float FourWayRotateSpeed = 20.0f;

    // Reserved for later if the pattern needs a limited sweep angle.
    inline constexpr float FourWayRotateMinAngle = 90.0f;
    inline constexpr float FourWayRotateMaxAngle = 180.0f;

    // -------------------------------------------------------------------------
    // Cross Wall Yellow Laser
    // -------------------------------------------------------------------------

    inline constexpr double CrossWallLaserLength     = 5000.0;
    inline constexpr double CrossWallLaserActiveTime = 1.2;

    // -------------------------------------------------------------------------
    // Orange Laser Rain
    // -------------------------------------------------------------------------

    inline constexpr int OrangeRainLaserCount = 8;

    inline constexpr double OrangeRainLaserLength     = 3000.0;
    inline constexpr double OrangeRainLaserActiveTime = 0.8;

    // -------------------------------------------------------------------------
    // Red Tracking Laser
    // -------------------------------------------------------------------------

    inline constexpr double RedTrackingLaserLength     = 2000.0;
    inline constexpr double RedTrackingLaserActiveTime = 2.5;
    inline constexpr float  RedTrackingRotateSpeed     = 10.0f;

    // -------------------------------------------------------------------------
    // Triple Short Parry Laser
    // -------------------------------------------------------------------------

    inline constexpr int TripleShortLaserCount = 3;

    inline constexpr float TripleShortLaserInterval = 0.4f;
    inline constexpr float TripleShortLaserDuration = 0.8f;

    inline constexpr double ShortParryLaserLength     = 160.0;
    inline constexpr double ShortParryLaserSpeed      = 900.0;
    inline constexpr double ShortParryLaserRange      = 3000.0;
    inline constexpr double ShortParryLaserActiveTime = ShortParryLaserRange / ShortParryLaserSpeed;

    // -------------------------------------------------------------------------
    // Charge Shot
    // -------------------------------------------------------------------------

    inline constexpr double ChargeShotLength       = 3200.0;
    inline constexpr double ChargeShotBeamDuration = 0.25;
    inline constexpr double ChargeShotWidth        = 18.0;
    inline constexpr double ChargeShotHitRadius    = 48.0;
}