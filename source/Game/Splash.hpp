#pragma once
#include "Engine/GameState.hpp"

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
    double timer = 0.0;
    const double displayTime = 3.0;
};