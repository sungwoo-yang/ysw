/**
 * \file
 * \author Rudy Castan
 * \author Jonathan Holmes
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#pragma once
#include <chrono>
#include <fstream>
#include <map>
#include <string>

namespace CS230
{
    class Logger
    {
    public:
        enum class Severity
        {
            Verbose, // Minor messages
            Debug,   // Only used while actively debugging
            Event,   // General event, like key press or state change
            Error    // Errors, such as file load errors
        };
        Logger(Severity severity, bool use_console, std::chrono::system_clock::time_point start_time);

        void LogError(std::string text);

        void LogEvent(std::string text);

        void LogDebug(std::string text);

        void LogVerbose(std::string text);

    private:
        void                                  log(Severity severity, std::string message);
        double                                seconds_since_start() const;
        Severity                              min_level;
        std::chrono::system_clock::time_point start_time;
        std::ofstream                         out_stream;
        bool                                  mirror_to_console;
    };
}