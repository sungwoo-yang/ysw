
#include "Dash.hpp"
#include "Engine.hpp"
#include "Logger.hpp"

namespace CS230
{
    void DashComponent::UpdateTimers(double delta_time)
    {
        // Decrease cooldown
        if (dashCooldownTimer > 0.0)
        {
            dashCooldownTimer -= delta_time;
        }

        // Decrease dash duration
        if (isDashing)
        {
            dashTimer -= delta_time;
            if (dashTimer <= 0.0)
            {
                isDashing = false; // Stop Dash
            }
        }
    }

    bool DashComponent::TryStartDash(bool currentFaceRight)
    {
        // Execute if cooldown is 0 and not dashing
        if (dashCooldownTimer <= 0.0 && !isDashing)
        {
            isDashing         = true;
            dashTimer         = dashDuration;
            dashCooldownTimer = dashCooldown;
            dashDirection     = currentFaceRight ? 1 : -1;

            Engine::GetLogger().LogEvent("Event: Player Dash Started");

            return true;
        }
        return false;
    }

    double DashComponent::GetDashVelocityX() const
    {
        return isDashing ? (dashSpeed * dashDirection) : 0.0;
    }

    bool DashComponent::IsDashing() const
    {
        return isDashing;
    }

}