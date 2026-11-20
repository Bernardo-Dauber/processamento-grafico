# Mini-Mario — Trabalho Grau A (Processamento Gráfico)

## Equipe

- **Bernardo Dauber** — Ciência da Computação, Unisinos — `be.daubervieira@gmail.com`

Trabalho **individual** — um único autor para código, assets derivados, testes e documentação.

---

## Descrição do Projeto

Este programa foi desenvolvido como parte da disciplina *Processamento Gráfico: Fundamentos* (Unisinos, 2026/2) para o **Trabalho Prático do Grau A**. Trata-se de um jogo 2D *platformer* simplificado em **C++17 e OpenGL 3.3+** (pipeline programável com *shaders* obrigatórios), aplicando na prática:

- Renderização de sprites texturizados com transparência (`GL_BLEND`) sobre um único quad unitário compartilhado (`VAO` / `VBO` / `EBO`);
- Projeção ortográfica (`glm::ortho`) e matrizes de modelo (translação, escala e espelhamento horizontal);
- Animação baseada em *spritesheets* em grade regular com recorte de UV no *fragment shader*;
- Câmera 2D com acompanhamento horizontal e fundo em múltiplas camadas com efeito *parallax*;
- Física 2D (gravidade, pulo com altura variável) e detecção de colisão AABB (sólidos eixo a eixo + *triggers* de moedas, inimigos e bandeira);
- Leitura completa da fase e parâmetros a partir de arquivo externo de configuração (`config.txt`).

---

## Como Compilar e Executar

O build usa CMake a partir da pasta `PG2026-2/` (onde fica o `CMakeLists.txt`). O CMake copia automaticamente `config.txt` e `assets/` para o diretório do executável `GrauA`.

```bash
cd PG2026-2
cmake -S . -B build
cmake --build build --target GrauA -j
```

Execute a partir da pasta onde o executável foi gerado (para que ele encontre `config.txt` e `assets/`):

```bash
# Linux / macOS
cd build
./GrauA                  # jogar
./GrauA captura.png      # modo screenshot (captura o frame 30 e sai)

# Windows (PowerShell, MinGW / Ninja)
cd build
.\GrauA.exe              # jogar
.\GrauA.exe captura.png  # modo screenshot
```

---

## Como Jogar

| Tecla | Ação |
| --- | --- |
| `←` / `→` ou `A` / `D` | Andar |
| `Espaço` / `↑` / `W` | Pular (segurar = pulo mais alto; soltar cedo = pulo curto) |
| `R` | Reiniciar fase após a vitória |
| `ESC` | Sair do jogo |

Colete moedas, pule em cima dos inimigos para derrotá-los (encostar de lado mata), evite cair nos buracos e toque a **bandeira** para vencer. Morrer reinicia a fase automaticamente após o efeito visual de morte.

---

## Estrutura do Projeto

| Arquivo | Descrição |
| --- | --- |
| `main.cpp` | Inicialização GLFW/GLAD, *game loop*, máquina de estados (`Playing`, `Dying`, `Win`), input e *triggers*. |
| `Shader.hpp` / `Shader.cpp` | Compilação, linkagem e envio de *uniforms* para os *shaders* GLSL. |
| `Texture2D.hpp` / `Texture2D.cpp` | Carregamento de PNG via `stb_image` e criação de texturas OpenGL 2D. |
| `SpriteRenderer.hpp` / `SpriteRenderer.cpp` | Quad unitário compartilhado (`VAO`/`VBO`/`EBO`) e rotina de desenho de sprites. |
| `AABB.hpp` | Estrutura de *hitbox* alinhada aos eixos e teste de sobreposição (*overlap*). |
| `Animation.hpp` | Metadados de animação e cálculo de coordenadas UV por quadro do *spritesheet*. |
| `Camera.hpp` | Câmera 2D com deslocamento horizontal (*offset* X) acompanhando o jogador. |
| `Level.hpp` / `Level.cpp` | Parser do `config.txt`, carregamento das entidades e renderização do *parallax*. |
| `Player.hpp` / `Player.cpp` | Controle do jogador, física (gravidade e pulo variável) e resolução de colisão por eixos. |
| `Enemy.hpp` | Inimigo patrulheiro com inversão automática nos limites configurados. |
| `config.txt` | Definição completa da fase (janela, mundo, *parallax*, física, spawn, moedas, inimigos, plataformas e bandeira). |
| `assets/` | *Spritesheets* (`player.png`, `enemy.png`, `coin.png`, `flag.png`, `platform.png`) e camadas de *parallax* (`sky.png`, `hills.png`, `trees.png`). |
| `tools/make_assets.py` | Script auxiliar que gera os *spritesheets* e camadas de *parallax* a partir do pack CC0 da Kenney. |

---

## Informações Técnicas

