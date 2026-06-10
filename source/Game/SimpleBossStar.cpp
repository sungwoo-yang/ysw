#include "SimpleBossStar.hpp"

#include "CS200/IRenderer2D.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Matrix.hpp"
#include "Player.hpp"
#include "TargetStar.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

SimpleBossStar::SimpleBossStar(Math::vec2 pos, Math::rect bossRoom, Player* in_player, std::vector<TargetStar*> targets)
    : CS230::GameObject(pos), player(in_player), targetStars(std::move(targets)), roomBounds(bossRoom)
{
}

void SimpleBossStar::Update(double dt)
{
    UpdateShots(dt);

    if (state != State::Cleared)
    {
        if (AreAllTargetsHit())
        {
            ClearBoss();
        }
        else
        {
            UpdateState(dt);
        }
    }

    CS230::GameObject::Update(dt);
}

void SimpleBossStar::Draw([[maybe_unused]] const Math::TransformationMatrix& camera_matrix)
{
    DrawBossBody();

    if (state == State::Warning)
    {
        DrawWarningLine();
    }
    else if (state == State::TrackingLaser)
    {
        DrawTrackingLaser();
    }

    DrawShots();
}

bool SimpleBossStar::IsCleared() const
{
    return state == State::Cleared;
}

bool SimpleBossStar::HasBashableShot() const
{
    for (const Shot& shot : shots)
    {
        if (shot.active && !shot.bashed && !shot.dying)
            return true;
    }

    return false;
}

Math::vec2 SimpleBossStar::GetNearestBashableShotPosition(Math::vec2 from) const
{
    Math::vec2 bestPos  = GetPosition();
    double     bestDist = std::numeric_limits<double>::max();

    for (const Shot& shot : shots)
    {
        if (!shot.active || shot.bashed || shot.dying)
            continue;

        const double d2 = (shot.pos - from).LengthSquared();
        if (d2 < bestDist)
        {
            bestDist = d2;
            bestPos  = shot.pos;
        }
    }

    return bestPos;
}

Math::vec2 SimpleBossStar::GetNearestBashableShotDirection(Math::vec2 from) const
{
    Math::vec2 bestDir  = attackDir;
    double     bestDist = std::numeric_limits<double>::max();

    for (const Shot& shot : shots)
    {
        if (!shot.active || shot.bashed || shot.dying)
            continue;

        const double d2 = (shot.pos - from).LengthSquared();
        if (d2 < bestDist)
        {
            bestDist = d2;
            bestDir  = shot.dir;
        }
    }

    return bestDir;
}

bool SimpleBossStar::BashNearestShot(Math::vec2 from, Math::vec2 newDir, double maxDistanceSquared)
{
    Shot*  bestShot = nullptr;
    double bestDist = maxDistanceSquared;

    for (Shot& shot : shots)
    {
        if (!shot.active || shot.bashed || shot.dying)
            continue;

        const double d2 = (shot.pos - from).LengthSquared();
        if (d2 <= bestDist)
        {
            bestDist = d2;
            bestShot = &shot;
        }
    }

    if (bestShot == nullptr)
        return false;

    bestShot->dir    = SafeNormalize(newDir, bestShot->dir);
    bestShot->bashed = true;
    bestShot->trail.clear();
    bestShot->trail.push_back(bestShot->pos);

    return true;
}

bool SimpleBossStar::IsPlayerInRoom() const
{
    if (player == nullptr)
        return false;

    const Math::vec2 p = player->GetPosition();

    return p.x >= roomBounds.Left() && p.x <= roomBounds.Right() && p.y >= roomBounds.Bottom() && p.y <= roomBounds.Top();
}

bool SimpleBossStar::AreAllTargetsHit() const
{
    if (targetStars.empty())
        return false;

    for (TargetStar* target : targetStars)
    {
        if (target == nullptr || !target->IsHit())
            return false;
    }

    return true;
}

void SimpleBossStar::StartWarning()
{
    state = State::Warning;
    timer = WarningDuration;

    if (player != nullptr)
    {
        trackingTargetPos = player->GetPosition();
        attackDir         = SafeNormalize(trackingTargetPos - GetPosition(), attackDir);
    }
}

