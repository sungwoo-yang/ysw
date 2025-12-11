/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include "Engine/Vec2.hpp"
#include "Handle.hpp"
#include "Texture.hpp"

namespace OpenGL
{
    /**
     * \brief Descriptive alias for OpenGL framebuffer object handles
     *
     * FramebufferHandle provides a more specific and readable name for the generic
     * OpenGL handle type when referring to framebuffer objects. Like other handle
     * aliases, this improves code clarity without adding compile-time type safety.
     */
    using FramebufferHandle = Handle;

    /**
     * \brief Complete framebuffer setup with color attachment for render-to-texture operations
     *
     * FramebufferWithColor represents a fully configured OpenGL framebuffer that can
     * be used as a render target. This struct bundles together the framebuffer object
     * and its associated color texture, providing everything needed for off-screen
     * rendering or render-to-texture effects.
     *
     * Purpose and Use Cases:
     * - Render-to-texture operations (post-processing effects, shadows, reflections)
     * - Off-screen rendering for UI elements or HUD components
     * - Multi-pass rendering techniques (deferred rendering, G-buffers)
     * - Creating texture atlases or procedural textures at runtime
     * - Screen capture and screenshot functionality
     *
     * The framebuffer acts as a rendering target that redirects all drawing operations
     * to the attached texture instead of the screen. This enables sophisticated
     * graphics techniques where the rendered result becomes input for subsequent
     * rendering passes or post-processing effects.
     *
     * Resource Management:
     * Both the framebuffer and its color texture must be properly destroyed when
     * no longer needed to prevent OpenGL resource leaks. The struct design makes
     * it easy to manage these related resources as a single unit.
     */
    struct [[nodiscard]] FramebufferWithColor
    {
        /** \brief Handle to the OpenGL framebuffer object */
        FramebufferHandle Framebuffer = 0;

        /** \brief Handle to the color texture attached to this framebuffer */
        TextureHandle ColorAttachment = 0;
    };

    /**
     * \brief Create a complete framebuffer with color texture attachment
     * \param size Dimensions of the framebuffer and its color texture in pixels
     * \return Fully configured framebuffer ready for rendering operations
     *
     * Creates a complete framebuffer setup suitable for render-to-texture operations.
     * This function handles all the complex OpenGL setup required for off-screen
     * rendering, including texture creation, framebuffer configuration, and
     * attachment binding.
     *
     * The created framebuffer includes:
     * - A color texture attachment in RGBA format for storing rendered pixels
     * - Proper draw buffer configuration for color output
     * - Complete framebuffer validation to ensure it's ready for use
     *
     * Common usage patterns:
     * - Post-processing pipelines: render scene to texture, then apply effects
     * - Shadow mapping: render depth information from light's perspective
     * - Reflection/refraction: capture environment from different viewpoints
     * - UI rendering: create interface elements as textures
     *
     * The implementation creates an RGBA texture matching the specified dimensions
     * and attaches it as GL_COLOR_ATTACHMENT0. The framebuffer is validated for
     * completeness before being returned, throwing an exception if setup fails.
     *
     * Memory considerations:
     * The texture size directly affects GPU memory usage (width × height × 4 bytes
     * for RGBA). Large framebuffers can impact performance and memory consumption.
     */
    FramebufferWithColor CreateFramebufferWithColor(Math::ivec2 size);

    /**
     * \brief Safely destroy framebuffer and release all associated resources
     * \param framebuffer_with_color Framebuffer structure to destroy (will be reset to zero)
     *
     * Properly cleans up both the framebuffer object and its associated color texture,
     * preventing OpenGL resource leaks. This function ensures that all GPU resources
     * are returned to the system and the structure is reset to a safe state.
     *
     * Resource cleanup includes:
     * - Deleting the OpenGL framebuffer object
     * - Deleting the associated color texture
     * - Resetting both handles to zero for safety
     *
     * The function is designed to be safe to call multiple times on the same
     * structure, as it resets handles to zero after deletion. This prevents
     * double-deletion errors that could occur with manual resource management.
     *
     * Usage patterns:
     * - Call when framebuffer is no longer needed
     * - Include in cleanup/shutdown code
     * - Use in RAII wrapper destructors for automatic cleanup
     * - Call before reassigning framebuffer variables
     *
     * After calling this function, the framebuffer structure should not be used
     * for rendering operations until a new framebuffer is created.
     */
    void DestroyFramebufferWithColor(FramebufferWithColor& framebuffer_with_color) noexcept;
}
