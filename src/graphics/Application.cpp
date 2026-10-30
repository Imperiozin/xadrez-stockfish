#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#undef GLAD_GL_IMPLEMENTATION
#include "Application.h"

#include <chrono>
#include <iostream>
#include <sstream>
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

    for (auto &piece : promotionPieces) piece.reset();
    board.reset();
    table.reset();
    primitives.reset();
    shader.reset();
    depthShader.reset();
    skybox.reset();
    if (shadowMap) glDeleteTextures(1, &shadowMap);
    if (shadowFramebuffer) glDeleteFramebuffers(1, &shadowFramebuffer);
    if (window) {
        glfwDestroyWindow(window);
    }

    glfwTerminate();
}

Vec3 Application::eye() const {
    return cameraTarget + Vec3{distance * std::cos(pitch) * std::sin(yaw), distance * std::sin(pitch),
            distance * std::cos(pitch) * std::cos(yaw)};
}

void Application::refresh() {
    cachedFen = game.fen();
    legal = game.movimentosValidos();
    board->sync(cachedFen);
    board->selected = -1;
    board->destinations.clear();
    board->captures.clear();
    board->castling.clear();
}

void Application::title() {
    std::ostringstream text;
    text << "XADREZ 3D | " << status << " | Nivel " << game.dificuldade() << " / "
         << game.tempoPensamento()
         << " ms | Clique: mover | Meio: camera | Dir: cancelar | Scroll: zoom | N: novo | Setas: camera | Shift+Setas: altura | +/-: nivel | PgUp/Dn: "
            "tempo | Home: camera | Esc: sair";
    glfwSetWindowTitle(window, text.str().c_str());
}

int Application::pick(double x, double y) const {
    int w, h;
    glfwGetWindowSize(window, &w, &h);
    if (w <= 0 || h <= 0) {
        return -1;
    }
    auto e = eye();
    auto forward = normalized(cameraTarget - e);
    auto right = normalized(cross(forward, {0, 1, 0}));
    auto up = cross(right, forward);
    float nx = float(2 * x / w - 1);
    float ny = float(1 - 2 * y / h);
    float tangent = std::tan(.65f / 2);

    Vec3 ray = normalized(forward + right * (nx * float(w) / h * tangent) + up * (ny * tangent));
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
    if (pending.valid() || finished || !promotionPrefix.empty()) {
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
                    status = "Promocao: clique em rainha, torre, bispo ou cavalo";
                    title();
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
        status = "Selecione uma peca branca";
    }
    title();
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

    board->sync(preview.getFen());
    board->selected = -1;
    board->destinations.clear();
    board->captures.clear();
    board->castling.clear();
    promotionPrefix.clear();
    status = "Stockfish pensando...";
    title();

    pending = std::async(std::launch::async, [this, move] {
        return game.fazerMovimento(move);
    });
}

void Application::update() {
    if (!pending.valid() ||
        pending.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
        return;
    }
    try {
        auto response = pending.get();
        if (response) {
            finished = response->fimPartida;
            status = finished ? response->resultado : "Sua vez | Stockfish: " + response->movimento;
        } else {
            status = "Lance invalido. Tente novamente.";
        }
    } catch (const std::exception &e) {
        status = std::string("Erro do motor: ") + e.what();
    }
    refresh();
    title();
}

void Application::reset() {
    if (pending.valid()) {
        return;
    }
    game.reiniciar();
    finished = false;
    promotionPrefix.clear();
    status = "Nova partida: brancas jogam";
    refresh();
    title();
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
        if (k == GLFW_KEY_N) {
            reset();
        }
        if (k == GLFW_KEY_EQUAL || k == GLFW_KEY_MINUS || k == GLFW_KEY_KP_ADD || k == GLFW_KEY_KP_SUBTRACT) {
            game.definirDificuldade(
                std::clamp(game.dificuldade() + ((k == GLFW_KEY_EQUAL || k == GLFW_KEY_KP_ADD) ? 1 : -1), -20, 20));
        }
        if (k == GLFW_KEY_PAGE_UP || k == GLFW_KEY_PAGE_DOWN) {
            game.definirTempoPensamento(std::clamp(
                game.tempoPensamento() + (k == GLFW_KEY_PAGE_UP ? 100 : -100), 1, 60000));
        }
        title();
    } catch (const std::exception &e) {
        status = e.what();
        title();
    }
}

