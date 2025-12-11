#pragma once
#include "Component.hpp"

namespace CS230
{
    class CountdownTimer : public Component
    {
    public:
        CountdownTimer(double time_remaining);
        void   Set(double time_remaining);
        void   Update(double dt) override;
        void   Reset();
        double Remaining();
        int    RemainingInt();
        bool   TickTock();

    private:
        double timer;
        double timer_max;
        bool   pendulum;
    };
}
