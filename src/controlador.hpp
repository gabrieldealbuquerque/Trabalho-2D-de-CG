#pragma once

#include "jogo.hpp"

namespace cg
{
    class Controlador
    {
      public:
        explicit Controlador(Jogo& jogo) : jogo_(jogo)
        {
        }

        bool pressionarTecla(unsigned char tecla);

        void soltarTecla(unsigned char tecla)
        {
            jogo_.definirTecla(tecla, false);
        }

        void teclaEspecial(int numeroFuncao);

        void moverMouse(int x);
        void botaoEsquerdo(bool pressionada, int x);
        void sairDaJanela();

      private:
        Jogo& jogo_;

        bool mouseConhecido_ = false;
        int xAnterior_ = 0;
    };
}
