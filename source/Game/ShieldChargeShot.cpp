#include "ShieldChargeShot.hpp"

#include "BossConfig.hpp"
#include "CS200/IRenderer2D.hpp"
#include "Engine/Camera.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Input.hpp"
#include "Engine/Window.hpp"
#include "Player.hpp"
#include "ShieldEnergy.hpp"
#include "TargetStar.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Boss
{
    ShieldChargeShot::ShieldChargeShot(Player* in_player, ShieldEnergy* in_shieldEnergy)
        : player(in_player), shieldEnergy(in_shieldEnergy)
    {
    }

    void ShieldChargeShot::SetTargetStars(const std::vector<TargetStar*>& in_targetStars)
    {
        targetStars = in_targetStars;
    }

    void ShieldChargeShot::Update(double dt)
    {
        auto& input = Engine::GetInput();

        if (beamVisible)
        {
            beamTimer -= dt;

            if (beamTimer <= 0.0)
            {
                beamTimer = 0.0;
                beamVisible = false;
            }
        }

        if (player == nullptr || shieldEnergy == nullptr)
        {
            return;
        }

        const bool canCharge = shieldEnergy->IsFull();
        const bool leftDown = input.MouseButtonDown(CS230::Input::MouseButton::Left);
        const bool leftPressed = input.MouseButtonJustPressed(CS230::Input::MouseButton::Left);
        const bool leftReleased = input.MouseButtonJustReleased(CS230::Input::MouseButton::Left);

        if (leftPressed && canCharge)
        {
            StartCharging();
        }

        if (!isCharging)
        {
            return;
        }

        if (!leftDown)
        {
            if (readyToFire && leftReleased)
            {
                Fire();
            }
            else
            {
                CancelCharging();
            }

            return;
        }

        chargeTimer += dt;

        if (chargeTimer >= Config::ChargeHoldTime)
        {
            readyToFire = true;
            chargeTimer = Config::ChargeHoldTime;
        }
    }

    void ShieldChargeShot::Draw([[maybe_unused]] const Math::TransformationMatrix& camera_matrix)
    {
        auto& renderer = Engine::GetRenderer2D();

        if (isCharging && player != nullptr)
        {
            const Math::vec2 start = player->GetPosition();
            const Math::vec2 mouse = GetMouseWorldPosition();

            Math::vec2 direction = mouse - start;

            if (direction.LengthSquared() > 0.0)
            {
                direction = direction.Normalize();
            }
            else
            {
                direction = { 1.0, 0.0 };
            }

            const Math::vec2 previewEnd = start + direction * Config::ChargeShotLength;

            const CS200::RGBA previewColor = readyToFire ? 0xFFFFAAFF : 0x88FFFFFF;
            renderer.DrawLine(start, previewEnd, previewColor, 4.0);
        }

        if (beamVisible)
        {
            renderer.DrawLine(beamStart, beamEnd, CS200::WHITE, Config::ChargeShotWidth + 6.0);
            renderer.DrawLine(beamStart, beamEnd, 0xFFFFAAFF, Config::ChargeShotWidth);
        }
    }

    bool ShieldChargeShot::IsCharging() const
    {
        return isCharging;
    }

    bool ShieldChargeShot::IsReadyToFire() const
    {
        return readyToFire;
    }

    float ShieldChargeShot::GetChargeRatio() const
    {
        if constexpr (Config::ChargeHoldTime <= 0.0f)
        {
            return 1.0f;
        }

        return static_cast<float>(std::clamp(chargeTimer / Config::ChargeHoldTime, 0.0, 1.0));
    }

    void ShieldChargeShot::StartCharging()
    {
        isCharging = true;
        readyToFire = false;
        chargeTimer = 0.0;
    }

    void ShieldChargeShot::CancelCharging()
    {
        isCharging = false;
        readyToFire = false;
        chargeTimer = 0.0;
    }

    void ShieldChargeShot::Fire()
    {
        if (player == nullptr || shieldEnergy == nullptr)
        {
            CancelCharging();
            return;
        }

        if (!shieldEnergy->ConsumeEnergy(Config::RequiredChargeEnergy))
        {
            CancelCharging();
            return;
        }

        beamStart = player->GetPosition();

        Math::vec2 direction = GetMouseWorldPosition() - beamStart;

        if (direction.LengthSquared() > 0.0)
        {
            direction = direction.Normalize();
        }
        else
        {
            direction = { 1.0, 0.0 };
        }

        beamEnd = beamStart + direction * Config::ChargeShotLength;

        TargetStar* hitTarget = FindFirstHitTarget(beamStart, beamEnd);

        if (hitTarget != nullptr)
        {
            hitTarget->ActivateInstantly();
        }

        beamVisible = true;
        beamTimer = Config::ChargeShotBeamDuration;

        CancelCharging();
    }

    Math::vec2 ShieldChargeShot::GetMouseWorldPosition() const
    {
        auto& input = Engine::GetInput();

        Math::vec2 mouseScreenPos = input.GetMousePosition();
        Math::ivec2 winSize = Engine::GetWindow().GetSize();

        auto camera = Engine::GetGameStateManager().GetGSComponent<CS230::Camera>();

        Math::vec2 cameraPosition = { 0.0, 0.0 };
        double cameraScale = 1.0;

        if (camera != nullptr)
        {
            cameraPosition = camera->GetPosition();
            cameraScale = camera->GetScale();
        }

        const double mouseGLY = static_cast<double>(winSize.y) - mouseScreenPos.y;

        return {
            cameraPosition.x + mouseScreenPos.x / cameraScale,
            cameraPosition.y + mouseGLY / cameraScale
        };
    }

    TargetStar* ShieldChargeShot::FindFirstHitTarget(Math::vec2 start, Math::vec2 end) const
    {
        TargetStar* bestTarget = nullptr;
        double bestDistanceFromStart = std::numeric_limits<double>::max();

        for (TargetStar* target : targetStars)
        {
            if (target == nullptr || target->IsHit())
            {
                continue;
            }

            const double hitRadius = target->GetRadius() + Config::ChargeShotHitRadius;
            const double distanceToBeamSq = DistanceToSegmentSquared(target->GetPosition(), start, end);

            if (distanceToBeamSq > hitRadius * hitRadius)
            {
                continue;
            }

            const double distanceFromStartSq = (target->GetPosition() - start).LengthSquared();

            if (distanceFromStartSq < bestDistanceFromStart)
            {
                bestDistanceFromStart = distanceFromStartSq;
                bestTarget = target;
            }
        }

        return bestTarget;
    }

    double ShieldChargeShot::DistanceToSegmentSquared(Math::vec2 point, Math::vec2 a, Math::vec2 b)
    {
        const double lengthSquared = (b - a).LengthSquared();

        if (lengthSquared <= 0.0)
        {
            return (point - a).LengthSquared();
        }

        const double t = std::clamp((point - a).Dot(b - a) / lengthSquared, 0.0, 1.0);
        const Math::vec2 projection = a + (b - a) * t;

        return (point - projection).LengthSquared();
    }
}