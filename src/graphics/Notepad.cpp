#include "Notepad.h"
#include "stockfish.hpp"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include <assimp/Importer.hpp>
#include <assimp/config.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cfloat>
#include <cstring>
#include <map>
#include <stdexcept>

namespace {
// Restaura o contexto do HUD mesmo se uma operação falhar.
struct ContextScope {
    ImGuiContext *previous = ImGui::GetCurrentContext();
    explicit ContextScope(ImGuiContext *context) { ImGui::SetCurrentContext(context); }
    ~ContextScope() { ImGui::SetCurrentContext(previous); }
};
constexpr Vec3 paperColor{.94f, .925f, .86f};
constexpr float handwritingSize = 94.f;
}

Notepad::Notepad() { load(); }

void Notepad::load() {
    const auto path = (pastaDoExecutavel() / "assets/objects/NotepadwithRingbindings.fbx").string();
    Assimp::Importer importer;
    // Mantém papel e espiral separados, embora usem o mesmo atlas no FBX.
    importer.SetPropertyBool(AI_CONFIG_PP_PTV_KEEP_HIERARCHY, true);
    const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate |
        aiProcess_PreTransformVertices | aiProcess_GenSmoothNormals | aiProcess_ValidateDataStructure);
    if (!scene || !scene->HasMeshes() || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE))
        throw std::runtime_error("Falha ao importar o bloco de notas: " + std::string(importer.GetErrorString()));

    Vec3 low{FLT_MAX, FLT_MAX, FLT_MAX}, high{-FLT_MAX, -FLT_MAX, -FLT_MAX};
    for (unsigned i = 0; i < scene->mNumMeshes; ++i) {
        const auto &mesh = *scene->mMeshes[i];
        if (!mesh.HasNormals()) throw std::runtime_error("Bloco de notas sem normais.");
        Part part;
        for (unsigned f = 0; f < mesh.mNumFaces; ++f) {
            const auto &face = mesh.mFaces[f];
            if (face.mNumIndices != 3) continue;
            for (unsigned j = 0; j < 3; ++j) {
                const unsigned index = face.mIndices[j];
                const auto &p = mesh.mVertices[index], &n = mesh.mNormals[index];
                if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z) ||
                    !std::isfinite(n.x) || !std::isfinite(n.y) || !std::isfinite(n.z))
                    throw std::runtime_error("Coordenadas inválidas no bloco de notas.");
                Vec2 uv{};
                if (mesh.HasTextureCoords(0)) uv = {mesh.mTextureCoords[0][index].x, mesh.mTextureCoords[0][index].y};
                part.vertices.push_back({{p.x, p.y, p.z}, {n.x, n.y, n.z}, uv});
                low = {std::min(low.x, p.x), std::min(low.y, p.y), std::min(low.z, p.z)};
                high = {std::max(high.x, p.x), std::max(high.y, p.y), std::max(high.z, p.z)};
            }
        }
        if (!part.vertices.empty()) parts.push_back(std::move(part));
    }
    if (parts.empty() || high.x <= low.x || high.z <= low.z)
        throw std::runtime_error("Geometria vazia no bloco de notas.");

    // Normalização própria para o objeto; não usa o limite de largura das peças.
    // A meia-volta coloca a espiral no topo da página vista pelo jogador.
    const float factor = 6.1f / (high.x - low.x);
    const Vec3 origin{(low.x + high.x) * .5f, low.y, (low.z + high.z) * .5f};
    boundsLow = {-(high.x - origin.x) * factor, 0, -(high.z - origin.z) * factor};
    boundsHigh = {-(low.x - origin.x) * factor, (high.y - origin.y) * factor,
                  -(low.z - origin.z) * factor};
    struct Plane {
        float area = 0, y = 0;
        Vec3 low{FLT_MAX, FLT_MAX, FLT_MAX}, high{-FLT_MAX, -FLT_MAX, -FLT_MAX};
    };
    std::map<int, Plane> planes;
    for (auto &part : parts) {
        Vec3 partLow{FLT_MAX, FLT_MAX, FLT_MAX}, partHigh{-FLT_MAX, -FLT_MAX, -FLT_MAX};
        for (auto &vertex : part.vertices) {
            auto p = (vertex.position - origin) * factor;
            vertex.position = {-p.x, p.y, -p.z};
            vertex.normal.x *= -1;
            vertex.normal.z *= -1;
            p = vertex.position;
            partLow = {std::min(partLow.x, p.x), std::min(partLow.y, p.y), std::min(partLow.z, p.z)};
            partHigh = {std::max(partHigh.x, p.x), std::max(partHigh.y, p.y), std::max(partHigh.z, p.z)};
        }
        // A espiral ocupa uma faixa curta no eixo longitudinal deste modelo.
        part.ruled = partHigh.z - partLow.z >= (high.z - low.z) * factor * .25f;
        part.color = part.ruled ? paperColor : Vec3{.095f, .105f, .115f};
        for (size_t i = 0; i < part.vertices.size(); i += 3) {
            const auto a = part.vertices[i].position, b = part.vertices[i + 1].position,
                       c = part.vertices[i + 2].position;
            const auto normal = cross(b - a, c - a);
            if (normal.y <= 0 || std::abs(a.y - b.y) > .0001f || std::abs(a.y - c.y) > .0001f) continue;
            auto &plane = planes[int(std::round(a.y * 10000))];
            plane.area += normal.y * .5f;
            plane.y = a.y;
            for (auto p : {a, b, c}) {
                plane.low = {std::min(plane.low.x, p.x), p.y, std::min(plane.low.z, p.z)};
                plane.high = {std::max(plane.high.x, p.x), p.y, std::max(plane.high.z, p.z)};
            }
        }
        part.mesh = std::make_unique<Mesh>(part.vertices);
    }
    if (planes.empty()) throw std::runtime_error("O bloco de notas não possui uma área plana para a interface.");
    const auto best = std::max_element(planes.begin(), planes.end(),
        [](const auto &a, const auto &b) { return a.second.area < b.second.area; });
    const auto &plane = best->second;
    // A projeção cobre a folha até a borda; tinta e cliques acompanham a malha curva.
    page.topLeft = {plane.low.x + .20f, plane.y, plane.low.z + .35f};
    page.bottomRight = {plane.high.x - .20f, page.topLeft.y, plane.high.z - .20f};
    if (page.bottomRight.x <= page.topLeft.x || page.bottomRight.z <= page.topLeft.z)
        throw std::runtime_error("Área útil insuficiente na folha do bloco de notas.");
}

