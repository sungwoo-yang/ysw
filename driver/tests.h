/*
 * Rudy Castan
 * CS180
 */
#pragma once

#include <chrono>
#include <cmath>
#include <iostream>
#include <string>

class Timer
{
    const std::string                                    name;
    const std::chrono::high_resolution_clock::time_point startPoint;

public:
    Timer(std::string label) : name(std::move(label)), startPoint(std::chrono::high_resolution_clock::now()) {}
    ~Timer()
    {
        using namespace std::chrono;
        const auto end_point = high_resolution_clock::now();
        std::cout << name << " : " << duration_cast<milliseconds>(end_point - startPoint).count() << " ms\n";
    }

public:
    Timer(const Timer& other)     = delete;
    Timer(Timer&& other) noexcept = default;
    Timer& operator=(const Timer& other) = delete;
    Timer& operator=(Timer&& other) noexcept = delete;
};


struct DummyData
{
    static constexpr int COUNT    = 16;
    double               m[COUNT] = {0.0};

    void Reset()
    {
        bool do_positive = true;
        for (auto& i : m)
        {
            i           = do_positive ? 1.0 : -1.0;
            do_positive = !do_positive;
        }
    }

    void Compute(int value)
    {
        if (value == 0)
            value = 1;
        for (int i = 0; i < COUNT; ++i)
        {
            m[i] = m[i] * std::sin(3.14 * 2.0 * std::cos(value + i)) * 360.0;
        }
    }
};
