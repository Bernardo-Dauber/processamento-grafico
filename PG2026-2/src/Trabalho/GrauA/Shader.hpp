#pragma once

#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

// Encapsula um programa de shader: compila vertex + fragment, faz o link
// e expõe helpers para enviar uniforms.
class Shader {
public:
    Shader() = default;

    // Compila e linka o programa a partir dos códigos-fonte GLSL.
    // Retorna false e imprime o log em caso de erro de compilação/linkagem.
    bool load(const std::string &vertexSrc, const std::string &fragmentSrc,
              const std::string &debugName = "shader");

    void use() const;

    GLuint id() const { return mProgram; }

    // Helpers de uniform
    void setMat4(const std::string &name, const glm::mat4 &value) const;
    void setVec2(const std::string &name, const glm::vec2 &value) const;
    void setVec4(const std::string &name, const glm::vec4 &value) const;
    void setInt(const std::string &name, int value) const;

private:
    GLuint compile(GLenum type, const std::string &src, const std::string &debugName) const;

    GLuint mProgram = 0;
};
