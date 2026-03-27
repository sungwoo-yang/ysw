#include "PuzzleStar.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameObjectManager.hpp"
#include "Engine/GameStateManager.hpp"
#include "RedLaser.hpp"
#include "WhiteLaser.hpp"
#include "YellowLaser.hpp"
#include <cmath>

PuzzleStar::PuzzleStar(Math::vec2 pos, Player* in_player, LaserType type, Pattern pattern, Math::vec2 initialDir)
    : Star(pos, in_player), currentType(type), currentPattern(pattern), aimDirection(initialDir.Normalize()), myLaser(nullptr)
{
    myLaser = new WhiteLaser(GetPosition(), aimDirection, player);
    Engine::GetGameStateManager().GetGSComponent<CS230::GameObjectManager>()->Add(myLaser);
}

PuzzleStar::~PuzzleStar()
{
    if (myLaser != nullptr)
    {
        myLaser->Destroy();
    }
}

void PuzzleStar::Update(double dt)
{
    Star::Update(dt);

    switch (currentPattern)
    {
        case Pattern::Static:
            if (myLaser)
                myLaser->SetStartPos(GetPosition());
            break;

        case Pattern::Rotating:
            {
                double currentAngle = std::atan2(aimDirection.y, aimDirection.x);
                currentAngle += rotationSpeed * dt;

                aimDirection = Math::vec2{ std::cos(currentAngle), std::sin(currentAngle) };

                if (myLaser)
                {
                    myLaser->SetStartPos(GetPosition());
                    myLaser->SetDirection(aimDirection);
                }
                break;
            }

        case Pattern::Blink:
            {
                blinkTimer += dt;
                if (blinkTimer >= blinkInterval)
                {
                    isLaserOn  = !isLaserOn;
                    blinkTimer = 0.0;

                    if (myLaser != nullptr)
                    {
                        myLaser->SetIsActive(isLaserOn);
                        myLaser->SetVisible(isLaserOn);
                    }
                }

                if (myLaser)
                    myLaser->SetStartPos(GetPosition());
                break;
            }
    }

    CS230::GameObject::Update(dt);
}

void PuzzleStar::SetLaserActive(bool active)
{
    if (active && myLaser == nullptr)
    {
        myLaser = new WhiteLaser(GetPosition(), aimDirection, player);
        Engine::GetGameStateManager().GetGSComponent<CS230::GameObjectManager>()->Add(myLaser);
    }
    else if (!active && myLaser != nullptr)
    {
        myLaser->Destroy();
        myLaser = nullptr;
    }
}

void PuzzleStar::SetPattern(Pattern newPattern)
{
    currentPattern = newPattern;
    blinkTimer     = 0.0;
}

void PuzzleStar::SetAimDirection(Math::vec2 newDir)
{
    aimDirection = newDir.Normalize();
}

void PuzzleStar::OnWarningComplete()
{
    auto gom = Engine::GetGameStateManager().GetGSComponent<CS230::GameObjectManager>();

    switch (currentType)
    {
        case LaserType::White: gom->Add(new WhiteLaser(GetPosition(), aimDirection, player)); break;
        case LaserType::Yellow: gom->Add(new YellowLaser(GetPosition(), aimDirection, player)); break;
        case LaserType::Red: gom->Add(new RedLaser(GetPosition(), aimDirection, player)); break;
    }
}