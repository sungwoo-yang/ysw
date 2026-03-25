/**
 * \file
 * \author Rudy Castan
 * \author Jonathan Holmes
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#pragma once
#include "CS200/Image.hpp"
#include "Rect.hpp"
#include "Texture.hpp"
#include "Vec2.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

namespace CS230
{
    /**
     * \brief Bitmap font system for rendering text using pre-generated font textures
     *
     * Font provides a lightweight and efficient text rendering system based on
     * bitmap font textures. Rather than using complex font rasterization at runtime,
     * this system loads pre-rendered character sets from image files and uses them
     * to compose text strings quickly and efficiently.
     *
     * Bitmap Font Advantages:
     * - Consistent visual appearance across all platforms and systems
     * - Fast rendering performance with minimal runtime computation
     * - Predictable memory usage and loading times
     * - Simple integration with existing texture and rendering systems
     * - Pixel-perfect control over character appearance and spacing
     *
     * Font File Format:
     * The system expects specially formatted font textures where characters are
     * arranged horizontally in a single row, with specific color markers to
     * indicate character boundaries. The first pixel must be white, and color
     * changes signal the separation between characters.
     *
     * Character Set:
     * Supports ASCII characters from space (' ') to lowercase 'z', covering
     * basic alphanumeric characters, punctuation, and symbols commonly used
     * in game interfaces and debugging displays.
     *
     * Text Rendering Workflow:
     * 1. Load font texture and parse character boundaries during construction
     * 2. Measure text dimensions for layout calculations
     * 3. Render text to texture or directly to screen using character sprites
     * 4. Support for colored text and transformation matrices
     *
     * This font system is particularly well-suited for game development where
     * consistent, fast text rendering is more important than dynamic font
     * sizing or complex typography features.
     */
    class Font
    {
    public:
        /**
         * \brief Load and initialize a bitmap font from an image file
         * \param file_name Path to the font texture image file
         *
         * Constructs a Font object by loading a specially formatted bitmap font
         * texture and parsing the character boundary information. The constructor
         * analyzes the font image to determine where each character begins and
         * ends, creating a lookup table for efficient character rendering.
         *
         * Font Image Requirements:
         * - Characters arranged horizontally in a single row
         * - First pixel must be white (0xFFFFFFFF) as a format marker
         * - Color changes between characters indicate boundaries
         * - Characters cover ASCII range from space (' ') to 'z'
         * - Image should contain exactly the expected number of characters
         *
         * Parsing Process:
         * The constructor scans the top row of pixels to detect color changes,
         * which indicate character boundaries. Each character's rectangular
         * region is calculated and stored for later use during text rendering.
         *
         * Error Handling:
         * If the font file is malformed (wrong format, missing characters, or
         * incorrect structure), the constructor will throw an error to indicate
         * the problem. This ensures that only valid fonts are used for rendering.
         *
         * The loaded font is immediately ready for text rendering operations
         * and will remain valid for the lifetime of the Font object.
         */
        Font(const std::filesystem::path& file_name);

        /**
         * \brief Render text string to a cached texture for efficient reuse
         * \param text String of text to render using this font
         * \param color RGBA color value for the text (default: white)
         * \return Shared pointer to texture containing the rendered text (cached if previously rendered)
         *
         * Creates or retrieves a cached texture containing the rendered text string,
         * allowing the text to be used like any other texture in the rendering system.
         * This approach enables text to be drawn with transformations, effects,
         * and blending modes just like sprite graphics.
         *
         * Advanced Caching System:
         * - Cache key: Combination of text string and color (format: "text_0xCOLOR")
         * - Frame tracking: Each cached texture records when it was last accessed
         * - Reference counting: Uses shared_ptr for automatic memory management
         * - Intelligent cleanup: Removes old textures only when they have single reference and are older than 60 frames
         * - Performance optimization: Eliminates redundant text rendering operations
         *
         * Caching Strategy:
         * 1. Check cache using generated key (text + color combination)
         * 2. If found: Update last-used frame timestamp and return cached texture
         * 3. If not found: Create new texture, cache it, and return
         * 4. Periodic cleanup: Remove textures older than 60 frames with reference count of 1
         *
         * Rendering Process (for new textures):
         * 1. Measure total text dimensions to determine optimal texture size
         * 2. Create render target texture using TextureManager
         * 3. Render each character from font atlas to the target texture
         * 4. Store in cache with current frame timestamp
         * 5. Return shared_ptr for client use
         *
         * Memory Management Benefits:
         * - Shared ownership: Multiple objects can reference the same text texture
         * - Automatic cleanup: Textures are removed when no longer referenced elsewhere
         * - Frame-based aging: Prevents immediate removal of frequently used text
         * - Cache efficiency: Only removes textures that are truly unused
         *
         * Text-to-Texture Advantages:
         * - Caching eliminates redundant text rendering for repeated strings
         * - Supports full transformation matrices (rotation, scaling, positioning)
         * - Enables text participation in sprite batching and effects systems
         * - Provides consistent rendering performance regardless of text complexity
         * - Allows text to be treated as standard texture assets
         *
         * Color and Formatting:
         * The color parameter applies a tint to the entire text string while
         * preserving original font character shapes, anti-aliasing, and spacing.
         * Different colors of the same text are cached separately for maximum
         * flexibility without color bleeding between cache entries.
         */
        std::shared_ptr<Texture> PrintToTexture(const std::string& text, CS200::RGBA color = 0xFFFFFFFF);

    private:
        void         FindCharRects();
        Math::irect& GetCharRect(char c);
        Math::ivec2  MeasureText(const std::string& text);
        void         DrawChar(Math::TransformationMatrix& matrix, char c, CS200::RGBA color);
        CS200::RGBA  GetPixel(Math::ivec2 texel) const;
        void         CleanupCache();

        CS200::Image             original_image;
        std::shared_ptr<Texture> texture_ptr;
        static constexpr int     num_chars = '~' - ' ' + 1;
        Math::irect              char_rects[num_chars];

        std::unordered_map<std::string, std::shared_ptr<Texture>> text_cache;
        std::unordered_map<std::string, uint64_t>                 cache_timestamps;
        static constexpr uint64_t                                 CACHE_CLEANUP_INTERVAL = 300;
    };
}
