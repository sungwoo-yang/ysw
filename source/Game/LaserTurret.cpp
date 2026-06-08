#include "LaserTurret.hpp"
#include "FallingBlock.hpp"
#include "LaserCutRope.hpp"
#include "Player.hpp"

#include "CS200/IRenderer2D.hpp"
#include "Engine/Collision.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameObjectManager.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Matrix.hpp"
#include <algorithm>
#include <cmath>

// ──────────────────────────────────────────────────────────────────────────────

LaserTurret::LaserTurret(Math::vec2 pos, Math::vec2 dir, double interval, Player* player)
    : Laser(pos, dir, player),
      fireInterval     (interval > 0.1 ? interval : 0.1),
      fireTimer        (interval > 0.1 ? interval : 0.1),
      originalDirection(dir.Normalize())
{
    color = 0xFFCC44FF;
    SetIsActive(false);
}

// ──────────────────────────────────────────────────────────────────────────────
// Bash interface
// ──────────────────────────────────────────────────────────────────────────────

bool LaserTurret::IsBulletActive() const
{
    for (const auto& b : _bullets)
        if (b.active && !b.bashed) return true;
    return false;
}

bool LaserTurret::IsBulletBashed() const
{
    // Returns true when there is NO unbashed active bullet to target
    return !IsBulletActive();
}

Math::vec2 LaserTurret::GetBulletPosition() const
{
    for (const auto& b : _bullets)
        if (b.active && !b.bashed && !b.dying && b.path.size() >= 2)
            return b.path[0] + b.dir * b.t;
    return GetPosition();
}

Math::vec2 LaserTurret::GetBulletDirection() const
{
    for (const auto& b : _bullets)
        if (b.active && !b.bashed) return b.dir;
    return originalDirection;
}

