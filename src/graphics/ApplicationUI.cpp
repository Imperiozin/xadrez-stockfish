#include "Application.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <stdexcept>

void Application::initializeInterface() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
    ImFontConfig font;
    font.SizePixels = 17.f;
    io.Fonts->AddFontDefault(&font); // A fonte inclui os acentos do português (Latin-1).

    ImGui::StyleColorsDark();
    auto &style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(16, 12);
    style.FramePadding = ImVec2(10, 6);
    style.ItemSpacing = ImVec2(14, 8);
    style.FrameRounding = 5.f;
    style.WindowRounding = 0.f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(.04f, .06f, .08f, .97f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(.95f, .74f, .36f, 1.f);
    style.Colors[ImGuiCol_Button] = ImVec4(.20f, .32f, .40f, 1.f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(.28f, .44f, .54f, 1.f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(.16f, .27f, .34f, 1.f);

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        throw std::runtime_error("Falha ao inicializar a interface GLFW do Dear ImGui.");
    }
    if (!ImGui_ImplOpenGL3_Init("#version 330 core")) {
        throw std::runtime_error("Falha ao inicializar a interface OpenGL do Dear ImGui.");
    }
    interfaceDifficulty = game.dificuldade();
    interfaceThinkTime = game.tempoPensamento();
}

bool Application::interfaceCapturesMouse(double x, double y) const {
    return ImGui::GetCurrentContext() &&
           (ImGui::GetIO().WantCaptureMouse ||
            (x >= 0 && x < ImGui::GetIO().DisplaySize.x && y >= 0 && y < toolbarHeight));
}

void Application::setDifficulty(int level) {
    if (pending.valid()) return;
    try {
        game.definirDificuldade(std::clamp(level, -20, 20));
        interfaceDifficulty = game.dificuldade();
    } catch (const std::exception &e) {
        status = std::string("Erro ao alterar a dificuldade: ") + e.what();
    }
}

void Application::setThinkTime(int milliseconds) {
    if (pending.valid()) return;
    try {
        game.definirTempoPensamento(std::clamp(milliseconds, 1, 60000));
        interfaceThinkTime = game.tempoPensamento();
    } catch (const std::exception &e) {
        status = std::string("Erro ao alterar o tempo de pensamento: ") + e.what();
    }
}

void Application::drawInterface() {
    const auto &io = ImGui::GetIO();
    const auto &style = ImGui::GetStyle();
    const auto flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                       ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize |
                       ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
    auto newGame = [this] {
        try {
            reset();
        } catch (const std::exception &e) {
            status = std::string("Erro ao iniciar uma nova partida: ") + e.what();
        }
    };

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, 0));
    ImGui::Begin("Controles da partida###toolbar", nullptr, flags);

    // Quebra as opções em mais linhas quando a janela fica estreita.
    auto sameLineIfFits = [&style](float itemWidth) {
        float right = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
        if (ImGui::GetItemRectMax().x + style.ItemSpacing.x + itemWidth <= right) {
            ImGui::SameLine();
        }
    };

    // O worker é o único que acessa Jogo durante a busca do Stockfish.
    ImGui::BeginDisabled(pending.valid());
    if (ImGui::Button("Novo Jogo")) newGame();
    sameLineIfFits(ImGui::CalcTextSize("Dificuldade:").x);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Dificuldade:");
    const struct { const char *label; int level; } levels[] = {
        {"Iniciante", -20}, {"Fácil", -10}, {"Intermediário", 0},
        {"Difícil", 10}, {"Impossível", 20}
    };
    for (const auto &level : levels) {
        sameLineIfFits(ImGui::CalcTextSize(level.label).x + ImGui::GetFrameHeight() + style.ItemInnerSpacing.x);
        if (ImGui::RadioButton(level.label, interfaceDifficulty == level.level)) {
            setDifficulty(level.level);
        }
    }

    ImGui::EndDisabled();
    ImGui::Separator();
    ImGui::PushTextWrapPos(0);
    ImGui::TextUnformatted(status.c_str());
    ImGui::PopTextWrapPos();
    toolbarHeight = ImGui::GetWindowSize().y;
    ImGui::End();

    if (!finished) return;

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * .5f, io.DisplaySize.y * .5f),
                            ImGuiCond_Always, ImVec2(.5f, .5f));
    float resultWidth = std::max(1.f, std::min(560.f, io.DisplaySize.x - 32.f));
    ImGui::SetNextWindowSize(ImVec2(resultWidth, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24, 24));
    ImGui::Begin("Fim da partida###result", nullptr, flags);
    auto centeredText = [](const std::string &text, const ImVec4 &color) {
        float available = ImGui::GetContentRegionAvail().x;
        float textWidth = ImGui::CalcTextSize(text.c_str(), nullptr, false, available).x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.f, (available - textWidth) * .5f));
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + std::min(available, textWidth));
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextUnformatted(text.c_str());
        ImGui::PopStyleColor();
        ImGui::PopTextWrapPos();
    };
    centeredText("Fim da partida", ImVec4(.95f, .74f, .36f, 1.f));
    ImGui::Spacing();
    centeredText(resultTitle, style.Colors[ImGuiCol_Text]);
    ImGui::Spacing();
    centeredText(resultReason, style.Colors[ImGuiCol_TextDisabled]);
    ImGui::Spacing();
    const float buttonWidth = 150.f;
    ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), (resultWidth - buttonWidth) * .5f));
    if (ImGui::Button("Novo Jogo###resultNewGame", ImVec2(buttonWidth, 0))) newGame();
    ImGui::End();
    ImGui::PopStyleVar(2);
}
