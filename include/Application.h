#pragma once
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include "Board.h"
#include "jogo.hpp"
#include <GLFW/glfw3.h>
#include <future>

class Application {
    Jogo &game;
    GLFWwindow *window = nullptr;
    std::unique_ptr<Shader> shader;
    std::unique_ptr<Primitives> primitives;
    std::unique_ptr<Board> board;
    std::future<std::unique_ptr<RespostaMovimento>> pending;
    std::vector<std::string> legal;
    std::string cachedFen, status = "Sua vez: selecione uma peca branca", promotionPrefix;
    bool finished = false, dragging = false;
    double mouseX = 0, mouseY = 0;
    float yaw = 0, pitch = .87f, distance = 17;
    int width = 1280, height = 900;
    Vec3 eye() const;
    void refresh();
    void title();
    void click(int square);
    void submit(const std::string &move);
    void update();
    void render();
    void reset();
    void key(int key);

  public:
    explicit Application(Jogo &game) : game(game) {}

    ~Application();
    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;
    int run();
    int pick(double x, double y) const;
};
