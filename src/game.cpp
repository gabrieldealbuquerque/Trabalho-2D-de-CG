#include "game.hpp"

#include <cctype>
#include <stdexcept>

namespace cg {
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
    paused_ = helpVisible_ = false;
    accumulator_ = elapsed_ = 0;
    startStage(1);
}

void Game::startStage(int stage) {
    stage_ = stage;
    stageDestroyed_ = 0;
    transitionRemaining_ = spawnClock_ = 0;
    player_ = {{config_.arenaWidth / 2.0,
                std::min(2.0 * config_.playerRadius, config_.arenaHeight / 2.0 - config_.playerRadius)},
               config_.playerRadius, 0, 0, 0};
    barrels_.clear();
    bullets_.clear();
    clearInput();
    if (isBossStage()) spawnBoss();
    else spawnBarrel();
}

void Game::completeStage() {
    barrels_.clear();
    bullets_.clear();
    clearInput();
    if (stage_ == stageCount) finish(Status::Won);
    else transitionRemaining_ = stageBreak;
}

const Barrel* Game::boss() const {
    const auto it = std::find_if(barrels_.begin(), barrels_.end(), [](const Barrel& b) { return b.isBoss(); });
    return it == barrels_.end() ? nullptr : &*it;
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
    while (shotCountdown_ <= 1e-12) {
        fire();
        shotCountdown_ += 1 / config_.playerShotsPerSecond;
    }
}
void Game::aim(double horizontalDelta) {
    if (active() && std::isfinite(horizontalDelta))
        player_.gunAngle = std::clamp(player_.gunAngle - horizontalDelta * config_.mouseSensitivity,
                                     -gunLimit, gunLimit);
}
void Game::fire() {
    if (!active()) return;
    bullets_.push_back({player_.muzzle(), forward(player_.angle + player_.gunAngle) * config_.playerShotSpeed,
                        std::max(1.0, player_.radius * 0.12), Owner::Player, true});
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
    if (option == 2) demo_.barrelMotion = !demo_.barrelMotion;
    if (option == 3) demo_.enemyFire = !demo_.enemyFire;
    if (option == 4) demo_.spawning = !demo_.spawning;
    if (option == 5) demo_.hitboxes = !demo_.hitboxes;
}

void Game::advance(double seconds) {
    if (!std::isfinite(seconds) || seconds < 0)
        throw std::invalid_argument("Tempo deve ser finito e nao negativo.");
    if (!running()) return;
    accumulator_ += seconds;
    while (accumulator_ + 1e-12 >= timeStep && running()) {
        accumulator_ -= timeStep;
        step(timeStep);
    }
    accumulator_ = std::max(0.0, accumulator_);
}

void Game::movePlayer(double dt) {
    const int turn = static_cast<int>(keys_['a']) - static_cast<int>(keys_['d']);
    const int motion = static_cast<int>(keys_['w']) - static_cast<int>(keys_['s']);
    const double oldAngle = radians(player_.angle);
    const double omega = radians(turn * config_.turnSpeed);
    const double velocity = motion * config_.playerSpeed;
    Vec2 delta;
    if (turn != 0) {
        delta = {velocity / omega * (std::cos(oldAngle + omega * dt) - std::cos(oldAngle)),
                 velocity / omega * (std::sin(oldAngle + omega * dt) - std::sin(oldAngle))};
    } else {
        delta = forward(player_.angle) * (velocity * dt);
    }
    player_.angle = wrapAngle(player_.angle + turn * config_.turnSpeed * dt);
    const Vec2 before = player_.position;
    player_.position.x = std::clamp(before.x + delta.x, player_.radius, config_.arenaWidth - player_.radius);
    player_.position.y = std::clamp(before.y + delta.y, player_.radius, config_.arenaHeight / 2.0 - player_.radius);
    const double distance = length(player_.position - before);
    if (distance > 1e-10)
        player_.walkPhase = std::fmod(player_.walkPhase + motion * distance / player_.radius * 2.5, 2 * pi);
    else
        player_.walkPhase = 0;
}

