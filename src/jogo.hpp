#pragma once

#include "chess.hpp"
#include "stockfish.hpp"
#include <iosfwd>
#include <memory>
#include <random>
#include <string>
#include <vector>

struct RespostaMovimento {
    std::string movimento; // Resposta UCI do Stockfish, por exemplo "e7e5".
    bool fimPartida = false;
    std::string resultado; // Vazio enquanto a partida continua.
};

class Jogo {
  public:
    explicit Jogo(const arquivos::path &stockfish, int tempoMs = 300, int nivel = 0,
                  bool trace = false);

    // API PRINCIPAL. O humano joga de brancas; o motor responde de pretas.
    // Inválido => nullptr, sem alterar o estado nem consultar o motor.
    // Válido => aplica os dois lances e devolve a resposta do motor.
    // Se o humano encerrar a partida: objeto com fimPartida=true e movimento="".
    // Falha do motor => exceção; o estado local permanece intacto.
    // A instância deve ser acessada por apenas uma thread de cada vez.
    std::unique_ptr<RespostaMovimento> fazerMovimento(const std::string &movimento);

    void reiniciar();
    // Dificuldade: -20 a 20; negativos misturam alternativas legais ao nivel 0.
    // Tempo: mesmos limites da API Stockfish. Nao alteram o tabuleiro.
    // A configuração é preservada ao reiniciar a partida.
    void definirDificuldade(int nivel);
    int dificuldade() const;
    void definirTempoPensamento(int tempoMs);
    int tempoPensamento() const;
    void desenharTabuleiro(std::ostream &output) const;
    std::string fen() const;
    std::vector<std::string> movimentosValidos() const;

  private:
    chess::Board tabuleiro_;
    std::vector<std::string> historico_;
    int nivel_;
    Stockfish motor_;
    std::mt19937 aleatorio_{std::random_device{}()};
};
