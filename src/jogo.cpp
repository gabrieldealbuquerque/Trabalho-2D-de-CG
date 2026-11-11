#include "jogo.hpp"

#include <cctype>
#include <stdexcept>

namespace cg
{
    namespace
    {
        constexpr double toleranciaTempo = 1e-12;
        constexpr double ritmoPassadaInimigo = 8;
        constexpr double aumentoVelocidadeBarris = 0.08;
        constexpr double aumentoFrequenciaBarris = 0.10;
        constexpr int barrisExtrasPorFase = 2;
        constexpr double atrasoReforcosMinichefe = 5;
    }

    Jogo::Jogo(Configuracao configuracao)
        : configuracao_(configuracao), sementeInicial_(configuracao.sementeFixa ? configuracao.semente : std::random_device{}()), aleatorio_(sementeInicial_)
    {
        configuracao_.validar();
        reiniciar();
    }

    void Jogo::selecionarModo(ModoJogo modo)
    {
        modo_ = modo;
        reiniciar();
    }

    void Jogo::mostrarMenu()
    {
        estado_ = Estado::Menu;
        pausado_ = false;
        ajudaVisivel_ = false;

        acumulador_ = 0;
        transicaoRestante_ = 0;
        protecaoRestante_ = 0;

        barris_.clear();
        tiros_.clear();
        limparEntradas();
    }

    void Jogo::reiniciar()
    {
        aleatorio_.seed(sementeInicial_);
        demonstracao_ = {};

        estado_ = Estado::Jogando;
        motivoDerrota_ = MotivoDerrota::Nenhum;
        destruidos_ = 0;

        pausado_ = false;
        ajudaVisivel_ = false;

        acumulador_ = 0;
        tempoDecorrido_ = 0;

        vidaJogador_ = vidaMaximaJogador;

        iniciarFase(1);
    }

    void Jogo::iniciarFase(int fase)
    {
        fase_ = fase;
        destruidosNaFase_ = 0;
        transicaoRestante_ = 0;
        tempoCriacaoBarris_ = 0;
        protecaoRestante_ = 0;

        if (campanha() && fase > 1)
            vidaJogador_ = std::min(vidaMaximaJogador, vidaJogador_ + curaEntreFases);

        jogador_ = {{configuracao_.larguraArena / 2.0,
                    std::min(2.0 * configuracao_.raioJogador, configuracao_.alturaArena / 2.0 - configuracao_.raioJogador)},
                   configuracao_.raioJogador,
                   0,
                   0,
                   0};

        barris_.clear();
        tiros_.clear();
        limparEntradas();

        if (ehFaseChefe())
            criarChefe();
        else
            criarBarril();
    }

    void Jogo::concluirFase()
    {
        barris_.clear();
        tiros_.clear();
        limparEntradas();

        if (!campanha() || fase_ == quantidadeFases)
            finalizar(Estado::Vitoria);
        else
            transicaoRestante_ = intervaloFases;
    }

    const Barril* Jogo::chefe() const
    {
        for (const auto& barril : barris_)
            if (barril.ehChefe())
                return &barril;

        return nullptr;
    }

    int Jogo::metaFase() const
    {
        if (!campanha())
            return configuracao_.barrisParaVencer;

        if (ehFaseChefe())
            return 1;

        return configuracao_.barrisParaVencer + barrisExtrasPorFase * (fase_ - 1);
    }

    double Jogo::velocidadeBarril() const
    {
        if (!campanha())
            return configuracao_.velocidadeBarril;

        return configuracao_.velocidadeBarril * (1 + aumentoVelocidadeBarris * (fase_ - 1));
    }

    double Jogo::intervaloBarris() const
    {
        if (!campanha())
            return configuracao_.intervaloBarris;

        double intervalo = configuracao_.intervaloBarris;
        if (fase_ == faseMinichefe)
            intervalo *= atrasoReforcosMinichefe;

        return intervalo / (1 + aumentoFrequenciaBarris * (fase_ - 1));
    }