- **Linguagem:** C++17 (`CMAKE_CXX_STANDARD 17`)
- **API Gráfica:** OpenGL 3.3+ (*Core Profile*, pipeline programável)
- **Dependências:** GLFW, GLAD, GLM, `stb_image` (baixadas automaticamente via `FetchContent` no CMake, exceto GLAD incluída em `PG2026-2/include/glad/` e `PG2026-2/Common/glad.c`)
- **Build:** CMake 3.11+ (`PG2026-2/CMakeLists.txt`)
- **Plataformas:** Linux, Windows e macOS

---

## Decisões de Implementação

- **Buffers e shaders:** Um único quad unitário (4 vértices, `VAO` + `VBO` + `EBO` com 2 triângulos via índices) é criado uma vez e reutilizado por todos os sprites. Cada vértice carrega apenas posição (`vec2`); o UV é derivado no *vertex shader* como `position + 0.5`. O *vertex shader* aplica `projection * model`; o *fragment shader* recorta o quadro do *spritesheet* com os *uniforms* `uvOffset` / `uvScale` e multiplica por `tint` (usado no flash vermelho de morte e no *fade* verde de vitória). `GL_BLEND` com `GL_SRC_ALPHA` / `GL_ONE_MINUS_SRC_ALPHA` garante a transparência dos PNGs.
- **Projeção e transformações:** `glm::ortho(0, 960, 540, 0)` define um mundo virtual `960×540` com Y crescendo para baixo (convenção 2D, 1:1 com pixels — ver [ADR 0001](../../../../docs/adr/0001-mundo-virtual-960x540-camera.md)). Posição e tamanho entram na matriz `model` (translação + escala via `spriteModel`); escala negativa em X espelha o sprite sem deslocar o quad.
- **Sprites e animação:** *Spritesheets* em grade regular (quadros de `32×32`). Cada animação corresponde a uma linha (`Animation{row, frameCount, frameTime}`); o quadro atual é `col = (timer / frameTime) % frameCount` e `uvOffset = (col * 32 / larguraTex, row * 32 / alturaTex)`. O jogador possui 3 estados (parado, andando e pulando → linhas 0, 1 e 2 de `player.png`), a moeda gira em 4 quadros e o inimigo anda em 2.
- **Controle e colisão:** Input por *polling* (`glfwGetKey`) com detecção de borda de subida no pulo. Colisões divididas em dois tipos:
  - **Triggers (overlap AABB):** moeda, bandeira e inimigo. Pular sobre o inimigo (velocidade vertical para baixo + pés próximos ao topo) o derrota e gera um quique no jogador; contato lateral mata.
  - **Sólidos (jogador × plataforma):** resolvidos **eixo a eixo** (move em X e resolve paredes; move em Y e resolve aterrissagem/`onGround` ou batida de cabeça usando a posição anterior). A *hitbox* do jogador (`20×30`) é menor que o sprite (`32×32`) para colisões mais justas.
- **Física:** `velocidade += gravidade * dt` por quadro; pulo como impulso vertical negativo com **pulo variável** (soltar a tecla cedo corta o impulso). Queda no vazio (`y > 540 + margem`) mata o jogador.
- **Parallax:** Cada camada de fundo possui fator `0..1` no `config.txt`; o deslocamento da câmera é multiplicado pelo fator e a camada repete horizontalmente (*tiling*) cobrindo a tela — céu fixo (`0.0`), montes distantes (`0.3`) e vegetação próxima (`0.6`).
- **Arquivo de configuração:** `config.txt` (seções + `chave = valor` + linhas CSV) permite editar toda a fase sem recompilar, com validação de linhas malformadas indicando o número da linha.
- **Estados do jogo:** `Playing → Dying` (*tint* vermelho, reinicia após 1 s) e `Playing → Win` (*fade* verde sobre a cena, reinicia com `R`).

---

## Checklist de Requisitos

- [x] Criação de janela e contexto OpenGL 3.3+ (GLFW + GLAD)
- [x] Configuração de *shaders* GLSL e pipeline programável
- [x] Projeção ortográfica e transformações (`model` / `projection`)
- [x] Carregamento de texturas com transparência (*alpha blending*)
- [x] Animação de personagens e coletáveis via *spritesheets*
- [x] Controle de movimentação, física (gravidade/pulo) e câmera 2D
- [x] Fundo em camadas com efeito *parallax*
- [x] Detecção e resolução de colisões AABB (plataformas, moedas, inimigos e objetivo)
- [x] Definição externa da fase via arquivo de configuração (`config.txt`)

---

## Referências e Créditos

- **Assets:** Gerados por `tools/make_assets.py` a partir do pack [**Pixel Platformer**](https://kenney.nl/assets/pixel-platformer) da Kenney (**CC0** — domínio público).
- **Bibliotecas:** [GLFW](https://www.glfw.org/), [GLAD](https://glad.dav1d.de/), [GLM](https://github.com/g-truc/glm), [stb_image](https://github.com/nothings/stb).
- **Documentação interna:** [ADR 0001 — Mundo virtual 960×540 e câmera](../../../../docs/adr/0001-mundo-virtual-960x540-camera.md) · [Enunciado do Trabalho Grau A](../../../../EnunciadoGrauA-Jogo.pdf) · [Índice do Repositório](../../../../README.md).
