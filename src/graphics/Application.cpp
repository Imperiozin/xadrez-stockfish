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

    board.reset();
    primitives.reset();
    shader.reset();
    if (window) {
        glfwDestroyWindow(window);
    }

    glfwTerminate();
}

Vec3 Application::eye() const {
    return {distance * std::cos(pitch) * std::sin(yaw), distance * std::sin(pitch),
            distance * std::cos(pitch) * std::cos(yaw)};
}

void Application::refresh() {
    cachedFen = game.fen();
    legal = game.movimentosValidos();
    board->sync(cachedFen);
    board->selected = -1;
    board->destinations.clear();
}

void Application::title() {
    std::ostringstream text;
    text << "XADREZ 3D | " << status << " | Nivel " << game.dificuldade() << " / "
         << game.tempoPensamento()
         << " ms | Clique: mover | Dir: camera | Scroll: zoom | N: novo | Setas: nivel | PgUp/Dn: "
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
    auto forward = normalized(Vec3{} - e);
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
        board->selected = -1;
        board->destinations.clear();
        return;
    }

    if (board->selected >= 0) {
        std::string prefix = squareName(board->selected) + squareName(square);
        for (auto &move : legal) {
            if (move.compare(0, 4, prefix) == 0) {
                if (move.size() == 5) {
                    promotionPrefix = prefix;
                    status = "Promocao: Q rainha / R torre / B bispo / C cavalo";
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

    if (board->selected >= 0) {
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
            promotionPrefix.clear();
            status = "Promocao cancelada";
            title();
        } else {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        return;
    }
    if (k == GLFW_KEY_HOME) {
        yaw = 0;
        pitch = .87f;
        distance = 17;
        return;
    }
    if (pending.valid()) {
        return;
    }
    try {
        if (!promotionPrefix.empty()) {
            char c = k == GLFW_KEY_Q   ? 'q'
                     : k == GLFW_KEY_R ? 'r'
                     : k == GLFW_KEY_B ? 'b'
                     : k == GLFW_KEY_C ? 'n'
                                       : 0;
            if (c) {
                submit(promotionPrefix + c);
            }
            return;
        }
        if (k == GLFW_KEY_N) {
            reset();
        }
        if (k == GLFW_KEY_UP || k == GLFW_KEY_DOWN) {
            game.definirDificuldade(
                std::clamp(game.dificuldade() + (k == GLFW_KEY_UP ? 1 : -1), -20, 20));
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
    glViewport(0, 0, width, height);
    glClearColor(.035f, .055f, .075f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shader->use(perspective(float(width) / height) * lookAt(eye()), eye());
    auto identity = Mat4::identity();
    primitives->draw(*shader, Primitive::Cube, identity, {0, -.85f, 0}, {200, .10f, 200},
                     {.055f, .085f, .10f});
    board->draw(*shader, identity);
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
    shader = std::make_unique<Shader>();
    primitives = std::make_unique<Primitives>();
    board = std::make_unique<Board>(*primitives);
    refresh();
    title();

    // Eventos de mouse, camera e teclado.
    glfwSetWindowUserPointer(window, this);
    glfwSetMouseButtonCallback(window, [](GLFWwindow *w, int button, int action, int) {
        auto &a = *static_cast<Application *>(glfwGetWindowUserPointer(w));
        double x, y;
        glfwGetCursorPos(w, &x, &y);
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            a.click(a.pick(x, y));
        }
        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            a.dragging = action == GLFW_PRESS;
            a.mouseX = x;
            a.mouseY = y;
        }
    });

    glfwSetCursorPosCallback(window, [](GLFWwindow *w, double x, double y) {
        auto &a = *static_cast<Application *>(glfwGetWindowUserPointer(w));
        if (a.dragging) {
            a.yaw -= float(x - a.mouseX) * .006f;
            a.pitch = std::clamp(a.pitch + float(y - a.mouseY) * .006f, .25f, 1.48f);
        }
        a.mouseX = x;
        a.mouseY = y;
    });

    glfwSetScrollCallback(window, [](GLFWwindow *w, double, double y) {
        auto &a = *static_cast<Application *>(glfwGetWindowUserPointer(w));
        a.distance = std::clamp(a.distance - float(y), 10.f, 28.f);
    });

    glfwSetKeyCallback(window, [](GLFWwindow *w, int key, int, int action, int) {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            static_cast<Application *>(glfwGetWindowUserPointer(w))->key(key);
        }
    });

    // Loop principal: atualiza o jogo, desenha a cena e processa eventos.
    while (!glfwWindowShouldClose(window)) {
        update();
        render();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    return 0;
}
