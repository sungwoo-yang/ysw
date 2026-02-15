#pragma once

#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include <string>

class Sign : public CS230::GameObject
{
public:
    Sign(Math::vec2 in_start_pos, Math::vec2 in_size, std::string in_msg);

    void Draw(const Math::TransformationMatrix& camera_matrix) override;

    [[nodiscard]] GameObjectTypes Type() override
    {
        return GameObjectTypes::Sign;
    }

    [[nodiscard]] std::string TypeName() override
    {
        return "Sign";
    }

    // Called when the player gets close and presses the interaction key
    void Interact(CS230::GameObject* interactor) override;

private:
    Math::vec2  signSize;
    std::string message;
};