#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

namespace cg
{
    constexpr double pi = 3.14159265358979323846;
    constexpr double semImpacto = std::numeric_limits<double>::infinity();

    struct Vetor2
    {
        double x = 0;
        double y = 0;

        Vetor2 operator+(Vetor2 b) const
        {
            return {x + b.x, y + b.y};
        }

        Vetor2 operator-(Vetor2 b) const
        {
            return {x - b.x, y - b.y};
        }

        Vetor2 operator*(double s) const
        {
            return {x * s, y * s};
        }
    };

    inline double produtoEscalar(Vetor2 a, Vetor2 b)
    {
        return a.x * b.x + a.y * b.y;
    }

    inline double comprimento(Vetor2 a)
    {
        return std::sqrt(produtoEscalar(a, a));
    }

    inline double radianos(double graus)
    {
        return graus * pi / 180;
    }

    inline double normalizarAngulo(double graus)
    {
        return std::remainder(graus, 360.0);
    }

    inline Vetor2 direcaoFrente(double graus)
    {
        const double a = radianos(graus);
        return {-std::sin(a), std::cos(a)};
    }

    inline double anguloPara(Vetor2 origem, Vetor2 alvo)
    {
        const Vetor2 direcao = alvo - origem;
        return std::atan2(-direcao.x, direcao.y) * 180 / pi;
    }

    inline Vetor2 girar(Vetor2 p, double graus)
    {
        const double a = radianos(graus);
        return {p.x * std::cos(a) - p.y * std::sin(a), p.x * std::sin(a) + p.y * std::cos(a)};
    }

    struct Retangulo
    {
        Vetor2 centro;
        double largura;
        double altura;
    };

    bool circuloRetangulo(Vetor2 centro, double raio, const Retangulo& retangulo);
    double segmentoCirculo(Vetor2 inicio, Vetor2 destino, Vetor2 centro, double raio);
    double segmentoRetangulo(Vetor2 inicio, Vetor2 destino, const Retangulo& retangulo);
    double varrerCirculoRetangulo(Vetor2 inicio, Vetor2 destino, double raio, const Retangulo& retangulo);
}
