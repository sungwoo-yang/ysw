#include "LaserStar.hpp"

#include "Engine/Engine.hpp"
#include "Engine/GameObjectManager.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Logger.hpp"

#include "Laser.hpp"
#include "Player.hpp"
#include "RedLaser.hpp"
#include "Shield.hpp"
#include "WhiteLaser.hpp"
#include "YellowLaser.hpp"

#include <cmath>

namespace
{
    double NormalizeAngleDifference(double diff)
    {
        while (diff <= -PI)
        {
            diff += 2.0 * PI;
        }

        while (diff > PI)
        {
            diff -= 2.0 * PI;
        }

        return diff;
    }
}

LaserStar::LaserStar(
    Math::vec2 pos, Player* in_player, LaserType type, Pattern pattern, Math::vec2 initialDir, FireMode mode, MoveMode in_moveMode, Math::vec2 in_moveDir, double in_moveSpeed, double in_moveDistance)
    : Star(pos, in_player), currentType(type), currentPattern(pattern), currentFireMode(mode), aimDirection(initialDir.Normalize()), laserDirection(initialDir.Normalize()),
      currentMoveMode(in_moveMode), startPosition(pos), moveDirection(in_moveDir.Length() > 0.0 ? in_moveDir.Normalize() : Math::vec2{ 0.0, 0.0 }), moveSpeed(in_moveSpeed),
      moveDistance(in_moveDistance)
{
    if (currentFireMode == FireMode::WarningShot)
    {
        warningDuration = 4.0;

        if (currentType == LaserType::Yellow)
        {
            cooldownDuration = 0.0;
        }
        else
        {
            cooldownDuration = 4.0;
        }
    }
}

LaserStar::~LaserStar()
{
    DestroyContinuousLaser();

    if (activeShotLaser != nullptr)
    {
        activeShotLaser->Destroy();
        activeShotLaser = nullptr;
    }
}

void LaserStar::Update(double dt)
{
    UpdateMovement(dt);

    if (currentFireMode == FireMode::Continuous)
    {
        UpdateContinuous(dt);
    }
    else
    {
        UpdateWarningShot(dt);
    }

    CS230::GameObject::Update(dt);
}

void LaserStar::UpdateContinuous(double dt)
{
    if (isFirstFrame)
    {
        CreateContinuousLaser();

        if (currentPattern == Pattern::Blink && !isLaserOn && continuousLaser != nullptr)
        {
            continuousLaser->SetIsActive(false);
            continuousLaser->SetVisible(false);
        }

        isFirstFrame = false;
    }

    if (continuousLaser == nullptr)
    {
        return;
    }

    switch (currentPattern)
    {
        case Pattern::Static:
            {
                continuousLaser->SetStartPos(GetPosition());
                continuousLaser->SetDirection(aimDirection);
                break;
            }

        case Pattern::Rotating:
            {
                double currentAngle = std::atan2(aimDirection.y, aimDirection.x);
                currentAngle += rotationSpeed * dt;

                aimDirection = Math::vec2{ std::cos(currentAngle), std::sin(currentAngle) };

                continuousLaser->SetStartPos(GetPosition());
                continuousLaser->SetDirection(aimDirection);
                break;
            }

        case Pattern::Blink:
            {
                blinkTimer += dt;

                if (blinkTimer >= blinkInterval)
                {
                    isLaserOn  = !isLaserOn;
                    blinkTimer = 0.0;

                    continuousLaser->SetIsActive(isLaserOn);
                    continuousLaser->SetVisible(isLaserOn);
                }

                continuousLaser->SetStartPos(GetPosition());
                continuousLaser->SetDirection(aimDirection);
                break;
            }

        case Pattern::Tracking:
            {
                if (player != nullptr)
                {
                    aimDirection = (player->GetPosition() - GetPosition()).Normalize();
                }

                continuousLaser->SetStartPos(GetPosition());
                continuousLaser->SetDirection(aimDirection);
                break;
            }
    }
}

void LaserStar::UpdateWarningShot(double dt)
{
    if (currentType == LaserType::Red && currentState == State::Warning)
    {
        Shield* shield = player ? player->GetShield() : nullptr;

        if (shield != nullptr)
        {
            if (timer <= parryWindowTime)
            {
                shield->SetParryWindowActive(true);
            }
            else
            {
                shield->SetParryWindowActive(false);
            }
        }
    }

    if (activeShotLaser != nullptr)
    {
        firingTimer += dt;

        if (firingTimer >= currentFiringDuration)
        {
            activeShotLaser->Destroy();
            activeShotLaser = nullptr;
        }
        else
        {
            if (player != nullptr && currentType == LaserType::Yellow)
            {
                Math::vec2 targetDir    = (player->GetPosition() - GetPosition()).Normalize();
                double     currentAngle = std::atan2(laserDirection.y, laserDirection.x);
                double     targetAngle  = std::atan2(targetDir.y, targetDir.x);

                double diff = NormalizeAngleDifference(targetAngle - currentAngle);

                double maxRotate = rotationSpeed * dt;
                currentAngle += (std::abs(diff) < maxRotate) ? diff : ((diff > 0.0) ? maxRotate : -maxRotate);

                laserDirection = Math::vec2{ std::cos(currentAngle), std::sin(currentAngle) };
                activeShotLaser->SetDirection(laserDirection);
            }

            activeShotLaser->SetStartPos(GetPosition());
        }
    }
    else
    {
        HandleBasicAI(dt);
    }
}

