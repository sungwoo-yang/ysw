#pragma once
#include "Engine/GameObjectTypes.hpp" // 누락 방지!
#include "Star.hpp"

class BossStar : public Star
{
public:
    BossStar(Math::vec2 pos, Player* player, const std::vector<TargetStar*>& targets);

    void Update(double dt) override;

    // Star 부모 클래스의 순수 가상 함수 구현
    void        OnWarningComplete() override;
    CS200::RGBA GetTelegraphColor() const override;

    GameObjectTypes Type() override
    {
        return GameObjectTypes::Boss;
    }

    std::string TypeName() override
    {
        return "BossStar";
    }

private:
    int attackStep; // 0: Red, 1: Red, 2: Yellow 순서를 기억하는 카운터

    const double parryWindowTime = 0.5; // 빨간색 레이저 발사 직전 패링 가능 시간
};