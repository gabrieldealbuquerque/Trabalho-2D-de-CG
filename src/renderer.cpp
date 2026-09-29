#include "renderer.hpp"
#include <GL/freeglut.h>

#include <array>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace cg {
namespace {
struct Color { float r, g, b; };
constexpr Color ink{0.055f, 0.10f, 0.16f};
constexpr Color white{0.91f, 0.96f, 0.98f};
constexpr Color muted{0.53f, 0.73f, 0.82f};
constexpr Color green{0.40f, 0.83f, 0.38f};
constexpr Color purple{0.72f, 0.41f, 0.91f};
void color(Color c) { glColor3f(c.r, c.g, c.b); }
void rect(double x, double y, double w, double h, Color c) {
    color(c);
    glBegin(GL_QUADS);
    glVertex2d(x, y); glVertex2d(x + w, y);
    glVertex2d(x + w, y + h); glVertex2d(x, y + h);
    glEnd();
}
void disk(double x, double y, double radius, Color c, bool outline = false) {
    color(c);
    glBegin(outline ? GL_LINE_LOOP : GL_TRIANGLE_FAN);
    if (!outline) glVertex2d(x, y);
    for (int i = 0; i <= 64; ++i) {
        const double a = 2 * pi * i / 64;
        glVertex2d(x + std::cos(a) * radius, y + std::sin(a) * radius);
    }
    glEnd();
}
std::array<unsigned char, 7> glyph(char ch) {
    switch (std::toupper(static_cast<unsigned char>(ch))) {
    case 'A': return {14,17,17,31,17,17,17};
    case 'B': return {30,17,17,30,17,17,30};
    case 'C': return {14,17,16,16,16,17,14};
    case 'D': return {30,17,17,17,17,17,30};
    case 'E': return {31,16,16,30,16,16,31};
    case 'F': return {31,16,16,30,16,16,16};
    case 'G': return {14,17,16,23,17,17,14};
    case 'H': return {17,17,17,31,17,17,17};
    case 'I': return {14,4,4,4,4,4,14};
    case 'J': return {7,2,2,2,18,18,12};
    case 'K': return {17,18,20,24,20,18,17};
    case 'L': return {16,16,16,16,16,16,31};
    case 'M': return {17,27,21,21,17,17,17};
    case 'N': return {17,25,21,19,17,17,17};
    case 'O': return {14,17,17,17,17,17,14};
    case 'P': return {30,17,17,30,16,16,16};
    case 'Q': return {14,17,17,17,21,18,13};
    case 'R': return {30,17,17,30,20,18,17};
    case 'S': return {15,16,16,14,1,1,30};
    case 'T': return {31,4,4,4,4,4,4};
    case 'U': return {17,17,17,17,17,17,14};
    case 'V': return {17,17,17,17,17,10,4};
    case 'W': return {17,17,17,21,21,21,10};
    case 'X': return {17,17,10,4,10,17,17};
    case 'Y': return {17,17,10,4,4,4,4};
    case 'Z': return {31,1,2,4,8,16,31};
    case '0': return {14,17,19,21,25,17,14};
    case '1': return {4,12,4,4,4,4,14};
    case '2': return {14,17,1,2,4,8,31};
    case '3': return {30,1,1,14,1,1,30};
    case '4': return {2,6,10,18,31,2,2};
    case '5': return {31,16,16,30,1,1,30};
    case '6': return {14,16,16,30,17,17,14};
    case '7': return {31,1,2,4,8,8,8};
    case '8': return {14,17,17,14,17,17,14};
    case '9': return {14,17,17,15,1,1,14};
    case ':': return {0,4,4,0,4,4,0};
    case '/': return {1,1,2,4,8,16,16};
    case '-': return {0,0,0,31,0,0,0};
    case '+': return {0,4,4,31,4,4,0};
    case '.': return {0,0,0,0,0,4,4};
    case '(': return {2,4,8,8,8,4,2};
    case ')': return {8,4,2,2,2,4,8};
    case '!': return {4,4,4,4,4,0,4};
    default: return {};
    }
}
double textWidth(const std::string& text, double scale) { return text.size() * 6 * scale; }
void text(const std::string& value, double x, double y, double scale, Color c) {
    color(c);
    glBegin(GL_QUADS);
    for (char ch : value) {
        const auto rows = glyph(ch);
        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (!(rows[row] & (1 << (4 - col)))) continue;
                const double left = x + col * scale, bottom = y + (6 - row) * scale;
                glVertex2d(left, bottom); glVertex2d(left + scale, bottom);
                glVertex2d(left + scale, bottom + scale); glVertex2d(left, bottom + scale);
            }
        }
        x += 6 * scale;
    }
    glEnd();
}
void centered(const std::string& value, double x, double y, double scale, Color c) {
    text(value, x - textWidth(value, scale) / 2, y, scale, c);
}
void outlineRect(const Rect& r, Color c) {
    color(c);
    glBegin(GL_LINE_LOOP);
    glVertex2d(r.center.x - r.width / 2, r.center.y - r.height / 2);
    glVertex2d(r.center.x + r.width / 2, r.center.y - r.height / 2);
    glVertex2d(r.center.x + r.width / 2, r.center.y + r.height / 2);
    glVertex2d(r.center.x - r.width / 2, r.center.y + r.height / 2);
    glEnd();
}
}

