#pragma once

#include "stockfish.hpp"
#include "chess.hpp"
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

struct RespostaMovimento {
    std::string movimento;  // Resposta UCI do Stockfish, por exemplo "e7e5".
    bool fimPartida = false;
    std::string resultado;  // Vazio enquanto a partida continua.
};

class Jogo {
public:
    explicit Jogo(const arquivos::path& stockfish,
                  int tempoMs = 300, int nivel = 5, bool trace = false);

    // API PRINCIPAL. O humano joga de brancas; o motor responde de pretas.
    // Inválido => nullptr, sem alterar o estado nem consultar o motor.
    // Válido => aplica os dois lances e devolve a resposta do motor.
    // Se o humano encerrar a partida: objeto com fimPartida=true e movimento="".
    // Falha do motor => exceção; o estado local permanece intacto.
    // A instância deve ser acessada por apenas uma thread de cada vez.
    std::unique_ptr<RespostaMovimento> fazerMovimento(const std::string& movimento);

    void reiniciar();
    // Mesmos limites e exceções da API Stockfish. Não alteram o tabuleiro.
    // A configuração é preservada ao reiniciar a partida.
    void definirDificuldade(int nivel);
    int dificuldade() const;
    void definirTempoPensamento(int tempoMs);
    int tempoPensamento() const;
    void desenharTabuleiro(std::ostream& output) const;
    std::string fen() const;
    std::vector<std::string> movimentosValidos() const;

private:
    chess::Board tabuleiro_;
    std::vector<std::string> historico_;
    Stockfish motor_;
};
