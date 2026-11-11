#include "controlador.hpp"
#include <cctype>

namespace cg
{
    bool Controlador::pressionarTecla(unsigned char tecla)
    {
        tecla = static_cast<unsigned char>(std::tolower(tecla));

        if (tecla == 27)
            return false;

        if (jogo_.estado() == Estado::Menu)
        {
            if (tecla == '1' || tecla == '2')
            {
                jogo_.selecionarModo(tecla == '1' ? ModoJogo::Trabalho : ModoJogo::Campanha);
                mouseConhecido_ = false;
            }

            return true;
        }

        switch (tecla)
        {
            case 'm':
                jogo_.mostrarMenu();
                mouseConhecido_ = false;
                break;
            case 'r':
                jogo_.reiniciar();
                mouseConhecido_ = false;
                break;
            case 'p':
                jogo_.alternarPausa();
                break;
            case 'h':
                jogo_.alternarAjuda();
                break;
            default:
                jogo_.definirTecla(tecla, true);
        }

        return true;
    }

    void Controlador::teclaEspecial(int numeroFuncao)
    {
        if (numeroFuncao == 1)
            jogo_.alternarAjuda();
        else if (numeroFuncao >= 6 && numeroFuncao <= 11)
        {
            jogo_.carregarDemonstracao(numeroFuncao);
            mouseConhecido_ = false;
        }
        else
            jogo_.alternarDemonstracao(numeroFuncao);
    }

    void Controlador::moverMouse(int x)
    {
        if (mouseConhecido_)
            jogo_.mirar(static_cast<double>(x) - xAnterior_);

        xAnterior_ = x;
        mouseConhecido_ = true;
    }

    void Controlador::botaoEsquerdo(bool pressionada, int x)
    {
        moverMouse(x);
        jogo_.definirDisparo(pressionada);
    }

    void Controlador::sairDaJanela()
    {
        jogo_.limparEntradas();
        mouseConhecido_ = false;
    }
}
