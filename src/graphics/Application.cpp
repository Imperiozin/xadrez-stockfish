#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#include <assimp/defs.h>
#undef GLAD_GL_IMPLEMENTATION
#include "Application.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <chrono>
#include <iostream>
#include <random>
#include <stdexcept>

namespace {

std::string squareName(int s) {
    return {char('a' + s % 8), char('1' + s / 8)};
}

int squareIndex(const std::string &s) {
    return (s[1] - '1') * 8 + s[0] - 'a';
}

void require(bool condition, const char *message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

} // namespace

Application::~Application() {
    if (pending.valid()) {
        pending.wait();
    }

    pet.reset();
    table.reset();
    notepad = nullptr;
    if (ImGui::GetCurrentContext()) {
        if (ImGui::GetIO().BackendRendererUserData) ImGui_ImplOpenGL3_Shutdown();
        if (ImGui::GetIO().BackendPlatformUserData) ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    for (auto &piece : promotionPieces) piece.reset();
    board.reset();
    primitives.reset();
    shader.reset();
    depthShader.reset();
    skybox.reset();
    if (shadowMap) glDeleteTextures(1, &shadowMap);
    if (shadowFramebuffer) glDeleteFramebuffers(1, &shadowFramebuffer);
    if (secondaryShadowMap) glDeleteTextures(1, &secondaryShadowMap);
    if (secondaryShadowFramebuffer) glDeleteFramebuffers(1, &secondaryShadowFramebuffer);
    if (window) {
        glfwDestroyWindow(window);
    }

    glfwTerminate();
}

Vec3 Application::eye() const {
    return cameraTarget + Vec3{distance * std::cos(pitch) * std::sin(yaw), distance * std::sin(pitch),
            distance * std::cos(pitch) * std::cos(yaw)};
}

Ray Application::pointerRay(double x, double y) const {
    int w, h;
    glfwGetWindowSize(window, &w, &h);
    const auto e = eye();
    if (w <= 0 || h <= 0) return {e, {0, 1, 0}};
    const auto forward = normalized(cameraTarget - e);
    const auto right = normalized(cross(forward, {0, 1, 0}));
    const auto up = cross(right, forward);
    const float nx = float(2 * x / w - 1), ny = float(1 - 2 * y / h);
    const float tangent = std::tan(.65f / 2);
    return {e, normalized(forward + right * (nx * float(w) / h * tangent) + up * (ny * tangent))};
}

Notepad::Hit Application::notepadHit(double x, double y) const {
    if (!notepad || !table) return {};
    int w, h;
    glfwGetWindowSize(window, &w, &h);
    if (x < 0 || y < 0 || x >= w || y >= h) return {};
    const auto ray = pointerRay(x, y);
    const auto parent = table->model(Mat4::identity());
    auto hit = notepad->hit(ray, parent);
    if (hit.body && board) {
        Mat4 boardInverse;
        if (inverseAffine(board->model(Mat4::identity()), boardInverse)) {
            const auto local = transformRay(boardInverse, ray);
            float distance;
            if ((intersectBox(local, {-4.6f, -.705f, -4.6f}, {4.6f, -.055f, 4.6f}, distance) &&
                 distance < hit.distance) ||
                (intersectBox(local, {-4, -.03f, -4}, {4, .03f, 4}, distance) &&
                 distance < hit.distance)) return {};
        }
    }
    // A mesa deve ocultar os controles ao observar o bloco por baixo.
    Mat4 inverse;
    if (hit.body && inverseAffine(parent, inverse)) {
        const auto local = transformRay(inverse, ray);
        if (std::abs(local.direction.y) > .000001f) {
            const float t = (Table::surfaceY - local.origin.y) / local.direction.y;
            const auto p = local.origin + local.direction * t;
            if (t > 0 && t < hit.distance && std::abs(p.x) <= Table::width / 2 &&
                std::abs(p.z) <= Table::depth / 2) return {};
        }
    }
    return hit;
}

void Application::updateNotepadPointer(double x, double y) {
    if (notepad) notepad->pointer(notepadHit(x, y), promotionPrefix.empty() && !dragging &&
        !(ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse));
}

void Application::drawNotepadInterface(float dt) {
    if (!notepad) return;
    updateNotepadPointer(mouseX, mouseY);
    const auto action = notepad->renderInterface(dt, interfaceDifficulty,
        pending.valid() || finished || !promotionPrefix.empty());
    if (action.close) glfwSetWindowShouldClose(window, GLFW_TRUE);
    if (action.changeDifficulty) setDifficulty(action.difficulty);
    if (action.newGame) {
        try { reset(); }
        catch (const std::exception &e) {
            status = std::string("Erro ao iniciar uma nova partida: ") + e.what();
        }
    }
}

void Application::refresh(const std::string &move) {
    cachedFen = game.fen();
    legal = game.movimentosValidos();
    if (move.empty()) board->sync(cachedFen);
    else board->animateMove(move, cachedFen);
    board->selected = -1;
    board->destinations.clear();
    board->captures.clear();
    board->castling.clear();
}

int Application::pick(double x, double y) const {
    const auto pointer = pointerRay(x, y);
    const auto e = pointer.origin, ray = pointer.direction;
    if (ray.y >= -.0001f) {
        return -1;
    }

    Vec3 p = e + ray * (-e.y / ray.y);
    if (p.x < -4 || p.x >= 4 || p.z < -4 || p.z >= 4) {
        return -1;
    }
    return int(std::floor(4 - p.z)) * 8 + int(std::floor(p.x + 4));
}

void Application::click(int square) {
    if (pending.valid() || board->isAnimating() || finished || !promotionPrefix.empty()) {
        return;
    }
    if (square < 0) {
        cancelSelection();
        return;
    }

    if (board->selected >= 0) {
        std::string prefix = squareName(board->selected) + squareName(square);
        for (auto &move : legal) {
            if (move.compare(0, 4, prefix) == 0) {
                if (move.size() == 5) {
                    promotionPrefix = prefix;
                    status = "Promoção: clique em rainha, torre, bispo ou cavalo";
                    return;
                }
                submit(move);
                return;
            }
        }
    }

    board->selected = board->isWhite(square) ? square : -1;
    board->destinations.clear();
    board->captures.clear();
    board->castling.clear();

    if (board->selected >= 0) {
        chess::Board position(cachedFen);
        chess::Movelist moves;
        chess::movegen::legalmoves(moves, position);
        for (auto move : moves) {
            if (move.from().index() != square) continue;
            int destination = move.to().index();
            if (move.typeOf() == chess::Move::CASTLING) {
                destination = square / 8 * 8 + (destination > square ? 6 : 2);
                board->castling.push_back(destination);
            } else if (position.isCapture(move)) {
                // En passant destaca a base do peao que sera capturado.
                board->captures.push_back(move.typeOf() == chess::Move::ENPASSANT
                    ? destination - 8 : destination);
            }
        }
        auto name = squareName(square);
        for (auto &move : legal) {
            if (move.compare(0, 2, name) == 0) {
                board->destinations.push_back(squareIndex(move.substr(2, 2)));
            }
        }
        status = "Selecionada " + name + ": clique em um destino verde";
    } else {
        status = "Selecione uma peça branca";
    }
}

void Application::submit(const std::string &move) {
    // A cena usa uma copia: apenas a tarefa em segundo plano acessa o jogo.
    chess::Board preview(cachedFen);
    chess::Movelist moves;
    chess::movegen::legalmoves(moves, preview);
    for (auto m : moves) {
        if (chess::uci::moveToUci(m) == move) {
            preview.makeMove(m);
            break;
        }
    }

    board->animateMove(move, preview.getFen());
    board->selected = -1;
    board->destinations.clear();
    board->captures.clear();
    board->castling.clear();
    promotionPrefix.clear();
    status = "Stockfish pensando...";
    pet->thinking();
    pending = std::async(std::launch::async, [this, move] {
        return game.fazerMovimento(move);
    });
}

void Application::update() {
    if (board->isAnimating() || !pending.valid() ||
        pending.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
        return;
    }
    std::string reply;
    try {
        auto response = pending.get();
        if (response) {
            reply = response->movimento;
            finished = response->fimPartida;
            resultTitle = response->resultado;
            resultReason = response->motivo;
            lastReview = response->avaliacao;
            lastOutcome = response->desfecho;
            pet->review(lastReview);
            pet->finish(lastOutcome);
            status = finished ? response->resultado : "Sua vez | Stockfish: " + response->movimento;
        } else {
            status = "Lance inválido. Tente novamente.";
            pet->reset();
        }
    } catch (const std::exception &e) {
        status = std::string("Erro do motor: ") + e.what();
        pet->reset();
    }
    refresh(reply);
}

void Application::reset() {
    if (pending.valid()) {
        return;
    }
    game.reiniciar();
    lastReview = {};
    lastOutcome = ResultadoPartida::EmAndamento;
    pet->reset();
    finished = false;
    resultTitle.clear();
    resultReason.clear();
    promotionPrefix.clear();
    status = "Nova partida: brancas jogam";
    refresh();
}

void Application::key(int k) {
    if (k == GLFW_KEY_ESCAPE) {
        if (!promotionPrefix.empty()) {
            cancelSelection();
        } else {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        return;
    }
    if (k == GLFW_KEY_HOME) {
        yaw = 0;
        pitch = .65f;
        distance = 32;
        cameraTarget = {};
        return;
    }
    if (pending.valid()) {
        return;
    }
    try {
        if (!promotionPrefix.empty()) {
            return;
        }
        // if (k == GLFW_KEY_N) {
        //     reset();
        // }
        // if (k == GLFW_KEY_EQUAL || k == GLFW_KEY_MINUS || k == GLFW_KEY_KP_ADD || k == GLFW_KEY_KP_SUBTRACT) {
        //     setDifficulty(interfaceDifficulty + ((k == GLFW_KEY_EQUAL || k == GLFW_KEY_KP_ADD) ? 1 : -1));
        // }
        // if (k == GLFW_KEY_PAGE_UP || k == GLFW_KEY_PAGE_DOWN) {
        //     setThinkTime(interfaceThinkTime + (k == GLFW_KEY_PAGE_UP ? 100 : -100));
        // }
    } catch (const std::exception &e) {
        status = e.what();
    }
}

void Application::render() {
    glfwGetFramebufferSize(window, &width, &height);
    if (width < 1 || height < 1) {
        return;
    }
    frameScenario = scenario.sample(glfwGetTime() - scenarioStart, environmentSeed);
    const Vec3 lightPosition = frameScenario.lightPosition;
    const Mat4 lightViewProjection = frameScenario.lightViewProjection();
    const bool secondaryShadows = frameScenario.animation == Scenario::Animation::BinarySuns ||
        (frameScenario.animation == Scenario::Animation::Storm && frameScenario.secondaryLightIntensity > 0);
    Mat4 secondaryProjection;
    if (secondaryShadows) secondaryProjection = frameScenario.lightViewProjection(true);

    // Primeira passagem: somente profundidade, vista pela luz.
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glViewport(0, 0, shadowResolution, shadowResolution);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);
    glClear(GL_DEPTH_BUFFER_BIT);
    depthShader->use(lightViewProjection, lightPosition);
    drawScene(*depthShader);
    if (secondaryShadows) {
        glViewport(0, 0, shadowResolution / 2, shadowResolution / 2);
        glBindFramebuffer(GL_FRAMEBUFFER, secondaryShadowFramebuffer);
        glClear(GL_DEPTH_BUFFER_BIT);
        depthShader->use(secondaryProjection, frameScenario.secondaryLightPosition);
        drawScene(*depthShader);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Segunda passagem: iluminação com consulta ao mapa de sombras.
    glViewport(0, 0, width, height);
    glClearColor(.035f, .055f, .075f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shader->use(perspective(float(width) / height) * lookAt(eye(), cameraTarget), eye());
    shader->lighting(frameScenario);
    shader->shadows(lightViewProjection, lightPosition, shadowMap);
    if (secondaryShadows) shader->shadows(secondaryProjection, frameScenario.secondaryLightPosition, secondaryShadowMap, true);
    drawScene(*shader);
    skybox->draw(eye(), cameraTarget, float(width) / height, frameScenario);
    if (!promotionPrefix.empty()) drawPromotion();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Application::initializeInput() {
    // Eventos de mouse, câmera e teclado.
    glfwSetWindowUserPointer(window, this);
    glfwSetWindowFocusCallback(window, [](GLFWwindow *w, int focused) {
        auto &a = *static_cast<Application *>(glfwGetWindowUserPointer(w));
        if (a.notepad) a.notepad->focus(focused == GLFW_TRUE);
        if (!focused) a.dragging = false;
    });
    glfwSetMouseButtonCallback(window, [](GLFWwindow *w, int button, int action, int) {
        auto &a = *static_cast<Application *>(glfwGetWindowUserPointer(w));
        double x, y;
        glfwGetCursorPos(w, &x, &y);
        // O bloco recebe o botão esquerdo; o botão do meio pertence à câmera.
        if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
            if (action == GLFW_RELEASE) a.dragging = false;
            else if (action == GLFW_PRESS &&
                     !(ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse)) {
                a.dragging = true;
                a.mouseX = x;
                a.mouseY = y;
            }
            a.updateNotepadPointer(x, y);
            return;
        }
        a.updateNotepadPointer(x, y);
        if (button == GLFW_MOUSE_BUTTON_LEFT && a.notepad)
            a.notepad->mouseButton(action == GLFW_PRESS);
        if (a.dragging || a.interfaceCapturesMouse(x, y)) return;
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            if (!a.promotionPrefix.empty()) {
                int choice = a.promotionChoice(x, y);
                if (choice >= 0) a.submit(a.promotionPrefix + "qrbn"[choice]);
            } else a.click(a.pick(x, y));
        }
        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) a.cancelSelection();
    });

    glfwSetCursorPosCallback(window, [](GLFWwindow *w, double x, double y) {
        auto &a = *static_cast<Application *>(glfwGetWindowUserPointer(w));
        // Um arraste já iniciado continua ao atravessar a folha ou um painel.
        if (a.dragging) {
            a.yaw -= float(x - a.mouseX) * .006f;
            a.pitch = std::clamp(a.pitch + float(y - a.mouseY) * .006f, -.20f, 1.53f);
            a.constrainCamera();
            a.board->hovered = -1;
            a.mouseX = x;
            a.mouseY = y;
            a.updateNotepadPointer(x, y);
            return;
        }
        a.updateNotepadPointer(x, y);
        if (a.interfaceCapturesMouse(x, y)) {
            a.board->hovered = -1;
            a.mouseX = x;
            a.mouseY = y;
            return;
        }
        a.constrainCamera();
        a.board->hovered = !a.finished && a.promotionPrefix.empty() ? a.pick(x, y) : -1;
        a.mouseX = x;
        a.mouseY = y;
    });

    glfwSetScrollCallback(window, [](GLFWwindow *w, double, double y) {
        auto &a = *static_cast<Application *>(glfwGetWindowUserPointer(w));
        if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) return;
        a.distance = std::clamp(a.distance - float(y), 4.f, 45.f);
        a.constrainCamera();
    });

    glfwSetKeyCallback(window, [](GLFWwindow *w, int key, int, int action, int) {
        if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureKeyboard) return;
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            static_cast<Application *>(glfwGetWindowUserPointer(w))->key(key);
        }
    });
}

