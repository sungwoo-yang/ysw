#include "DoorActionHandler.hpp"

#include "Boss1.hpp"
#include "FallCutscene.hpp"
#include "Mode1.hpp"
#include "Mode3.hpp"
#include "Player.hpp"

#include "Engine/Engine.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Logger.hpp"

DoorActionHandler::Result DoorActionHandler::Execute(Door& door, Player& player)
{
    const Door::Config& config = door.GetConfig();

    Result result;
    result.actionType = config.actionType;
    result.event      = config.event;

    switch (config.actionType)
    {
        case Door::ActionType::None:
            {
                Engine::GetLogger().LogError("Door has no action: " + door.GetName());
                result.handled = false;
                return result;
            }

        case Door::ActionType::ChangeState:
            {
                ChangeToDestination(config.destination);
                result.handled = true;
                return result;
            }

        case Door::ActionType::CutsceneThenChangeState:
            {
                Door::Destination nextDestination = config.destination;

                FallCutscene::SetNextState([nextDestination]() { ChangeToDestination(nextDestination); });

                Engine::GetGameStateManager().ChangeStateWithFade<FallCutscene>();

                result.handled = true;
                return result;
            }

        case Door::ActionType::Teleport:
            {
                player.SetPosition(config.teleportPosition);

                Engine::GetLogger().LogEvent("Door teleport: " + door.GetName() + " -> (" + std::to_string(config.teleportPosition.x) + ", " + std::to_string(config.teleportPosition.y) + ")");

                result.handled = true;
                return result;
            }
    }

    return result;
}

void DoorActionHandler::ChangeToDestination(Door::Destination destination)
{
    switch (destination)
    {
        case Door::Destination::Mode1: Engine::GetGameStateManager().ChangeStateWithFade<Mode1>(); break;

        case Door::Destination::Mode3: Engine::GetGameStateManager().ChangeStateWithFade<Mode3>(); break;

        case Door::Destination::Boss1: Engine::GetGameStateManager().ChangeStateWithFade<Boss1>(); break;

        case Door::Destination::None: Engine::GetLogger().LogError("Door destination is None."); break;
    }
}