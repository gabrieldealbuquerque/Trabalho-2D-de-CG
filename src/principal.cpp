#include "configuracao.hpp"
#include "controlador.hpp"
#include "renderizador.hpp"
#include <GL/freeglut.h>

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

namespace
{
    std::unique_ptr<cg::Jogo> jogo;
    std::unique_ptr<cg::Controlador> controlador;
    cg::Renderizador renderizador;
    std::filesystem::path diagnosticos;
    bool capturaSolicitada = false;
    int numeroCaptura = 0;
    int codigoSaida = 0;

    using Relogio = std::chrono::steady_clock;
    Relogio::time_point ultimaAtualizacao;
    Relogio::time_point ultimoQuadro;
    constexpr double intervaloQuadros = 1.0 / 60;

    struct Opcoes
    {
        std::filesystem::path caminhoConfiguracao;
        std::filesystem::path caminhoDiagnosticos;
        bool apenasVerificar = false;
        bool ajuda = false;
    };

    Opcoes lerArgumentos(int argc, char** argv)
    {
        Opcoes opcoes;
        opcoes.caminhoConfiguracao = cg::diretorioExecutavel(argv[0]) / "configuracoes.xml";

        for (int i = 1; i < argc; ++i)
        {
            const std::string argumento = argv[i];
            if (argumento == "--config" || argumento == "--diagnostico")
            {
                if (i + 1 >= argc)
                    throw std::runtime_error("Argumento sem valor: " + argumento);

                const std::filesystem::path valor = argv[++i];
                if (argumento == "--config")
                    opcoes.caminhoConfiguracao = valor;
                else
                    opcoes.caminhoDiagnosticos = valor;
            }
            else if (argumento == "--verificar-config")
            {
                opcoes.apenasVerificar = true;
            }
            else if (argumento == "--ajuda" || argumento == "--help")
            {
                opcoes.ajuda = true;
            }
            else
            {
                throw std::runtime_error("Argumento invalido: " + argumento);
            }

            if (opcoes.ajuda)
                break;
        }

        return opcoes;
    }

    void imprimirAjuda()
    {
        std::cout << "Uso: trabalhocg [--config arquivo.xml] [--verificar-config] [--diagnostico pasta]\n"
                  << "W/S: andar; A/D: girar; mouse: arma; clique esquerdo: tiro; segurar: continuo.\n"
                  << "F1/H: ajuda; P: pausa; R: reinicia; ESC: sair.\n"
                  << "Menu: 1 modo do trabalho; 2 campanha; M volta ao menu.\n"
                  << "F2: movimento dos barris; F3: tiros inimigos; F4: novos barris; F5: colisoes.\n"
                  << "Cenas do trabalho: F6 movimento/limites; F7 mira/resistencia; F8 oponente.\n"
                  << "F9 colisao; F10 derrota por tiro; F11 ultimo barril (placar inicial N-1).\n"
                  << "As cenas reiniciam no modo do trabalho. R volta a partida normal.\n"
                  << "F12: salvar imagem/estado, quando --diagnostico estiver ativo.\n";
    }

    void capturar()
    {
        const auto prefixo = diagnosticos / ("quadro-" + std::to_string(++numeroCaptura));
        renderizador.salvarQuadro(prefixo.string() + ".ppm");

        std::ofstream arquivo(prefixo.string() + ".json");
        const auto& p = jogo->jogador();
        arquivo << std::setprecision(12) << "{\"width\":" << renderizador.largura() << ",\"height\":" << renderizador.altura()
             << ",\"x\":" << p.posicao.x << ",\"y\":" << p.posicao.y << ",\"angle\":" << p.angulo
             << ",\"gunAngle\":" << p.anguloArma << ",\"walkPhase\":" << p.fasePassada
             << ",\"destroyed\":" << jogo->destruidos() << ",\"stage\":" << jogo->fase()
             << ",\"stageDestroyed\":" << jogo->destruidosNaFase() << ",\"stageGoal\":" << jogo->metaFase()
             << ",\"transitioning\":" << jogo->emTransicao()
             << ",\"transitionRemaining\":" << jogo->transicaoRestante()
             << ",\"status\":" << static_cast<int>(jogo->estado()) << ",\"mode\":" << static_cast<int>(jogo->modo())
             << ",\"playerHealth\":" << jogo->vidaJogador() << ",\"invulnerable\":" << jogo->invulneravel()
             << ",\"lossReason\":" << static_cast<int>(jogo->motivoDerrota()) << ",\"paused\":" << jogo->pausado()
             << ",\"help\":" << jogo->ajudaVisivel() << ",\"barrelMotion\":" << jogo->demonstracao().movimentoBarris
             << ",\"enemyFire\":" << jogo->demonstracao().dispararInimigo << ",\"spawning\":" << jogo->demonstracao().criarNovosBarris
             << ",\"hitboxes\":" << jogo->demonstracao().contornosColisao << ",\"bullets\":" << jogo->tiros().size()
             << ",\"barrels\":" << jogo->barris().size() << ",\"elapsed\":" << jogo->tempoDecorrido()
             << ",\"barrelStates\":[";

        bool primeiro = true;
        for (const auto& b : jogo->barris())
        {
            if (!primeiro)
                arquivo << ',';

            primeiro = false;
            arquivo << "{\"x\":" << b.posicao.x << ",\"y\":" << b.posicao.y << ",\"hp\":" << b.resistencia
                 << ",\"enemy\":" << b.temInimigo << ",\"maxHp\":" << b.resistenciaMaxima
                 << ",\"kind\":" << static_cast<int>(b.tipo) << ",\"enemyKind\":" << static_cast<int>(b.tipoInimigo)
                 << ",\"width\":" << b.largura << ",\"height\":" << b.altura << ",\"enraged\":" << b.emFuria()
                 << ",\"attackIndex\":" << b.indiceAtaque << ",\"telegraph\":" << b.avisoAtaque
                 << ",\"gunAngle\":" << b.inimigo.anguloArma << '}';
        }

        arquivo << "]}\n";

        if (!arquivo)
            throw std::runtime_error("Nao foi possivel gravar diagnostico.");
    }

