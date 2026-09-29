#pragma once
#include "game.hpp"

namespace cg {
struct GameTestAccess {
    static Character& player(Game& game) { return game.player_; }
    static std::vector<Barrel>& barrels(Game& game) { return game.barrels_; }
    static std::vector<Bullet>& bullets(Game& game) { return game.bullets_; }
    static void empty(Game& game) {
        game.barrels_.clear(); game.bullets_.clear();
        game.demo_.spawning = false; game.demo_.enemyFire = false;
    }
    static void addBarrel(Game& game, Vec2 p, int hp = 1, bool enemy = false) {
        Barrel b;
        b.position = p; b.hitPoints = b.maxHitPoints = hp; b.hasEnemy = enemy;
        b.width = game.config_.barrelWidth; b.height = game.config_.barrelHeight;
        b.enemy = {p, game.config_.enemyRadius, 180, 0, 0};
        game.barrels_.push_back(b);
        game.aimEnemy(game.barrels_.back());
    }
    static void addBullet(Game& game, Vec2 p, Vec2 v, Owner owner = Owner::Player, double r = 2) {
        game.bullets_.push_back({p, v, r, owner, true});
    }
    static void spawn(Game& game) { game.spawnBarrel(); }
    static void stage(Game& game, int stage) { game.startStage(stage); }
    static void bossAttack(Game& game, int pattern) {
        auto& b = game.barrels_.at(0);
        b.attackIndex = pattern;
        game.bossAttack(b);
    }
    static void aimEnemy(Game& game) { game.aimEnemy(game.barrels_.at(0)); }
    static void terminal(Game& game, Status status, LossReason loss = LossReason::None) {
        game.status_ = status; game.lossReason_ = loss;
        if (status == Status::Won) {
            game.stage_ = Game::stageCount;
            game.stageDestroyed_ = 1;
            game.destroyed_ = 8 * game.config_.barrelsToWin + 2;
        }
    }
};
}
