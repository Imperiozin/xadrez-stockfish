#include "Application.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <algorithm>
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
    return (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) ||
           (promotionPrefix.empty() && notepad &&
            (notepad->capturesPointer() || notepadHit(x, y).body));
}

void Application::updateMouseCursor() {
    if (dragging || !glfwGetWindowAttrib(window, GLFW_FOCUSED) ||
        !glfwGetWindowAttrib(window, GLFW_HOVERED)) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
        return;
    }
    // Os controles do painel definem o próprio cursor e têm prioridade sobre a cena.
    if (ImGui::GetIO().WantCaptureMouse) return;

    bool selectable = false;
    if (!promotionPrefix.empty()) {
        selectable = !finished && !pending.valid() && promotionChoice(mouseX, mouseY) >= 0;
    } else if (notepad && (notepad->capturesPointer() || notepadHit(mouseX, mouseY).body)) {
        selectable = notepad->wantsPointerCursor();
    } else if (!finished && !pending.valid()) {
        const int square = pick(mouseX, mouseY);
        selectable = board->isWhite(square) ||
            std::find(board->destinations.begin(), board->destinations.end(), square) !=
                board->destinations.end();
    }
    if (selectable) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
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

    ImGui::SetNextWindowPos(ImVec2(12, io.DisplaySize.y - 12), ImGuiCond_Always, ImVec2(0, 1));
    ImGui::SetNextWindowSize(ImVec2(std::max(1.f, std::min(600.f, io.DisplaySize.x - 24)), 0));
    ImGui::Begin("Estado da partida###status", nullptr, flags | ImGuiWindowFlags_NoInputs);
    ImGui::PushTextWrapPos(0);
    ImGui::TextUnformatted(status.c_str());
    ImGui::PopTextWrapPos();
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
    if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    ImGui::End();
    ImGui::PopStyleVar(2);
}