    void Jogo::definirTecla(unsigned char tecla, bool pressionada)
    {
        teclas_[static_cast<unsigned char>(std::tolower(tecla))] = pressionada;
    }

    void Jogo::limparEntradas()
    {
        teclas_.fill(false);
        atirando_ = false;
        contagemTiro_ = 0;
    }

    void Jogo::definirDisparo(bool pressionada)
    {
        if (!pressionada)
        {
            atirando_ = false;
            contagemTiro_ = 0;
        }
        else if (ativo() && !atirando_)
        {
            atirando_ = true;
            contagemTiro_ = 1 / configuracao_.tirosJogadorPorSegundo;

            atirar();
        }
    }

    void Jogo::atualizarDisparos(double dt)
    {
        if (!atirando_)
            return;

        contagemTiro_ -= dt;
        while (contagemTiro_ <= toleranciaTempo)
        {
            atirar();
            contagemTiro_ += 1 / configuracao_.tirosJogadorPorSegundo;
        }
    }

    void Jogo::mirar(double deslocamentoHorizontal)
    {
        if (!ativo() || !std::isfinite(deslocamentoHorizontal))
            return;

        const double novoAngulo = jogador_.anguloArma - deslocamentoHorizontal * configuracao_.sensibilidadeMouse;
        jogador_.anguloArma = std::clamp(novoAngulo, -limiteArma, limiteArma);
    }

    void Jogo::adicionarTiro(Vetor2 origem, double angulo, double velocidade, double raio, DonoTiro dono, bool tiroChefe)
    {
        Tiro tiro;
        tiro.posicao = origem;
        tiro.velocidade = direcaoFrente(angulo) * velocidade;
        tiro.raio = raio;
        tiro.dono = dono;
        tiro.tiroChefe = tiroChefe;

        tiros_.push_back(tiro);
    }

    void Jogo::atirar()
    {
        if (!ativo())
            return;

        const double raio = std::max(1.0, jogador_.raio * 0.12);
        adicionarTiro(jogador_.pontaArma(), jogador_.angulo + jogador_.anguloArma, configuracao_.velocidadeTiroJogador, raio, DonoTiro::Jogador);
    }

    void Jogo::alternarPausa()
    {
        if (estado_ != Estado::Jogando)
            return;

        pausado_ = !pausado_;
        acumulador_ = 0;
        limparEntradas();
    }

    void Jogo::alternarAjuda()
    {
        if (estado_ == Estado::Menu)
            return;

        ajudaVisivel_ = !ajudaVisivel_;
        acumulador_ = 0;
        limparEntradas();
    }

    void Jogo::alternarDemonstracao(int opcao)
    {
        if (estado_ == Estado::Menu)
            return;

        switch (opcao)
        {
            case 2:
                demonstracao_.movimentoBarris = !demonstracao_.movimentoBarris;
                break;
            case 3:
                demonstracao_.dispararInimigo = !demonstracao_.dispararInimigo;
                break;
            case 4:
                demonstracao_.criarNovosBarris = !demonstracao_.criarNovosBarris;
                break;
            case 5:
                demonstracao_.contornosColisao = !demonstracao_.contornosColisao;
                break;
        }
    }

