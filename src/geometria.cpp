#include "geometria.hpp"

namespace cg
{
    namespace
    {
        constexpr double toleranciaContato = 1e-10;
        constexpr double toleranciaDirecao = 1e-14;
        constexpr double toleranciaComprimentoZero = 1e-20;
    }

    bool circuloRetangulo(Vetor2 centro, double raio, const Retangulo& retangulo)
    {
        const Vetor2 maisProximo{std::clamp(centro.x, retangulo.centro.x - retangulo.largura / 2, retangulo.centro.x + retangulo.largura / 2),
                           std::clamp(centro.y, retangulo.centro.y - retangulo.altura / 2, retangulo.centro.y + retangulo.altura / 2)};
        const Vetor2 distancia = centro - maisProximo;

        return produtoEscalar(distancia, distancia) <= raio * raio + toleranciaContato;
    }

    double segmentoCirculo(Vetor2 inicio, Vetor2 destino, Vetor2 centro, double raio)
    {
        const Vetor2 deslocamento = inicio - centro;
        const Vetor2 delta = destino - inicio;
        const double c = produtoEscalar(deslocamento, deslocamento) - raio * raio;
        if (c <= toleranciaContato)
            return 0;

        const double a = produtoEscalar(delta, delta);
        if (a < toleranciaComprimentoZero)
            return semImpacto;

        const double b = produtoEscalar(deslocamento, delta);
        const double discriminante = b * b - a * c;
        if (discriminante < 0)
            return semImpacto;

        const double tempoImpacto = (-b - std::sqrt(discriminante)) / a;
        if (tempoImpacto >= 0 && tempoImpacto <= 1)
            return tempoImpacto;

        return semImpacto;
    }

    double segmentoRetangulo(Vetor2 inicio, Vetor2 destino, const Retangulo& retangulo)
    {
        double tempoEntrada = 0;
        double tempoSaida = 1;

        const Vetor2 local = inicio - retangulo.centro;
        const Vetor2 delta = destino - inicio;
        const double origens[] = {local.x, local.y};
        const double direcoes[] = {delta.x, delta.y};
        const double meiasDimensoes[] = {retangulo.largura / 2, retangulo.altura / 2};

        for (int eixo = 0; eixo < 2; ++eixo)
        {
            if (std::abs(direcoes[eixo]) < toleranciaDirecao)
            {
                if (std::abs(origens[eixo]) > meiasDimensoes[eixo])
                    return semImpacto;
            }
            else
            {
                double tempoProximo = (-meiasDimensoes[eixo] - origens[eixo]) / direcoes[eixo];
                double tempoDistante = (meiasDimensoes[eixo] - origens[eixo]) / direcoes[eixo];
                if (tempoProximo > tempoDistante)
                    std::swap(tempoProximo, tempoDistante);

                tempoEntrada = std::max(tempoEntrada, tempoProximo);
                tempoSaida = std::min(tempoSaida, tempoDistante);
                if (tempoEntrada > tempoSaida)
                    return semImpacto;
            }
        }

        return tempoEntrada;
    }

    double varrerCirculoRetangulo(Vetor2 inicio, Vetor2 destino, double raio, const Retangulo& retangulo)
    {
        if (circuloRetangulo(inicio, raio, retangulo))
            return 0;

        const Retangulo faixaHorizontal{retangulo.centro, retangulo.largura + 2 * raio, retangulo.altura};
        const Retangulo faixaVertical{retangulo.centro, retangulo.largura, retangulo.altura + 2 * raio};
        double impacto = std::min(segmentoRetangulo(inicio, destino, faixaHorizontal), segmentoRetangulo(inicio, destino, faixaVertical));

        for (double x : {-1.0, 1.0})
        {
            for (double y : {-1.0, 1.0})
            {
                const Vetor2 canto = retangulo.centro + Vetor2{x * retangulo.largura / 2, y * retangulo.altura / 2};
                impacto = std::min(impacto, segmentoCirculo(inicio, destino, canto, raio));
            }
        }

        return impacto;
    }
}
