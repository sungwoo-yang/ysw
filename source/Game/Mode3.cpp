#include "Mode3.hpp"

#include "LevelEditor.hpp"
#include "MainMenu.hpp"

#include "BossConfig.hpp"
#include "BreakableWall.hpp"
#include "BullBoss.hpp"
#include "CutscenePlayer.hpp"
#include "Door.hpp"
#include "DoorActionHandler.hpp"
#include "Gate.hpp"
#include "InGameScriptEditor.hpp"
#include "LevelStreamer.hpp"
#include "LightOrb.hpp"
#include "LightOrbManager.hpp"
#include "MiniMap.hpp"
#include "ObjectFactory.hpp"
#include "Player.hpp"
#include "SaveManager.hpp"
#include "ScriptManager.hpp"
#include "ShieldChargeShot.hpp"
#include "ShieldEnergy.hpp"
#include "SimpleBossStar.hpp"
#include "TargetStar.hpp"
#include "TutorialOverlay.hpp"
#include "WorldTextManager.hpp"

#include "CS200/IRenderer2D.hpp"
#include "CS200/NDC.hpp"

#include "Engine/AudioManager.hpp"
#include "Engine/BackgroundElement.hpp"
#include "Engine/Camera.hpp"
#include "Engine/Collision.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Font.hpp"
#include "Engine/GameObjectManager.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Input.hpp"
#include "Engine/Logger.hpp"
#include "Engine/MapElement.h"
#include "Engine/MapManager.h"
#include "Engine/Path.hpp"
#include "Engine/SettingsManager.hpp"
#include "Engine/ShowCollision.hpp"
#include "Engine/TextureManager.hpp"
#include "Engine/Window.hpp"

#include "OpenGL/GL.hpp"

#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <iterator>
#include <regex>
#include <span>
#include <vector>

std::optional<Math::vec2> Mode3::pendingReturnPosition = std::nullopt;
bool                      Mode3::s_startInEditor       = false;

namespace
{
    constexpr double BASH_RADIUS      = 160.0;
    constexpr double BASH_SLOW_FACTOR = 0.12;

    constexpr double SIMPLE_BOSS_ROOM_LEFT   = 8840.0;
    constexpr double SIMPLE_BOSS_ROOM_RIGHT  = 11240.0;
    constexpr double SIMPLE_BOSS_ROOM_BOTTOM = 2160.0;
    constexpr double SIMPLE_BOSS_ROOM_TOP    = 3520.0;
    constexpr double SIMPLE_BOSS_CAMERA_PAD  = 180.0;

    Math::rect GetSimpleBossRoomBounds()
    {
        return Math::rect{
            {  SIMPLE_BOSS_ROOM_LEFT, SIMPLE_BOSS_ROOM_BOTTOM },
            { SIMPLE_BOSS_ROOM_RIGHT,    SIMPLE_BOSS_ROOM_TOP }
        };
    }

    bool IsPointInRect(Math::vec2 p, const Math::rect& r)
    {
        return p.x >= r.Left() && p.x <= r.Right() && p.y >= r.Bottom() && p.y <= r.Top();
    }

