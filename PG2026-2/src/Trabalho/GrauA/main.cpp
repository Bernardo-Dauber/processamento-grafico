/*
 * Mini-Mario -- Grau A
 * Disciplina: Processamento Gráfico: Fundamentos (Unisinos, 2026/2)
 *
 * Jogo 2D (platformer simplificado) em OpenGL 3.3:
 *  - Sprites texturizados via UM quad unitário compartilhado (VAO/VBO/EBO),
 *    posicionado por model matrix e recortado do spritesheet por uniforms de UV;
 *  - Projeção ortográfica (mundo virtual 960x540, Y para baixo) + câmera
 *    horizontal com parallax de 3 camadas;
 *  - Física de plataforma (gravidade, pulo variável) e colisão por eixos
 *    separados (sólidos) + triggers AABB (moedas, inimigo, bandeira);
 *  - Fase definida em config.txt (parallax, plataformas, entidades, física).
 */

#include <algorithm>
#include <cmath>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Animation.hpp"
#include "AABB.hpp"
#include "Camera.hpp"
#include "Level.hpp"
#include "Player.hpp"
#include "Shader.hpp"
#include "SpriteRenderer.hpp"
#include "Texture2D.hpp"

// stb_image_write: usado só pelo modo screenshot (./GrauA captura.png)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

// ---------------------------------------------------------------------------
// Máquina de estados do jogo
//   Playing -> (tocou a bandeira)              -> Win
//   Playing -> (encostou no inimigo / caiu)    -> Dying -> Playing (reinicia)
// ---------------------------------------------------------------------------
enum class GameState { Playing, Dying, Win };

static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    (void)scancode;
    (void)mode;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

// Desenha uma moeda (4 frames girando na linha 0 da spritesheet)
static void drawCoin(SpriteRenderer &renderer, const Level &level, const Coin &coin, float camX)
{
    const float kFrameTime = 0.15f;
    int col = static_cast<int>(coin.timer / kFrameTime) % 4;
    glm::vec2 texSize(level.coinTexture.width(), level.coinTexture.height());
    renderer.draw(level.coinTexture,
                  spriteModel(glm::vec2(coin.position.x - camX, coin.position.y), coin.size),
                  uvOffsetFor(level.coinFrame, texSize, col, 0),
                  uvScaleFor(level.coinFrame, texSize));
}

// Desenha um inimigo (2 frames de andar na linha 0, espelhado pela direção)
static void drawEnemy(SpriteRenderer &renderer, const Level &level, const Enemy &enemy, float camX)
{
    const float kFrameTime = 0.15f;
    int col = static_cast<int>(enemy.timer / kFrameTime) % 2;
    glm::vec2 texSize(level.enemyTexture.width(), level.enemyTexture.height());
    glm::vec2 drawnSize = (enemy.direction > 0) ? enemy.size : glm::vec2(-enemy.size.x, enemy.size.y);
    renderer.draw(level.enemyTexture,
                  spriteModel(glm::vec2(enemy.position.x - camX, enemy.position.y), drawnSize),
                  uvOffsetFor(level.enemyFrame, texSize, col, 0),
                  uvScaleFor(level.enemyFrame, texSize));
}