void Renderer::resize(int width, int height) {
    width_ = std::max(1, width);
    height_ = std::max(1, height);
    glViewport(0, 0, width_, height_);
}

void Renderer::character(const Character& c, bool enemy, EnemyKind kind) const {
    glPushMatrix();
    glTranslated(c.position.x, c.position.y, 0);
    glRotated(c.angle, 0, 0, 1);
    glScaled(c.radius, c.radius, 1);
    const double stride = std::sin(c.walkPhase) * 0.58;
    const Color black{0.015f, 0.025f, 0.035f};
    rect(-0.85, -1.40 + stride, 0.5, 1.65, black);
    rect(0.35, -1.40 - stride, 0.5, 1.65, black);
    const bool spread = enemy && kind == EnemyKind::SpreadGunner;
    const bool cannon = enemy && kind == EnemyKind::Cannoneer;
    const Color uniform = spread ? Color{0.93f,0.43f,0.25f} : cannon ? Color{0.34f,0.65f,0.83f} : purple;
    disk(0, 0, 1, enemy ? uniform : green);
    disk(0, 0, 1, ink, true);
    disk(-0.27, 0.30, 0.18, enemy ? Color{0.85f,0.64f,0.97f} : Color{0.67f,0.95f,0.55f});
    if (spread || cannon) {
        rect(-0.82, 0.08, 1.64, 0.62, ink);
        rect(-0.68, 0.24, 1.36, 0.33, uniform);
        rect(-0.48, 0.12, 0.96, 0.12, white);
    }
    glTranslated(1.05, -0.35, 0);
    glRotated(c.gunAngle, 0, 0, 1);
    rect(-0.22, -0.35, 0.44, 3.15, ink);
    rect(-0.16, -0.26, 0.32, 3.0, {0.91f,0.25f,0.29f});
    rect(-0.16, 2.5, 0.32, 0.24, {1.0f,0.53f,0.44f});
    if (spread) {
        for (double side : {-1.0, 1.0}) {
            rect(side * 0.30 - 0.12, 1.05, 0.24, 1.75, ink);
            rect(side * 0.30 - 0.07, 1.15, 0.14, 1.55, uniform);
        }
    } else if (cannon) {
        rect(-0.43, 0.1, 0.86, 2.70, ink);
        rect(-0.32, 0.2, 0.64, 2.48, {0.40f,0.49f,0.55f});
        rect(-0.32, 2.25, 0.64, 0.27, uniform);
    }
    glPopMatrix();
}

