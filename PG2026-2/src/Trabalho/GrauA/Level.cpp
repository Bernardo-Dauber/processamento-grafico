#include "Level.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

#include "Animation.hpp"
#include "SpriteRenderer.hpp"

// ---------------------------------------------------------------------------
// Parser do arquivo de configuração
//
// Formato: seções entre colchetes; dentro de cada seção, linhas "chave = valor"
// ou linhas CSV numéricas (listas de tamanho variável, ex.: plataformas).
// Comentários iniciam com '#'.
// ---------------------------------------------------------------------------

static std::string trim(const std::string &s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::vector<float> Level::parseFloats(const std::string &csv, bool &ok)
{
    std::vector<float> out;
    ok = true;
    std::stringstream ss(csv);
    std::string token;
    while (std::getline(ss, token, ',')) {
        try {
            out.push_back(std::stof(trim(token)));
        } catch (...) {
            ok = false;
            return out;
        }
    }
    ok = !out.empty();
    return out;
}

bool Level::parseConfig(const std::string &configPath, RawConfig &raw)
{
    std::ifstream file(configPath);
    if (!file.is_open()) {
        std::cerr << "ERROR::CONFIG::não foi possível abrir " << configPath << std::endl;
        return false;
    }

    std::string section;
    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line)) {
        ++lineNumber;
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;

        // Nova seção
        if (line.front() == '[' && line.back() == ']') {
            section = trim(line.substr(1, line.size() - 2));
            continue;
        }

        // Linha "chave = valor"
        size_t eq = line.find('=');
        if (eq != std::string::npos) {
            std::string key = trim(line.substr(0, eq));
            std::string value = trim(line.substr(eq + 1));

            if (section == "window" && key == "title") {
                windowTitle = value;
            } else if (section == "window" && key == "width") {
                windowWidth = std::stoi(value);
            } else if (section == "window" && key == "height") {
                windowHeight = std::stoi(value);
            } else if (section == "world" && key == "width") {
                worldWidth = std::stof(value);
            } else if (section == "physics" && key == "gravity") {
                gravity = std::stof(value);
            } else if (section == "physics" && key == "move_speed") {
                moveSpeed = std::stof(value);
            } else if (section == "physics" && key == "jump_velocity") {
                jumpVelocity = std::stof(value);
            } else if (section == "physics" && key == "enemy_speed") {
                enemySpeed = std::stof(value);
            } else if (section == "player" && key == "spawn") {
                bool ok;
                std::vector<float> v = parseFloats(value, ok);
                if (!ok || v.size() != 2) {
                    std::cerr << "ERROR::CONFIG::[player] spawn precisa de 2 valores (linha "
                              << lineNumber << ")" << std::endl;
                    return false;
                }
                playerSpawn = { v[0], v[1] };
            } else if (section == "player" && key == "frame_size") {
                bool ok;
                std::vector<float> v = parseFloats(value, ok);
                if (!ok || v.size() != 2) {
                    std::cerr << "ERROR::CONFIG::[player] frame_size precisa de 2 valores (linha "
                              << lineNumber << ")" << std::endl;
                    return false;
                }
                playerFrame = { v[0], v[1] };
            } else if (section == "enemy" && key == "frame_size") {
                bool ok;
                std::vector<float> v = parseFloats(value, ok);
                if (!ok || v.size() != 2) {
                    std::cerr << "ERROR::CONFIG::[enemy] frame_size precisa de 2 valores (linha "
                              << lineNumber << ")" << std::endl;
                    return false;
                }
                enemyFrame = { v[0], v[1] };
            } else if (section == "coin" && key == "frame_size") {
                bool ok;
                std::vector<float> v = parseFloats(value, ok);
                if (!ok || v.size() != 2) {
                    std::cerr << "ERROR::CONFIG::[coin] frame_size precisa de 2 valores (linha "
                              << lineNumber << ")" << std::endl;
                    return false;
                }
                coinFrame = { v[0], v[1] };
            } else if (section == "flag") {
                if (key == "frame_size") {
                    bool ok;
                    std::vector<float> v = parseFloats(value, ok);
                    if (!ok || v.size() != 2) {
                        std::cerr << "ERROR::CONFIG::[flag] frame_size precisa de 2 valores (linha "
                                  << lineNumber << ")" << std::endl;
                        return false;
                    }
                    flagFrame = { v[0], v[1] };
                } else if (key == "position") {
                    bool ok;
                    std::vector<float> v = parseFloats(value, ok);
                    if (!ok || v.size() != 2) {
                        std::cerr << "ERROR::CONFIG::[flag] position precisa de 2 valores (linha "
                                  << lineNumber << ")" << std::endl;
                        return false;
                    }
                    flag.position = { v[0], v[1] };
                }
            } else if (section == "parallax" && key == "layer") {
                // valor = "caminho.png, fator"
                size_t comma = value.find(',');
                if (comma == std::string::npos) {
                    std::cerr << "ERROR::CONFIG::[parallax] layer = \"caminho, fator\" (linha "
                              << lineNumber << ")" << std::endl;
                    return false;
                }
                std::string path = trim(value.substr(0, comma));
                float factor = std::stof(trim(value.substr(comma + 1)));
                raw.parallaxLayers.push_back({ path, factor });
            }
            // Chaves desconhecidas são ignoradas silenciosamente (extensível)
            continue;
        }

        // Linha CSV (lista de tamanho variável, dependente da seção)
        bool ok;
        std::vector<float> v = parseFloats(line, ok);
        if (!ok) {
            std::cerr << "ERROR::CONFIG::linha inválida na seção [" << section << "] (linha "
                      << lineNumber << "): \"" << line << "\"" << std::endl;
            return false;
        }
        if (section == "platform") {
            if (v.size() != 4) {
                std::cerr << "ERROR::CONFIG::[platform] precisa de \"x, y, largura, altura\" (linha "
                          << lineNumber << ")" << std::endl;
                return false;
            }
            raw.platforms.push_back(v);
        } else if (section == "coins") {
            if (v.size() != 2) {
                std::cerr << "ERROR::CONFIG::[coins] precisa de \"x, y\" (linha "
                          << lineNumber << ")" << std::endl;
                return false;
            }
            raw.coins.push_back(v);
        } else if (section == "enemies") {
            if (v.size() != 4) {
                std::cerr << "ERROR::CONFIG::[enemies] precisa de \"x, y, minX, maxX\" (linha "
                          << lineNumber << ")" << std::endl;
                return false;
            }
            raw.enemies.push_back(v);
        } else {
            std::cerr << "ERROR::CONFIG::lista CSV inesperada na seção [" << section << "] (linha "
                      << lineNumber << ")" << std::endl;
            return false;
        }
    }
    return true;
}

