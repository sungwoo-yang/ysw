#pragma once
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/Vec2.hpp"
#include <vector>
#include <utility>

class PushableMirror : public CS230::GameObject
{
public:
    PushableMirror(Math::vec2 in_start_pos, Math::vec2 in_size);

    void Update(double dt) override;
    void Draw(const Math::TransformationMatrix& camera_matrix) override;
    
    void ResolveCollision(CS230::GameObject* other_object) override;
    bool CanCollideWith(GameObjectTypes other_object_type) override;

    GameObjectTypes Type() override { return GameObjectTypes::PushableMirror; }
    std::string TypeName() override { return "PushableMirror"; }

    // Returns the diagonal reflective surface for laser bounce calculations
    std::vector<std::pair<Math::vec2, Math::vec2>> GetSegments() const;

    // Applies horizontal velocity when pushed by the player
    void Push(Math::vec2 pushVelocity);

private:
    Math::vec2 size;
    double velocityY = 0.0;
    
    // Physics properties for realistic pushing behavior
    const double gravity = 1500.0;
    const double friction = 8.0;
};