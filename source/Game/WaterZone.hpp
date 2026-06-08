#pragma once
#include "Engine/GameObject.hpp"
#include "Engine/GameObjectTypes.hpp"
#include "Engine/Vec2.hpp"
#include "OpenGL/GL.hpp"
#include "OpenGL/Shader.hpp"

// Water terrain zone — depth-based effects on the player.
//   Shallow  (0 – 30 px submerged) : walk speed × 0.70
//   Mid      (30 – 70 px)          : walk speed × 0.45, reduced jump
//   Swimming (> 70 px)             : near-zero gravity, buoyancy, W/S to swim
class WaterZone : public CS230::GameObject {
public:
    WaterZone(Math::vec2 pos, Math::vec2 size);
    ~WaterZone() override;

    void Update(double dt)                                        override;
    void Draw  (const Math::TransformationMatrix& camera_matrix) override;

    GameObjectTypes Type()     override { return GameObjectTypes::Water; }
    std::string     TypeName() override { return "WaterZone"; }
    bool CanCollideWith(GameObjectTypes) override { return false; }
    void ResolveCollision(CS230::GameObject*) override {}

    // Returns how many px below the water surface 'worldBottomY' is.
    double GetSubmergedDepth(double worldX, double worldBottomY) const;

    double GetSurfaceY() const { return GetPosition().y + size.y * 0.5; }

private:
    Math::vec2 size;
    double     waveTime = 0.0;

    // Per-instance GL resources
    GLuint _vao = 0;
    GLuint _vbo = 0;

    // Shared shader across all WaterZone instances
    static OpenGL::CompiledShader s_shader;
    static int                    s_refCount;

    void EnsureGL();
};