    void Jogo::carregarDemonstracao(int opcao)
    {
        if (estado_ == Estado::Menu || opcao < 6 || opcao > 11)
            return;

        selecionarModo(ModoJogo::Trabalho);
        barris_.clear();
        demonstracao_ = {false, false, false, false, opcao};
        jogador_.posicao.y = std::clamp(configuracao_.alturaArena * 0.25, jogador_.raio,
                                       configuracao_.alturaArena / 2.0 - jogador_.raio);

        if (opcao == 6)
        {
            demonstracao_.contornosColisao = true;
            return;
        }

        Barril barril;
        barril.largura = configuracao_.larguraBarril;
        barril.altura = configuracao_.alturaBarril;
        barril.resistencia = barril.resistenciaMaxima = configuracao_.resistenciaBarril;
        barril.temInimigo = opcao == 8 || opcao == 10;
        barril.posicao = {jogador_.pontaArma().x, configuracao_.alturaArena * 0.68};

        if (barril.temInimigo)
            barril.posicao.x = jogador_.posicao.x + 1.05 * configuracao_.raioInimigo;

        if (opcao == 9)
        {
            demonstracao_.movimentoBarris = true;
            demonstracao_.contornosColisao = true;
            barril.posicao.x = jogador_.posicao.x;
            barril.posicao.y = jogador_.posicao.y + jogador_.raio + barril.altura / 2 +
                                std::max(20.0, configuracao_.velocidadeBarril * 0.8);
        }
        else if (opcao == 10)
        {
            demonstracao_.dispararInimigo = true;
            barril.posicao.y = jogador_.posicao.y +
                                std::max(jogador_.raio + barril.altura / 2 + 20,
                                         2.45 * configuracao_.raioInimigo + jogador_.raio + configuracao_.velocidadeTiroInimigo * 0.45);
        }
        else if (opcao == 11)
        {
            destruidos_ = configuracao_.barrisParaVencer - 1;
            destruidosNaFase_ = destruidos_;
        }

        const double margemX = std::max(barril.largura / 2, barril.temInimigo ? configuracao_.raioInimigo : 0.0);
        const double margemY = std::max(barril.altura / 2, barril.temInimigo ? configuracao_.raioInimigo : 0.0);
        barril.posicao.x = std::clamp(barril.posicao.x, margemX, configuracao_.larguraArena - margemX);
        barril.posicao.y = std::clamp(barril.posicao.y, margemY, configuracao_.alturaArena - margemY);
        barril.inimigo = {barril.posicao, configuracao_.raioInimigo, 180, 0, 0};

        mirarInimigo(barril);
        barris_.push_back(barril);
    }

    const char* Jogo::nomeDemonstracao() const
    {
        switch (demonstracao_.cena)
        {
            case 6:
                return "DEMO F6: MOVIMENTO E LIMITES";
            case 7:
                return "DEMO F7: MIRA E RESISTENCIA";
            case 8:
                return "DEMO F8: OPONENTE";
            case 9:
                return "DEMO F9: COLISAO COM BARRIL";
            case 10:
                return "DEMO F10: DERROTA POR TIRO";
            case 11:
                return "DEMO F11: INICIO COM N-1";
            default:
                return "";
        }
    }

    void Jogo::avancar(double segundos)
    {
        if (!std::isfinite(segundos) || segundos < 0)
            throw std::invalid_argument("Tempo deve ser finito e nao negativo.");

        if (!emExecucao())
            return;

        acumulador_ += segundos;
        while (acumulador_ + toleranciaTempo >= passoTempo && emExecucao())
        {
            acumulador_ -= passoTempo;
            passo(passoTempo);
        }

        acumulador_ = std::max(0.0, acumulador_);
    }

    void Jogo::moverJogador(double dt)
    {
        const int direcaoGiro = teclas_['a'] - teclas_['d'];
        const int direcaoPasso = teclas_['w'] - teclas_['s'];

        const double anguloAnterior = radianos(jogador_.angulo);
        const double velocidadeAngular = radianos(direcaoGiro * configuracao_.velocidadeGiro);
        const double velocidade = direcaoPasso * configuracao_.velocidadeJogador;

        Vetor2 delta;
        if (direcaoGiro != 0)
        {
            const double proximoAngulo = anguloAnterior + velocidadeAngular * dt;
            const double raioGiro = velocidade / velocidadeAngular;
            delta.x = raioGiro * (std::cos(proximoAngulo) - std::cos(anguloAnterior));
            delta.y = raioGiro * (std::sin(proximoAngulo) - std::sin(anguloAnterior));
        }
        else
        {
            delta = direcaoFrente(jogador_.angulo) * (velocidade * dt);
        }

        jogador_.angulo = normalizarAngulo(jogador_.angulo + direcaoGiro * configuracao_.velocidadeGiro * dt);
        const Vetor2 anterior = jogador_.posicao;
        jogador_.posicao.x = std::clamp(anterior.x + delta.x, jogador_.raio, configuracao_.larguraArena - jogador_.raio);
        jogador_.posicao.y = std::clamp(anterior.y + delta.y, jogador_.raio, configuracao_.alturaArena / 2.0 - jogador_.raio);

        const double distancia = comprimento(jogador_.posicao - anterior);
        if (distancia > 1e-10)
            jogador_.fasePassada = std::fmod(jogador_.fasePassada + direcaoPasso * distancia / jogador_.raio, 2 * pi);
    }

