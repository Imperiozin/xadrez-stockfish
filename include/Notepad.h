#pragma once
#include "Object.h"
#include "Ray.h"
#include <limits>

struct ImGuiContext;

// Modelo, página e interação compartilham exclusivamente Object::model(parent).
class Notepad : public Object {
    struct Part {
        std::vector<Vertex> vertices;
        std::unique_ptr<Mesh> mesh;
        Vec3 color;
        bool ruled = false;
    };
    std::vector<Part> parts;
    PageSurface page;
    Vec3 boundsLow, boundsHigh;
    ImGuiContext *context = nullptr;
    GLuint framebuffer = 0, texture = 0;
    bool pressed = false, pointerOnPage = false, pointerOnControl = false;
    void load();

  public:
    static constexpr int canvasWidth = 768, canvasHeight = 640;
    struct Hit {
        bool body = false, page = false;
        Vec2 pixel;
        float distance = std::numeric_limits<float>::infinity();
    };
    struct Action {
        int difficulty = 0;
        bool changeDifficulty = false, newGame = false;
    };

    Notepad();
    ~Notepad() override;
    void initializeInterface();
    Action renderInterface(float dt, int difficulty, bool disabled);
    void pointer(const Hit &hit, bool enabled);
    void mouseButton(bool down);
    void focus(bool focused);
    bool capturesPointer() const { return pressed; }
    bool wantsPointerCursor() const { return pointerOnPage && pointerOnControl; }
    Hit hit(const Ray &ray, const Mat4 &parent) const;
    void draw(Shader &shader, const Mat4 &parent) override;
};
