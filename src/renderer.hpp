#pragma once

#include "game.hpp"
#include <filesystem>
#include <string>

namespace cg {
class Renderer {
public:
    void resize(int width, int height);
    void draw(const Game& game) const;
    void saveFrame(const std::filesystem::path& path) const;
    int width() const { return width_; }
    int height() const { return height_; }
private:
    void character(const Character& character, bool enemy, EnemyKind kind = EnemyKind::Gunner) const;
    void barrel(const Barrel& barrel) const;
    void bossDetails(const Barrel& barrel) const;
    void barrelEnemy(const Barrel& barrel) const;
    void barrelHealth(const Barrel& barrel) const;
    void bullet(const Bullet& bullet) const;
    void hitboxes(const Game& game) const;
    void hud(const Game& game) const;
    void overlay(const Game& game) const;
    int width_ = 600;
    int height_ = 800;
};
}
