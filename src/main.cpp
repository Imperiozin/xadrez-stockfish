#include "jogo.hpp"
#include "autoteste.hpp"

#include <chrono>
#include <cstdlib>
#include <future>
#include <iostream>
#include <sstream>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {
int inteiro(const std::string& value) {
    std::size_t end = 0;
    try {
        const int result = std::stoi(value, &end);
        if (end == value.size()) return result;
    } catch (const std::invalid_argument&) {
    } catch (const std::out_of_range&) {
    }
    throw std::invalid_argument("Informe um número inteiro válido.");
}
}  // namespace

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    try {
        auto engine = pastaDoExecutavel() / "engines" /
#ifdef _WIN32
            "stockfish.exe";
#else
            "stockfish";
#endif
        int time = 300, skill = 5;
        bool trace = false, selfTest = false;

        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--trace") trace = true;
            else if (arg == "--self-test") selfTest = true;
            else if (arg == "--help") {
                std::cout << "xadrez [--engine caminho] [--time 1..60000] [--skill 0..20] [--trace] [--self-test]\n"
                          << "Tempo em milissegundos por lance; dificuldade de 0 a 20.\n"
                          << "Durante a partida: dificuldade N, tempo N, config, fen, lances, novo, ajuda, sair.\n";
                return 0;
            } else if ((arg == "--engine" || arg == "--time" || arg == "--skill") && i + 1 < argc) {
                const std::string value = argv[++i];
                if (arg == "--engine") engine = arquivos::u8path(value);
                else if (arg == "--time") time = inteiro(value);
                else skill = inteiro(value);
            } else throw std::runtime_error("Argumento desconhecido ou sem valor: " + arg);
        }

        if (!arquivos::is_regular_file(engine))
            throw std::runtime_error("Motor ausente: " + engine.u8string() +
                "\nExecute preparar.bat / preparar.sh ou tools/setup_engine.py antes de jogar.");
        std::cout << "Motor: " << engine.u8string() << '\n';
        if (selfTest) return executarAutoteste(engine);
        Jogo jogo(engine, time, skill, trace);
        std::cout << "Comandos: dificuldade N (0 a 20), tempo N (1 a 60000 ms), config, ajuda.\n";
        jogo.desenharTabuleiro(std::cout);
        std::string input;
        bool finished = false;
        while (std::cout << "Seu lance> " && std::getline(std::cin, input)) {
            if (input == "sair" || input == "quit") break;
            if (input == "ajuda") {
                std::cout << "Digite um lance UCI, por exemplo e2e4.\n"
                          << "Comandos: dificuldade N (0 a 20), tempo N (1 a 60000 ms), config, fen, lances, novo, sair.\n";
                continue;
            }
            std::istringstream command(input);
            std::string name;
            command >> name;
            if (name == "dificuldade" || name == "tempo") {
                try {
                    std::string value, extra;
                    if (!(command >> value) || (command >> extra))
                        throw std::invalid_argument("Use dificuldade N ou tempo N, com um único número inteiro.");
                    if (name == "dificuldade") jogo.definirDificuldade(inteiro(value));
                    else jogo.definirTempoPensamento(inteiro(value));
                } catch (const std::invalid_argument& error) {
                    std::cout << "Configuração inválida: " << error.what() << '\n';
                    continue;
                }
                std::cout << "Dificuldade: " << jogo.dificuldade()
                          << "; tempo de pensamento: " << jogo.tempoPensamento() << " ms.\n";
                continue;
            }
            if (input == "config") {
                std::cout << "Dificuldade: " << jogo.dificuldade()
                          << "; tempo de pensamento: " << jogo.tempoPensamento() << " ms.\n";
                continue;
            }
            if (input == "fen") { std::cout << jogo.fen() << '\n'; continue; }
            if (input == "lances") {
                for (const auto& move : jogo.movimentosValidos()) std::cout << move << ' ';
                std::cout << '\n'; continue;
            }
            if (input == "novo") {
                jogo.reiniciar(); finished = false;
                jogo.desenharTabuleiro(std::cout); continue;
            }
            if (finished) { std::cout << "Partida encerrada. Digite novo ou sair.\n"; continue; }

            // Na futura interface OpenGL, continue desenhando enquanto aguarda o future.
            // Não leia/escreva Jogo em outra thread até este trabalho terminar.
            auto pending = std::async(std::launch::async,
                                      [&jogo, input] { return jogo.fazerMovimento(input); });
            bool waiting = false;
            while (pending.wait_for(std::chrono::milliseconds(150)) != std::future_status::ready) {
                if (!waiting) { std::cout << "Stockfish pensando" << std::flush; waiting = true; }
                std::cout << '.' << std::flush;
            }
            if (waiting) std::cout << '\n';
            const auto response = pending.get();
            if (!response) {
                std::cout << "Movimento inválido => nullptr. Tabuleiro mantido.\n";
                continue;
            }
            if (!response->movimento.empty())
                std::cout << "Stockfish respondeu: " << response->movimento << '\n';
            jogo.desenharTabuleiro(std::cout);
            finished = response->fimPartida;
            if (finished) std::cout << response->resultado << '\n';
        }
        std::cout << "Até mais!\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "ERRO: " << error.what() << '\n';
        return 1;
    }
}
