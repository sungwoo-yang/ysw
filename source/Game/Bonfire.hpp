#pragma once

#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"

class Bonfire : public CS230::GameObject
{
public:
    Bonfire(Math::vec2 start_pos, Math::vec2 size);

    void Draw(const Math::TransformationMatrix& camera_matrix) override;

    GameObjectTypes Type() override
    {
        return GameObjectTypes::Bonfire;
    }

    std::string TypeName() override
    {
        return "Bonfire";
    }

    void Interact(CS230::GameObject* interactor) override;

private:
    Math::vec2 bonfireSize;
};