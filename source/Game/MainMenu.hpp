#pragma once
#include "Engine/GameState.hpp"
#include "Engine/Texture.hpp"
#include "Engine/Rect.hpp"
#include <memory>

class MainMenu : public CS230::GameState
{
public:
    void Load() override;
    void Update(double dt) override;
    void Unload() override;
    void Draw() override;
    void DrawImGui() override;

    gsl::czstring GetName() const override { return "MainMenu"; }

private:
    std::shared_ptr<CS230::Texture> titleTexture;
    std::shared_ptr<CS230::Texture> startTexture;
    std::shared_ptr<CS230::Texture> exitTexture;

    Math::rect startButtonRect;
    Math::rect exitButtonRect;

    bool isStartHovered = false;
    bool isExitHovered = false;

    void SetupButtons();

    
};