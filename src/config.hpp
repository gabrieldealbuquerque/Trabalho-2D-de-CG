#pragma once

#include <cstdint>
#include <filesystem>

namespace cg {
struct Config {
    int arenaWidth = 600;
    int arenaHeight = 800;

    double playerRadius = 18;
    double playerSpeed = 180;
    double turnSpeed = 150;
    double playerShotSpeed = 480;
    double playerShotsPerSecond = 6;

    double enemyRadius = 15;
    double enemyShotsPerSecond = 0.7;
    double enemyShotSpeed = 190;

    double barrelWidth = 76;
    double barrelHeight = 64;
    double barrelSpeed = 65;
    int barrelHitPoints = 3;
    int barrelsToWin = 10;

    double spawnInterval = 2.1;
    double mouseSensitivity = 0.25;
    std::uint32_t seed = 0;
    bool fixedSeed = false;

    static Config load(const std::filesystem::path& path);
    void validate() const;
};
std::filesystem::path executableDirectory(const char* argv0);
}
