#pragma once
#include "CS200/RGBA.hpp"
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/Vec2.hpp"

class Gate : public CS230::GameObject
{
public:
    Gate(Math::vec2 position, Math::vec2 size);

    void Update(double dt) override;
    void Draw(const Math::TransformationMatrix& camera_matrix) override;

    bool CanCollideWith(GameObjectTypes other_object_type) override;

    GameObjectTypes Type() override
    {
        return GameObjectTypes::Gate;
    }

    std::string TypeName() override
    {
        return "Gate";
    }

    void Open();
    void Close();

    bool IsOpen() const
    {
        return isOpen;
    }

private:
    Math::vec2  size;
    bool        isOpen; 
    CS200::RGBA color;
};