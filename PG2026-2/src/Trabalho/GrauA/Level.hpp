#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "Texture2D.hpp"
#include "Enemy.hpp"

// Retângulo sólido do cenário (chão ou plataforma flutuante).
struct Platform {
    glm::vec2 position{0.0f}; // canto superior esquerdo
    glm::vec2 size{0.0f};
};

// Moeda colecionável (animada por spritesheet).
struct Coin {
    glm::vec2 position{0.0f};
    glm::vec2 size{32.0f};
    float timer = 0.0f;
    bool collected = false;

    void update(float dt) { timer += dt; }
};

// Marcador do fim da fase: tocá-la encerra com vitória.
struct Flag {
    glm::vec2 position{0.0f};
    glm::vec2 size{32.0f, 64.0f};
};

// Uma camada de fundo com fator de parallax (0 = fixa, 1 = acompanha o mundo).
struct ParallaxLayer {
    Texture2D texture;
    float factor = 0.0f;
};

// Carrega o arquivo de configuração da fase (config.txt) e guarda tudo que
// a fase contém: camadas de parallax, plataformas, entidades, física e as
// texturas de cada tipo de sprite.
class Level {
public:
    // Lê e valida o config; constrói entidades. NÃO usa OpenGL
    // (pode ser chamado antes da janela existir).
    // Retorna false (com mensagem de erro descritiva) em caso de problema.
    bool load(const std::string &configPath);

    // Carrega as texturas dos sprites — requer contexto OpenGL corrente
    // (chamar depois de glfwMakeContextCurrent).
    bool loadTextures();

    // Restaura o estado inicial das entidades (usado quando o jogador morre).
    void reset();

    // Desenha as camadas de fundo com parallax (cada uma deslocada por
    // camX * fator, com tiling horizontal para cobrir a tela).
    void drawBackground(class SpriteRenderer &renderer, float camX,
                        float viewWidth, float viewHeight) const;

    // ---- Config da janela ----
    int windowWidth = 960;
    int windowHeight = 540;
    std::string windowTitle = "Mini-Mario -- Grau A";

    // ---- Mundo ----
    float worldWidth = 1920.0f;

    // ---- Física (constantes lidas do config) ----
    float gravity = 2200.0f;
    float moveSpeed = 300.0f;
    float jumpVelocity = 720.0f;
    float enemySpeed = 60.0f;

    // ---- Jogador ----
    glm::vec2 playerSpawn{60.0f, 460.0f};
    glm::vec2 playerFrame{32.0f, 32.0f};
    Texture2D playerTexture;

    // ---- Cenário e entidades ----
    std::vector<ParallaxLayer> layers;
    std::vector<Platform> platforms;
    std::vector<Coin> coins;
    std::vector<Enemy> enemies;
    Flag flag;

    glm::vec2 enemyFrame{32.0f, 32.0f};
    glm::vec2 coinFrame{32.0f, 32.0f};
    glm::vec2 flagFrame{32.0f, 64.0f};

    Texture2D platformTexture;
    Texture2D enemyTexture;
    Texture2D coinTexture;
    Texture2D flagTexture;

private:
    // Helpers do parser (definidos em Level.cpp)
    struct RawConfig {
        std::vector<std::pair<std::string, float>> parallaxLayers;
        std::vector<std::vector<float>> platforms;
        std::vector<std::vector<float>> coins;
        std::vector<std::vector<float>> enemies;
    };
    bool parseConfig(const std::string &configPath, RawConfig &raw);
    static std::vector<float> parseFloats(const std::string &csv, bool &ok);

    std::vector<std::string> mParallaxPaths; // caminhos das camadas (para loadTextures)
};