void Game::spawnBarrel() {
    const bool enemy = std::bernoulli_distribution(0.5)(random_);
    const double margin = std::max(config_.barrelWidth / 2, enemy ? config_.enemyRadius : 0.0);
    const double x = std::uniform_real_distribution<double>(margin, config_.arenaWidth - margin)(random_);
    const double y = config_.arenaHeight - std::max(config_.barrelHeight / 2, enemy ? config_.enemyRadius : 0.0);
    Barrel barrel;
    barrel.position = {x, y};
    barrel.hitPoints = barrel.maxHitPoints = config_.barrelHitPoints + (stage_ - 1) / 3;
    barrel.width = config_.barrelWidth;
    barrel.height = config_.barrelHeight;
    barrel.hasEnemy = enemy;
    if (enemy && stage_ > 5) {
        const int variant = std::uniform_int_distribution<int>(0, 3)(random_);
        if (variant == 2) barrel.enemyKind = EnemyKind::SpreadGunner;
        if (variant == 3) barrel.enemyKind = EnemyKind::Cannoneer;
        const int armor = barrel.enemyKind == EnemyKind::Cannoneer ? 3 :
                          barrel.enemyKind == EnemyKind::SpreadGunner ? 1 : 0;
        barrel.maxHitPoints += armor;
        barrel.hitPoints = barrel.maxHitPoints;
    }
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
    barrel.hitPoints = barrel.maxHitPoints = final ? config_.barrelHitPoints * 10 : 150;
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
        barrel.position.x = config_.arenaWidth / 2.0 + amplitude * std::sin(barrel.motionClock * speed / std::max(1.0, amplitude));
        barrel.rollPhase = std::fmod(barrel.rollPhase + speed * dt / barrel.height, 1.0);
    } else {
        const double distance = barrelSpeed() * dt;
        barrel.position.y -= distance;
        barrel.rollPhase = std::fmod(barrel.rollPhase + distance / barrel.height, 1.0);
    }
}

void Game::enemyFire(Barrel& barrel, double dt) {
    if (barrel.isBoss()) { bossFire(barrel, dt); return; }
    if (barrel.enemyKind != EnemyKind::Gunner) { specialEnemyFire(barrel, dt); return; }
    if (!demo_.enemyFire || config_.enemyShotsPerSecond <= 0) return;
    const double multiplier = 1 + 0.06 * (stage_ - 1);
    const double interval = 1 / (config_.enemyShotsPerSecond * multiplier);
    barrel.shotClock += dt;
    while (barrel.shotClock + 1e-12 >= interval) {
        barrel.shotClock -= interval;
        bullets_.push_back({barrel.enemy.muzzle(), forward(barrel.enemy.angle + barrel.enemy.gunAngle) * config_.enemyShotSpeed,
                           std::max(1.0, barrel.enemy.radius * 0.12), Owner::Enemy, true});
    }
}

void Game::specialEnemyFire(Barrel& barrel, double dt) {
    if (!demo_.enemyFire || config_.enemyShotsPerSecond <= 0) {
        barrel.telegraph = barrel.shotClock = 0;
        return;
    }
    const bool spread = barrel.enemyKind == EnemyKind::SpreadGunner;
    const double multiplier = (spread ? 0.85 : 0.55) * (1 + 0.06 * (stage_ - 6));
    const double interval = std::max(spread ? 0.9 : 1.5, 1 / (config_.enemyShotsPerSecond * multiplier));
    const double warning = 0.5;
    barrel.shotClock += dt;
    while (barrel.shotClock + 1e-12 >= interval) {
        barrel.shotClock -= interval;
        const double aimed = barrel.enemy.angle + barrel.enemy.gunAngle;
        const int count = spread ? 3 : 8;
        for (int i = 0; i < count; ++i) {
            const double angle = spread ? aimed + (i - 1) * 14 : i * 360.0 / count + barrel.attackIndex * 22.5;
            const Vec2 direction = forward(angle);
            const Vec2 origin = spread ? barrel.enemy.muzzle() :
                barrel.position + Vec2{direction.x * barrel.width * 0.40, direction.y * barrel.height * 0.40};
            bullets_.push_back({origin, direction * (config_.enemyShotSpeed * (spread ? 1.1 : 0.8)),
                               std::clamp(barrel.enemy.radius * (spread ? 0.15 : 0.24), 2.0, 5.0), Owner::Enemy, true});
        }
        ++barrel.attackIndex;
    }
    barrel.telegraph = std::clamp((barrel.shotClock - interval + warning) / warning, 0.0, 1.0);
}

