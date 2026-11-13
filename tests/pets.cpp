#include "AnimatedModel.h"
#include "Scenario.h"
#include "jogo.hpp"
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok, const char *message) {
    if (!ok) throw std::runtime_error(message);
}
void evaluationTests() {
    AvaliacaoPosicao score;
    lerAvaliacaoUci("info depth 18 multipv 1 score cp 130 wdl 750 200 50 pv e2e4 e7e5", score);
    check(score.valida && score.temWdl && score.valor == 130 && !score.mate, "UCI centipawns/WDL");
    lerAvaliacaoUci("info depth 19 multipv 2 score mate -3 wdl 0 0 1000 pv d2d4", score);
    check(score.valor == 130 && !score.mate, "Ignore other MultiPV lines");
    lerAvaliacaoUci("info depth 20 score cp 900 lowerbound wdl 1000 0 0 pv e2e4", score);
    check(score.valor == 130, "Do not classify bounds as exact scores");
    lerAvaliacaoUci("info string score cp 999 wdl 1000 0 0", score);
    check(score.valor == 130, "Ignore info string diagnostics");
    lerAvaliacaoUci("info score cp 900 wdl 1000 1000 0", score);
    check(score.valor == 130, "Reject invalid WDL sums");
    lerAvaliacaoUci("info depth 21 score mate -3 wdl 0 0 1000 pv e2e4", score);
    check(score.mate && score.valor == -3, "Preserve negative mate scores");

    AvaliacaoPosicao best;
    lerAvaliacaoUci("info score cp 100 wdl 800 100 100", best);
    best.melhorMovimento = "e2e4";
    check(classificarJogada("e2e4", best, best).qualidade == QualidadeJogada::Melhor, "Best move");
    const QualidadeJogada expected[] = {QualidadeJogada::Excelente, QualidadeJogada::Boa,
        QualidadeJogada::Imprecisao, QualidadeJogada::Erro, QualidadeJogada::ErroGrave};
    const int losses[] = {19, 20, 50, 100, 200};
    for (int i = 0; i < 5; ++i) {
        auto played = best;
        played.vitorias -= losses[i];
        played.derrotas += losses[i];
        check(classificarJogada("a2a3", best, played).qualidade == expected[i], "Quality thresholds");
    }
    check(classificarJogada("e2e4", {}, {}).qualidade == QualidadeJogada::Desconhecida,
        "Missing evaluation must not become an excellent move");
}
void modelTests() {
    const char *pets[] = {"Penguin", "Panda", "Deer", "Alien", "Ghost"};
    for (int scene = 0; scene < 5; ++scene) {
        const auto scenario = Scenario::load(Scenario::fileForDifficulty(-20 + scene*10));
        check(scenario.pet == pets[scene], "Scenario/pet mapping");
        AnimatedModel model((pastaDoExecutavel() /
            ("assets/objects/pets/" + scenario.pet + ".gltf")).string());
        check(model.texturePng().size() > 100, "Embedded pet texture");
        const auto count = model.vertices().size();
        check(count > 100, "Pet geometry");
        for (const char *clip : {"Idle", "Yes", "No", "Jump", "Dance", "HitRecieve"}) {
            model.sample(clip, 0);
            const auto first = model.vertices();
            double difference = 0;
            for (int frame = 1; frame <= 24; ++frame) {
                model.sample(clip, model.duration(clip)*frame/24);
                check(model.vertices().size() == count, "Stable animation topology");
                for (std::size_t i = 0; i < count; ++i) {
                    const auto &v = model.vertices()[i];
                    check(std::isfinite(v.position.x) && std::isfinite(v.position.y) &&
                        std::isfinite(v.position.z) && std::isfinite(v.normal.x) &&
                        std::abs(dot(v.normal,v.normal)-1) < .01f, "Finite skinned vertices/normals");
                    check(std::abs(v.position.x) < 10 && std::abs(v.position.y) < 10 &&
                        std::abs(v.position.z) < 10, "Bones stay in normalized pet bounds");
                    const auto delta = v.position - first[i].position;
                    difference += dot(delta,delta);
                }
            }
            check(difference > .001, "Animation must actually deform the mesh");
            model.sample(clip, 0);
            const auto reset = model.vertices();
            for (std::size_t i = 0; i < count; ++i)
                check(dot(reset[i].position-first[i].position, reset[i].position-first[i].position) < 1e-8,
                    "Animation loops deterministically");
        }
        std::cout << "PASS pet " << scenario.pet << ": " << count << " vertices, six reaction clips\n";
    }
}
void engineTests() {
    const auto executable = pastaDoExecutavel() / "engines" /
#ifdef _WIN32
        "stockfish.exe";
#else
        "stockfish";
#endif
    Stockfish analyst(executable, 200, 20);
    const auto initial = analyst.analisar({});
    check(initial.valida && initial.temWdl && !initial.melhorMovimento.empty(), "Live analysis protocol");
    // White can prevent scholar's mate with g2g3; a2a3 allows Qxf2#.
    const std::vector<std::string> history{"e2e4","e7e5","f1c4","f8c5","b1c3","d8h4"};
    const auto best = analyst.analisar(history);
    const auto blunder = analyst.analisar(history, "a2a3");
    check(blunder.valida && blunder.temWdl && blunder.mate && blunder.valor < 0, "Forced move detects lost mate");
    check(classificarJogada("a2a3", best, blunder).qualidade == QualidadeJogada::ErroGrave,
        "Real blunder, with scores from the same side's perspective");
    analyst.novaPartida();
    Jogo game(executable, 20, -20);
    const auto before = game.fen();
    check(!game.fazerMovimento("e2e5") && game.fen() == before, "Invalid move remains atomic");
    const auto reply = game.fazerMovimento("e2e4");
    check(reply && !reply->movimento.empty() && reply->avaliacao.movimento == "e2e4" &&
        reply->avaliacao.qualidade != QualidadeJogada::Desconhecida, "Human move review at beginner difficulty");
    check(game.dificuldade() == -20, "Analyst does not change opponent difficulty");
    game.reiniciar();
    check(game.fen() == before, "New game clears history");
    std::cout << "PASS live Stockfish: analysis, forced blunder, beginner review, reset\n";
}
}
void testPetRendering();
int main(int argc, char **argv) {
    try {
        bool engine = true, render = false;
        for (int i = 1; i < argc; ++i) {
            if (std::string(argv[i]) == "--no-engine") engine = false;
            if (std::string(argv[i]) == "--render") render = true;
        }
        evaluationTests();
        modelTests();
        if (engine) engineTests();
        if (render) testPetRendering();
        std::cout << "PASS pet checks\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
