
#pragma once

#include "Door.hpp"

class Player;

class DoorActionHandler
{
public:
    struct Result
    {
        bool             handled    = false;
        Door::ActionType actionType = Door::ActionType::None;
        Door::Event      event      = Door::Event::None;
    };

    static Result Execute(Door& door, Player& player);

private:
    static void ChangeToDestination(Door::Destination destination);
};