int main(int argc, char **argv)
{
    // Modo opcional de captura: ./GrauA caminho.png grava o frame 30 e sai
    // (útil para verificar a renderização sem ferramentas externas)
    std::string screenshotPath = (argc > 1) ? argv[1] : "";
    // ---------------- GLFW / OpenGL 3.3 ----------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Janela padrão: o config (carregado já com contexto de GL, dentro do
    // escopo abaixo) redimensiona e renomeia conforme as suas seções
    GLFWwindow *window = glfwCreateWindow(960, 540, "Mini-Mario -- Grau A", nullptr, nullptr);
    if (!window) {
        std::cerr << "Falha ao criar a janela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Falha ao inicializar GLAD" << std::endl;
        glfwTerminate();
        return -1;
    }

    const GLubyte *version = glGetString(GL_VERSION);
    std::cout << "OpenGL " << version << std::endl;

    // Transparência dos PNGs (sprites) — obrigatório para spritesheets
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Todo recurso que usa o contexto OpenGL vive dentro deste escopo:
    // ele é fechado ANTES do glfwTerminate, garantindo que os destrutores
    // (glDeleteTextures) rodem com o contexto ainda corrente.
    {
        // Fase lida do config.txt (parse sem GL + texturas com GL)
        Level level;
        if (!level.load("config.txt")) {
            std::cerr << "Falha ao carregar config.txt (rode o executável a partir da pasta dele)"
                      << std::endl;
            glfwTerminate();
            return -1;
        }
        glfwSetWindowSize(window, level.windowWidth, level.windowHeight);
        glfwSetWindowTitle(window, level.windowTitle.c_str());

        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        glViewport(0, 0, fbWidth, fbHeight);

        if (!level.loadTextures()) {
            glfwTerminate();
            return -1;
        }


    // ---------------- Shaders ----------------
    // Um ÚNICO programa para tudo: a geometria é sempre o mesmo quad;
    // projection e model fazem o trabalho de posicionamento; uvOffset/uvScale
    // recortam o frame do spritesheet; tint dá o flash de dano e os fades.
    const GLchar *vertexShaderSource = R"glsl(
    #version 400
    layout (location = 0) in vec2 position;
    uniform mat4 projection;
    uniform mat4 model;
    out vec2 texCoord;
    void main()
    {
        gl_Position = projection * model * vec4(position, 0.0, 1.0);
        texCoord = position + vec2(0.5); // quad [-0.5, 0.5] -> UV [0, 1]
    }
    )glsl";

    const GLchar *fragmentShaderSource = R"glsl(
    #version 400
    in vec2 texCoord;
    out vec4 color;
    uniform sampler2D sprite;
    uniform vec2 uvOffset;
    uniform vec2 uvScale;
    uniform vec4 tint;
    void main()
    {
        vec4 texel = texture(sprite, uvOffset + texCoord * uvScale);
        if (texel.a < 0.1) discard;
        color = texel * tint;
    }
    )glsl";

    Shader shader;
    if (!shader.load(vertexShaderSource, fragmentShaderSource, "sprite")) {
        glfwTerminate();
        return -1;
    }
    shader.use();

    // Projeção ORTOGRÁFICA do mundo virtual (Y cresce para baixo)
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(level.windowWidth),
                                      static_cast<float>(level.windowHeight), 0.0f);
    shader.setMat4("projection", projection);

    // ---------------- Renderer e recursos ----------------
    SpriteRenderer renderer;
    if (!renderer.init(&shader)) {
        glfwTerminate();
        return -1;
    }
    Texture2D whiteTexture = Texture2D::solid(glm::vec4(1.0f)); // overlays de cor

    // ---------------- Entidades e estado do jogo ----------------
    Player player;
    player.spawn(level.playerSpawn);

    Camera camera;
    GameState state = GameState::Playing;
    float stateTimer = 0.0f; // conta o tempo dentro de Dying
    float winFade = 0.0f;    // alpha do fade verde de vitória
    bool prevJump = false;   // para detectar a borda de subida da tecla de pulo

    double prevS = glfwGetTime();
    double titleCountdownS = 0.1;
    int frameCount = 0;

    // ---------------- Game loop ----------------
    while (!glfwWindowShouldClose(window)) {
        // dt do frame (limitado para o jogo não "explodir" em hiccups)
        double currS = glfwGetTime();
        float dt = static_cast<float>(std::min(currS - prevS, 0.05));
        prevS = currS;

        // FPS no título (convenção da disciplina)
        titleCountdownS -= dt;
        if (titleCountdownS <= 0.0 && dt > 0.0) {
            char tmp[256];
            snprintf(tmp, sizeof(tmp), "%s | FPS %.2f", level.windowTitle.c_str(), 1.0 / dt);
            glfwSetWindowTitle(window, tmp);
            titleCountdownS = 0.1;
        }

        glfwPollEvents();

        // ------------- Input (teclado) -------------
        PlayerInput input;
        input.left = glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS ||
                     glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
        input.right = glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS ||
                      glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
        input.jumpHeld = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS ||
                         glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS ||
                         glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
        input.jumpPressed = input.jumpHeld && !prevJump;
        prevJump = input.jumpHeld;

        bool restartRequested = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;

        float camX = camera.x();
        camera.update(player.position.x + player.size.x * 0.5f, level.worldWidth,
                      static_cast<float>(level.windowWidth));
        camX = camera.x();

        // ------------- Lógica por estado -------------
        if (state == GameState::Playing) {
            player.update(dt, level, input);

            // Moedas: trigger de coleta (overlap AABB)
            for (Coin &coin : level.coins) {
                coin.update(dt);
                if (!coin.collected &&
                    player.hitbox().overlaps(aabbFrom(coin.position, coin.size)))
                    coin.collected = true;
            }

            // Inimigos: patrulha + trigger (pisão mata o inimigo; contato lateral mata o jogador)
            for (Enemy &enemy : level.enemies) {
                enemy.update(dt, level.enemySpeed);
                if (!enemy.alive ||
                    !player.hitbox().overlaps(enemy.hitbox()))
                    continue;

                AABB phb = player.hitbox();
                AABB ehb = enemy.hitbox();
                bool stomping = player.velocity.y > 0.0f &&
                                (phb.bottom() - ehb.y) < enemy.size.y * 0.6f;
                if (stomping) {
                    enemy.alive = false;
                    player.velocity.y = -level.jumpVelocity * 0.55f; // quique
                } else {
                    state = GameState::Dying;
                    stateTimer = 0.0f;
                }
            }

            // Bandeira: trigger de vitória
            if (state == GameState::Playing &&
                player.hitbox().overlaps(aabbFrom(level.flag.position, level.flag.size))) {
                state = GameState::Win;
                winFade = 0.0f;
                std::cout << "Fase concluida!" << std::endl;
            }

            // Queda no vazio
            if (player.position.y > level.windowHeight + 120.0f) {
                state = GameState::Dying;
                stateTimer = 0.0f;
            }
        } else if (state == GameState::Dying) {
            stateTimer += dt;
            if (stateTimer >= 1.0f) {
                // Reinicia a fase: entidades restauradas + jogador no spawn
                level.reset();
                player.spawn(level.playerSpawn);
                state = GameState::Playing;
            }
        } else { // Win
            winFade += dt * 0.8f;
        }

        if (restartRequested && state == GameState::Win) {
            level.reset();
            player.spawn(level.playerSpawn);
            winFade = 0.0f;
            state = GameState::Playing;
        }

        // ------------- Renderização -------------
        glClearColor(0.55f, 0.78f, 1.0f, 1.0f); // cor do céu (atrás das camadas)
        glClear(GL_COLOR_BUFFER_BIT);

        shader.use();

        // 1) Fundo com parallax (cada camada usa uma fração do offset da câmera)
        level.drawBackground(renderer, camX, static_cast<float>(level.windowWidth),
                             static_cast<float>(level.windowHeight));

        // 2) Plataformas (imagem esticada para o retângulo do config)
        for (const Platform &p : level.platforms)
            renderer.draw(level.platformTexture,
                          spriteModel(glm::vec2(p.position.x - camX, p.position.y), p.size),
                          glm::vec2(0.0f), glm::vec2(1.0f));

        // 3) Moedas (só as não coletadas)
        for (const Coin &coin : level.coins)
            if (!coin.collected)
                drawCoin(renderer, level, coin, camX);

        // 4) Inimigos
        for (const Enemy &enemy : level.enemies)
            if (enemy.alive)
                drawEnemy(renderer, level, enemy, camX);

        // 5) Bandeira (imagem única, frame inteiro)
        renderer.draw(level.flagTexture,
                      spriteModel(glm::vec2(level.flag.position.x - camX, level.flag.position.y),
                                  level.flag.size),
                      glm::vec2(0.0f), glm::vec2(1.0f));

        // 6) Jogador (com tint vermelho durante a morte)
        glm::vec4 playerTint = (state == GameState::Dying)
                                   ? glm::vec4(1.0f, 0.15f, 0.15f, 1.0f)
                                   : glm::vec4(1.0f);
        player.draw(renderer, level.playerTexture, camX, playerTint);

        // 7) Overlay de vitória (fade verde por cima da cena)
        if (state == GameState::Win) {
            float alpha = std::min(winFade, 0.85f);
            renderer.draw(whiteTexture,
                          spriteModel(glm::vec2(0.0f), glm::vec2(level.windowWidth, level.windowHeight)),
                          glm::vec2(0.0f), glm::vec2(1.0f),
                          glm::vec4(0.15f, 0.75f, 0.3f, alpha));
        }

        // Modo screenshot: captura o framebuffer e encerra
        if (!screenshotPath.empty() && frameCount >= 30) {
            std::vector<unsigned char> pixels(static_cast<size_t>(fbWidth * fbHeight) * 4);
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glReadPixels(0, 0, fbWidth, fbHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            stbi_flip_vertically_on_write(1); // OpenGL lê de baixo para cima
            stbi_write_png(screenshotPath.c_str(), fbWidth, fbHeight, 4, pixels.data(), fbWidth * 4);
            std::cout << "Screenshot salva em " << screenshotPath << std::endl;
            break;
        }
        ++frameCount;

        glfwSwapBuffers(window);
    }
    } // fim do escopo dos recursos de OpenGL (destrutores rodam aqui)

    glfwTerminate();
    return 0;
}
