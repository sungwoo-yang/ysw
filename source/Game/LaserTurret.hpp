#pragma once
#include "CS200/IRenderer2D.hpp"
#include "CS200/RGBA.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Laser.hpp"
#include <array>
#include <deque>
#include <vector>

class LaserTurret : public Laser {
public:
    LaserTurret(Math::vec2 pos, Math::vec2 dir, double interval, Player* player);

    void Update(double dt) override;
    void Draw(const Math::TransformationMatrix& camera_matrix) override;

    GameObjectTypes Type()     override { return GameObjectTypes::LaserTurret; }
    std::string     TypeName() override { return "LaserTurret"; }

    bool CanCollideWith(GameObjectTypes) override { return false; }
    void ResolveCollision(CS230::GameObject*) override {}

    // Bash interface — targets first active unbashed bullet
    bool       IsBulletActive()   const;
    bool       IsBulletBashed()   const;
    Math::vec2 GetBulletPosition() const;
    Math::vec2 GetBulletDirection() const;
    void       BashBullet(Math::vec2 newDir);

private:
    static constexpr double BULLET_SPEED   = 650.0;
    static constexpr double HIT_RADIUS     = 16.0;
    static constexpr double MAX_RANGE      = 5000.0;
    static constexpr double DAMAGE_PER_SEC = 5.0;
    static constexpr double FADE_DURATION  = 0.18;
    static constexpr int    TRAIL_LEN      = 10;
    static constexpr int    MAX_BULLETS    = 4;

    struct BulletState {
        bool       active = false;
        bool       bashed = false;
        bool       dying  = false;
        double     t      = 0.0;   // distance traveled
        double     len    = 0.0;   // total path length
        float      alpha  = 1.0f;  // fade-out [0,1]
        Math::vec2 startPos;
        Math::vec2 dir;
        std::vector<Math::vec2>  path;
        std::deque<Math::vec2>   trail;
    };

    std::array<BulletState, MAX_BULLETS> _bullets;

    double     fireInterval;
    double     fireTimer;
    Math::vec2 originalDirection;

    bool IsPlayerInPath() const;  // true if player is in the laser's line of fire

    void FireNewBullet();
    void UpdateBullet(BulletState& b, double dt);
    void DrawBullet(CS200::IRenderer2D& r, const BulletState& b) const;
    void DrawStar(CS200::IRenderer2D& r, Math::vec2 pos, double size,
                  CS200::RGBA col, double pulse) const;
};
