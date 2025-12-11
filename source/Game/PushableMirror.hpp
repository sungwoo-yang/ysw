#pragma once
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/Vec2.hpp"
#include <vector>
#include <utility>

class PushableMirror : public CS230::GameObject
{
public:
    PushableMirror(Math::vec2 start_pos, Math::vec2 size);

    void Update(double dt) override;
    void Draw(const Math::TransformationMatrix& camera_matrix) override;
    
    void ResolveCollision(CS230::GameObject* other_object) override;
    bool CanCollideWith(GameObjectTypes other_object_type) override;

    GameObjectTypes Type() override { return GameObjectTypes::PushableMirror; }
    std::string TypeName() override { return "PushableMirror"; }

    std::vector<std::pair<Math::vec2, Math::vec2>> GetSegments() const;

    void Push(Math::vec2 pushVelocity);

private:
    Math::vec2 size;
    double velocityY = 0.0;
    
    const double gravity = 1500.0;
    const double friction = 8.0;
};