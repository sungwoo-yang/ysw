

#include "DemoAstar.hpp"

#include "CS200/IRenderer2D.hpp"
#include "CS200/NDC.hpp"
#include "CS200/RenderingAPI.hpp"
#include "DemoReflection.hpp"
#include "Engine/Dash.hpp" 
#include "Engine/Engine.hpp"
#include "Engine/GameStateManager.hpp"
#include "Engine/Input.hpp"
#include "Engine/Matrix.hpp"
#include "Engine/Texture.hpp"
#include "Engine/TextureManager.hpp"
#include "Engine/Window.hpp"
#include "OpenGL/GL.hpp"

#include <algorithm> 
#include <cmath>
#include <imgui.h>
#include <limits>   
#include <optional> 

namespace
{
    constexpr Math::ivec2 OLIM_FRAME_SIZE{ 63, 127 };
    constexpr Math::ivec2 OLIM_HOT_SPOT{ 32, 63 };
    constexpr int         OLIM_NUM_FRAMES = 2;
} 

void DemoAstar::ResetOlimState()
{
    
    idleOlim.animation            = OlimAnimation::None;
    idleOlim.frameIndex           = 0;
    idleOlim.timer                = 0.0;
    idleOlim.position             = Math::vec2{ 400.0, groundLevel + OLIM_HOT_SPOT.y + 100.0 }; 
    idleOlim.faceRight            = true;
    idleOlim.velocityY            = 0.0;
    idleOlim.isJumping            = true; 
    idleOlim.currentPlatformIndex = std::nullopt;

    olimDash.isDashing         = false;
    olimDash.dashTimer         = 0.0;
    olimDash.dashCooldownTimer = 0.0; 
    isSprinting                = false;
    shiftHoldTimer             = 0.0;
}

void DemoAstar::Load()
{
    Engine::GetLogger().LogEvent("Event: DemoAstar (Platformer) Load");

    auto& texture_manager = Engine::GetTextureManager();
    olimTexture           = texture_manager.Load("Assets/images/DemoAstar/Robot.png"); 

    initializeOlimAnimations();

    CS200::RenderingAPI::SetClearColor(0x223344FF);

    ResetOlimState();

    platforms.clear();
    const auto windowSize = Engine::GetWindow().GetSize();

    platforms.push_back(
        {
            {                50.0, groundLevel },
            { windowSize.x - 50.0, groundLevel },
            platformThickness,
            false  
    });

    platforms.push_back(
        {
            { windowSize.x * 0.3, groundLevel + 150.0 },
            { windowSize.x * 0.7, groundLevel + 150.0 },
            thinPlatformThickness,
            true  
    });

    if (platforms.size() > 1) 
    {
        initialThinPlatformStart = platforms[1].start;
        initialThinPlatformEnd   = platforms[1].end;
    }

    Engine::GetLogger().LogEvent("Event: Created " + std::to_string(platforms.size()) + " platforms");

    olimDash              = CS230::DashComponent{};
    olimDash.dashCooldown = 3.0;
}

