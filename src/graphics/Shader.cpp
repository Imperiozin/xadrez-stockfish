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

Shader::Shader(const char *vertexFile, const char *fragmentFile) {
    const auto shaderDirectory = pastaDoExecutavel() / "shaders";
    GLuint vs = compile(GL_VERTEX_SHADER, shaderDirectory / vertexFile), fs = 0;
    try {
        fs = compile(GL_FRAGMENT_SHADER, shaderDirectory / fragmentFile);
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
    glUniform3f(glGetUniformLocation(id, "lightPosition"), -4.f, 10.f, 5.f);
    glUniform1i(glGetUniformLocation(id, "shadowsEnabled"), GL_FALSE);
    glUniformMatrix4fv(glGetUniformLocation(id, "viewProjection"), 1, GL_FALSE, vp.v);
    glUniform3f(glGetUniformLocation(id, "eye"), eye.x, eye.y, eye.z);
}

void Shader::object(const Mat4 &m, Vec3 c) {
    glUniformMatrix4fv(glGetUniformLocation(id, "model"), 1, GL_FALSE, m.v);
    glUniform3f(glGetUniformLocation(id, "color"), c.x, c.y, c.z);
}

void Shader::shadows(const Mat4 &lightViewProjection, Vec3 lightPosition, GLuint depthMap) {
    glUniformMatrix4fv(glGetUniformLocation(id, "lightViewProjection"), 1, GL_FALSE,
                       lightViewProjection.v);
    glUniform3f(glGetUniformLocation(id, "lightPosition"), lightPosition.x, lightPosition.y,
                lightPosition.z);
    glUniform1i(glGetUniformLocation(id, "shadowsEnabled"), GL_TRUE);
    glUniform1i(glGetUniformLocation(id, "shadowMap"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthMap);
}
