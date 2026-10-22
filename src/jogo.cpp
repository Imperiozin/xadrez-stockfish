#include "jogo.hpp"

#include <algorithm>
#include <cctype>
#include <ostream>
#include <stdexcept>

namespace {
int nivelMotor(int nivel) {
    if (nivel < -20 || nivel > 20) {
        throw std::invalid_argument("A dificuldade deve estar entre -20 e 20.");
    }
    return std::max(0, nivel);
}

std::string normalizar(std::string input) {
    const auto first = input.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    const auto last = input.find_last_not_of(" \t\r\n");
    input = input.substr(first, last - first + 1);
    std::transform(input.begin(), input.end(), input.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return input;
}

bool encontrarLegal(const chess::Board &board, const std::string &uci, chess::Move &result) {
    chess::Movelist legal;
    chess::movegen::legalmoves(legal, board);
    // Compara com lances legais ANTES de converter/aplicar uma entrada externa.
    for (const auto move : legal) {
        if (chess::uci::moveToUci(move) == uci) {
            result = move;
            return true;
        }
    }
    return false;
}

void atualizarResultado(const chess::Board &board, RespostaMovimento &response) {
    const auto result = board.isGameOver();
    response.fimPartida = result.second != chess::GameResult::NONE;
    if (!response.fimPartida) {
        return;
    }
    if (result.first == chess::GameResultReason::CHECKMATE) {
        response.resultado = board.sideToMove() == chess::Color::WHITE
                                 ? "Xeque-mate. Stockfish venceu."
                                 : "Xeque-mate. Você venceu!";
    } else {
        response.resultado =
            "Empate (afogamento, repetição, material insuficiente ou regra dos 50 lances).";
    }
}
} // namespace

Jogo::Jogo(const arquivos::path &stockfish, int tempoMs, int nivel, bool trace)
    : nivel_(nivel), motor_(stockfish, tempoMs, nivelMotor(nivel), trace) {
    motor_.novaPartida();
}

std::unique_ptr<RespostaMovimento> Jogo::fazerMovimento(const std::string &movimento) {
    if (tabuleiro_.isGameOver().second != chess::GameResult::NONE) {
        return nullptr;
    }
    const std::string input = normalizar(movimento);
    chess::Move human;
    if (!encontrarLegal(tabuleiro_, input, human)) {
        return nullptr;
    }

    // Trabalha sobre uma copia: erro do motor nao deixa uma jogada pela metade.
    auto candidate = tabuleiro_;
    auto history = historico_;
    candidate.makeMove(human);
    history.push_back(input);
    auto response = std::make_unique<RespostaMovimento>();
    atualizarResultado(candidate, *response);
    if (!response->fimPartida) {
        std::vector<std::string> melhores;
        response->movimento = motor_.responder(history, nivel_ < 0 ? &melhores : nullptr);
        chess::Move computer;
        if (!encontrarLegal(candidate, response->movimento, computer)) {
            throw std::runtime_error("O motor retornou um lance inválido: " + response->movimento);
        }
        // Cada nivel negativo acrescenta 5% de chance de ignorar a escolha do motor.
        // Sorteia apenas alternativas legais; nao ha garantia de erro em cada lance.
        if (nivel_ < 0 && std::uniform_int_distribution<int>(1, 20)(aleatorio_) <= -nivel_) {
            chess::Movelist legal;
            chess::movegen::legalmoves(legal, candidate);
            // Exclui os candidatos da analise e a resposta escolhida pelo Skill Level.
            // Mantem pelo menos uma opcao quando ha poucos lances legais.
            std::vector<chess::Move> alternativas;
            for (const auto move : legal) {
                if (move != computer) {
                    alternativas.push_back(move);
                }
            }
            for (const auto &best : melhores) {
                if (alternativas.size() <= 1) {
                    break;
                }
                alternativas.erase(std::remove_if(alternativas.begin(), alternativas.end(),
                    [&](chess::Move move) { return chess::uci::moveToUci(move) == best; }),
                    alternativas.end());
            }
            if (!alternativas.empty()) {
                computer = alternativas[std::uniform_int_distribution<std::size_t>(
                    0, alternativas.size() - 1)(aleatorio_)];
                response->movimento = chess::uci::moveToUci(computer);
            }
        }
        candidate.makeMove(computer);
        history.push_back(response->movimento);
        atualizarResultado(candidate, *response);
    }
    tabuleiro_ = std::move(candidate);
    historico_ = std::move(history);
    return response;
}

void Jogo::reiniciar() {
    motor_.novaPartida();
    tabuleiro_ = chess::Board();
    historico_.clear();
}

void Jogo::definirDificuldade(int nivel) {
    motor_.definirDificuldade(nivelMotor(nivel));
    nivel_ = nivel;
}

int Jogo::dificuldade() const {
    return nivel_;
}

void Jogo::definirTempoPensamento(int tempoMs) {
    motor_.definirTempoPensamento(tempoMs);
}

int Jogo::tempoPensamento() const {
    return motor_.tempoPensamento();
}

std::string Jogo::fen() const {
    return tabuleiro_.getFen();
}

std::vector<std::string> Jogo::movimentosValidos() const {
    std::vector<std::string> result;
    if (tabuleiro_.isGameOver().second != chess::GameResult::NONE) {
        return result;
    }
    chess::Movelist moves;
    chess::movegen::legalmoves(moves, tabuleiro_);
    for (const auto move : moves) {
        result.push_back(chess::uci::moveToUci(move));
    }
    return result;
}

void Jogo::desenharTabuleiro(std::ostream &output) const {
    output << "\n    a b c d e f g h\n  +-----------------+\n";
    for (int rank = 7; rank >= 0; --rank) {
        output << rank + 1 << " | ";
        for (int file = 0; file < 8; ++file) {
            const auto piece = tabuleiro_.at(chess::Square(rank * 8 + file));
            output << (piece == chess::Piece::NONE ? "." : static_cast<std::string>(piece)) << ' ';
        }
        output << "| " << rank + 1 << '\n';
    }
    output << "  +-----------------+\n    a b c d e f g h\n\n";
    if (tabuleiro_.inCheck()) {
        output << "XEQUE!\n";
    }
}