Notepad::~Notepad() {
    if (context) {
        {
            ContextScope scope(context);
            if (ImGui::GetIO().BackendRendererUserData) ImGui_ImplOpenGL3_Shutdown();
        }
        ImGui::DestroyContext(context);
    }
    if (texture) glDeleteTextures(1, &texture);
    if (framebuffer) glDeleteFramebuffers(1, &framebuffer);
}

void Notepad::initializeInterface() {
    auto *previous = ImGui::GetCurrentContext();
    context = ImGui::CreateContext();
    ImGui::SetCurrentContext(previous);
    ContextScope scope(context);
    auto &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.DisplaySize = ImVec2(canvasWidth, canvasHeight);
    const auto fontPath = pastaDoExecutavel() / "assets/fonts/Schoolbell-Regular.ttf";
    if (!arquivos::is_regular_file(fontPath))
        throw std::runtime_error("Fonte manuscrita não encontrada: " + fontPath.string());
    ImFontConfig font;
    font.OversampleH = 3;
    font.OversampleV = 2;
    if (!io.Fonts->AddFontFromFileTTF(fontPath.string().c_str(), handwritingSize, &font,
                                   io.Fonts->GetGlyphRangesDefault()))
        throw std::runtime_error("Falha ao carregar a fonte manuscrita do bloco de notas.");
    ImGui::StyleColorsLight();
    auto &style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(0, 0);
    style.WindowBorderSize = 0;
    style.Colors[ImGuiCol_Text] = ImVec4(.12f, .18f, .24f, 1);
    if (!ImGui_ImplOpenGL3_Init("#version 330 core"))
        throw std::runtime_error("Falha ao inicializar a interface do bloco de notas.");

    GLint oldFramebuffer, oldTexture;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFramebuffer);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTexture);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, canvasWidth, canvasHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // Preserva a nitidez horizontal quando a folha é vista em ângulo rasante.
    GLint extensionCount = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);
    for (GLint i = 0; i < extensionCount; ++i) {
        const auto *name = reinterpret_cast<const char *>(glGetStringi(GL_EXTENSIONS, GLuint(i)));
        if (name && (std::strcmp(name, "GL_EXT_texture_filter_anisotropic") == 0 ||
                     std::strcmp(name, "GL_ARB_texture_filter_anisotropic") == 0)) {
            constexpr GLenum textureMaxAnisotropy = 0x84FE, maxTextureMaxAnisotropy = 0x84FF;
            GLfloat maximum = 1;
            glGetFloatv(maxTextureMaxAnisotropy, &maximum);
            glTexParameterf(GL_TEXTURE_2D, textureMaxAnisotropy, std::min(8.f, maximum));
            break;
        }
    }
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    const bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, oldFramebuffer);
    glBindTexture(GL_TEXTURE_2D, oldTexture);
    if (!complete) throw std::runtime_error("Falha ao criar a textura da folha do bloco de notas.");
}

