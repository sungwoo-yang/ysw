#include "BossController.hpp"

#include "Constellation.hpp"
#include "LaserStar.hpp"
#include "Player.hpp"
#include "TargetStar.hpp"

#include "Engine/Engine.hpp"
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectManager.hpp"
#include "Engine/Logger.hpp"

#include <cctype>
#include <string>

BossController::BossController(Player* in_player, Constellation* in_constellation) : player(in_player), constellation(in_constellation)
{
}

void BossController::CollectPhaseObjects(CS230::GameObjectManager* gom)
{
    if (gom == nullptr)
    {
        return;
    }

    phaseObjects.clear();
    reflectObjects.clear();
    clearObjects.clear();

    for (CS230::GameObject* obj : gom->GetObjects())
    {
        if (obj == nullptr)
        {
            continue;
        }

        const std::string name = obj->GetName();

        const int phaseNumber = ExtractPhaseNumber(name);

        if (NameContains(name, "CLEAR"))
        {
            clearObjects.push_back(obj);
        }
        else if (phaseNumber > 0)
        {
            const int phaseIndex = phaseNumber - 1;

            if (static_cast<int>(phaseObjects.size()) <= phaseIndex)
            {
                phaseObjects.resize(static_cast<size_t>(phaseIndex + 1));
            }

            phaseObjects[static_cast<size_t>(phaseIndex)].push_back(obj);
        }
        else if (name.rfind("DOOR_", 0) != 0 && NameContains(name, "REFLECT"))
        {
            reflectObjects.push_back(obj);
        }
    }

    std::string log = "BossController collected objects. PhaseCount: " + std::to_string(phaseObjects.size());

    for (size_t i = 0; i < phaseObjects.size(); ++i)
    {
        log += ", P" + std::to_string(i + 1) + ": " + std::to_string(phaseObjects[i].size());
    }

    log += ", Reflect: " + std::to_string(reflectObjects.size());
    log += ", Clear: " + std::to_string(clearObjects.size());

    if (constellation != nullptr)
    {
        log += ", ConstellationTargets: " + std::to_string(constellation->GetTotalTargetCount());
    }

    Engine::GetLogger().LogEvent(log);

    SetState(State::Intro);
}

void BossController::Update(double)
{
    if (constellation != nullptr && constellation->IsRestored())
    {
        SetState(State::Clear);
        return;
    }

    switch (currentState)
    {
        case State::Intro:
            if (!phaseObjects.empty())
            {
                StartSurvivalPhase(0);
            }
            else
            {
                SetState(State::Reflect);
            }
            break;

        case State::Survival: break;

        case State::Reflect:
            if (constellation != nullptr && constellation->GetActivatedTargetCount() > targetCountAtReflectStart)
            {
                AdvanceAfterReflect();
            }
            break;

        case State::Clear: break;
    }
}

void BossController::SetState(State nextState)
{
    if (currentState == nextState)
    {
        return;
    }

    currentState = nextState;
    ApplyStateVisibility();
}

BossController::State BossController::GetState() const
{
    return currentState;
}

int BossController::GetSurvivalClearCount() const
{
    return survivalClearCount;
}

int BossController::GetCurrentPhaseIndex() const
{
    return currentPhaseIndex;
}

int BossController::GetPhaseCount() const
{
    return static_cast<int>(phaseObjects.size());
}

