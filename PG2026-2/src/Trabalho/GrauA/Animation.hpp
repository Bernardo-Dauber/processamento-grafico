#pragma once

#include <glm/glm.hpp>

// Descreve uma animação de spritesheet: os frames ficam dispostos em uma
// LINHA da grade ('row'), exibidos em sequência a cada 'frameTime' segundos.
// O frameSize (tamanho do quadriculado da grade) vem do arquivo de config;
// as sequências de cada animação ficam aqui, junto do código da entidade.
struct Animation {
    int row = 0;          // linha da grade onde estão os frames
    int frameCount = 1;   // quantos frames essa animação tem
    float frameTime = 0.12f; // segundos por frame
};

// Calcula o recorte (UV) de um frame da spritesheet.
// frame: tamanho do quadriculado; texSize: tamanho total da textura;
// col/row: índice do frame (coluna, linha).
inline glm::vec2 uvOffsetFor(const glm::vec2 &frame, const glm::vec2 &texSize, int col, int row)
{
    return glm::vec2(col * frame.x / texSize.x, row * frame.y / texSize.y);
}

inline glm::vec2 uvScaleFor(const glm::vec2 &frame, const glm::vec2 &texSize)
{
    return frame / texSize;
}
