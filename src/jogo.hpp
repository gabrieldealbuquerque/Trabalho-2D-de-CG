#pragma once

#include "configuracao.hpp"
#include "geometria.hpp"
#include <array>
#include <random>
#include <vector>

namespace cg
{
    struct Personagem
    {
        Vetor2 posicao;
        double raio = 1;
        double angulo = 0;
        double anguloArma = 0;
        double fasePassada = 0;

        Vetor2 pivoArma() const
        {
            return posicao + girar({1.05 * raio, -0.35 * raio}, angulo);
        }

        Vetor2 pontaArma() const
        {
            return pivoArma() + direcaoFrente(angulo + anguloArma) * (2.8 * raio);
        }
    };

    enum class TipoBarril
    {
        Normal,
        Minichefe,
        ChefeFinal
    };

    enum class TipoInimigo
    {
        Atirador,
        AtiradorEmLeque,
        Canhoneiro
    };

    struct Barril
    {
        Vetor2 posicao;
        int resistencia = 1;
        int resistenciaMaxima = 1;
        double largura = 1;
        double altura = 1;
        TipoBarril tipo = TipoBarril::Normal;
        bool temInimigo = false;
        TipoInimigo tipoInimigo = TipoInimigo::Atirador;
        Personagem inimigo;

        double tempoTiro = 0;
        double faseRolagem = 0;
        double tempoMovimento = 0;
        double avisoAtaque = 0;
        int indiceAtaque = 0;

        bool ehChefe() const
        {
            return tipo == TipoBarril::Minichefe || tipo == TipoBarril::ChefeFinal;
        }

        bool emFuria() const
        {
            return tipo == TipoBarril::ChefeFinal && resistencia <= resistenciaMaxima / 2;
        }

        Retangulo limites() const
        {
            return {posicao, largura, altura};
        }

        Vetor2 pontaCanhao(double lado) const
        {
            return posicao + Vetor2{lado * largura * 0.38, -altura * 0.42};
        }

        int padraoAtaque() const
        {
            const int quantidadePadroes = tipo == TipoBarril::Minichefe ? 2 : 3;
            return indiceAtaque % quantidadePadroes;
        }

        const char* nome() const
        {
            switch (tipo)
            {
                case TipoBarril::Minichefe:
                    return "BARAO DO TAMBOR";
                case TipoBarril::ChefeFinal:
                    return "REI DA POLVORA";
                default:
                    return "BARRIL";
            }
        }

        const char* nomeAtaque() const
        {
            switch (padraoAtaque())
            {
                case 0:
                    return "LEQUE";
                case 1:
                    return "ANEL COM ABERTURA";
                default:
                    return "RAJADA DUPLA";
            }
        }
    };

    enum class DonoTiro
    {
        Jogador,
        Inimigo
    };

    struct Tiro
    {
        Vetor2 posicao;
        Vetor2 velocidade;
        double raio = 2;
        DonoTiro dono = DonoTiro::Jogador;
        bool ativo = true;
        bool tiroChefe = false;
    };

    enum class ModoJogo
    {
        Trabalho,
        Campanha
    };

    enum class Estado
    {
        Jogando,
        Vitoria,
        Derrota,
        Menu
    };

    enum class MotivoDerrota
    {
        Nenhum,
        Barril,
        Tiro
    };

    struct OpcoesDemonstracao
    {
        bool movimentoBarris = true;
        bool dispararInimigo = true;
        bool criarNovosBarris = true;
        bool contornosColisao = false;
        int cena = 0;
    };

    class Jogo
    {
      public:
        static constexpr double passoTempo = 1.0 / 240.0;
        static constexpr double limiteArma = 30;

        static constexpr int quantidadeFases = 10;
        static constexpr int faseMinichefe = 5;
        static constexpr int resistenciaMinichefe = 150;
        static constexpr double intervaloFases = 2.0;

        static constexpr int vidaMaximaJogador = 100;
        static constexpr int danoTiro = 20;
        static constexpr int danoContato = 25;
        static constexpr int curaEntreFases = 20;
        static constexpr double duracaoProtecao = 1.0;

        explicit Jogo(Configuracao configuracao);
        void selecionarModo(ModoJogo modo);
        void mostrarMenu();
        void reiniciar();
        void avancar(double segundos);

        void definirTecla(unsigned char tecla, bool pressionada);
        void limparEntradas();
        void definirDisparo(bool pressionada);
        void mirar(double deslocamentoHorizontal);
        void atirar();

        void alternarPausa();
        void alternarAjuda();
        void alternarDemonstracao(int opcao);
        void carregarDemonstracao(int opcao);
        const char* nomeDemonstracao() const;

