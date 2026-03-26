#include "Mode1.hpp"
#include "Bonfire.hpp"
#include "CS200/IRenderer2D.hpp"
#include "CS200/NDC.hpp"
#include "Door.hpp"
#include "Gate.hpp"
#include "MainMenu.hpp"
#include "MiniMap.hpp"
#include "Player.hpp"
#include "PushableMirror.hpp"
#include "Sign.hpp"
#include "Star.hpp"
#include "TargetStar.hpp"
#include "WorldTextManager.hpp"

#include "Engine/AudioManager.hpp"
#include "Engine/BackgroundElement.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Font.hpp"
#include "Engine/GameObjectManager.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Input.hpp"
#include "Engine/Logger.hpp"
#include "Engine/MapElement.h"
#include "Engine/MapManager.h"
#include "Engine/ShowCollision.hpp"
#include "Engine/TextureManager.hpp"
#include "Engine/Window.hpp"

#include "OpenGL/GL.hpp"

#include <algorithm>
#include <filesystem>
#include <imgui.h>
#include <span>
#include <string>
#include <vector>

void Mode1::Load()
{
    // 1. Initialize Essential Components First
    AddGSComponent(new CS230::GameObjectManager());

    // 2. Create Player Before Loading Map (Crucial for Factory)
    player = new Player({ 0.0, -400.0 });
    Engine::GetGameStateManager().GetGSComponent<CS230::GameObjectManager>()->Add(player);

    // 3. Initialize MapManager
    mapManager = new CS230::MapManager();

    // 4. Set Factory BEFORE calling LoadMap
    mapManager->SetGameObjectFactory(
        [this](GameObjectTypes /*type*/, Math::vec2 pos, const std::string& color, const std::string& id) -> CS230::GameObject*
        {
            if (this->player == nullptr)
                return nullptr;

            if (color == "#786721" || id.find("door") != std::string::npos)
            {
                return new Door(pos, { 100, 100 });
            }
            if (color == "#808080")
            {
                Engine::GetLogger().LogEvent("Pillar created at: " + std::to_string(pos.x) + ", " + std::to_string(pos.y));
                return new CS230::BackgroundElement(pos, "Assets/textures/pillar.png");
            }

            if (color == "#00ff00")
            {
                return new Sign(pos, this->player->GetPosition(), "test message");
            }

            if (color == "#ff0000")
            {
                return new Bonfire(pos, this->player->GetPosition());
            }

            return nullptr;
        });

    // 5. Setup Camera
    camera = new CS230::Camera(
        Math::rect{
            {   0,   0 },
            { 800, 600 }
    });
    Math::ivec2 winSize = Engine::GetWindow().GetSize();
    camera->SetLimit(
        Math::irect{
            {              static_cast<int>(level1_boundary.Left()), -5000 },
            { static_cast<int>(level1_boundary.Right()) - winSize.x,  5000 }
    });
    AddGSComponent(camera);

    // 6. Load Map Data
    mapManager->AddMap(new CS230::Map("Assets/maps/Tutorial10.svg"));
    mapManager->LoadMap();
    AddGSComponent(mapManager);

    // 7. Graphics & Shaders
    backgroundShader = OpenGL::CreateShader(std::filesystem::path("Assets/shaders/Cradle.vart"), std::filesystem::path("Assets/shaders/Cradle.frag"));

    std::vector<float> quadVertices = { -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f };
    backgroundVBO                   = OpenGL::CreateBuffer(OpenGL::BufferType::Vertices, std::as_bytes(std::span{ quadVertices }));

    OpenGL::VertexBuffer vb;
    vb.Handle     = backgroundVBO;
    vb.Layout     = OpenGL::BufferLayout({ OpenGL::Attribute::Float2, OpenGL::Attribute::Float2 });
    backgroundVAO = OpenGL::CreateVertexArrayObject({ vb });

    miniMap = new MiniMap();
    miniMap->SetWorldBounds(level1_boundary);

    currentState = State::Loading;

    // Audio
    AudioManager::LoadSound("BGM_Virgo", std::filesystem::path("Assets/sounds/Virgo.wav"), AudioTypes::BGM);
    AudioManager::LoadSound("SFX_Landing", std::filesystem::path("Assets/sounds/Landing_Effect.wav"), AudioTypes::SFX);
}

