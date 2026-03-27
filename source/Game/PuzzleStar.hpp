#pragma once
#include "Engine/GameObjectTypes.hpp"
#include "Star.hpp"

class WhiteLaser;

class PuzzleStar : public Star
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
        Blink
    };

    PuzzleStar(Math::vec2 pos, Player* player, LaserType type, Pattern pattern, Math::vec2 initialDir);
    ~PuzzleStar() override;

    void Update(double dt) override;
    void OnWarningComplete() override;

    CS200::RGBA GetTelegraphColor() const override
    {
        return 0xFFFFFFFF;
    }

    GameObjectTypes Type() override
    {
        return GameObjectTypes::Star;
    }

    std::string TypeName() override
    {
        return "PuzzleStar";
    }

    void SetPattern(Pattern newPattern);
    void SetAimDirection(Math::vec2 newDir);

private:
    LaserType   currentType;
    Pattern     currentPattern;
    Math::vec2  aimDirection;
    WhiteLaser* myLaser;


    double rotationSpeed = 0.5;

    double       blinkTimer    = 0.0;
    const double blinkInterval = 2;
    bool         isLaserOn     = true;

    void SetLaserActive(bool active);
};