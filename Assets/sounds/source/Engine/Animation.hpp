/**
 * \file
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once
#include <filesystem>
#include <vector>

enum class CommandType
{
    PlayFrame,
    Loop,
    End
};

class Command
{
public:
    virtual ~Command()
    {
    }

    virtual CommandType Type() = 0;
};

class End : public Command
{
public:
    virtual CommandType Type() override
    {
        return CommandType::End;
    }

private:
};

class Loop : public Command
{
public:
    Loop(int loop_index);

    virtual CommandType Type() override
    {
        return CommandType::Loop;
    }

    int LoopIndex();

private:
    int loop_index;
};

class PlayFrame : public Command
{
public:
    PlayFrame(int fr, double dura);

    virtual CommandType Type() override
    {
        return CommandType::PlayFrame;
    }

    void Update(double dt);
    bool Ended();
    void ResetTime();
    int  Frame();

private:
    int    frame;
    double target_time;
    double timer;
};

namespace CS230
{
    class Animation
    {
    public:
        Animation();
        Animation(const std::filesystem::path& animation_file);
        ~Animation();

        void Update(double dt);
        int  CurrentFrame();
        void Reset();
        bool Ended();

    private:
        int                   current_command;
        std::vector<Command*> commands;
        bool                  ended;
        PlayFrame*            current_frame;
    };
}