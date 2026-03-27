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
    HandleBasicAI(dt);


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
        if (player->GetShield())
            player->GetShield()->SetParryWindowActive(false);
        gom->Add(new RedLaser(GetPosition(), dir, player));
        Engine::GetLogger().LogEvent("Boss Fired Red Laser! (Parry Required)");
    }
    else
    {
        gom->Add(new YellowLaser(GetPosition(), dir, player));
        Engine::GetLogger().LogEvent("Boss Fired Yellow Laser! (Evade)");
    }

    attackStep = (attackStep + 1) % 3;
}

CS200::RGBA BossStar::GetTelegraphColor() const
{
    if (attackStep < 2)
        return 0xFF0000FF;
    else
        return 0xFFFF00FF;
}