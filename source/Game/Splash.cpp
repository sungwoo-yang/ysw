#include "Splash.hpp"
#include "CS200/IRenderer2D.hpp"
#include "CS200/NDC.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Input.hpp"
#include "Engine/TextureManager.hpp"
#include "Engine/Window.hpp"
#include "Engine/Texture.hpp" // Required for Texture->Draw()
#include "MainMenu.hpp"

void Splash::Load()
{
    currentStage = Stage::DigipenLogo;
    timer = 0.0;
    alpha = 0.0f;

    // Pre-load textures using Load() as it acts as a Getter too
    auto& texManager = Engine::GetTextureManager();
    texManager.Load("assets/images/DigipenLogo.png");
    texManager.Load("assets/images/TeamLogo.png");
}

void Splash::Update(double dt)
{
    timer += dt;
    auto& input = Engine::GetInput();

    // Skip to Main Menu on Mouse Left Click
    if (input.MouseButtonJustPressed(CS230::Input::MouseButton::Left)) 
    {
        Engine::GetGameStateManager().PushState<MainMenu>();
        return;
    }

    switch (currentStage)
    {
    case Stage::DigipenLogo:
        if (timer < fadeTime) alpha = static_cast<float>(timer / fadeTime);
        else if (timer < fadeTime + displayTime) alpha = 1.0f;
        else if (timer < fadeTime * 2 + displayTime) alpha = 1.0f - static_cast<float>((timer - (fadeTime + displayTime)) / fadeTime);
        else {
            currentStage = Stage::WaitBetween;
            timer = 0.0;
            alpha = 0.0f;
        }
        break;

    case Stage::WaitBetween:
        if (timer >= waitTime) {
            currentStage = Stage::TeamLogo;
            timer = 0.0;
        }
        break;

    case Stage::TeamLogo:
        if (timer < fadeTime) alpha = static_cast<float>(timer / fadeTime);
        else if (timer < fadeTime + displayTime) alpha = 1.0f;
        else if (timer < fadeTime * 2 + displayTime) alpha = 1.0f - static_cast<float>((timer - (fadeTime + displayTime)) / fadeTime);
        else currentStage = Stage::Finished;
        break;

    case Stage::Finished:
        Engine::GetGameStateManager().PushState<MainMenu>();
        break;
    }
}

void Splash::Draw()
{
    Engine::GetWindow().Clear(CS200::BLACK); 
    auto& renderer = Engine::GetRenderer2D();
    Math::ivec2 winSize = Engine::GetWindow().GetSize();
    Math::TransformationMatrix screen_matrix = CS200::build_ndc_matrix(winSize);

    renderer.BeginScene(screen_matrix);

    if (currentStage != Stage::WaitBetween)
    {
        std::string currentAsset = (currentStage == Stage::DigipenLogo) ? "Assets/images/DigipenLogo.png" : "Assets/images/TeamLogo.png";
        auto texture = Engine::GetTextureManager().Load(currentAsset); 

        if (texture)
        {
            Math::ivec2 texSize = texture->GetSize();
            Math::vec2 centerPos = { winSize.x * 0.5f, winSize.y * 0.5f };
            
            Math::TransformationMatrix transform = 
                Math::TranslationMatrix(centerPos) * Math::ScaleMatrix({ static_cast<double>(texSize.x), static_cast<double>(texSize.y) });

            renderer.DrawQuad(
                transform, 
                texture->GetHandle(), 
                Math::vec2{ 0.0, 0.0 }, 
                Math::vec2{ 1.0, 1.0 }, 
                CS200::pack_color({ 1.0f, 1.0f, 1.0f, alpha })
            ); 
        }
    }

    renderer.EndScene();
}

void Splash::Unload() {}
void Splash::DrawImGui() {}