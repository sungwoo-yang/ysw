/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include "Engine/Vec2.hpp"
#include "OpenGL/Texture.hpp"
#include "RGBA.hpp"

namespace Math
{
    class TransformationMatrix;
}

namespace CS200
{
    /**
     * \brief Abstract interface for 2D rendering systems
     *
     * IRenderer2D defines the contract that all 2D renderers must implement, providing
     * a consistent API for drawing sprites, shapes, and lines in 2D graphics applications.
     * This interface abstracts away the specific rendering implementation (OpenGL, DirectX, etc.)
     * allowing for different rendering backends or optimization strategies.
     *
     * Design Philosophy:
     * - Simple, immediate-mode style API for ease of use
     * - Hardware-accelerated rendering for performance
     * - Transformation matrix support for flexible positioning/scaling/rotation
     * - Consistent color and coordinate system handling
     * - Resource management through Init/Shutdown lifecycle
     *
     * Rendering Model:
     * - Scene-based rendering: BeginScene() → Draw*() calls → EndScene()
     * - Transform-based positioning using transformation matrices
     * - Texture-based sprite rendering with tinting and UV mapping
     * - Vector-based shape rendering with fill and outline support
     * - Immediate submission (no built-in batching at this level)
     *
     * Implementation Requirements:
     * - Must handle OpenGL resource management (shaders, buffers, VAOs)
     * - Should support camera/view transformations
     * - Must implement proper cleanup in destructor
     * - Should handle edge cases (zero-size shapes, invalid textures)
     *
     * Common Use Cases:
     * - Game sprite rendering (characters, backgrounds, UI elements)
     * - Debug visualization (bounding boxes, collision shapes, debug lines)
     * - 2D graphics applications (drawing tools, data visualization)
     * - Educational graphics programming
     *
     * Example Implementation Structure:
     * \code
     * class MyRenderer2D : public IRenderer2D
     * {
     * private:
     *     // OpenGL resources
     *     GLuint shader_program;
     *     GLuint vertex_array_object;
     *     GLuint vertex_buffer, index_buffer;
     *
     *     // Rendering state
     *     Math::TransformationMatrix view_projection_matrix;
     *
     * public:
     *     void Init() override {  setup OpenGL resources  }
     *     void Shutdown() override {  cleanup OpenGL resources  }
     *     // ... implement all virtual functions
     * };
     * \endcode
     */
    class IRenderer2D
    {
    public:
        /**
         * \brief Virtual destructor ensures proper cleanup of derived classes
         *
         * Implementation notes:
         * - Derived classes should call Shutdown() in their destructor
         * - Must be virtual to enable polymorphic destruction
         * - Should handle cleanup even if Shutdown() wasn't explicitly called
         */
        virtual ~IRenderer2D() = default;

        /**
         * \brief Initialize the renderer and allocate necessary resources
         *
         * This method sets up all OpenGL resources needed for rendering including
         * shaders, vertex buffers, vertex array objects, and any other GPU resources.
         * Must be called before any drawing operations.
         *
         * \throws May throw exceptions if OpenGL resources cannot be created
         */
        virtual void Init() = 0;

        /**
         * \brief Clean up all allocated resources
         *
         * Releases all OpenGL resources allocated during Init(). Should restore
         * the renderer to its pre-Init() state. Must be called before destruction
         * or when switching rendering contexts.
         *
         */
        virtual void Shutdown() = 0;

        /**
         * \brief Begin a new rendering frame with camera transformation
         * \param view_projection Combined view and projection matrix
         *
         * Sets up the rendering pipeline for a new frame. The view_projection matrix
         * transforms from world coordinates to normalized device coordinates (-1 to 1).
         * All subsequent draw calls will use this transformation.
         *
         */
        virtual void BeginScene(const Math::TransformationMatrix& view_projection) = 0;

        /**
         * \brief End the current rendering frame
         *
         * Finalizes the current frame.
         */
        virtual void EndScene() = 0;

        /**
         * \brief Draw a textured quadrilateral with transformation and tinting
         * \param transform World transformation matrix (position, rotation, scale)
         * \param texture OpenGL texture handle to sample from
         * \param texture_coord_bl Bottom-left UV coordinate (default: {0,0})
         * \param texture_coord_tr Top-right UV coordinate (default: {1,1})
         * \param tintColor Color to multiply with texture samples (default: white)
         *
         * Renders a textured quad that can represent sprites, backgrounds, or UI elements.
         * The quad is defined in local coordinates from -0.5 to 0.5, then transformed
         * by the given matrix.
         *
         */
        virtual void DrawQuad(
            const Math::TransformationMatrix& transform, OpenGL::TextureHandle texture, Math::vec2 texture_coord_bl = Math::vec2{ 0.0, 0.0 }, Math::vec2 texture_coord_tr = Math::vec2{ 1.0, 1.0 },
            CS200::RGBA tintColor = CS200::WHITE) = 0;

        /**
         * \brief Draw a circle with optional fill and outline
         * \param transform World transformation matrix (position, rotation, scale)
         * \param fill_color Interior color (default: transparent)
         * \param line_color Outline color (default: white)
         * \param line_width Outline thickness in world units (default: 2.0)
         *
         * Renders a circle using the transform's scale to determine radius.
         * Can render filled circles, outlined circles, or both.
         *
         */
        virtual void DrawCircle(const Math::TransformationMatrix& transform, CS200::RGBA fill_color = CS200::CLEAR, CS200::RGBA line_color = CS200::WHITE, double line_width = 2.0) = 0;

        /**
         * \brief Draw a rectangle with optional fill and outline
         * \param transform World transformation matrix (position, rotation, scale)
         * \param fill_color Interior color (default: transparent)
         * \param line_color Outline color (default: white)
         * \param line_width Outline thickness in world units (default: 2.0)
         *
         * Renders a rectangle using the transform's scale to determine size.
         * Can render filled rectangles, outlined rectangles, or both.
         *
         */
        virtual void DrawRectangle(const Math::TransformationMatrix& transform, CS200::RGBA fill_color = CS200::CLEAR, CS200::RGBA line_color = CS200::WHITE, double line_width = 2.0) = 0;

        /**
         * \brief Draw a line segment with additional transformation
         * \param transform Additional transformation to apply to line endpoints
         * \param startPoint Starting point in local coordinates
         * \param endPoint Ending point in local coordinates
         * \param line_color Color of the line (default: white)
         * \param line_width Thickness of the line in world units (default: 2.0)
         *
         * Renders a line between two points after applying the transform.
         * Useful for drawing lines relative to objects or in transformed coordinate spaces.
         *
         */
        virtual void DrawLine(const Math::TransformationMatrix& transform, Math::vec2 startPoint, Math::vec2 endPoint, CS200::RGBA line_color = CS200::WHITE, double line_width = 2.0) = 0;

        /**
         * \brief Draw a line segment in world coordinates
         * \param start_point Starting point in world coordinates
         * \param end_point Ending point in world coordinates
         * \param line_color Color of the line (default: white)
         * \param line_width Thickness of the line in world units (default: 2.0)
         *
         * Convenience method for drawing lines directly in world space without
         * additional transformation. Equivalent to calling the other DrawLine()
         * with an identity transformation matrix.
         *
         */
        virtual void DrawLine(Math::vec2 start_point, Math::vec2 end_point, CS200::RGBA line_color = CS200::WHITE, double line_width = 2.0) = 0;
    };

}