bool Level::load(const std::string &configPath)
{
    RawConfig raw;
    if (!parseConfig(configPath, raw))
        return false;

    // Camadas de parallax (as texturas só são carregadas em loadTextures,
    // que exige contexto OpenGL corrente)
    for (const auto &entry : raw.parallaxLayers) {
        ParallaxLayer layer;
        layer.factor = entry.second;
        mParallaxPaths.push_back(entry.first);
        layers.push_back(std::move(layer));
    }

    // Plataformas
    for (const auto &v : raw.platforms)
        platforms.push_back({ { v[0], v[1] }, { v[2], v[3] } });

    // Moedas
    for (const auto &v : raw.coins)
        coins.push_back({ { v[0], v[1] }, coinFrame });

    // Inimigos
    for (const auto &v : raw.enemies) {
        Enemy e;
        e.position = { v[0], v[1] };
        e.size = enemyFrame;
        e.initialPosition = e.position;
        e.minX = v[2];
        e.maxX = v[3];
        e.initialDirection = e.direction;
        enemies.push_back(e);
    }
    flag.size = flagFrame;

    std::cout << "Fase carregada: " << platforms.size() << " plataformas, "
              << coins.size() << " moedas, " << enemies.size() << " inimigos, "
              << layers.size() << " camadas de parallax" << std::endl;
    return true;
}

bool Level::loadTextures()
{
    for (size_t i = 0; i < layers.size(); ++i) {
        if (!layers[i].texture.load(mParallaxPaths[i])) {
            std::cerr << "ERROR::CONFIG::falha ao carregar camada de parallax: "
                      << mParallaxPaths[i] << std::endl;
            return false;
        }
    }

    // Sprites do jogo (caminhos fixos: um único pack de assets por projeto)
    if (!playerTexture.load("assets/player.png") ||
        !enemyTexture.load("assets/enemy.png") ||
        !coinTexture.load("assets/coin.png") ||
        !flagTexture.load("assets/flag.png") ||
        !platformTexture.load("assets/platform.png")) {
        std::cerr << "ERROR::CONFIG::falha ao carregar sprites de assets/" << std::endl;
        return false;
    }
    return true;
}

void Level::reset()
{
    for (Coin &c : coins) {
        c.collected = false;
        c.timer = 0.0f;
    }
    for (Enemy &e : enemies)
        e.reset();
}

void Level::drawBackground(SpriteRenderer &renderer, float camX,
                           float viewWidth, float viewHeight) const
{
    const glm::vec2 uv0(0.0f), uv1(1.0f);
    for (const ParallaxLayer &layer : layers) {
        if (layer.factor <= 0.0f) {
            // Camada fixa (ex.: céu): um único quad cobrindo a tela
            renderer.draw(layer.texture, spriteModel(glm::vec2(0, 0), glm::vec2(viewWidth, viewHeight)), uv0, uv1);
            continue;
        }
        // Camada com parallax: desloca por camX * fator e repete
        // horizontalmente (tiling) para cobrir a tela inteira.
        float offset = std::fmod(camX * layer.factor, viewWidth);
        for (float x = -offset; x < viewWidth; x += viewWidth)
            renderer.draw(layer.texture, spriteModel(glm::vec2(x, 0), glm::vec2(viewWidth, viewHeight)), uv0, uv1);
    }
}
