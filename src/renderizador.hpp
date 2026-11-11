#pragma once

#include "jogo.hpp"
#include <filesystem>
#include <string>

namespace cg
{
    class Renderizador
    {
      public:
        void redimensionar(int largura, int altura);
        void desenhar(const Jogo& jogo) const;
        void salvarQuadro(const std::filesystem::path& caminho) const;

        int largura() const
        {
            return largura_;
        }

        int altura() const
        {
            return altura_;
        }

      private:
        void personagem(const Personagem& personagem, bool inimigo, TipoInimigo tipo = TipoInimigo::Atirador) const;
        void barril(const Barril& barril) const;
        void detalhesChefe(const Barril& barril) const;
        void inimigoDoBarril(const Barril& barril) const;
        void resistenciaDoBarril(const Barril& barril) const;

        void tiro(const Tiro& tiro) const;
        void contornosColisao(const Jogo& jogo) const;

        void painelInformacoes(const Jogo& jogo) const;
        void menu(const Jogo& jogo) const;
        void painelSobreposto(const Jogo& jogo) const;

        int largura_ = 600;
        int altura_ = 800;
    };
}