    std::optional<Math::vec2> LoadEditorSpawnMarker()
    {
        std::filesystem::path path;
        try
        {
            path = assets::locate_asset("Assets/maps/editor_output.svg");
        }
        catch (...)
        {
            return std::nullopt;
        }

        std::ifstream f(path);
        if (!f.is_open())
            return std::nullopt;

        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        std::replace(content.begin(), content.end(), '\n', ' ');
        std::replace(content.begin(), content.end(), '\r', ' ');

        static const std::regex rCircle(R"xxx(<circle\s[^>]*>)xxx", std::regex::icase);
        static const std::regex rSpawnFill(R"xxx(fill\s*=\s*"#ff8800")xxx", std::regex::icase);
        static const std::regex rSpawnId(R"xxx(\bid\s*=\s*"PlayerSpawn")xxx", std::regex::icase);
        static const std::regex rCX(R"xxx(\bcx\s*=\s*"([^"]+)")xxx");
        static const std::regex rCY(R"xxx(\bcy\s*=\s*"([^"]+)")xxx");

        auto it  = std::sregex_iterator(content.begin(), content.end(), rCircle);
        auto end = std::sregex_iterator{};
        for (; it != end; ++it)
        {
            const std::string tag = (*it)[0].str();
            if (!std::regex_search(tag, rSpawnFill) && !std::regex_search(tag, rSpawnId))
                continue;

            std::smatch mx;
            std::smatch my;
            if (!std::regex_search(tag, mx, rCX) || !std::regex_search(tag, my, rCY))
                continue;

            try
            {
                return Math::vec2{ std::stod(mx[1].str()), -std::stod(my[1].str()) };
            }
            catch (...)
            {
                return std::nullopt;
            }
        }

        return std::nullopt;
    }
}

void Mode3::SetReturnPosition(Math::vec2 position)
{
    pendingReturnPosition = position;

    Engine::GetLogger().LogEvent("Mode3 return position saved: (" + std::to_string(position.x) + ", " + std::to_string(position.y) + ")");
}

void Mode3::ClearReturnPosition()
{
    pendingReturnPosition = std::nullopt;
}

void Mode3::Load()
{
    Engine::GetGameStateManager().HoldFadeIn(true);

    AddGSComponent(new CS230::GameObjectManager());

    spawnPos = LoadEditorSpawnMarker().value_or(Math::vec2{ -10.0, 0.0 });

    // Priority: editor return > save file checkpoint > editor map spawn marker > fallback
    if (pendingReturnPosition.has_value())
    {
        spawnPos              = pendingReturnPosition.value();
        pendingReturnPosition = std::nullopt;
    }
    else if (SaveManager::HasSave())
    {
        if (auto sd = SaveManager::Load())
            spawnPos = { sd->spawnX, sd->spawnY };
    }
    Math::vec2 spawnPosition = spawnPos;
    deathTimer               = -1.0;
    respawnDone              = false;

    player = new Player(spawnPosition);

    mapManager = new CS230::MapManager();

    signTexts = {
        { "sign_01",                  "A/D to Move" },
        { "sign_02",           "W or Space to Jump" },
        { "sign_03", "Press 'F' at Bonfire to Save" },
        { "sign_04",         "Press LShift to Dash" }
    };

    mapManager->SetGameObjectFactory(ObjectFactory::Create(player, signTexts));

    camera = new CS230::Camera(
        Math::rect{
            {   0,   0 },
            { 800, 600 }
    });

    Math::ivec2 winSize = Engine::GetWindow().GetSize();
    postProcessor.Initialize(winSize);

    camera->SetLimit(
        Math::irect{
            {  static_cast<int>(level_boundary.Left()) - 2000, -10000 },
            { static_cast<int>(level_boundary.Right()) + 2000,  10000 }
    });

    AddGSComponent(camera);

    mapManager->AddMap(new CS230::Map("Assets/maps/editor_output.svg"));
    mapManager->LoadMap();
    AddGSComponent(mapManager);

    backgroundShader = OpenGL::CreateShader(std::filesystem::path("Assets/shaders/Cradle.vert"), std::filesystem::path("Assets/shaders/Cradle.frag"));

    std::vector<float> quadVertices = { -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f };

    backgroundVBO = OpenGL::CreateBuffer(OpenGL::BufferType::Vertices, std::as_bytes(std::span{ quadVertices }));

    OpenGL::VertexBuffer vb;
    vb.Handle = backgroundVBO;
    vb.Layout = OpenGL::BufferLayout({ OpenGL::Attribute::Float2, OpenGL::Attribute::Float2 });

    backgroundVAO = OpenGL::CreateVertexArrayObject({ vb });

    miniMap = new MiniMap();
    miniMap->SetWorldBounds(level_boundary);

    AudioManager::LoadSound("BGM_Virgo", std::filesystem::path("Assets/sounds/Virgo.mp3"), AudioTypes::BGM);
    AudioManager::LoadSound("SFX_Landing", std::filesystem::path("Assets/sounds/Landing_Effect.mp3"), AudioTypes::SFX);

    // Auto-enter the level editor on first launch.
    // s_startInEditor is reset to false here so subsequent Mode3 loads
    // (after returning from the editor) start the game normally.
    if (s_startInEditor)
    {
        s_startInEditor = false;
        LevelEditor::SetGameObjects(GetGSComponent<CS230::GameObjectManager>());
        LevelEditor::SetSpawnPos(spawnPosition);
        LevelEditor::SetInitialCamera(spawnPosition, 1.0);
        Engine::GetGameStateManager().PushState<LevelEditor>();
    }
}

bool Mode3::CanPause() const
{
    if (miniMap && miniMap->IsVisible())
        return false;
    return currentState == State::Playing;
}

void Mode3::InitGame()
{
    auto gom = GetGSComponent<CS230::GameObjectManager>();

    if (player != nullptr)
    {
        gom->Add(player);
    }

    // shieldEnergy = new Boss::ShieldEnergy();
    // shieldEnergy->SetEnergy(shieldEnergy->GetMaxEnergy());
    // shieldChargeShot = new Boss::ShieldChargeShot(player, shieldEnergy);

    worldTextManager = new WorldTextManager();
    worldTextManager->SetCamera(camera);
    AddGSComponent(worldTextManager);

    tutorialOverlay = new TutorialOverlay();
    AddGSComponent(tutorialOverlay);

    cutscenePlayer = new CutscenePlayer();
    cutscenePlayer->SetRefs(player, tutorialOverlay, camera, Engine::GetWindow().GetSize());
    AddGSComponent(cutscenePlayer);

    scriptManager = new ScriptManager();
    scriptManager->SetCutscenePlayer(cutscenePlayer);
    scriptManager->LoadTriggers();
    AddGSComponent(scriptManager);

    scriptEditorIG = new InGameScriptEditor();
    scriptEditorIG->SetRefs(camera, cutscenePlayer, scriptManager, Engine::GetWindow().GetSize());
    scriptEditorIG->LoadTriggers();

    // Room bounds: global union for minimap + per-room clamping for camera
    {
        const auto& rooms = mapManager->GetAllRooms();
        if (!rooms.empty())
        {
            double minX = rooms[0].Left(), maxX = rooms[0].Right();
            double minY = rooms[0].Bottom(), maxY = rooms[0].Top();
            for (const auto& r : rooms)
            {
                minX = std::min(minX, r.Left());
                maxX = std::max(maxX, r.Right());
                minY = std::min(minY, r.Bottom());
                maxY = std::max(maxY, r.Top());
            }
            worldBounds = Math::rect{
                { minX, minY },
                { maxX, maxY }
            };
            cameraBounds = worldBounds;
        }
    }

    if (miniMap)
    {
        miniMap->SetWorldBounds(worldBounds ? *worldBounds : level_boundary);
        miniMap->AttachPlayer(player);
        miniMap->AttachCamera(camera);
        miniMap->AttachMapManager(mapManager);
        miniMap->AttachGameObjectManager(gom);
        miniMap->SetMode(MiniMapMode::Full);
        miniMap->SetVisible(false);
    }

    // Set initial camera position
    {
        Math::ivec2 winSize = Engine::GetWindow().GetSize();
        camera->SetPosition(player->GetPosition() - Math::vec2{ winSize.x * 0.5, winSize.y * 0.5 });
    }

    // Simple boss fight object must be added before LevelStreamer::Init
    // so streaming can assign it to the boss room.
    InitSimpleBossFight(gom);

    // Level streaming: assign objects to rooms, start with all active
    levelStreamer = new LevelStreamer();
    levelStreamer->Init(mapManager->GetAllRooms(), gom->GetObjects());

    // Apply saved game state (abilities, HP, gate states)
    if (SaveManager::HasSave())
    {
        if (auto sd = SaveManager::Load())
            ApplySave(*sd);
    }

    AudioManager::Play("BGM_Virgo");
}

void Mode3::Update(double dt)
{
    UpdateGSComponents(dt);
    shaderTime += dt;
    postProcessor.Tick(dt);

    if (currentState == State::Loading)
    {
        if (mapManager->GetCurrentMap() && mapManager->GetCurrentMap()->IsLevelLoaded())
        {
            Engine::GetLogger().LogEvent("Mode3 Map Loading Complete! Starting Game...");
            InitGame();
            currentState = State::Playing;
            Engine::GetGameStateManager().HoldFadeIn(false);
        }
        return;
    }

    // Map open: lock player, handle ESC to close
    const bool mapVisible = miniMap && miniMap->IsVisible();
    if (player)
        player->inputLocked = mapVisible;

    if (mapVisible)
    {
        if (Engine::GetInput().KeyJustPressed(CS230::Input::Keys::Escape))
            miniMap->SetVisible(false);
        // Skip game-object updates while map is open
        goto camera_update;
    }

    // M key: open full-screen map
    if (Engine::GetInput().KeyJustPressed(CS230::Input::Keys::M))
    {
        if (miniMap)
            miniMap->SetVisible(true);
    }

#ifdef DEVELOPER_VERSION
    // Tab: switch to level editor (debug builds only)
    {
        const Uint8* sdlKeys = SDL_GetKeyboardState(nullptr);
        if (sdlKeys[SDL_SCANCODE_TAB])
        {
            const Math::vec2 playerPos = player ? player->GetPosition() : Math::vec2{ 300.0, -3000.0 };
            LevelEditor::SetGameObjects(GetGSComponent<CS230::GameObjectManager>());
            LevelEditor::SetSpawnPos(playerPos);
            LevelEditor::SetInitialCamera(playerPos, 1.0);
            Engine::GetGameStateManager().PushState<LevelEditor>();
            return;
        }
    }
#endif

    // ---- Bash system ----
    if (player && player->bashEnabled)
    {
        bool           foundTarget   = false;
        LaserTurret*   nearestTurret = nullptr;
        BashTargetKind nearestKind   = BashTargetKind::None;
        double         nearestDist   = BASH_RADIUS * BASH_RADIUS;

        for (auto* obj : GetGSComponent<CS230::GameObjectManager>()->GetObjects())
        {
            if (obj->Type() != GameObjectTypes::LaserTurret)
                continue;

            auto* turret = static_cast<LaserTurret*>(obj);

            for (const Math::vec2 bulletPos : turret->GetBashableBulletPositions())
            {
                const double d2 = (bulletPos - player->GetPosition()).LengthSquared();
                if (d2 <= nearestDist)
                {
                    nearestDist   = d2;
                    nearestTurret = turret;
                    nearestKind   = BashTargetKind::LaserTurret;
                    foundTarget   = true;
                }
            }
        }

        if (simpleBossStar != nullptr && simpleBossStar->HasBashableShot())
        {
            const Math::vec2 shotPos = simpleBossStar->GetNearestBashableShotPosition(player->GetPosition());

            const double d2 = (shotPos - player->GetPosition()).LengthSquared();

            if (d2 <= nearestDist)
            {
                nearestDist   = d2;
                nearestTurret = nullptr;
                nearestKind   = BashTargetKind::SimpleBoss;
                foundTarget   = true;
            }
        }

        if (foundTarget)
        {
            // Reset window timer when acquiring a new target
            if (!bashReady || bashTargetKind != nearestKind || bashTarget != nearestTurret)
                bashWindowTimer = 0.0;

            bashWindowTimer += dt;
            bashReady      = true;
            bashTarget     = nearestTurret;
            bashTargetKind = nearestKind;

            timeScaleTarget = (bashWindowTimer < BASH_WINDOW) ? BASH_SLOW_FACTOR : 1.0;
        }
        else
        {
            bashReady       = false;
            bashTarget      = nullptr;
            bashTargetKind  = BashTargetKind::None;
            timeScaleTarget = 1.0;
            bashWindowTimer = 0.0;
        }

        // Smooth time-scale transition (tutorial Frozen state overrides to gradual stop)
        if (parryTut.state == ParryTutState::Frozen)
            timeScale += (0.0 - timeScale) * std::min(dt * 8.0, 1.0);
        else
            timeScale += (timeScaleTarget - timeScale) * std::min(dt * 14.0, 1.0);

        // ---- Execute bash on left click ----
        if (bashReady && player && Engine::GetInput().MouseButtonJustPressed(CS230::Input::MouseButton::Left))
        {
            const Math::ivec2 win        = Engine::GetWindow().GetSize();
            const Math::vec2  ms         = Engine::GetInput().GetMousePosition();
            const Math::vec2  mouseWorld = camera->GetPosition() + Math::vec2{ ms.x, win.y - ms.y };

            Math::vec2 bulletPos;
            Math::vec2 bulletDir;

            if (!GetCurrentBashTargetInfo(bulletPos, bulletDir))
            {
                bashReady       = false;
                bashTarget      = nullptr;
                bashTargetKind  = BashTargetKind::None;
                timeScale       = 1.0;
                timeScaleTarget = 1.0;
                bashWindowTimer = 0.0;
            }
            else
            {
                Math::vec2 toMouse = mouseWorld - bulletPos;

                if (toMouse.LengthSquared() < 1.0)
                    toMouse = { 1.0, 0.0 };
                const Math::vec2 bashDir = toMouse.Normalize();

                const double     dot        = bulletDir.Dot(bashDir);
                const double     forceMult  = 1.0 - 0.5 * dot;
                constexpr double BASE_FORCE = 950.0;
                const Math::vec2 impulse    = -bashDir * (BASE_FORCE * forceMult);

                if (BashCurrentTarget(bashDir))
                    player->ApplyBashImpulse(impulse);

                bashReady       = false;
                bashTarget      = nullptr;
                bashTargetKind  = BashTargetKind::None;
                timeScale       = 1.0;
                timeScaleTarget = 1.0;
                bashWindowTimer = 0.0;
            }
        }
    }
    else
    {
        bashReady       = false;
        bashTarget      = nullptr;
        bashTargetKind  = BashTargetKind::None;
        timeScale       = 1.0;
        timeScaleTarget = 1.0;
        bashWindowTimer = 0.0;
    }

    // ── Hardcoded parry tutorial ─────────────────────────────────────────────
    if (player && parryTut.state != ParryTutState::Done)
    {
        switch (parryTut.state)
        {
            case ParryTutState::Idle:
                if (player->GetPosition().x >= 2500.0)
                    parryTut.state = ParryTutState::Braking;
                break;

            case ParryTutState::Braking:
                {
                    const double px        = player->GetPosition().x;
                    const double py        = player->GetPosition().y;
                    const double dist      = 2800.0 - px;
                    player->autoMoveActive = false;

                    if (dist < 8.0)
                    {
                        player->SetPosition({ 2800.0, py });
                        player->SetVelocityX(0.0);
                        player->inputLocked = true;
                        parryTut.state      = ParryTutState::Waiting;
                        parryTut.waitTimer  = 0.0;
                    }
                    else
                    {
                        // Player can still move freely, just cap rightward speed
                        static constexpr double BRAKE_SPEED = 280.0;
                        if (player->GetVelocity().x > BRAKE_SPEED)
                            player->SetVelocityX(BRAKE_SPEED);
                    }
                    break;
                }

            case ParryTutState::Waiting:
                player->inputLocked = true;
                parryTut.waitTimer += dt;
                if (parryTut.waitTimer >= 2.0)
                {
                    parryTut.state = ParryTutState::Moving;
                    player->SetAutoMove(Math::vec2{ 3200.0, player->GetPosition().y });
                }
                break;

            case ParryTutState::Moving:
                if (!player->autoMoveActive)
                    parryTut.state = ParryTutState::WaitBash;
                break;

            case ParryTutState::WaitBash:
                if (bashReady)
                    parryTut.state = ParryTutState::Frozen;
                break;

            case ParryTutState::Frozen:
                if (!bashReady)
                {
                    parryTut.state = ParryTutState::Done;
                    if (tutorialOverlay)
                        tutorialOverlay->Clear();
                }
                else if (tutorialOverlay && tutorialOverlay->IsIdle())
                    tutorialOverlay->Show("Left Click to Parry!", 3.0, { 0.5, 0.6 });
                break;

            default: break;
        }
    }

    // ── Bull boss entrance sequence ──────────────────────────────────────────
    if (player && bossSeq.state != BossSeqState::Done)
    {
        auto* gom = GetGSComponent<CS230::GameObjectManager>();
        switch (bossSeq.state)
        {
            case BossSeqState::Idle:
                if (player->GetPosition().x >= 16200.0)
                {
                    BreakableWall* entranceWall = nullptr;
                    for (auto* obj : gom->GetObjects())
                    {
                        if (obj && obj->Type() == GameObjectTypes::BreakableWall)
                        {
                            auto* bw = static_cast<BreakableWall*>(obj);
                            if (bw->GetName() == "BOSS_ENTRANCE")
                            {
                                entranceWall = bw;
                                break;
                            }
                        }
                    }
                    if (entranceWall)
                    {
                        player->inputLocked    = true;
                        player->autoMoveActive = false;
                        player->SetVelocityX(0.0);

                        // Spawn boss at user-specified position
                        auto* boss = new BullBoss({ 18800.0, 2400.0 }, entranceWall, player);
                        gom->Add(boss);
                        bossSeq.boss = boss;
                        boss->TriggerEntrance();
                        bossSeq.state = BossSeqState::BossEntrance;
                    }
                }
                break;

            case BossSeqState::BossEntrance:
                if (bossSeq.boss && bossSeq.boss->GetState() == BullBoss::State::Chasing)
                {
                    player->inputLocked = false;
                    bossSeq.state       = BossSeqState::Chase;
                }
                break;

            case BossSeqState::Chase: break;

            default: break;
        }
    }

    {
        const double freezeMult = (cutscenePlayer && cutscenePlayer->IsTimeFrozen()) ? 0.0 : 1.0;
        const double scaledDt   = dt * timeScale * freezeMult;
        GetGSComponent<CS230::GameObjectManager>()->UpdateAll(scaledDt);
        GetGSComponent<CS230::GameObjectManager>()->CollisionTest();
    }

    // Level streaming — activate / deactivate objects by room proximity
    if (levelStreamer && player)
        levelStreamer->Update(player->GetPosition(), dt);

    // Deferred save (requested by Bonfire::Interact)
    if (SaveManager::ConsumeSaveRequest())
    {
        SaveGame();
        // Show brief on-screen feedback
        if (tutorialOverlay)
            tutorialOverlay->Show("Game Saved.", 1.8, { 0.5, 0.12 });
    }

    // 스크립트 트리거 감지 (in-game editor 활성 시 비활성)
    if (scriptManager && player && !(scriptEditorIG && scriptEditorIG->active))
        scriptManager->CheckTriggers(player->GetPosition());

    // 컷씬 플레이어 업데이트
    if (cutscenePlayer)
        cutscenePlayer->Update(dt);

    // 인게임 스크립트 에디터 입력 처리
    if (scriptEditorIG && scriptEditorIG->active)
        scriptEditorIG->Update();

    // if (shieldChargeShot != nullptr)
    //     shieldChargeShot->Update(dt * timeScale);

    if (player != nullptr && player->interactionTarget == nullptr)
        player->isInteracting = false;

    if (player != nullptr && player->isInteracting && player->interactionTarget != nullptr)
    {
        Door* interactedDoor = dynamic_cast<Door*>(player->interactionTarget);

        if (interactedDoor != nullptr && interactedDoor->ConsumeInteractionRequest())
        {
            const Door::Config& doorConfig = interactedDoor->GetConfig();

            if ((doorConfig.actionType == Door::ActionType::ChangeState || doorConfig.actionType == Door::ActionType::CutsceneThenChangeState) && doorConfig.destination == Door::Destination::Boss1)
            {
                Mode3::SetReturnPosition(player->GetPosition());
            }

            DoorActionHandler::Execute(*interactedDoor, *player);

            player->isInteracting     = false;
            player->interactionTarget = nullptr;

            return;
        }
    }

    if (player != nullptr)
    {
        const Math::ivec2 winSize = Engine::GetWindow().GetSize();
        const double      winW    = winSize.x;
        const double      winH    = winSize.y;

        const Math::rect bossRoom     = GetSimpleBossRoomBounds();
        const bool       inBossRoom   = IsPointInRect(player->GetPosition(), bossRoom);
        const bool       cutsceneZoom = cutscenePlayer && cutscenePlayer->overridingZoom;
        const bool       cutsceneCam  = cutscenePlayer && cutscenePlayer->overridingCamera;

        double desiredZoom = 1.0;

        if (inBossRoom)
        {
            const double bossW = bossRoom.Right() - bossRoom.Left();
            const double bossH = bossRoom.Top() - bossRoom.Bottom();

            const double fitX = winW / (bossW + SIMPLE_BOSS_CAMERA_PAD * 2.0);
            const double fitY = winH / (bossH + SIMPLE_BOSS_CAMERA_PAD * 2.0);

            desiredZoom = std::clamp(std::min(fitX, fitY), 0.25, 1.0);
        }

        if (cutsceneZoom)
            desiredZoom = cutscenePlayer->zoomTarget;

        camera->SetScale(desiredZoom);

        const double camZoom = camera->GetScale();
        const double visW    = winW / camZoom;
        const double visH    = winH / camZoom;

        Math::vec2 target;

        if (inBossRoom && !cutsceneCam)
        {
            const Math::vec2 bossCenter{ (bossRoom.Left() + bossRoom.Right()) * 0.5, (bossRoom.Bottom() + bossRoom.Top()) * 0.5 };

            target = bossCenter - Math::vec2{ visW * 0.5, visH * 0.5 };
        }
        else
        {
            target = player->GetPosition() - Math::vec2{ visW * 0.5, visH * 0.5 };
        }

        if (cameraBounds && !cutsceneCam)
        {
            const double bW = cameraBounds->Right() - cameraBounds->Left();
            const double bH = cameraBounds->Top() - cameraBounds->Bottom();

            if (visW >= bW)
                target.x = cameraBounds->Left() - (visW - bW) * 0.5;
            else
                target.x = std::clamp(target.x, cameraBounds->Left(), cameraBounds->Right() - visW);

            if (visH >= bH)
                target.y = cameraBounds->Bottom() - (visH - bH) * 0.5;
            else
                target.y = std::clamp(target.y, cameraBounds->Bottom(), cameraBounds->Top() - visH);
        }

        if (cutscenePlayer && cutscenePlayer->overridingCamera)
            target = cutscenePlayer->cameraTarget;

        // Smooth lerp — no teleport
        constexpr double CAM_SPEED = 8.0;
        const Math::vec2 cur       = camera->GetPosition();
        camera->SetPosition(cur + (target - cur) * std::min(dt * CAM_SPEED, 1.0));
    }

    // Pass player HP and camera position to post-processor
    if (player)
        postProcessor.SetPlayerHp(static_cast<float>(player->GetHP()), 5.0f);
    if (camera)
    {
        const Math::vec2 cp = camera->GetPosition();
        postProcessor.SetCameraPos(static_cast<float>(cp.x), static_cast<float>(cp.y));
    }

    // ── Death → light-dissolve → respawn ────────────────────────────────────
    if (player && player->IsDead())
    {
        if (deathTimer < 0.0)
            deathTimer = 0.0;
        deathTimer += dt;

        // Phase durations
        constexpr double FADE_IN  = 1.0; // 0→1 blackout (death → black)
        constexpr double HOLD     = 0.5; // full black (respawn happens here)
        constexpr double FADE_OUT = 0.8; // 1→0 blackout (black → normal)

        player->UpdateDeathEffect(dt);

        if (deathTimer < FADE_IN)
        {
            const float blackout = static_cast<float>(deathTimer / FADE_IN);
            postProcessor.SetBlackout(blackout);
        }
        else if (deathTimer < FADE_IN + HOLD)
        {
            postProcessor.SetBlackout(1.0f);
            if (!respawnDone)
            {
                const Math::vec2 respawnPos = player->GetSavePoint();
                spawnPos                    = respawnPos;
                player->ResetState();
                player->SetPosition(respawnPos);
                // Tutorial runs once: if it had started, skip it permanently
                if (parryTut.state != ParryTutState::Idle && parryTut.state != ParryTutState::Done)
                    parryTut.state = ParryTutState::Done;
                // Boss sequence runs once
                if (bossSeq.state != BossSeqState::Idle)
                    bossSeq.state = BossSeqState::Done;
                bossSeq.boss = nullptr;
                player->ClearAutoMove();
                player->inputLocked = false;

                // Reset broken water walls
                for (auto* obj : GetGSComponent<CS230::GameObjectManager>()->GetObjects())
                    if (obj && obj->Type() == GameObjectTypes::BreakableWall)
                        static_cast<BreakableWall*>(obj)->ResetIfWater();

                // Snap camera to spawn position while screen is black
                if (camera)
                {
                    const Math::ivec2 win     = Engine::GetWindow().GetSize();
                    const double      zoom    = camera->GetScale();
                    const double      visW    = win.x / zoom;
                    const double      visH    = win.y / zoom;
                    Math::vec2        camSnap = respawnPos - Math::vec2{ visW * 0.5, visH * 0.5 };

                    if (cameraBounds)
                    {
                        const double bW = cameraBounds->Right() - cameraBounds->Left();
                        const double bH = cameraBounds->Top() - cameraBounds->Bottom();
                        camSnap.x       = (visW >= bW) ? cameraBounds->Left() - (visW - bW) * 0.5 : std::clamp(camSnap.x, cameraBounds->Left(), cameraBounds->Right() - visW);
                        camSnap.y       = (visH >= bH) ? cameraBounds->Bottom() - (visH - bH) * 0.5 : std::clamp(camSnap.y, cameraBounds->Bottom(), cameraBounds->Top() - visH);
                    }

                    camera->SetPosition(camSnap);
                }

                respawnDone = true;
            }
        }
        else
        {
            const double fadeProgress = (deathTimer - FADE_IN - HOLD) / FADE_OUT;
            postProcessor.SetBlackout(static_cast<float>(1.0 - fadeProgress));
            if (fadeProgress >= 1.0)
            {
                postProcessor.SetBlackout(0.0f);
                deathTimer  = -1.0;
                respawnDone = false;
            }
        }
        goto camera_update;
    }
    else
    {
        postProcessor.SetBlackout(0.0f);
    }

camera_update:
    if (miniMap)
        miniMap->Update(dt);
}

void Mode3::Draw()
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

    // Snap camera position to nearest pixel to eliminate sub-pixel jitter
    const Math::vec2           rawCamPos  = camera->GetPosition();
    const Math::vec2           snapCamPos = { std::round(rawCamPos.x), std::round(rawCamPos.y) };
    Math::TransformationMatrix vp         = CS200::build_ndc_matrix(display_size_int) * (Math::ScaleMatrix({ camera->GetScale(), camera->GetScale() }) * Math::TranslationMatrix(-snapCamPos));

    // Scene pass: background + all game objects
    postProcessor.BeginSceneRender();
    {
        GL::UseProgram(backgroundShader.Shader);
        GL::Uniform2f(GL::GetUniformLocation(backgroundShader.Shader, "u_resolution"), static_cast<float>(display_size_int.x), static_cast<float>(display_size_int.y));
        GL::Uniform1f(GL::GetUniformLocation(backgroundShader.Shader, "u_time"), static_cast<float>(shaderTime));
        if (camera)
        {
            const Math::vec2 cp = camera->GetPosition();
            GL::Uniform2f(GL::GetUniformLocation(backgroundShader.Shader, "u_camPos"), static_cast<float>(cp.x), static_cast<float>(cp.y));
        }
        GL::Uniform1f(GL::GetUniformLocation(backgroundShader.Shader, "u_parallax"), 0.25f);
        GL::BindVertexArray(backgroundVAO);
        GL::DrawArrays(GL_TRIANGLE_FAN, 0, 4);
        GL::BindVertexArray(0);
        GL::UseProgram(0);
    }
    renderer.BeginScene(vp);
    GetGSComponent<CS230::GameObjectManager>()->DrawAll(vp);
    // if (shieldChargeShot != nullptr)
    //     shieldChargeShot->Draw(vp);

    // Developer overlay: show the real bash interaction radius around turret bullets.
    if (player && player->IsDeveloperMode() && player->bashEnabled)
    {
        auto* gom = GetGSComponent<CS230::GameObjectManager>();
        if (gom)
        {
            for (auto* obj : gom->GetObjects())
            {
                if (obj->Type() != GameObjectTypes::LaserTurret)
                    continue;

                auto* turret = static_cast<LaserTurret*>(obj);
                if (!turret->IsBulletActive() || turret->IsBulletBashed())
                    continue;

                for (const Math::vec2 bulletPos : turret->GetBashableBulletPositions())
                {
                    const double      dist    = std::sqrt((bulletPos - player->GetPosition()).LengthSquared());
                    const bool        inRange = dist <= BASH_RADIUS;
                    const CS200::RGBA ringCol = inRange ? 0x00FF88DDu : 0xFFCC4488u;

                    const auto radiusMarker = Math::TranslationMatrix(bulletPos) * Math::ScaleMatrix({ BASH_RADIUS * 2.0, BASH_RADIUS * 2.0 });
                    renderer.DrawCircle(radiusMarker, CS200::CLEAR, ringCol, inRange ? 3.0 : 2.0);
                }
            }
        }
    }

    // Bash aim indicator
    if (bashReady && player)
    {
        Math::vec2 bPos;
        Math::vec2 bDir;

        if (GetCurrentBashTargetInfo(bPos, bDir))
        {
            constexpr double kPI    = 3.14159265358979323846;
            constexpr double TWO_PI = kPI * 2.0;

            const Math::ivec2 win = Engine::GetWindow().GetSize();
            const Math::vec2  ms  = Engine::GetInput().GetMousePosition();
            const Math::vec2  mw  = camera->GetPosition() + Math::vec2{ ms.x, win.y - ms.y };

            const double progress = std::min(bashWindowTimer / BASH_WINDOW, 1.0);

            // ---- Aim line & arrow ----
            renderer.DrawLine(bPos, mw, 0xFFFFFF90u, 1.5);

            const Math::vec2 toMouse = (mw - bPos).LengthSquared() > 1.0 ? (mw - bPos).Normalize() : Math::vec2{ 1.0, 0.0 };

            constexpr double ARROW     = 18.0;
            const Math::vec2 arrowLeft = { -toMouse.y, toMouse.x };

            renderer.DrawLine(mw, mw - toMouse * ARROW + arrowLeft * (ARROW * 0.5), 0xFFFFFF90u, 1.5);
            renderer.DrawLine(mw, mw - toMouse * ARROW - arrowLeft * (ARROW * 0.5), 0xFFFFFF90u, 1.5);

            // Player rebound direction
            renderer.DrawLine(player->GetPosition(), player->GetPosition() + (-toMouse) * 80.0, 0xFF8844FFu, 2.0);

            // ---- Circular light sweep ----
            constexpr double RADIUS    = 55.0;
            constexpr double START_ANG = kPI * 0.5;
            constexpr int    RING_SEGS = 72;
            constexpr double BULGE_ANG = kPI / 5.0;

            const double headAng = START_ANG - progress * TWO_PI;

            for (int i = 0; i < RING_SEGS; ++i)
            {
                const double a0   = START_ANG - static_cast<double>(i) / RING_SEGS * TWO_PI;
                const double a1   = START_ANG - static_cast<double>(i + 1) / RING_SEGS * TWO_PI;
                const double aMid = (a0 + a1) * 0.5;

                const double dAng = std::acos(std::max(-1.0, std::min(1.0, std::cos(aMid) * std::cos(headAng) + std::sin(aMid) * std::sin(headAng))));

                const double glow       = std::max(0.0, 1.0 - dAng / BULGE_ANG);
                const double glowSmooth = glow * glow;

                const double lineW = 1.0 + glowSmooth * 7.0;

                const uint32_t baseAlpha = static_cast<uint32_t>(0x55 + glowSmooth * 0xAA);
                const uint32_t rComp     = static_cast<uint32_t>(0x55 + glowSmooth * 0xAA);
                const uint32_t gComp     = static_cast<uint32_t>(0x66 + glowSmooth * 0x99);
                const uint32_t bComp     = static_cast<uint32_t>(0x77 + glowSmooth * 0x88);
                const uint32_t col       = (rComp << 24) | (gComp << 16) | (bComp << 8) | baseAlpha;

                renderer.DrawLine({ bPos.x + RADIUS * std::cos(a0), bPos.y + RADIUS * std::sin(a0) }, { bPos.x + RADIUS * std::cos(a1), bPos.y + RADIUS * std::sin(a1) }, col, lineW);
            }
        }
    }

    // In-game script editor world overlay
    if (scriptEditorIG)
        scriptEditorIG->DrawWorld(renderer);

    renderer.EndScene();

    // Bloom is now extracted automatically by bright_pass in OriPostProcessor

    // Composite bloom to screen
    postProcessor.EndRenderAndDraw();

    // Screen-space UI (world text)
    Math::TransformationMatrix screen_matrix = CS200::build_ndc_matrix(display_size_int);
    renderer.BeginScene(screen_matrix);
    if (worldTextManager != nullptr)
        worldTextManager->Draw();
    if (tutorialOverlay != nullptr)
        tutorialOverlay->Draw();
    renderer.EndScene();
}

void Mode3::DrawImGui()
{
#ifdef DEVELOPER_VERSION
    ImGui::Begin("Mode3 Debug");
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
    if (ImGui::CollapsingHeader("Light Gauge / Charge Shot", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // if (shieldEnergy != nullptr) { ... }
        // if (shieldChargeShot != nullptr) { ... }
    }
    ImGui::End();

    ImGui::Begin("Mode3");
    if (player)
    {
        const int maxHp     = 5;
        const int currentHp = static_cast<int>(player->GetHP() + 0.01);
        ImGui::TextColored({ 1.0f, 0.3f, 0.3f, 1.0f }, "HP  ");
        ImGui::SameLine();
        for (int i = 0; i < maxHp; ++i)
        {
            const bool filled = (i < currentHp);
            ImGui::TextColored(filled ? ImVec4{ 1.0f, 0.2f, 0.2f, 1.0f } : ImVec4{ 0.4f, 0.1f, 0.1f, 1.0f }, filled ? "[*]" : "[ ]");
            if (i < maxHp - 1)
                ImGui::SameLine();
        }
        ImGui::Separator();

        ImGui::TextColored({ 0.4f, 1.0f, 0.6f, 1.0f }, "Abilities");
        bool dash = player->dashEnabled;
        bool wall = player->wallClimbEnabled;
        bool bash = player->bashEnabled;
        if (ImGui::Checkbox("Dash [LShift]", &dash))
            player->dashEnabled = dash;
        if (ImGui::Checkbox("Wall Climb", &wall))
            player->wallClimbEnabled = wall;
        if (ImGui::Checkbox("Bash [LClick near bullet]", &bash))
            player->bashEnabled = bash;
    }
    if (currentState == State::Playing)
    {
        ImGui::Separator();
        if (ImGui::Button("Level Editor [Tab]", { 180.0f, 0.0f }))
        {
            const Math::vec2 playerPos = player ? player->GetPosition() : Math::vec2{ -10.0, 0.0 };
            LevelEditor::SetGameObjects(GetGSComponent<CS230::GameObjectManager>());
            LevelEditor::SetSpawnPos(playerPos);
            LevelEditor::SetInitialCamera(playerPos, 1.0);
            Engine::GetGameStateManager().PushState<LevelEditor>();
        }

        if (scriptEditorIG)
        {
            ImGui::SameLine();
            const bool igOn = scriptEditorIG->active;
            if (igOn)
                ImGui::PushStyleColor(ImGuiCol_Button, { 0.2f, 0.7f, 0.3f, 1.0f });
            if (ImGui::Button(igOn ? "ScriptEd [ON]" : "Script Ed", { 120.0f, 0.0f }))
                scriptEditorIG->active = !scriptEditorIG->active;
            if (igOn)
                ImGui::PopStyleColor();
        }
    }
    ImGui::Separator();
    if (ImGui::Button("Main Menu", { 265.0f, 0.0f }))
    {
        CS230::SettingsManager::Instance().SaveSettings();
        Engine::GetGameStateManager().ChangeStateWithFade<MainMenu>();
    }
    if (levelStreamer)
        ImGui::TextDisabled("Streaming: %d rooms active", levelStreamer->ActiveRoomCount());
    ImGui::End();

    if (scriptEditorIG)
        scriptEditorIG->DrawImGui();
#endif

    if (miniMap)
        miniMap->DrawImGui();
}

// ---------------------------------------------------------------------------
// Save / Load helpers
// ---------------------------------------------------------------------------

void Mode3::SaveGame()
{
    SaveData data;

    if (player)
    {
        const Math::vec2 sp = player->GetSavePoint();
        spawnPos            = sp;
        data.spawnX         = sp.x;
        data.spawnY         = sp.y;
        data.hp             = player->GetHP();
        data.dash           = player->dashEnabled;
        data.wallClimb      = player->wallClimbEnabled;
        data.bash           = player->bashEnabled;
    }

    // Collect open-gate names (crack walls that have been destroyed)
    auto* gom = GetGSComponent<CS230::GameObjectManager>();
    if (gom)
    {
        for (auto* obj : gom->GetObjects())
        {
            if (obj->Type() != GameObjectTypes::Gate)
                continue;
            auto* gate = static_cast<Gate*>(obj);
            if (!gate->IsOpen())
                continue;

            // Use object name if set, otherwise fall back to "x,y" position key
            std::string id = gate->GetName();
            if (id.empty())
            {
                const Math::vec2 p = gate->GetPosition();
                id                 = std::to_string(static_cast<int>(p.x)) + "," + std::to_string(static_cast<int>(p.y));
            }
            data.openGates.push_back(id);
        }
    }

    SaveManager::Save(data);
}

void Mode3::ApplySave(const SaveData& data)
{
    if (player)
    {
        player->dashEnabled      = data.dash;
        player->wallClimbEnabled = data.wallClimb;
        player->bashEnabled      = data.bash;
        // HP is set indirectly: player starts with maxHp then we adjust
        // (Player doesn't expose SetHP — apply via damage from max to target)
        // For simplicity, we leave HP at max on load (safe respawn design)
    }

    // Re-open gates that were open when the game was saved
    auto* gom = GetGSComponent<CS230::GameObjectManager>();
    if (gom && !data.openGates.empty())
    {
        for (auto* obj : gom->GetObjects())
        {
            if (obj->Type() != GameObjectTypes::Gate)
                continue;
            auto* gate = static_cast<Gate*>(obj);

            std::string id = gate->GetName();
            if (id.empty())
            {
                const Math::vec2 p = gate->GetPosition();
                id                 = std::to_string(static_cast<int>(p.x)) + "," + std::to_string(static_cast<int>(p.y));
            }

            for (const auto& saved : data.openGates)
            {
                if (saved == id)
                {
                    gate->Open();
                    break;
                }
            }
        }
    }
}

void Mode3::Unload()
{
    AudioManager::StopBGM();

    OpenGL::DestroyShader(backgroundShader);
    GL::DeleteVertexArrays(1, &backgroundVAO);
    GL::DeleteBuffers(1, &backgroundVBO);

    delete shieldChargeShot;
    shieldChargeShot = nullptr;

    delete shieldEnergy;
    shieldEnergy = nullptr;

    ClearGSComponents();
    postProcessor.Shutdown();

    simpleBossStar = nullptr;
    bashTargetKind = BashTargetKind::None;

    player           = nullptr;
    mapManager       = nullptr;
    worldTextManager = nullptr;

    delete miniMap;
    miniMap = nullptr;

    delete scriptEditorIG;
    scriptEditorIG = nullptr;

    delete levelStreamer;
    levelStreamer = nullptr;
}

void Mode3::InitSimpleBossFight(CS230::GameObjectManager* gom)
{
    if (gom == nullptr || player == nullptr || simpleBossStar != nullptr)
        return;

    const Math::rect bossRoom = GetSimpleBossRoomBounds();

    const Math::vec2 bossCenter{ (bossRoom.Left() + bossRoom.Right()) * 0.5, (bossRoom.Bottom() + bossRoom.Top()) * 0.5 };

    std::vector<TargetStar*> bossTargets;

    for (auto* obj : gom->GetObjects())
    {
        if (obj == nullptr || obj->Type() != GameObjectTypes::Target)
            continue;

        auto*            target = static_cast<TargetStar*>(obj);
        const Math::vec2 p      = target->GetPosition();

        if (p.x >= bossRoom.Left() && p.x <= bossRoom.Right() && p.y >= bossRoom.Bottom() && p.y <= bossRoom.Top())
        {
            bossTargets.push_back(target);
        }
    }

    if (bossTargets.empty())
    {
        Engine::GetLogger().LogEvent("SimpleBossStar skipped: no TargetStar objects in boss room.");
        return;
    }

    simpleBossStar = new SimpleBossStar(bossCenter, bossRoom, player, bossTargets);
    simpleBossStar->SetName("SIMPLE_BOSS_STAR");
    gom->Add(simpleBossStar);

    Engine::GetLogger().LogEvent("SimpleBossStar initialized. targets=" + std::to_string(bossTargets.size()));
}

bool Mode3::GetCurrentBashTargetInfo(Math::vec2& outPos, Math::vec2& outDir) const
{
    if (player == nullptr)
        return false;

    if (bashTargetKind == BashTargetKind::LaserTurret && bashTarget != nullptr)
    {
        outPos = bashTarget->GetNearestBashableBulletPosition(player->GetPosition());
        outDir = bashTarget->GetNearestBashableBulletDirection(player->GetPosition());
        return true;
    }

    if (bashTargetKind == BashTargetKind::SimpleBoss && simpleBossStar != nullptr)
    {
        outPos = simpleBossStar->GetNearestBashableShotPosition(player->GetPosition());
        outDir = simpleBossStar->GetNearestBashableShotDirection(player->GetPosition());
        return true;
    }

    return false;
}

bool Mode3::BashCurrentTarget(Math::vec2 newDir)
{
    if (player == nullptr)
        return false;

    if (bashTargetKind == BashTargetKind::LaserTurret && bashTarget != nullptr)
    {
        return bashTarget->BashNearestBullet(player->GetPosition(), newDir, BASH_RADIUS * BASH_RADIUS);
    }

    if (bashTargetKind == BashTargetKind::SimpleBoss && simpleBossStar != nullptr)
    {
        return simpleBossStar->BashNearestShot(player->GetPosition(), newDir, BASH_RADIUS * BASH_RADIUS);
    }

    return false;
}