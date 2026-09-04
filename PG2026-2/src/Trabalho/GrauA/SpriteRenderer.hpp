#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Shader.hpp"
#include "Texture2D.hpp"

// Monta a model matrix de um sprite: translação para (pos + |size|/2) seguida
// da escala (pos é o canto superior esquerdo, convenção de mundo Y-para-baixo).
// Uma escala NEGATIVA em x espelha o sprite horizontalmente sem deslocar o
// quad: ele continua ancorado em [pos.x, pos.x + |size.x|].
inline glm::mat4 spriteModel(const glm::vec2 &pos, const glm::vec2 &size)
{
    glm::mat4 m(1.0f);
    m = glm::translate(m, glm::vec3(pos + glm::abs(size) * 0.5f, 0.0f));
    return glm::scale(m, glm::vec3(size, 1.0f));
}

// Desenha todos os sprites do jogo a partir de UM ÚNICO quad unitário
// (VAO + VBO + EBO criados uma única vez aqui). A posição/tamanho de cada
// sprite entram pela model matrix; o recorte do spritesheet entra pelos
// uniforms de UV; a cor extra (flash de dano, fades) entra pelo tint.
class SpriteRenderer {
public:
    bool init(Shader *shader);

    // texture: spritesheet (ou imagem única); uvOffset/uvScale: recorte do
    // frame dentro da sheet; tint: multiplicador de cor.
    void draw(const Texture2D &texture, const glm::mat4 &model,
              const glm::vec2 &uvOffset, const glm::vec2 &uvScale,
              const glm::vec4 &tint = glm::vec4(1.0f)) const;

private:
    Shader *mShader = nullptr;
    GLuint mVAO = 0, mVBO = 0, mEBO = 0;
    GLint mModelLoc = -1, mUVOffsetLoc = -1, mUVScaleLoc = -1, mTintLoc = -1, mSpriteLoc = -1;
};
