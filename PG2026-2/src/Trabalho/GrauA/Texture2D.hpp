#pragma once

#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

// Carrega uma imagem (PNG com transparência) via stb_image para uma
// textura OpenGL, com filtro NEAREST (pixel art) e wrap CLAMP_TO_EDGE.
class Texture2D {
public:
    Texture2D() = default;
    ~Texture2D();

    // Textura não pode ser copiada (o destrutor faria double-delete),
    // mas pode ser movida.
    Texture2D(const Texture2D &) = delete;
    Texture2D &operator=(const Texture2D &) = delete;
    Texture2D(Texture2D &&other) noexcept;
    Texture2D &operator=(Texture2D &&other) noexcept;

    // Carrega o arquivo de imagem; retorna false em caso de falha.
    bool load(const std::string &path);

    // Cria uma textura sólida 1x1 (usada para overlays de cor, como o
    // fade de vitória).
    static Texture2D solid(const glm::vec4 &rgba);

    GLuint id() const { return mID; }
    int width() const { return mWidth; }
    int height() const { return mHeight; }

private:
    void createFromPixels(int width, int height, int channels, const unsigned char *pixels);

    GLuint mID = 0;
    int mWidth = 0;
    int mHeight = 0;
};