void Game::bossFire(Barrel& barrel, double dt) {
    if (!demo_.enemyFire || config_.enemyShotsPerSecond <= 0) {
        barrel.telegraph = barrel.shotClock = 0;
        return;
    }
    const bool miniboss = barrel.kind == BarrelKind::Miniboss;
    const double multiplier = miniboss ? 1.30 : barrel.enraged() ? 1.1 : 0.85;
    const double interval = std::max(miniboss ? 0.6 : 1.2, 1 / (config_.enemyShotsPerSecond * multiplier));
    const double warning = std::min(0.7, interval * 0.45);
    barrel.shotClock += dt;
    while (barrel.shotClock + 1e-12 >= interval) {
        barrel.shotClock -= interval;
        bossAttack(barrel);
        ++barrel.attackIndex;
    }
    barrel.telegraph = std::clamp((barrel.shotClock - interval + warning) / warning, 0.0, 1.0);
}

void Game::bossAttack(Barrel& barrel) {
    const bool final = barrel.kind == BarrelKind::FinalBoss;
    const bool enraged = barrel.enraged();
    const double speed = config_.enemyShotSpeed * (!final ? 1.15 : enraged ? 1.1 : 0.85);
    const double radius = std::clamp(config_.playerRadius * 0.22, 2.0, 5.0);
    const auto emit = [&](Vec2 origin, double angle, double velocity) {
        bullets_.push_back({origin, forward(angle) * velocity, radius, Owner::Enemy, true, true});
    };
    const auto aimed = [&](Vec2 origin) {
        const Vec2 delta = player_.position - origin;
        return std::atan2(-delta.x, delta.y) * 180 / pi;
    };
    if (barrel.attackPattern() == 0) {
        const Vec2 origin = barrel.bossMuzzle(0);
        const int count = !final ? 3 : enraged ? 7 : 5;
        const double spread = !final ? 18 : enraged ? 32 : 26;
        for (int i = 0; i < count; ++i)
            emit(origin, aimed(origin) - spread + 2 * spread * i / (count - 1), speed);
    } else if (barrel.attackPattern() == 1) {
        const int count = !final ? 12 : enraged ? 22 : 18;
        const double gap = 180 + 45 * std::sin(barrel.attackIndex * 1.1);
        for (int i = 0; i < count; ++i) {
            const double angle = 360.0 * i / count + barrel.attackIndex * 11;
            if (std::abs(wrapAngle(angle - gap)) < 30) continue;
            const Vec2 origin = barrel.position + forward(angle) * (std::min(barrel.width, barrel.height) * 0.4);
            emit(origin, angle, speed * 0.82);
        }
    } else {
        const int count = enraged ? 5 : 4;
        for (double side : {-1.0, 1.0}) {
            const Vec2 origin = barrel.bossMuzzle(side);
            for (int i = 0; i < count; ++i)
                emit(origin, aimed(origin) + side * (i * 11 - 16), speed * (0.85 + 0.05 * i));
        }
    }
}

void Game::aimEnemy(Barrel& barrel) {
    const Vec2 offset = player_.position - barrel.enemy.gunPivot();
    const double desired = std::atan2(-offset.x, offset.y) * 180 / pi;
    barrel.enemy.gunAngle = std::clamp(wrapAngle(desired - barrel.enemy.angle), -gunLimit, gunLimit);
}

void Game::finish(Status status, LossReason reason) {
    status_ = status;
    lossReason_ = reason;
    clearInput();
}