int Application::run() {
    // Inicialização da janela e do contexto OpenGL.
    glfwSetErrorCallback([](int, const char *message) {
        std::cerr << "GLFW: " << message << '\n';
    });
    require(glfwInit() == GLFW_TRUE, "Falha ao inicializar GLFW");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    auto *monitor = glfwGetPrimaryMonitor();
    require(monitor != nullptr, "Não foi possível encontrar o monitor principal");
    const auto *videoMode = glfwGetVideoMode(monitor);
    require(videoMode != nullptr, "Não foi possível consultar a resolução do monitor principal");
    int monitorX, monitorY;
    glfwGetMonitorPos(monitor, &monitorX, &monitorY);
    // Janela sem bordas ocupa o monitor, mantendo o modo de vídeo do desktop.
    window = glfwCreateWindow(videoMode->width, videoMode->height, "Xadrez 3D", nullptr, nullptr);
    require(window != nullptr, "Falha ao criar janela OpenGL 3.3");
    glfwSetWindowPos(window, monitorX, monitorY);
    glfwMakeContextCurrent(window);
    require(gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)) != 0,
            "Falha ao carregar OpenGL");
    glfwSwapInterval(1);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    // Recursos graficos e estado inicial da cena.
    scenario = Scenario::load(scenarioFile.empty() ? Scenario::fileForDifficulty(game.dificuldade()) : scenarioFile);
    environmentSeed = std::random_device{}();
    frameScenario = scenario.sample(0, environmentSeed);
    shader = std::make_unique<Shader>();
    initializeShadows();
    if (scenario.animation == Scenario::Animation::BinarySuns ||
        scenario.animation == Scenario::Animation::Storm) initializeShadows(true);
    skybox = std::make_unique<Skybox>(scenario.skybox);
    primitives = std::make_unique<Primitives>();
    board = std::make_unique<Board>(*primitives);
    table = std::make_unique<Table>();
    pet = makePet(scenario);
    auto pad = std::make_unique<Notepad>();
    // Único ponto de configuração do bloco: modelo, texto e cliques seguem juntos.
    pad->position = {7.3f, Table::surfaceY + .025f, 3.7f};
    pad->rotation = {0, -.10f - 10.f * AI_MATH_PI_F / 180.f, 0}; // Mais 10° no sentido horário.
    pad->scale = {1, 1, 1};
    notepad = pad.get();
    table->addProp(std::move(pad));
    const char symbols[] = "QRBN";
    for (int i = 0; i < 4; ++i) promotionPieces[i] = Piece::create(symbols[i], *primitives);
    refresh();

    initializeInput();

    // O backend encadeia os callbacks acima e recebe todos os eventos da interface.
    initializeInterface();
    notepad->initializeInterface();
    scenarioStart = glfwGetTime();
    lastFrame = scenarioStart;
    glfwShowWindow(window);

    // Loop principal: atualiza o jogo, desenha a cena e processa eventos.
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        double now = glfwGetTime();
        float dt = float(std::min(now - lastFrame, .05));
        lastFrame = now;
        board->update(dt);
        pet->update(dt);
        update();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        moveCamera(dt);
        drawNotepadInterface(dt);
        drawInterface();
        updateMouseCursor();
        ImGui::Render();
        render();
        glfwSwapBuffers(window);
    }
    return 0;
}

