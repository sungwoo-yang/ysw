/**
 * \file
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#include "Sprite.hpp"
#include "Collision.hpp"
#include "Engine.hpp"
#include "Logger.hpp"
#include "TextureManager.hpp"
#include "Path.hpp"
#include <fstream>

namespace CS230
{
    Sprite::Sprite(const std::filesystem::path& sprite_file)
    {
        Load(sprite_file);
    }

    Sprite::Sprite(const std::filesystem::path& sprite_file, GameObject* given_object) : object(given_object)
    {
        Load(sprite_file);
    }

    Sprite::~Sprite()
    {
        for (Animation* anime : animations)
        {
            delete anime;
        }
        animations.clear();
    }

    Sprite::Sprite(Sprite&& temporary) noexcept
        : texture(std::move(temporary.texture)), hotspots(std::move(temporary.hotspots)), current_animation(temporary.current_animation), frame_size(temporary.frame_size),
          frame_texels(std::move(temporary.frame_texels)), animations(std::move(temporary.animations)), object(temporary.object)
    {
        temporary.object = nullptr;
        temporary.animations.clear();
    }

    Sprite& Sprite::operator=(Sprite&& temporary) noexcept
    {
        if (this == &temporary)
        {
            return *this;
        }

        std::swap(texture, temporary.texture);
        std::swap(hotspots, temporary.hotspots);
        std::swap(current_animation, temporary.current_animation);
        std::swap(frame_size, temporary.frame_size);
        std::swap(frame_texels, temporary.frame_texels);
        std::swap(animations, temporary.animations);
        std::swap(object, temporary.object);
        return *this;
    }

    void Sprite::Update(double dt)
    {
        animations[static_cast<size_t>(current_animation)]->Update(dt);
    }

    void Sprite::Load(const std::filesystem::path& sprite_file)
    {
        if (sprite_file.extension() != ".spt")
        {
            throw std::runtime_error(sprite_file.generic_string() + " is not a .spt file");
        }
        std::ifstream in_file(assets::locate_asset(sprite_file));

        if (in_file.is_open() == false)
        {
            throw std::runtime_error("Failed to load " + assets::locate_asset(sprite_file).string());
        }

        hotspots.clear();
        frame_texels.clear();
        animations.clear();

        std::string text;
        in_file >> text;
        texture    = Engine::GetTextureManager().Load(text);
        frame_size = texture->GetSize();

        in_file >> text;
        while (in_file.eof() == false)
        {
            if (text == "FrameSize")
            {
                in_file >> frame_size.x;
                in_file >> frame_size.y;
            }
            else if (text == "NumFrames")
            {
                int frame_count;
                in_file >> frame_count;
                for (int i = 0; i < frame_count; i++)
                {
                    frame_texels.push_back({ frame_size.x * i, 0 });
                }
            }
            else if (text == "Frame")
            {
                int frame_location_x, frame_location_y;
                in_file >> frame_location_x;
                in_file >> frame_location_y;
                frame_texels.push_back({ frame_location_x, frame_location_y });
            }
            else if (text == "HotSpot")
            {
                int hotspot_x, hotspot_y;
                in_file >> hotspot_x;
                in_file >> hotspot_y;
                hotspots.push_back({ hotspot_x, hotspot_y });
            }
            else if (text == "Anim")
            {
                std::string anim_path;
                in_file >> anim_path;
                Engine::GetLogger().LogDebug("Reading animation: " + anim_path);
                animations.push_back(new Animation(anim_path));
            }
            else if (text == "RectCollision")
            {
                Math::irect boundary;
                in_file >> boundary.point_1.x >> boundary.point_1.y >> boundary.point_2.x >> boundary.point_2.y;
                if (object == nullptr)
                {
                    Engine::GetLogger().LogError("Cannot add collision to a null object");
                }
                else
                {
                    object->AddGOComponent(new RectCollision(boundary, object));
                }
            }
            else if (text == "CircleCollision")
            {
                double radius;
                in_file >> radius;

                if (object == nullptr)
                {
                    Engine::GetLogger().LogError("Cannot add collision to a null object");
                }
                else
                {
                    object->AddGOComponent(new CircleCollision(radius, object));
                }
            }
            else
            {
                Engine::GetLogger().LogError("Unknown command: " + text);
            }
            in_file >> text;
        }
        if (frame_texels.empty() == true)
        {
            frame_texels.push_back({ 0, 0 });
        }

        if (animations.empty())
        {
            animations.push_back(new Animation());
            PlayAnimation(0);
        }
    }

    void Sprite::Draw(Math::TransformationMatrix display_matrix)
    {
        texture->Draw(display_matrix * Math::TranslationMatrix(-GetHotSpot(0)), GetFrameTexel(animations[static_cast<size_t>(current_animation)]->CurrentFrame()), GetFrameSize());
    }

    Math::ivec2 Sprite::GetHotSpot(int index)
    {
        if (index < 0 && index >= static_cast<int>(hotspots.size()))
        {
            return { 0, 0 };
        }
        else
        {
            return hotspots[static_cast<size_t>(index)];
        }
    }

    Math::ivec2 Sprite::GetFrameSize()
    {
        return frame_size;
    }

    void Sprite::PlayAnimation(int animation)
    {
        if (animation >= 0 && animation < static_cast<int>(animations.size()))
        {
            current_animation = animation;
            animations[static_cast<size_t>(current_animation)]->Reset();
        }
        else
        {
            Engine::GetLogger().LogError("Invalid animation index.");
            return;
        }
    }

    bool Sprite::AnimationEnded()
    {
        return animations[static_cast<size_t>(current_animation)]->Ended();
    }

    Math::ivec2 Sprite::GetFrameTexel(int index) const
    {
        if (index < 0 || index >= int(frame_texels.size()))
        {
            Engine::GetLogger().LogError("Invalid frame index");
            return { 0, 0 };
        }
        return frame_texels[static_cast<size_t>(index)];
    }

    int Sprite::CurrentAnimation() const
    {
        return current_animation;
    }
}