void Mode1::InitGame()
{
    auto gom = GetGSComponent<CS230::GameObjectManager>();

    worldTextManager = new WorldTextManager();
    worldTextManager->SetCamera(camera);
    AddGSComponent(worldTextManager);


    if (miniMap)
    {
        miniMap->AttachPlayer(player);
        miniMap->AttachCamera(camera);
        miniMap->AttachMapManager(mapManager);
        miniMap->AttachGameObjectManager(gom);
    }

    Math::ivec2 winSize = Engine::GetWindow().GetSize();
    camera->SetPosition(player->GetPosition() - Math::vec2{ winSize.x * 0.5, winSize.y * 0.5 });

    // targetStars.clear();

    // double t1_x = 4500.0;
    // double t1_y = 750.0;
    // double t2_x = 4800.0;
    // double t2_y = 1000.0;
    // double t3_x = 5200.0;
    // double t4_x = 5500.0;
    // double t5_x = 6500.0;
    // double t6_x = 7500.0;
    // double t7_x = 8800.0;

    // TargetStar* t1 = new TargetStar({ t1_x, t1_y });
    // gom->Add(t1);
    // targetStars.push_back(t1);
    // TargetStar* t2 = new TargetStar({ t2_x, t2_y });
    // gom->Add(t2);
    // targetStars.push_back(t2);
    // TargetStar* t3 = new TargetStar({ t3_x, t2_y });
    // gom->Add(t3);
    // targetStars.push_back(t3);
    // TargetStar* t4 = new TargetStar({ t4_x, t1_y });
    // gom->Add(t4);
    // targetStars.push_back(t4);
    // TargetStar* t5 = new TargetStar({ t5_x, t1_y });
    // gom->Add(t5);
    // targetStars.push_back(t5);
    // TargetStar* t6 = new TargetStar({ t6_x, t1_y });
    // gom->Add(t6);
    // targetStars.push_back(t6);
    // TargetStar* t7 = new TargetStar({ t7_x, t1_y });
    // gom->Add(t7);
    // targetStars.push_back(t7);

    // double y1_x = 5000.0;
    // double y1_y = 750.0;
    // double r1_x = 7000.0;
    // double r1_y = 750.0;
    // double y2_x = 9000.0;
    // double y2_y = 250.0;

    // Star* yellowStar = new Star({ y1_x, y1_y }, player, targetStars, StarType::Yellow);
    // gom->Add(yellowStar);
    // Star* redStar = new Star({ r1_x, r1_y }, player, targetStars, StarType::Red);
    // gom->Add(redStar);
    // Star* yellowStar2 = new Star({ y2_x, y2_y }, player, targetStars, StarType::Yellow);
    // gom->Add(yellowStar2);

    // double     platformY  = 200;
    // double     platformY2 = 500.0;
    // Math::vec2 signSize   = { 50.0, 25.0 };

    // gom->Add(new Sign({ 0.0, platformY + 12.5 }, signSize, "A/D to Move"));
    // gom->Add(new Sign({ 200.0, platformY + 12.5 }, signSize, "W or Space to Jump"));
    // gom->Add(new Sign({ 800.0, platformY2 + 12.5 }, signSize, "Press 'F' at Bonfire to Save"));
    // gom->Add(new Sign({ 1100.0, platformY + 12.5 }, signSize, "Press 'R' to Respawn"));
    // gom->Add(new Sign({ 1900.0, platformY2 + 12.5 }, signSize, "Press LShift to Dash"));
    // gom->Add(new Sign({ 2700.0, platformY + 12.5 }, signSize, "Hold 'LShift' to Sprint"));
    // gom->Add(new Sign({ 4500.0, platformY + 12.5 }, signSize, "Hold RMB: Shield (Reflects Light)"));
    // gom->Add(new Sign({ 5000.0, platformY + 12.5 }, signSize, "Reflect Light to Hit the Star!"));
    // gom->Add(new Sign({ 5900.0, platformY + 12.5 }, signSize, "Red Lasers Hurt! Parry with Timed Block!"));
    // gom->Add(new Sign({ 7900.0, platformY + 12.5 }, signSize, "Door"));

    // Math::vec2 bonfireSize = { 25.0, 25.0 };
    // gom->Add(new Bonfire({ 900.0, platformY2 + 12.5 }, bonfireSize));
    // gom->Add(new Bonfire({ 6000.0, platformY + 12.5 }, bonfireSize));
    // gom->Add(new Bonfire({ 7800.0, platformY + 12.5 }, bonfireSize));

    // Math::vec2 doorSize = { 80, 120 };
    // gom->Add(new Door({ 10000.0, platformY + 60.0 }, doorSize));

    // PushableMirror* box = new PushableMirror({ 8500.0, 300.0 }, { 80.0, 80.0 });
    // gom->Add(box);

    // Background Music
    AudioManager::Play("BGM_Virgo");
}

