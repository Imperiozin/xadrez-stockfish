#pragma once

#include "arquivos.hpp"
#include <memory>
#include <string>
#include <vector>

// Ponte UCI: este arquivo não conhece tabuleiro, terminal ou OpenGL.
// A instância deve ser acessada por apenas uma thread de cada vez.
class Stockfish {
  public:
    explicit Stockfish(const arquivos::path &executable, int milliseconds = 300, int skill = 0,
                       bool trace = false);
    ~Stockfish();
    Stockfish(const Stockfish &) = delete;
    Stockfish &operator=(const Stockfish &) = delete;

    // Quando solicitado, coleta ate tres candidatos via MultiPV para o modo iniciante.
    std::string responder(const std::vector<std::string> &history,
                          std::vector<std::string> *melhores = nullptr);
    void novaPartida();

    // Alterações valem para a próxima busca e são mantidas em novaPartida().
    // Valores inválidos lançam std::invalid_argument sem alterar a configuração.
    void definirDificuldade(int nivel); // 0 (menor força) a 20 (força máxima).
    int dificuldade() const;
    void definirTempoPensamento(int tempoMs); // 1 a 60000 milissegundos por lance.
    int tempoPensamento() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// Descobre a pasta do executável, independentemente da pasta de trabalho do IDE.
arquivos::path pastaDoExecutavel();