void Game::step(double dt) {
    elapsed_ += dt;
    if (transitioning()) {
        transitionRemaining_ = std::max(0.0, transitionRemaining_ - dt);
        if (transitionRemaining_ < 1e-10) startStage(stage_ + 1);
        return;
    }
    if (demo_.spawning) {
        spawnClock_ += dt;
        while (spawnClock_ + 1e-12 >= spawnInterval()) {
            spawnClock_ -= spawnInterval();
            spawnBarrel();
        }
    }
    const Vec2 previousPlayer = player_.position;
    movePlayer(dt);
    updateFiring(dt);
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

    enum class Impact { BarrelShot, EnemyShot, Body };
    struct Event { double time; Impact type; std::size_t bullet, barrel; };
    std::vector<Event> events;
    for (std::size_t i = 0; i < bullets_.size(); ++i) {
        auto& bullet = bullets_[i];
        const Vec2 before = bullet.position;
        bullet.position = before + bullet.velocity * dt;
        if (bullet.owner == Owner::Player) {
            for (std::size_t j = 0; j < barrels_.size(); ++j) {
                const double t = sweepCircleRect(before - previousBarrels[j], bullet.position - barrels_[j].position,
                    bullet.radius, {{0, 0}, barrels_[j].width, barrels_[j].height});
                if (std::isfinite(t)) events.push_back({t, Impact::BarrelShot, i, j});
            }
        } else {
            const double t = segmentCircle(before - previousPlayer, bullet.position - player_.position,
                                          {0, 0}, bullet.radius + player_.radius);
            if (std::isfinite(t)) events.push_back({t, Impact::EnemyShot, i, 0});
        }
    }
    for (std::size_t j = 0; j < barrels_.size(); ++j) {
        const double t = sweepCircleRect(previousPlayer - previousBarrels[j], player_.position - barrels_[j].position,
            player_.radius, {{0, 0}, barrels_[j].width, barrels_[j].height});
        if (std::isfinite(t)) events.push_back({t, Impact::Body, 0, j});
    }
    std::stable_sort(events.begin(), events.end(), [](const Event& a, const Event& b) {
        if (a.time != b.time) return a.time < b.time;
        return a.type < b.type;
    });
    for (const auto& event : events) {
        if (status_ != Status::Playing) break;
        if (event.type == Impact::BarrelShot) {
            auto& barrel = barrels_[event.barrel];
            auto& bullet = bullets_[event.bullet];
            if (!bullet.alive || barrel.hitPoints <= 0) continue;
            bullet.alive = false;
            if (--barrel.hitPoints == 0) {
                ++destroyed_;
                if (barrel.isBoss()) {
                    ++stageDestroyed_;
                    completeStage();
                    return;
                }
                if (barrel.kind == BarrelKind::Normal && !isBossStage()) ++stageDestroyed_;
                if (!isBossStage() && stageDestroyed_ >= stageGoal()) {
                    completeStage();
                    return;
                }
            }
        } else if (event.type == Impact::EnemyShot) {
            if (!bullets_[event.bullet].alive) continue;
            bullets_[event.bullet].alive = false;
            finish(Status::Lost, LossReason::Shot);
        } else if (barrels_[event.barrel].hitPoints > 0) {
            finish(Status::Lost, LossReason::Barrel);
        }
    }
    barrels_.erase(std::remove_if(barrels_.begin(), barrels_.end(), [](const Barrel& b) {
        const double bottom = b.hasEnemy ? b.enemy.radius : -b.height / 2;
        return b.hitPoints <= 0 || (!b.isBoss() && b.position.y < bottom);
    }), barrels_.end());
    bullets_.erase(std::remove_if(bullets_.begin(), bullets_.end(), [this](const Bullet& b) {
        return !b.alive || b.position.x < -b.radius || b.position.x > config_.arenaWidth + b.radius ||
               b.position.y < -b.radius || b.position.y > config_.arenaHeight + b.radius;
    }), bullets_.end());
}
}
