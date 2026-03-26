#pragma once
#include "Star.hpp"
#include "Engine/GameObjectTypes.hpp"

class WhiteLaser;

class PuzzleStar : public Star
{
public:
    enum class Pattern
    {
        Static,   // 고정된 방향으로 계속 발사
        Rotating, // 일정한 속도로 빙글빙글 회전
        Blink     // 일정 주기로 켜졌다 꺼짐
    };

    PuzzleStar(Math::vec2 pos, Player* player, const std::vector<TargetStar*>& targets, Pattern pattern, Math::vec2 initialDir);
    ~PuzzleStar() override;

    void Update(double dt) override;

    // Star 부모 클래스의 순수 가상 함수 구현 (사용하지 않지만 구현은 필요함)
    void OnWarningComplete() override
    {
    }

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

private:
    Pattern     currentPattern;
    Math::vec2  aimDirection;
    WhiteLaser* myLaser;

    // 회전 패턴용
    double rotationSpeed = 1.0; // 라디안/초

    // 깜빡임 패턴용
    double       blinkTimer    = 0.0;
    const double blinkInterval = 2.0; // 2초마다 On/Off
    bool         isLaserOn     = true;

    void SetLaserActive(bool active);
};