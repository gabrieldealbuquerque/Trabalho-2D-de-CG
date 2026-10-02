#include "config.hpp"
#include "tinyxml2.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <string>
#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace cg {
namespace {
const tinyxml2::XMLElement& requiredElement(const tinyxml2::XMLElement& root, const char* name) {
    const auto* value = root.FirstChildElement(name);
    if (!value) {
        throw std::runtime_error(std::string("XML: elemento obrigatorio ausente: ") + name);
    }
    if (value->NextSiblingElement(name)) {
        throw std::runtime_error(std::string("XML: elemento duplicado: ") + name);
    }
    return *value;
}

double readNumber(const tinyxml2::XMLElement& e, const char* name) {
    const char* raw = e.Attribute(name);
    if (raw) {
        char* end = nullptr;
        const double value = std::strtod(raw, &end);
        if (end != raw && *end == '\0' && std::isfinite(value)) {
            return value;
        }
    }
    throw std::runtime_error(std::string("XML: numero invalido ou ausente em ") + e.Name() + "." + name);
}

double readOptionalNumber(const tinyxml2::XMLElement& e, const char* name, double defaultValue) {
    if (!e.Attribute(name)) {
        return defaultValue;
    }
    return readNumber(e, name);
}

int readInteger(const tinyxml2::XMLElement& e, const char* name) {
    const double value = readNumber(e, name);
    if (std::floor(value) != value || value < 1 || value > 1000000) {
        throw std::runtime_error(std::string("XML: inteiro positivo esperado em ") + e.Name() + "." + name);
    }
    return static_cast<int>(value);
}

void range(double value, double minimum, double maximum, const char* name) {
    if (!std::isfinite(value) || value < minimum || value > maximum) {
        throw std::runtime_error(std::string("Configuracao fora do intervalo permitido: ") + name);
    }
}
}

Config Config::load(const std::filesystem::path& path) {
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(path.string().c_str()) != tinyxml2::XML_SUCCESS) {
        throw std::runtime_error("Nao foi possivel ler " + path.string() + ": " + doc.ErrorStr());
    }
    const auto* root = doc.RootElement();
    if (!root || std::string(root->Name()) != "jogo" || root->NextSiblingElement()) {
        throw std::runtime_error("XML: deve haver uma unica raiz <jogo>.");
    }

    const auto& arena = requiredElement(*root, "arena");
    const auto& player = requiredElement(*root, "jogador");
    const auto& enemy = requiredElement(*root, "inimigo");
    const auto& barrel = requiredElement(*root, "barril");
    Config config;

    config.arenaWidth = readInteger(arena, "largura");
    config.arenaHeight = readInteger(arena, "altura");
    config.playerRadius = readNumber(player, "raioCabeca");
    config.playerSpeed = readNumber(player, "velocidade");
    config.turnSpeed = readOptionalNumber(player, "velocidadeGiro", config.turnSpeed);
    config.enemyRadius = readNumber(enemy, "raioCabeca");
    config.enemyShotsPerSecond = readNumber(enemy, "tirosPorSegundo");
    config.enemyShotSpeed = readNumber(enemy, "velocidadeTiro");
    config.playerShotSpeed = readOptionalNumber(player, "velocidadeTiro", config.enemyShotSpeed);
    config.playerShotsPerSecond = readOptionalNumber(player, "tirosPorSegundo", config.playerShotsPerSecond);
    config.barrelWidth = readNumber(barrel, "largura");
    config.barrelHeight = readNumber(barrel, "altura");
    config.barrelHitPoints = readInteger(barrel, "numeroTiros");
    config.barrelsToWin = readInteger(barrel, "nParaGanhar");
    config.barrelSpeed = readNumber(barrel, "velocidade");

    if (const auto* extra = root->FirstChildElement("outros")) {
        if (extra->NextSiblingElement("outros")) {
            throw std::runtime_error("XML: elemento duplicado: outros");
        }
        config.spawnInterval = readOptionalNumber(*extra, "intervaloBarris", config.spawnInterval);
        config.mouseSensitivity = readOptionalNumber(*extra, "sensibilidadeMouse", config.mouseSensitivity);
        if (extra->Attribute("semente")) {
            const double seed = readNumber(*extra, "semente");
            range(seed, 0, 4294967295.0, "outros.semente");
            if (std::floor(seed) != seed) {
                throw std::runtime_error("XML: semente deve ser inteira.");
            }
            config.seed = static_cast<std::uint32_t>(seed);
            config.fixedSeed = true;
        }
    }
    config.validate();
    return config;
}

void Config::validate() const {
    range(arenaWidth, 80, 8192, "arena.largura (80..8192)");
    range(arenaHeight, 120, 8192, "arena.altura (120..8192)");
    range(playerRadius, 1, std::min(arenaWidth / 2.0, arenaHeight / 4.0), "jogador.raioCabeca");
    range(enemyRadius, 1, std::min(arenaWidth / 2.0, arenaHeight / 2.0), "inimigo.raioCabeca");
    range(playerSpeed, 0, 10000, "jogador.velocidade");
    range(turnSpeed, 0.01, 3600, "jogador.velocidadeGiro");
    range(playerShotSpeed, 0.01, 100000, "jogador.velocidadeTiro");
    range(playerShotsPerSecond, 0.1, 60, "jogador.tirosPorSegundo (0.1..60)");
    range(enemyShotSpeed, 0.01, 100000, "inimigo.velocidadeTiro");
    range(enemyShotsPerSecond, 0, 100, "inimigo.tirosPorSegundo");
    range(barrelWidth, 1, arenaWidth, "barril.largura");
    range(barrelHeight, 1, arenaHeight / 2.0, "barril.altura");
    range(barrelSpeed, 0, 10000, "barril.velocidade");
    range(barrelHitPoints, 1, 1000000, "barril.numeroTiros");
    range(barrelsToWin, 1, 1000000, "barril.nParaGanhar");
    range(spawnInterval, 0.05, 3600, "outros.intervaloBarris");
    range(mouseSensitivity, 0.001, 30, "outros.sensibilidadeMouse");
}

std::filesystem::path executableDirectory(const char* argv0) {
#if defined(__linux__)
    std::error_code error;
    const auto executable = std::filesystem::read_symlink("/proc/self/exe", error);
    if (!error) return executable.parent_path();
#elif defined(_WIN32)
    wchar_t buffer[32768];
    const DWORD count = GetModuleFileNameW(nullptr, buffer, 32768);
    if (count > 0 && count < 32768) return std::filesystem::path(buffer).parent_path();
#elif defined(__APPLE__)
    char buffer[4096];
    std::uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) == 0)
        return std::filesystem::weakly_canonical(buffer).parent_path();
#endif
    auto path = std::filesystem::absolute(argv0);
    if (path.has_parent_path() && std::filesystem::exists(path))
        return std::filesystem::weakly_canonical(path).parent_path();
    throw std::runtime_error("Nao foi possivel localizar o diretorio do executavel.");
}
}
