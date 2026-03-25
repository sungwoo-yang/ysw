#pragma once
#include "CS200/RGBA.hpp"
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"

class TargetStar : public CS230::GameObject
{
public:
    TargetStar(Math::vec2 in_position);

    void Update(double dt) override;
    void Draw([[maybe_unused]] const Math::TransformationMatrix& camera_matrix) override;

    // Called externally by laser objects when their beam intersects with this target
    void OnHit();

    // Check if the puzzle target has been successfully activated
    bool   IsHit() const;
    double GetRadius() const;

    GameObjectTypes Type() override
    {
        return GameObjectTypes::Target;
    }

    std::string TypeName() override
    {
        return "TargetStar";
    }

private:
    CS200::RGBA color;
    double      radius = 40.0;

    // Permanent activation state once successfully charged
    bool        isHit  = false;

    // Variables for continuous laser charging logic
    double       hitTimer       = 0.0;
    bool         isBeingHit     = false;    // State flag updated per frame
    const double activationTime = 3.0;      // Required continuous hit duration
};