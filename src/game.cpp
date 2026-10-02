#include "game.hpp"

#include <cctype>
#include <stdexcept>

namespace cg {
namespace {
constexpr double clockTolerance = 1e-12;
constexpr double barrelSpeedIncrease = 0.08;
constexpr double spawnRateIncrease = 0.10;
constexpr int extraBarrelsPerStage = 2;
constexpr double minibossSpawnDelay = 5;
}

Game::Game(Config config)
    : config_(config), initialSeed_(config.fixedSeed ? config.seed : std::random_device{}()),
      random_(initialSeed_) {
    config_.validate();
    reset();
}

void Game::reset() {
    random_.seed(initialSeed_);
    demo_ = {};
    status_ = Status::Playing;
    lossReason_ = LossReason::None;
    destroyed_ = 0;
    paused_ = false;
    helpVisible_ = false;
    accumulator_ = 0;
    elapsed_ = 0;
    startStage(1);
}

void Game::startStage(int stage) {
    stage_ = stage;
    stageDestroyed_ = 0;
    transitionRemaining_ = 0;
    spawnClock_ = 0;
    player_ = {{config_.arenaWidth / 2.0,
                std::min(2.0 * config_.playerRadius, config_.arenaHeight / 2.0 - config_.playerRadius)},
               config_.playerRadius, 0, 0, 0};
    barrels_.clear();
    bullets_.clear();
    clearInput();
    if (isBossStage()) {
        spawnBoss();
    } else {
        spawnBarrel();
    }
}

void Game::completeStage() {
    barrels_.clear();
    bullets_.clear();
    clearInput();
    if (stage_ == stageCount) {
        finish(Status::Won);
    } else {
        transitionRemaining_ = stageBreak;
    }
}

const Barrel* Game::boss() const {
    for (const auto& barrel : barrels_) {
        if (barrel.isBoss()) {
            return &barrel;
        }
    }
    return nullptr;
}

int Game::stageGoal() const {
    if (isBossStage()) {
        return 1;
    }
    return config_.barrelsToWin + extraBarrelsPerStage * (stage_ - 1);
}

double Game::barrelSpeed() const {
    return config_.barrelSpeed * (1 + barrelSpeedIncrease * (stage_ - 1));
}

double Game::spawnInterval() const {
    double interval = config_.spawnInterval;
    if (stage_ == minibossStage) {
        interval *= minibossSpawnDelay;
    }
    return interval / (1 + spawnRateIncrease * (stage_ - 1));
}

void Game::setKey(unsigned char key, bool pressed) {
    keys_[static_cast<unsigned char>(std::tolower(key))] = pressed;
}

void Game::clearInput() {
    keys_.fill(false);
    firing_ = false;
    shotCountdown_ = 0;
}

void Game::setFiring(bool pressed) {
    if (!pressed) {
        firing_ = false;
        shotCountdown_ = 0;
    } else if (active() && !firing_) {
        firing_ = true;
        shotCountdown_ = 1 / config_.playerShotsPerSecond;
        fire();
    }
}

void Game::updateFiring(double dt) {
    if (!firing_) return;
    shotCountdown_ -= dt;
    while (shotCountdown_ <= clockTolerance) {
        fire();
        shotCountdown_ += 1 / config_.playerShotsPerSecond;
    }
}

void Game::aim(double horizontalDelta) {
    if (!active() || !std::isfinite(horizontalDelta)) return;
    const double newAngle = player_.gunAngle - horizontalDelta * config_.mouseSensitivity;
    player_.gunAngle = std::clamp(newAngle, -gunLimit, gunLimit);
}

void Game::addBullet(Vec2 origin, double angle, double speed, double radius, Owner owner, bool bossShot) {
    Bullet bullet;
    bullet.position = origin;
    bullet.velocity = forward(angle) * speed;
    bullet.radius = radius;
    bullet.owner = owner;
    bullet.bossShot = bossShot;
    bullets_.push_back(bullet);
}

void Game::fire() {
    if (!active()) return;
    const double radius = std::max(1.0, player_.radius * 0.12);
    addBullet(player_.muzzle(), player_.angle + player_.gunAngle, config_.playerShotSpeed, radius, Owner::Player);
}

void Game::togglePause() {
    if (status_ != Status::Playing) return;
    paused_ = !paused_;
    accumulator_ = 0;
    clearInput();
}

void Game::toggleHelp() {
    helpVisible_ = !helpVisible_;
    accumulator_ = 0;
    clearInput();
}

void Game::toggleDemo(int option) {
    switch (option) {
    case 2: demo_.barrelMotion = !demo_.barrelMotion; break;
    case 3: demo_.enemyFire = !demo_.enemyFire; break;
    case 4: demo_.spawning = !demo_.spawning; break;
    case 5: demo_.hitboxes = !demo_.hitboxes; break;
    }
}

void Game::advance(double seconds) {
    if (!std::isfinite(seconds) || seconds < 0) {
        throw std::invalid_argument("Tempo deve ser finito e nao negativo.");
    }
    if (!running()) return;
    accumulator_ += seconds;
    while (accumulator_ + clockTolerance >= timeStep && running()) {
        accumulator_ -= timeStep;
        step(timeStep);
    }
    accumulator_ = std::max(0.0, accumulator_);
}

void Game::movePlayer(double dt) {
    const int turnDirection = keys_['a'] - keys_['d'];
    const int walkDirection = keys_['w'] - keys_['s'];
    const double previousAngle = radians(player_.angle);
    const double angularSpeed = radians(turnDirection * config_.turnSpeed);
    const double speed = walkDirection * config_.playerSpeed;
    Vec2 delta;
    if (turnDirection != 0) {
        const double nextAngle = previousAngle + angularSpeed * dt;
        const double turnRadius = speed / angularSpeed;
        delta.x = turnRadius * (std::cos(nextAngle) - std::cos(previousAngle));
        delta.y = turnRadius * (std::sin(nextAngle) - std::sin(previousAngle));
    } else {
        delta = forward(player_.angle) * (speed * dt);
    }
    player_.angle = wrapAngle(player_.angle + turnDirection * config_.turnSpeed * dt);
    const Vec2 before = player_.position;
    player_.position.x = std::clamp(before.x + delta.x, player_.radius, config_.arenaWidth - player_.radius);
    player_.position.y = std::clamp(before.y + delta.y, player_.radius, config_.arenaHeight / 2.0 - player_.radius);
    const double distance = length(player_.position - before);
    if (distance > 1e-10) {
        player_.walkPhase = std::fmod(player_.walkPhase + walkDirection * distance / player_.radius * 2.5, 2 * pi);
    } else {
        player_.walkPhase = 0;
    }
}

void Game::spawnBarrel() {
    const bool enemy = std::bernoulli_distribution(0.5)(random_);
    const double margin = std::max(config_.barrelWidth / 2, enemy ? config_.enemyRadius : 0.0);
    const double x = std::uniform_real_distribution<double>(margin, config_.arenaWidth - margin)(random_);
    const double y = config_.arenaHeight - std::max(config_.barrelHeight / 2, enemy ? config_.enemyRadius : 0.0);
    Barrel barrel;
    barrel.position = {x, y};
    barrel.maxHitPoints = config_.barrelHitPoints + (stage_ - 1) / 3;
    barrel.width = config_.barrelWidth;
    barrel.height = config_.barrelHeight;
    barrel.hasEnemy = enemy;
    if (enemy && stage_ > minibossStage) {
        const int variant = std::uniform_int_distribution<int>(0, 3)(random_);
        if (variant == 2) {
            barrel.enemyKind = EnemyKind::SpreadGunner;
            barrel.maxHitPoints += 1;
        } else if (variant == 3) {
            barrel.enemyKind = EnemyKind::Cannoneer;
            barrel.maxHitPoints += 3;
        }
    }
    barrel.hitPoints = barrel.maxHitPoints;
    barrel.enemy = {barrel.position, config_.enemyRadius, 180, 0, 0};
    aimEnemy(barrel);
    barrels_.push_back(barrel);
}

void Game::spawnBoss() {
    const bool final = stage_ == stageCount;
    Barrel barrel;
    barrel.kind = final ? BarrelKind::FinalBoss : BarrelKind::Miniboss;
    barrel.width = std::min(config_.barrelWidth * (final ? 3.2 : 2.4), config_.arenaWidth * 0.70);
    barrel.height = std::min(config_.barrelHeight * (final ? 1.9 : 1.6), config_.arenaHeight * 0.18);
    barrel.position = {config_.arenaWidth / 2.0, config_.arenaHeight * 0.76};
    barrel.maxHitPoints = minibossHitPoints;
    if (final) {
        barrel.maxHitPoints = config_.barrelHitPoints * 10;
    }
    barrel.hitPoints = barrel.maxHitPoints;
    barrel.hasEnemy = true;
    const double radius = std::min({config_.enemyRadius * (final ? 1.8 : 1.4),
                                    barrel.width * 0.23, barrel.height * 0.28});
    barrel.enemy = {barrel.position, radius, 180, 0, 0};
    aimEnemy(barrel);
    barrels_.push_back(barrel);
}

void Game::moveBarrel(Barrel& barrel, double dt) {
    if (!demo_.barrelMotion) return;
    if (barrel.isBoss()) {
        const double amplitude = std::max(0.0, (config_.arenaWidth - barrel.width) / 2 - 4);
        const double speed = config_.barrelSpeed * (barrel.kind == BarrelKind::FinalBoss ? 1.6 : 1.2);
        barrel.motionClock += dt;
        const double phase = barrel.motionClock * speed / std::max(1.0, amplitude);
        barrel.position.x = config_.arenaWidth / 2.0 + amplitude * std::sin(phase);
        barrel.rollPhase = std::fmod(barrel.rollPhase + speed * dt / barrel.height, 1.0);
    } else {
        const double distance = barrelSpeed() * dt;
        barrel.position.y -= distance;
        barrel.rollPhase = std::fmod(barrel.rollPhase + distance / barrel.height, 1.0);
    }
}

void Game::enemyFire(Barrel& barrel, double dt) {
    if (barrel.isBoss()) {
        bossFire(barrel, dt);
        return;
    }
    if (barrel.enemyKind != EnemyKind::Gunner) {
        specialEnemyFire(barrel, dt);
        return;
    }
    if (!demo_.enemyFire || config_.enemyShotsPerSecond <= 0) return;
    const double multiplier = 1 + 0.06 * (stage_ - 1);
    const double interval = 1 / (config_.enemyShotsPerSecond * multiplier);
    barrel.shotClock += dt;
    while (barrel.shotClock + clockTolerance >= interval) {
        barrel.shotClock -= interval;
        const double angle = barrel.enemy.angle + barrel.enemy.gunAngle;
        const double radius = std::max(1.0, barrel.enemy.radius * 0.12);
        addBullet(barrel.enemy.muzzle(), angle, config_.enemyShotSpeed, radius, Owner::Enemy);
    }
}

void Game::specialEnemyFire(Barrel& barrel, double dt) {
    if (!demo_.enemyFire || config_.enemyShotsPerSecond <= 0) {
        barrel.telegraph = 0;
        barrel.shotClock = 0;
        return;
    }
    const bool spread = barrel.enemyKind == EnemyKind::SpreadGunner;
    const double multiplier = (spread ? 0.85 : 0.55) * (1 + 0.06 * (stage_ - 6));
    const double interval = std::max(spread ? 0.9 : 1.5, 1 / (config_.enemyShotsPerSecond * multiplier));
    const double warning = 0.5;
    barrel.shotClock += dt;
    while (barrel.shotClock + clockTolerance >= interval) {
        barrel.shotClock -= interval;
        specialEnemyAttack(barrel);
        ++barrel.attackIndex;
    }
    barrel.telegraph = std::clamp((barrel.shotClock - interval + warning) / warning, 0.0, 1.0);
}

void Game::specialEnemyAttack(Barrel& barrel) {
    if (barrel.enemyKind == EnemyKind::SpreadGunner) {
        const Vec2 origin = barrel.enemy.muzzle();
        const double aimed = barrel.enemy.angle + barrel.enemy.gunAngle;
        const double radius = std::clamp(barrel.enemy.radius * 0.15, 2.0, 5.0);
        for (int shot = -1; shot <= 1; ++shot) {
            addBullet(origin, aimed + shot * 14, config_.enemyShotSpeed * 1.1, radius, Owner::Enemy);
        }
        return;
    }

    const int shotCount = 8;
    const double radius = std::clamp(barrel.enemy.radius * 0.24, 2.0, 5.0);
    for (int shot = 0; shot < shotCount; ++shot) {
        const double angle = shot * 360.0 / shotCount + barrel.attackIndex * 22.5;
        const Vec2 direction = forward(angle);
        const Vec2 origin = barrel.position + Vec2{direction.x * barrel.width * 0.40,
                                                  direction.y * barrel.height * 0.40};
        addBullet(origin, angle, config_.enemyShotSpeed * 0.8, radius, Owner::Enemy);
    }
}

void Game::bossFire(Barrel& barrel, double dt) {
    if (!demo_.enemyFire || config_.enemyShotsPerSecond <= 0) {
        barrel.telegraph = 0;
        barrel.shotClock = 0;
        return;
    }
    const bool miniboss = barrel.kind == BarrelKind::Miniboss;
    double multiplier = 0.85;
    if (miniboss) {
        multiplier = 1.30;
    } else if (barrel.enraged()) {
        multiplier = 1.1;
    }
    const double interval = std::max(miniboss ? 0.6 : 1.2, 1 / (config_.enemyShotsPerSecond * multiplier));
    const double warning = std::min(0.7, interval * 0.45);
    barrel.shotClock += dt;
    while (barrel.shotClock + clockTolerance >= interval) {
        barrel.shotClock -= interval;
        bossAttack(barrel);
        ++barrel.attackIndex;
    }
    barrel.telegraph = std::clamp((barrel.shotClock - interval + warning) / warning, 0.0, 1.0);
}

void Game::bossAttack(Barrel& barrel) {
    const bool final = barrel.kind == BarrelKind::FinalBoss;
    const bool enraged = barrel.enraged();
    double speedMultiplier = 0.85;
    if (!final) {
        speedMultiplier = 1.15;
    } else if (enraged) {
        speedMultiplier = 1.1;
    }
    const double speed = config_.enemyShotSpeed * speedMultiplier;
    const double radius = std::clamp(config_.playerRadius * 0.22, 2.0, 5.0);
    if (barrel.attackPattern() == 0) {
        const Vec2 origin = barrel.bossMuzzle(0);
        int count = 5;
        double spread = 26;
        if (!final) {
            count = 3;
            spread = 18;
        } else if (enraged) {
            count = 7;
            spread = 32;
        }
        const double aimed = angleTo(origin, player_.position);
        for (int shot = 0; shot < count; ++shot) {
            const double angle = aimed - spread + 2 * spread * shot / (count - 1);
            addBullet(origin, angle, speed, radius, Owner::Enemy, true);
        }
    } else if (barrel.attackPattern() == 1) {
        int count = 18;
        if (!final) {
            count = 12;
        } else if (enraged) {
            count = 22;
        }
        const double gap = 180 + 45 * std::sin(barrel.attackIndex * 1.1);
        for (int i = 0; i < count; ++i) {
            const double angle = 360.0 * i / count + barrel.attackIndex * 11;
            if (std::abs(wrapAngle(angle - gap)) < 30) continue;
            const Vec2 origin = barrel.position + forward(angle) * (std::min(barrel.width, barrel.height) * 0.4);
            addBullet(origin, angle, speed * 0.82, radius, Owner::Enemy, true);
        }
    } else {
        const int count = enraged ? 5 : 4;
        for (double side : {-1.0, 1.0}) {
            const Vec2 origin = barrel.bossMuzzle(side);
            const double aimed = angleTo(origin, player_.position);
            for (int shot = 0; shot < count; ++shot) {
                const double angle = aimed + side * (shot * 11 - 16);
                addBullet(origin, angle, speed * (0.85 + 0.05 * shot), radius, Owner::Enemy, true);
            }
        }
    }
}

void Game::aimEnemy(Barrel& barrel) {
    const double desired = angleTo(barrel.enemy.gunPivot(), player_.position);
    barrel.enemy.gunAngle = std::clamp(wrapAngle(desired - barrel.enemy.angle), -gunLimit, gunLimit);
}

void Game::finish(Status status, LossReason reason) {
    status_ = status;
    lossReason_ = reason;
    clearInput();
}

void Game::updateSpawning(double dt) {
    if (!demo_.spawning) return;
    const double interval = spawnInterval();
    spawnClock_ += dt;
    while (spawnClock_ + clockTolerance >= interval) {
        spawnClock_ -= interval;
        spawnBarrel();
    }
}

std::vector<Vec2> Game::updateBarrels(double dt) {
    std::vector<Vec2> previousBarrels;
    previousBarrels.reserve(barrels_.size());
    for (auto& barrel : barrels_) {
        previousBarrels.push_back(barrel.position);
        moveBarrel(barrel, dt);
        if (barrel.hasEnemy) {
            barrel.enemy.position = barrel.position;
            barrel.enemy.walkPhase = std::fmod(barrel.enemy.walkPhase + 8 * dt, 2 * pi);
            aimEnemy(barrel);
            enemyFire(barrel, dt);
        }
    }
    return previousBarrels;
}

std::vector<Game::Collision> Game::findCollisions(double dt, Vec2 previousPlayer,
                                                 const std::vector<Vec2>& previousBarrels) {
    std::vector<Collision> collisions;
    for (std::size_t i = 0; i < bullets_.size(); ++i) {
        auto& bullet = bullets_[i];
        const Vec2 before = bullet.position;
        bullet.position = before + bullet.velocity * dt;
        if (bullet.owner == Owner::Player) {
            for (std::size_t j = 0; j < barrels_.size(); ++j) {
                const auto& barrel = barrels_[j];
                const Vec2 relativeStart = before - previousBarrels[j];
                const Vec2 relativeEnd = bullet.position - barrel.position;
                const Rect bounds{{0, 0}, barrel.width, barrel.height};
                const double hitTime = sweepCircleRect(relativeStart, relativeEnd, bullet.radius, bounds);
                if (std::isfinite(hitTime)) {
                    collisions.push_back({hitTime, CollisionType::PlayerShot, i, j});
                }
            }
        } else {
            const double hitTime = segmentCircle(before - previousPlayer, bullet.position - player_.position,
                                                {0, 0}, bullet.radius + player_.radius);
            if (std::isfinite(hitTime)) {
                collisions.push_back({hitTime, CollisionType::EnemyShot, i, 0});
            }
        }
    }

    for (std::size_t j = 0; j < barrels_.size(); ++j) {
        const auto& barrel = barrels_[j];
        const Rect bounds{{0, 0}, barrel.width, barrel.height};
        const double hitTime = sweepCircleRect(previousPlayer - previousBarrels[j],
                                              player_.position - barrel.position, player_.radius, bounds);
        if (std::isfinite(hitTime)) {
            collisions.push_back({hitTime, CollisionType::Contact, 0, j});
        }
    }

    std::stable_sort(collisions.begin(), collisions.end(), [](const Collision& first, const Collision& second) {
        if (first.time != second.time) {
            return first.time < second.time;
        }
        return first.type < second.type;
    });
    return collisions;
}

bool Game::applyCollisions(const std::vector<Collision>& collisions) {
    for (const auto& collision : collisions) {
        if (status_ != Status::Playing) break;

        if (collision.type == CollisionType::Contact) {
            if (barrels_[collision.barrelIndex].hitPoints > 0) {
                finish(Status::Lost, LossReason::Barrel);
            }
            continue;
        }

        auto& bullet = bullets_[collision.bulletIndex];
        if (!bullet.alive) continue;
        if (collision.type == CollisionType::EnemyShot) {
            bullet.alive = false;
            finish(Status::Lost, LossReason::Shot);
            continue;
        }

        auto& barrel = barrels_[collision.barrelIndex];
        if (barrel.hitPoints <= 0) continue;
        bullet.alive = false;
        --barrel.hitPoints;
        if (barrel.hitPoints > 0) continue;

        ++destroyed_;
        if (barrel.isBoss()) {
            ++stageDestroyed_;
            completeStage();
            return true;
        }
        if (!isBossStage()) {
            ++stageDestroyed_;
            if (stageDestroyed_ >= stageGoal()) {
                completeStage();
                return true;
            }
        }
    }
    return false;
}

void Game::removeFinishedObjects() {
    barrels_.erase(std::remove_if(barrels_.begin(), barrels_.end(), [](const Barrel& barrel) {
        const double exitHeight = barrel.hasEnemy ? barrel.enemy.radius : -barrel.height / 2;
        const bool escaped = !barrel.isBoss() && barrel.position.y < exitHeight;
        return barrel.hitPoints <= 0 || escaped;
    }), barrels_.end());

    bullets_.erase(std::remove_if(bullets_.begin(), bullets_.end(), [this](const Bullet& bullet) {
        const bool outsideX = bullet.position.x < -bullet.radius ||
                              bullet.position.x > config_.arenaWidth + bullet.radius;
        const bool outsideY = bullet.position.y < -bullet.radius ||
                              bullet.position.y > config_.arenaHeight + bullet.radius;
        return !bullet.alive || outsideX || outsideY;
    }), bullets_.end());
}

void Game::step(double dt) {
    elapsed_ += dt;
    if (transitioning()) {
        transitionRemaining_ = std::max(0.0, transitionRemaining_ - dt);
        if (transitionRemaining_ < 1e-10) {
            startStage(stage_ + 1);
        }
        return;
    }

    updateSpawning(dt);
    const Vec2 previousPlayer = player_.position;
    movePlayer(dt);
    updateFiring(dt);

    const auto previousBarrels = updateBarrels(dt);
    const auto collisions = findCollisions(dt, previousPlayer, previousBarrels);
    if (applyCollisions(collisions)) return;
    removeFinishedObjects();
}
}
