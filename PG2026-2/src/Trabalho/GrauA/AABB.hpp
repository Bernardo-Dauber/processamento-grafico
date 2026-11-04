#pragma once

#include <algorithm>
#include <glm/glm.hpp>

// Caixa de colisão alinhada aos eixos (Axis-Aligned Bounding Box).
// É a base de TODOS os testes de colisão do jogo:
//  - triggers (moeda, bandeira, inimigo): só overlap;
//  - sólidos (jogador × plataforma): overlap + resolução de penetração
//    eixo a eixo (X primeiro, depois Y).
struct AABB {
    float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;

    bool overlaps(const AABB &other) const
    {
        return x < other.x + other.w && x + w > other.x &&
               y < other.y + other.h && y + h > other.y;
    }

    float right() const { return x + w; }
    float bottom() const { return y + h; }
};

// Constrói uma AABB a partir de posição (canto superior esquerdo) + tamanho.
inline AABB aabbFrom(const glm::vec2 &position, const glm::vec2 &size)
{
    return { position.x, position.y, size.x, size.y };
}