void Application::cancelSelection() {
    promotionPrefix.clear();
    board->selected = -1;
    board->destinations.clear();
    board->captures.clear();
    board->castling.clear();
    if (!pending.valid() && !board->isAnimating() && !finished)
        status = "Selecione uma peça branca";
}

void Application::constrainCamera() {
    Vec3 e = eye();
    Vec3 bounded{std::clamp(e.x, -45.f, 45.f), std::clamp(e.y, -7.f, 40.f),
                 std::clamp(e.z, -45.f, 45.f)};
    cameraTarget = cameraTarget + bounded - e;
}

void Application::moveCamera(float dt) {
    auto pressed = [this](int k) {
        return glfwGetKey(window, k) == GLFW_PRESS;
    };
    float horizontal = float(pressed(GLFW_KEY_RIGHT)) - float(pressed(GLFW_KEY_LEFT));
    float vertical = float(pressed(GLFW_KEY_UP)) - float(pressed(GLFW_KEY_DOWN));
    Vec3 right{std::cos(yaw), 0, -std::sin(yaw)};
    Vec3 forward{-std::sin(yaw), 0, -std::cos(yaw)};
    Vec3 movement = right * horizontal;
    if (pressed(GLFW_KEY_LEFT_SHIFT) || pressed(GLFW_KEY_RIGHT_SHIFT)) movement.y = vertical;
    else movement = movement + forward * vertical;
    if (dot(movement, movement) > 0) {
        cameraTarget = cameraTarget + normalized(movement) * (dt * 6.f);
        constrainCamera();
    }
    glfwGetCursorPos(window, &mouseX, &mouseY);
    board->hovered = !finished && promotionPrefix.empty() && !interfaceCapturesMouse(mouseX, mouseY)
        ? pick(mouseX, mouseY) : -1;
}

