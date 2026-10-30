#include "Application.h"
#include <iostream>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#endif
namespace {
int integer(const std::string &text) {
    std::size_t end = 0;
    int value = std::stoi(text, &end);
    if (end != text.size()) {
        throw std::invalid_argument("Numero invalido: " + text);
    }
    return value;
}
} // namespace

int main(int argc, char **argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    try {
        auto engine = pastaDoExecutavel() / "engines" /
#ifdef _WIN32
                      "stockfish.exe";
#else
                      "stockfish";
#endif
        int time = 300, skill = 0;
        bool trace = false;
        std::string scenarioFile = "assets/scenarios/sunset.cfg";
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--trace") {
                trace = true;
            } else if (arg == "--help") {
                std::cout << "Xadrez 3D: --engine caminho --time 1..60000 --skill -20..20 "
                             "--trace --scenario caminho.cfg\n";
                return 0;
            } else if ((arg == "--engine" || arg == "--time" || arg == "--skill" || arg == "--scenario") &&
                       i + 1 < argc) {
                std::string value = argv[++i];
                if (arg == "--engine") {
                    engine = arquivos::u8path(value);
                } else if (arg == "--time") {
                    time = integer(value);
                } else if (arg == "--skill") {
                    skill = integer(value);
                } else if (arg == "--scenario") {
                    scenarioFile = value;
                }
            } else {
                throw std::invalid_argument("Argumento desconhecido ou sem valor: " + arg);
            }
        }
        Jogo game(engine, time, skill, trace);
        Application app(game, scenarioFile);
        return app.run();
    } catch (const std::exception &e) {
        std::cerr << "ERRO: " << e.what() << '\n';
        return 1;
    }
}
