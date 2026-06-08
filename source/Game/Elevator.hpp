#pragma once
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/Vec2.hpp"

class Player;

class Elevator : public CS230::GameObject
{
public:
    Elevator(Math::vec2 center, Math::vec2 size, double travelDist, double periodSecs);

    void Update(double dt) override;
    void Draw(const Math::TransformationMatrix& camera_matrix) override;

    bool CanCollideWith(GameObjectTypes) override { return false; }
    void ResolveCollision(CS230::GameObject*) override {}

    GameObjectTypes Type()     override { return GameObjectTypes::Elevator; }
    std::string     TypeName() override { return "Elevator"; }

    double GetTopY() const { return GetPosition().y + size.y * 0.5; }
    void   SetRider(Player* p) { ridingPlayer = p; }

private:
    Math::vec2 size;
    Math::vec2 startPos;
    double     travelDist;
    double     period;
    double     phase      = 0.0;
    Player*    ridingPlayer = nullptr;
};