void Mode1::Update(double dt)
{
    UpdateGSComponents(dt);

    // Update shader time
    shaderTime += dt;

    if (currentState == State::Loading)
    {
        if (mapManager->GetCurrentMap() && mapManager->GetCurrentMap()->IsLevelLoaded())
        {
            Engine::GetLogger().LogEvent("Mode1 Map Loading Complete! Starting Game...");
            InitGame();
            currentState = State::Playing;
        }
        return;
    }

    if (Engine::GetInput().KeyJustPressed(CS230::Input::Keys::M))
    {
        if (miniMap)
            miniMap->ToggleMode();
    }

    GetGSComponent<CS230::GameObjectManager>()->UpdateAll(dt);
    GetGSComponent<CS230::GameObjectManager>()->CollisionTest();

    if (player != nullptr && player->interactionTarget == nullptr)
        player->isInteracting = false;

    if (player != nullptr)
    {
        Math::vec2 winSize   = static_cast<Math::vec2>(Engine::GetWindow().GetSize());
        Math::vec2 targetPos = player->GetPosition() - Math::vec2{ winSize.x * 0.5, winSize.y * 0.3 };

        double minX = level1_boundary.Left();
        double maxX = level1_boundary.Right() - winSize.x;

        if (targetPos.x < minX)
            targetPos.x = minX;
        if (targetPos.x > maxX)
            targetPos.x = maxX;

        camera->SetPosition(targetPos);
    }

    if (Engine::GetInput().KeyJustPressed(CS230::Input::Keys::P))
    {
        player->SetPosition({ 8000, 300 });
    }

    if (Engine::GetInput().KeyJustPressed(CS230::Input::Keys::Escape))
    {
        Engine::GetGameStateManager().Clear();
        Engine::GetGameStateManager().PushState<MainMenu>();
    }

    miniMap->Update(dt);
}

void Mode1::Draw()
{
    CS200::IRenderer2D& renderer         = Engine::GetRenderer2D();
    Math::ivec2         display_size_int = Engine::GetWindow().GetSize();

    if (currentState == State::Loading)
    {
        Engine::GetWindow().Clear(CS200::BLACK);
        Math::TransformationMatrix screen_matrix = CS200::build_ndc_matrix(display_size_int);
        renderer.BeginScene(screen_matrix);

        CS230::Font& font       = Engine::GetFont(0);
        auto         loadingTex = font.PrintToTexture("Loading Tutorial...", CS200::WHITE);
        if (loadingTex)
        {
            Math::ivec2                texSize   = loadingTex->GetSize();
            Math::vec2                 centerPos = { display_size_int.x * 0.5, display_size_int.y * 0.5 };
            Math::vec2                 drawPos   = centerPos - Math::vec2{ texSize.x * 0.5, texSize.y * 0.5 };
            Math::TransformationMatrix transform = Math::TranslationMatrix(drawPos);
            loadingTex->Draw(transform);
        }
        renderer.EndScene();
        return;
    }

    Engine::GetWindow().Clear(CS200::BLACK);

    // Draw Shader Background
    {
        GL::UseProgram(backgroundShader.Shader);

        GLint resLoc  = GL::GetUniformLocation(backgroundShader.Shader, "u_resolution");
        GLint timeLoc = GL::GetUniformLocation(backgroundShader.Shader, "u_time");

        // u_resolution
        GL::Uniform2f(resLoc, static_cast<float>(display_size_int.x), static_cast<float>(display_size_int.y));

        // u_time
        GL::Uniform1f(timeLoc, static_cast<float>(shaderTime));

        GL::BindVertexArray(backgroundVAO);
        GL::DrawArrays(GL_TRIANGLE_FAN, 0, 4);
        GL::BindVertexArray(0);
        GL::UseProgram(0);
    }

    // Math::vec2 camPos  = camera->GetPosition();
    // Math::vec2 winSize = static_cast<Math::vec2>(display_size_int);

    // [Removed] DrawParallaxLayer definition and usage

    Math::TransformationMatrix view_projection_matrix = CS200::build_ndc_matrix(display_size_int) * camera->GetMatrix();
    renderer.BeginScene(view_projection_matrix);
    GetGSComponent<CS230::GameObjectManager>()->DrawAll(view_projection_matrix);
    renderer.EndScene();

    Math::TransformationMatrix screen_matrix = CS200::build_ndc_matrix(display_size_int);
    renderer.BeginScene(screen_matrix);
    
    renderer.EndScene();
}

void Mode1::DrawImGui()
{
#ifdef DEVELOPER_VERSION
    ImGui::Begin("Mode1 Debug");
    if (ImGui::CollapsingHeader("Global Info", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("FPS: %d", Engine::GetWindowEnvironment().FPS);
        if (camera)
        {
            Math::vec2 camPos = camera->GetPosition();
            ImGui::Text("Camera Pos: (%.1f, %.1f)", camPos.x, camPos.y);
        }
    }
    if (ImGui::CollapsingHeader("Object Inspector", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto gom = GetGSComponent<CS230::GameObjectManager>();
        if (gom)
            gom->DrawAllImGui();
    }
    ImGui::End();
#endif
    ImGui::Begin("Mode1 Release");
    if (miniMap)
        miniMap->DrawImGui();
    ImGui::End();
}

void Mode1::Unload()
{
    // Cleanup shader resources
    OpenGL::DestroyShader(backgroundShader);
    // GL::DeleteVertexArrays(1, reinterpret_cast<GLuint*>(&backgroundVAO));
    // GL::DeleteBuffers(1, reinterpret_cast<GLuint*>(&backgroundVBO));

    ClearGSComponents();
    player           = nullptr;
    mapManager       = nullptr;
    worldTextManager = nullptr;
    
    // [Removed] textureLayer2_Trees cleanup
    textureLayer3_Silhouette = nullptr;

    delete miniMap;
    miniMap = nullptr;
}