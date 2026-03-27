#include "HostileStar.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameObjectManager.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Logger.hpp"
#include "Player.hpp"
#include "RedLaser.hpp"
#include "Shield.hpp"
#include "YellowLaser.hpp"

HostileStar::HostileStar(Math::vec2 pos, Player* in_player, StarType type) : Star(pos, in_player), currentStarType(type)
{
    warningDuration  = 4.0;
    cooldownDuration = 4.0;
}

void HostileStar::Update(double dt)
{
    if (activeLaser != nullptr)
    {
        firingTimer += dt;
        if (firingTimer >= currentFiringDuration)
        {
            activeLaser->Destroy();
            activeLaser = nullptr;
        }
        else
        {
            if (currentStarType == StarType::Yellow)
            {
                Math::vec2 targetDir    = (player->GetPosition() - GetPosition()).Normalize();
                double     currentAngle = std::atan2(laserDirection.y, laserDirection.x);
                double     targetAngle  = std::atan2(targetDir.y, targetDir.x);

                double diff = targetAngle - currentAngle;
                while (diff <= -PI)
                    diff += 2 * PI;
                while (diff > PI)
                    diff -= 2 * PI;

                double maxRotate = rotationSpeed * dt;
                currentAngle += (std::abs(diff) < maxRotate) ? diff : ((diff > 0) ? maxRotate : -maxRotate);

                laserDirection = Math::vec2{ std::cos(currentAngle), std::sin(currentAngle) };
                activeLaser->SetDirection(laserDirection);
            }
            activeLaser->SetStartPos(GetPosition());
        }
    }
    else
    {
        HandleBasicAI(dt);
    }

    if (currentStarType == StarType::Red && currentState == State::Warning)
    {
        Shield* shield = player->GetShield();
        if (shield != nullptr)
        {
            if (timer <= parryWindowTime)
                shield->SetParryWindowActive(true);
            else
                shield->SetParryWindowActive(false);
        }
    }

    CS230::GameObject::Update(dt);
}

void HostileStar::OnWarningComplete()
{
    Math::vec2 dir = (player->GetPosition() - GetPosition()).Normalize();
    auto       gom = Engine::GetGameStateManager().GetGSComponent<CS230::GameObjectManager>();

    if (currentStarType == StarType::Red)
    {
        bool    parrySuccess = false;
        Shield* shield       = player->GetShield();

        if (shield && shield->ConsumeParryState())
        {
            shield->HandleHit(true);
            Engine::GetLogger().LogEvent("Hostile Red Laser Perfect Parry!");
            parrySuccess = true;
        }

        if (shield)
            shield->SetParryWindowActive(false);

        RedLaser* rLaser = new RedLaser(GetPosition(), dir, player);

        if (parrySuccess)
        {
            rLaser->SetParried(true);
        }

        gom->Add(rLaser);
        activeLaser           = rLaser;
        firingTimer           = 0.0;
        currentFiringDuration = 0.2;
        laserDirection        = dir;
    }
    else
    {
        YellowLaser* yLaser = new YellowLaser(GetPosition(), dir, player);
        gom->Add(yLaser);

        activeLaser           = yLaser;
        firingTimer           = 0.0;
        currentFiringDuration = 5.0;
        laserDirection        = dir;
        rotationSpeed         = 1.5;
    }
}

CS200::RGBA HostileStar::GetTelegraphColor() const
{
    if (currentStarType == StarType::Red && currentState == State::Warning && timer <= parryWindowTime)
    {
        return 0x00FFFFFF;
    }
    return (currentStarType == StarType::Red) ? 0xFF0000FF : 0xFFFF00FF;
}

void HostileStar::SetStarType(StarType newType)
{
    currentStarType = newType;

    if (currentStarType == StarType::Red)
    {
        cooldownDuration = 4.0;
    }
    else
    {
        cooldownDuration = 9.0;
    }
}