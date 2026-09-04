# Mini-Mario — Trabalho Grau A (Processamento Gráfico)

Jogo 2D platformer simplificado em **OpenGL 3.3+** (pipeline programável, shaders
obrigatórios), desenvolvido para o Trabalho Prático da disciplina
**Processamento Gráfico: Fundamentos** (Unisinos, 2026/2).

Executável: `GrauA` (rode a partir da pasta do executável, onde o CMake copia
`config.txt` e `assets/`).

```bash
cmake ..            # na pasta build/
cmake --build . --target GrauA
cd build && ./GrauA             # jogar
./GrauA captura.png             # modo screenshot (captura o frame 30 e sai)
```

## Como jogar

| Tecla | Ação |
|---|---|
| ← / → ou A / D | andar |
| Espaço / ↑ / W | pular (segurar = pulo mais alto; soltar cedo = pulo curto) |
| ESC | sair |

Colete moedas, pule em cima dos inimigos para derrotá-los (encostar de lado
mata), não caia nos buracos e toque a **bandeira** para vencer. Morrer
reinicia a fase automaticamente; `R` recomeça após a vitória.

## Estrutura do código

```
src/Trabalho/GrauA/
├── main.cpp             # GLFW, game loop, máquina de estados, input, triggers
├── Shader.hpp/.cpp      # compilação/link de shaders + uniforms
├── Texture2D.hpp/.cpp   # carregamento de PNG (stb_image) → textura GL
├── SpriteRenderer.hpp/.cpp  # O quad unitário compartilhado (VAO/VBO/EBO) + draw
├── AABB.hpp             # hitbox alinhada aos eixos + teste de overlap
├── Animation.hpp        # metadados de animação + UV do frame do spritesheet
├── Camera.hpp           # offset X que segue o jogador
├── Level.hpp/.cpp       # parser do config.txt + entidades + parallax
├── Player.hpp/.cpp      # input, física (gravidade/pulo), colisão por eixos
├── Enemy.hpp            # inimigo patrulheiro (vira nos limites)
├── config.txt           # TODA a definição da fase (ver abaixo)
├── assets/              # spritesheets e camadas de parallax (CC0, ver abaixo)
└── tools/make_assets.py # gera os assets a partir do pack da Kenney
```

## Decisões de implementação (roteiro da apresentação)

**Buffers e shaders.** Um ÚNICO quad unitário (4 vértices, VAO + VBO + EBO com
2 triângulos via índices) é criado uma vez e reutilizado por todos os sprites.
Cada vértice tem só posição (vec2); o UV é derivado no vertex shader como
`position + 0.5`. O vertex shader aplica `projection * model`; o fragment
shader recorta o frame do spritesheet com os uniforms `uvOffset`/`uvScale` e
multiplica pelo `tint` (usado no flash vermelho de morte e no fade de vitória).
`GL_BLEND` com `SRC_ALPHA / ONE_MINUS_SRC_ALPHA` para a transparência dos PNGs.

**Projeção e transformações.** `glm::ortho(0, 960, 540, 0)` — mundo virtual
960×540 com Y crescendo para baixo (convenção 2D, mapeia 1:1 com pixels).
A posição/tamanho de cada sprite entram numa **model matrix**
(translação + escala, montada por `spriteModel`); escala negativa em X espelha
o sprite (virar de lado) sem deslocar o quad.

**Sprites e animação.** Spritesheets em grade regular (frames de 32×32).
A animação é uma linha da grade: `Animation{row, frameCount, frameTime}`;
o frame atual vira `col = (timer/frameTime) % frameCount` e o UV é aritmética:
`uvOffset = (col*32/larguraTex, row*32/alturaTex)`. O jogador tem 3 estados
(parado/andando/pulando → linhas 0/1/2 da `player.png`), a moeda gira em 4
frames e o inimigo anda em 2.

**Controle e colisão.** Input por polling (`glfwGetKey`), com detecção de
"borda de subida" da tecla de pulo. Colisões de dois tipos:
- **Triggers (overlap AABB):** moeda, bandeira e inimigo. Pular em cima do
  inimigo (velocidade vertical para baixo + pés perto do topo dele) o derrota
  e dá um quique no jogador; contato lateral mata.
- **Sólidos (jogador × plataforma):** resolvidos **eixo a eixo** — move em X e
  resolve paredes; move em Y e resolve aterrissagem (`onGround`) ou batida de
  cabeça, usando a posição do frame anterior para saber de onde o jogador veio.
  A hitbox do jogador (20×30) é menor que o sprite (32×32) para encostos de
  raspão não serem injustos.

**Física.** `velocidade += gravidade*dt` por frame; pulo = impulso vertical
negativo; **pulo variável**: soltar a tecla cedo corta o impulso. Queda no
vazio (`y > 540 + margem`) mata.

**Parallax.** Cada camada de fundo tem um fator `0..1` no config; o deslocamento
da câmera é multiplicado pelo fator e a camada repete horizontalmente (tiling)
para cobrir a tela. Três camadas: céu (fixa), montes distantes (0.3) e
vegetação próxima (0.6).

**Arquivo de configuração.** `config.txt` (texto próprio, seções + `chave =
valor` + linhas CSV) define: janela, largura do mundo, camadas de parallax,
constantes de física, spawn/frames do jogador, inimigos (com limites de
patrulha), moedas, plataformas e bandeira — a fase inteira é editável sem
recompilar. O parser valida linhas malformadas com número de linha.

**Estados do jogo.** `Playing → Dying` (tint vermelho, reinicia após 1s) e
`Playing → Win` (fade verde por cima da cena). Extras (HUD com FreeType, som,
telas de menu) ficaram fora do escopo.

## Assets

Gerados por `tools/make_assets.py` a partir do pack
[**Pixel Platformer**](https://kenney.nl/assets/pixel-platformer) da Kenney
(licença **CC0** — domínio público). O pack distribui sprites individuais
(18×18 tiles, 24×24 personagens); o script os combina em spritesheets em grade
regular e compõe as camadas de parallax 960×540 (céu com nuvens, montes,
vegetação) e a bandeira.

## Autor

Bernardo Dauber — Ciência da Computação, Unisinos.
