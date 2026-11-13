#include "Pet.h"
#include "AnimatedModel.h"
#include "stockfish.hpp"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "../../vendor/stb_image.h"
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace {
const char *displayName(const std::string &name) {
    if (name == "Penguin") return "Pinguim";
    if (name == "Deer") return "Cervo";
    if (name == "Ghost") return "Fantasma";
    return name == "Panda" ? "Panda" : "Alien";
}
constexpr int labelWidth = 512, labelHeight = 160;
std::vector<Vertex> plaque() {
    return {{{-1.9f, .015f, 1.35f}, {0,1,0}, {0,1}},
            {{-1.9f, .015f, 2.65f}, {0,1,0}, {0,0}},
            {{ 1.9f, .015f, 2.65f}, {0,1,0}, {1,0}},
            {{-1.9f, .015f, 1.35f}, {0,1,0}, {0,1}},
            {{ 1.9f, .015f, 2.65f}, {0,1,0}, {1,0}},
            {{ 1.9f, .015f, 1.35f}, {0,1,0}, {1,1}}};
}
}

struct Pet::Impl {
    std::string name, title = "Faça sua jogada", detail = "Seu companheiro de partida";
    ImU32 titleColor = IM_COL32(35, 80, 94, 255);
    AnimatedModel model;
    Mesh body, base{Mesh::cube()}, label{plaque()};
    GLuint texture = 0, labelTexture = 0, framebuffer = 0;
    std::string clip = "Idle";
    std::string nextClip;
    double time = 0, remaining = 0;
    float transition = 1;
    std::vector<Vertex> previous;
    bool labelDirty = true;

    explicit Impl(const std::string &id) : name(displayName(id)),
        model((pastaDoExecutavel() / ("assets/objects/pets/" + id + ".gltf")).string()),
        body(model.vertices()) {
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
        glGenTextures(1, &labelTexture);
        glBindTexture(GL_TEXTURE_2D, labelTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, labelWidth, labelHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        GLint old;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old);
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, labelTexture, 0);
        const bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
        glBindFramebuffer(GL_FRAMEBUFFER, old);
        if (!complete) {
            glDeleteFramebuffers(1, &framebuffer);
            glDeleteTextures(1, &labelTexture);
            glDeleteTextures(1, &texture);
            throw std::runtime_error("Falha ao criar placa do pet");
        }
    }
    ~Impl() {
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteTextures(1, &labelTexture);
        glDeleteTextures(1, &texture);
    }
    void play(const std::string &next, double hold) {
        previous = model.vertices();
        clip = next;
        nextClip.clear();
        time = 0;
        remaining = hold;
        transition = 0;
    }
};

Pet::Pet(const std::string &name) : impl(std::make_unique<Impl>(name)) {}
Pet::~Pet() = default;

void Pet::update(float dt) {
    auto &m = *impl;
    dt = std::max(0.f, dt);
    m.time += dt;
    if (m.clip != "Idle") {
        m.remaining -= dt;
        if (m.remaining <= 0) {
            const auto next = m.nextClip;
            m.play(next.empty() ? "Idle" : next, next.empty() ? 0 : 1.5);
        }
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
}

void Pet::review(const AvaliacaoJogada &evaluation) {
    auto &m = *impl;
    m.title = nomeQualidade(evaluation.qualidade);
    m.detail = "Sua jogada: " + evaluation.movimento;
    if (evaluation.posicao.valida) {
        std::ostringstream score;
        if (evaluation.posicao.mate) score << " | Mate " << evaluation.posicao.valor;
        else score << " | " << std::showpos << std::fixed << std::setprecision(2)
                   << evaluation.posicao.valor / 100.0;
        m.detail += score.str();
    }
    m.titleColor = IM_COL32(35, 80, 94, 255);
    switch (evaluation.qualidade) {
    case QualidadeJogada::Melhor: m.play("Dance", 2.5); break;
    case QualidadeJogada::Excelente: m.play("Jump", 1.6); break;
    case QualidadeJogada::Boa: m.play("Yes", 1.5); break;
    case QualidadeJogada::Imprecisao:
        m.titleColor = IM_COL32(151, 105, 22, 255);
        m.play("No", 1.5); break;
    case QualidadeJogada::Erro:
        m.titleColor = IM_COL32(170, 71, 29, 255);
        m.play("HitRecieve", 1.5); break;
    case QualidadeJogada::ErroGrave:
        m.titleColor = IM_COL32(162, 37, 42, 255);
        m.play("No", 1.0);
        m.nextClip = "HitRecieve";
        break;
    default: m.play("Idle", 0); break;
    }
    m.labelDirty = true;
}
void Pet::thinking() {
    impl->title = "Analisando sua jogada...";
    impl->detail = "Observe a reação após o lance";
    impl->titleColor = IM_COL32(35, 80, 94, 255);
    impl->labelDirty = true;
}
void Pet::reset() {
    impl->title = "Faça sua jogada";
    impl->detail = "Seu companheiro de partida";
    impl->titleColor = IM_COL32(35, 80, 94, 255);
    impl->play("Idle", 0);
    impl->labelDirty = true;
}

void Pet::renderLabel() {
    auto &m = *impl;
    if (!m.labelDirty) return;
    auto &io = ImGui::GetIO();
    ImDrawList draw(ImGui::GetDrawListSharedData());
    draw.PushTextureID(io.Fonts->TexID);
    draw.PushClipRect({0,0}, {float(labelWidth),float(labelHeight)});
    draw.AddRectFilled({0,0}, {float(labelWidth),float(labelHeight)}, IM_COL32(245,236,210,255));
    auto text = [&](float y, float size, const std::string &value, ImU32 color) {
        draw.AddText(io.Fonts->Fonts[0], size, {22,y}, color, value.c_str());
    };
    text(14, 26, m.name, IM_COL32(75,68,54,255));
    text(53, 32, m.title, m.titleColor);
    text(111, 23, m.detail, IM_COL32(75,68,54,255));
    draw.PopClipRect();
    draw.PopTextureID();
    ImDrawData data;
    data.Valid = true;
    data.DisplaySize = {float(labelWidth),float(labelHeight)};
    data.FramebufferScale = {1,1};
    data.AddDrawList(&draw);
    GLint oldFramebuffer, viewport[4];
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFramebuffer);
    glGetIntegerv(GL_VIEWPORT, viewport);
    glBindFramebuffer(GL_FRAMEBUFFER, m.framebuffer);
    glViewport(0,0,labelWidth,labelHeight);
    ImGui_ImplOpenGL3_RenderDrawData(&data);
    glBindFramebuffer(GL_FRAMEBUFFER, oldFramebuffer);
    glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);
    glBindTexture(GL_TEXTURE_2D, m.labelTexture);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    m.labelDirty = false;
}

void Pet::draw(Shader &shader, const Mat4 &parent) {
    const auto matrix = model(parent);
    // A base separates the pet from the board and holds the physical review plaque.
    shader.object(matrix * transform({0,-.10f,.8f}, {4.1f,.20f,4.1f}), {.28f,.16f,.08f}, true);
    impl->base.draw();
    shader.object(matrix, {1,1,1});
    shader.modelTexture(impl->texture);
    impl->body.draw();
    shader.object(matrix, {1,1,1});
    shader.modelTexture(impl->labelTexture);
    impl->label.draw();
}
