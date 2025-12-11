#pragma once

#include "Input.hpp"
#include "Vec2.hpp"

namespace CS230
{
    struct DashComponent
    {
        bool   isDashing         = false;
        double dashTimer         = 0.0;
        double dashCooldownTimer = 0.0;
        int    dashDirection     = 1;

        double dashSpeed            = 1000.0;
        double dashDuration         = 0.15;
        double dashCooldown         = 3.0;
        bool   disableGravityOnDash = true;

        void   UpdateTimers(double delta_time);
        bool   TryStartDash(bool currentFaceRight);
        double GetDashVelocityX() const;
        bool   IsDashing() const;
    };

}