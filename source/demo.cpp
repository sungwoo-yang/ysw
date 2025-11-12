#include "BatchRenderer2D.hpp"
#include "FPS.hpp"
#include "ImmediateRenderer2D.hpp"
#include "InstancedRenderer2D.hpp"
#include "Path.hpp"
#include "Random.hpp"
#include <GL/glew.h>
#include <SDL.h>
#include <array>
#include <imgui.h>
#include <memory>
#include <stb_image.h>
#include <vector>

// Request high-performance GPU on systems with multiple GPUs (laptops with integrated + discrete)
// https://docs.nvidia.com/gameworks/content/technologies/desktop/optimus.htm
// https://gpuopen.com/learn/amdpowerxpressrequesthighperformance/
// https://gcc.gnu.org/wiki/Visibility

#if defined _WIN32 || defined __CYGWIN__
// Windows: Use __declspec(dllexport)
#    define GPU_EXPORT __declspec(dllexport)
#else
// Linux/Mac: Use visibility attribute
#    define GPU_EXPORT __attribute__((visibility("default")))
#endif

extern "C"
{
// NVIDIA Optimus: Request high-performance GPU
GPU_EXPORT unsigned long NvOptimusEnablement = 0x00000001;

// AMD PowerXpress: Request high-performance GPU
GPU_EXPORT int AmdPowerXpressRequestHighPerformance = 0x00000001;
}

// Simple vector types
struct vec2
{
    double x, y;
};

struct ivec2
{
    int x, y;
};

extern int gWidth;
extern int gHeight;

// Robot sprite sheet constants
static constexpr ivec2 ROBOT_FRAME_SIZE{ 63, 127 };
static constexpr int   ROBOT_NUM_FRAMES = 5;
static constexpr int   ROBOT_VARIATIONS = 64;

// Robot instance data
struct Robot
{
    vec2  position;
    int   frame;
    float r, g, b; // tint color
    int   variation;
};

std::vector<Robot>                           gRobots;
std::unique_ptr<IRenderer2D>                 gRenderer;
// OpenGL::Handle                               gRobotTexture = 0;
std::array<OpenGL::Handle, ROBOT_VARIATIONS> gRobotTextures{};
util::FPS                                    gFPSTracker;
Uint32                                       gLastTicks       = 0;
bool                                         gVSyncEnabled    = true;
const char*                                  gOpenGLRenderer  = nullptr;
int                                          gMaxTextureUnits = 0;

enum class RendererType
{
    Immediate,
    Batch,
    Instanced
};
RendererType gCurrentRenderer = RendererType::Immediate;

// Forward declarations
Robot CreateRandomRobot();
void  SwitchRenderer(RendererType type);

