#pragma once

#include "CS200/RGBA.hpp"
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/Rect.hpp"
#include "Engine/Vec2.hpp"

#include <deque>
#include <vector>

class Player;
class TargetStar;

class SimpleBossStar : public CS230::GameObject
{
public:
    SimpleBossStar(Math::vec2 pos, Math::rect bossRoom, Player* player, std::vector<TargetStar*> targets);

    void Update(double dt) override;
    void Draw(const Math::TransformationMatrix& camera_matrix) override;

    GameObjectTypes Type() override
    {
        return GameObjectTypes::Star;
    }

    std::string TypeName() override
    {
        return "SimpleBossStar";
    }

    bool IsCleared() const;

    const Math::rect& GetRoomBounds() const
    {
        return roomBounds;
    }

    bool       HasBashableShot() const;
    Math::vec2 GetNearestBashableShotPosition(Math::vec2 from) const;
    Math::vec2 GetNearestBashableShotDirection(Math::vec2 from) const;
    bool       BashNearestShot(Math::vec2 from, Math::vec2 newDir, double maxDistanceSquared);

private:
    enum class State
    {
        WaitingForPlayer,
        Warning,
        TrackingLaser,
        Cooldown,
        Cleared
    };

    struct Shot
    {
        bool active = false;
        bool bashed = false;
        bool dying  = false;

        Math::vec2 pos;
        Math::vec2 dir;

        double travelled = 0.0;
        float  alpha     = 1.0f;

        std::deque<Math::vec2> trail;
    };

    static constexpr double WarningDuration  = 2.5;
    static constexpr double TrackingDuration = 4.0;
    static constexpr double CooldownDuration = 2.5;

    static constexpr double TrackingLaserLength  = 5000.0;
    static constexpr double TrackingDamageRadius = 30.0;
    static constexpr double TrackingAimSpeed     = 300.0;

    static constexpr double ShotSpeed        = 650.0;
    static constexpr double ShotHitRadius    = 16.0;
    static constexpr double ShotMaxRange     = 5000.0;
    static constexpr double ShotFadeDuration = 0.18;
    static constexpr int    ShotTrailLength  = 10;

    Player*                  player = nullptr;
    std::vector<TargetStar*> targetStars;

    Math::rect roomBounds;
    State      state = State::WaitingForPlayer;

    double     timer             = 0.0;
    Math::vec2 attackDir         = { 1.0, 0.0 };
    Math::vec2 trackingTargetPos = { 0.0, 0.0 };

    std::vector<Shot> shots;

    bool IsPlayerInRoom() const;
    bool AreAllTargetsHit() const;

    void StartWarning();
    void StartTrackingLaser();
    void MoveTrackingTarget(double dt);
    void FireBossShot();
    void StartCooldown();
    void ClearBoss();

    void UpdateState(double dt);
    void UpdateShots(double dt);

    void DrawBossBody();
    void DrawWarningLine();
    void DrawTrackingLaser();
    void DrawShots();

    static Math::vec2 SafeNormalize(Math::vec2 v, Math::vec2 fallback);
    static double     DistanceToSegmentSquared(Math::vec2 p, Math::vec2 a, Math::vec2 b);
};