void BossController::ApplyStateVisibility()
{
    for (const auto& objects : phaseObjects)
    {
        SetObjectsEnabled(objects, false);
    }

    SetObjectsEnabled(reflectObjects, false);
    SetObjectsEnabled(clearObjects, false);
    SetConstellationEnabled(false);

    switch (currentState)
    {
        case State::Intro: SetConstellationEnabled(true); break;

        case State::Survival:
            if (currentPhaseIndex >= 0 && currentPhaseIndex < static_cast<int>(phaseObjects.size()))
            {
                SetObjectsEnabled(phaseObjects[static_cast<size_t>(currentPhaseIndex)], true);
            }
            break;

        case State::Reflect:
            SetObjectsEnabled(reflectObjects, true);
            SetConstellationEnabled(true);
            break;

        case State::Clear:
            SetConstellationEnabled(true);
            SetObjectsEnabled(clearObjects, true);
            break;
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

void BossController::SetConstellationEnabled(bool enabled)
{
    if (constellation == nullptr)
    {
        return;
    }

    LaserStar* mainStar = constellation->GetMainStar();
    if (mainStar != nullptr)
    {
        mainStar->SetEnabled(enabled);
    }

    for (TargetStar* target : constellation->GetTargetStars())
    {
        if (target == nullptr)
        {
            continue;
        }

        target->SetIsActive(enabled);
        target->SetVisible(enabled);
    }
}

void BossController::StartSurvivalPhase(int phaseIndex)
{
    if (phaseIndex < 0 || phaseIndex >= static_cast<int>(phaseObjects.size()))
    {
        SetState(State::Clear);
        return;
    }

    currentPhaseIndex = phaseIndex;
    SetState(State::Survival);

    Engine::GetLogger().LogEvent("BossController Start Survival Phase: P" + std::to_string(currentPhaseIndex + 1));
}

void BossController::StartReflectPhase()
{
    if (constellation != nullptr)
    {
        targetCountAtReflectStart = constellation->GetActivatedTargetCount();
    }
    else
    {
        targetCountAtReflectStart = 0;
    }

    SetState(State::Reflect);

    Engine::GetLogger().LogEvent("BossController Start Reflect Phase after P" + std::to_string(currentPhaseIndex + 1));
}

void BossController::StartReflectFromDoor(Math::vec2 returnPosition)
{
    if (currentState != State::Survival)
    {
        Engine::GetLogger().LogEvent("BossController ignored door reflect request because current state is not Survival.");
        return;
    }

    reflectReturnPosition    = returnPosition;
    hasReflectReturnPosition = true;

    ++survivalClearCount;
    StartReflectPhase();
}

void BossController::StartFirstPhase()
{
    if (!phaseObjects.empty())
    {
        StartSurvivalPhase(0);
    }
    else
    {
        SetState(State::Reflect);
    }
}

void BossController::AdvanceAfterReflect()
{
    const int nextPhaseIndex = currentPhaseIndex + 1;

    if (nextPhaseIndex >= static_cast<int>(phaseObjects.size()))
    {
        SetState(State::Clear);
        Engine::GetLogger().LogEvent("BossController Clear");
        return;
    }

    if (player != nullptr && hasReflectReturnPosition)
    {
        player->SetPosition(reflectReturnPosition);
        player->velocityY         = 0.0;
        player->isInteracting     = false;
        player->interactionTarget = nullptr;
    }

    hasReflectReturnPosition = false;

    StartSurvivalPhase(nextPhaseIndex);
}

bool BossController::HasReachedCurrentSafeZone() const
{
    if (player == nullptr)
    {
        return false;
    }

    if (ShouldMoveRightForCurrentPhase())
    {
        return player->GetPosition().x >= rightSafeX;
    }

    return player->GetPosition().x <= leftSafeX;
}

bool BossController::ShouldMoveRightForCurrentPhase() const
{
    return currentPhaseIndex % 2 == 0;
}

int BossController::ExtractPhaseNumber(const std::string& name) const
{
    for (size_t i = 0; i < name.size(); ++i)
    {
        if (name[i] != 'P')
        {
            continue;
        }

        const size_t digitStart = i + 1;

        if (digitStart >= name.size() || !std::isdigit(static_cast<unsigned char>(name[digitStart])))
        {
            continue;
        }

        size_t digitEnd = digitStart;

        while (digitEnd < name.size() && std::isdigit(static_cast<unsigned char>(name[digitEnd])))
        {
            ++digitEnd;
        }

        return std::stoi(name.substr(digitStart, digitEnd - digitStart));
    }

    return 0;
}

bool BossController::NameContains(const std::string& name, const std::string& token) const
{
    return name.find(token) != std::string::npos;
}