void SimpleBossStar::StartTrackingLaser()
{
    state = State::TrackingLaser;
    timer = TrackingDuration;

    if (player != nullptr)
    {
        trackingTargetPos = player->GetPosition();
        attackDir         = SafeNormalize(trackingTargetPos - GetPosition(), attackDir);
    }
}

void SimpleBossStar::MoveTrackingTarget(double dt)
{
    if (player == nullptr)
        return;

    const Math::vec2 desiredTarget = player->GetPosition();
    const Math::vec2 toDesired     = desiredTarget - trackingTargetPos;
    const double     distance      = toDesired.Length();

    if (distance <= 0.001)
    {
        trackingTargetPos = desiredTarget;
    }
    else
    {
        const double maxStep = TrackingAimSpeed * dt;
        const double step    = std::min(maxStep, distance);
        trackingTargetPos += toDesired * (step / distance);
    }

    attackDir = SafeNormalize(trackingTargetPos - GetPosition(), attackDir);
}

void SimpleBossStar::FireBossShot()
{
    Shot shot;
    shot.active    = true;
    shot.bashed    = false;
    shot.dying     = false;
    shot.pos       = GetPosition();
    shot.dir       = attackDir;
    shot.travelled = 0.0;
    shot.alpha     = 1.0f;
    shot.trail.clear();
    shot.trail.push_back(shot.pos);

    shots.push_back(std::move(shot));
}

void SimpleBossStar::StartCooldown()
{
    state = State::Cooldown;
    timer = CooldownDuration;
}

void SimpleBossStar::ClearBoss()
{
    state = State::Cleared;
    timer = 0.0;

    for (Shot& shot : shots)
    {
        shot.dying = true;
        shot.alpha = 1.0f;
    }
}

void SimpleBossStar::UpdateState(double dt)
{
    if (player == nullptr)
        return;

    switch (state)
    {
        case State::WaitingForPlayer:
            if (IsPlayerInRoom())
            {
                StartWarning();
            }
            break;

        case State::Warning:
            trackingTargetPos = player->GetPosition();
            attackDir         = SafeNormalize(trackingTargetPos - GetPosition(), attackDir);

            timer -= dt;
            if (timer <= 0.0)
            {
                StartTrackingLaser();
            }
            break;

        case State::TrackingLaser:
            {
                MoveTrackingTarget(dt);

                const Math::vec2 start = GetPosition();
                const Math::vec2 end   = start + attackDir * TrackingLaserLength;

                if (DistanceToSegmentSquared(player->GetPosition(), start, end) <= TrackingDamageRadius * TrackingDamageRadius)
                {
                    player->ApplyLaserDamage(1.0);
                }

                timer -= dt;
                if (timer <= 0.0)
                {
                    FireBossShot();
                    StartCooldown();
                }
                break;
            }

        case State::Cooldown:
            timer -= dt;
            if (timer <= 0.0)
            {
                StartWarning();
            }
            break;

        case State::Cleared: break;
    }
}

void SimpleBossStar::UpdateShots(double dt)
{
    for (Shot& shot : shots)
    {
        if (shot.dying)
        {
            shot.alpha -= static_cast<float>(dt / ShotFadeDuration);
            if (shot.alpha <= 0.0f)
            {
                shot.active = false;
                shot.dying  = false;
                shot.trail.clear();
            }
            continue;
        }

        if (!shot.active)
            continue;

        const Math::vec2 step = shot.dir * (ShotSpeed * dt);
        shot.pos += step;
        shot.travelled += step.Length();

        shot.trail.push_back(shot.pos);
        if (static_cast<int>(shot.trail.size()) > ShotTrailLength)
            shot.trail.pop_front();

        if (!shot.bashed && player != nullptr)
        {
            if ((player->GetPosition() - shot.pos).LengthSquared() <= ShotHitRadius * ShotHitRadius)
            {
                player->ApplyLaserDamage(1.0);
                shot.dying = true;
                shot.alpha = 1.0f;
                continue;
            }
        }

        if (shot.bashed)
        {
            for (TargetStar* target : targetStars)
            {
                if (target == nullptr || target->IsHit())
                    continue;

                const double hitRadius = target->GetRadius() + ShotHitRadius;
                if ((target->GetPosition() - shot.pos).LengthSquared() <= hitRadius * hitRadius)
                {
                    target->ActivateInstantly();
                    shot.dying = true;
                    shot.alpha = 1.0f;
                    break;
                }
            }
        }

        if (shot.travelled >= ShotMaxRange)
        {
            shot.dying = true;
            shot.alpha = 1.0f;
        }
    }

    shots.erase(std::remove_if(shots.begin(), shots.end(), [](const Shot& shot) { return !shot.active && !shot.dying; }), shots.end());
}

