#include "Mode2.hpp"
#include "CS200/IRenderer2D.hpp"
#include "CS200/NDC.hpp"
#include "Engine/Camera.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Font.hpp"
#include "Engine/GameObjectManager.hpp"
#include "Engine/Logger.hpp"
#include "Engine/MapManager.h"
#include "Engine/ShowCollision.hpp"
#include "Engine/Window.hpp"
#include "Game/Sign.hpp"
#include "Gate.hpp"
#include "Player.hpp"
#include "Star.hpp"
#include "TargetStar.hpp"
#include "WorldTextManager.hpp"
#include "YellowLaser.hpp"
#include <imgui.h>

void Mode2::Load()
{
    currentState = State::Loading;

    AddGSComponent(new CS230::GameObjectManager());
#ifdef DEVERLOPER_VERSION
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

    mapManager = new CS230::MapManager();
    mapManager->AddMap(new CS230::Map("Assets/maps/Boss.svg"));
    mapManager->LoadMap();
    AddGSComponent(mapManager);
}

void Mode2::InitGame()
{
    auto gom = GetGSComponent<CS230::GameObjectManager>();

    player = new Player({ 0.0, 400.0 });
    gom->Add(player);

    puzzleTarget = new TargetStar({ 500.0, 400.0 });
    gom->Add(puzzleTarget);

    std::vector<TargetStar*> targets     = { puzzleTarget };
    Star*                    laserSource = new Star({ 300.0, 600.0 }, player, targets, StarType::Yellow);
    gom->Add(laserSource);

    gom->Add(new Sign({ 200.0, 250.0 }, { 80.0, 40.0 }, "Hit Target -> Open Gate"));
}

void Mode2::Update(double dt)
{
    UpdateGSComponents(dt);

    if (currentState == State::Loading)
    {
        if (mapManager->GetCurrentMap() && mapManager->GetCurrentMap()->IsLevelLoaded())
        {
            Engine::GetLogger().LogEvent("Map Loading Complete! Starting Game...");
            InitGame();
            currentState = State::Playing;
        }
        return;
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

    GetGSComponent<CS230::GameObjectManager>()->UpdateAll(dt);
    GetGSComponent<CS230::GameObjectManager>()->CollisionTest();

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

    Math::TransformationMatrix view_projection_matrix = CS200::build_ndc_matrix(display_size_int) * camera->GetMatrix();
    renderer.BeginScene(view_projection_matrix);
    GetGSComponent<CS230::GameObjectManager>()->DrawAll(view_projection_matrix);
    renderer.EndScene();

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

    ImGui::Text("FPS: %d", Engine::GetWindowEnvironment().FPS);
    if (camera)
    {
        Math::vec2 camPos = camera->GetPosition();
        ImGui::Text("Camera Pos: (%.1f, %.1f)", camPos.x, camPos.y);
    }

    ImGui::Separator();

    auto gom = GetGSComponent<CS230::GameObjectManager>();
    if (gom)
    {
        gom->DrawAllImGui();
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