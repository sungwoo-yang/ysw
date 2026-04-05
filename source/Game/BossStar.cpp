#include "BossStar.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameObjectManager.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Logger.hpp"
#include "Player.hpp"
#include "RedLaser.hpp"
#include "Shield.hpp"
#include "YellowLaser.hpp"

BossStar::BossStar(Math::vec2 pos, Player* in_player) : Star(pos, in_player), attackStep(0)
{
    detectionRadius = 800.0;
    chaseRadius     = 1600.0;

    warningDuration  = 4.0;
    cooldownDuration = 4.0;
}

void BossStar::Update(double dt)
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
            if (activeLaser->TypeName() == "YellowLaser")
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

    if (attackStep < 2 && currentState == State::Warning)
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

void BossStar::OnWarningComplete()
{
    Math::vec2 dir = (player->GetPosition() - GetPosition()).Normalize();
    auto       gom = Engine::GetGameStateManager().GetGSComponent<CS230::GameObjectManager>();

    if (attackStep < 2)
    {
        bool    parrySuccess = false;
        Shield* shield       = player->GetShield();

        if (shield && shield->ConsumeParryState())
        {
            shield->HandleHit(true);
            Engine::GetLogger().LogEvent("Boss Red Laser Perfect Parry!");
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

        Engine::GetLogger().LogEvent("Boss Fired Red Laser! (Parry Required)");
    }
    else
    {
        YellowLaser* yLaser = new YellowLaser(GetPosition(), dir, player);
        yLaser->StartExtending();

        gom->Add(yLaser);

        activeLaser           = yLaser;
        firingTimer           = 0.0;
        currentFiringDuration = 5.0;
        laserDirection        = dir;
        rotationSpeed         = 0.2;

        Engine::GetLogger().LogEvent("Boss Fired Yellow Laser! (Evade)");
    }

    attackStep = (attackStep + 1) % 3;
}

CS200::RGBA BossStar::GetTelegraphColor() const
{
    if (attackStep < 2 && currentState == State::Warning && timer <= parryWindowTime)
    {
        return 0x00FFFFFF;
    }

    if (attackStep < 2)
        return 0xFF0000FF;
    else
        return 0xFFFF00FF;
}

int BossStar::GetMaxBounces() const
{
    if (attackStep < 2)
    {
        return 0;
    }
    return 2;
}