    void Jogo::criarBarril()
    {
        const bool inimigo = std::bernoulli_distribution(0.5)(aleatorio_);
        const double margem = std::max(configuracao_.larguraBarril / 2, inimigo ? configuracao_.raioInimigo : 0.0);
        const double x = std::uniform_real_distribution<double>(margem, configuracao_.larguraArena - margem)(aleatorio_);
        const double y = configuracao_.alturaArena - std::max(configuracao_.alturaBarril / 2, inimigo ? configuracao_.raioInimigo : 0.0);

        Barril barril;
        barril.posicao = {x, y};
        barril.resistenciaMaxima = configuracao_.resistenciaBarril + (campanha() ? (fase_ - 1) / 3 : 0);
        barril.largura = configuracao_.larguraBarril;
        barril.altura = configuracao_.alturaBarril;
        barril.temInimigo = inimigo;

        if (inimigo && campanha() && fase_ > faseMinichefe)
        {
            const int variante = std::uniform_int_distribution<int>(0, 3)(aleatorio_);
            if (variante == 2)
            {
                barril.tipoInimigo = TipoInimigo::AtiradorEmLeque;
                barril.resistenciaMaxima += 1;
            }
            else if (variante == 3)
            {
                barril.tipoInimigo = TipoInimigo::Canhoneiro;
                barril.resistenciaMaxima += 3;
            }
        }

        barril.resistencia = barril.resistenciaMaxima;
        barril.inimigo = {barril.posicao, configuracao_.raioInimigo, 180, 0, 0};

        mirarInimigo(barril);
        barris_.push_back(barril);
    }

    void Jogo::criarChefe()
    {
        const bool chefeFinal = fase_ == quantidadeFases;

        Barril barril;
        barril.tipo = chefeFinal ? TipoBarril::ChefeFinal : TipoBarril::Minichefe;
        barril.largura = std::min(configuracao_.larguraBarril * (chefeFinal ? 3.2 : 2.4), configuracao_.larguraArena * 0.70);
        barril.altura = std::min(configuracao_.alturaBarril * (chefeFinal ? 1.9 : 1.6), configuracao_.alturaArena * 0.18);
        barril.posicao = {configuracao_.larguraArena / 2.0, configuracao_.alturaArena * 0.76};

        barril.resistenciaMaxima = resistenciaMinichefe;
        if (chefeFinal)
            barril.resistenciaMaxima = configuracao_.resistenciaBarril * 10;
        barril.resistencia = barril.resistenciaMaxima;

        barril.temInimigo = true;
        const double raio =
            std::min({configuracao_.raioInimigo * (chefeFinal ? 1.8 : 1.4), barril.largura * 0.23, barril.altura * 0.28});
        barril.inimigo = {barril.posicao, raio, 180, 0, 0};

        mirarInimigo(barril);
        barris_.push_back(barril);
    }

