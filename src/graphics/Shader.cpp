#include "Shader.h"
#include "stockfish.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
std::string readShader(const arquivos::path &path) {
    std::ifstream file(path.string());
    if (!file) {
        throw std::runtime_error("Nao foi possivel abrir o shader: " + path.string());
    }
    std::ostringstream source;
    source << file.rdbuf();
    if (file.bad()) {
        throw std::runtime_error("Falha ao ler o shader: " + path.string());
    }
    return source.str();
}

GLuint compile(GLenum type, const arquivos::path &path) {
    const std::string source = readShader(path);
    const char *sourceText = source.c_str();
    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &sourceText, nullptr);
    glCompileShader(id);
    GLint ok;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetShaderInfoLog(id, sizeof(log), nullptr, log);
        glDeleteShader(id);
        throw std::runtime_error("Falha ao compilar " + path.string() + ":\n" + log);
    }
    return id;
}
} // namespace

Shader::Shader() {
    const auto shaderDirectory = pastaDoExecutavel() / "shaders";
    GLuint vs = compile(GL_VERTEX_SHADER, shaderDirectory / "scene.vert"), fs = 0;
    try {
        fs = compile(GL_FRAGMENT_SHADER, shaderDirectory / "scene.frag");
    } catch (...) {
        glDeleteShader(vs);
        throw;
    }
    id = glCreateProgram();
    glAttachShader(id, vs);
    glAttachShader(id, fs);
    glLinkProgram(id);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint ok;
    glGetProgramiv(id, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetProgramInfoLog(id, sizeof(log), nullptr, log);
        glDeleteProgram(id);
        throw std::runtime_error(log);
    }
}

Shader::~Shader() {
    glDeleteProgram(id);
}

void Shader::use(const Mat4 &vp, Vec3 eye) {
    glUseProgram(id);
    glUniformMatrix4fv(glGetUniformLocation(id, "viewProjection"), 1, GL_FALSE, vp.v);
    glUniform3f(glGetUniformLocation(id, "eye"), eye.x, eye.y, eye.z);
}

void Shader::object(const Mat4 &m, Vec3 c) {
    glUniformMatrix4fv(glGetUniformLocation(id, "model"), 1, GL_FALSE, m.v);
    glUniform3f(glGetUniformLocation(id, "color"), c.x, c.y, c.z);
}
