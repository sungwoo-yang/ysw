/**
 * \file
 * \author Rudy Castan
 * \author Jonathan Holmes
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#include "Font.hpp"
#include "CS200/Image.hpp"
#include "Engine.hpp"
#include "Error.hpp"
#include "Matrix.hpp"
#include "TextureManager.hpp"
#include <algorithm>

namespace CS230
{
    Font::Font(const std::filesystem::path& file_name) : original_image(file_name, false)
    {
        const Math::ivec2 img_size = original_image.GetSize();
        if (img_size.x <= 0 || img_size.y <= 0)
        {
            Engine::GetLogger().LogError("Font: invalid image size: " + file_name.string());
            throw_error_message("Invalid font image");
        }

        if (GetPixel({ 0, 0 }) != CS200::WHITE)
        {
            Engine::GetLogger().LogEvent("Font: top-left pixel is not WHITE (expected boundary line).");
        }

        texture_ptr = Engine::GetTextureManager().Load(file_name);
        if (!texture_ptr)
        {
            Engine::GetLogger().LogError("Font: failed to load texture: " + file_name.string());
            throw_error_message("Failed to load font texture");
        }

        FindCharRects();
        Engine::GetLogger().LogEvent("Font loaded: " + file_name.string());
    }

    CS200::RGBA Font::GetPixel(Math::ivec2 texel) const
    {
        const Math::ivec2 size = original_image.GetSize();
        if (texel.x < 0 || texel.x >= size.x || texel.y < 0 || texel.y >= size.y)
            return CS200::CLEAR;
        return original_image.data()[texel.y * size.x + texel.x];
    }

    void Font::FindCharRects()
    {
        const Math::ivec2 size = original_image.GetSize();
        if (size.x <= 1)
        {
            Engine::GetLogger().LogError("Font: image too narrow to parse characters.");
            return;
        }

        int         last_x_pos   = 1;
        CS200::RGBA last_color   = GetPixel({ 0, 0 });
        char        current_char = ' ';

        for (int x = 1; x < size.x; ++x)
        {
            const CS200::RGBA cur = GetPixel({ x, 0 });
            if (cur != last_color)
            {
                if (current_char <= 'z')
                {
                    char_rects[current_char - ' '] = Math::irect{
                        { last_x_pos,      1 },
                        {          x, size.y }
                    };
                }

                last_x_pos = x;
                last_color = cur;
                ++current_char;

                if (current_char > 'z')
                    break;
            }
        }

        if (current_char <= 'z' && last_x_pos < size.x)
        {
            char_rects[current_char - ' '] = Math::irect{
                { last_x_pos,      1 },
                {     size.x, size.y }
            };
        }

        if (current_char < 'z')
        {
            Engine::GetLogger().LogEvent("Font: scan ended early at '" + std::string(1, current_char) + "'.");
        }
    }

    Math::irect& Font::GetCharRect(char c)
    {
        if (c >= ' ' && c <= 'z')
            return char_rects[c - ' '];
        return char_rects[0];
    }

    Math::ivec2 Font::MeasureText(const std::string& text)
    {
        int total_w = 0, max_h = 0;
        for (char c : text)
        {
            const Math::irect& r = GetCharRect(c);
            const Math::ivec2  s = { static_cast<int>(r.Size().x), static_cast<int>(r.Size().y) };
            total_w += s.x;
            if (s.y > max_h)
                max_h = s.y;
        }
        return { total_w, max_h };
    }

    void Font::CleanupCache()
    {
        const uint64_t frame = Engine::GetWindowEnvironment().FrameCount;

        static uint64_t last_cleanup_frame = 0;
        if (frame < last_cleanup_frame + CACHE_CLEANUP_INTERVAL)
            return;
        last_cleanup_frame = frame;

        for (auto it = text_cache.begin(); it != text_cache.end();)
        {
            auto time_it = cache_timestamps.find(it->first);
            if (time_it != cache_timestamps.end() && (frame - time_it->second) > 60 && it->second.use_count() == 1)
            {
                cache_timestamps.erase(time_it);
                it = text_cache.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    std::shared_ptr<Texture> Font::PrintToTexture(const std::string& text, CS200::RGBA color)
    {
        CleanupCache();

        char color_str[16];
        snprintf(color_str, sizeof(color_str), "%08X", color);
        std::string cache_key = text + "_" + color_str;

        if (auto it = text_cache.find(cache_key); it != text_cache.end())
        {
            cache_timestamps[cache_key] = Engine::GetWindowEnvironment().FrameCount;
            return it->second;
        }

        const Math::ivec2 size = MeasureText(text);
        if (size.x <= 0 || size.y <= 0)
            return nullptr;

        TextureManager::StartRenderTextureMode(size.x, size.y);
        int pen_x = 0;

        for (char c : text)
        {
            const Math::irect& rect = GetCharRect(c);
            const Math::ivec2  cs   = { static_cast<int>(rect.Size().x), static_cast<int>(rect.Size().y) };
            if (cs.x > 0 && cs.y > 0)
            {
                const Math::ivec2          texel_pos = { rect.point_1.x, rect.point_1.y };
                Math::TransformationMatrix tr        = Math::TranslationMatrix(Math::vec2{ static_cast<double>(pen_x), 0.0 });
                texture_ptr->Draw(tr, texel_pos, cs, color);
                pen_x += cs.x;
            }
        }

        auto result = TextureManager::EndRenderTextureMode();
        if (!result)
        {
            Engine::GetLogger().LogError("Font: failed to create text texture.");
            return nullptr;
        }

        text_cache[cache_key]       = result;
        cache_timestamps[cache_key] = Engine::GetWindowEnvironment().FrameCount;
        return result;
    }
}