void Renderer::barrel(const Barrel& b) const {
    const double left = b.position.x - b.width / 2;
    const double bottom = b.position.y - b.height / 2;
    const Color body = b.kind == BarrelKind::Normal ? Color{0.91f,0.46f,0.12f} :
        b.kind == BarrelKind::Miniboss ? Color{0.67f,0.28f,0.09f} : Color{0.56f,0.10f,0.18f};
    const Color metal = b.isBoss() ? Color{0.85f,0.66f,0.27f} :
        b.enemyKind == EnemyKind::SpreadGunner ? Color{0.64f,0.23f,0.19f} :
        b.enemyKind == EnemyKind::Cannoneer ? Color{0.22f,0.42f,0.55f} : Color{0.27f,0.21f,0.18f};
    rect(left, bottom, b.width, b.height, body);
    for (int i = 0; i < 5; ++i) {
        const double y = std::fmod(i / 5.0 + b.rollPhase, 1.0) * b.height;
        rect(left, bottom + y, b.width, std::min(1.3, b.height / 20), {0.40f,0.19f,0.10f});
    }
    for (double offset : {0.17, 0.75}) {
        rect(left + offset * b.width, bottom, b.width * 0.08, b.height, metal);
        rect(left + offset * b.width, bottom + b.height * 0.06,
             b.width * 0.025, b.height * 0.88, {0.75f,0.63f,0.39f});
    }
    outlineRect(b.bounds(), ink);
    if (b.isBoss()) {
        for (double x : {0.06, 0.91})
            for (double y : {0.15, 0.50, 0.85})
                disk(left + b.width * x, bottom + b.height * y, std::min(b.width, b.height) * 0.035, metal);
    }
    if (b.isBoss()) {
        const bool final = b.kind == BarrelKind::FinalBoss;
        const double faceWidth = b.width * 0.32, faceHeight = b.height * 0.54;
        const double faceX = b.position.x - faceWidth / 2, faceY = b.position.y - faceHeight * 0.38;
        const Color brass{0.96f,0.75f,0.30f}, bone{0.90f,0.86f,0.68f};
        rect(faceX - 3, faceY - 3, faceWidth + 6, faceHeight + 6, ink);
        rect(faceX, faceY, faceWidth, faceHeight, final ? bone : brass);
        rect(faceX + faceWidth * 0.12, faceY + faceHeight * 0.86,
             faceWidth * 0.76, faceHeight * 0.20, final ? bone : brass);
        color(b.enraged() ? Color{1.0f,0.31f,0.22f} : metal);
        glBegin(GL_TRIANGLES);
        for (int i = -1; i <= 1; ++i) {
            const double x = b.position.x + i * faceWidth * 0.28;
            glVertex2d(x - faceWidth * 0.16, faceY + faceHeight);
            glVertex2d(x + faceWidth * 0.16, faceY + faceHeight);
            glVertex2d(x, faceY + faceHeight * (i == 0 ? 1.30 : 1.22));
        }
        glEnd();
        const Color eyes = b.enraged() ? Color{1,0.12f,0.08f} : ink;
        for (double side : {0.15, 0.65}) {
            rect(faceX + faceWidth * side, faceY + faceHeight * 0.47,
                 faceWidth * 0.20, faceHeight * 0.23, eyes);
            if (final)
                rect(faceX + faceWidth * (side + 0.06), faceY + faceHeight * 0.51,
                     faceWidth * 0.07, faceHeight * 0.10, {1,0.25f,0.20f});
        }
        rect(faceX + faceWidth * 0.24, faceY + faceHeight * 0.12, faceWidth * 0.52, faceHeight * 0.13, ink);
        if (final) {
            rect(faceX + faceWidth * 0.44, faceY + faceHeight * 0.29, faceWidth * 0.12, faceHeight * 0.13, ink);
            for (int i = 0; i < 3; ++i)
                rect(faceX + faceWidth * (0.3 + 0.15 * i), faceY + faceHeight * 0.12,
                     faceWidth * 0.06, faceHeight * 0.09, bone);
        }
        for (double side : {-1.0, 1.0}) {
            const Vec2 muzzle = b.bossMuzzle(side);
            rect(muzzle.x - b.width * 0.085, muzzle.y - 2, b.width * 0.17, b.height * 0.52, ink);
            rect(muzzle.x - b.width * 0.06, muzzle.y + 2, b.width * 0.12, b.height * 0.48, {0.30f,0.34f,0.39f});
            rect(muzzle.x - b.width * 0.035, muzzle.y + 5, b.width * 0.03, b.height * 0.38, {0.58f,0.62f,0.65f});
            if (b.telegraph > 0) {
                disk(muzzle.x, muzzle.y + 4, 3 + b.enemy.radius * 0.35 * b.telegraph, {1,0.66f,0.15f});
                disk(muzzle.x, muzzle.y + 4, 1 + b.enemy.radius * 0.14 * b.telegraph, white);
            }
        }
        if (b.telegraph > 0) {
            const double radius = std::min(b.width, b.height) * (0.58 + 0.10 * b.telegraph);
            disk(b.position.x, b.position.y, radius, {1,0.68f,0.24f}, true);
        }
    } else if (b.hasEnemy) {
        if (b.enemyKind == EnemyKind::Cannoneer) {
            for (int i = 0; i < 8; ++i) {
                const Vec2 direction = forward(i * 45.0 + b.attackIndex * 22.5);
                const Vec2 port = b.position + Vec2{direction.x * b.width * 0.40, direction.y * b.height * 0.40};
                const double radius = std::min(b.width, b.height) * 0.055;
                disk(port.x, port.y, radius, ink);
                disk(port.x, port.y, radius * 0.55,
                     b.telegraph > 0 ? Color{1,0.66f,0.15f} : Color{0.52f,0.65f,0.73f});
            }
        }
        character(b.enemy, true, b.enemyKind);
        if (b.telegraph > 0) {
            const Vec2 muzzle = b.enemy.muzzle();
            disk(muzzle.x, muzzle.y, b.enemy.radius * (0.15 + 0.25 * b.telegraph), {1,0.66f,0.15f});
        }
    }
    const std::string hp = std::to_string(b.hitPoints);
    const double scale = std::min({b.hasEnemy ? 1.4 : 2.4, b.width / (hp.size() * 6 + 6), b.height / 12});
    const double y = b.isBoss() ? bottom + 2 * scale : b.hasEnemy ? bottom + b.height - 10 * scale : b.position.y - 3.5 * scale;
    rect(b.position.x - (textWidth(hp, scale) + 4 * scale) / 2, y - scale,
         textWidth(hp, scale) + 4 * scale, 9 * scale, ink);
    centered(hp, b.position.x, y, scale, white);
}

