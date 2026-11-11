#include "configuracao.hpp"
#include "renderizador.hpp"
#include <GL/gl.h>
#include <emscripten.h>
#include <emscripten/html5.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

EM_JS(void, inicializarEmulacao, (), {
    Browser.useWebGL = true;
    Browser.moduleContextCreatedCallbacks.forEach((callback) => callback());
});

EM_JS(void, reportarErro, (const char* mensagem), {
    throw new Error(UTF8ToString(mensagem));
});

namespace
{
    std::unique_ptr<cg::Jogo> jogo;
    cg::Renderizador renderizador;
    std::array<bool, 256> teclas{};
    double ultimoQuadro = 0;
    std::string estadoJson;

    void liberarEntradas()
    {
        teclas.fill(false);
        jogo->limparEntradas();
    }

    bool atualizar(double agora, void*)
    {
        const double dt = ultimoQuadro > 0 ? std::clamp((agora - ultimoQuadro) / 1000.0, 0.0, 0.05) : 0;
        ultimoQuadro = agora;
        jogo->avancar(dt);

        int largura = 0, altura = 0;
        emscripten_get_canvas_element_size("#game-canvas", &largura, &altura);
        if (largura != renderizador.largura() || altura != renderizador.altura())
            renderizador.redimensionar(largura, altura);
        renderizador.desenhar(*jogo);
        return true;
    }
}

extern "C"
{
    EMSCRIPTEN_KEEPALIVE void cg_reset()
    {
        if (!jogo)
            return;
        jogo->selecionarModo(cg::ModoJogo::Campanha);
        liberarEntradas();
        ultimoQuadro = 0;
    }

    EMSCRIPTEN_KEEPALIVE void cg_release()
    {
        if (jogo)
            liberarEntradas();
        ultimoQuadro = 0;
    }

    EMSCRIPTEN_KEEPALIVE void cg_pause(int pausado)
    {
        if (!jogo)
            return;
        liberarEntradas();
        if (jogo->pausado() != (pausado != 0))
            jogo->alternarPausa();
        ultimoQuadro = 0;
    }

    EMSCRIPTEN_KEEPALIVE void cg_key(int ascii, int pressionada)
    {
        if (!jogo || ascii < 0 || ascii > 255)
            return;
        const auto tecla = static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(ascii)));
        const bool repetida = teclas[tecla];
        teclas[tecla] = pressionada != 0;

        if (tecla == 'w' || tecla == 'a' || tecla == 's' || tecla == 'd')
            jogo->definirTecla(tecla, pressionada != 0);
        else if (pressionada && !repetida)
        {
            if (tecla == 'r')
                cg_reset();
            else if (tecla == 'p' || tecla == 27)
                cg_pause(!jogo->pausado());
            else if (tecla == 'h')
            {
                jogo->alternarAjuda();
                teclas.fill(false);
                ultimoQuadro = 0;
            }
            teclas[tecla] = true;
        }
    }

    EMSCRIPTEN_KEEPALIVE void cg_aim(double deslocamento)
    {
        if (jogo)
            jogo->mirar(deslocamento);
    }

    EMSCRIPTEN_KEEPALIVE void cg_fire(int pressionada)
    {
        if (jogo)
            jogo->definirDisparo(pressionada != 0);
    }

    EMSCRIPTEN_KEEPALIVE const char* cg_state()
    {
        if (!jogo)
            return "{}";
        const auto& p = jogo->jogador();
        const auto& c = jogo->configuracao();
        std::ostringstream json;
        json << std::boolalpha << std::setprecision(12)
             << "{\"mode\":" << static_cast<int>(jogo->modo())
             << ",\"campaign\":" << jogo->campanha()
             << ",\"status\":" << static_cast<int>(jogo->estado())
             << ",\"stage\":" << jogo->fase()
             << ",\"health\":" << jogo->vidaJogador()
             << ",\"paused\":" << jogo->pausado()
             << ",\"help\":" << jogo->ajudaVisivel()
             << ",\"x\":" << p.posicao.x << ",\"y\":" << p.posicao.y
             << ",\"angle\":" << p.angulo << ",\"gunAngle\":" << p.anguloArma
             << ",\"elapsed\":" << jogo->tempoDecorrido()
             << ",\"barrels\":" << jogo->barris().size()
             << ",\"bullets\":" << jogo->tiros().size()
             << ",\"width\":" << c.larguraArena << ",\"height\":" << c.alturaArena
             << ",\"destroyed\":" << jogo->destruidos()
             << ",\"stageDestroyed\":" << jogo->destruidosNaFase()
             << ",\"stageGoal\":" << jogo->metaFase()
             << ",\"transitioning\":" << jogo->emTransicao() << '}';
        estadoJson = json.str();
        return estadoJson.c_str();
    }
}

int main()
{
    try
    {
        const auto configuracao = cg::Configuracao::carregar("/configuracoes.xml");
        jogo = std::make_unique<cg::Jogo>(configuracao);
        jogo->selecionarModo(cg::ModoJogo::Campanha);
        emscripten_set_canvas_element_size("#game-canvas", configuracao.larguraArena, configuracao.alturaArena);

        EmscriptenWebGLContextAttributes atributos;
        emscripten_webgl_init_context_attributes(&atributos);
        atributos.alpha = false;
        atributos.depth = false;
        atributos.stencil = false;
        atributos.antialias = false;
        atributos.majorVersion = 1;
        const auto contexto = emscripten_webgl_create_context("#game-canvas", &atributos);
        if (!contexto || emscripten_webgl_make_context_current(contexto) != EMSCRIPTEN_RESULT_SUCCESS)
            throw std::runtime_error("WebGL indisponivel neste navegador.");

        // A API HTML5 cria o contexto sem executar os inicializadores da emulacao OpenGL 1.x.
        inicializarEmulacao();
        renderizador.redimensionar(configuracao.larguraArena, configuracao.alturaArena);
        renderizador.desenhar(*jogo);
        emscripten_request_animation_frame_loop(atualizar, nullptr);
        return 0;
    }
    catch (const std::exception& erro)
    {
        std::cerr << erro.what() << '\n';
        reportarErro(erro.what());
        return 1;
    }
}
