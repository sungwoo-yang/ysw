#pragma once
#include "Engine/Camera.hpp"
#include "Engine/GameState.hpp"
#include "Engine/Rect.hpp"
#include "Engine/Texture.hpp"
#include "MiniMap.hpp"
#include "OpenGL/Buffer.hpp"
#include "OpenGL/Shader.hpp"
#include "OpenGL/VertexArray.hpp"
#include <map>
#include <memory>
#include <string>

class Player;
class WorldTextManager;

namespace CS230
{
    class MapManager;
}

class Mode3 : public CS230::GameState
{
public:
    void Load() override;
    void Update(double dt) override;
    void Unload() override;
    void Draw() override;
    void DrawImGui() override;

    gsl::czstring GetName() const override
    {
        return "Mode3";
    }

private:
    void InitGame();

    enum class State
    {
        Loading,
        Playing
    };

    State currentState = State::Loading;

    CS230::Camera* camera;
    Player* player;
    CS230::MapManager* mapManager;
    WorldTextManager* worldTextManager;
    MiniMap* miniMap;
    std::map<std::string, std::string> signTexts;

    OpenGL::CompiledShader    backgroundShader;
    OpenGL::VertexArrayHandle backgroundVAO;
    OpenGL::BufferHandle      backgroundVBO;
    double                    shaderTime = 0.0;

    Math::rect level_boundary = {
        {  -500,  2000 },
        { 10000, -2000 }
    };
};