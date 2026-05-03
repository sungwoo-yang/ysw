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
class TargetStar;

class BossController
{
public:
    enum class State
    {
        Intro,
        Survival,
        Reflect,
        Clear
    };

    BossController(Player* player, Constellation* constellation);

    void CollectPhaseObjects(CS230::GameObjectManager* gom);
    void Update(double dt);

    void StartReflectFromDoor();

    void  SetState(State nextPhase);
    State GetState() const;

    int GetSurvivalClearCount() const;
    int GetCurrentPhaseIndex() const;
    int GetPhaseCount() const;

private:
    Player*        player        = nullptr;
    Constellation* constellation = nullptr;

    State currentState = State::Intro;

    // 0-based phase index.
    // P1 -> 0, P2 -> 1, P3 -> 2, ... PN -> N - 1
    int currentPhaseIndex = 0;

    int survivalClearCount        = 0;
    int targetCountAtReflectStart = 0;

    // Temporary safe-zone rule.
    // Even phase index: move right.
    // Odd phase index: move left.
    const double leftSafeX  = 200.0;
    const double rightSafeX = 2800.0;

    std::vector<std::vector<CS230::GameObject*>> phaseObjects;
    std::vector<CS230::GameObject*>              reflectObjects;

    void SetObjectsEnabled(const std::vector<CS230::GameObject*>& objects, bool enabled);
    void SetConstellationEnabled(bool enabled);
    void ApplyStateVisibility();

    void StartSurvivalPhase(int phaseIndex);
    void StartReflectPhase();
    void AdvanceAfterReflect();

    bool HasReachedCurrentSafeZone() const;
    bool ShouldMoveRightForCurrentPhase() const;

    int  ExtractPhaseNumber(const std::string& name) const;
    bool NameContains(const std::string& name, const std::string& token) const;
};