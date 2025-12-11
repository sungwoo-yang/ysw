
#include "Dash.hpp"
#include "Engine.hpp"
#include "Logger.hpp"

namespace CS230
{
    void DashComponent::UpdateTimers(double delta_time)
    {
        if (dashCooldownTimer > 0.0)
        {
            dashCooldownTimer -= delta_time;
        }

        if (isDashing)
        {
            dashTimer -= delta_time;
            if (dashTimer <= 0.0)
            {
                isDashing = false;
            }
        }
    }

    bool DashComponent::TryStartDash(bool currentFaceRight)
    {
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
        if (isDashing)
        {
            return dashSpeed * dashDirection;
        }
        return 0.0;
    }

    bool DashComponent::IsDashing() const
    {
        return isDashing;
    }

}