Notepad::Hit Notepad::hit(const Ray &ray, const Mat4 &parent) const {
    Hit result;
    Mat4 inverse;
    if (!inverseAffine(model(parent), inverse)) return result;
    const auto local = transformRay(inverse, ray);
    float boundsDistance;
    if (!intersectBox(local, boundsLow, boundsHigh, boundsDistance)) return result;
    for (const auto &part : parts) {
        for (size_t i = 0; i < part.vertices.size(); i += 3) {
            float distance;
            const auto a = part.vertices[i].position, b = part.vertices[i + 1].position,
                       c = part.vertices[i + 2].position;
            if (intersectTriangle(local, a, b, c, distance) && distance < result.distance) {
                result.body = true;
                result.distance = distance;
                result.page = false;
                const auto point = local.origin + local.direction * distance;
                const auto normal = cross(b - a, c - a);
                const Vec2 uv{(point.x - page.topLeft.x) / (page.bottomRight.x - page.topLeft.x),
                              (point.z - page.topLeft.z) / (page.bottomRight.z - page.topLeft.z)};
                if (part.ruled && point.y >= page.topLeft.y - .0005f &&
                    normal.y > 0 && dot(normal, local.direction) < 0 &&
                    uv.x >= 0 && uv.x <= 1 && uv.y >= 0 && uv.y <= 1) {
                    result.page = true;
                    result.pixel = {uv.x * canvasWidth, uv.y * canvasHeight};
                }
            }
        }
    }
    return result;
}

void Notepad::pointer(const Hit &hit, bool enabled) {
    if (!context) return;
    pointerOnPage = enabled && hit.page;
    ContextScope scope(context);
    ImGui::GetIO().AddMousePosEvent(enabled && hit.page ? hit.pixel.x : -FLT_MAX,
                                  enabled && hit.page ? hit.pixel.y : -FLT_MAX);
}

void Notepad::mouseButton(bool down) {
    if (!context) return;
    ContextScope scope(context);
    auto &io = ImGui::GetIO();
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, down);
    pressed = down && pointerOnPage;
}

void Notepad::focus(bool focused) {
    if (!context) return;
    ContextScope scope(context);
    ImGui::GetIO().AddFocusEvent(focused);
    if (!focused) pressed = pointerOnPage = false;
}

