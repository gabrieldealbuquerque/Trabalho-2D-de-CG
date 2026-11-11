#pragma once

#include <cstdint>
#include <filesystem>

namespace cg
{
    struct Configuracao
    {
        int larguraArena = 600;
        int alturaArena = 800;

        double raioJogador = 18;
        double velocidadeJogador = 180;
        double velocidadeGiro = 150;
        double velocidadeTiroJogador = 480;
        double tirosJogadorPorSegundo = 6;

        double raioInimigo = 15;
        double tirosInimigoPorSegundo = 0.7;
        double velocidadeTiroInimigo = 190;

        double larguraBarril = 76;
        double alturaBarril = 64;
        double velocidadeBarril = 65;
        int resistenciaBarril = 3;
        int barrisParaVencer = 10;

        double intervaloBarris = 2.1;
        double sensibilidadeMouse = 0.25;

        std::uint32_t semente = 0;
        bool sementeFixa = false;

        static Configuracao carregar(const std::filesystem::path& caminho);
        void validar() const;
    };

    std::filesystem::path diretorioExecutavel(const char* argv0);
}