void Renderer::hud(const Game& game) const {
    const auto& c = game.config();
    const std::string score = "BARRIS DESTRUIDOS: " + std::to_string(game.destroyed());
    const double scale = std::min(1.65, (c.arenaWidth - 16.0) / (score.size() * 6));
    const double boxWidth = textWidth(score, scale) + 12;
    rect(c.arenaWidth - boxWidth, c.arenaHeight - 25 * scale, boxWidth, 25 * scale, ink);
    text(score, c.arenaWidth - boxWidth + 6, c.arenaHeight - 12 * scale, scale, white);
    const std::string stage = "FASE " + std::to_string(game.stage()) + " / " + std::to_string(Game::stageCount);
    const double stageScale = std::min(1.7, (c.arenaWidth - 12.0) / (stage.size() * 6));
    const bool compact = c.arenaWidth - boxWidth < textWidth(stage, stageScale) + 12;
    const double stageY = c.arenaHeight - (compact ? 25 * scale + 17 : 12 * scale);
    text(stage, 6, stageY, stageScale, game.isBossStage() ? Color{1.0f,0.70f,0.30f} : green);
    rect(c.arenaWidth - boxWidth + 6, c.arenaHeight - 20 * scale, boxWidth - 12, 2.5 * scale, {0.13f,0.24f,0.30f});
    rect(c.arenaWidth - boxWidth + 6, c.arenaHeight - 20 * scale,
         (boxWidth - 12) * game.stageDestroyed() / game.stageGoal(), 2.5 * scale, green);
    const double small = std::min(1.1, (c.arenaWidth - 10.0) / 174);
    double nextY = c.arenaHeight - 25 * scale - (compact ? 33 : 12);
    const std::string progress = game.isBossStage() ? (game.stage() == 5 ? "MINIBOSS: BARAO DO TAMBOR" : "BOSS FINAL: REI DA POLVORA") :
        "META DA FASE: " + std::to_string(game.stageDestroyed()) + " / " + std::to_string(game.stageGoal());
    const double progressScale = std::min(1.15, (c.arenaWidth - 12.0) / (progress.size() * 6));
    text(progress, 6, nextY, progressScale, white);
    nextY -= 17;
    if (const auto* boss = game.boss()) {
        const std::string label = std::string(boss->enraged() ? "FURIA - " : "VIDA: ") +
            std::to_string(boss->hitPoints) + " / " + std::to_string(boss->maxHitPoints);
        const double labelScale = std::min(1.15, (c.arenaWidth - 16.0) / (label.size() * 6));
        rect(5, nextY - 11, c.arenaWidth - 10, 25, ink);
        text(label, 9, nextY + 2, labelScale, white);
        rect(9, nextY - 7, c.arenaWidth - 18, 4, {0.25f,0.17f,0.19f});
        rect(9, nextY - 7, (c.arenaWidth - 18) * boss->hitPoints / boss->maxHitPoints, 4,
             boss->enraged() ? Color{1.0f,0.22f,0.16f} : Color{0.96f,0.65f,0.24f});
        nextY -= 27;
        if (boss->telegraph > 0) {
            const std::string warning = std::string("ALERTA: ") + boss->attackName();
            const double s = std::min(1.1, (c.arenaWidth - 12.0) / (warning.size() * 6));
            text(warning, 6, nextY, s, {1,0.70f,0.30f});
            nextY -= 15;
        }
    }
    text("F1 AJUDA   P PAUSA   R REINICIA", 5, nextY, small, muted);
    const auto& d = game.demo();
    if (!d.barrelMotion || !d.enemyFire || !d.spawning || d.hitboxes) {
        const std::string mode = std::string("DEMO  F2:") + (d.barrelMotion ? "ON" : "OFF") +
            " F3:" + (d.enemyFire ? "ON" : "OFF") + " F4:" + (d.spawning ? "ON" : "OFF") +
            " F5:" + (d.hitboxes ? "ON" : "OFF");
        const double s = std::min(1.15, (c.arenaWidth - 10.0) / (mode.size() * 6));
        const double y = nextY - 19;
        rect(0, y, c.arenaWidth, 14, ink);
        text(mode, 5, y + 3, s, {1.0f,0.80f,0.34f});
    }
}

