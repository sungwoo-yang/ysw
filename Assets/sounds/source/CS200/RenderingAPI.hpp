/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include "Engine/Vec2.hpp"
#include "RGBA.hpp"

namespace CS200::RenderingAPI
{
    /**
     * \brief High-level rendering API abstraction for OpenGL initialization and state management
     * 
     * RenderingAPI provides a simplified, safe interface for common OpenGL operations that
     * are needed for basic 2D graphics applications. It abstracts away the complexity of
     * OpenGL state management, version checking, and debug setup while providing essential
     * rendering functionality.
     * 
     * Purpose and Design:
     * - Simplifies OpenGL initialization and version validation
     * - Provides safe defaults for 2D graphics rendering
     * - Abstracts common rendering state operations
     * - Handles cross-platform OpenGL differences
     * - Enables debug output in development builds
     * 
     * Key Features:
     * - OpenGL version validation and capability detection
     * - Automatic blend mode setup for alpha transparency
     * - Debug message callback registration (development builds)
     * - Viewport and clear color management
     * - Hardware capability logging
     * 
     * Rendering Setup:
     * The API configures OpenGL for optimal 2D rendering:
     * - Alpha blending enabled (SRC_ALPHA, ONE_MINUS_SRC_ALPHA)
     * - Depth testing disabled (not needed for 2D)
     * - Debug output enabled in development builds
     * - Texture and rendering limits detected and logged
     * 
     * Common Usage Pattern:
     * \code
     * // Application startup
     * RenderingAPI::Init();
     * RenderingAPI::SetClearColor(CS200::BLACK);
     * 
     * // Each frame
     * RenderingAPI::Clear();
     * RenderingAPI::SetViewport(window_size);
     * // ... render your content ...
     * \endcode
     * 
     */

    /**
     * \brief Initialize OpenGL context and set up rendering state for 2D graphics
     * 
     * Performs comprehensive OpenGL initialization including version validation,
     * capability detection, state setup, and debug configuration. This must be
     * called after creating an OpenGL context but before any rendering operations.
     * 
     * Implementation requirements:
     * - Query and validate OpenGL version (minimum required version check)
     * - Store major/minor version numbers in OpenGL environment
     * - Query hardware capabilities (max texture size, texture units, etc.)
     * - Enable debug output callback in development builds (OpenGL 4.3+)
     * - Configure blend mode for alpha transparency: GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA
     * - Disable depth testing (not needed for 2D rendering)
     * - Log all OpenGL settings and capabilities for debugging
     * 
     * OpenGL State Configuration:
     * - Blending: Enabled with standard alpha blending
     * - Depth Test: Disabled for 2D rendering
     * - Debug Output: Enabled in development builds
     * - Error Checking: Debug callbacks for OpenGL errors
     * 
     * Error Handling:
     * - Throw exception if OpenGL version is insufficient
     * - Log warnings for missing optional features
     * - Validate that required extensions are available
     * 
     * \throws std::runtime_error if minimum OpenGL version not supported
     * \note Must be called with valid OpenGL context
     */
    void Init() noexcept;
    
    /**
     * \brief Set the background clear color for the framebuffer
     * \param color Packed RGBA color value for background clearing
     * 
     * Configures the color used when clearing the framebuffer with Clear().
     * The color persists until changed, so this typically only needs to be
     * called once during initialization or when changing scenes.
     * 
     * Color Format:
     * - Input: 32-bit packed RGBA (0xRRGGBBAA)
     * - Output: 4 float values in range [0.0, 1.0]
     * - Conversion: component_float = component_int / 255.0f
     * 
     * \note Does not immediately clear the framebuffer, only sets the clear color
     * \note Color persists until explicitly changed
     */
    void SetClearColor(CS200::RGBA color) noexcept;
    
    /**
     * \brief Clear the framebuffer with the previously set clear color
     * 
     * Clears the color buffer using the color specified by the most recent
     * SetClearColor() call. This is typically called once per frame before
     * rendering to provide a clean background.
     * 
     * Usage Pattern:
     * - Call once per frame before any drawing operations
     * - Ensures clean background for transparent/blended rendering
     * - Required for proper frame-to-frame rendering
     * 
     * \note Must be called within valid OpenGL context
     * \note Only clears color buffer, not depth/stencil buffers
     */
    void Clear() noexcept;
    
    /**
     * \brief Set the rendering viewport (visible area) within the window
     * \param size Width and height of the viewport in pixels
     * \param anchor_left_bottom Bottom-left corner position (default: {0,0})
     * 
     * Defines the rectangular area of the window where rendering will appear.
     * This controls the coordinate space transformation from normalized device
     * coordinates (-1 to +1) to window pixel coordinates.
     * 
     * Coordinate System:
     * - anchor_left_bottom: Bottom-left corner in window coordinates
     * - size: Width and height extending from anchor point
     * - OpenGL coordinate system: (0,0) at bottom-left of window
     * 
     * Common Use Cases:
     * - Full window rendering: SetViewport(window_size, {0,0})
     * - Split-screen rendering: Multiple viewports with different anchors
     * - UI overlay rendering: Specific rectangular regions
     * - Aspect ratio preservation: Letterboxing/pillarboxing
     * 
     * \note Affects all subsequent rendering operations
     * \note Coordinates use OpenGL convention (bottom-left origin)
     */
    void SetViewport(Math::ivec2 size, Math::ivec2 anchor_left_bottom = { 0, 0 }) noexcept;
}
