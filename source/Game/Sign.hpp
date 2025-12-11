#pragma once

#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include <string>

class Sign : public CS230::GameObject
{
public:
    Sign(Math::vec2 start_pos, Math::vec2 size, std::string msg);

    void Draw(const Math::TransformationMatrix& camera_matrix) override;

    GameObjectTypes Type() override
    {
        return GameObjectTypes::Sign;
    }

    std::string TypeName() override
    {
        return "Sign";
    }

    void Interact(CS230::GameObject* interactor) override;

private:
    Math::vec2  signSize;
    std::string message;
};