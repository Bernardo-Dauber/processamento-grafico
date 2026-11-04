#include "Shader.hpp"

#include <iostream>

bool Shader::load(const std::string &vertexSrc, const std::string &fragmentSrc,
                  const std::string &debugName)
{
    GLuint vertex = compile(GL_VERTEX_SHADER, vertexSrc, debugName);
    GLuint fragment = compile(GL_FRAGMENT_SHADER, fragmentSrc, debugName);
    if (vertex == 0 || fragment == 0)
        return false;

    mProgram = glCreateProgram();
    glAttachShader(mProgram, vertex);
    glAttachShader(mProgram, fragment);
    glLinkProgram(mProgram);

    GLint success;
    glGetProgramiv(mProgram, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(mProgram, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED (" << debugName << ")\n"
                  << infoLog << std::endl;
        return false;
    }

    // Os shaders individuais podem ser descartados depois do link
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return true;
}

void Shader::use() const
{
    glUseProgram(mProgram);
}

GLuint Shader::compile(GLenum type, const std::string &src, const std::string &debugName) const
{
    GLuint shader = glCreateShader(type);
    const GLchar *srcPtr = src.c_str();
    glShaderSource(shader, 1, &srcPtr, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        const char *kind = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
        std::cerr << "ERROR::SHADER::" << kind << "::COMPILATION_FAILED (" << debugName << ")\n"
                  << infoLog << std::endl;
        return 0;
    }
    return shader;
}

void Shader::setMat4(const std::string &name, const glm::mat4 &value) const
{
    glUniformMatrix4fv(glGetUniformLocation(mProgram, name.c_str()), 1, GL_FALSE, &value[0][0]);
}

void Shader::setVec2(const std::string &name, const glm::vec2 &value) const
{
    glUniform2f(glGetUniformLocation(mProgram, name.c_str()), value.x, value.y);
}

void Shader::setVec4(const std::string &name, const glm::vec4 &value) const
{
    glUniform4f(glGetUniformLocation(mProgram, name.c_str()), value.x, value.y, value.z, value.w);
}

void Shader::setInt(const std::string &name, int value) const
{
    glUniform1i(glGetUniformLocation(mProgram, name.c_str()), value);
}
