#pragma once
#include "Engine/GameState.hpp"
#include <vector>
#include <string>

class Splash : public CS230::GameState
{
public:
    void Load() override;
    void Update(double dt) override;
    void Unload() override;
    void Draw() override;
    void DrawImGui() override;
    
    gsl::czstring GetName() const override { return "Splash"; }

private:
    // Stages of the splash screen
    enum class Stage {
        DigipenLogo,
        WaitBetween,
        TeamLogo,
        Finished
    };

    Stage  currentStage = Stage::DigipenLogo;
    double timer = 0.0;
    float  alpha = 0.0f; // For fade effect (0.0 to 1.0)
    
    // Timing constants
    const double fadeTime = 1.0;     // Fade in (1s) + Fade out (1s)
    const double displayTime = 1.5;  // Stay fully visible
    const double waitTime = 2.0;    // Black screen interval
};