void demo_setup()
{
    // Cache OpenGL renderer info
    gOpenGLRenderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &gMaxTextureUnits);

    // Initialize renderer
    gRenderer = std::make_unique<ImmediateRenderer2D>();
    gRenderer->Init();

    glGenTextures(ROBOT_VARIATIONS, gRobotTextures.data());
    for (int i = 0; i < ROBOT_VARIATIONS; ++i)
    {
        // Load robot texture
        std::ostringstream sout;
        sout << "Assets/variations/robot_var_" << std::setfill('0') << std::setw(2) << (i + 1) << ".png";
        const std::filesystem::path image_path = assets::locate_asset(sout.str());
        const bool                  FLIP       = true;
        stbi_set_flip_vertically_on_load(FLIP);
        int        w = 0, h = 0;
        const int  num_channels       = 4;
        int        files_num_channels = 0;
        const auto image_bytes        = stbi_load(image_path.string().c_str(), &w, &h, &files_num_channels, num_channels);


        glBindTexture(GL_TEXTURE_2D, gRobotTextures[i]);

        // Texture filtering
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // GL_LINEAR
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // Texture wrapping
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        constexpr int base_mipmap_level = 0;
        constexpr int zero_border       = 0;
        glTexImage2D(GL_TEXTURE_2D, base_mipmap_level, GL_RGBA8, w, h, zero_border, GL_RGBA, GL_UNSIGNED_BYTE, image_bytes);
        stbi_image_free(image_bytes);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Create random robots
    constexpr int NUM_ROBOTS = 20;
    gRobots.reserve(NUM_ROBOTS);
    for (int i = 0; i < NUM_ROBOTS; ++i)
    {
        gRobots.push_back(CreateRandomRobot());
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    // Initialize VSync (adaptive vsync with fallback to regular vsync)
    // https://wiki.libsdl.org/SDL_GL_SetSwapInterval
    constexpr int ADAPTIVE_VSYNC = -1;
    constexpr int VSYNC          = 1;
    if (const auto result = SDL_GL_SetSwapInterval(ADAPTIVE_VSYNC); result != 0)
    {
        SDL_GL_SetSwapInterval(VSYNC);
    }

    // Initialize FPS tracking
    gLastTicks = SDL_GetTicks();
}

void demo_draw()
{
    // Update FPS tracker
    const Uint32 currentTicks = SDL_GetTicks();
    const Uint32 deltaTicks   = currentTicks - gLastTicks;
    const double deltaSeconds = deltaTicks / 1000.0;
    gLastTicks                = currentTicks;
    gFPSTracker.Update(deltaSeconds);

    glClearColor(0.34f, 0.56f, 0.9f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Create NDC transform matrix
    std::array<float, 9> to_ndc{
        2.0f / static_cast<float>(gWidth),
        0.0f,
        0.0f, // column 0
        0.0f,
        2.0f / static_cast<float>(gHeight),
        0.0f, // column 1
        0.0f,
        0.0f,
        1.0f // column 2
    };

    gRenderer->BeginScene(to_ndc);

    // Draw each robot
    for (const auto& robot : gRobots)
    {
        // Create transform matrix (scale by size and translate to position)
        const float width  = static_cast<float>(ROBOT_FRAME_SIZE.x);
        const float height = static_cast<float>(ROBOT_FRAME_SIZE.y);
        const float pos_x  = static_cast<float>(robot.position.x);
        const float pos_y  = static_cast<float>(robot.position.y);

        std::array<float, 9> transform{
            width, 0.0f,   0.0f, // column 0: scale X
            0.0f,  height, 0.0f, // column 1: scale Y
            pos_x, pos_y,  1.0f  // column 2: translation
        };

        // Texture coordinates for sprite frame selection (left, bottom, right, top)
        // The sprite sheet is laid out horizontally with 5 frames
        const float frame_width = 1.0f / static_cast<float>(ROBOT_NUM_FRAMES);
        const float left        = frame_width * static_cast<float>(robot.frame);
        const float right       = left + frame_width;
        const float bottom      = 0.0f;
        const float top         = 1.0f;

        std::array<float, 4> texture_coords{ left, bottom, right, top };

        // Tint color
        std::array<float, 4> tint{ robot.r, robot.g, robot.b, 1.0f };

        gRenderer->DrawQuad(transform, robot.variation, texture_coords, tint);
    }

    gRenderer->EndScene();
}

void demo_shutdown()
{
    glDeleteTextures(ROBOT_VARIATIONS, gRobotTextures.data());
    gRenderer->Shutdown();
    gRenderer.reset();
}

void demo_imgui()
{
    ImGui::Begin("Demo Settings");

    // Display FPS at the top
    ImGui::Text("FPS: %d", static_cast<int>(gFPSTracker));
    ImGui::Separator();

    // Display OpenGL renderer info
    if (gOpenGLRenderer)
    {
        ImGui::Text("OpenGL Renderer: %s", gOpenGLRenderer);
        ImGui::Text("Max Texture Units: %d", gMaxTextureUnits);
        ImGui::Separator();
    }

    // Renderer selection
    ImGui::Text("Renderer:");
    if (ImGui::RadioButton("Immediate", gCurrentRenderer == RendererType::Immediate))
    {
        SwitchRenderer(RendererType::Immediate);
    }
    ImGui::SameLine();

    if (ImGui::RadioButton("Batch", gCurrentRenderer == RendererType::Batch))
    {
        SwitchRenderer(RendererType::Batch);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Instanced", gCurrentRenderer == RendererType::Instanced))
    {
        SwitchRenderer(RendererType::Instanced);
    }
    ImGui::Separator();

    // VSync toggle
    if (ImGui::Checkbox("VSync", &gVSyncEnabled))
    {
        // https://wiki.libsdl.org/SDL_GL_SetSwapInterval
        constexpr int ADAPTIVE_VSYNC = -1;
        constexpr int VSYNC          = 1;
        constexpr int NO_VSYNC       = 0;

        if (gVSyncEnabled)
        {
            // Try adaptive vsync first, fall back to regular vsync
            if (const auto result = SDL_GL_SetSwapInterval(ADAPTIVE_VSYNC); result != 0)
            {
                SDL_GL_SetSwapInterval(VSYNC);
            }
        }
        else
        {
            SDL_GL_SetSwapInterval(NO_VSYNC);
        }
    }
    ImGui::Separator();

    // Display current robot count
    ImGui::Text("Current Robot Count: %zu", gRobots.size());
    ImGui::Separator();

    // Amounts for adding/removing
    constexpr int    amounts[]  = { 1, 10, 100, 1000, 10000, 100000 };
    constexpr size_t MAX_ROBOTS = 1000000; // Sanity limit to prevent crashes

    // Add robots buttons
    ImGui::Text("Add Robots:");
    for (int amount : amounts)
    {
        // Disable button if it would exceed max limit
        const bool can_add = (gRobots.size() + static_cast<size_t>(amount)) <= MAX_ROBOTS;
        if (!can_add)
        {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button((std::string("+") + std::to_string(amount)).c_str()))
        {
            const size_t new_count = std::min(gRobots.size() + static_cast<size_t>(amount), MAX_ROBOTS);
            const size_t to_add    = new_count - gRobots.size();

            if (to_add > 0)
            {
                gRobots.reserve(new_count);
                for (size_t i = 0; i < to_add; ++i)
                {
                    gRobots.push_back(CreateRandomRobot());
                }
            }
        }

        if (!can_add)
        {
            ImGui::EndDisabled();
        }
        ImGui::SameLine();
    }
    ImGui::NewLine();

    // Remove robots buttons
    ImGui::Text("Remove Robots:");
    for (int amount : amounts)
    {
        // Disable button if there are no robots to remove
        const bool can_remove = !gRobots.empty();
        if (!can_remove)
        {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button((std::string("-") + std::to_string(amount)).c_str()))
        {
            const size_t to_remove = std::min(static_cast<size_t>(amount), gRobots.size());
            if (to_remove > 0)
            {
                gRobots.resize(gRobots.size() - to_remove);
            }
        }

        if (!can_remove)
        {
            ImGui::EndDisabled();
        }
        ImGui::SameLine();
    }
    ImGui::NewLine();

    ImGui::Separator();

    // Clear all button
    const bool has_robots = !gRobots.empty();
    if (!has_robots)
    {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Clear All"))
    {
        gRobots.clear();
    }

    if (!has_robots)
    {
        ImGui::EndDisabled();
    }

    // Show warning when approaching limit
    if (gRobots.size() > static_cast<size_t>(MAX_ROBOTS * 0.8))
    {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Warning: Approaching maximum robot limit!");
    }

    ImGui::End();
}

// Helper function to create a random robot
Robot CreateRandomRobot()
{
    Robot       robot;
    const float half_width  = static_cast<float>(gWidth) / 2.0f;
    const float half_height = static_cast<float>(gHeight) / 2.0f;
    robot.position.x        = static_cast<double>(util::random(-half_width, half_width));
    robot.position.y        = static_cast<double>(util::random(-half_height, half_height));
    robot.frame             = util::random(ROBOT_NUM_FRAMES);
    if (util::random(0.0f, 1.0f) < 0.85f)
    {
        robot.r = robot.g = robot.b = 1.0f;
    }
    else
    {
        robot.r = util::random(0.5f, 1.0f);
        robot.g = util::random(0.6f, 1.0f);
        robot.b = util::random(0.45f, 1.0f);
    }
    robot.variation = gRobotTextures[util::random(ROBOT_VARIATIONS)];
    return robot;
}

// Helper function to switch between renderers
void SwitchRenderer(RendererType type)
{
    if (gCurrentRenderer == type)
        return; // Already using this renderer

    // Shutdown current renderer
    if (gRenderer)
    {
        gRenderer->Shutdown();
        gRenderer.reset();
    }

    // Create and initialize new renderer
    gCurrentRenderer = type;
    switch (type)
    {
        case RendererType::Immediate: gRenderer = std::make_unique<ImmediateRenderer2D>(); break;
        case RendererType::Batch: gRenderer = std::make_unique<BatchRenderer2D>(); break;
        case RendererType::Instanced: gRenderer = std::make_unique<InstancedRenderer2D>(); break;
        default: gRenderer = std::make_unique<ImmediateRenderer2D>(); break;
    }

    if (gRenderer)
    {
        gRenderer->Init();
    }
}
