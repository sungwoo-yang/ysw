/*
 * Rudy Castan
 * CS180
 *
 */
#pragma once

#include <functional>

class JobSystem
{
public:
    using Job = std::function<void(void)>;
    void DoJob(Job job);
    void WaitUntilDone();

    using ComputeAtIndex = std::function<void(int)>;
    void DoJobs(int how_many, ComputeAtIndex compute);
};
