/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include <array>
#include <cstdint>

namespace CS200
{
    /**
     * \brief Compact 32-bit color representation for efficient graphics operations
     *
     * RGBA provides a memory-efficient way to store and manipulate colors using a single
     * 32-bit integer. This format is widely used in graphics programming because it packs
     * four 8-bit color channels (Red, Green, Blue, Alpha) into one value that can be
     * easily passed around, stored in arrays, and processed by graphics hardware.
     *
     * The color format uses the layout: 0xRRGGBBAA where:
     * - RR = Red channel (bits 31-24)
     * - GG = Green channel (bits 23-16)
     * - BB = Blue channel (bits 15-8)
     * - AA = Alpha channel (bits 7-0)
     *
     * Each channel ranges from 0x00 (0) to 0xFF (255), providing 256 levels per channel
     * and over 16 million possible color combinations with transparency support.
     *
     * This compact representation is ideal for:
     * - Storing vertex colors in graphics buffers
     * - Passing colors as function parameters
     * - Performing fast bitwise color operations
     * - Interfacing with graphics APIs that expect packed colors
     */
    using RGBA            = uint32_t;
    constexpr RGBA WHITE  = 0xffffffff;
    constexpr RGBA BLACK  = 0x000000ff;
    constexpr RGBA RED    = 0xff0000ff;
    constexpr RGBA GREEN  = 0x00ff00ff;
    constexpr RGBA BLUE   = 0x0000ffff;
    constexpr RGBA CYAN   = 0x00ffffff;
    constexpr RGBA YELLOW = 0xffff00ff;
    constexpr RGBA CLEAR  = 0x00000000;

    /**
     * \brief Convert packed RGBA color to floating-point component array
     * \param rgba 32-bit packed color in RGBA format
     * \return Array of 4 floats [r, g, b, a] normalized to [0.0, 1.0] range
     *
     * Unpacks a compact RGBA color into separate floating-point components that
     * are commonly required by graphics APIs like OpenGL. Each 8-bit integer
     * channel (0-255) is converted to a normalized float (0.0-1.0).
     *
     * This conversion is essential for:
     * - Passing colors to OpenGL shader uniforms
     * - Performing floating-point color math operations
     * - Interfacing with APIs that expect normalized color values
     * - Blending and interpolation calculations
     *
     * The bit extraction isolates each color channel:
     * - Red: (rgba & 0xff000000) >> 24
     * - Green: (rgba & 0x00ff0000) >> 16
     * - Blue: (rgba & 0x0000ff00) >> 8
     * - Alpha: (rgba & 0x000000ff) >> 0
     *
     * Each extracted value is then divided by 255.0f to normalize to [0.0, 1.0].
     */
    constexpr std::array<float, 4> unpack_color(RGBA rgba) noexcept
    {
        constexpr float scale = 1.0f / 255.0f;
        return { static_cast<float>((rgba & 0xff000000) >> 24) * scale, static_cast<float>((rgba & 0x00ff0000) >> 16) * scale, static_cast<float>((rgba & 0x0000ff00) >> 8) * scale,
                 static_cast<float>((rgba & 0x000000ff) >> 0) * scale };
    }

    /**
     * \brief Convert floating-point color components to packed RGBA format
     * \param color Array of 4 floats [r, g, b, a] in normalized [0.0, 1.0] range
     * \return 32-bit packed color in RGBA format (0xRRGGBBAA)
     *
     * Packs separate floating-point color components into a compact RGBA color
     * suitable for efficient storage and graphics operations. This is the inverse
     * operation of unpack_color(), converting normalized float values back into
     * the 8-bit integer format used by graphics hardware and APIs.
     *
     * This conversion is essential for:
     * - Converting computed color results back to graphics-friendly format
     * - Storing blended or interpolated colors efficiently
     * - Interfacing with graphics APIs that expect packed color values
     * - Optimizing memory usage in color arrays and vertex buffers
     *
     * The packing process:
     * 1. Scale each float component from [0.0, 1.0] to [0.0, 255.0]
     * 2. Clamp values to valid range [0, 255] to handle out-of-bounds inputs
     * 3. Convert to 8-bit integers (uint8_t)
     * 4. Shift and combine channels into final RGBA format:
     *    - Red: bits 31-24 (R << 24)
     *    - Green: bits 23-16 (G << 16)
     *    - Blue: bits 15-8 (B << 8)
     *    - Alpha: bits 7-0 (A << 0)
     *
     * Input values are automatically clamped to [0.0, 1.0] range to prevent
     * overflow and ensure valid color output even with out-of-range inputs.
     */
    constexpr RGBA pack_color(const std::array<float, 4>& color) noexcept
    {
        constexpr float scale = 255.0f;
        const float     r = color[0] * scale, g = color[1] * scale, b = color[2] * scale, a = color[3] * scale;
        const uint8_t   R = static_cast<uint8_t>(r <= 255.0f ? (r >= 0.0f ? r : 0.0f) : 255.0f);
        const uint8_t   G = static_cast<uint8_t>(g <= 255.0f ? (g >= 0.0f ? g : 0.0f) : 255.0f);
        const uint8_t   B = static_cast<uint8_t>(b <= 255.0f ? (b >= 0.0f ? b : 0.0f) : 255.0f);
        const uint8_t   A = static_cast<uint8_t>(a <= 255.0f ? (a >= 0.0f ? a : 0.0f) : 255.0f);
        return (static_cast<RGBA>(R) << 24) | (static_cast<RGBA>(G) << 16) | (static_cast<RGBA>(B) << 8) | (static_cast<RGBA>(A) << 0);
    }

    /**
     * \brief Convert RGBA format to ABGR format by reversing byte order
     * \param rgba Color in RGBA format (0xRRGGBBAA)
     * \return Color converted to ABGR format (0xAABBGGRR)
     *
     * Converts between different color byte ordering formats that are used by
     * various graphics systems and file formats. This conversion is necessary
     * when interfacing with systems that expect colors in ABGR order instead
     * of the standard RGBA order.
     *
     * The conversion process:
     * 1. Extract each color channel from the original RGBA value
     * 2. Rearrange the channels from RGBA order to ABGR order
     * 3. Pack the rearranged channels into a new 32-bit value
     *
     * Common use cases:
     * - Converting colors for specific graphics APIs or hardware
     * - Interfacing with image file formats that use different byte orders
     * - Preparing colors for systems with different endianness
     * - Working with legacy graphics formats that expect ABGR ordering
     *
     * The byte reversal ensures compatibility across different systems while
     * maintaining the same color information in a different memory layout.
     */
    constexpr uint32_t rgba_to_abgr(RGBA rgba) noexcept
    {
        const uint8_t r = static_cast<uint8_t>((rgba & 0xff000000) >> 24);
        const uint8_t g = static_cast<uint8_t>((rgba & 0x00ff0000) >> 16);
        const uint8_t b = static_cast<uint8_t>((rgba & 0x0000ff00) >> 8);
        const uint8_t a = static_cast<uint8_t>((rgba & 0x000000ff) >> 0);

        return (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(b) << 16) | (static_cast<uint32_t>(g) << 8) | (static_cast<uint32_t>(r) << 0);
    }


}
