#pragma once
#include "Engine/Camera.hpp"
#include "Engine/GameState.hpp"
#include "Engine/Rect.hpp"
#include "Engine/Texture.hpp"
#include "MiniMap.hpp"
#include "OpenGL/Buffer.hpp"
#include "OpenGL/Shader.hpp"
#include "OpenGL/VertexArray.hpp"
#include <memory>

class Player;
class WorldTextManager;
class Star;
class YellowLaser;
class TargetStar;

namespace CS230
{
    class MapManager;
}

class Mode1 : public CS230::GameState
{
public:
    void Load() override;
    void Update(double dt) override;
    void Unload() override;
    void Draw() override;
    void DrawImGui() override;

    gsl::czstring GetName() const override
    {
        return "Mode1";
    }

private:
    // Deferred initialization once the map is fully loaded
    void InitGame();

    enum class State
    {
        Loading,
        Playing
    };

    State currentState = State::Loading;

    // Core scene components
    CS230::Camera*     camera;
    Player*            player;
    CS230::MapManager* mapManager;
    WorldTextManager*  worldTextManager;
    MiniMap*           miniMap;

    // Gameplay objectives
    std::vector<TargetStar*> targetStars;
    // Star*                    shooterStar;
    // Star*                    targetStar;
    // YellowLaser*             yellowLaser;

    // Background Shader Members
    OpenGL::CompiledShader    backgroundShader;
    OpenGL::VertexArrayHandle backgroundVAO;
    OpenGL::BufferHandle      backgroundVBO;
    double                    shaderTime = 0.0;

    // [Removed] textureLayer2_Trees definition
    std::shared_ptr<CS230::Texture> textureLayer3_Silhouette;

    // Level spatial constraints
    Math::rect level1_boundary = {
        {  -500,  2000 },
        { 10000, -2000 }
    };
};