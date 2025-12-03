/*
 * Rudy Castan
 * CS180
 *
 */

#include "JobSystem.h"



void JobSystem::DoJob(JobSystem::Job job)
{
    job();
}

void JobSystem::WaitUntilDone()
{

}

void JobSystem::DoJobs(int how_many, JobSystem::ComputeAtIndex compute)
{
    for(int i = 0; i < how_many; ++i)
        compute(i);
}