        const Configuracao& configuracao() const
        {
            return configuracao_;
        }

        const Personagem& jogador() const
        {
            return jogador_;
        }

        const std::vector<Barril>& barris() const
        {
            return barris_;
        }

        const std::vector<Tiro>& tiros() const
        {
            return tiros_;
        }

        const OpcoesDemonstracao& demonstracao() const
        {
            return demonstracao_;
        }
        ModoJogo modo() const
        {
            return modo_;
        }

        bool campanha() const
        {
            return modo_ == ModoJogo::Campanha;
        }

        int vidaJogador() const
        {
            return vidaJogador_;
        }

        bool invulneravel() const
        {
            return protecaoRestante_ > 0;
        }
        Estado estado() const
        {
            return estado_;
        }
        MotivoDerrota motivoDerrota() const
        {
            return motivoDerrota_;
        }

        int destruidos() const
        {
            return destruidos_;
        }

        int fase() const
        {
            return fase_;
        }

        int destruidosNaFase() const
        {
            return destruidosNaFase_;
        }

        int metaFase() const;

        bool ehFaseChefe() const
        {
            return campanha() && (fase_ == faseMinichefe || fase_ == quantidadeFases);
        }

        bool emTransicao() const
        {
            return transicaoRestante_ > 0;
        }

        double transicaoRestante() const
        {
            return transicaoRestante_;
        }

        double velocidadeBarril() const;
        double intervaloBarris() const;
        const Barril* chefe() const;

        bool pausado() const
        {
            return pausado_;
        }

        bool ajudaVisivel() const
        {
            return ajudaVisivel_;
        }

        bool ativo() const
        {
            return emExecucao() && !emTransicao();
        }

        double tempoDecorrido() const
        {
            return tempoDecorrido_;
        }

      private:
        friend struct AcessoTesteJogo;

        enum class TipoColisao
        {
            TiroJogador,
            TiroInimigo,
            Contato
        };

        struct Colisao
        {
            double tempo;
            TipoColisao tipo;
            std::size_t indiceTiro;
            std::size_t indiceBarril;
        };

        void passo(double dt);
        void moverJogador(double dt);
        void atualizarDisparos(double dt);
        void atualizarCriacaoBarris(double dt);
        std::vector<Vetor2> atualizarBarris(double dt);
        std::vector<Colisao> detectarColisoes(double dt, Vetor2 jogadorAnterior, const std::vector<Vetor2>& barrisAnteriores);
        bool aplicarColisoes(const std::vector<Colisao>& colisoes);
        void removerObjetosFinalizados();

        void adicionarTiro(Vetor2 origem, double angulo, double velocidade, double raio, DonoTiro dono, bool tiroChefe = false);

        void iniciarFase(int fase);
        void concluirFase();
        void criarBarril();
        void criarChefe();
        void moverBarril(Barril& barril, double dt);

        void dispararInimigo(Barril& barril, double dt);
        void dispararInimigoEspecial(Barril& barril, double dt);
        void ataqueInimigoEspecial(Barril& barril);
        void dispararChefe(Barril& barril, double dt);
        void ataqueChefe(Barril& barril);

        void mirarInimigo(Barril& barril);
        void causarDanoJogador(MotivoDerrota motivo);

        bool emExecucao() const
        {
            return estado_ == Estado::Jogando && !pausado_ && !ajudaVisivel_;
        }

        void finalizar(Estado estado, MotivoDerrota motivo = MotivoDerrota::Nenhum);

        Configuracao configuracao_;
        std::uint32_t sementeInicial_;
        std::mt19937 aleatorio_;

        Personagem jogador_;
        std::vector<Barril> barris_;
        std::vector<Tiro> tiros_;

        std::array<bool, 256> teclas_{};
        OpcoesDemonstracao demonstracao_;

        ModoJogo modo_ = ModoJogo::Trabalho;
        Estado estado_ = Estado::Jogando;
        MotivoDerrota motivoDerrota_ = MotivoDerrota::Nenhum;

        int destruidos_ = 0;
        int fase_ = 1;
        int destruidosNaFase_ = 0;
        int vidaJogador_ = vidaMaximaJogador;

        bool pausado_ = false;
        bool ajudaVisivel_ = false;
        bool atirando_ = false;

        double contagemTiro_ = 0;
        double acumulador_ = 0;
        double tempoCriacaoBarris_ = 0;
        double tempoDecorrido_ = 0;
        double transicaoRestante_ = 0;
        double protecaoRestante_ = 0;
    };
}