    void Jogo::moverBarril(Barril& barril, double dt)
    {
        if (!demonstracao_.movimentoBarris)
            return;

        if (barril.ehChefe())
        {
            const double amplitude = std::max(0.0, (configuracao_.larguraArena - barril.largura) / 2 - 4);
            const double velocidade = configuracao_.velocidadeBarril * (barril.tipo == TipoBarril::ChefeFinal ? 1.6 : 1.2);

            barril.tempoMovimento += dt;
            const double fase = barril.tempoMovimento * velocidade / std::max(1.0, amplitude);
            barril.posicao.x = configuracao_.larguraArena / 2.0 + amplitude * std::sin(fase);
        }
        else
        {
            const double distancia = velocidadeBarril() * dt;
            barril.posicao.y -= distancia;
        }

        barril.faseRolagem = std::fmod(barril.faseRolagem + ritmoPassadaInimigo * dt / (3 * pi), 1.0);
    }

    void Jogo::dispararInimigo(Barril& barril, double dt)
    {
        if (barril.ehChefe())
        {
            dispararChefe(barril, dt);
            return;
        }

        if (barril.tipoInimigo != TipoInimigo::Atirador)
        {
            dispararInimigoEspecial(barril, dt);
            return;
        }

        if (!demonstracao_.dispararInimigo || configuracao_.tirosInimigoPorSegundo <= 0)
            return;

        const double multiplicador = campanha() ? 1 + 0.06 * (fase_ - 1) : 1;
        const double intervalo = 1 / (configuracao_.tirosInimigoPorSegundo * multiplicador);

        barril.tempoTiro += dt;
        while (barril.tempoTiro + toleranciaTempo >= intervalo)
        {
            barril.tempoTiro -= intervalo;
            const double angulo = barril.inimigo.angulo + barril.inimigo.anguloArma;
            const double raio = std::max(1.0, barril.inimigo.raio * 0.12);

            adicionarTiro(barril.inimigo.pontaArma(), angulo, configuracao_.velocidadeTiroInimigo, raio, DonoTiro::Inimigo);
        }
    }

    void Jogo::dispararInimigoEspecial(Barril& barril, double dt)
    {
        if (!demonstracao_.dispararInimigo || configuracao_.tirosInimigoPorSegundo <= 0)
        {
            barril.avisoAtaque = 0;
            barril.tempoTiro = 0;
            return;
        }

        const bool leque = barril.tipoInimigo == TipoInimigo::AtiradorEmLeque;
        const double multiplicador = (leque ? 0.85 : 0.55) * (1 + 0.06 * (fase_ - 6));
        const double intervalo = std::max(leque ? 0.9 : 1.5, 1 / (configuracao_.tirosInimigoPorSegundo * multiplicador));
        const double aviso = 0.5;

        barril.tempoTiro += dt;
        while (barril.tempoTiro + toleranciaTempo >= intervalo)
        {
            barril.tempoTiro -= intervalo;
            ataqueInimigoEspecial(barril);
            ++barril.indiceAtaque;
        }

        barril.avisoAtaque = std::clamp((barril.tempoTiro - intervalo + aviso) / aviso, 0.0, 1.0);
    }

    void Jogo::ataqueInimigoEspecial(Barril& barril)
    {
        if (barril.tipoInimigo == TipoInimigo::AtiradorEmLeque)
        {
            const Vetor2 origem = barril.inimigo.pontaArma();
            const double anguloMira = barril.inimigo.angulo + barril.inimigo.anguloArma;
            const double raio = std::clamp(barril.inimigo.raio * 0.15, 2.0, 5.0);

            for (int disparo = -1; disparo <= 1; ++disparo)
                adicionarTiro(origem, anguloMira + disparo * 14, configuracao_.velocidadeTiroInimigo * 1.1, raio, DonoTiro::Inimigo);

            return;
        }

        const int quantidadeTiros = 8;
        const double raio = std::clamp(barril.inimigo.raio * 0.24, 2.0, 5.0);

        for (int disparo = 0; disparo < quantidadeTiros; ++disparo)
        {
            const double angulo = disparo * 360.0 / quantidadeTiros + barril.indiceAtaque * 22.5;
            const Vetor2 direcao = direcaoFrente(angulo);
            const Vetor2 origem =
                barril.posicao + Vetor2{direcao.x * barril.largura * 0.40, direcao.y * barril.altura * 0.40};

            adicionarTiro(origem, angulo, configuracao_.velocidadeTiroInimigo * 0.8, raio, DonoTiro::Inimigo);
        }
    }

