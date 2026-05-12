

#pragma once

#include "CS200/IRenderer2D.hpp"
#include "Engine/Dash.hpp"
#include "Engine/GameState.hpp"
#include "Engine/Rect.hpp"
#include "Engine/Texture.hpp"

#include <GL/glew.h>
#include <limits>
#include <optional>
#include <string>
#include <vector>

struct Platform
{
    Math::vec2 start{};
    Math::vec2 end{};
    double     thickness   = 20.0;
    bool       canDropDown = false; 
};

class DemoAstar final : public CS230::GameState
{
public:
    DemoAstar() = default;

    void          Load() override;
    void          Update() override;
    void          Draw() const override;
    void          DrawImGui() override;
    void          Unload() override;
    gsl::czstring GetName() const override;

private:
    enum class OlimAnimation
    {
        None,
        OlimWalking,
        OlimJump
    };

    struct OlimFrame
    {
        int    frameIndex;
        double duration;
    };

    struct OlimAnimData
    {
        std::string            name;
        std::vector<OlimFrame> frames;
        int                    loopFrame;
    };

    struct OlimState
    {
        OlimAnimation         animation;
        int                   frameIndex;
        double                timer;
        Math::vec2            position;
        bool                  faceRight;
        double                velocityY            = 0.0;
        bool                  isJumping            = false;
        std::optional<size_t> currentPlatformIndex = std::nullopt;
    };

    struct RenderInfo
    {
        GLuint      Framebuffer{ 0 };
        GLuint      ColorTexture{ 0 };
        Math::ivec2 Size{};
    };

private:
    void initializeOlimAnimations();
    void updateOlimAnimation(OlimState& character, double delta_time);
    void drawOlim(const OlimState& character) const;

    void ResetOlimState();

    RenderInfo beginOffscreenRendering() const;
    GLuint     endOffscreenRendering(const RenderInfo& render_info) const;

private:
    std::shared_ptr<CS230::Texture> olimTexture{};
    std::vector<OlimAnimData>       olimAnimations{};
    OlimState                       idleOlim{};

    GLuint lastFramebufferTexture{ 0 };
    
    double gravity               = 1500.0; 
    double jumpStrength          = 700.0;  
    double groundLevel           = 100.0;  
    double platformThickness     = 20.0;   
    double thinPlatformThickness = 5.0;

    std::vector<Platform> platforms; 

    CS230::DashComponent olimDash;

    double      dashGaugeWidth   = 40.0;       
    double      dashGaugeHeight  = 5.0;        
    double      dashGaugeOffsetY = 10.0;       
    CS200::RGBA dashGaugeBgColor = 0x55555580; 
    CS200::RGBA dashGaugeFgColor = 0x00BCD4FF;

    double       shiftHoldTimer        = 0.0;   
    bool         isSprinting           = false; 
    const double baseSpeed             = 300.0; 
    const double sprintSpeedMultiplier = 1.8;   
    const double sprintActivationTime  = 1.0;   
    
    Math::vec2   initialThinPlatformStart{};
    Math::vec2   initialThinPlatformEnd{};
    const double platformMoveSpeed = 200.0;
};