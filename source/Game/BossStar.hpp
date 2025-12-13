#pragma once
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "CS200/RGBA.hpp"
#include <vector>

class Player;
class TargetStar;

class BossStar : public CS230::GameObject
{
public:
    BossStar(Math::vec2 position, Player* player, const std::vector<TargetStar*>& targets);

    void Update(double dt) override;
    void Draw(const Math::TransformationMatrix& camera_matrix) override;

    GameObjectTypes Type() override { return GameObjectTypes::Boss; }
    std::string TypeName() override { return "BossStar"; }

private:
    enum class State
    {
        Idle,
        Warning,
        Cooldown
    };

    enum class NextLaser
    {
        Red,
        Yellow
    };

    State currentState;
    NextLaser nextLaserType; // 다음에 쏠 레이저 종류

    Player* player;
    std::vector<TargetStar*> targets;

    double timer;
    
    // 보스 설정값
    const double detectionRadius  = 800.0; // 인식 범위 (일반 별보다 넓게)
    const double warningDuration  = 1.5;   // 경고 시간
    const double cooldownDuration = 2.0;   // 발사 후 대기 시간
    const double size             = 80.0;  // 크기 (일반 별의 2배)
};