void DemoAstar::Update()
{
    const double delta_time = Engine::GetWindowEnvironment().DeltaTime;
    auto&        input      = Engine::GetInput();

    Math::vec2 platformMove{ 0.0, 0.0 };
    if (input.KeyDown(CS230::Input::Keys::Left))
    {
        platformMove.x -= 1.0;
    }
    if (input.KeyDown(CS230::Input::Keys::Right))
    {
        platformMove.x += 1.0;
    }
    if (input.KeyDown(CS230::Input::Keys::Up))
    {
        platformMove.y += 1.0;
    }
    if (input.KeyDown(CS230::Input::Keys::Down))
    {
        platformMove.y -= 1.0;
    }

    if (platformMove.Length() > 0.0 && platforms.size() > 1)
    {
        Math::vec2 moveVector = platformMove.Normalize() * platformMoveSpeed * delta_time;
        platforms[1].start += moveVector;
        platforms[1].end += moveVector;
    }

    olimDash.UpdateTimers(delta_time);

    const bool shiftDown         = input.KeyDown(CS230::Input::Keys::LShift);
    const bool shiftJustReleased = input.KeyJustReleased(CS230::Input::Keys::LShift);

    bool canStartSpecialMove = olimDash.dashCooldownTimer <= 0.0 && !olimDash.IsDashing() && !isSprinting;

    if (shiftDown)
    {
        if (canStartSpecialMove)
        {
            shiftHoldTimer += delta_time;
            if (shiftHoldTimer >= sprintActivationTime)
            {
                isSprinting = true; 
            }
        }
    }
    else if (shiftJustReleased)
    {
        if (shiftHoldTimer > 0.0 && shiftHoldTimer < sprintActivationTime) 
        {
            if (canStartSpecialMove)
            {
                
                olimDash.TryStartDash(idleOlim.faceRight);
            }
        }
        
        isSprinting    = false;
        shiftHoldTimer = 0.0;
    }
    else 
    {
        isSprinting    = false;
        shiftHoldTimer = 0.0;
    }

    bool applyGravity = true;
    if (olimDash.IsDashing() && olimDash.disableGravityOnDash)
    {
        applyGravity = false;
    }
    else if (!idleOlim.isJumping && idleOlim.currentPlatformIndex.has_value())
    {
        applyGravity       = false;
        idleOlim.velocityY = 0.0;
    }
    if (applyGravity)
    {
        idleOlim.velocityY -= gravity * delta_time;
    }

    Math::vec2 move{ 0.0, 0.0 };
    double     currentSpeedX = 0.0; 

    double currentBaseSpeed = baseSpeed;
    bool   wasSprinting     = isSprinting;

    if (isSprinting) 
    {
        currentBaseSpeed *= sprintSpeedMultiplier;
    }

    if (olimDash.IsDashing()) 
    {
        currentSpeedX = olimDash.GetDashVelocityX();
    }
    else 
    {
        if (input.KeyDown(CS230::Input::Keys::A))
        {
            move.x -= 1.0;
            idleOlim.faceRight = false;
        }
        if (input.KeyDown(CS230::Input::Keys::D))
        {
            move.x += 1.0;
            idleOlim.faceRight = true;
        }
        currentSpeedX = move.x * currentBaseSpeed; 
    }

    if (wasSprinting && !isSprinting && !olimDash.IsDashing()) 
    {
        Engine::GetLogger().LogEvent("Event: Player Sprint Stop");
    }
    else if (!wasSprinting && isSprinting)
    {
        Engine::GetLogger().LogEvent("Event: Player Sprint Start");
    }

    bool justJumped = false;
    if (!olimDash.IsDashing() && !idleOlim.isJumping && (input.KeyJustPressed(CS230::Input::Keys::Space) || input.KeyJustPressed(CS230::Input::Keys::W)))
    {
        idleOlim.velocityY            = jumpStrength;
        idleOlim.isJumping            = true;
        idleOlim.currentPlatformIndex = std::nullopt;
        justJumped                    = true;

        Engine::GetLogger().LogEvent("Event: Player Jump");
    }

    bool tryingToDropDown = false;
    if (!olimDash.IsDashing() && idleOlim.currentPlatformIndex.has_value() && platforms[*idleOlim.currentPlatformIndex].canDropDown && input.KeyDown(CS230::Input::Keys::S) && !idleOlim.isJumping)
    {
        tryingToDropDown              = true;
        idleOlim.isJumping            = true;
        idleOlim.currentPlatformIndex = std::nullopt;
        idleOlim.position.y -= 50.0; 

        Engine::GetLogger().LogEvent("Event: Player Drop Down");

    }

    double     previousFeetY     = idleOlim.position.y - OLIM_HOT_SPOT.y;
    Math::vec2 predictedPosition = idleOlim.position;
    predictedPosition.x += currentSpeedX * delta_time; 

    if (!olimDash.IsDashing() || !olimDash.disableGravityOnDash)
    {
        predictedPosition.y += idleOlim.velocityY * delta_time;
    }

    double predictedFeetY = predictedPosition.y - OLIM_HOT_SPOT.y;

    bool                  landedOnPlatform    = false;
    double                landingPlatformY    = -std::numeric_limits<double>::infinity();
    std::optional<size_t> landedPlatformIndex = std::nullopt;
    const double          landingEpsilon      = 1.0;

    if (idleOlim.velocityY <= 0 && !justJumped && !tryingToDropDown && !olimDash.IsDashing())
    {
        for (size_t i = 0; i < platforms.size(); ++i)
        {
            const auto& platform  = platforms[i];
            double      platformY = platform.start.y;

            if (previousFeetY >= platformY - landingEpsilon && predictedFeetY <= platformY + landingEpsilon && predictedPosition.x >= platform.start.x - (OLIM_FRAME_SIZE.x / 2.0 - OLIM_HOT_SPOT.x) &&
                predictedPosition.x <= platform.end.x + (OLIM_FRAME_SIZE.x / 2.0 - OLIM_HOT_SPOT.x))
            {
                if (platformY > landingPlatformY) 
                {
                    landingPlatformY    = platformY;
                    landedPlatformIndex = i;
                    landedOnPlatform    = true;
                }
            }
        }
    }

    if (landedOnPlatform)
    {
        if (idleOlim.isJumping)
        {
            Engine::GetLogger().LogEvent("Event: Player Landed on platform " + std::to_string(*landedPlatformIndex));
        }

        predictedPosition.y           = landingPlatformY + OLIM_HOT_SPOT.y;
        idleOlim.velocityY            = 0.0;
        idleOlim.isJumping            = false;
        idleOlim.currentPlatformIndex = landedPlatformIndex;
    }
    else 
    {

        if (idleOlim.currentPlatformIndex.has_value() && !tryingToDropDown)
        {
            Engine::GetLogger().LogEvent("Event: Player Fell off platform");
        }

        if (idleOlim.currentPlatformIndex.has_value() || tryingToDropDown)
        {
            idleOlim.currentPlatformIndex = std::nullopt; 
        }
        
        idleOlim.isJumping = true;
    }

    idleOlim.position = predictedPosition;

    const auto windowSize = Engine::GetWindow().GetSize();
    idleOlim.position.x   = std::clamp(idleOlim.position.x, 0.0 + OLIM_HOT_SPOT.x, static_cast<double>(windowSize.x - OLIM_HOT_SPOT.x));

    const OlimAnimation prevAnim = idleOlim.animation;
    OlimAnimation       newAnim  = idleOlim.animation;

    if (olimDash.IsDashing()) 
    {
        newAnim = OlimAnimation::OlimJump; 
    }
    else if (idleOlim.isJumping) 
    {
        newAnim = OlimAnimation::OlimJump;
        if (currentSpeedX > 0.0)
            idleOlim.faceRight = true;
        else if (currentSpeedX < 0.0)
            idleOlim.faceRight = false;
    }
    
    else if (std::abs(currentSpeedX) > 0.01)
    {
        newAnim = OlimAnimation::OlimWalking;
        if (currentSpeedX > 0.0)
            idleOlim.faceRight = true;
        else if (currentSpeedX < 0.0)
            idleOlim.faceRight = false;
    }
    else 
    {
        newAnim = OlimAnimation::None;
    }

    if (newAnim != prevAnim)
    {
        idleOlim.animation  = newAnim;
        idleOlim.frameIndex = 0;
        idleOlim.timer      = 0.0;
    }
    updateOlimAnimation(idleOlim, delta_time);
}

