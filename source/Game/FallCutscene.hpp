#pragma once

#include "Engine/GameState.hpp"
#include "Engine/Vec2.hpp"
#include <gsl/gsl>

class FallCutscene : public CS230::GameState
{
public:
    static void SetNextState(std::function<void()> nextStateAction);

    void Load() override;
    void Update(double dt) override;
    void Unload() override;
    void Draw() override;
    void DrawImGui() override;

    gsl::czstring GetName() const override
    {
        return "FallCutscene";
    }

private:
    static std::function<void()> pendingNextStateAction;

    double     timer        = 0.0;
    double     fallDuration = 2.5;
    Math::vec2 playerPos{};
    Math::vec2 playerSize{ 40.0, 80.0 };
};