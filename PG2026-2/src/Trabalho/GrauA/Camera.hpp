#pragma once

#include <algorithm>

// Câmera 2D: apenas um offset horizontal que segue o jogador.
// A projeção continua ortho(0, 960, 540, 0); o offset entra como
// translação na model matrix de cada objeto (e nas camadas de parallax,
// multiplicado pelo fator de cada camada).
class Camera {
public:
    void update(float playerCenterX, float worldWidth, float viewWidth)
    {
        float target = playerCenterX - viewWidth * 0.5f;
        mX = std::clamp(target, 0.0f, std::max(0.0f, worldWidth - viewWidth));
    }

    float x() const { return mX; }

private:
    float mX = 0.0f;
};
