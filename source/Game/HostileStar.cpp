#include "HostileStar.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameObjectManager.hpp"
#include "Engine/GameStateManager.hpp"
#include "Player.hpp"
#include "RedLaser.hpp"
#include "Shield.hpp"
#include "YellowLaser.hpp"

HostileStar::HostileStar(Math::vec2 pos, Player* in_player, StarType type) : Star(pos, in_player), currentStarType(type)
{
    warningDuration = 4.0;

    if (currentStarType == StarType::Red)
    {
        cooldownDuration = 4.0;
    }
    else
    {
        cooldownDuration = 9.0;
    }
}

void HostileStar::Update(double dt)
{
    HandleBasicAI(dt);

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
        if (player->GetShield())
            player->GetShield()->SetParryWindowActive(false);
        gom->Add(new RedLaser(GetPosition(), dir, player));
    }
    else
    {
        gom->Add(new YellowLaser(GetPosition(), dir, player));
    }
}

CS200::RGBA HostileStar::GetTelegraphColor() const
{
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