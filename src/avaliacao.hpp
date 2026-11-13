#pragma once
#include <algorithm>
#include <sstream>
#include <string>

// Scores are always relative to the side to move at the analyzed root.
struct AvaliacaoPosicao {
    bool valida = false;
    bool temWdl = false;
    bool mate = false;
    int valor = 0; // Centipawns, or signed moves to mate when mate=true.
    int vitorias = 0, empates = 0, derrotas = 0; // Per mille from Stockfish.
    std::string melhorMovimento;
    double pontosEsperados() const {
        return (vitorias + .5 * empates) / 1000.0;
    }
};

enum class QualidadeJogada { Desconhecida, Melhor, Excelente, Boa, Imprecisao, Erro, ErroGrave };

inline const char *nomeQualidade(QualidadeJogada qualidade) {
    switch (qualidade) {
    case QualidadeJogada::Melhor: return "Melhor lance";
    case QualidadeJogada::Excelente: return "Excelente";
    case QualidadeJogada::Boa: return "Boa jogada";
    case QualidadeJogada::Imprecisao: return "Imprecisão";
    case QualidadeJogada::Erro: return "Erro";
    case QualidadeJogada::ErroGrave: return "Erro grave";
    default: return "Avaliação indisponível";
    }
}

struct AvaliacaoJogada {
    QualidadeJogada qualidade = QualidadeJogada::Desconhecida;
    AvaliacaoPosicao posicao;
    std::string movimento;
    double perda = 0;
};

// Both searches start at the SAME position; the second restricts the root move.
// This avoids reversing the score when the turn changes and preserves history.
inline AvaliacaoJogada classificarJogada(const std::string &movimento,
                                        const AvaliacaoPosicao &melhor,
                                        const AvaliacaoPosicao &jogada) {
    AvaliacaoJogada resultado;
    resultado.movimento = movimento;
    resultado.posicao = jogada;
    if (!melhor.valida || !jogada.valida || !melhor.temWdl || !jogada.temWdl) return resultado;
    const int perda = std::max(0, 2*melhor.vitorias + melhor.empates - 2*jogada.vitorias - jogada.empates);
    resultado.perda = perda / 2000.0;
    if (movimento == melhor.melhorMovimento) resultado.qualidade = QualidadeJogada::Melhor;
    else if (perda < 40) resultado.qualidade = QualidadeJogada::Excelente;
    else if (perda < 100) resultado.qualidade = QualidadeJogada::Boa;
    else if (perda < 200) resultado.qualidade = QualidadeJogada::Imprecisao;
    else if (perda < 400) resultado.qualidade = QualidadeJogada::Erro;
    else resultado.qualidade = QualidadeJogada::ErroGrave;
    return resultado;
}

// Ignore incomplete MultiPV entries and bounds; only keep exact score snapshots.
inline void lerAvaliacaoUci(const std::string &line, AvaliacaoPosicao &ultima) {
    if (line.rfind("info ", 0) != 0) return;
    std::istringstream input(line);
    AvaliacaoPosicao nova;
    std::string token;
    int multipv = 1;
    bool bound = false;
    while (input >> token) {
        if (token == "string" || token == "pv") break;
        if (token == "multipv") input >> multipv;
        else if (token == "lowerbound" || token == "upperbound") bound = true;
        else if (token == "score") {
            std::string kind;
            if (input >> kind >> nova.valor) {
                nova.valida = kind == "cp" || kind == "mate";
                nova.mate = kind == "mate";
            }
        } else if (token == "wdl") {
            if (input >> nova.vitorias >> nova.empates >> nova.derrotas)
                nova.temWdl = nova.vitorias >= 0 && nova.empates >= 0 && nova.derrotas >= 0 &&
                    nova.vitorias + nova.empates + nova.derrotas == 1000;
        }
    }
    if (nova.valida && nova.temWdl && !bound && multipv == 1) ultima = nova;
}
