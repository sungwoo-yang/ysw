#pragma once

#include "Input.hpp"
#include "Vec2.hpp"

namespace CS230
{
    struct DashComponent
    {
        // Dash State
        bool   isDashing         = false;
        double dashTimer         = 0.0;
        double dashCooldownTimer = 0.0;
        int    dashDirection     = 1;

        // Dash Settings
        double dashSpeed            = 1000.0;
        double dashDuration         = 0.15;
        double dashCooldown         = 3.0;
        bool   disableGravityOnDash = true;

        // Upadate timers
        void   UpdateTimers(double delta_time);

        // Try to start dash
        bool   TryStartDash(bool currentFaceRight);

        // Get current dash velocity
        [[nodiscard]] double GetDashVelocityX() const;

        // Check if dashing
        [[nodiscard]] bool   IsDashing() const;
    };

}