void DemoAstar::Draw() const
{
    CS200::RenderingAPI::Clear();
    auto& renderer_2d = Engine::GetRenderer2D();

    renderer_2d.BeginScene(CS200::build_ndc_matrix(Engine::GetWindow().GetSize()));

    for (const auto& platform : platforms)
    {
        renderer_2d.DrawLine(platform.start, platform.end, CS200::WHITE, platform.thickness);
    }

    drawOlim(idleOlim);

    if (olimDash.dashCooldownTimer > 0.0 && olimDash.dashCooldown > 0.0) 
    {
        
        double cooldownRatio = std::clamp(olimDash.dashCooldownTimer / olimDash.dashCooldown, 0.0, 1.0);

        Math::vec2 gaugeCenterPos = {
            idleOlim.position.x,                                     
            idleOlim.position.y - OLIM_HOT_SPOT.y - dashGaugeOffsetY 
        };

        Math::TransformationMatrix bgTransform = Math::TranslationMatrix(gaugeCenterPos) * Math::ScaleMatrix({ dashGaugeWidth, dashGaugeHeight });
        renderer_2d.DrawRectangle(bgTransform, dashGaugeBgColor); 

        double                     fgWidth     = dashGaugeWidth * cooldownRatio;
        
        Math::vec2                 fgCenterPos = { gaugeCenterPos.x - (dashGaugeWidth - fgWidth) / 2.0, gaugeCenterPos.y };
        Math::TransformationMatrix fgTransform = Math::TranslationMatrix(fgCenterPos) * Math::ScaleMatrix({ fgWidth, dashGaugeHeight });
        renderer_2d.DrawRectangle(fgTransform, dashGaugeFgColor); 
    }

    renderer_2d.EndScene();
}

