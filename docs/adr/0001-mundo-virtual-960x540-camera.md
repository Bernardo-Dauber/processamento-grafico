# ADR 0001 — Mundo virtual fixo de 960×540 com câmera de scroll horizontal

**Status:** Aceito

**Contexto:** O jogo é pixel art 2D e o arquivo de configuração da fase precisa de coordenadas estáveis. A janela GLFW pode ser redimensionada; colisões e layout da fase não podem depender do tamanho da janela.

**Decisão:**
- O mundo usa um sistema de coordenadas virtual fixo: **960×540 unidades** ("pixels do jogo"), 16:9.
- A projeção ortográfica é `ortho(0, 960, 540, 0)` (origem no canto superior esquerdo, Y para baixo — convenção de jogos 2D).
- A fase tem largura maior que a tela (2–3× de 960). A **câmera** é um offset X que segue o Jogador e é aplicado como translação na model matrix de cada objeto.
- Cada Camada de parallax aplica uma fração do offset da câmera (mais distante = fração menor).

**Consequências:**
- Config da fase em coordenadas do mundo virtual, independentes da janela.
- Sprites de ~32×32 px escalam bem nessa resolução.
- Redimensionar a janela estica a imagem (aceitável para o escopo).
