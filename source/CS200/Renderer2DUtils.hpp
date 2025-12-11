/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include "Engine/Matrix.hpp"
#include "Engine/Vec2.hpp"
#include "RGBA.hpp"
#include <array>
#include <optional>

namespace CS200::Renderer2DUtils
{
    /**
     * \brief Utility functions for 2D rendering calculations and transformations
     *
     * Renderer2DUtils provides a collection of helper functions that handle common
     * mathematical operations needed for 2D rendering systems. These utilities
     * abstract away complex transformation calculations, color format conversions,
     * and geometry computations that are frequently needed by renderer implementations.
     */

    // Type aliases for OpenGL-compatible data formats
    using mat3 = std::array<float, 9>; ///< 3x3 matrix in column-major order for OpenGL
    using vec2 = std::array<float, 2>; ///< 2D vector for OpenGL uniform uploads

    /**
     * \brief Convert engine transformation matrix to OpenGL 3x3 matrix format
     * \param transform Engine transformation matrix (row-major, double precision)
     * \return 3x3 matrix in column-major order suitable for OpenGL uniforms
     *
     * Converts from the engine's internal matrix representation to the format
     * expected by OpenGL shaders. This handles both data type conversion
     * (double→float) and memory layout conversion (row-major→column-major).
     *
     * Implementation requirements:
     * - Convert from double precision to single precision floats
     * - Transpose matrix layout from row-major to column-major order
     * - Handle the 3x3 format used for 2D transformations
     * - Ensure proper element mapping: result[i] = transform[col][row]
     *
     * Memory Layout Conversion:
     * Engine (row-major): [m00, m01, m02, m10, m11, m12, m20, m21, m22]
     * OpenGL (col-major): [m00, m10, m20, m01, m11, m21, m02, m12, m22]
     */
    inline mat3 to_opengl_mat3(const Math::TransformationMatrix& transform) noexcept
    {
        return { static_cast<float>(transform[0][0]), static_cast<float>(transform[1][0]), static_cast<float>(transform[2][0]),
                 static_cast<float>(transform[0][1]), static_cast<float>(transform[1][1]), static_cast<float>(transform[2][1]),
                 static_cast<float>(transform[0][2]), static_cast<float>(transform[1][2]), static_cast<float>(transform[2][2]) };
    }

    /**
     * \brief Calculate transformation matrix for rendering a line segment as a rectangle
     * \param transform Base transformation to apply to the line
     * \param start_point Starting point of the line in local coordinates
     * \param end_point Ending point of the line in local coordinates
     * \param line_width Thickness of the line in world units
     * \return Complete transformation matrix for line rendering
     *
     * Converts a line defined by two points into a transformation matrix that positions
     * and orients a unit rectangle to represent that line. This allows line rendering
     * using standard quad/rectangle rendering primitives.
     *
     * Implementation requirements:
     * - Calculate line vector: end_point - start_point
     * - Determine line length: magnitude of line vector
     * - Calculate line angle: atan2(vector.y, vector.x)
     * - Find line center: midpoint between start and end points
     * - Create scale matrix: [line_length, line_width]
     * - Create rotation matrix from calculated angle
     * - Create translation matrix to center position
     * - Combine transformations: base_transform * translation * rotation * scale
     *
     * Mathematical Steps:
     * 1. Line vector = end - start
     * 2. Length = ||line_vector||
     * 3. Angle = atan2(line_vector.y, line_vector.x)
     * 4. Center = (start + end) * 0.5
     * 5. Final = transform * T(center) * R(angle) * S(length, width)
     *
     * Usage: The resulting matrix transforms a unit rectangle into the desired line shape
     */
    Math::TransformationMatrix CalculateLineTransform(const Math::TransformationMatrix& transform, const Math::vec2& start_point, const Math::vec2& end_point, double line_width) noexcept;

    /**
     * \brief Data structure containing transformation information for SDF shape rendering
     *
     * SDFTransform packages the mathematical data needed to render shapes using
     * Signed Distance Field (SDF) techniques. SDF rendering requires both the
     * quad transformation and size information for proper distance calculations.
     */
    struct SDFTransform
    {
        mat3 QuadTransform; ///< OpenGL transformation matrix for the rendering quad
        vec2 WorldSize;     ///< Original shape size in world coordinates
        vec2 QuadSize;      ///< Expanded quad size including line width padding
    };

    /**
     * \brief Calculate transformation data for SDF-based shape rendering
     * \param transform Base transformation matrix for the shape
     * \param line_width Width of the outline/border in world units
     * \return SDFTransform containing all data needed for SDF rendering
     *
     * SDF (Signed Distance Field) rendering requires special handling because the
     * rendered quad must be larger than the actual shape to accommodate anti-aliasing
     * and outline rendering. This function calculates the expanded quad size and
     * provides both world-space and quad-space dimensions for the shader.
     *
     * Implementation requirements:
     * - Extract world size from transform matrix scale components
     * - Calculate expanded quad size: world_size + (2 * line_width)
     * - Compute scale factors: quad_size / world_size
     * - Apply scale factors to transformation matrix
     * - Convert final transform to OpenGL format
     * - Package all data into SDFTransform struct
     *
     * Mathematical Process:
     * 1. Extract scale from transform: sqrt(m00^2 + m10^2), sqrt(m01^2 + m11^2)
     * 2. World size = extracted scale values
     * 3. Quad size = world size + (2 * line_width)
     * 4. Scale factors = quad_size / world_size
     * 5. Apply scale factors to transform matrix elements
     * 6. Convert to OpenGL matrix format
     *
     * Usage: The shader uses WorldSize for SDF calculations and QuadTransform for positioning
     */
    SDFTransform CalculateSDFTransform(const Math::TransformationMatrix& transform, double line_width) noexcept;
}
