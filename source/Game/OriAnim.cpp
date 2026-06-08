#include "OriAnim.hpp"
#include "OriAnimData.hpp"

#include "Engine/Engine.hpp"
#include "Engine/Matrix.hpp"
#include "Engine/TextureManager.hpp"
#include <algorithm>

void OriAnimation::Build() {
    auto& texMgr = Engine::GetTextureManager();

    for (const auto& cd : ANIM_CLIPS) {
        OriClip clip;
        clip.name = cd.name;
        clip.fps  = cd.fps;
        clip.loop = cd.loop;

        if      (cd.name == std::string("run"))        clip.scale = 1.5f;
        else if (cd.name == std::string("jump"))       clip.scale = 1.5f;
        else if (cd.name == std::string("doublejump")) clip.scale = 1.5f;
        else                                           clip.scale = 1.0f;

        for (const auto& fd : cd.frames) {
            auto tex = texMgr.Load(std::string(ATLAS_DIR) + fd.png);
            if (!tex) continue;

            OriFrame f;
            f.texture  = tex;
            f.texel    = { fd.x, fd.y };
            f.crop     = { fd.w, fd.h };
            f.displayW = static_cast<int>(fd.w / 2 * clip.scale);
            f.displayH = static_cast<int>(fd.h / 2 * clip.scale);
            clip.frames.push_back(std::move(f));
        }

        if (!clip.frames.empty())
            AddClip(std::move(clip));
    }

    Play("idle");
}

void OriAnimation::AddClip(OriClip clip) {
    std::string n = clip.name;
    clips[n]      = std::move(clip);
}

void OriAnimation::Play(const std::string& name) {
    if (current == name) return;
    current = name;
    timer   = 0.f;
    frame   = 0;
}

void OriAnimation::Update(double dt) {
    auto it = clips.find(current);
    if (it == clips.end() || it->second.frames.empty()) return;

    const OriClip& clip = it->second;
    timer += static_cast<float>(dt);

    float dur = 1.f / clip.fps;
    while (timer >= dur) {
        timer -= dur;
        ++frame;
        if (frame >= static_cast<int>(clip.frames.size()))
            frame = clip.loop ? 0 : static_cast<int>(clip.frames.size()) - 1;
    }
}

void OriAnimation::Draw(Math::vec2 feet_pos, bool facing_right) const {
    auto it = clips.find(current);
    if (it == clips.end() || it->second.frames.empty()) return;

    const OriFrame& f = it->second.frames[frame];

    // Texture::Draw internally multiplies by ScaleMatrix(frame_size), so we pass
    // the ratio (displaySize / frame_size) — not displaySize itself.
    double sx = (facing_right ? 1.0 : -1.0) * static_cast<double>(f.displayW) / f.crop.x;
    double sy = static_cast<double>(f.displayH) / f.crop.y;

    // Bottom-center anchor: sprite bottom at feet_pos.y, center-x at feet_pos.x
    // (the internal TranslationMatrix(frame_size/2)*scale shifts the quad's left edge
    //  to ox, so we subtract half the display width to center it)
    double ox = feet_pos.x + (facing_right ? -1.0 : 1.0) * f.displayW * 0.5;
    double oy = feet_pos.y;

    Math::TransformationMatrix mat =
        Math::TranslationMatrix(Math::vec2{ ox, oy }) *
        Math::ScaleMatrix(Math::vec2{ sx, sy });

    f.texture->Draw(mat, f.texel, f.crop);
}

bool OriAnimation::Finished() const {
    auto it = clips.find(current);
    if (it == clips.end()) return true;
    return !it->second.loop &&
           frame >= static_cast<int>(it->second.frames.size()) - 1;
}
