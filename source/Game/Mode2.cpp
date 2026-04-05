#include "Mode2.hpp"
#include "BossStar.hpp"
#include "Gate.hpp"
#include "HostileStar.hpp"
#include "Mirror.hpp"
#include "Player.hpp"
#include "PuzzleStar.hpp"
#include "RedHitParticle.hpp"
#include "Sign.hpp"
#include "Star.hpp"
#include "TargetStar.hpp"
#include "WorldTextManager.hpp"
#include "YellowLaser.hpp"

#include "CS200/IRenderer2D.hpp"
#include "CS200/NDC.hpp"

#include "Engine/Camera.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Font.hpp"
#include "Engine/GameObjectManager.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Logger.hpp"
#include "Engine/MapManager.h"
#include "Engine/ShowCollision.hpp"
#include "Engine/Window.hpp"

#include <imgui.h>

std::vector<std::string> SplitID(const std::string& s, char delimiter)
{
    std::vector<std::string> tokens;
    std::string              token;
    std::istringstream       tokenStream(s);
    while (std::getline(tokenStream, token, delimiter))
    {
        tokens.push_back(token);
    }

    return tokens;
}

void Mode2::Load()
{
    Engine::GetGameStateManager().HoldFadeIn(true);

    currentState = State::Loading;

    AddGSComponent(new CS230::GameObjectManager());

    AddGSComponent(new CS230::ParticleManager<RedHitParticle>());
#ifdef DEVELOPER_VERSION
    AddGSComponent(new CS230::ShowCollision());
#endif

    camera = new CS230::Camera(
        Math::rect{
            {   0,   0 },
            { 800, 600 }
    });
    camera->SetLimit(
        Math::irect{
            {  static_cast<int>(level_boundary.Left()), static_cast<int>(level_boundary.Bottom()) },
            { static_cast<int>(level_boundary.Right()),    static_cast<int>(level_boundary.Top()) }
    });
    AddGSComponent(camera);

    // Load boss-specific map geometry
    mapManager = new CS230::MapManager();

    InitGame();

    mapManager->AddMap(new CS230::Map("Assets/maps/Boss01.svg"));
    mapManager->LoadMap();
    AddGSComponent(mapManager);
}

void Mode2::InitGame()
{
    auto gom = GetGSComponent<CS230::GameObjectManager>();

    player = new Player({ 0.0, 400.0 });
    gom->Add(player);

    // Star
    mapManager->SetGameObjectFactory(
        [this](GameObjectTypes /*type*/, Math::vec2 pos, const std::string& color, const std::string& id) -> CS230::GameObject*
        {
            if (this->player == nullptr)
                return nullptr;

            // TargetStar : T_(Num)
            if (color == "#ffffff")
            {
                auto* target = new TargetStar(pos);
                target->SetName(id);

                return target;
            }

            // PuzzleStar : P_(Type)_(Pattern)_(Direction)_(Num)
            if (color == "#00ff00")
            {
                auto parts = SplitID(id, '_');

                if (parts.size() >= 5 && parts[0] == "P")
                {
                    PuzzleStar::LaserType type = PuzzleStar::LaserType::White;
                    if (parts[1] == "Y")
                    {
                        type = PuzzleStar::LaserType::Yellow;
                    }
                    else if (parts[1] == "R")
                    {
                        type = PuzzleStar::LaserType::Red;
                    }

                    PuzzleStar::Pattern pattern = PuzzleStar::Pattern::Static;
                    if (parts[2] == "R")
                    {
                        pattern = PuzzleStar::Pattern::Rotating;
                    }
                    else if (parts[2] == "B")
                    {
                        pattern = PuzzleStar::Pattern::Blink;
                    }

                    Math::vec2  dir  = { 0, -1 };
                    std::string dStr = parts[3];

                    if (dStr == "N")
                    {
                        dir = { 0, 1 };
                    }
                    else if (dStr == "S")
                    {
                        dir = { 0, -1 };
                    }
                    else if (dStr == "E")
                    {
                        dir = { 1, 0 };
                    }
                    else if (dStr == "W")
                    {
                        dir = { -1, 0 };
                    }
                    else if (dStr == "NE")
                    {
                        dir = { 0.707, 0.707 };
                    }
                    else if (dStr == "NW")
                    {
                        dir = { -0.707, 0.707 };
                    }
                    else if (dStr == "SE")
                    {
                        dir = { 0.707, -0.707 };
                    }
                    else if (dStr == "SW")
                    {
                        dir = { -0.707, -0.707 };
                    }

                    auto* star = new PuzzleStar(pos, this->player, type, pattern, dir);
                    star->SetName(id);

                    return star;
                }
            }
            // HostileStar : H_(Type)_(Num)
            if (color == "#ffff00")
            {
                HostileStar::StarType type = HostileStar::StarType::Yellow;

                if (id.find("_R_") != std::string::npos)
                {
                    type = HostileStar::StarType::Red;
                }

                auto* star = new HostileStar(pos, this->player, type);
                star->SetName(id);

                return star;
            }
            // BossStar : B_(Num)
            if (color == "#ff00ff")
            {
                auto* star = new BossStar(pos, this->player);
                star->SetName(id);

                return star;
            }

            return nullptr;
        });
}

