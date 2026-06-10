#pragma once
#include "Engine/Camera.hpp"
#include "Engine/GameState.hpp"
#include "Engine/Rect.hpp"
#include "Engine/Texture.hpp"
#include "Game/LaserTurret.hpp"
#include "Game/OriPostProcessor.hpp"
#include "Game/SaveData.hpp"
#include "MiniMap.hpp"
#include "OpenGL/Buffer.hpp"
#include "OpenGL/Shader.hpp"
#include "OpenGL/VertexArray.hpp"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class Player;
class WorldTextManager;
class TutorialOverlay;
class CutscenePlayer;
class ScriptManager;
class InGameScriptEditor;
class LevelStreamer;
class BullBoss;
class SimpleBossStar;

namespace Boss
{
    class ShieldEnergy;
    class LightOrbManager;
    class ShieldChargeShot;
}

namespace CS230
{
    class MapManager;
}

class Mode3 : public CS230::GameState
{
public:
    static void SetReturnPosition(Math::vec2 position);
    static void ClearReturnPosition();

    void Load() override;
    void Update(double dt) override;
    void Unload() override;
    void Draw() override;
    void DrawImGui() override;

    void SaveGame();                      // collect all persistent state and write to disk
    void ApplySave(const SaveData& data); // restore state after a load

    gsl::czstring GetName() const override
    {
        return "Mode3";
    }

private:
    void InitGame();
    bool CanPause() const override;

    void InitSimpleBossFight(CS230::GameObjectManager* gom);
    bool GetCurrentBashTargetInfo(Math::vec2& outPos, Math::vec2& outDir) const;
    bool BashCurrentTarget(Math::vec2 newDir);

    static std::optional<Math::vec2> pendingReturnPosition;

    enum class State
    {
        Loading,
        Playing
    };

    State currentState = State::Loading;

    CS230::Camera*                     camera           = nullptr;
    Player*                            player           = nullptr;
    CS230::MapManager*                 mapManager       = nullptr;
    WorldTextManager*                  worldTextManager = nullptr;
    TutorialOverlay*                   tutorialOverlay  = nullptr;
    CutscenePlayer*                    cutscenePlayer   = nullptr;
    ScriptManager*                     scriptManager    = nullptr;
    InGameScriptEditor*                scriptEditorIG   = nullptr;
    LevelStreamer*                     levelStreamer    = nullptr;
    MiniMap*                           miniMap          = nullptr;
    std::map<std::string, std::string> signTexts;

    Boss::ShieldEnergy*     shieldEnergy     = nullptr;
    Boss::LightOrbManager*  lightOrbManager  = nullptr;
    Boss::ShieldChargeShot* shieldChargeShot = nullptr;

    OpenGL::CompiledShader    backgroundShader;
    OpenGL::VertexArrayHandle backgroundVAO;
    OpenGL::BufferHandle      backgroundVBO;
    double                    shaderTime = 0.0;

    Math::rect level_boundary = {
        {  -500,  2000 },
        { 10000, -2000 }
    };

    std::optional<Math::rect> cameraBounds;
    std::optional<Math::rect> worldBounds; // global union of all rooms (minimap)

    OriPostProcessor postProcessor;

    // Death / respawn
    Math::vec2 spawnPos    = { -10.0, 0.0 };
    double     deathTimer  = -1.0; // -1 = alive
    bool       respawnDone = false;

    // Bash system
    enum class BashTargetKind
    {
        None,
        LaserTurret,
        SimpleBoss
    };

    LaserTurret*            bashTarget      = nullptr;
    BashTargetKind          bashTargetKind  = BashTargetKind::None;
    bool                    bashReady       = false;
    double                  timeScale       = 1.0;
    double                  timeScaleTarget = 1.0;
    double                  bashWindowTimer = 0.0; // 0 → BASH_WINDOW: slow-mo window
    static constexpr double BASH_WINDOW     = 1.0;

    // Bull boss entrance sequence
    enum class BossSeqState : uint8_t
    {
        Idle,
        BossEntrance,
        Chase,
        Done
    };

    struct
    {
        BossSeqState state = BossSeqState::Idle;
        BullBoss*    boss  = nullptr;
    } bossSeq;

    SimpleBossStar* simpleBossStar = nullptr;

    // Hardcoded parry tutorial
    enum class ParryTutState : uint8_t
    {
        Idle,
        Braking,
        Waiting,
        Moving,
        WaitBash,
        Frozen,
        Done
    };

    struct
    {
        ParryTutState state     = ParryTutState::Idle;
        double        waitTimer = 0.0;
    } parryTut;

    static bool s_startInEditor;
};
