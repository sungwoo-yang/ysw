#pragma once

#include "Engine/MapManager.h"

#include <map>
#include <string>

class Player;

namespace ObjectFactory
{
    using SignTextMap = std::map<std::string, std::string>;

    CS230::MapManager::GameObjectFactory Create(Player* player, SignTextMap signTexts = {});
}