#pragma once

#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"

class Door : public CS230::GameObject
{
public:
    enum class ActionType
    {
        None,
        ChangeState,
        CutsceneThenChangeState,
        Teleport
    };

    enum class Destination
    {
        None,
        Mode1,
        Mode3,
        Boss1
    };

    enum class Event
    {
        None,
        BossStartReflect
    };

    struct Config
    {
        ActionType  actionType       = ActionType::None;
        Destination destination      = Destination::None;
        Math::vec2  teleportPosition = { 0.0, 0.0 };
        Event       event            = Event::None;
    };

    Door(Math::vec2 in_position, Math::vec2 in_size);

    GameObjectTypes Type() override
    {
        return GameObjectTypes::Door;
    }

    std::string TypeName() override
    {
        return "Door";
    }

    void Draw(const Math::TransformationMatrix& camera_matrix) override;
    void Interact(CS230::GameObject* interactor) override;

    void          SetConfig(const Config& newConfig);
    const Config& GetConfig() const;

    bool ConsumeInteractionRequest();

private:
    Math::vec2 size;
    bool       interactionRequested = false;

    Config config;
};