#pragma once
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"

class Door : public CS230::GameObject
{
public:
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

    // Interact with Door
    void Interact(CS230::GameObject* interactor) override;

private:
    Math::vec2 size;
};