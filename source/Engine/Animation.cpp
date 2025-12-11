/**
 * \file
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#include "Animation.hpp"
#include "Engine.hpp"
#include "Logger.hpp"
#include "Path.hpp"
#include <fstream>

CS230::Animation::Animation(const std::filesystem::path& animation_file) : current_command(0)
{
    if (animation_file.extension() != ".anm")
    {
        throw std::runtime_error(animation_file.generic_string() + " is not a .anm file");
    }
    std::ifstream in_file(assets::locate_asset(animation_file));
    if (in_file.is_open() == false)
    {
        throw std::runtime_error("Failed to load " + assets::locate_asset(animation_file).generic_string());
    }

    std::string command;
    while (in_file.eof() == false)
    {
        in_file >> command;
        if (command == "PlayFrame")
        {
            int   frame;
            float target_time;
            in_file >> frame;
            in_file >> target_time;

            commands.push_back(new PlayFrame(frame, target_time));
        }
        else if (command == "Loop")
        {
            int loop_to_frame;
            in_file >> loop_to_frame;
            commands.push_back(new Loop(loop_to_frame));
        }
        else if (command == "End")
        {
            commands.push_back(new End());
        }
        else
        {
            Engine::GetLogger().LogError(command + " in " + animation_file.generic_string());
        }
    }
    Reset();
}

CS230::Animation::Animation() : Animation("./Assets/None.anm")
{
}

CS230::Animation::~Animation()
{
    for (Command* command : commands)
    {
        delete command;
    }
    commands.clear();
}

int CS230::Animation::CurrentFrame()
{
    return current_frame->Frame();
}

void CS230::Animation::Reset()
{
    current_command = 0;
    ended           = false;
    current_frame   = static_cast<PlayFrame*>(commands[static_cast<size_t>(current_command)]);
    current_frame->ResetTime();
}

bool CS230::Animation::Ended()
{
    return ended;
}

void CS230::Animation::Update(double dt)
{
    current_frame->Update(dt);
    if (current_frame->Ended() == true)
    {
        current_frame->ResetTime();
        current_command++;
        if (commands[static_cast<size_t>(current_command)]->Type() == CommandType::PlayFrame)
        {
            current_frame = static_cast<PlayFrame*>(commands[static_cast<size_t>(current_command)]);
        }
        else if (commands[static_cast<size_t>(current_command)]->Type() == CommandType::Loop)
        {
            Loop* loop_data = static_cast<Loop*>(commands[static_cast<size_t>(current_command)]);
            current_command = loop_data->LoopIndex();
            if (commands[static_cast<size_t>(current_command)]->Type() == CommandType::PlayFrame)
            {
                current_frame = static_cast<PlayFrame*>(commands[static_cast<size_t>(current_command)]);
            }
            else
            {
                Engine::GetLogger().LogError("Loop does not go to PlayFrame");
                Reset();
            }
        }
        else if (commands[static_cast<size_t>(current_command)]->Type() == CommandType::End)
        {
            ended = true;
        }
    }
}

PlayFrame::PlayFrame(int fr, double dura) : frame(fr), target_time(dura), timer(0.0)
{
}

void PlayFrame::Update(double dt)
{
    timer += dt;
}

bool PlayFrame::Ended()
{
    return timer >= target_time;
}

int PlayFrame::Frame()
{
    return frame;
}

void PlayFrame::ResetTime()
{
    timer = 0.0;
}

Loop::Loop(int li) : loop_index(li)
{
}

int Loop::LoopIndex()
{
    return loop_index;
}