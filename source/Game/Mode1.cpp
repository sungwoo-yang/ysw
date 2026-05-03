#include "Mode1.hpp"

#include "Door.hpp"
#include "FallCutscene.hpp"
#include "MainMenu.hpp"
#include "MiniMap.hpp"
#include "Mode3.hpp"
#include "ObjectFactory.hpp"
#include "Player.hpp"
#include "WorldTextManager.hpp"

#include "CS200/IRenderer2D.hpp"
#include "CS200/NDC.hpp"

#include "Engine/AudioManager.hpp"
#include "Engine/Camera.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Font.hpp"
#include "Engine/GameObjectManager.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Input.hpp"
#include "Engine/Logger.hpp"
#include "Engine/MapManager.h"
#include "Engine/Window.hpp"

#include "OpenGL/GL.hpp"

#include <filesystem>
#include <imgui.h>
#include <span>
#include <vector>

void Mode1::Load()
{
    Engine::GetGameStateManager().HoldFadeIn(true);
    // 1. Initialize Essential Components First
    AddGSComponent(new CS230::GameObjectManager());

    // 2. Create Player Before Loading Map (Crucial for Factory)
    player = new Player({ 0.0, 400.0 });

    // 3. Initialize MapManager
    mapManager = new CS230::MapManager();

    signTexts = {
        { "sign_01",                  "A/D to Move" },
        { "sign_02",           "W or Space to Jump" },
        { "sign_03", "Press 'F' at Bonfire to Save" },
        { "sign_04",         "Press LShift to Dash" }
    };

    // 4. Set Factory BEFORE calling LoadMap
    mapManager->SetGameObjectFactory(ObjectFactory::Create(player, signTexts));

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

    // Load level map from SVG data
    // mapManager = new CS230::MapManager();
    mapManager->AddMap(new CS230::Map("Assets/maps/tutorial05.svg"));
    mapManager->LoadMap();
    AddGSComponent(mapManager);

    backgroundShader = OpenGL::CreateShader(std::filesystem::path("Assets/shaders/Cradle.vert"), std::filesystem::path("Assets/shaders/Cradle.frag"));


    // Create Full Screen Quad (NDC coordinates: -1 to 1)
    std::vector<float> quadVertices = { -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f };


    backgroundVBO = OpenGL::CreateBuffer(OpenGL::BufferType::Vertices, std::as_bytes(std::span{ quadVertices }));

    OpenGL::VertexBuffer vb;
    vb.Handle = backgroundVBO;
    vb.Layout = OpenGL::BufferLayout(
        {
            OpenGL::Attribute::Float2, // Location 0: aPos
            OpenGL::Attribute::Float2  // Location 1: aTexCoord
        });

    backgroundVAO = OpenGL::CreateVertexArrayObject({ vb });

    miniMap = new MiniMap();
    miniMap->SetWorldBounds(level1_boundary);

    AudioManager::LoadSound("BGM_Virgo", std::filesystem::path("Assets/sounds/Virgo.mp3"), AudioTypes::BGM);
    AudioManager::LoadSound("SFX_Landing", std::filesystem::path("Assets/sounds/Landing_Effect.mp3"), AudioTypes::SFX);
}

void Mode1::InitGame()
{
    auto gom = GetGSComponent<CS230::GameObjectManager>();

    if (player != nullptr)
    {
        gom->Add(player);
    }

    worldTextManager = new WorldTextManager();
    worldTextManager->SetCamera(camera);
    AddGSComponent(worldTextManager);

    // Create player and link to UI systems
    player = new Player({ 0.0, 400.0 });
    gom->Add(player);

    if (miniMap)
    {
        miniMap->AttachPlayer(player);
        miniMap->AttachCamera(camera);
        miniMap->AttachMapManager(mapManager);
        miniMap->AttachGameObjectManager(gom);
    }

    Math::ivec2 winSize = Engine::GetWindow().GetSize();
    camera->SetPosition(player->GetPosition() - Math::vec2{ winSize.x * 0.5, winSize.y * 0.5 });

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

            Engine::GetGameStateManager().HoldFadeIn(false);
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

    if (player != nullptr && player->isInteracting && player->interactionTarget != nullptr)

    {
        Door* interactedDoor = dynamic_cast<Door*>(player->interactionTarget);

        if (interactedDoor != nullptr && interactedDoor->ConsumeInteractionRequest())
        {
            const std::string& doorName = interactedDoor->GetName();

            if (doorName == "DOOR_FALL_TO_MODE3")
            {
                FallCutscene::SetNextState([]() { Engine::GetGameStateManager().ChangeStateWithFade<Mode3>(); });

                Engine::GetGameStateManager().ChangeStateWithFade<FallCutscene>();
                return;
            }

            Engine::GetLogger().LogError("Mode1 unknown door id: " + doorName);
        }
    }

    if (player != nullptr)
    {
        Math::vec2 winSize = static_cast<Math::vec2>(Engine::GetWindow().GetSize());

        float currentScale = 0.8f;
        camera->SetScale(currentScale);

        Math::vec2 scaledWinSize = winSize / currentScale;

        Math::vec2 targetPos = player->GetPosition() - Math::vec2{ scaledWinSize.x * 0.5, scaledWinSize.y * 0.5 };

        double minX = level1_boundary.Left();
        double maxX = level1_boundary.Right() - scaledWinSize.x;

        double minY = -800.0;
        double maxY = 800.0;

        if (targetPos.x < minX)
            targetPos.x = minX;
        if (targetPos.x > maxX)
            targetPos.x = maxX;
        if (targetPos.y < minY)
            targetPos.y = minY;
        if (targetPos.y > maxY)
            targetPos.y = maxY;

        camera->Update(targetPos, dt);
    }
    if (Engine::GetInput().KeyJustPressed(CS230::Input::Keys::P))
    {
        player->SetPosition({ 4200, 300 });
    }

    if (Engine::GetInput().KeyJustPressed(CS230::Input::Keys::Escape))
    {
        // Engine::GetGameStateManager().Clear();
        // Engine::GetGameStateManager().PushState<MainMenu>();
        Engine::GetGameStateManager().ChangeStateWithFade<MainMenu>();
    }

    miniMap->Update(dt);
}

