#include "OriMode.hpp"
#include "Player.hpp"

#include "CS200/IRenderer2D.hpp"
#include "CS200/NDC.hpp"
#include "Engine/Camera.hpp"
#include "Engine/Engine.hpp"
#include "Engine/GameObjectManager.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Input.hpp"
#include "Engine/Font.hpp"
#include "Engine/Logger.hpp"
#include "Engine/MapManager.h"
#include "Engine/Window.hpp"
#include "Game/ObjectFactory.hpp"
#include <imgui.h>

void OriMode::Load() {
    AddGSComponent(new CS230::GameObjectManager());

    player = new Player({ 400.0, -2400.0 });

    mapManager = new CS230::MapManager();
    mapManager->SetGameObjectFactory(ObjectFactory::Create(player, {}));
    mapManager->AddMap(new CS230::Map("Assets/maps/main.svg"));
    mapManager->LoadMap();
    AddGSComponent(mapManager);

    Math::ivec2 winSize = Engine::GetWindow().GetSize();
    camera = new CS230::Camera(Math::rect{ { 0, 0 }, { static_cast<double>(winSize.x), static_cast<double>(winSize.y) } });
    camera->SetLimit(Math::irect{
        { static_cast<int>(boundary.Left()),  -5000 },
        { static_cast<int>(boundary.Right()) - winSize.x, 5000 }
    });
    AddGSComponent(camera);

    postProcessor.Initialize(winSize);

    state = State::Loading;
}

void OriMode::InitGame() {
    auto* gom = GetGSComponent<CS230::GameObjectManager>();
    gom->Add(player);

    Math::ivec2 winSize = Engine::GetWindow().GetSize();
    camera->SetPosition(player->GetPosition() - Math::vec2{ winSize.x * 0.5, winSize.y * 0.5 });
}

void OriMode::Update(double dt) {
    UpdateGSComponents(dt);

    if (state == State::Loading) {
        auto* map = mapManager->GetCurrentMap();
        if (map && map->IsLevelLoaded()) {
            InitGame();
            state = State::Playing;
        }
        return;
    }

    auto* gom = GetGSComponent<CS230::GameObjectManager>();
    gom->UpdateAll(dt);
    gom->CollisionTest();

    Math::ivec2 winSize     = Engine::GetWindow().GetSize();
    Math::vec2  scaledWin   = static_cast<Math::vec2>(winSize);
    Math::vec2  targetPos   = player->GetPosition() - Math::vec2{ scaledWin.x * 0.5, scaledWin.y * 0.5 };
    targetPos.x = std::clamp(targetPos.x, boundary.Left(), boundary.Right() - scaledWin.x);
    camera->Update(targetPos, dt);
}

void OriMode::Draw() {
    CS200::IRenderer2D& renderer  = Engine::GetRenderer2D();
    Math::ivec2         winSize   = Engine::GetWindow().GetSize();

    if (state == State::Loading) {
        Engine::GetWindow().Clear(CS200::BLACK);
        Math::TransformationMatrix screen = CS200::build_ndc_matrix(winSize);
        renderer.BeginScene(screen);
        auto loadTex = Engine::GetFont(0).PrintToTexture("Loading...", CS200::WHITE);
        if (loadTex) {
            Math::vec2 pos = static_cast<Math::vec2>(winSize) * 0.5 - static_cast<Math::vec2>(loadTex->GetSize()) * 0.5;
            loadTex->Draw(Math::TranslationMatrix(pos));
        }
        renderer.EndScene();
        return;
    }

    Math::TransformationMatrix vp = CS200::build_ndc_matrix(winSize) * camera->GetMatrix();

    // Scene pass: map + all game objects
    postProcessor.BeginSceneRender();
    renderer.BeginScene(vp);
    GetGSComponent<CS230::GameObjectManager>()->DrawAll(vp);
    renderer.EndScene();

    // Bloom mask pass: player sprite only (source of the glow aura)
    postProcessor.BeginBloomMaskRender();
    renderer.BeginScene(vp);
    {
        constexpr double FEET_OFFSET = 40.0;
        Math::vec2 feet = { player->GetPosition().x, player->GetPosition().y - FEET_OFFSET };
        player->oriAnim.Draw(feet, player->faceRight);
    }
    renderer.EndScene();

    // Composite: bloom glow applied to scene, output to screen
    postProcessor.EndRenderAndDraw();
}

void OriMode::Unload() {
    ClearGSComponents();
    postProcessor.Shutdown();
    player     = nullptr;
    mapManager = nullptr;
    camera     = nullptr;
    state      = State::Loading;
}

void OriMode::DrawImGui() {
    if (ImGui::Begin("OriMode")) {
        if (player) {
            Math::vec2 pos = player->GetPosition();
            ImGui::Text("Player pos: (%.1f, %.1f)", pos.x, pos.y);
        }
        ImGui::Text("State: %s", state == State::Loading ? "Loading" : "Playing");
    }
    ImGui::End();
}