void LaserTurret::BashBullet(Math::vec2 newDir)
{
    for (auto& b : _bullets)
    {
        if (!b.active || b.bashed || b.dying || b.path.size() < 2) continue;

        const Math::vec2 curPos = b.path[0] + b.dir * b.t;
        b.dir    = newDir.Normalize();
        b.bashed = true;

        // Recalculate path from current bullet position in new direction
        startPos  = curPos;
        direction = b.dir;
        CalculatePath(0, MAX_RANGE);

        b.path = pathPoints;
        b.t    = 0.0;
        b.len  = (pathPoints.size() >= 2)
               ? (pathPoints.back() - pathPoints[0]).Length()
               : 0.0;
        b.trail.clear();
        b.trail.push_back(curPos);
        return;
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Fire
// ──────────────────────────────────────────────────────────────────────────────

void LaserTurret::FireNewBullet()
{
    // Find a free slot (not active, not fading)
    BulletState* slot = nullptr;
    for (auto& b : _bullets)
    {
        if (!b.active && !b.dying) { slot = &b; break; }
    }
    if (!slot) return; // all slots full, skip this shot

    startPos  = GetPosition();
    direction = originalDirection;
    CalculatePath(0, MAX_RANGE);

    if (pathPoints.size() < 2) return;

    slot->startPos = GetPosition();
    slot->dir      = originalDirection;
    slot->path     = pathPoints;
    slot->len      = (pathPoints.back() - pathPoints[0]).Length();
    slot->t        = 0.0;
    slot->alpha    = 1.0f;
    slot->bashed   = false;
    slot->dying    = false;
    slot->active   = true;
    slot->trail.clear();
    slot->trail.push_back(pathPoints[0]);
    SetIsActive(true);
}

// ──────────────────────────────────────────────────────────────────────────────
// Per-bullet update
// ──────────────────────────────────────────────────────────────────────────────

void LaserTurret::UpdateBullet(BulletState& b, double dt)
{
    // Fade-out dying bullet
    if (b.dying)
    {
        b.alpha -= static_cast<float>(dt / FADE_DURATION);
        if (b.alpha <= 0.0f)
        {
            b.active = b.dying = false;
            b.trail.clear();
        }
        return;
    }

    if (!b.active) return;

    b.t += BULLET_SPEED * dt;
    const Math::vec2 pos = b.path[0] + b.dir * b.t;

    b.trail.push_back(pos);
    if (static_cast<int>(b.trail.size()) > TRAIL_LEN)
        b.trail.pop_front();

    // ── Player hit ──────────────────────────────────────────────────────────
    if (player != nullptr)
    {
        if ((player->GetPosition() - pos).LengthSquared() <= HIT_RADIUS * HIT_RADIUS)
        {
            player->ApplyLaserDamage(1.0); // 1 HP per hit (invincibility timer prevents rapid hits)
            b.dying = true; b.alpha = 1.0f;
            return;
        }
    }

    // ── World-object hits (FallingBlock blocks, LaserCutRope cuts) ──────────
    auto* gom = Engine::GetGameStateManager().GetGSComponent<CS230::GameObjectManager>();
    if (gom)
    {
        for (auto* obj : gom->GetObjects())
        {
            if (obj->Type() == GameObjectTypes::FallingBlock)
            {
                auto* col = obj->GetGOComponent<CS230::RectCollision>();
                if (!col) continue;
                const Math::rect box = col->WorldBoundary();
                if (pos.x >= box.Left() && pos.x <= box.Right() &&
                    pos.y >= box.Bottom() && pos.y <= box.Top())
                {
                    b.dying = true; b.alpha = 1.0f;
                    return;
                }
            }
            else if (obj->Type() == GameObjectTypes::LaserCutRope)
            {
                auto* rope = static_cast<LaserCutRope*>(obj);
                if (rope->IsCut()) continue;
                auto* col = rope->GetGOComponent<CS230::RectCollision>();
                if (!col) continue;
                const Math::rect box = col->WorldBoundary();
                if (pos.x >= box.Left() && pos.x <= box.Right() &&
                    pos.y >= box.Bottom() && pos.y <= box.Top())
                {
                    rope->Cut(); // bullet keeps going through the rope
                }
            }
        }
    }

    // ── Reached end of pre-calculated path → start fade ────────────────────
    if (b.active && b.t >= b.len)
    {
        b.dying = true;
        b.alpha = 1.0f;
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Update
// ──────────────────────────────────────────────────────────────────────────────

bool LaserTurret::IsPlayerInPath() const
{
    if (!player) return false;

    const Math::vec2 origin    = GetPosition();
    const Math::vec2 dir       = originalDirection; // already normalized
    const Math::vec2 playerPos = player->GetPosition();

    // Project player onto the ray
    const Math::vec2 toPlayer = playerPos - origin;
    const double proj = toPlayer.x * dir.x + toPlayer.y * dir.y;

    // Must be in front of turret and within range
    if (proj < 0.0 || proj > MAX_RANGE) return false;

    // Perpendicular distance from ray
    const Math::vec2 closest = { origin.x + dir.x * proj, origin.y + dir.y * proj };
    const double dist = std::sqrt(
        (playerPos.x - closest.x) * (playerPos.x - closest.x) +
        (playerPos.y - closest.y) * (playerPos.y - closest.y));

    // Player bounding box ≈ 40×80, use 55px threshold
    return dist <= 55.0;
}

void LaserTurret::Update(double dt)
{
    fireTimer -= dt;

    for (auto& b : _bullets)
        UpdateBullet(b, dt);

    if (fireTimer <= 0.0)
    {
        fireTimer = fireInterval;
        if (IsPlayerInPath())
            FireNewBullet();
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Rendering helpers
// ──────────────────────────────────────────────────────────────────────────────

void LaserTurret::DrawStar(CS200::IRenderer2D& r, Math::vec2 pos, double sz,
                            CS200::RGBA col, double pulse) const
{
    const double s = sz * pulse;
    const Math::vec2 T = { pos.x,     pos.y + s };
    const Math::vec2 R = { pos.x + s, pos.y     };
    const Math::vec2 B = { pos.x,     pos.y - s };
    const Math::vec2 L = { pos.x - s, pos.y     };
    r.DrawLine(T, R, col, 2.5);
    r.DrawLine(R, B, col, 2.5);
    r.DrawLine(B, L, col, 2.5);
    r.DrawLine(L, T, col, 2.5);

    const double h = s * 0.55;
    r.DrawLine({ pos.x + h, pos.y + h }, { pos.x + h, pos.y - h }, col, 1.5);
    r.DrawLine({ pos.x + h, pos.y - h }, { pos.x - h, pos.y - h }, col, 1.5);
    r.DrawLine({ pos.x - h, pos.y - h }, { pos.x - h, pos.y + h }, col, 1.5);
    r.DrawLine({ pos.x - h, pos.y + h }, { pos.x + h, pos.y + h }, col, 1.5);

    const auto aura = Math::TranslationMatrix(pos) * Math::ScaleMatrix(Math::vec2{ s * 3.2, s * 3.2 });
    r.DrawCircle(aura, (col & 0xFFFFFF00u) | 0x12u, CS200::CLEAR, 0.0);
    const auto glow = Math::TranslationMatrix(pos) * Math::ScaleMatrix(Math::vec2{ s * 1.5, s * 1.5 });
    r.DrawCircle(glow, (col & 0xFFFFFF00u) | 0x30u, CS200::CLEAR, 0.0);
    const auto core = Math::TranslationMatrix(pos) * Math::ScaleMatrix(Math::vec2{ s * 0.35, s * 0.35 });
    r.DrawCircle(core, CS200::WHITE, CS200::CLEAR, 0.0);
}

void LaserTurret::DrawBullet(CS200::IRenderer2D& r, const BulletState& b) const
{
    if (b.trail.empty()) return;

    const float a = b.dying ? b.alpha : 1.0f;

    // Trail glow
    const int n = static_cast<int>(b.trail.size());
    for (int i = 0; i < n; ++i)
    {
        const float  frac   = static_cast<float>(i + 1) / static_cast<float>(n);
        const double radius = 5.0 + frac * 8.0;
        const auto   ca = (color & 0xFFFFFF00u)
                        | static_cast<CS200::RGBA>(frac * 0.5f * a * 255.0f);
        const auto   mat = Math::TranslationMatrix(b.trail[i])
                         * Math::ScaleMatrix(Math::vec2{ radius * 2.0, radius * 2.0 });
        r.DrawCircle(mat, ca, CS200::CLEAR, 0.0);
    }

    // Core orb at bullet tip
    const Math::vec2 tip = b.trail.back();
    const double     sz  = 24.0 * a;
    const auto outerMat  = Math::TranslationMatrix(tip) * Math::ScaleMatrix(Math::vec2{ sz, sz });
    r.DrawCircle(outerMat, (color & 0xFFFFFF00u) | static_cast<CS200::RGBA>(0x55 * a),
                 CS200::CLEAR, 0.0);
    const auto innerMat  = Math::TranslationMatrix(tip) * Math::ScaleMatrix(Math::vec2{ 8.0, 8.0 });
    r.DrawCircle(innerMat,
                 CS200::WHITE & (0xFFFFFF00u | static_cast<CS200::RGBA>(a * 255.0f)),
                 CS200::CLEAR, 0.0);
}

void LaserTurret::Draw(const Math::TransformationMatrix& /*camera_matrix*/)
{
    auto&        r     = Engine::GetRenderer2D();
    const double time  = Engine::GetWindowEnvironment().ElapsedTime;
    const double pulse = 0.82 + std::sin(time * 2.8) * 0.18;

    DrawStar(r, GetPosition(), 30.0, color, pulse);

    for (const auto& b : _bullets)
        if (b.active || b.dying)
            DrawBullet(r, b);
}
