#include "Particle.hpp"

CS230::Particle::Particle(const std::filesystem::path& sprite_file) : GameObject({ 0.0, 0.0 }), life(0.0) {
	AddGOComponent(new CS230::Sprite(sprite_file));
}

void CS230::Particle::Start(Math::vec2 pos, Math::vec2 vel, double max_life) {
    SetPosition(pos);
    SetVelocity(vel);
    life = max_life;

    auto sprite = GetGOComponent<CS230::Sprite>();
    if (sprite != nullptr) {
        sprite->PlayAnimation(0);
    }
}

void CS230::Particle::Update(double dt) {
    if (life > 0.0) {
        life -= dt;
        GameObject::Update(dt);
    }
}

void CS230::Particle::Draw(const Math::TransformationMatrix& camera_matrix) {
    if (life > 0.0) {
        GameObject::Draw(camera_matrix);
    }
}