void Application::render() {
    glfwGetFramebufferSize(window, &width, &height);
    if (width < 1 || height < 1) {
        return;
    }
    const Vec3 lightPosition = scenario.lightPosition;
    const Mat4 lightViewProjection = scenario.lightViewProjection();

    // Primeira passagem: somente profundidade, vista pela luz.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glViewport(0, 0, shadowResolution, shadowResolution);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);
    glClear(GL_DEPTH_BUFFER_BIT);
    depthShader->use(lightViewProjection, lightPosition);
    drawScene(*depthShader);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Segunda passagem: iluminação com consulta ao mapa de sombras.
    glViewport(0, 0, width, height);
    glClearColor(.035f, .055f, .075f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shader->use(perspective(float(width) / height) * lookAt(eye(), cameraTarget), eye());
    shader->lighting(scenario);
    shader->shadows(lightViewProjection, lightPosition, shadowMap);
    drawScene(*shader);
    skybox->draw(eye(), cameraTarget, float(width) / height);
    if (!promotionPrefix.empty()) drawPromotion();
}

int Application::run() {
    // Inicializacao da janela e do contexto OpenGL.
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
    window = glfwCreateWindow(width, height, "Xadrez 3D", nullptr, nullptr);
    require(window != nullptr, "Falha ao criar janela OpenGL 3.3");
    glfwMakeContextCurrent(window);
    require(gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)) != 0,
            "Falha ao carregar OpenGL");
    glfwSwapInterval(1);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    // Recursos graficos e estado inicial da cena.
    scenario = Scenario::load(scenarioFile);
    shader = std::make_unique<Shader>();
    initializeShadows();
    skybox = std::make_unique<Skybox>(scenario.skybox);
    primitives = std::make_unique<Primitives>();
    board = std::make_unique<Board>(*primitives);
    table = std::make_unique<Table>();
    const char symbols[] = "QRBN";
    for (int i = 0; i < 4; ++i) promotionPieces[i] = Piece::create(symbols[i], *primitives);
    refresh();
    title();

    // Eventos de mouse, camera e teclado.
    glfwSetWindowUserPointer(window, this);
    glfwSetMouseButtonCallback(window, [](GLFWwindow *w, int button, int action, int) {
        auto &a = *static_cast<Application *>(glfwGetWindowUserPointer(w));
        double x, y;
        glfwGetCursorPos(w, &x, &y);
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            if (!a.promotionPrefix.empty()) {
                int choice = a.promotionChoice(x, y);
                if (choice >= 0) a.submit(a.promotionPrefix + "qrbn"[choice]);
            } else a.click(a.pick(x, y));
        }
        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) a.cancelSelection();
        if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
            a.dragging = action == GLFW_PRESS;
            a.mouseX = x;
            a.mouseY = y;
        }
    });

    glfwSetCursorPosCallback(window, [](GLFWwindow *w, double x, double y) {
        auto &a = *static_cast<Application *>(glfwGetWindowUserPointer(w));
        if (a.dragging) {
            a.yaw -= float(x - a.mouseX) * .006f;
            a.pitch = std::clamp(a.pitch + float(y - a.mouseY) * .006f, -.20f, 1.53f);
        }
        a.constrainCamera();
        a.board->hovered = a.promotionPrefix.empty() ? a.pick(x, y) : -1;
        a.mouseX = x;
        a.mouseY = y;
    });

    glfwSetScrollCallback(window, [](GLFWwindow *w, double, double y) {
        auto &a = *static_cast<Application *>(glfwGetWindowUserPointer(w));
        a.distance = std::clamp(a.distance - float(y), 4.f, 45.f);
        a.constrainCamera();
    });

    glfwSetKeyCallback(window, [](GLFWwindow *w, int key, int, int action, int) {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            static_cast<Application *>(glfwGetWindowUserPointer(w))->key(key);
        }
    });

    // Loop principal: atualiza o jogo, desenha a cena e processa eventos.
    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        moveCamera(float(std::min(now - lastFrame, .05)));
        lastFrame = now;
        update();
        render();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    return 0;
}

void Application::cancelSelection() {
    promotionPrefix.clear();
    board->selected = -1;
    board->destinations.clear();
    board->captures.clear();
    board->castling.clear();
    if (!pending.valid() && !finished) status = "Selecione uma peca branca";
    title();
}

void Application::constrainCamera() {
    Vec3 e = eye();
    Vec3 bounded{std::clamp(e.x, -45.f, 45.f), std::clamp(e.y, -7.f, 40.f),
                 std::clamp(e.z, -45.f, 45.f)};
    cameraTarget = cameraTarget + bounded - e;
}

void Application::moveCamera(float dt) {
    auto pressed = [this](int k) { return glfwGetKey(window, k) == GLFW_PRESS; };
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
    board->hovered = promotionPrefix.empty() ? pick(mouseX, mouseY) : -1;
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
    const float rotation = float(std::fmod(glfwGetTime() * .8, 6.2831853));
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
        shader->lighting(scenario);
        promotionPieces[i]->angle = rotation;
        promotionPieces[i]->draw(*shader, Mat4::identity());
    }
    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, width, height);
}

void Application::initializeShadows() {
    depthShader = std::make_unique<Shader>("shadow.vert", "shadow.frag");
    glGenTextures(1, &shadowMap);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, shadowResolution, shadowResolution,
                 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float border[] = {1.f, 1.f, 1.f, 1.f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
    glGenFramebuffers(1, &shadowFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    const bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    require(complete, "Falha ao criar framebuffer de sombras");
}

void Application::drawScene(Shader &sceneShader) {
    const auto identity = Mat4::identity();
    table->draw(sceneShader, identity);
    board->draw(sceneShader, identity);
}
