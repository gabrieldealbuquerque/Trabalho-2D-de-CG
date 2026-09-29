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
const tinyxml2::XMLElement& element(const tinyxml2::XMLElement& root, const char* name) {
    const auto* value = root.FirstChildElement(name);
    if (!value) throw std::runtime_error(std::string("XML: elemento obrigatorio ausente: ") + name);
    if (value->NextSiblingElement(name))
        throw std::runtime_error(std::string("XML: elemento duplicado: ") + name);
    return *value;
}
double number(const tinyxml2::XMLElement& e, const char* name, double fallback, bool required = true) {
    const char* raw = e.Attribute(name);
    if (!raw && !required) return fallback;
    if (raw) {
        char* end = nullptr;
        const double value = std::strtod(raw, &end);
        if (end != raw && *end == '\0' && std::isfinite(value)) return value;
    }
    throw std::runtime_error(std::string("XML: numero invalido ou ausente em ") + e.Name() + "." + name);
}
int integer(const tinyxml2::XMLElement& e, const char* name, int fallback) {
    const double value = number(e, name, fallback);
    if (std::floor(value) != value || value < 1 || value > 1000000)
        throw std::runtime_error(std::string("XML: inteiro positivo esperado em ") + e.Name() + "." + name);
    return static_cast<int>(value);
}
void range(double value, double minimum, double maximum, const char* name) {
    if (!std::isfinite(value) || value < minimum || value > maximum)
        throw std::runtime_error(std::string("Configuracao fora do intervalo permitido: ") + name);
}
}

Config Config::load(const std::filesystem::path& path) {
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(path.string().c_str()) != tinyxml2::XML_SUCCESS)
        throw std::runtime_error("Nao foi possivel ler " + path.string() + ": " + doc.ErrorStr());
    const auto* root = doc.RootElement();
    if (!root || std::string(root->Name()) != "jogo" || root->NextSiblingElement())
        throw std::runtime_error("XML: deve haver uma unica raiz <jogo>.");
    const auto& arena = element(*root, "arena");
    const auto& player = element(*root, "jogador");
    const auto& enemy = element(*root, "inimigo");
    const auto& barrel = element(*root, "barril");
    Config c;
    c.arenaWidth = integer(arena, "largura", c.arenaWidth);
    c.arenaHeight = integer(arena, "altura", c.arenaHeight);
    c.playerRadius = number(player, "raioCabeca", c.playerRadius);
    c.playerSpeed = number(player, "velocidade", c.playerSpeed);
    c.turnSpeed = number(player, "velocidadeGiro", c.turnSpeed, false);
    c.enemyRadius = number(enemy, "raioCabeca", c.enemyRadius);
    c.enemyShotsPerSecond = number(enemy, "tirosPorSegundo", c.enemyShotsPerSecond);
    c.enemyShotSpeed = number(enemy, "velocidadeTiro", c.enemyShotSpeed);
    c.playerShotSpeed = number(player, "velocidadeTiro", c.enemyShotSpeed, false);
    c.playerShotsPerSecond = number(player, "tirosPorSegundo", c.playerShotsPerSecond, false);
    c.barrelWidth = number(barrel, "largura", c.barrelWidth);
    c.barrelHeight = number(barrel, "altura", c.barrelHeight);
    c.barrelHitPoints = integer(barrel, "numeroTiros", c.barrelHitPoints);
    c.barrelsToWin = integer(barrel, "nParaGanhar", c.barrelsToWin);
    c.barrelSpeed = number(barrel, "velocidade", c.barrelSpeed);
    if (const auto* extra = root->FirstChildElement("outros")) {
        c.spawnInterval = number(*extra, "intervaloBarris", c.spawnInterval, false);
        c.mouseSensitivity = number(*extra, "sensibilidadeMouse", c.mouseSensitivity, false);
        if (extra->Attribute("semente")) {
            const double seed = number(*extra, "semente", 0);
            range(seed, 0, 4294967295.0, "outros.semente");
            if (std::floor(seed) != seed) throw std::runtime_error("XML: semente deve ser inteira.");
            c.seed = static_cast<std::uint32_t>(seed);
            c.fixedSeed = true;
        }
    }
    c.validate();
    return c;
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
