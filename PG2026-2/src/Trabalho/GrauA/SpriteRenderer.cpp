#include "SpriteRenderer.hpp"

#include <iostream>

bool SpriteRenderer::init(Shader *shader)
{
    mShader = shader;

    // Quad unitário centrado na origem: posições de -0.5 a +0.5.
    // O UV é derivado no vertex shader como (position + 0.5), mapeando o
    // quad inteiro para o intervalo [0,1] — depois os uniforms uvOffset/
    // uvScale recortam o frame desejado dentro do spritesheet.
    GLfloat vertices[] = {
        -0.5f, -0.5f, // v0 (canto superior-esquerdo do UV)
         0.5f, -0.5f, // v1
         0.5f,  0.5f, // v2
        -0.5f,  0.5f, // v3
    };
    GLuint indices[] = { 0, 1, 2, 0, 2, 3 }; // dois triângulos via EBO

    glGenVertexArrays(1, &mVAO);
    glGenBuffers(1, &mVBO);
    glGenBuffers(1, &mEBO);

    glBindVertexArray(mVAO);

    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Atributo 0: posição (vec2)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // Cache dos locations dos uniforms (o programa não muda em runtime)
    mShader->use();
    mModelLoc = glGetUniformLocation(mShader->id(), "model");
    mUVOffsetLoc = glGetUniformLocation(mShader->id(), "uvOffset");
    mUVScaleLoc = glGetUniformLocation(mShader->id(), "uvScale");
    mTintLoc = glGetUniformLocation(mShader->id(), "tint");
    mSpriteLoc = glGetUniformLocation(mShader->id(), "sprite");

    if (mModelLoc < 0 || mUVOffsetLoc < 0 || mUVScaleLoc < 0 || mTintLoc < 0 || mSpriteLoc < 0) {
        std::cerr << "ERROR::SPRITERENDERER::uniform não encontrado no shader" << std::endl;
        return false;
    }
    return true;
}

void SpriteRenderer::draw(const Texture2D &texture, const glm::mat4 &model,
                          const glm::vec2 &uvOffset, const glm::vec2 &uvScale,
                          const glm::vec4 &tint) const
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture.id());
    glUniform1i(mSpriteLoc, 0);

    glUniformMatrix4fv(mModelLoc, 1, GL_FALSE, &model[0][0]);
    glUniform2f(mUVOffsetLoc, uvOffset.x, uvOffset.y);
    glUniform2f(mUVScaleLoc, uvScale.x, uvScale.y);
    glUniform4f(mTintLoc, tint.r, tint.g, tint.b, tint.a);

    glBindVertexArray(mVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