void Mode2::Update(double dt)
{
    UpdateGSComponents(dt);

    if (currentState == State::Loading)
    {
        if (mapManager->GetCurrentMap() && mapManager->GetCurrentMap()->IsLevelLoaded())
        {
            Engine::GetLogger().LogEvent("Map Loading Complete! Starting Game...");
            currentState = State::Playing;

            Engine::GetGameStateManager().HoldFadeIn(false);

            playingTimer    = 0.0;
            isCameraScaling = false;
        }
        return;
    }

    if (currentState == State::Playing)
    {
        playingTimer += dt;

        if (playingTimer >= 1.0 && !isCameraScaling)
        {
            isCameraScaling = true;
        }

        if (isCameraScaling && camera != nullptr)
        {
            double currentScale = camera->GetScale();
            if (currentScale > targetCameraScale)
            {
                currentScale -= dt * cameraScaleSpeed;
                if (currentScale < targetCameraScale)
                {
                    currentScale = targetCameraScale;
                }
                camera->SetScale(currentScale);
            }
        }
    }

    auto gom = GetGSComponent<CS230::GameObjectManager>();

    gom->UpdateAll(dt);
    gom->CollisionTest();

    if (puzzleGate == nullptr)
    {
        for (auto obj : gom->GetObjects())
        {
            if (obj->TypeName() == "Gate")
            {
                puzzleGate = static_cast<Gate*>(obj);
                break;
            }
        }
    }

    if (puzzleTarget != nullptr && puzzleTarget->IsHit())
    {
        if (puzzleGate != nullptr && !puzzleGate->IsOpen())
        {
            puzzleGate->Open();
        }
    }

    if (player != nullptr)
    {
        Math::vec2 winSize   = static_cast<Math::vec2>(Engine::GetWindow().GetSize());
        Math::vec2 targetPos = player->GetPosition() - Math::vec2{ winSize.x * 0.5, winSize.y * 0.3 };
        camera->SetPosition(targetPos);
    }
}

void Mode2::Draw()
{
    CS200::IRenderer2D& renderer         = Engine::GetRenderer2D();
    Math::ivec2         display_size_int = Engine::GetWindow().GetSize();

    if (currentState == State::Loading)
    {
        // Draw boss stage loading text
        Engine::GetWindow().Clear(CS200::BLACK);
        Math::TransformationMatrix screen_matrix = CS200::build_ndc_matrix(display_size_int);
        renderer.BeginScene(screen_matrix);

        CS230::Font& font       = Engine::GetFont(0);
        auto         loadingTex = font.PrintToTexture("Loading Boss Stage...", CS200::WHITE);
        if (loadingTex)
        {
            Math::ivec2 texSize   = loadingTex->GetSize();
            Math::vec2  centerPos = { display_size_int.x * 0.5, display_size_int.y * 0.5 };
            Math::vec2  drawPos   = centerPos - Math::vec2{ texSize.x * 0.5, texSize.y * 0.5 };

            Math::TransformationMatrix transform = Math::TranslationMatrix(drawPos);
            loadingTex->Draw(transform);
        }
        renderer.EndScene();
        return;
    }

    Engine::GetWindow().Clear(0x202020FF);

    // Standard game object rendering pass
    Math::TransformationMatrix view_projection_matrix = CS200::build_ndc_matrix(display_size_int) * camera->GetMatrix();
    renderer.BeginScene(view_projection_matrix);
    GetGSComponent<CS230::GameObjectManager>()->DrawAll(view_projection_matrix);
    renderer.EndScene();

    // World text overlay (Sign contents, etc.)
    Math::TransformationMatrix screen_matrix = CS200::build_ndc_matrix(display_size_int);
    renderer.BeginScene(screen_matrix);
    if (worldTextManager != nullptr)
    {
        worldTextManager->Draw();
    }
    renderer.EndScene();
}

void Mode2::DrawImGui()
{
#ifdef DEVELOPER_VERSION
    ImGui::Begin("Mode2 (Boss) Debug");

    if (ImGui::CollapsingHeader("Global Info", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("FPS: %d", Engine::GetWindowEnvironment().FPS);

        const char* stateStr = "Unknown";
        switch (currentState)
        {
            case State::Loading: stateStr = "Loading"; break;
            case State::Playing: stateStr = "Playing"; break;
        }
        ImGui::Text("Current State: %s", stateStr);

        if (camera)
        {
            Math::vec2 camPos = camera->GetPosition();
            ImGui::Text("Camera Pos: (%.1f, %.1f)", camPos.x, camPos.y);
        }

        ImGui::Text("Level Boundary: L:%.0f R:%.0f B:%.0f T:%.0f", level_boundary.Left(), level_boundary.Right(), level_boundary.Bottom(), level_boundary.Top());
    }

    if (ImGui::CollapsingHeader("Object Inspector", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto gom = GetGSComponent<CS230::GameObjectManager>();
        if (gom)
        {
            gom->DrawAllImGui();
        }
    }

    ImGui::End();
#endif
}

void Mode2::Unload()
{
    ClearGSComponents();
    player           = nullptr;
    mapManager       = nullptr;
    worldTextManager = nullptr;
}