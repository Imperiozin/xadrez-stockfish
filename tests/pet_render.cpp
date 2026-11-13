#include "Pet.h"
#include "Board.h"
#include "Table.h"
#include "Notepad.h"
#include "Skybox.h"
#include "stockfish.hpp"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <iostream>
#include <stdexcept>

void testPetRendering() {
    if (!glfwInit()) throw std::runtime_error("GLFW indisponivel para teste de renderizacao");
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    auto *window = glfwCreateWindow(960, 720, "Pet render checks", nullptr, nullptr);
    if (!window) { glfwTerminate(); throw std::runtime_error("Janela OpenGL oculta indisponivel"); }
    glfwMakeContextCurrent(window);
    if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)))
        throw std::runtime_error("GLAD no teste de pets");
    {
        ImGui::CreateContext();
        auto &io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.DisplaySize = {960,720};
        io.DeltaTime = 1.f/60;
        ImFontConfig font;
        font.SizePixels = 24;
        io.Fonts->AddFontDefault(&font);
        if (!ImGui_ImplOpenGL3_Init("#version 330 core")) throw std::runtime_error("ImGui no teste de pets");
        Shader shader, shadow("shadow.vert", "shadow.frag");
        Primitives primitives;
        Board board(primitives);
        board.sync("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        Table table;
        auto pad = std::make_unique<Notepad>();
        pad->position = {7.3f,Table::surfaceY+.025f,3.7f};
        pad->rotation = {0,-.274533f,0};
        auto *notepad = pad.get();
        table.addProp(std::move(pad));
        notepad->initializeInterface();
        GLuint maps[2], framebuffer;
        glGenTextures(2,maps);
        glGenFramebuffers(1,&framebuffer);
        for (auto map : maps) {
            glBindTexture(GL_TEXTURE_2D,map);
            glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24,2048,2048,0,GL_DEPTH_COMPONENT,GL_FLOAT,nullptr);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_COMPARE_MODE,GL_COMPARE_REF_TO_TEXTURE);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_COMPARE_FUNC,GL_LEQUAL);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_BORDER);
            const float border[]{1,1,1,1};
            glTexParameterfv(GL_TEXTURE_2D,GL_TEXTURE_BORDER_COLOR,border);
        }
        stbi_flip_vertically_on_write(true);
        for (int index = 0; index < 5; ++index) {
            const auto scenario = Scenario::load(Scenario::fileForDifficulty(-20+index*10)).sample(0);
            Pet pet(scenario.pet);
            pet.position = {-7.3f,Table::surfaceY+.2f,2.0f};
            Skybox sky(scenario.skybox);
            auto draw = [&](Shader &s) {
                table.draw(s,Mat4::identity());
                board.draw(s,Mat4::identity());
                pet.draw(s,table.model(Mat4::identity()));
            };
            auto render = [&](const std::string &name) {
                ImGui_ImplOpenGL3_NewFrame();
                ImGui::NewFrame();
                notepad->renderInterface(1.f/60,-20+index*10,false);
                ImGui::Render();
                pet.renderLabel();
                glEnable(GL_DEPTH_TEST);
                const bool two = scenario.animation == Scenario::Animation::BinarySuns;
                glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
                for (int pass = 0; pass < (two ? 2 : 1); ++pass) {
                    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,maps[pass],0);
                    glDrawBuffer(GL_NONE); glReadBuffer(GL_NONE);
                    if (glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)
                        throw std::runtime_error("Framebuffer de sombras do teste");
                    glViewport(0,0,2048,2048);
                    glClear(GL_DEPTH_BUFFER_BIT);
                    shadow.use(scenario.lightViewProjection(pass!=0),{});
                    draw(shadow);
                }
                glBindFramebuffer(GL_FRAMEBUFFER,0);
                glDrawBuffer(GL_BACK); glReadBuffer(GL_BACK);
                glViewport(0,0,960,720);
                glClearColor(.04f,.05f,.08f,1);
                glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
                const Vec3 eye{0,19.36f,25.47f};
                shader.use(perspective(960.f/720)*lookAt(eye),eye);
                shader.lighting(scenario);
                shader.shadows(scenario.lightViewProjection(),scenario.lightPosition,maps[0]);
                if (two) shader.shadows(scenario.lightViewProjection(true),scenario.secondaryLightPosition,maps[1],true);
                draw(shader);
                sky.draw(eye,{},960.f/720,scenario);
                glFinish();
                if (glGetError()!=GL_NO_ERROR) throw std::runtime_error("Erro OpenGL ao desenhar pet");
                std::vector<unsigned char> pixels(960*720*3);
                glPixelStorei(GL_PACK_ALIGNMENT,1);
                glReadPixels(0,0,960,720,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
                const auto path=(pastaDoExecutavel()/".."/".."/(name+".png")).string();
                if (!stbi_write_png(path.c_str(),960,720,3,pixels.data(),960*3))
                    throw std::runtime_error("Falha ao salvar captura do pet");
                return pixels;
            };
            pet.update(0);
            const auto idle = render("pet-"+scenario.pet);
            AvaliacaoJogada review;
            review.qualidade = QualidadeJogada::Melhor;
            review.movimento = "e2e4";
            pet.review(review);
            pet.update(.23f);
            const auto dance = render("pet-"+scenario.pet+"-dance");
            int changed = 0;
            for (std::size_t i=0;i<idle.size();++i) if (idle[i]!=dance[i]) ++changed;
            if (changed<500) throw std::runtime_error("Pet sem mudanca visual na animacao");
            if (index == 2) {
                for (auto quality : {QualidadeJogada::Excelente,QualidadeJogada::Boa,QualidadeJogada::Imprecisao,
                                     QualidadeJogada::Erro,QualidadeJogada::ErroGrave}) {
                    review.qualidade=quality;
                    pet.review(review);
                    pet.update(.23f);
                    render("pet-review-"+std::to_string(int(quality)));
                    pet.update(3);
                }
                pet.thinking();
                render("pet-thinking");
                pet.reset();
                render("pet-reset");
            }
            std::cout << "PASS render " << scenario.pet << ": textures, labels, animation and shadows\n";
        }
        glDeleteTextures(2,maps);
        glDeleteFramebuffers(1,&framebuffer);
        ImGui_ImplOpenGL3_Shutdown();
        ImGui::DestroyContext();
    }
    glfwDestroyWindow(window);
    glfwTerminate();
}
