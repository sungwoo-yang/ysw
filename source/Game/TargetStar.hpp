#pragma once
#include "CS200/RGBA.hpp"
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"

class TargetStar : public CS230::GameObject
{
public:
    TargetStar(Math::vec2 position);

    void Update(double dt) override;
    void Draw([[maybe_unused]] const Math::TransformationMatrix& camera_matrix) override;
    void OnHit();

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
    bool        isHit  = false;

    double       hitTimer       = 0.0;
    bool         isBeingHit     = false;
    const double activationTime = 3.0;
};