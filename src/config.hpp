#pragma once

#include <cstdint>
#include <filesystem>

namespace cg {
struct Config {
    int arenaWidth = 600, arenaHeight = 800;
    double playerRadius = 18, playerSpeed = 180, turnSpeed = 150;
    double playerShotSpeed = 480, playerShotsPerSecond = 6, enemyRadius = 15;
    double enemyShotsPerSecond = 0.7, enemyShotSpeed = 190;
    double barrelWidth = 76, barrelHeight = 64, barrelSpeed = 65;
    int barrelHitPoints = 3, barrelsToWin = 10;
    double spawnInterval = 2.1, mouseSensitivity = 0.25;
    std::uint32_t seed = 0;
    bool fixedSeed = false;

    static Config load(const std::filesystem::path& path);
    void validate() const;
};
std::filesystem::path executableDirectory(const char* argv0);
}
