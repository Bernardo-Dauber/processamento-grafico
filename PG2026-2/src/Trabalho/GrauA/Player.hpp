#pragma once

#include <glm/glm.hpp>

#include "AABB.hpp"
#include "Animation.hpp"

class Level;
class SpriteRenderer;
class Texture2D;
class Shader;

// Input agregado do teclado (preenchido no main a cada frame via glfwGetKey)
struct PlayerInput {
    bool left = false;
    bool right = false;
    bool jumpPressed = false; // borda de subida da tecla (evita re-pular segurando)
    bool jumpHeld = false;    // tecla de pulo ainda pressionada (pulo variável)
};

// Estados de animação do jogador — cada estado é uma LINHA do spritesheet
enum class PlayerAnim {
    Idle = 0, // linha 0: 1 frame (parado)
    Walk = 1, // linha 1: 2 frames (andando)
    Jump = 2  // linha 2: 1 frame (no ar)
};

// O Jogador concentra a física do platformer:
//  - movimento horizontal com aceleração instantânea (arcade);
//  - gravidade + impulso de pulo (com corte do impulso se soltar a tecla);
//  - colisão SÓLIDA com plataformas resolvida eixo a eixo:
//      1) move em X, resolve penetrações horizontais;
//      2) move em Y, resolve aterrissagem (onGround) e batida de cabeça.
class Player {
public:
    void spawn(const glm::vec2 &position);

    void update(float dt, const Level &level, const PlayerInput &input);

    void draw(SpriteRenderer &renderer, const Texture2D &texture, float camX,
              const glm::vec4 &tint = glm::vec4(1.0f)) const;

    // Hitbox reduzida: menores que o sprite de 32x32, para que encostar de
    // raspão em paredes/inimigos não pareça injusto.
    AABB hitbox() const
    {
        return { position.x + kInsetX, position.y + kInsetY,
                 size.x - 2 * kInsetX, size.y - kInsetY - kInsetBottom };
    }

    // Estado (o main lê para os triggers e para a máquina de estados)
    glm::vec2 position{0.0f};
    glm::vec2 size{32.0f, 32.0f};
    glm::vec2 velocity{0.0f};
    bool onGround = false;
    bool facingRight = true;

private:
    // Margens da hitbox dentro do sprite (px)
    static constexpr float kInsetX = 6.0f;
    static constexpr float kInsetY = 2.0f;
    static constexpr float kInsetBottom = 0.0f;

    void resolveCollisionsX(const Level &level);
    void resolveCollisionsY(const Level &level, float prevBottom, float prevTop);
    void updateAnimation(float dt, bool moving);

    // Metadados de animação (linhas do spritesheet do jogador)
    const Animation kIdle{ 0, 1, 0.2f };
    const Animation kWalk{ 1, 2, 0.12f };
    const Animation kJump{ 2, 1, 0.2f };
    Animation mAnim = kIdle;
    float mAnimTimer = 0.0f;
};
