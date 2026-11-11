#include "renderizador.hpp"
#ifdef __EMSCRIPTEN__
#include <GL/gl.h>
#define glVertex2d glVertex2f
#include <cctype>
#else
#include <GL/freeglut.h>
#endif

#include <array>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace cg
{
    namespace
    {
        struct Cor
        {
            float r, g, b;
        };

        constexpr Cor escuro{0.055f, 0.10f, 0.16f};
        constexpr Cor branco{0.91f, 0.96f, 0.98f};
        constexpr Cor azulClaro{0.53f, 0.73f, 0.82f};
        constexpr Cor verde{0.53f, 0.75f, 0.32f};
        constexpr Cor roxo{0.65f, 0.43f, 0.80f};

        double unidadesPorPixelX = 1;
        double unidadesPorPixelY = 1;

        void definirCor(Cor c)
        {
            glColor3f(c.r, c.g, c.b);
        }

        void retangulo(double x, double y, double w, double h, Cor c)
        {
            definirCor(c);
#ifdef __EMSCRIPTEN__
            glBegin(GL_QUADS);
            glVertex2d(x, y);
            glVertex2d(x + w, y);
            glVertex2d(x + w, y + h);
            glVertex2d(x, y + h);
            glEnd();
#else
            glRectd(x, y, x + w, y + h);
#endif
        }

        void disco(double x, double y, double raio, Cor c, bool contorno = false)
        {
            definirCor(c);
            glBegin(contorno ? GL_LINE_LOOP : GL_TRIANGLE_FAN);
            if (!contorno)
                glVertex2d(x, y);

            for (int i = 0; i <= 64; ++i)
            {
                const double a = 2 * pi * i / 64;
                glVertex2d(x + std::cos(a) * raio, y + std::sin(a) * raio);
            }

            glEnd();
        }

#ifdef __EMSCRIPTEN__
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
#else
        void* fonteTexto(double tamanho)
        {
            if (tamanho >= 2.5)
                return GLUT_BITMAP_HELVETICA_18;
            if (tamanho >= 1.5)
                return GLUT_BITMAP_9_BY_15;

            return GLUT_BITMAP_HELVETICA_10;
        }

#endif

        double larguraTexto(const std::string& valor, double tamanho)
        {
#ifdef __EMSCRIPTEN__
            return valor.size() * 6 * tamanho;
#else
            double largura = 0;
            for (unsigned char caractere : valor)
                largura += glutBitmapWidth(fonteTexto(tamanho), caractere);

            return largura * unidadesPorPixelX;
#endif
        }

        double alturaTexto(double tamanho)
        {
#ifdef __EMSCRIPTEN__
            return 7 * tamanho;
#else
            return glutBitmapHeight(fonteTexto(tamanho)) * unidadesPorPixelY;
#endif
        }

        double ajustarTexto(const std::string& valor, double largura, double tamanhoPreferido)
        {
            for (double tamanho : {tamanhoPreferido, 1.5, 1.0})
                if (tamanho <= tamanhoPreferido && larguraTexto(valor, tamanho) <= largura)
                    return tamanho;

            return 1.0;
        }

        void texto(const std::string& valor, double x, double y, double tamanho, Cor c)
        {
            definirCor(c);
#ifdef __EMSCRIPTEN__
            glBegin(GL_QUADS);
            for (char caractere : valor)
            {
                const auto linhas = glyph(caractere);
                for (int linha = 0; linha < 7; ++linha)
                    for (int coluna = 0; coluna < 5; ++coluna)
                    {
                        if (!(linhas[linha] & (1 << (4 - coluna))))
                            continue;

                        const double esquerda = x + coluna * tamanho;
                        const double base = y + (6 - linha) * tamanho;
                        glVertex2d(esquerda, base);
                        glVertex2d(esquerda + tamanho, base);
                        glVertex2d(esquerda + tamanho, base + tamanho);
                        glVertex2d(esquerda, base + tamanho);
                    }
                x += 6 * tamanho;
            }
            glEnd();
#else
            glRasterPos2d(x, y);

            for (unsigned char caractere : valor)
                glutBitmapCharacter(fonteTexto(tamanho), caractere);
#endif
        }

        void textoCentralizado(const std::string& valor, double x, double y, double escala, Cor c)
        {
            texto(valor, x - larguraTexto(valor, escala) / 2, y, escala, c);
        }

        void contornoRetangulo(const Retangulo& r, Cor c)
        {
            definirCor(c);
            glBegin(GL_LINE_LOOP);
            glVertex2d(r.centro.x - r.largura / 2, r.centro.y - r.altura / 2);
            glVertex2d(r.centro.x + r.largura / 2, r.centro.y - r.altura / 2);
            glVertex2d(r.centro.x + r.largura / 2, r.centro.y + r.altura / 2);
            glVertex2d(r.centro.x - r.largura / 2, r.centro.y + r.altura / 2);
            glEnd();
        }
    }

    void Renderizador::redimensionar(int largura, int altura)
    {
        largura_ = std::max(1, largura);
        altura_ = std::max(1, altura);

        glViewport(0, 0, largura_, altura_);
    }

    void Renderizador::personagem(const Personagem& c, bool inimigo, TipoInimigo tipo) const
    {
        glPushMatrix();
        glTranslated(c.posicao.x, c.posicao.y, 0);
        glRotated(c.angulo, 0, 0, 1);
        glScaled(c.raio, c.raio, 1);

        const double passada = std::cos(c.fasePassada) * 0.75;
        const Cor preto{0.015f, 0.025f, 0.035f};
        const Cor vermelho{0.83f, 0.26f, 0.26f};

        retangulo(-0.85, -0.8 - passada, 0.5, 1.6, preto);
        retangulo(0.35, -0.8 + passada, 0.5, 1.6, preto);

        disco(0, 0, 1, escuro);
        disco(0, 0, 0.90, inimigo ? roxo : verde);
        disco(-0.25, 0.30, 0.15, inimigo ? Cor{0.80f, 0.62f, 0.91f} : Cor{0.72f, 0.87f, 0.48f});

        glTranslated(1.05, -0.35, 0);
        glRotated(c.anguloArma, 0, 0, 1);

        const double larguraArma = inimigo && tipo == TipoInimigo::Canhoneiro ? 0.88 : 0.58;
        retangulo(-larguraArma / 2, -0.26, larguraArma, 3.06, escuro);
        retangulo(-larguraArma / 2 + 0.07, -0.19, larguraArma - 0.14, 2.92, vermelho);
        retangulo(-larguraArma / 2 + 0.07, 2.48, larguraArma - 0.14, 0.20, {0.99f, 0.53f, 0.42f});

        if (inimigo && tipo == TipoInimigo::AtiradorEmLeque)
            for (double lado : {-1.0, 1.0})
            {
                retangulo(lado * 0.38 - 0.11, 1.05, 0.22, 1.75, escuro);
                retangulo(lado * 0.38 - 0.06, 1.15, 0.12, 1.55, vermelho);
            }

        glPopMatrix();
    }

    void Renderizador::barril(const Barril& b) const
    {
        const double esquerda = b.posicao.x - b.largura / 2;
        const double base = b.posicao.y - b.altura / 2;

        Cor corpo{0.55f, 0.32f, 0.16f};
        if (b.tipo == TipoBarril::Minichefe)
            corpo = {0.46f, 0.24f, 0.11f};
        else if (b.tipo == TipoBarril::ChefeFinal)
            corpo = {0.56f, 0.10f, 0.18f};

        Cor metal{0.27f, 0.21f, 0.18f};
        if (b.ehChefe())
            metal = {0.85f, 0.66f, 0.27f};
        else if (b.tipoInimigo == TipoInimigo::AtiradorEmLeque)
            metal = {0.64f, 0.23f, 0.19f};
        else if (b.tipoInimigo == TipoInimigo::Canhoneiro)
            metal = {0.22f, 0.42f, 0.55f};

        retangulo(esquerda, base, b.largura, b.altura, corpo);
        retangulo(esquerda, base, b.largura, b.altura * 0.07, {corpo.r * 0.75f, corpo.g * 0.75f, corpo.b * 0.75f});
        retangulo(esquerda, base + b.altura * 0.93, b.largura, b.altura * 0.07,
             {std::min(1.0f, corpo.r + 0.08f), corpo.g + 0.10f, corpo.b + 0.08f});

        const double alturaLinha = std::min(1.3, b.altura / 30);
        for (int i = 0; i < 3; ++i)
        {
            const double fase = std::fmod(i / 3.0 + b.faseRolagem, 1.0);
            retangulo(esquerda, base + (b.altura - alturaLinha) * fase, b.largura, alturaLinha,
                 {corpo.r * 0.64f, corpo.g * 0.64f, corpo.b * 0.64f});
        }

        for (double deslocamento : {0.17, 0.75})
        {
            retangulo(esquerda + deslocamento * b.largura, base, b.largura * 0.08, b.altura, metal);
            retangulo(esquerda + deslocamento * b.largura, base + b.altura * 0.06, b.largura * 0.025, b.altura * 0.88,
                 {0.76f, 0.65f, 0.44f});
        }
        contornoRetangulo(b.limites(), escuro);

        if (b.ehChefe())
            detalhesChefe(b);
        else if (b.temInimigo)
            inimigoDoBarril(b);
        resistenciaDoBarril(b);
    }

    void Renderizador::detalhesChefe(const Barril& b) const
    {
        const double raio = b.inimigo.raio;
        const Cor dourado{0.90f, 0.70f, 0.30f};
        const Cor olhos = b.emFuria() ? Cor{1, 0.24f, 0.15f} : branco;

        disco(b.posicao.x, b.posicao.y, raio, escuro);
        disco(b.posicao.x, b.posicao.y, raio * 0.88, roxo);
        retangulo(b.posicao.x - raio * 0.88, b.posicao.y - raio * 0.12, raio * 1.76, raio * 0.48, escuro);
        retangulo(b.posicao.x - raio * 0.88, b.posicao.y + raio * 0.36, raio * 1.76, raio * 0.12, dourado);

        for (double lado : {-1.0, 1.0})
            retangulo(b.posicao.x + lado * raio * 0.42 - raio * 0.12, b.posicao.y,
                 raio * 0.24, raio * 0.14, olhos);

        if (b.tipo == TipoBarril::ChefeFinal)
            for (int i = -1; i <= 1; ++i)
                retangulo(b.posicao.x + i * raio * 0.50 - raio * 0.15, b.posicao.y + raio * 0.60,
                     raio * 0.30, raio * (i == 0 ? 0.65 : 0.45), dourado);

        for (double lado : {-1.0, 1.0})
        {
            const Vetor2 pontaArma = b.pontaCanhao(lado);
            const Cor canhao = b.emFuria() ? Cor{0.95f, 0.25f, 0.20f} : Cor{0.30f, 0.34f, 0.39f};

            retangulo(pontaArma.x - b.largura * 0.075, pontaArma.y, b.largura * 0.15, b.altura * 0.50, escuro);
            retangulo(pontaArma.x - b.largura * 0.06, pontaArma.y, b.largura * 0.12, b.altura * 0.48, canhao);
            retangulo(pontaArma.x - b.largura * 0.04, pontaArma.y + b.altura * 0.06,
                 b.largura * 0.025, b.altura * 0.36, {0.61f, 0.65f, 0.68f});
            retangulo(pontaArma.x - b.largura * 0.06, pontaArma.y, b.largura * 0.12, b.altura * 0.06, escuro);

            if (b.avisoAtaque > 0)
            {
                disco(pontaArma.x, pontaArma.y, 2 + raio * 0.3 * b.avisoAtaque, {1, 0.75f, 0.25f});
                disco(pontaArma.x, pontaArma.y, 1 + raio * 0.12 * b.avisoAtaque, branco);
            }
        }
    }

    void Renderizador::inimigoDoBarril(const Barril& b) const
    {
        if (b.tipoInimigo == TipoInimigo::Canhoneiro)
        {
            for (int i = 0; i < 8; ++i)
            {
                const Vetor2 direcao = direcaoFrente(i * 45.0 + b.indiceAtaque * 22.5);
                const Vetor2 saidaCanhao = b.posicao + Vetor2{direcao.x * b.largura * 0.40, direcao.y * b.altura * 0.40};
                const double raio = std::min(b.largura, b.altura) * 0.055;
                disco(saidaCanhao.x, saidaCanhao.y, raio, b.avisoAtaque > 0 ? Cor{1, 0.66f, 0.15f} : escuro);
            }
        }

        personagem(b.inimigo, true, b.tipoInimigo);

        if (b.avisoAtaque > 0)
        {
            const Vetor2 pontaArma = b.inimigo.pontaArma();
            disco(pontaArma.x, pontaArma.y, b.inimigo.raio * (0.15 + 0.25 * b.avisoAtaque), {1, 0.66f, 0.15f});
        }
    }

    void Renderizador::resistenciaDoBarril(const Barril& b) const
    {
        const double base = b.posicao.y - b.altura / 2;
        const std::string resistencia = std::to_string(b.resistencia);
        const double escala = ajustarTexto(resistencia, b.largura - 6 * unidadesPorPixelX, b.temInimigo ? 1.4 : 2.4);
        const double altura = alturaTexto(escala);

        double y = b.posicao.y - altura / 2 + 3 * unidadesPorPixelY;
        if (b.ehChefe())
            y = base + 5 * unidadesPorPixelY;
        else if (b.temInimigo)
            y = base + b.altura - altura + 2 * unidadesPorPixelY;

        retangulo(b.posicao.x - (larguraTexto(resistencia, escala) + 6 * unidadesPorPixelX) / 2, y - 4 * unidadesPorPixelY,
             larguraTexto(resistencia, escala) + 6 * unidadesPorPixelX, altura + 2 * unidadesPorPixelY, escuro);
        textoCentralizado(resistencia, b.posicao.x, y, escala, branco);
    }

    void Renderizador::painelInformacoes(const Jogo& jogo) const
    {
        const auto& c = jogo.configuracao();

        const std::string placar = "BARRIS DESTRUIDOS: " + std::to_string(jogo.destruidos());
        const double escala = ajustarTexto(placar, c.larguraArena - 16, 1.65);
        const double larguraCaixa = larguraTexto(placar, escala) + 12;

        retangulo(c.larguraArena - larguraCaixa, c.alturaArena - 25 * escala, larguraCaixa, 25 * escala, escuro);
        texto(placar, c.larguraArena - larguraCaixa + 6, c.alturaArena - 12 * escala, escala, branco);

        bool compacto = false;
        if (jogo.campanha())
        {
            const std::string fase = "FASE " + std::to_string(jogo.fase()) + " / " + std::to_string(Jogo::quantidadeFases);
            const double escalaFase = ajustarTexto(fase, c.larguraArena - 12, 1.7);
            compacto = c.larguraArena - larguraCaixa < larguraTexto(fase, escalaFase) + 12;
            const double posicaoYFase = c.alturaArena - (compacto ? 25 * escala + 17 : 12 * escala);

            retangulo(4, posicaoYFase - 3, larguraTexto(fase, escalaFase) + 4, 7 * escalaFase + 6, escuro);
            texto(fase, 6, posicaoYFase, escalaFase, jogo.ehFaseChefe() ? Cor{1.0f, 0.70f, 0.30f} : verde);
        }

#ifdef __EMSCRIPTEN__
        const std::string atalhos = "P PAUSA  R REINICIA";
#else
        const std::string atalhos = c.larguraArena / unidadesPorPixelX < 320
                                          ? "F1:AJUDA P:PAUSA R:RESET M:MENU"
                                          : "F1 AJUDA  P PAUSA  R REINICIA  M MENU";
#endif
        const double menor = ajustarTexto(atalhos, c.larguraArena - 10, 1.1);
        double proximoY = c.alturaArena - 25 * escala - (compacto ? 33 : 12);

        std::string progresso = std::string(jogo.campanha() ? "META DA FASE: " : "META: ") +
                               std::to_string(jogo.destruidosNaFase()) + " / " + std::to_string(jogo.metaFase());
        if (jogo.ehFaseChefe())
            progresso = jogo.fase() == Jogo::faseMinichefe ? "MINIBOSS: BARAO DO TAMBOR" : "BOSS FINAL: REI DA POLVORA";

        const double escalaProgresso = ajustarTexto(progresso, c.larguraArena - 12, 1.15);
        retangulo(4, proximoY - 2, larguraTexto(progresso, escalaProgresso) + 4, 7 * escalaProgresso + 4, escuro);
        texto(progresso, 6, proximoY, escalaProgresso, branco);
        proximoY -= 17;

        if (jogo.campanha())
        {
            const std::string vida = "JOGADOR: " + std::to_string(jogo.vidaJogador()) + " / " +
                                       std::to_string(Jogo::vidaMaximaJogador) +
                                       (jogo.invulneravel() ? "  PROTEGIDO" : "");
            const double escalaVida = ajustarTexto(vida, c.larguraArena - 12, 1.1);
            const double larguraBarra = std::min(180.0, c.larguraArena - 12.0);
            const Cor tonalidade = jogo.vidaJogador() <= 25 ? Cor{1, 0.35f, 0.30f} : verde;

            retangulo(4, proximoY - 12, std::max(larguraBarra, larguraTexto(vida, escalaVida)) + 4, 7 * escalaVida + 14, escuro);
            texto(vida, 6, proximoY, escalaVida, branco);
            retangulo(6, proximoY - 10, larguraBarra, 5, escuro);
            retangulo(6, proximoY - 10, larguraBarra * jogo.vidaJogador() / Jogo::vidaMaximaJogador, 5, tonalidade);
            proximoY -= 27;
        }

        if (const auto* chefe = jogo.chefe())
        {
            const std::string rotulo = std::string(chefe->emFuria() ? "CHEFE EM FURIA: " : "CHEFE: ") +
                                      std::to_string(chefe->resistencia) + " / " + std::to_string(chefe->resistenciaMaxima);
            const double escalaRotulo = ajustarTexto(rotulo, c.larguraArena - 16, 1.15);

            retangulo(5, proximoY - 11, c.larguraArena - 10, 25, escuro);
            texto(rotulo, 9, proximoY + 2, escalaRotulo, branco);
            retangulo(9, proximoY - 7, c.larguraArena - 18, 4, {0.25f, 0.17f, 0.19f});
            retangulo(9, proximoY - 7, (c.larguraArena - 18) * chefe->resistencia / chefe->resistenciaMaxima, 4,
                 chefe->emFuria() ? Cor{1.0f, 0.22f, 0.16f} : Cor{0.96f, 0.65f, 0.24f});
            proximoY -= 27;

            if (chefe->avisoAtaque > 0)
            {
                const std::string aviso = std::string("ALERTA: ") + chefe->nomeAtaque();
                const double s = ajustarTexto(aviso, c.larguraArena - 12, 1.1);

                retangulo(4, proximoY - 2, larguraTexto(aviso, s) + 4, 7 * s + 4, escuro);
                texto(aviso, 6, proximoY, s, {1, 0.70f, 0.30f});
                proximoY -= 15;
            }
        }

        retangulo(3, proximoY - 2, larguraTexto(atalhos, menor) + 4, 7 * menor + 4, escuro);
        texto(atalhos, 5, proximoY, menor, azulClaro);

        const auto& d = jogo.demonstracao();
        if (!d.movimentoBarris || !d.dispararInimigo || !d.criarNovosBarris || d.contornosColisao)
        {
            const std::string modo = std::string("F2:") + (d.movimentoBarris ? "ON" : "OFF") +
                                     " F3:" + (d.dispararInimigo ? "ON" : "OFF") + " F4:" + (d.criarNovosBarris ? "ON" : "OFF") +
                                     " F5:" + (d.contornosColisao ? "ON" : "OFF");
            const double s = ajustarTexto(modo, c.larguraArena - 10, 1.15);
            const double y = proximoY - 19;

            retangulo(0, y, c.larguraArena, 14, escuro);
            texto(modo, 5, y + 3, s, {1.0f, 0.80f, 0.34f});
        }

        if (d.cena != 0)
        {
            const std::string nome = jogo.nomeDemonstracao();
            const double s = ajustarTexto(nome, c.larguraArena - 10, 1.1);
            const double y = proximoY - 36;

            retangulo(0, y, c.larguraArena, 14, escuro);
            texto(nome, 5, y + 3, s, {1.0f, 0.80f, 0.34f});
        }
    }

    void Renderizador::menu(const Jogo& jogo) const
    {
        const double w = jogo.configuracao().larguraArena, h = jogo.configuracao().alturaArena;
        const double escala = std::min({2.0, (w - 12) / 200, (h - 20) / 210});
        const double esquerda = w / 2 - 92 * escala;
        const double topo = h / 2 + 100 * escala;

        retangulo(w / 2 - 100 * escala, h / 2 - 105 * escala, 200 * escala, 210 * escala, escuro);
        contornoRetangulo({{w / 2, h / 2}, 200 * escala, 210 * escala}, azulClaro);
        textoCentralizado("ATAQUE DE BARRIS", w / 2, topo - 20 * escala, 1.8 * escala, verde);

        texto("1  MODO DO TRABALHO", esquerda, topo - 55 * escala, 1.4 * escala, branco);
        texto("VENCA APOS " + std::to_string(jogo.configuracao().barrisParaVencer) + " BARRIS", esquerda, topo - 73 * escala, escala,
             azulClaro);
        texto("UM IMPACTO CAUSA DERROTA", esquerda, topo - 87 * escala, escala, azulClaro);

        retangulo(esquerda, topo - 102 * escala, 184 * escala, escala, azulClaro);

        texto("2  CAMPANHA", esquerda, topo - 121 * escala, 1.4 * escala, branco);
        texto("10 FASES, MINIBOSS E CHEFE", esquerda, topo - 139 * escala, escala, azulClaro);
        texto("100 DE VIDA, CURA POR FASE", esquerda, topo - 153 * escala, escala, azulClaro);

        textoCentralizado("1 OU 2 PARA COMECAR", w / 2, topo - 182 * escala, escala, branco);
        textoCentralizado("ESC PARA SAIR", w / 2, topo - 197 * escala, escala, azulClaro);
    }

    void Renderizador::painelSobreposto(const Jogo& jogo) const
    {
        const double w = jogo.configuracao().larguraArena, h = jogo.configuracao().alturaArena;

        if (jogo.ajudaVisivel())
        {
#ifdef __EMSCRIPTEN__
            static const std::array<std::string, 9> linhas = {
                "W/S    ANDAR FRENTE/TRAS", "A/D    GIRAR ESQUERDA/DIREITA",
                "MOUSE  ARMA: -30 A +30 GRAUS", "CLIQUE TIRO / SEGURAR CONTINUO",
                "P      PAUSAR/CONTINUAR", "R      REINICIAR CAMPANHA",
                "10 FASES, MINIBOSS E CHEFE", "100 DE VIDA, CURA POR FASE",
                "H      FECHAR AJUDA"};
#else
            static const std::array<std::string, 19> linhas = {
                "W/S    ANDAR FRENTE/TRAS",     "A/D    GIRAR ESQUERDA/DIREITA",
                "MOUSE  ARMA: -30 A +30 GRAUS", "CLIQUE TIRO / SEGURAR CONTINUO",
                "P      PAUSAR/CONTINUAR",      "R      REINICIAR SEM DEMO",
                "M      MENU DE MODOS",         "F2     MOVIMENTO DOS BARRIS",
                "F3     TIROS INIMIGOS",        "F4     NOVOS BARRIS",
                "F5     MOSTRAR COLISOES",      "F6     ARENA LIVRE E LIMITES",
                "F7     MIRA E RESISTENCIA",    "F8     OPONENTE (F2/F3 LIGAM)",
                "F9     COLISAO COM BARRIL",    "F10    DERROTA POR TIRO",
                "F11    ULTIMO BARRIL (N-1)",   "ESC    SAIR",
                "F1/H   FECHAR AJUDA"};
#endif

            const double escala = std::min({1.5, (w - 12) / 190, (h - 20) / 180});
            const double larguraCaixa = 190 * escala;
            double tamanhoFonte = escala;
            for (const auto& linha : linhas)
                tamanhoFonte = std::min(tamanhoFonte, ajustarTexto(linha, larguraCaixa - 14 * escala, escala));

            const double tamanhoTitulo = ajustarTexto("ATAQUE DE BARRIS", larguraCaixa - 14 * escala, 1.7 * escala);
            const double alturaLinha = std::max(9 * escala, alturaTexto(tamanhoFonte) + 2 * unidadesPorPixelY);
            const double alturaCaixa = alturaTexto(tamanhoTitulo) + linhas.size() * alturaLinha + 22 * escala;
            const double topo = (h + alturaCaixa) / 2;

            retangulo((w - larguraCaixa) / 2, (h - alturaCaixa) / 2, larguraCaixa, alturaCaixa, escuro);
            textoCentralizado("ATAQUE DE BARRIS", w / 2, topo - alturaTexto(tamanhoTitulo) - 6 * escala, tamanhoTitulo, verde);

            for (std::size_t i = 0; i < linhas.size(); ++i)
                texto(linhas[i], (w - larguraCaixa) / 2 + 7 * escala,
                     topo - alturaTexto(tamanhoTitulo) - 20 * escala - i * alturaLinha, tamanhoFonte, branco);
            return;
        }

        if (!jogo.pausado() && !jogo.emTransicao() && jogo.estado() == Estado::Jogando)
            return;

        const double escala = std::min(2.8, (w - 20) / 132);
        retangulo(w * 0.06, h / 2 - 29 * escala, w * 0.88, 66 * escala, escuro);

        const bool venceu = jogo.estado() == Estado::Vitoria;
        std::string titulo;
        std::string detalhe;
#ifdef __EMSCRIPTEN__
        std::string instrucao = "R REINICIA A CAMPANHA";
#else
        std::string instrucao = "R REINICIA  M MENU  ESC SAI";
#endif

        if (jogo.pausado())
        {
            titulo = "PAUSADO";
            detalhe = "P PARA CONTINUAR";
        }
        else if (jogo.emTransicao())
        {
            titulo = "FASE " + std::to_string(jogo.fase()) + " CONCLUIDA";
            const int proximaFase = jogo.fase() + 1;
            if (proximaFase == Jogo::faseMinichefe)
                detalhe = "MINIBOSS NA FASE 5";
            else if (proximaFase == Jogo::quantidadeFases)
                detalhe = "BOSS FINAL NA FASE 10";
            else
                detalhe = "PREPARE-SE PARA A FASE " + std::to_string(proximaFase);

            const int segundosRestantes = static_cast<int>(std::ceil(jogo.transicaoRestante()));
            instrucao = "CONTINUA EM " + std::to_string(segundosRestantes) + "...";
        }
        else if (venceu)
        {
            titulo = "VOCE GANHOU!";
            detalhe = jogo.campanha() ? "10 FASES CONCLUIDAS"
                                     : std::to_string(jogo.configuracao().barrisParaVencer) + " BARRIS DESTRUIDOS";
        }
        else
        {
            titulo = "VOCE PERDEU";
            detalhe = jogo.motivoDerrota() == MotivoDerrota::Tiro ? "ATINGIDO POR UM TIRO" : "COLISAO COM UM BARRIL";
        }

        const double escalaTitulo = ajustarTexto(titulo, w * 0.82, 2.8);
        textoCentralizado(titulo, w / 2, h / 2 + 14 * escala, escalaTitulo, venceu || jogo.emTransicao() ? verde : branco);

        const double menor = ajustarTexto(detalhe, w * 0.85, 1.25);
        textoCentralizado(detalhe, w / 2, h / 2 - 2 * escala, menor, azulClaro);

        const double escalaInstrucao = ajustarTexto(instrucao, w * 0.85, 1.2);
        textoCentralizado(instrucao, w / 2, h / 2 - 18 * escala, escalaInstrucao, branco);
    }

    void Renderizador::tiro(const Tiro& disparo) const
    {
        if (disparo.dono == DonoTiro::Inimigo)
        {
            disco(disparo.posicao.x, disparo.posicao.y, disparo.raio, {1.0f, 0.04f, 0.04f});
            disco(disparo.posicao.x, disparo.posicao.y, disparo.raio, {1.0f, 0.85f, 0.75f}, true);
        }
        else
            disco(disparo.posicao.x, disparo.posicao.y, disparo.raio, {1, 0.90f, 0.4f});
    }

    void Renderizador::contornosColisao(const Jogo& jogo) const
    {
        const auto contornoPersonagem = [](const Personagem& personagem, Cor tonalidade)
        {
            disco(personagem.posicao.x, personagem.posicao.y, personagem.raio, tonalidade, true);

            const Vetor2 ponta = personagem.pontaArma();
            const Vetor2 fim = ponta + direcaoFrente(personagem.angulo + personagem.anguloArma) * 75;
            definirCor(tonalidade);
            glBegin(GL_LINES);
            glVertex2d(ponta.x, ponta.y);
            glVertex2d(fim.x, fim.y);
            glEnd();
        };

        contornoPersonagem(jogo.jogador(), branco);

        for (const auto& barril : jogo.barris())
        {
            contornoRetangulo(barril.limites(), branco);

            if (barril.temInimigo)
                contornoPersonagem(barril.inimigo, {1.0f, 0.80f, 0.34f});
        }
    }

    void Renderizador::desenhar(const Jogo& jogo) const
    {
        const auto& c = jogo.configuracao();

        unidadesPorPixelX = static_cast<double>(c.larguraArena) / largura_;
        unidadesPorPixelY = static_cast<double>(c.alturaArena) / altura_;

        glClearColor(0.29f, 0.46f, 0.65f, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, c.larguraArena, 0, c.alturaArena, -1, 1);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glLineWidth(1);

        if (jogo.estado() == Estado::Menu)
        {
            menu(jogo);
            return;
        }

        definirCor(escuro);
        glBegin(GL_LINES);
        for (int x = 0; x < c.larguraArena; x += 20)
        {
            glVertex2d(x, c.alturaArena / 2.0);
            glVertex2d(std::min(x + 10, c.larguraArena), c.alturaArena / 2.0);
        }
        glEnd();

        for (const auto& objeto : jogo.barris())
            barril(objeto);

        personagem(jogo.jogador(), false);
        if (jogo.invulneravel())
            disco(jogo.jogador().posicao.x, jogo.jogador().posicao.y, jogo.jogador().raio + 3, {1, 0.80f, 0.35f}, true);

        for (const auto& disparo : jogo.tiros())
            tiro(disparo);

        if (jogo.demonstracao().contornosColisao)
            contornosColisao(jogo);

        painelInformacoes(jogo);
        painelSobreposto(jogo);
    }

    void Renderizador::salvarQuadro(const std::filesystem::path& caminho) const
    {
        std::vector<unsigned char> pixels(static_cast<std::size_t>(largura_) * altura_ * 3);
#ifndef __EMSCRIPTEN__
        glReadBuffer(GL_BACK);
#endif
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, largura_, altura_, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

        std::ofstream arquivo(caminho, std::ios::binary);
        arquivo << "P6\n" << largura_ << ' ' << altura_ << "\n255\n";

        for (int y = altura_ - 1; y >= 0; --y)
            arquivo.write(reinterpret_cast<const char*>(pixels.data() + static_cast<std::size_t>(y) * largura_ * 3),
                       largura_ * 3);

        if (!arquivo)
            throw std::runtime_error("Nao foi possivel gravar captura: " + caminho.string());
    }
}
