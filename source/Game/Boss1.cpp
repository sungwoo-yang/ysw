#include "Boss1.hpp"

#include "BossController.hpp"
#include "Constellation.hpp"
#include "Gate.hpp"
#include "LaserStar.hpp"
#include "ObjectFactory.hpp"
#include "Player.hpp"
#include "RedHitParticle.hpp"
#include "TargetStar.hpp"
#include "WorldTextManager.hpp"

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

void Boss1::BuildConstellation()
{
    auto gom = GetGSComponent<CS230::GameObjectManager>();

    if (gom == nullptr)
    {
        return;
    }

    constellation = new Constellation("Aries");

    for (auto obj : gom->GetObjects())
    {
        if (obj == nullptr)
        {
            continue;
        }

        const std::string& name = obj->GetName();

        if (obj->TypeName() == "LaserStar" && name == "LS_Y_SHOT_TRACK_E_ARIESMAIN")
        {
            constellation->SetMainStar(static_cast<LaserStar*>(obj));
        }
        else if (obj->TypeName() == "TargetStar")
        {
            if (name == "ARIES_T_01" || name == "ARIES_T_02" || name == "ARIES_T_03")
            {
                constellation->AddTargetStar(static_cast<TargetStar*>(obj));
            }
        }
    }

    Engine::GetLogger().LogEvent("Aries boss built. Targets: " + std::to_string(constellation->GetTotalTargetCount()));
}

void Boss1::Load()
{
    Engine::GetGameStateManager().HoldFadeIn(true);

    currentState = State::Loading;

    AddGSComponent(new CS230::GameObjectManager());
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
            {                                        0,                                  -5000 },
            { static_cast<int>(level_boundary.Right()), static_cast<int>(level_boundary.Top()) }
    });
    AddGSComponent(camera);

    // Load boss-specific map geometry
    mapManager = new CS230::MapManager();

    InitGame();

    mapManager->AddMap(new CS230::Map("Assets/maps/Boss.svg"));
    mapManager->LoadMap();
    AddGSComponent(mapManager);

    AddGSComponent(new CS230::ParticleManager<RedHitParticle>());
}

void Boss1::InitGame()
{
    auto gom = GetGSComponent<CS230::GameObjectManager>();

    player = new Player({ 50.0, 200.0 });
    gom->Add(player);

    mapManager->SetGameObjectFactory(ObjectFactory::Create(player));
}

void Boss1::Update(double dt)
{
    UpdateGSComponents(dt);

    if (currentState == State::Loading)
    {
        if (mapManager->GetCurrentMap() && mapManager->GetCurrentMap()->IsLevelLoaded())
        {
            Engine::GetLogger().LogEvent("Map Loading Complete! Starting Game...");

            BuildConstellation();

            bossController = new BossController(player, constellation);
            bossController->CollectPhaseObjects(GetGSComponent<CS230::GameObjectManager>());

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

        if (bossController != nullptr)
        {
            bossController->Update(dt);
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
        camera->Update(targetPos, dt);
    }
}

void Boss1::Draw()
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

void Boss1::DrawImGui()
{
#ifdef DEVELOPER_VERSION
    ImGui::Begin("Boss1 (Boss) Debug");

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

    if (constellation != nullptr)
    {
        ImGui::Text("Constellation: %s", constellation->GetName().c_str());
        ImGui::Text("Targets: %d / %d", constellation->GetActivatedTargetCount(), constellation->GetTotalTargetCount());
        ImGui::Text("Restored: %s", constellation->IsRestored() ? "Yes" : "No");
    }

    ImGui::End();
#endif
}

void Boss1::Unload()
{
    delete constellation;
    constellation = nullptr;

    delete bossController;
    bossController = nullptr;

    ClearGSComponents();
    player           = nullptr;
    mapManager       = nullptr;
    worldTextManager = nullptr;
}