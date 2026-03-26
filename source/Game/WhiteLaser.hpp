#pragma once
#include "Engine/GameObjectTypes.hpp"
#include "Laser.hpp"

// 데미지 없이 퍼즐 타겟(TargetStar)만 활성화하는 지속형 레이저입니다.
class WhiteLaser : public Laser
{
public:
    WhiteLaser(Math::vec2 in_startPos, Math::vec2 in_dir, Player* in_player, const std::vector<TargetStar*>& in_targets);

    void Update(double dt) override;

    // PuzzleStar가 회전하거나 이동할 때 레이저도 따라가도록 업데이트하는 함수
    void SetStartPos(Math::vec2 newPos)
    {
        startPos = newPos;
    }

    void SetDirection(Math::vec2 newDir)
    {
        direction = newDir.Normalize();
    }

    GameObjectTypes Type() override
    {
        return GameObjectTypes::Laser;
    }

    std::string TypeName() override
    {
        return "WhiteLaser";
    }

private:
    const int    maxBounces = 5;
    const double maxLength  = 3000.0;
    const double hitRadius  = 15.0; // 타겟 판정 범위
};