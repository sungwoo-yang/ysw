#pragma once
#include "CS200/RGBA.hpp"
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/Vec2.hpp"
#include <string>

class Gate : public CS230::GameObject
{
public:
    Gate(Math::vec2 in_position, Math::vec2 in_size);

    void Update(double dt) override;
    void Draw(const Math::TransformationMatrix& camera_matrix) override;

    // Check if player can pass through based on current state
    bool CanCollideWith(GameObjectTypes other_object_type) override;

    [[nodiscard]] GameObjectTypes Type() override
    {
        return GameObjectTypes::Gate;
    }

    [[nodiscard]] std::string TypeName() override
    {
        return "Gate";
    }

    // State management methods
    void Open();
    void Close();
    [[nodiscard]] bool IsOpen() const
    {
        return isOpen;
    }

private:
    Math::vec2  size;
    bool        isOpen;
    CS200::RGBA color;
};