// void Mode1::Update(double dt)
// {
//     UpdateGSComponents(dt);

//     shaderTime += dt;

//     if (currentState == State::Loading)
//     {
//         if (mapManager->GetCurrentMap() && mapManager->GetCurrentMap()->IsLevelLoaded())
//         {
//             Engine::GetLogger().LogEvent("Mode1 Map Loading Complete! Starting Game...");
//             InitGame();
//             currentState = State::Playing;
//         }
//         return;
//     }

//     if (Engine::GetInput().KeyJustPressed(CS230::Input::Keys::M))
//     {
//         if (miniMap)
//             miniMap->ToggleMode();
//     }

//     GetGSComponent<CS230::GameObjectManager>()->UpdateAll(dt);
//     GetGSComponent<CS230::GameObjectManager>()->CollisionTest();

//     if (player != nullptr && player->interactionTarget == nullptr)
//         player->isInteracting = false;

//     if (player != nullptr)
//     {
//         Math::vec2 winSize   = static_cast<Math::vec2>(Engine::GetWindow().GetSize());

//         Math::vec2 targetPos = player->GetPosition() - Math::vec2{ winSize.x * 0.5, winSize.y * 0.5 };


//         camera->Update(targetPos, dt);
//     }

//     if (Engine::GetInput().KeyJustPressed(CS230::Input::Keys::P))
//     {
//         player->SetPosition({ 8000, 300 });
//     }

//     if (Engine::GetInput().KeyJustPressed(CS230::Input::Keys::Escape))
//     {
//         Engine::GetGameStateManager().Clear();
//         Engine::GetGameStateManager().PushState<MainMenu>();
//     }

//     miniMap->Update(dt);
// }

void Mode1::Draw()
{
    CS200::IRenderer2D& renderer         = Engine::GetRenderer2D();
    Math::ivec2         display_size_int = Engine::GetWindow().GetSize();

    if (currentState == State::Loading)
    {
        // Simple loading screen draw logic
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

    // Main world rendering pass
    Math::TransformationMatrix view_projection_matrix = CS200::build_ndc_matrix(display_size_int) * camera->GetMatrix();
    renderer.BeginScene(view_projection_matrix);

    // if (texture20 != nullptr)
    // {
    //     Math::vec2 svg_center = { 4750.0, 0.0 };
    //     Math::TransformationMatrix bg_transform = Math::TranslationMatrix(svg_center);
    //     texture20->Draw(view_projection_matrix * bg_transform);
    // }

    GetGSComponent<CS230::GameObjectManager>()->DrawAll(view_projection_matrix);
    renderer.EndScene();

    // HUD / Screen-space UI rendering
    Math::TransformationMatrix screen_matrix = CS200::build_ndc_matrix(display_size_int);
    renderer.BeginScene(screen_matrix);

    if (worldTextManager != nullptr)
    {
        worldTextManager->Draw();
    }

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
    AudioManager::StopBGM();

    OpenGL::DestroyShader(backgroundShader);
    GL::DeleteVertexArrays(1, &backgroundVAO);
    GL::DeleteBuffers(1, &backgroundVBO);

    ClearGSComponents();
    player           = nullptr;
    mapManager       = nullptr;
    worldTextManager = nullptr;
    // targetStars.clear();

    textureLayer3_Silhouette = nullptr;
    // texture20 = nullptr;
    delete miniMap;
    miniMap = nullptr;
}