    void Jogo::dispararChefe(Barril& barril, double dt)
    {
        if (!demonstracao_.dispararInimigo || configuracao_.tirosInimigoPorSegundo <= 0)
        {
            barril.avisoAtaque = 0;
            barril.tempoTiro = 0;
            return;
        }

        const bool minichefe = barril.tipo == TipoBarril::Minichefe;
        double multiplicador = 0.85;
        if (minichefe)
            multiplicador = 1.30;
        else if (barril.emFuria())
            multiplicador = 1.1;

        const double intervalo = std::max(minichefe ? 0.6 : 1.2, 1 / (configuracao_.tirosInimigoPorSegundo * multiplicador));
        const double aviso = std::min(0.7, intervalo * 0.45);

        barril.tempoTiro += dt;
        while (barril.tempoTiro + toleranciaTempo >= intervalo)
        {
            barril.tempoTiro -= intervalo;
            ataqueChefe(barril);
            ++barril.indiceAtaque;
        }

        barril.avisoAtaque = std::clamp((barril.tempoTiro - intervalo + aviso) / aviso, 0.0, 1.0);
    }

    void Jogo::ataqueChefe(Barril& barril)
    {
        const bool chefeFinal = barril.tipo == TipoBarril::ChefeFinal;
        const bool emFuria = barril.emFuria();

        double multiplicadorVelocidade = 0.85;
        if (!chefeFinal)
            multiplicadorVelocidade = 1.15;
        else if (emFuria)
            multiplicadorVelocidade = 1.1;

        const double velocidade = configuracao_.velocidadeTiroInimigo * multiplicadorVelocidade;
        const double raio = std::clamp(configuracao_.raioJogador * 0.22, 2.0, 5.0);

        if (barril.padraoAtaque() == 0)
        {
            const Vetor2 origem = barril.pontaCanhao(0);
            int quantidade = 5;
            double leque = 26;
            if (!chefeFinal)
            {
                quantidade = 3;
                leque = 18;
            }
            else if (emFuria)
            {
                quantidade = 7;
                leque = 32;
            }

            const double anguloMira = anguloPara(origem, jogador_.posicao);
            for (int disparo = 0; disparo < quantidade; ++disparo)
            {
                const double angulo = anguloMira - leque + 2 * leque * disparo / (quantidade - 1);
                adicionarTiro(origem, angulo, velocidade, raio, DonoTiro::Inimigo, true);
            }
        }
        else if (barril.padraoAtaque() == 1)
        {
            int quantidade = 18;
            if (!chefeFinal)
                quantidade = 12;
            else if (emFuria)
                quantidade = 22;

            const double abertura = 180 + 45 * std::sin(barril.indiceAtaque * 1.1);
            for (int i = 0; i < quantidade; ++i)
            {
                const double angulo = 360.0 * i / quantidade + barril.indiceAtaque * 11;
                if (std::abs(normalizarAngulo(angulo - abertura)) < 30)
                    continue;

                const Vetor2 origem = barril.posicao + direcaoFrente(angulo) * (std::min(barril.largura, barril.altura) * 0.4);
                adicionarTiro(origem, angulo, velocidade * 0.82, raio, DonoTiro::Inimigo, true);
            }
        }
        else
        {
            const int quantidade = emFuria ? 5 : 4;
            for (double lado : {-1.0, 1.0})
            {
                const Vetor2 origem = barril.pontaCanhao(lado);

                const double anguloMira = anguloPara(origem, jogador_.posicao);
                for (int disparo = 0; disparo < quantidade; ++disparo)
                {
                    const double angulo = anguloMira + lado * (disparo * 11 - 16);
                    adicionarTiro(origem, angulo, velocidade * (0.85 + 0.05 * disparo), raio, DonoTiro::Inimigo, true);
                }
            }
        }
    }