void LaserStar::UpdateMovement(double dt)
{
    if (currentMoveMode == MoveMode::None || moveComplete)
    {
        return;
    }

    if (currentMoveMode == MoveMode::Linear)
    {
        double step = moveSpeed * dt;

        if (moveDistance > 0.0 && movedDistance + step >= moveDistance)
        {
            step         = moveDistance - movedDistance;
            moveComplete = true;
        }

        if (step > 0.0)
        {
            UpdatePosition(moveDirection * step);
            movedDistance += step;
        }
    }
}

void LaserStar::OnWarningComplete()
{
    if (player == nullptr)
    {
        return;
    }

    Math::vec2 dir = (player->GetPosition() - GetPosition()).Normalize();
    auto       gom = Engine::GetGameStateManager().GetGSComponent<CS230::GameObjectManager>();

    if (gom == nullptr)
    {
        return;
    }

    if (currentType == LaserType::Red)
    {
        bool    parrySuccess = false;
        Shield* shield       = player->GetShield();

        if (shield && shield->ConsumeParryState())
        {
            shield->HandleHit(true);
            Engine::GetLogger().LogEvent("LaserStar Red Laser Perfect Parry!");
            parrySuccess = true;
        }

        if (shield)
        {
            shield->SetParryWindowActive(false);
        }

        RedLaser* rLaser = new RedLaser(GetPosition(), dir, player);

        if (parrySuccess)
        {
            rLaser->SetParried(true);
        }

        gom->Add(rLaser);

        activeShotLaser       = rLaser;
        firingTimer           = 0.0;
        currentFiringDuration = 0.2;
        laserDirection        = dir;
    }
    else if (currentType == LaserType::Yellow)
    {
        YellowLaser* yLaser = new YellowLaser(GetPosition(), dir, player);
        gom->Add(yLaser);

        activeShotLaser       = yLaser;
        firingTimer           = 0.0;
        currentFiringDuration = 5.0;
        laserDirection        = dir;
        rotationSpeed         = 1.5;
    }
    else
    {
        WhiteLaser* wLaser = new WhiteLaser(GetPosition(), dir, player);
        gom->Add(wLaser);

        activeShotLaser       = wLaser;
        firingTimer           = 0.0;
        currentFiringDuration = 2.0;
        laserDirection        = dir;
    }
}

void LaserStar::CreateContinuousLaser()
{
    if (continuousLaser != nullptr)
    {
        return;
    }

    continuousLaser = CreateLaserObject(GetPosition(), aimDirection);

    auto gom = Engine::GetGameStateManager().GetGSComponent<CS230::GameObjectManager>();
    if (gom != nullptr && continuousLaser != nullptr)
    {
        gom->Add(continuousLaser);
    }
}

void LaserStar::DestroyContinuousLaser()
{
    if (continuousLaser != nullptr)
    {
        continuousLaser->Destroy();
        continuousLaser = nullptr;
    }
}

Laser* LaserStar::CreateLaserObject(Math::vec2 startPos, Math::vec2 dir)
{
    switch (currentType)
    {
        case LaserType::White: return new WhiteLaser(startPos, dir, player);

        case LaserType::Yellow: return new YellowLaser(startPos, dir, player);

        case LaserType::Red: return new RedLaser(startPos, dir, player);
    }

    return nullptr;
}

CS200::RGBA LaserStar::GetTelegraphColor() const
{
    if (currentType == LaserType::Red && currentState == State::Warning && timer <= parryWindowTime)
    {
        return 0x00FFFFFF;
    }

    switch (currentType)
    {
        case LaserType::White: return 0xFFFFFFFF;

        case LaserType::Yellow: return 0xFFFF00FF;

        case LaserType::Red: return 0xFF0000FF;
    }

    return 0xFFFFFFFF;
}

CS200::RGBA LaserStar::GetBodyColor() const
{
    return 0x00FF00FF;
}

int LaserStar::GetMaxBounces() const
{
    if (currentType == LaserType::Red)
    {
        return 0;
    }

    return 2;
}

void LaserStar::SetLaserType(LaserType newType)
{
    if (currentType == newType)
    {
        return;
    }

    currentType = newType;

    DestroyContinuousLaser();
    activeShotLaser = nullptr;
    isFirstFrame    = true;
}

void LaserStar::SetPattern(Pattern newPattern)
{
    currentPattern = newPattern;
    blinkTimer     = 0.0;
}

void LaserStar::SetFireMode(FireMode newMode)
{
    if (currentFireMode == newMode)
    {
        return;
    }

    DestroyContinuousLaser();

    if (activeShotLaser != nullptr)
    {
        activeShotLaser->Destroy();
        activeShotLaser = nullptr;
    }

    currentFireMode = newMode;
    isFirstFrame    = true;
}

void LaserStar::SetAimDirection(Math::vec2 newDir)
{
    aimDirection = newDir.Normalize();

    if (continuousLaser != nullptr)
    {
        continuousLaser->SetDirection(aimDirection);
    }
}

void LaserStar::SetEnabled(bool enabled)
{
    SetIsActive(enabled);
    SetVisible(enabled);

    if (enabled)
    {
        ResetMovement();
        return;
    }

    DestroyContinuousLaser();

    if (activeShotLaser != nullptr)
    {
        activeShotLaser->Destroy();
        activeShotLaser = nullptr;
    }

    isFirstFrame = true;

    Shield* shield = player ? player->GetShield() : nullptr;
    if (shield != nullptr)
    {
        shield->SetParryWindowActive(false);
    }
}

bool LaserStar::IsMoveComplete() const
{
    return moveComplete;
}

void LaserStar::ResetMovement()
{
    SetPosition(startPosition);
    movedDistance = 0.0;
    moveComplete  = false;

    DestroyContinuousLaser();

    if (activeShotLaser != nullptr)
    {
        activeShotLaser->Destroy();
        activeShotLaser = nullptr;
    }

    isFirstFrame = true;
}