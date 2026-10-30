#pragma once
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include "Board.h"
#include "Skybox.h"
#include "Table.h"
#include "jogo.hpp"
#include <GLFW/glfw3.h>
#include <future>

class Application {
    Jogo &game;
    std::string scenarioFile;
    Scenario scenario;
    GLFWwindow *window = nullptr;
    std::unique_ptr<Shader> shader;
    std::unique_ptr<Shader> depthShader;
    std::unique_ptr<Skybox> skybox;
    GLuint shadowFramebuffer = 0, shadowMap = 0;
    static constexpr int shadowResolution = 4096;
    void initializeShadows();
    void drawScene(Shader &sceneShader);
    std::unique_ptr<Primitives> primitives;
    std::unique_ptr<Board> board;
    std::unique_ptr<Table> table;
    std::future<std::unique_ptr<RespostaMovimento>> pending;
    std::vector<std::string> legal;
    std::string cachedFen, status = "Sua vez: selecione uma peca branca", promotionPrefix;
    bool finished = false, dragging = false;
    double mouseX = 0, mouseY = 0;
    float yaw = 0, pitch = .65f, distance = 32;
    int width = 1280, height = 900;
    Vec3 cameraTarget{};
    double lastFrame = 0;
    std::array<std::unique_ptr<Piece>, 4> promotionPieces;
    Vec3 eye() const;
    void constrainCamera();
    void moveCamera(float dt);
    void cancelSelection();
    int promotionChoice(double x, double y) const;
    void drawPromotion();
    void refresh();
    void title();
    void click(int square);
    void submit(const std::string &move);
    void update();
    void render();
    void reset();
    void key(int key);

  public:
    explicit Application(Jogo &game, const std::string &scenarioFile = "assets/scenarios/sunset.cfg")
        : game(game), scenarioFile(scenarioFile) {}

    ~Application();
    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;
    int run();
    int pick(double x, double y) const;
};
