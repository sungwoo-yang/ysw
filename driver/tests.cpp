/*
 * Rudy Castan
 * CS180
 */

#include <JobSystem.h>

#include "tests.h"

#include <algorithm>
#include <atomic>
#include <iostream>
#include <numeric>
#include <thread>


void SpinTest(JobSystem& job_system);
void SpinTestSimulate60FPS(JobSystem& job_system);
void ComputationTestManyElements(JobSystem& job_system);
void NestedScheduling(JobSystem& job_system);
void ComputationTestRightAmount(JobSystem& job_system);

int main()
{
    JobSystem job_system;

    SpinTest(job_system);
    SpinTestSimulate60FPS(job_system);
    NestedScheduling(job_system);
    ComputationTestManyElements(job_system);
    ComputationTestRightAmount(job_system);

    return 0;
}

void Spin(std::chrono::milliseconds how_long) noexcept
{
    using namespace std::chrono;
    using namespace std::chrono_literals;
    const high_resolution_clock::time_point start_point  = high_resolution_clock::now();
    milliseconds                            elapsed_time = 0ms;
    while (elapsed_time < how_long)
    {
        const auto time_span = high_resolution_clock::now() - start_point;
        elapsed_time         = duration_cast<milliseconds>(time_span);
    }
}

void SpinTest(JobSystem& job_system)
{
    using namespace std;
    using namespace std::chrono_literals;
    using namespace std::string_literals;
    const auto thread_count = thread::hardware_concurrency();
    cout << thread_count << " hardware threads available\n";
    cout << "Spin " << thread_count << " times sequentially :\n";
    {
        Timer t{"Spin 100ms "s + to_string(thread_count) + " times sequentially "s};
        for (unsigned i = 0; i < thread_count; ++i)
        {
            Spin(100ms);
        }
    }
    {
        Timer t{"Spin 100ms "s + to_string(thread_count) + " times asynchronously "s};
        for (unsigned i = 0; i < thread_count; ++i)
        {
            job_system.DoJob([]() { Spin(100ms); });
        }
        job_system.WaitUntilDone();
    }
}

void SpinTestSimulate60FPS(JobSystem& job_system)
{
    using namespace std;
    using namespace std::chrono_literals;
    using namespace std::string_literals;
    const int NumJobs = 16;
    cout << "Spin " << NumJobs << " times sequentially :\n";
    {
        Timer t{"Spin 1ms "s + to_string(NumJobs) + " times sequentially "s};
        for (unsigned i = 0; i < NumJobs; ++i)
        {
            Spin(1ms);
        }
    }
    {
        Timer t{"Spin 1ms "s + to_string(NumJobs) + " times asynchronously "s};
        for (unsigned i = 0; i < NumJobs; ++i)
        {
            job_system.DoJob([]() { Spin(1ms); });
        }
        job_system.WaitUntilDone();
    }
}

void NestedScheduling(JobSystem& job_system)
{
    using namespace std;
    using namespace std::chrono_literals;
    using namespace std::string_literals;
    const int       NumJobs      = 100;
    std::atomic_int jobs_created = 0;
    {
        Timer t{"Spin with nested jobs "s};
        job_system.DoJobs(NumJobs,
                          [&job_system, &jobs_created](int i)
                          {
                              while (i > 0)
                              {
                                  job_system.DoJob(
                                      [&jobs_created]()
                                      {
                                          ++jobs_created;
                                          Spin(1ms);
                                      });
                                  --i;
                              }
                              Spin(10ms);
                          });
        job_system.WaitUntilDone();
        std::cout << "Created " << jobs_created << " nested jobs\n";
    }
}

void ComputationTestManyElements(JobSystem& job_system)
{
    using namespace std;
    constexpr int data_count = 100'000;
    cout << "Computation Test with " << data_count << " elements.\n";
    using SIZE = decltype(vector<DummyData>{}.size());
    vector<DummyData> data(static_cast<SIZE>(data_count));
    const auto sum_data = [](double l, DummyData& r) { return l + std::accumulate(r.m, r.m + DummyData::COUNT, 0.0); };
    {
        Timer t{"Computing "s + to_string(data_count) + " elements sequentially "s};
        int   index = 0;
        for (auto& d : data)
        {
            d.Reset();
            d.Compute(index++);
        }
        cout << "sum : " << accumulate(begin(data), end(data), 0.0, sum_data) << '\n';
    }
    {
        Timer t{"Computing "s + to_string(data_count) + " elements asynchronously "s};
        job_system.DoJobs(data_count,
                          [&data](int index)
                          {
                              const auto i = static_cast<SIZE>(index);
                              data[i].Reset();
                              data[i].Compute(index);
                          });
        job_system.WaitUntilDone();
        cout << "sum : " << accumulate(begin(data), end(data), 0.0, sum_data) << '\n';
    }
}

void ComputationTestRightAmount(JobSystem& job_system)
{
    using namespace std;
    vector<int> counts;
    using SIZE = decltype(counts.size());

    const auto set_to_ones = [&](int index)
    {
        const auto i = static_cast<SIZE>(index);
        counts[i]    = 1;
    };
    const auto count_the_ones = [&]() { return std::accumulate(begin(counts), end(counts), 0); };

    const auto count_jobs_run = [&](auto how_many)
    {
        if (how_many <= 0)
            how_many = 0;
        counts.resize(static_cast<SIZE>(how_many));
        std::fill(begin(counts), end(counts), 0);
        job_system.DoJobs(static_cast<int>(counts.size()), set_to_ones);
        job_system.WaitUntilDone();
        cout << "Did " << count_the_ones() << " jobs\n";
    };

    const auto thread_count = thread::hardware_concurrency();

    for (int i = -1; i < 16; ++i)
        count_jobs_run(i);
    count_jobs_run(thread_count * 10);
    count_jobs_run(thread_count - 1);
    count_jobs_run((thread_count - 1) * thread_count);
    count_jobs_run(thread_count + 1);
    count_jobs_run(thread_count * thread_count * 1000 + 3);
}