void Renderer::overlay(const Game& game) const {
    const double w = game.config().arenaWidth, h = game.config().arenaHeight;
    if (game.helpVisible()) {
        const double scale = std::min({1.5, (w - 20) / 270, (h - 20) / 168});
        const double boxHeight = 154 * scale, boxWidth = 276 * scale;
        rect((w - boxWidth) / 2, (h - boxHeight) / 2, boxWidth, boxHeight, ink);
        const double top = (h + boxHeight) / 2;
        centered("ATAQUE DE BARRIS", w / 2, top - 20 * scale, 1.7 * scale, green);
        const std::vector<std::string> lines = {
            "W / S     ANDAR PARA FRENTE / TRAS", "A / D     GIRAR ESQUERDA / DIREITA",
            "MOUSE     GIRAR ARMA (-30 A +30 GRAUS)", "CLICAR    TIRO / SEGURAR: CONTINUO",
            "P         PAUSAR OU CONTINUAR", "R         REINICIAR A PARTIDA",
            "F2        MOVIMENTO DOS BARRIS", "F3        TIROS DOS INIMIGOS",
            "F4        NASCIMENTO DE BARRIS", "F5        MOSTRAR COLISOES",
            "ESC       SAIR", "F1 / H    FECHAR AJUDA E JOGAR"};
        for (std::size_t i = 0; i < lines.size(); ++i)
            text(lines[i], (w - boxWidth) / 2 + 7 * scale, top - (35 + 9 * i) * scale, scale, white);
        return;
    }
    if (!game.paused() && !game.transitioning() && game.status() == Status::Playing) return;
    const double scale = std::min(2.8, (w - 20) / 132);
    rect(w * 0.06, h / 2 - 29 * scale, w * 0.88, 66 * scale, ink);
    const bool won = game.status() == Status::Won;
    const std::string title = game.paused() ? "PAUSADO" : game.transitioning() ? "FASE " + std::to_string(game.stage()) + " CONCLUIDA" :
        won ? "VOCE GANHOU!" : "VOCE PERDEU";
    const double titleScale = std::min(scale, w * 0.82 / (title.size() * 6));
    centered(title, w / 2, h / 2 + 14 * scale, titleScale, won || game.transitioning() ? green : white);
    const std::string next = game.stage() + 1 == 5 ? "MINIBOSS NA FASE 5" : game.stage() + 1 == 10 ?
        "BOSS FINAL NA FASE 10" : "PREPARE-SE PARA A FASE " + std::to_string(game.stage() + 1);
    const std::string detail = game.paused() ? "P PARA CONTINUAR" : game.transitioning() ? next : won ?
        "10 FASES CONCLUIDAS - BOSS DERROTADO" : game.lossReason() == LossReason::Shot ?
        "ATINGIDO POR UM TIRO" : "COLISAO COM UM BARRIL";
    const double small = std::min(scale * 0.48, (w * 0.85) / (detail.size() * 6));
    centered(detail, w / 2, h / 2 - 2 * scale, small, muted);
    const std::string hint = game.transitioning() && !game.paused() ? "CONTINUA EM " +
        std::to_string(static_cast<int>(std::ceil(game.transitionRemaining()))) + "..." : "R REINICIA    ESC SAI";
    centered(hint, w / 2, h / 2 - 18 * scale, scale * 0.45, white);
}

