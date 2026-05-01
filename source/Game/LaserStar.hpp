#pragma once

#include "Engine/GameObjectTypes.hpp"
#include "Star.hpp"

class Laser;

class LaserStar : public Star
{
public:
    enum class LaserType
    {
        White,
        Yellow,
        Red
    };

    enum class Pattern
    {
        Static,
        Rotating,
        Blink,
        Tracking
    };

    enum class FireMode
    {
        Continuous,
        WarningShot
    };

    LaserStar(Math::vec2 pos, Player* player, LaserType type, Pattern pattern, Math::vec2 initialDir, FireMode mode = FireMode::Continuous);

    ~LaserStar() override;

    void Update(double dt) override;
    void OnWarningComplete() override;

    CS200::RGBA GetTelegraphColor() const override;
    CS200::RGBA GetBodyColor() const override;

    GameObjectTypes Type() override
    {
        return GameObjectTypes::Star;
    }

    std::string TypeName() override
    {
        return "LaserStar";
    }

    int GetMaxBounces() const override;

    void SetLaserType(LaserType newType);
    void SetPattern(Pattern newPattern);
    void SetFireMode(FireMode newMode);
    void SetAimDirection(Math::vec2 newDir);

private:
    LaserType currentType;
    Pattern   currentPattern;
    FireMode  currentFireMode;

    Math::vec2 aimDirection;

    // Continuous mode laser
    Laser* continuousLaser = nullptr;
    bool   isFirstFrame    = true;

    // WarningShot mode laser
    Laser*     activeShotLaser       = nullptr;
    double     firingTimer           = 0.0;
    double     currentFiringDuration = 0.0;
    Math::vec2 laserDirection;

    // Shared pattern values
    double rotationSpeed = 0.5;

    double       blinkTimer    = 0.0;
    const double blinkInterval = 2.0;
    bool         isLaserOn     = true;

    // Red warning-shot parry timing
    const double parryWindowTime = 0.3;

    void UpdateContinuous(double dt);
    void UpdateWarningShot(double dt);

    void CreateContinuousLaser();
    void DestroyContinuousLaser();

    Laser* CreateLaserObject(Math::vec2 startPos, Math::vec2 dir);
};