#pragma once

#include <glm/glm.hpp>

#include "AABB.hpp"

// Inimigo que patrulha horizontalmente entre minX e maxX, sem IA: anda
// sempre na mesma direção e vira ao atingir um dos limites. Viver sobre
// uma plataforma específica é responsabilidade do config (spawn + limites).
class Enemy {
public:
    glm::vec2 position{0.0f}; // canto superior esquerdo do sprite
    glm::vec2 size{32.0f, 32.0f};

    bool alive = true;
    int direction = -1; // -1 esquerda, +1 direita
    float timer = 0.0f; // controla a animação de andar

    AABB hitbox() const { return { position.x + 4.0f, position.y + 4.0f, size.x - 8.0f, size.y - 4.0f }; }

    void update(float dt, float speed)
    {
        if (!alive)
            return;
        position.x += direction * speed * dt;
        if (position.x <= minX) {
            position.x = minX;
            direction = 1;
        }
        if (position.x + size.x >= maxX) {
            position.x = maxX - size.x;
            direction = -1;
        }
        timer += dt;
    }

    // Guarda o estado inicial para poder restaurar quando o jogador morre.
    glm::vec2 initialPosition{0.0f};
    float minX = 0.0f, maxX = 0.0f;
    int initialDirection = -1;

    void reset()
    {
        alive = true;
        position = initialPosition;
        direction = initialDirection;
        timer = 0.0f;
    }
};