void Renderer::draw(const Game& game) const {
    const auto& c = game.config();
    glClearColor(0.075f, 0.20f, 0.29f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, c.arenaWidth, 0, c.arenaHeight, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glLineWidth(1);
    color(muted);
    glBegin(GL_LINES);
    for (int x = 0; x < c.arenaWidth; x += 20) {
        glVertex2d(x, c.arenaHeight / 2.0); glVertex2d(std::min(x + 10, c.arenaWidth), c.arenaHeight / 2.0);
    }
    glEnd();
    for (const auto& b : game.barrels()) barrel(b);
    character(game.player(), false);
    for (const auto& b : game.bullets()) {
        if (b.bossShot) {
            disk(b.position.x, b.position.y, b.radius + 1, ink);
            disk(b.position.x, b.position.y, b.radius, {1,0.20f,0.25f});
            disk(b.position.x, b.position.y, b.radius * 0.45, {1,0.84f,0.62f});
        } else {
            disk(b.position.x, b.position.y, b.radius, b.owner == Owner::Player ? Color{1,0.90f,0.4f} : Color{1,0.40f,0.68f});
        }
    }
    if (game.demo().hitboxes) {
        disk(game.player().position.x, game.player().position.y, game.player().radius, white, true);
        for (const auto& b : game.barrels()) outlineRect(b.bounds(), white);
        color(white);
        const Vec2 tip = game.player().muzzle(), end = tip + forward(game.player().angle + game.player().gunAngle) * 75;
        glBegin(GL_LINES); glVertex2d(tip.x, tip.y); glVertex2d(end.x, end.y); glEnd();
    }
    hud(game);
    overlay(game);
}

void Renderer::saveFrame(const std::filesystem::path& path) const {
    std::vector<unsigned char> pixels(static_cast<std::size_t>(width_) * height_ * 3);
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width_, height_, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    std::ofstream file(path, std::ios::binary);
    file << "P6\n" << width_ << ' ' << height_ << "\n255\n";
    for (int y = height_ - 1; y >= 0; --y)
        file.write(reinterpret_cast<const char*>(pixels.data() + static_cast<std::size_t>(y) * width_ * 3), width_ * 3);
    if (!file) throw std::runtime_error("Nao foi possivel gravar captura: " + path.string());
}
}
