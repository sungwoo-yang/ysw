#pragma once
#include "Engine/GameObjectTypes.hpp"
#include "Star.hpp"

class HostileStar : public Star
{
public:
    enum class StarType
    {
        Yellow, // 노란색: 지속 추격 레이저 발사
        Red     // 빨간색: 짧고 강력한 패링 전용 레이저 발사
    };

    HostileStar(Math::vec2 pos, Player* player, const std::vector<TargetStar*>& targets, StarType type);

    void Update(double dt) override;

    // Star 부모 클래스의 순수 가상 함수 구현
    void        OnWarningComplete() override;
    CS200::RGBA GetTelegraphColor() const override;

    GameObjectTypes Type() override
    {
        return GameObjectTypes::Star;
    }

    std::string TypeName() override
    {
        return "HostileStar";
    }

private:
    StarType     currentStarType;
    const double parryWindowTime = 0.5; // 빨간색 레이저 발사 직전 패링 가능 시간
};