    void Jogo::mirarInimigo(Barril& barril)
    {
        const double desejado = anguloPara(barril.inimigo.pivoArma(), jogador_.posicao);
        barril.inimigo.anguloArma = std::clamp(normalizarAngulo(desejado - barril.inimigo.angulo), -limiteArma, limiteArma);
    }

    void Jogo::finalizar(Estado estado, MotivoDerrota motivo)
    {
        estado_ = estado;
        motivoDerrota_ = motivo;
        limparEntradas();
    }

    void Jogo::causarDanoJogador(MotivoDerrota motivo)
    {
        if (!ativo() || invulneravel())
            return;

        if (!campanha())
        {
            vidaJogador_ = 0;
        }
        else
        {
            const int dano = motivo == MotivoDerrota::Tiro ? danoTiro : danoContato;
            vidaJogador_ = std::max(0, vidaJogador_ - dano);
        }

        if (vidaJogador_ == 0)
            finalizar(Estado::Derrota, motivo);
        else
            protecaoRestante_ = duracaoProtecao;
    }

    void Jogo::atualizarCriacaoBarris(double dt)
    {
        if (!demonstracao_.criarNovosBarris)
            return;

        const double intervalo = intervaloBarris();
        tempoCriacaoBarris_ += dt;
        while (tempoCriacaoBarris_ + toleranciaTempo >= intervalo)
        {
            tempoCriacaoBarris_ -= intervalo;
            criarBarril();
        }
    }

    std::vector<Vetor2> Jogo::atualizarBarris(double dt)
    {
        std::vector<Vetor2> barrisAnteriores;
        barrisAnteriores.reserve(barris_.size());

        for (auto& barril : barris_)
        {
            barrisAnteriores.push_back(barril.posicao);
            moverBarril(barril, dt);

            if (barril.temInimigo)
            {
                barril.inimigo.posicao = barril.posicao;
                barril.inimigo.fasePassada = std::fmod(barril.inimigo.fasePassada + ritmoPassadaInimigo * dt, 2 * pi);
                mirarInimigo(barril);
                dispararInimigo(barril, dt);
            }
        }

        return barrisAnteriores;
    }

    std::vector<Jogo::Colisao> Jogo::detectarColisoes(double dt, Vetor2 jogadorAnterior,

                                                      const std::vector<Vetor2>& barrisAnteriores)
    {
        std::vector<Colisao> colisoes;

        for (std::size_t i = 0; i < tiros_.size(); ++i)
        {
            auto& tiro = tiros_[i];
            const Vetor2 anterior = tiro.posicao;
            tiro.posicao = anterior + tiro.velocidade * dt;

            if (tiro.dono == DonoTiro::Jogador)
            {
                for (std::size_t j = 0; j < barris_.size(); ++j)
                {
                    const auto& barril = barris_[j];
                    const Vetor2 inicioRelativo = anterior - barrisAnteriores[j];
                    const Vetor2 fimRelativo = tiro.posicao - barril.posicao;
                    const Retangulo limites{{0, 0}, barril.largura, barril.altura};

                    const double tempoImpacto = varrerCirculoRetangulo(inicioRelativo, fimRelativo, tiro.raio, limites);

                    if (std::isfinite(tempoImpacto))
                        colisoes.push_back({tempoImpacto, TipoColisao::TiroJogador, i, j});
                }
            }
            else
            {
                const double tempoImpacto = segmentoCirculo(anterior - jogadorAnterior, tiro.posicao - jogador_.posicao,
                                                     {0, 0}, tiro.raio + jogador_.raio);

                if (std::isfinite(tempoImpacto))
                    colisoes.push_back({tempoImpacto, TipoColisao::TiroInimigo, i, 0});
            }
        }

        for (std::size_t j = 0; j < barris_.size(); ++j)
        {
            const auto& barril = barris_[j];
            const Retangulo limites{{0, 0}, barril.largura, barril.altura};

            const double tempoImpacto = varrerCirculoRetangulo(jogadorAnterior - barrisAnteriores[j],
                                                   jogador_.posicao - barril.posicao, jogador_.raio, limites);

            if (std::isfinite(tempoImpacto))
                colisoes.push_back({tempoImpacto, TipoColisao::Contato, 0, j});
        }

        std::stable_sort(colisoes.begin(), colisoes.end(),
                         [](const Colisao& primeiro, const Colisao& segundo)
                         {
                             if (primeiro.tempo != segundo.tempo)
                                 return primeiro.tempo < segundo.tempo;
                             return primeiro.tipo < segundo.tipo;
                         });

        return colisoes;
    }