void DemoAstar::DrawImGui()
{
    if (ImGui::Begin("DemoAstar Controls"))
    {
        
        ImGui::Text("A/D to move, W/Space to Jump, S to Drop");
        ImGui::Text("LShift (Tap) = Dash, LShift (Hold > 1s) = Sprint");
        ImGui::Text("Arrow Keys = Move Thin Platform"); 

        ImGui::Separator(); 

        if (ImGui::Button("Reset Robot Position"))
        {
            ResetOlimState();
        }

        ImGui::SameLine(); 
        if (ImGui::Button("Reset Platform Position"))
        {
            if (platforms.size() > 1)
            {
                platforms[1].start = initialThinPlatformStart;
                platforms[1].end   = initialThinPlatformEnd;
            }
        }

        ImGui::Separator(); 

        ImGui::Text("Current position: (%.1f, %.1f)", idleOlim.position.x, idleOlim.position.y);
        ImGui::Text("Current Y Velocity: %.1f", idleOlim.velocityY);
        ImGui::Text("Is Jumping/Falling: %s", idleOlim.isJumping ? "Yes" : "No");
        if (idleOlim.currentPlatformIndex.has_value())
        {
            ImGui::Text("On Platform Index: %zu (Can Drop: %s)", *idleOlim.currentPlatformIndex, platforms[*idleOlim.currentPlatformIndex].canDropDown ? "Yes" : "No");
        }
        else
        {
            ImGui::Text("On Platform Index: None");
        }

        ImGui::Text("Is Dashing: %s", olimDash.IsDashing() ? "Yes" : "No");
        ImGui::Text("Is Sprinting: %s", isSprinting ? "Yes" : "No");
        ImGui::Text("Dash Cooldown: %.2f", std::max(0.0, olimDash.dashCooldownTimer));
        ImGui::Text("Shift Hold Time: %.2f", shiftHoldTimer); 

        ImGui::SeparatorText("Physics Controls");
        static double min_gravity = 100.0, max_gravity = 3000.0;
        ImGui::DragScalar("Gravity", ImGuiDataType_Double, &gravity, 10.0f, &min_gravity, &max_gravity, "%.1f");
        static double min_jump = 100.0, max_jump = 1500.0;
        ImGui::DragScalar("Jump Strength", ImGuiDataType_Double, &jumpStrength, 10.0f, &min_jump, &max_jump, "%.1f");

        ImGui::SeparatorText("Dash/Sprint Controls");
        static double min_dash_speed = 100.0, max_dash_speed = 2000.0;
        static double min_dash_dur = 0.05, max_dash_dur = 0.5;
        static double min_dash_cool = 0.1, max_dash_cool = 5.0; 
        ImGui::DragScalar("Dash Speed", ImGuiDataType_Double, &olimDash.dashSpeed, 10.0f, &min_dash_speed, &max_dash_speed, "%.1f");
        ImGui::DragScalar("Dash Duration", ImGuiDataType_Double, &olimDash.dashDuration, 0.01f, &min_dash_dur, &max_dash_dur, "%.2f");
        ImGui::DragScalar("Dash Cooldown", ImGuiDataType_Double, &olimDash.dashCooldown, 0.05f, &min_dash_cool, &max_dash_cool, "%.2f"); 
        ImGui::Checkbox("Disable Gravity on Dash", &olimDash.disableGravityOnDash);

        ImGui::SeparatorText("Switch Demo");
        if (ImGui::Button("Switch to Demo Reflection"))
        {
            Engine::GetGameStateManager().PopState();
            Engine::GetGameStateManager().PushState<DemoLaserReflection>();
        }
    }
    ImGui::End();
}