Notepad::Action Notepad::renderInterface(float dt, int difficulty, bool disabled) {
    ContextScope scope(context);
    auto &io = ImGui::GetIO();
    io.DeltaTime = std::max(dt, .001f);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
    pointerOnControl = false;
    Action action;
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("Configurações da partida###notepad", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    auto *draw = ImGui::GetForegroundDrawList();
    const ImU32 ink = IM_COL32(35, 49, 65, 255);
    const ImU32 blue = IM_COL32(35, 91, 145, 255);
    // Alinha o texto pela linha de base da fonte.
    auto write = [&](ImVec2 baseline, ImU32 color, const char *text, float size) {
        auto *font = ImGui::GetFont();
        baseline.y -= font->Ascent * (size / font->FontSize);
        draw->AddText(font, size, baseline, color, text);
    };
    write(ImVec2(30, 54), ink, "Dificuldade", 100);
    // As pautas pertencem ao material da folha e continuam além da área da UI.
    const struct { const char *label; int level; } levels[] = {
        {"Iniciante", -20}, {"Fácil", -10}, {"Intermediário", 0}, {"Difícil", 10}, {"Impossível", 20}
    };
    ImGui::BeginDisabled(disabled);
    for (int i = 0; i < 5; ++i) {
        const float y = 96.f + i * 76.f;
        ImGui::SetCursorPos(ImVec2(18, y));
        ImGui::PushID(i);
        if (ImGui::InvisibleButton("Escolher dificuldade", ImVec2(732, 76))) {
            action.difficulty = levels[i].level;
            action.changeDifficulty = true;
        }
        const bool hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);
        if (hovered && !disabled) pointerOnControl = true;
        const auto color = hovered ? blue : ink;
        draw->AddCircle(ImVec2(48, y + 36), 17, color, 24, 3);
        if (difficulty == levels[i].level) draw->AddCircleFilled(ImVec2(48, y + 36), 9, color);
        write(ImVec2(100, y + 48), color, levels[i].label, handwritingSize);
        if (hovered) draw->AddLine(ImVec2(100, y + 68), ImVec2(730, y + 68), color, 2);
        ImGui::PopID();
    }
    auto rightAlignedButton = [&](const char *label, float y) {
        const float width = ImGui::GetFont()->CalcTextSizeA(handwritingSize, FLT_MAX, 0, label).x;
        const float x = 730.f - width;
        ImGui::SetCursorPos(ImVec2(x - 12.f, y));
        const bool clicked = ImGui::InvisibleButton(label, ImVec2(width + 24.f, 76));
        const bool hovered = ImGui::IsItemHovered();
        if (hovered) pointerOnControl = true;
        const auto color = hovered ? blue : ink;
        write(ImVec2(x, y + 48), color, label, handwritingSize);
        if (hovered) draw->AddLine(ImVec2(x, y + 68), ImVec2(730, y + 68), color, 2);
        return clicked;
    };
    action.newGame = rightAlignedButton("Nova partida", 476);
    ImGui::EndDisabled();
    action.close = rightAlignedButton("Fechar", canvasHeight - 88.f);
    ImGui::End();
    ImGui::Render();

    GLint oldFramebuffer, viewport[4];
    GLfloat clearColor[4];
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFramebuffer);
    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetFloatv(GL_COLOR_CLEAR_VALUE, clearColor);
    const bool scissor = glIsEnabled(GL_SCISSOR_TEST) == GL_TRUE;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, canvasWidth, canvasHeight);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    // Envia somente a tinta: nenhuma decoração ou fundo de janela vai para o papel.
    const auto *frame = ImGui::GetDrawData();
    ImDrawData inkData;
    inkData.Valid = true;
    inkData.DisplayPos = frame->DisplayPos;
    inkData.DisplaySize = frame->DisplaySize;
    inkData.FramebufferScale = frame->FramebufferScale;
    inkData.AddDrawList(draw);
    ImGui_ImplOpenGL3_RenderDrawData(&inkData);
    glBindFramebuffer(GL_FRAMEBUFFER, oldFramebuffer);
    // Filtra a redução da página em perspectiva para evitar falhas nos traços.
    GLint oldTexture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTexture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, oldTexture);
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
    if (scissor) glEnable(GL_SCISSOR_TEST);
    return action;
}

void Notepad::draw(Shader &shader, const Mat4 &parent) {
    const auto matrix = model(parent);
    const Vec2 rulingOrigin{page.topLeft.x, page.topLeft.z};
    const Vec2 rulingScale{canvasWidth / (page.bottomRight.x - page.topLeft.x),
                          canvasHeight / (page.bottomRight.z - page.topLeft.z)};
    for (auto &part : parts) {
        shader.object(matrix, part.color);
        if (part.ruled) {
            shader.ruledPaper(rulingOrigin, rulingScale);
            // Imprime a tinta na própria malha, preservando sua cor, normais e sombras.
            if (texture) shader.pageTexture(texture, page.topLeft.y);
        }
        part.mesh->draw();
    }
}