    bool Jogo::aplicarColisoes(const std::vector<Colisao>& colisoes)
    {
        for (const auto& colisao : colisoes)
        {
            if (estado_ != Estado::Jogando)
                break;

            if (colisao.tipo == TipoColisao::Contato)
            {
                if (barris_[colisao.indiceBarril].resistencia > 0)
                    causarDanoJogador(MotivoDerrota::Barril);

                continue;
            }

            auto& tiro = tiros_[colisao.indiceTiro];
            if (!tiro.ativo)
                continue;

            if (colisao.tipo == TipoColisao::TiroInimigo)
            {
                tiro.ativo = false;
                causarDanoJogador(MotivoDerrota::Tiro);

                continue;
            }

            auto& barril = barris_[colisao.indiceBarril];
            if (barril.resistencia <= 0)
                continue;

            tiro.ativo = false;
            --barril.resistencia;
            if (barril.resistencia > 0)
                continue;

            ++destruidos_;
            if (barril.ehChefe())
            {
                ++destruidosNaFase_;
                concluirFase();
                return true;
            }

            if (!ehFaseChefe())
            {
                ++destruidosNaFase_;
                if (destruidosNaFase_ >= metaFase())
                {
                    concluirFase();
                    return true;
                }
            }
        }

        return false;
    }

    void Jogo::removerObjetosFinalizados()
    {
        barris_.erase(std::remove_if(barris_.begin(), barris_.end(),
                                      [](const Barril& barril)
                                      {
                                          const double alturaSaida =
                                              barril.temInimigo ? barril.inimigo.raio : -barril.altura / 2;
                                          const bool saiuDaArena = !barril.ehChefe() && barril.posicao.y < alturaSaida;
                                          return barril.resistencia <= 0 || saiuDaArena;
                                      }),
                       barris_.end());

        tiros_.erase(std::remove_if(tiros_.begin(), tiros_.end(),
                                      [this](const Tiro& tiro)
                                      {
                                          const bool foraHorizontal = tiro.posicao.x < -tiro.raio ||
                                                                tiro.posicao.x > configuracao_.larguraArena + tiro.raio;
                                          const bool foraVertical = tiro.posicao.y < -tiro.raio ||
                                                                tiro.posicao.y > configuracao_.alturaArena + tiro.raio;
                                          return !tiro.ativo || foraHorizontal || foraVertical;
                                      }),
                       tiros_.end());
    }

    void Jogo::passo(double dt)
    {
        tempoDecorrido_ += dt;

        if (emTransicao())
        {
            transicaoRestante_ = std::max(0.0, transicaoRestante_ - dt);
            if (transicaoRestante_ < 1e-10)
                iniciarFase(fase_ + 1);

            return;
        }

        protecaoRestante_ = std::max(0.0, protecaoRestante_ - dt);
        atualizarCriacaoBarris(dt);

        const Vetor2 jogadorAnterior = jogador_.posicao;
        moverJogador(dt);
        atualizarDisparos(dt);

        const auto barrisAnteriores = atualizarBarris(dt);
        const auto colisoes = detectarColisoes(dt, jogadorAnterior, barrisAnteriores);
        if (aplicarColisoes(colisoes))
            return;

        removerObjetosFinalizados();
    }
}
