#pragma once

#include <string>
#include <vector>

namespace CS230
{
    class GameObject;
    class GameObjectManager;
}

class Player;
class Constellation;
class LaserStar;

class BossController
{
public:
    enum class Phase
    {
        Intro,
        Survival1,
        Survival2,
        Survival3,
        Reflect,
        Clear
    };

    BossController(Player* player, Constellation* constellation);

    void CollectPhaseObjects(CS230::GameObjectManager* gom);
    void Update(double dt);

    void  SetPhase(Phase nextPhase);
    Phase GetPhase() const;

    int GetSurvivalClearCount() const;

private:
    Player*        player        = nullptr;
    Constellation* constellation = nullptr;

    Phase currentPhase = Phase::Intro;

    int survivalClearCount = 0;

    std::vector<CS230::GameObject*> phase1Objects;
    std::vector<CS230::GameObject*> phase2Objects;
    std::vector<CS230::GameObject*> phase3Objects;
    std::vector<CS230::GameObject*> reflectObjects;

    void SetObjectsEnabled(const std::vector<CS230::GameObject*>& objects, bool enabled);
    void ApplyPhaseVisibility();

    bool NameContains(const std::string& name, const std::string& token) const;
};