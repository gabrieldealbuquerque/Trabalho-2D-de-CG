#include "configuracao.hpp"
#include "tinyxml2.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <string>
#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace cg
{
    namespace
    {
        const tinyxml2::XMLElement& elementoObrigatorio(const tinyxml2::XMLElement& raiz, const char* nome)
        {
            const auto* valor = raiz.FirstChildElement(nome);

            if (!valor)
                throw std::runtime_error(std::string("XML: elemento obrigatorio ausente: ") + nome);

            if (valor->NextSiblingElement(nome))
                throw std::runtime_error(std::string("XML: elemento duplicado: ") + nome);

            return *valor;
        }

        double lerNumero(const tinyxml2::XMLElement& e, const char* nome)
        {
            const char* textoOriginal = e.Attribute(nome);
            if (textoOriginal)
            {
                char* fim = nullptr;
                const double valor = std::strtod(textoOriginal, &fim);
                if (fim != textoOriginal && *fim == '\0' && std::isfinite(valor))
                    return valor;
            }

            throw std::runtime_error(std::string("XML: numero invalido ou ausente em ") + e.Name() + "." + nome);
        }

        double lerNumeroOpcional(const tinyxml2::XMLElement& e, const char* nome, double valorPadrao)
        {
            if (!e.Attribute(nome))
                return valorPadrao;

            return lerNumero(e, nome);
        }

        int lerInteiro(const tinyxml2::XMLElement& e, const char* nome)
        {
            const double valor = lerNumero(e, nome);
            if (std::floor(valor) != valor || valor < 1 || valor > 1000000)
                throw std::runtime_error(std::string("XML: inteiro positivo esperado em ") + e.Name() + "." + nome);

            return static_cast<int>(valor);
        }

        void validarIntervalo(double valor, double minimo, double maximo, const char* nome)
        {
            if (!std::isfinite(valor) || valor < minimo || valor > maximo)
                throw std::runtime_error(std::string("Configuracao fora do intervalo permitido: ") + nome);
        }
    }

    Configuracao Configuracao::carregar(const std::filesystem::path& caminho)
    {
        tinyxml2::XMLDocument documento;
        if (documento.LoadFile(caminho.string().c_str()) != tinyxml2::XML_SUCCESS)
            throw std::runtime_error("Nao foi possivel ler " + caminho.string() + ": " + documento.ErrorStr());

        const auto* raiz = documento.RootElement();
        if (!raiz || std::string(raiz->Name()) != "jogo" || raiz->NextSiblingElement())
            throw std::runtime_error("XML: deve haver uma unica raiz <jogo>.");

        const auto& arena = elementoObrigatorio(*raiz, "arena");
        const auto& jogador = elementoObrigatorio(*raiz, "jogador");
        const auto& inimigo = elementoObrigatorio(*raiz, "inimigo");
        const auto& barril = elementoObrigatorio(*raiz, "barril");

        Configuracao configuracao;

        configuracao.larguraArena = lerInteiro(arena, "largura");
        configuracao.alturaArena = lerInteiro(arena, "altura");

        configuracao.raioJogador = lerNumero(jogador, "raioCabeca");
        configuracao.velocidadeJogador = lerNumero(jogador, "velocidade");
        configuracao.velocidadeGiro = lerNumeroOpcional(jogador, "velocidadeGiro", configuracao.velocidadeGiro);

        configuracao.raioInimigo = lerNumero(inimigo, "raioCabeca");
        configuracao.tirosInimigoPorSegundo = lerNumero(inimigo, "tirosPorSegundo");
        configuracao.velocidadeTiroInimigo = lerNumero(inimigo, "velocidadeTiro");

        configuracao.velocidadeTiroJogador = lerNumeroOpcional(jogador, "velocidadeTiro", configuracao.velocidadeTiroInimigo);
        configuracao.tirosJogadorPorSegundo = lerNumeroOpcional(jogador, "tirosPorSegundo", configuracao.tirosJogadorPorSegundo);

        configuracao.larguraBarril = lerNumero(barril, "largura");
        configuracao.alturaBarril = lerNumero(barril, "altura");
        configuracao.resistenciaBarril = lerInteiro(barril, "numeroTiros");
        configuracao.barrisParaVencer = lerInteiro(barril, "nParaGanhar");
        configuracao.velocidadeBarril = lerNumero(barril, "velocidade");

        if (const auto* extra = raiz->FirstChildElement("outros"))
        {
            if (extra->NextSiblingElement("outros"))
                throw std::runtime_error("XML: elemento duplicado: outros");

            configuracao.intervaloBarris = lerNumeroOpcional(*extra, "intervaloBarris", configuracao.intervaloBarris);
            configuracao.sensibilidadeMouse = lerNumeroOpcional(*extra, "sensibilidadeMouse", configuracao.sensibilidadeMouse);

            if (extra->Attribute("semente"))
            {
                const double semente = lerNumero(*extra, "semente");
                validarIntervalo(semente, 0, 4294967295.0, "outros.semente");
                if (std::floor(semente) != semente)
                    throw std::runtime_error("XML: semente deve ser inteira.");

                configuracao.semente = static_cast<std::uint32_t>(semente);
                configuracao.sementeFixa = true;
            }
        }

        configuracao.validar();
        return configuracao;
    }

    void Configuracao::validar() const
    {
        validarIntervalo(larguraArena, 80, 8192, "arena.largura (80..8192)");
        validarIntervalo(alturaArena, 120, 8192, "arena.altura (120..8192)");

        validarIntervalo(raioJogador, 1, std::min(larguraArena / 2.0, alturaArena / 4.0), "jogador.raioCabeca");
        validarIntervalo(raioInimigo, 1, std::min(larguraArena / 2.0, alturaArena / 2.0), "inimigo.raioCabeca");

        validarIntervalo(velocidadeJogador, 0, 10000, "jogador.velocidade");
        validarIntervalo(velocidadeGiro, 0.01, 3600, "jogador.velocidadeGiro");
        validarIntervalo(velocidadeTiroJogador, 0.01, 100000, "jogador.velocidadeTiro");
        validarIntervalo(tirosJogadorPorSegundo, 0.1, 60, "jogador.tirosPorSegundo (0.1..60)");

        validarIntervalo(velocidadeTiroInimigo, 0.01, 100000, "inimigo.velocidadeTiro");
        validarIntervalo(tirosInimigoPorSegundo, 0, 100, "inimigo.tirosPorSegundo");

        validarIntervalo(larguraBarril, 1, larguraArena, "barril.largura");
        validarIntervalo(alturaBarril, 1, alturaArena / 2.0, "barril.altura");
        validarIntervalo(velocidadeBarril, 0, 10000, "barril.velocidade");
        validarIntervalo(resistenciaBarril, 1, 1000000, "barril.numeroTiros");
        validarIntervalo(barrisParaVencer, 1, 1000000, "barril.nParaGanhar");

        validarIntervalo(intervaloBarris, 0.05, 3600, "outros.intervaloBarris");
        validarIntervalo(sensibilidadeMouse, 0.001, 30, "outros.sensibilidadeMouse");
    }

    std::filesystem::path diretorioExecutavel(const char* argv0)
    {
#if defined(__linux__)
        std::error_code erro;
        const auto executavel = std::filesystem::read_symlink("/proc/self/exe", erro);
        if (!erro)
            return executavel.parent_path();
#elif defined(_WIN32)
        wchar_t buffer[32768];
        const DWORD quantidade = GetModuleFileNameW(nullptr, buffer, 32768);
        if (quantidade > 0 && quantidade < 32768)
            return std::filesystem::path(buffer).parent_path();
#elif defined(__APPLE__)
        char buffer[4096];
        std::uint32_t tamanho = sizeof(buffer);
        if (_NSGetExecutablePath(buffer, &tamanho) == 0)
            return std::filesystem::weakly_canonical(buffer).parent_path();
#endif
        auto caminho = std::filesystem::absolute(argv0);
        if (caminho.has_parent_path() && std::filesystem::exists(caminho))
            return std::filesystem::weakly_canonical(caminho).parent_path();

        throw std::runtime_error("Nao foi possivel localizar o diretorio do executavel.");
    }
}