int Application::promotionChoice(double x, double y) const {
    int w, h;
    glfwGetWindowSize(window, &w, &h);
    if (w <= 0 || h <= 0) return -1;
    float nx = float(x / w), ny = float(y / h);
    if (ny < .32f || ny >= .68f) return -1;
    for (int i = 0; i < 4; ++i) {
        float left = .06f + i * .225f;
        if (nx >= left && nx < left + .205f) return i;
    }
    return -1;
}

void Application::drawPromotion() {
    // Coordenadas normalizadas compartilhadas com o teste de clique; funciona em HiDPI.
    int hovered = promotionChoice(mouseX, mouseY);
    glEnable(GL_SCISSOR_TEST);
    const Vec3 previewEye{0, 2.3f, 4.6f};
    const float rotation = float(std::fmod(glfwGetTime() * .8, AI_MATH_TWO_PI));
    for (int i = 0; i < 4; ++i) {
        int x = int((.06f + i * .225f) * width);
        int y = int(.32f * height);
        int w = int(.205f * width), h = int(.36f * height);
        if (w <= 8 || h <= 8) continue;
        glScissor(x, y, w, h);
        glClearColor(i == hovered ? .95f : .30f, i == hovered ? .68f : .77f, .53f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glScissor(x + 4, y + 4, w - 8, h - 8);
        glClearColor(.06f, .09f, .12f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(x, y, w, h);
        shader->use(perspective(float(w) / h) * lookAt(previewEye, {0, .6f, 0}), previewEye);
        shader->lighting(frameScenario);
        promotionPieces[i]->angle = rotation;
        promotionPieces[i]->draw(*shader, Mat4::identity());
    }
    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, width, height);
}

void Application::initializeShadows(bool secondary) {
    if (!depthShader) depthShader = std::make_unique<Shader>("shadow.vert", "shadow.frag");
    auto &map = secondary ? secondaryShadowMap : shadowMap;
    auto &framebuffer = secondary ? secondaryShadowFramebuffer : shadowFramebuffer;
    if (map) return;
    const int resolution = secondary ? shadowResolution / 2 : shadowResolution;
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &map);
    glBindTexture(GL_TEXTURE_2D, map);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, resolution, resolution,
                 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float border[] = {1.f, 1.f, 1.f, 1.f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, map, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    const bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    if (!complete) {
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteTextures(1, &map);
        framebuffer = map = 0;
    }
    require(complete, "Falha ao criar framebuffer de sombras");
}

void Application::drawScene(Shader &sceneShader) {
    const auto identity = Mat4::identity();
    table->draw(sceneShader, identity);
    board->draw(sceneShader, identity);
    pet->draw(sceneShader, table->model(identity));
}

std::unique_ptr<Pet> Application::makePet(const Scenario &scene) {
    return std::make_unique<Pet>(scene.pet, pet.get());
}
