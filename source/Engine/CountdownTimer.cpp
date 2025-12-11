#include "CountdownTimer.hpp"

namespace CS230
{
    CountdownTimer::CountdownTimer(double time_remaining)
    {
        Set(time_remaining);
    }

    void CountdownTimer::Set(double time_remaining)
    {
        timer_max = time_remaining;
        Reset();
        pendulum = false;
    }

    void CountdownTimer::Reset()
    {
        timer = timer_max;
    }

    void CountdownTimer::Update(double dt)
    {
        if (timer > 0.0)
        {
            timer -= dt;
            if (timer < 0.0)
            {
                timer = 0.0;
            }
            pendulum = !pendulum;
        }
    }

    double CountdownTimer::Remaining()
    {
        return timer;
    }

    int CountdownTimer::RemainingInt()
    {
        return static_cast<int>(timer);
    }

    bool CountdownTimer::TickTock()
    {
        return pendulum;
    }
}