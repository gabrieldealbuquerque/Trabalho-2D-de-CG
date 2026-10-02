#pragma once

#include "config.hpp"
#include "math.hpp"
#include <array>
#include <random>
#include <vector>

namespace cg {
struct Character {
    Vec2 position;
    double radius = 1;
    double angle = 0;
    double gunAngle = 0;
    double walkPhase = 0;

    Vec2 gunPivot() const {
        return position + rotate({1.05 * radius, -0.35 * radius}, angle);
    }
    Vec2 muzzle() const {
        return gunPivot() + forward(angle + gunAngle) * (2.8 * radius);
    }
};

enum class BarrelKind { Normal, Miniboss, FinalBoss };
enum class EnemyKind { Gunner, SpreadGunner, Cannoneer };

struct Barrel {
    Vec2 position;
    int hitPoints = 1;
    int maxHitPoints = 1;
    double width = 1;
    double height = 1;
    BarrelKind kind = BarrelKind::Normal;
    bool hasEnemy = false;
    EnemyKind enemyKind = EnemyKind::Gunner;
    Character enemy;
    double shotClock = 0;
    double rollPhase = 0;
    double motionClock = 0;
    double telegraph = 0;
    int attackIndex = 0;

    bool isBoss() const {
        return kind == BarrelKind::Miniboss || kind == BarrelKind::FinalBoss;
    }
    bool enraged() const {
        return kind == BarrelKind::FinalBoss && hitPoints <= maxHitPoints / 2;
    }
    Rect bounds() const { return {position, width, height}; }
    Vec2 bossMuzzle(double side) const {
        return position + Vec2{side * width * 0.38, -height * 0.42};
    }
    int attackPattern() const {
        const int patternCount = kind == BarrelKind::Miniboss ? 2 : 3;
        return attackIndex % patternCount;
    }
    const char* name() const {
        switch (kind) {
        case BarrelKind::Miniboss: return "BARAO DO TAMBOR";
        case BarrelKind::FinalBoss: return "REI DA POLVORA";
        default: return "BARRIL";
        }
    }
    const char* attackName() const {
        switch (attackPattern()) {
        case 0: return "LEQUE";
        case 1: return "ANEL COM ABERTURA";
        default: return "RAJADA DUPLA";
        }
    }
};

enum class Owner { Player, Enemy };

struct Bullet {
    Vec2 position;
    Vec2 velocity;
    double radius = 2;
    Owner owner = Owner::Player;
    bool alive = true;
    bool bossShot = false;
};

enum class Status { Playing, Won, Lost };
enum class LossReason { None, Barrel, Shot };

struct DemoOptions {
    bool barrelMotion = true;
    bool enemyFire = true;
    bool spawning = true;
    bool hitboxes = false;
};

class Game {
public:
    static constexpr double timeStep = 1.0 / 240.0;
    static constexpr double gunLimit = 30;
    static constexpr int stageCount = 10;
    static constexpr int minibossStage = 5;
    static constexpr int minibossHitPoints = 150;
    static constexpr double stageBreak = 2.0;

    explicit Game(Config config);
    void reset();
    void advance(double seconds);
    void setKey(unsigned char key, bool pressed);
    void clearInput();
    void setFiring(bool pressed);
    void aim(double horizontalDelta);
    void fire();
    void togglePause();
    void toggleHelp();
    void toggleDemo(int option);

    const Config& config() const { return config_; }
    const Character& player() const { return player_; }
    const std::vector<Barrel>& barrels() const { return barrels_; }
    const std::vector<Bullet>& bullets() const { return bullets_; }
    const DemoOptions& demo() const { return demo_; }
    Status status() const { return status_; }
    LossReason lossReason() const { return lossReason_; }
    int destroyed() const { return destroyed_; }
    int stage() const { return stage_; }
    int stageDestroyed() const { return stageDestroyed_; }
    int stageGoal() const;
    bool isBossStage() const { return stage_ == minibossStage || stage_ == stageCount; }
    bool transitioning() const { return transitionRemaining_ > 0; }
    double transitionRemaining() const { return transitionRemaining_; }
    double barrelSpeed() const;
    double spawnInterval() const;
    const Barrel* boss() const;
    bool paused() const { return paused_; }
    bool helpVisible() const { return helpVisible_; }
    bool active() const { return running() && !transitioning(); }
    double elapsed() const { return elapsed_; }

private:
    friend struct GameTestAccess;

    enum class CollisionType { PlayerShot, EnemyShot, Contact };
    struct Collision {
        double time;
        CollisionType type;
        std::size_t bulletIndex;
        std::size_t barrelIndex;
    };

    void step(double dt);
    void movePlayer(double dt);
    void updateFiring(double dt);
    void updateSpawning(double dt);
    std::vector<Vec2> updateBarrels(double dt);
    std::vector<Collision> findCollisions(double dt, Vec2 previousPlayer,
                                        const std::vector<Vec2>& previousBarrels);
    bool applyCollisions(const std::vector<Collision>& collisions);
    void removeFinishedObjects();
    void addBullet(Vec2 origin, double angle, double speed, double radius,
                   Owner owner, bool bossShot = false);
    void startStage(int stage);
    void completeStage();
    void spawnBarrel();
    void spawnBoss();
    void moveBarrel(Barrel& barrel, double dt);
    void enemyFire(Barrel& barrel, double dt);
    void specialEnemyFire(Barrel& barrel, double dt);
    void specialEnemyAttack(Barrel& barrel);
    void bossFire(Barrel& barrel, double dt);
    void bossAttack(Barrel& barrel);
    void aimEnemy(Barrel& barrel);
    bool running() const { return status_ == Status::Playing && !paused_ && !helpVisible_; }
    void finish(Status status, LossReason reason = LossReason::None);
    Config config_;
    std::uint32_t initialSeed_;
    std::mt19937 random_;
    Character player_;
    std::vector<Barrel> barrels_;
    std::vector<Bullet> bullets_;
    std::array<bool, 256> keys_{};
    DemoOptions demo_;
    Status status_ = Status::Playing;
    LossReason lossReason_ = LossReason::None;
    int destroyed_ = 0;
    int stage_ = 1;
    int stageDestroyed_ = 0;
    bool paused_ = false;
    bool helpVisible_ = false;
    bool firing_ = false;
    double shotCountdown_ = 0;
    double accumulator_ = 0;
    double spawnClock_ = 0;
    double elapsed_ = 0;
    double transitionRemaining_ = 0;
};
}
