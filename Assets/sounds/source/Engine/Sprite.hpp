/**
 * \file
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#pragma once
#include "Animation.hpp"
#include "Component.hpp"
#include "GameObject.hpp"
#include "Matrix.hpp"
#include "Texture.hpp"
#include "Vec2.hpp"
#include <memory>
#include <string>

namespace CS230
{
    class GameObject;

    class Sprite : public Component
    {
    public:
        friend class GameObject;
        Sprite(const std::filesystem::path& sprite_file);
        Sprite(const std::filesystem::path& sprite_file, GameObject* object);
        ~Sprite();

        Sprite(const Sprite&)            = delete;
        Sprite& operator=(const Sprite&) = delete;

        Sprite(Sprite&& temporary) noexcept;
        Sprite& operator=(Sprite&& temporary) noexcept;

        void        Update(double dt) override;
        void        Load(const std::filesystem::path& sprite_file);
        void        Draw(Math::TransformationMatrix display_matrix);
        Math::ivec2 GetHotSpot(int index);
        Math::ivec2 GetFrameSize();

        void PlayAnimation(int animation);
        bool AnimationEnded();
        int  CurrentAnimation() const;

    private:
        Math::ivec2 GetFrameTexel(int index) const;

        std::shared_ptr<Texture> texture = nullptr;

        std::vector<Math::ivec2> hotspots;
        int                      current_animation;
        Math::ivec2              frame_size;
        std::vector<Math::ivec2> frame_texels;
        std::vector<Animation*>  animations;
        GameObject*              object;
    };
}