/**
 * \file
 * \author Rudy Castan
 * \author Jonathan Holmes
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#include "Logger.hpp"
#include <iostream>

namespace CS230
{
    CS230::Logger::Logger(Logger::Severity severity, bool use_console, std::chrono::system_clock::time_point engine_start_time)
        : min_level(severity), start_time(engine_start_time), out_stream("Trace.log"), mirror_to_console(use_console)
    {
    }

    void Logger::LogError(std::string text)
    {
        log(Severity::Error, text);
    }

    void Logger::LogEvent(std::string text)
    {
        log(Severity::Event, text);
    }

    void Logger::LogDebug(std::string text)
    {
        log(Severity::Debug, text);
    }

    void Logger::LogVerbose(std::string text)
    {
        log(Severity::Verbose, text);
    }

    void Logger::log(Severity severity, std::string message)
    {
        std::string SeverityNames[] = { "Verbose", "Debug", "Event", "Error" };

        if (severity >= min_level)
        {
            out_stream.precision(4);
            out_stream << '[' << std::fixed << seconds_since_start() << "]\t";

            out_stream << SeverityNames[static_cast<int>(severity)] << "\t" << message << std::endl;

            if (mirror_to_console)
            {
                std::cout.precision(4);
                std::cout << '[' << std::fixed << seconds_since_start() << "]\t";
                std::cout << SeverityNames[static_cast<int>(severity)] << "\t" << message << std::endl;
            }
        }

        return;
    }

    double Logger::seconds_since_start() const
    {
        return std::chrono::duration<double>(std::chrono::system_clock::now() - start_time).count();
    }
}