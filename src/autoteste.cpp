#include "autoteste.hpp"
#include "jogo.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
void verificar(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template <typename Function>
void verificarInvalido(Function function) {
    try { function(); }
    catch (const std::invalid_argument&) { return; }
    throw std::runtime_error("Uma configuração inválida foi aceita.");
}

// Restaura o log mesmo quando uma verificação lança uma exceção.
struct CapturaLog {
    std::ostringstream log;
    std::streambuf* original = std::cerr.rdbuf(log.rdbuf());
    ~CapturaLog() { std::cerr.rdbuf(original); }
};
}  // namespace

int executarAutoteste(const arquivos::path& engine) {
    CapturaLog capture;
    {
        Stockfish motor(engine, 50, 5, true);
        verificar(motor.dificuldade() == 5 && motor.tempoPensamento() == 50,
                  "O motor não preservou os valores do construtor.");
        motor.definirDificuldade(0);
        motor.definirTempoPensamento(1);
        verificar(motor.dificuldade() == 0 && motor.tempoPensamento() == 1,
                  "O motor não aceitou os limites mínimos.");
        motor.definirDificuldade(20);
        motor.definirTempoPensamento(60000);
        verificar(motor.dificuldade() == 20 && motor.tempoPensamento() == 60000,
                  "O motor não aceitou os limites máximos.");
        const auto previous = capture.log.str();
        for (int value : {-1, 21})
            verificarInvalido([&] { motor.definirDificuldade(value); });
        for (int value : {-1, 0, 60001})
            verificarInvalido([&] { motor.definirTempoPensamento(value); });
        verificar(motor.dificuldade() == 20 && motor.tempoPensamento() == 60000 &&
                  capture.log.str() == previous,
                  "Uma entrada inválida alterou a configuração ou enviou comandos UCI.");
        motor.definirDificuldade(3);
        motor.definirTempoPensamento(25);
        motor.novaPartida();
        verificar(motor.dificuldade() == 3 && motor.tempoPensamento() == 25,
                  "novaPartida perdeu a configuração.");
        verificar(!motor.responder({"e2e4"}).empty(), "O motor não retornou um lance.");
    }
    verificar(capture.log.str().find(">> setoption name Skill Level value 3\n>> isready\n") != std::string::npos,
              "A dificuldade não foi enviada e sincronizada via UCI.");
    verificar(capture.log.str().find(">> go movetime 25\n") != std::string::npos,
              "O tempo atualizado não foi enviado na busca seguinte.");

    // Os argumentos são validados antes de tentar abrir um executável.
    for (int value : {-1, 21})
        verificarInvalido([&] { Stockfish invalid({}, 50, value); });
    for (int value : {0, 60001})
        verificarInvalido([&] { Stockfish invalid({}, value, 5); });

    {
        Jogo jogo(engine, 50, 5);
        const auto initial = jogo.fen();
        for (const auto& input : {"", "e2e5", "e7e5", "z9z1", "e2e4\nquit"}) {
            verificar(!jogo.fazerMovimento(input), "Um lance inválido foi aceito.");
            verificar(jogo.fen() == initial, "Um lance inválido alterou o tabuleiro.");
        }
        jogo.definirDificuldade(8);
        jogo.definirTempoPensamento(40);
        verificar(jogo.fen() == initial, "A configuração alterou o tabuleiro.");
        verificarInvalido([&] { jogo.definirDificuldade(21); });
        verificarInvalido([&] { jogo.definirTempoPensamento(0); });
        verificar(jogo.dificuldade() == 8 && jogo.tempoPensamento() == 40 && jogo.fen() == initial,
                  "A API Jogo não preservou a configuração após uma entrada inválida.");
        auto response = jogo.fazerMovimento("e2e4");
        verificar(response && !response->movimento.empty(), "O motor não respondeu ao primeiro lance.");
        chess::Board expected;
        expected.makeMove(chess::uci::uciToMove(expected, "e2e4"));
        expected.makeMove(chess::uci::uciToMove(expected, response->movimento));
        verificar(jogo.fen() == expected.getFen(), "O tabuleiro não corresponde aos dois lances.");
        const auto position = jogo.fen();
        jogo.definirDificuldade(2);
        jogo.definirTempoPensamento(20);
        verificar(jogo.fen() == position, "Mudar a configuração perdeu o histórico da partida.");
        const auto legal = jogo.movimentosValidos();
        verificar(!legal.empty(), "Não há lances disponíveis no segundo turno.");
        response = jogo.fazerMovimento(legal.front());
        verificar(response && !response->movimento.empty(), "O motor não respondeu após a alteração.");
        jogo.reiniciar();
        verificar(jogo.fen() == initial && jogo.dificuldade() == 2 && jogo.tempoPensamento() == 20,
                  "Reiniciar não preservou a configuração ou não restaurou o tabuleiro.");
    }
    std::cout << "Autoteste aprovado: API, limites, comandos UCI, lances e reinício com Stockfish real.\n";
    return 0;
}
