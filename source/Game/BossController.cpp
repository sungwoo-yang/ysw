#include "BossController.hpp"

#include "Constellation.hpp"
#include "LaserStar.hpp"
#include "Player.hpp"

#include "Engine/Engine.hpp"
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectManager.hpp"
#include "Engine/Logger.hpp"

BossController::BossController(Player* in_player, Constellation* in_constellation) : player(in_player), constellation(in_constellation)
{
}

void BossController::CollectPhaseObjects(CS230::GameObjectManager* gom)
{
    if (gom == nullptr)
    {
        return;
    }

    phase1Objects.clear();
    phase2Objects.clear();
    phase3Objects.clear();
    reflectObjects.clear();

    for (CS230::GameObject* obj : gom->GetObjects())
    {
        if (obj == nullptr)
        {
            continue;
        }

        const std::string name = obj->GetName();

        if (NameContains(name, "P1"))
        {
            phase1Objects.push_back(obj);
        }
        else if (NameContains(name, "P2"))
        {
            phase2Objects.push_back(obj);
        }
        else if (NameContains(name, "P3"))
        {
            phase3Objects.push_back(obj);
        }
        else if (NameContains(name, "REFLECT"))
        {
            reflectObjects.push_back(obj);
        }
    }

    Engine::GetLogger().LogEvent(
        "BossController collected objects. "
        "P1: " +
        std::to_string(phase1Objects.size()) + ", P2: " + std::to_string(phase2Objects.size()) + ", P3: " + std::to_string(phase3Objects.size()) +
        ", Reflect: " + std::to_string(reflectObjects.size()));

    SetPhase(Phase::Survival1);
}

void BossController::Update(double)
{
    if (constellation != nullptr && constellation->IsRestored())
    {
        SetPhase(Phase::Clear);
    }

    // 나중에 여기서 추가할 것:
    // 1. 플레이어가 오른쪽 안전지대 도착했는지 확인
    // 2. Survival1 -> Survival2
    // 3. Survival2 -> Survival3
    // 4. Survival3 -> Reflect
    // 5. Reflect에서 TargetStar 점등 후 다시 Survival1 or Clear
}

void BossController::SetPhase(Phase nextPhase)
{
    if (currentPhase == nextPhase)
    {
        return;
    }

    currentPhase = nextPhase;
    ApplyPhaseVisibility();
}

BossController::Phase BossController::GetPhase() const
{
    return currentPhase;
}

int BossController::GetSurvivalClearCount() const
{
    return survivalClearCount;
}

void BossController::ApplyPhaseVisibility()
{
    SetObjectsEnabled(phase1Objects, false);
    SetObjectsEnabled(phase2Objects, false);
    SetObjectsEnabled(phase3Objects, false);
    SetObjectsEnabled(reflectObjects, false);

    switch (currentPhase)
    {
        case Phase::Intro: break;

        case Phase::Survival1: SetObjectsEnabled(phase1Objects, true); break;

        case Phase::Survival2: SetObjectsEnabled(phase2Objects, true); break;

        case Phase::Survival3: SetObjectsEnabled(phase3Objects, true); break;

        case Phase::Reflect: SetObjectsEnabled(reflectObjects, true); break;

        case Phase::Clear: break;
    }
}

void BossController::SetObjectsEnabled(const std::vector<CS230::GameObject*>& objects, bool enabled)
{
    for (CS230::GameObject* obj : objects)
    {
        if (obj == nullptr)
        {
            continue;
        }

        if (obj->TypeName() == "LaserStar")
        {
            static_cast<LaserStar*>(obj)->SetEnabled(enabled);
        }
        else
        {
            obj->SetIsActive(enabled);
            obj->SetVisible(enabled);
        }
    }
}

bool BossController::NameContains(const std::string& name, const std::string& token) const
{
    return name.find(token) != std::string::npos;
}