    void exibir()
    {
        try
        {
            renderizador.desenhar(*jogo);
            if (capturaSolicitada)
            {
                capturar();
                capturaSolicitada = false;
            }

            const GLenum erro = glGetError();
            if (erro != GL_NO_ERROR)
                throw std::runtime_error("Erro OpenGL: " + std::to_string(erro));

            glutSwapBuffers();
        }
        catch (const std::exception& erro)
        {
            std::cerr << erro.what() << '\n';
            codigoSaida = 1;
            glutLeaveMainLoop();
        }
    }

    void redimensionarJanela(int largura, int altura)
    {
        renderizador.redimensionar(largura, altura);
    }

    void teclado(unsigned char tecla, int, int)
    {
        if (!controlador->pressionarTecla(tecla))
            glutLeaveMainLoop();

        glutPostRedisplay();
    }

    void soltarTeclado(unsigned char tecla, int, int)
    {
        controlador->soltarTecla(tecla);
    }

    void teclaEspecial(int tecla, int, int)
    {
        if (tecla >= GLUT_KEY_F1 && tecla <= GLUT_KEY_F11)
            controlador->teclaEspecial(tecla - GLUT_KEY_F1 + 1);

        if (tecla == GLUT_KEY_F12 && !diagnosticos.empty())
            capturaSolicitada = true;

        glutPostRedisplay();
    }

    void mouse(int botao, int estado, int x, int)
    {
        if (botao == GLUT_LEFT_BUTTON)
            controlador->botaoEsquerdo(estado == GLUT_DOWN, x);
    }

    void moverMouse(int x, int)
    {
        controlador->moverMouse(x);
    }

    void entradaMouse(int estado)
    {
        if (estado == GLUT_LEFT)
            controlador->sairDaJanela();
    }

    void atualizar()
    {
        const auto agora = Relogio::now();
        jogo->avancar(std::chrono::duration<double>(agora - ultimaAtualizacao).count());
        ultimaAtualizacao = agora;

        if (std::chrono::duration<double>(agora - ultimoQuadro).count() >= intervaloQuadros)
        {
            glutPostRedisplay();
            ultimoQuadro = agora;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

int main(int argc, char** argv)
{
    try
    {
        const Opcoes opcoes = lerArgumentos(argc, argv);
        if (opcoes.ajuda)
        {
            imprimirAjuda();
            return 0;
        }

        const auto configuracao = cg::Configuracao::carregar(opcoes.caminhoConfiguracao);
        if (opcoes.apenasVerificar)
        {
            std::cout << "Configuracao valida: " << configuracao.larguraArena << 'x' << configuracao.alturaArena << '\n';
            return 0;
        }

        diagnosticos = opcoes.caminhoDiagnosticos;
        if (!diagnosticos.empty())
            std::filesystem::create_directories(diagnosticos);

        jogo = std::make_unique<cg::Jogo>(configuracao);
        jogo->mostrarMenu();
        controlador = std::make_unique<cg::Controlador>(*jogo);

        int glutArgc = 1;
        glutInit(&glutArgc, argv);
        glutInitContextVersion(1, 0);
        glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
        glutInitWindowSize(configuracao.larguraArena, configuracao.alturaArena);
        glutCreateWindow("Ataque de Barris - Computacao Grafica");

        glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
        glutIgnoreKeyRepeat(1);
        renderizador.redimensionar(configuracao.larguraArena, configuracao.alturaArena);

        std::cout << "Driver OpenGL: " << glGetString(GL_VERSION) << "; API utilizada: OpenGL 1.0\n";

        glutDisplayFunc(exibir);
        glutReshapeFunc(redimensionarJanela);
        glutKeyboardFunc(teclado);
        glutKeyboardUpFunc(soltarTeclado);
        glutSpecialFunc(teclaEspecial);
        glutMouseFunc(mouse);
        glutMotionFunc(moverMouse);
        glutPassiveMotionFunc(moverMouse);
        glutEntryFunc(entradaMouse);
        glutIdleFunc(atualizar);

        ultimoQuadro = ultimaAtualizacao = Relogio::now();
        glutMainLoop();

        return codigoSaida;
    }
    catch (const std::exception& erro)
    {
        std::cerr << "Erro: " << erro.what() << '\n';
        return 1;
    }
}
