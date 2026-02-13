#pragma once

#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"

class Bonfire : public CS230::GameObject
{
public:
    Bonfire(Math::vec2 in_start_pos, Math::vec2 in_size);

    void Draw(const Math::TransformationMatrix& camera_matrix) override;

    [[nodiscard]] GameObjectTypes Type() override
    {
        return GameObjectTypes::Bonfire;
    }

    std::string TypeName() override
    {
        return "Bonfire";
    }

    // Handle interaction
    void Interact(CS230::GameObject* interactor) override;

private:
    Math::vec2 bonfireSize;
};