void DemoAstar::Unload()
{
    if (lastFramebufferTexture != 0)
    {
        GL::DeleteTextures(1, &lastFramebufferTexture);
        lastFramebufferTexture = 0;
    }
}

gsl::czstring DemoAstar::GetName() const
{
    return "Demo A* (Platformer)"; 
}

void DemoAstar::initializeOlimAnimations()
{
    olimAnimations.clear();
    olimAnimations.resize(3); 

    olimAnimations[static_cast<int>(OlimAnimation::None)] = { "Idle", { { 0, 0.5 } }, 0 };

    olimAnimations[static_cast<int>(OlimAnimation::OlimWalking)] = {
        "Walk", { { 0, 0.2 }, { 1, 0.2 } },
         0
    };

    olimAnimations[static_cast<int>(OlimAnimation::OlimJump)] = { "Jump", { { 2, 1.0 } }, 0 }; 
}

void DemoAstar::updateOlimAnimation(OlimState& character, double delta_time)
{
    const size_t anim_index = static_cast<size_t>(character.animation);
    if (anim_index >= olimAnimations.size())
        return; 

    const auto& anim = olimAnimations[anim_index];
    if (anim.frames.empty())
        return; 

    if (character.frameIndex < 0 || static_cast<size_t>(character.frameIndex) >= anim.frames.size())
    {
        character.frameIndex = anim.loopFrame;
        character.timer      = 0.0; 
    }

    character.timer += delta_time;
    const auto& current_frame = anim.frames[static_cast<size_t>(character.frameIndex)];

    if (character.timer >= current_frame.duration)
    {
        character.timer -= current_frame.duration; 
        character.frameIndex++;
        if (static_cast<size_t>(character.frameIndex) >= anim.frames.size())
        {
            character.frameIndex = anim.loopFrame; 
        }
    }
}

void DemoAstar::drawOlim(const OlimState& character) const
{
    const size_t anim_index = static_cast<size_t>(character.animation);
    if (anim_index >= olimAnimations.size())
        return;

    const auto& anim = olimAnimations[anim_index];
    if (anim.frames.empty())
        return;

    int currentFrameIdx = character.frameIndex;
    if (currentFrameIdx < 0 || static_cast<size_t>(currentFrameIdx) >= anim.frames.size())
    {
        currentFrameIdx = anim.loopFrame;
    }

    const int  sprite_frame = anim.frames[static_cast<size_t>(currentFrameIdx)].frameIndex;
    const auto texel_base   = Math::ivec2{ sprite_frame * OLIM_FRAME_SIZE.x, 0 };
    const auto frame_size   = OLIM_FRAME_SIZE;
    const auto hot_spot     = OLIM_HOT_SPOT;

    const auto to_center = Math::TranslationMatrix(Math::vec2{ static_cast<double>(-hot_spot.x), static_cast<double>(-hot_spot.y) });
    const auto scale     = character.faceRight ? Math::ScaleMatrix({ 1.0, 1.0 }) : Math::ScaleMatrix({ -1.0, 1.0 });
    const auto translate = Math::TranslationMatrix(character.position);
    const auto transform = translate * scale * to_center;

    if (olimTexture)
    { 
        olimTexture->Draw(transform, texel_base, frame_size);
    }
}

DemoAstar::RenderInfo DemoAstar::beginOffscreenRendering() const
{
    return {};
}

GLuint DemoAstar::endOffscreenRendering([[maybe_unused]] const RenderInfo& render_info) const
{
    return 0;
}