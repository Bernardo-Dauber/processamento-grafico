#include "Texture2D.hpp"

#include <iostream>

// Implementação única do stb_image neste executável
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Texture2D::~Texture2D()
{
    if (mID != 0)
        glDeleteTextures(1, &mID);
}

Texture2D::Texture2D(Texture2D &&other) noexcept
    : mID(other.mID), mWidth(other.mWidth), mHeight(other.mHeight)
{
    other.mID = 0;
}

Texture2D &Texture2D::operator=(Texture2D &&other) noexcept
{
    if (this != &other) {
        if (mID != 0)
            glDeleteTextures(1, &mID);
        mID = other.mID;
        mWidth = other.mWidth;
        mHeight = other.mHeight;
        other.mID = 0;
    }
    return *this;
}

bool Texture2D::load(const std::string &path)
{
    // NOTA: NÃO usamos stbi_set_flip_vertically_on_load. Como nossa projeção
    // ortográfica tem Y crescendo para BAIXO (convenção de jogos 2D), a linha
    // 0 do arquivo (topo da imagem) deve corresponder a v = 0 no UV.
    stbi_set_flip_vertically_on_load(false);

    int channels = 0;
    unsigned char *data = stbi_load(path.c_str(), &mWidth, &mHeight, &channels, 0);
    if (!data) {
        std::cerr << "ERROR::TEXTURE::Falha ao carregar imagem: " << path << std::endl;
        return false;
    }

    createFromPixels(mWidth, mHeight, channels, data);
    stbi_image_free(data);
    return true;
}

Texture2D Texture2D::solid(const glm::vec4 &rgba)
{
    unsigned char pixel[4] = {
        static_cast<unsigned char>(rgba.r * 255.0f),
        static_cast<unsigned char>(rgba.g * 255.0f),
        static_cast<unsigned char>(rgba.b * 255.0f),
        static_cast<unsigned char>(rgba.a * 255.0f),
    };
    Texture2D tex;
    tex.createFromPixels(1, 1, 4, pixel);
    return tex;
}

void Texture2D::createFromPixels(int width, int height, int channels, const unsigned char *pixels)
{
    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

    glGenTextures(1, &mID);
    glBindTexture(GL_TEXTURE_2D, mID);

    // Pixel art: sem interpolação (NEAREST) e sem repetição (CLAMP_TO_EDGE,
    // evita "vazamento" entre frames vizinhos do spritesheet).
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
}
