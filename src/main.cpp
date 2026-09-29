#include "config.hpp"
#include "controller.hpp"
#include "renderer.hpp"
#include <GL/freeglut.h>

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

namespace {
std::unique_ptr<cg::Game> game;
std::unique_ptr<cg::Controller> controller;
cg::Renderer renderer;
std::filesystem::path diagnostics;
bool captureRequested = false;
int captureNumber = 0, exitCode = 0;
using Clock = std::chrono::steady_clock;
Clock::time_point lastUpdate, lastFrame;

void capture() {
    const auto prefix = diagnostics / ("quadro-" + std::to_string(++captureNumber));
    renderer.saveFrame(prefix.string() + ".ppm");
    std::ofstream file(prefix.string() + ".json");
    const auto& p = game->player();
    file << std::setprecision(12)
         << "{\"width\":" << renderer.width() << ",\"height\":" << renderer.height()
         << ",\"x\":" << p.position.x << ",\"y\":" << p.position.y
         << ",\"angle\":" << p.angle << ",\"gunAngle\":" << p.gunAngle
         << ",\"walkPhase\":" << p.walkPhase << ",\"destroyed\":" << game->destroyed()
         << ",\"stage\":" << game->stage() << ",\"stageDestroyed\":" << game->stageDestroyed()
         << ",\"stageGoal\":" << game->stageGoal() << ",\"transitioning\":" << game->transitioning()
         << ",\"transitionRemaining\":" << game->transitionRemaining()
         << ",\"status\":" << static_cast<int>(game->status())
         << ",\"lossReason\":" << static_cast<int>(game->lossReason())
         << ",\"paused\":" << game->paused() << ",\"help\":" << game->helpVisible()
         << ",\"barrelMotion\":" << game->demo().barrelMotion
         << ",\"enemyFire\":" << game->demo().enemyFire
         << ",\"spawning\":" << game->demo().spawning
         << ",\"hitboxes\":" << game->demo().hitboxes
         << ",\"bullets\":" << game->bullets().size()
         << ",\"barrels\":" << game->barrels().size()
         << ",\"elapsed\":" << game->elapsed()
         << ",\"barrelStates\":[";
    bool first = true;
    for (const auto& b : game->barrels()) {
        if (!first) file << ',';
        first = false;
        file << "{\"x\":" << b.position.x << ",\"y\":" << b.position.y
             << ",\"hp\":" << b.hitPoints << ",\"enemy\":" << b.hasEnemy
             << ",\"maxHp\":" << b.maxHitPoints << ",\"kind\":" << static_cast<int>(b.kind)
             << ",\"enemyKind\":" << static_cast<int>(b.enemyKind)
             << ",\"width\":" << b.width << ",\"height\":" << b.height << ",\"enraged\":" << b.enraged()
             << ",\"attackIndex\":" << b.attackIndex << ",\"telegraph\":" << b.telegraph
             << ",\"gunAngle\":" << b.enemy.gunAngle << '}';
    }
    file << "]}\n";
    if (!file) throw std::runtime_error("Nao foi possivel gravar diagnostico.");
}
void display() {
    try {
        renderer.draw(*game);
        if (captureRequested) { capture(); captureRequested = false; }
        const GLenum error = glGetError();
        if (error != GL_NO_ERROR) throw std::runtime_error("Erro OpenGL: " + std::to_string(error));
        glutSwapBuffers();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        exitCode = 1;
        glutLeaveMainLoop();
    }
}
void reshape(int width, int height) { renderer.resize(width, height); }
void keyboard(unsigned char key, int, int) {
    if (!controller->keyDown(key)) glutLeaveMainLoop();
    glutPostRedisplay();
}
void keyboardUp(unsigned char key, int, int) { controller->keyUp(key); }
void special(int key, int, int) {
    if (key >= GLUT_KEY_F1 && key <= GLUT_KEY_F5) controller->special(key - GLUT_KEY_F1 + 1);
    if (key == GLUT_KEY_F12 && !diagnostics.empty()) captureRequested = true;
    glutPostRedisplay();
}
void mouse(int button, int state, int x, int) {
    if (button == GLUT_LEFT_BUTTON) controller->mouseLeft(state == GLUT_DOWN, x);
}
void motion(int x, int) { controller->motion(x); }
void entry(int state) { if (state == GLUT_LEFT) controller->leave(); }
void idle() {
    const auto now = Clock::now();
    game->advance(std::chrono::duration<double>(now - lastUpdate).count());
    lastUpdate = now;
    if (std::chrono::duration<double>(now - lastFrame).count() >= 1.0 / 60) {
        glutPostRedisplay();
        lastFrame = now;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}
}

int main(int argc, char** argv) {
    try {
        auto configPath = cg::executableDirectory(argv[0]) / "configuracoes.xml";
        bool checkOnly = false;
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if ((arg == "--config" || arg == "--diagnostico") && i + 1 < argc) {
                const std::filesystem::path value = argv[++i];
                if (arg == "--config") configPath = value;
                else diagnostics = value;
            } else if (arg == "--verificar-config") checkOnly = true;
            else if (arg == "--ajuda" || arg == "--help") {
                std::cout << "Uso: trabalhocg [--config arquivo.xml] [--verificar-config] [--diagnostico pasta]\n"
                          << "W/S: andar; A/D: girar; mouse: arma; clique esquerdo: tiro; segurar: continuo.\n"
                          << "F1/H: ajuda; P: pausa; R: reinicia; ESC: sair.\n"
                          << "F2: movimento dos barris; F3: tiros inimigos; F4: novos barris; F5: colisoes.\n"
                          << "F12: salvar imagem/estado, quando --diagnostico estiver ativo.\n";
                return 0;
            } else throw std::runtime_error("Argumento invalido ou sem valor: " + arg);
        }
        const auto config = cg::Config::load(configPath);
        if (checkOnly) {
            std::cout << "Configuracao valida: " << config.arenaWidth << 'x' << config.arenaHeight << '\n';
            return 0;
        }
        if (!diagnostics.empty()) std::filesystem::create_directories(diagnostics);
        game = std::make_unique<cg::Game>(config);
        controller = std::make_unique<cg::Controller>(*game);
        int glutArgc = 1;
        glutInit(&glutArgc, argv);
        glutInitContextVersion(1, 0);
        glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
        glutInitWindowSize(config.arenaWidth, config.arenaHeight);
        glutCreateWindow("Ataque de Barris - Computacao Grafica");
        glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
        glutIgnoreKeyRepeat(1);
        renderer.resize(config.arenaWidth, config.arenaHeight);
        std::cout << "Driver OpenGL: " << glGetString(GL_VERSION) << "; API utilizada: OpenGL 1.0\n";
        glutDisplayFunc(display);
        glutReshapeFunc(reshape);
        glutKeyboardFunc(keyboard);
        glutKeyboardUpFunc(keyboardUp);
        glutSpecialFunc(special);
        glutMouseFunc(mouse);
        glutMotionFunc(motion);
        glutPassiveMotionFunc(motion);
        glutEntryFunc(entry);
        glutIdleFunc(idle);
        lastFrame = lastUpdate = Clock::now();
        glutMainLoop();
        return exitCode;
    } catch (const std::exception& error) {
        std::cerr << "Erro: " << error.what() << '\n';
        return 1;
    }
}
