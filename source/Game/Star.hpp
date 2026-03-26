#pragma once
#include "CS200/RGBA.hpp"
#include "Engine/GameObject.hpp"
#include <vector>

class Player;
class TargetStar;

class Star : public CS230::GameObject
{
public:
    Star(Math::vec2 pos, Player* player, const std::vector<TargetStar*>& targets);
    virtual ~Star() = default;

    void Update(double dt) override;
    void Draw(const Math::TransformationMatrix& camera_matrix) override;

    // 자식 클래스(Puzzle, Hostile, Boss)에서 구현할 핵심 로직
    virtual void        OnWarningComplete()       = 0; // Warning 타이머가 끝났을 때 발사 로직
    virtual CS200::RGBA GetTelegraphColor() const = 0; // 예고선 색상 (White, Yellow, Red)

protected:
    enum class State
    {
        Idle,
        Warning,
        Cooldown
    };
    State  currentState;
    double timer;

    Player*                  player;
    std::vector<TargetStar*> targets;

    // 별마다 다를 수 있는 파라미터들
    double detectionRadius  = 600.0;
    double chaseRadius      = 900.0;
    double warningDuration  = 2.0;
    double cooldownDuration = 3.0;

    void HandleBasicAI(double dt); // 거리 체크 및 상태 전환 로직
    void DrawTrajectory();         // 공통 레이저 예고선 그리기 로직
};