void SimpleBossStar::DrawBossBody()
{
    auto& renderer = Engine::GetRenderer2D();

    const double time  = Engine::GetWindowEnvironment().ElapsedTime;
    const double pulse = 0.85 + std::sin(time * 3.0) * 0.15;

    const auto aura = Math::TranslationMatrix(GetPosition()) * Math::ScaleMatrix({ 150.0 * pulse, 150.0 * pulse });

    const auto body = Math::TranslationMatrix(GetPosition()) * Math::ScaleMatrix({ 70.0 * pulse, 70.0 * pulse });

    renderer.DrawCircle(aura, 0x5522FF22u, CS200::CLEAR, 0.0);
    renderer.DrawCircle(body, 0x5522FFFFu, 0xAA88FFFFu, 2.5);
}

void SimpleBossStar::DrawWarningLine()
{
    auto& renderer = Engine::GetRenderer2D();

    const Math::vec2 start = GetPosition();
    const Math::vec2 end   = start + attackDir * TrackingLaserLength;

    renderer.DrawLine(start, end, 0xFF444488u, 3.0);
    renderer.DrawLine(start, end, 0xFFFFFF66u, 1.0);
}

void SimpleBossStar::DrawTrackingLaser()
{
    auto& renderer = Engine::GetRenderer2D();

    const Math::vec2 start = GetPosition();
    const Math::vec2 end   = start + attackDir * TrackingLaserLength;

    renderer.DrawLine(start, end, 0xFFFF0044u, 26.0);
    renderer.DrawLine(start, end, 0xFFFF00AAu, 10.0);
    renderer.DrawLine(start, end, 0xFFFFFFFFu, 2.5);
}

void SimpleBossStar::DrawShots()
{
    auto& renderer = Engine::GetRenderer2D();

    for (const Shot& shot : shots)
    {
        if (!shot.active && !shot.dying)
            continue;

        const float a = shot.dying ? shot.alpha : 1.0f;

        const int n = static_cast<int>(shot.trail.size());
        for (int i = 0; i < n; ++i)
        {
            const float  frac   = static_cast<float>(i + 1) / static_cast<float>(n);
            const double radius = 5.0 + frac * 8.0;

            const auto color = 0xFFCC4400u | static_cast<CS200::RGBA>(frac * 0.5f * a * 255.0f);

            const auto mat = Math::TranslationMatrix(shot.trail[i]) * Math::ScaleMatrix({ radius * 2.0, radius * 2.0 });

            renderer.DrawCircle(mat, color, CS200::CLEAR, 0.0);
        }

        const double coreSize = 24.0 * a;
        const auto   outer    = Math::TranslationMatrix(shot.pos) * Math::ScaleMatrix({ coreSize, coreSize });

        const auto inner = Math::TranslationMatrix(shot.pos) * Math::ScaleMatrix({ 8.0, 8.0 });

        renderer.DrawCircle(outer, 0xFFCC4455u, CS200::CLEAR, 0.0);
        renderer.DrawCircle(inner, 0xFFFFFFFFu, CS200::CLEAR, 0.0);
    }
}

Math::vec2 SimpleBossStar::SafeNormalize(Math::vec2 v, Math::vec2 fallback)
{
    if (v.LengthSquared() <= 0.01)
        return fallback;

    return v.Normalize();
}

double SimpleBossStar::DistanceToSegmentSquared(Math::vec2 p, Math::vec2 a, Math::vec2 b)
{
    const Math::vec2 ab     = b - a;
    const double     abLen2 = ab.LengthSquared();

    if (abLen2 <= 0.001)
        return (p - a).LengthSquared();

    const double     t       = std::clamp((p - a).Dot(ab) / abLen2, 0.0, 1.0);
    const Math::vec2 closest = a + ab * t;

    return (p - closest).LengthSquared();
}