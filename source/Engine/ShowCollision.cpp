/**
 * \file
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#include "ShowCollision.hpp"
#include "Engine.hpp"
#include "Input.hpp"

CS230::ShowCollision::ShowCollision() : enabled(false) {}

void CS230::ShowCollision::Update([[maybe_unused]] double dt)
{
    if (Engine::GetInput().KeyJustReleased(CS230::Input::Keys::Tab) == true)
    {
        enabled = !enabled;
    }
}

bool CS230::ShowCollision::Enabled()
{
    return enabled;
}