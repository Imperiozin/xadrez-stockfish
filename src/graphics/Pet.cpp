#include "Pet.h"
// Pets, textura e animações: Cube Pets, por Kenney — https://kenney.nl (CC0).
#include "AnimatedModel.h"
#include "PetCatalog.h"
#include "Table.h"
#include "stockfish.hpp"
#include "jogo.hpp"
#include "../../vendor/stb_image.h"
#include <stdexcept>

namespace {
const PetDefinition &definition(const std::string &name) {
    const auto *pet = findPet(name);
    if (!pet) throw std::runtime_error("Pet desconhecido: " + name);
    return *pet;
}
constexpr float pi = 3.14159265358979323846f;
constexpr float laneX = -7.5f, laneEnd = 5.f, boardAngle = pi / 2;
constexpr double walkSpeed = 1.2, pauseDuration = 3;
constexpr double walkLeg = laneEnd / walkSpeed;
constexpr double segmentDuration = pauseDuration + walkLeg;
constexpr double tourDuration = 4 * segmentDuration;
constexpr float stops[] = {0, -laneEnd, 0, laneEnd, 0};
}

struct Pet::Impl {
    AnimatedModel model;
    Mesh body;
    GLuint texture = 0;
    std::string clip = "idle";
    std::string tourClip = "idle";
    double time = 0, remaining = 0, tourTime = 0;
    double tourClipTime = 0;
    bool stopped = false;
    float transition = 1;
    std::vector<Vertex> previous, pose;

    explicit Impl(const std::string &id) :
        model((pastaDoExecutavel() / petModelPath(definition(id))).string(), 2.2f),
        body(model.vertices()) {
        pose = model.vertices();
        int width, height, channels;
        const auto &png = model.texturePng();
        stbi_set_flip_vertically_on_load(false);
        auto *pixels = stbi_load_from_memory(png.data(), int(png.size()), &width, &height, &channels, 4);
        if (!pixels) throw std::runtime_error("Falha ao carregar textura do pet " + id);
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        stbi_image_free(pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    ~Impl() { glDeleteTextures(1, &texture); }
    void play(const std::string &next, double hold) {
        previous = pose;
        clip = next;
        time = 0;
        remaining = hold;
        transition = 0;
    }
};

Pet::Pet(const std::string &name, const Pet *previous) : impl(std::make_unique<Impl>(name)) {
    position = {laneX, Table::surfaceY, 0};
    angle = boardAngle;
    if (previous) {
        // Troca apenas o modelo; os relógios e a reação em curso continuam.
        const auto &source = *previous->impl;
        auto &m = *impl;
        position = previous->position;
        scale = previous->scale;
        rotation = previous->rotation;
        angle = previous->angle;
        m.clip = source.clip;
        m.time = source.time;
        m.remaining = source.remaining;
        m.tourTime = source.tourTime;
        m.tourClip = source.tourClip;
        m.tourClipTime = source.tourClipTime;
        m.stopped = source.stopped;
        m.model.sample(m.clip, m.time);
        m.pose = m.model.vertices();
        m.body.update(m.pose);
    }
}
Pet::~Pet() = default;

void Pet::update(float dt) {
    auto &m = *impl;
    dt = std::max(0.f, dt);
    float freeTime = dt;
    if (m.remaining > 0) {
        freeTime = float(std::max(0.0, double(dt) - m.remaining));
        m.remaining = std::max(0.0, m.remaining - dt);
        if (m.remaining == 0) m.play("idle", 0);
    }
    float heading = boardAngle;
    const bool touring = m.remaining == 0 && !m.stopped;
    if (touring) {
        m.tourTime = std::fmod(m.tourTime + freeTime, tourDuration);
        const int segment = int(m.tourTime / segmentDuration);
        const double phase = std::fmod(m.tourTime, segmentDuration);
        const char *clip = "idle";
        position.z = stops[segment];
        if (phase >= pauseDuration) {
            const float direction = stops[segment + 1] > stops[segment] ? 1.f : -1.f;
            position.z += direction * float((phase - pauseDuration) * walkSpeed);
            heading = direction > 0 ? 0 : pi;
            clip = "walk";
        }
        if (m.clip != clip) m.play(clip, 0);
    }
    // O passeio fica na lateral livre; nas pausas e reações, olha o tabuleiro.
    const float turn = std::remainder(heading - angle, 2 * pi);
    angle += std::clamp(turn, -dt * 6.f, dt * 6.f);
    m.time += dt;
    if (touring) {
        m.tourClip = m.clip;
        m.tourClipTime = m.time;
    }
    m.model.sample(m.clip, m.time);
    m.transition = std::min(1.f, m.transition + dt / .16f);
    auto vertices = m.model.vertices();
    if (m.transition < 1 && m.previous.size() == vertices.size()) {
        const float t = m.transition*m.transition*(3-2*m.transition);
        for (std::size_t i = 0; i < vertices.size(); ++i) {
            vertices[i].position = m.previous[i].position*(1-t) + vertices[i].position*t;
            vertices[i].normal = normalized(m.previous[i].normal*(1-t) + vertices[i].normal*t);
        }
    }
    m.body.update(vertices);
    m.pose = std::move(vertices);
}

void Pet::review(const AvaliacaoJogada &evaluation) {
    auto &m = *impl;
    const auto &score = evaluation.posicao;
    // O Stockfish fornece centésimos de peão; mate usa uma distância com sinal.
    if (!score.valida || score.valor == 0 ||
        (!score.mate && score.valor > -100 && score.valor < 100)) {
        reset();
        return;
    }
    m.stopped = false;
    m.play(score.valor > 0 ? "gesture-positive" : "gesture-negative", 1.5);
}
void Pet::thinking() {
    impl->stopped = true;
    impl->play("idle", 0);
}
void Pet::finish(ResultadoPartida result) {
    auto &m = *impl;
    switch (result) {
    case ResultadoPartida::Vitoria: m.play("dance", 0); break;
    case ResultadoPartida::Derrota: m.play("gesture-negative", 2); break;
    case ResultadoPartida::Empate: m.play("gesture-positive", 2); break;
    case ResultadoPartida::EmAndamento: return;
    }
    m.stopped = true;
}
void Pet::reset() {
    const bool interrupted = impl->stopped || impl->remaining > 0;
    impl->stopped = false;
    if (!interrupted) return;
    // Encerra a reação e retoma o quadro do passeio anterior à interrupção.
    impl->play(impl->tourClip, 0);
    impl->time = impl->tourClipTime;
    impl->model.sample(impl->tourClip, impl->tourClipTime);
    impl->pose = impl->model.vertices();
    impl->previous.clear();
    impl->transition = 1;
    impl->body.update(impl->pose);
}

void Pet::draw(Shader &shader, const Mat4 &parent) {
    const auto matrix = model(parent);
    shader.object(matrix, {1,1,1});
    shader.modelTexture(impl->texture);
    impl->body.draw();
}
