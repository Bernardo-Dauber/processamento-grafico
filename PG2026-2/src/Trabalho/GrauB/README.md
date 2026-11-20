# Grau B — Processamento de Imagens e Vídeo com OpenCV + Tkinter

## Equipe

- **Bernardo Dauber** — Ciência da Computação, Unisinos — `be.daubervieira@gmail.com`

Trabalho **individual** — um único autor para código, testes e documentação.

---

## Descrição do Projeto

Este programa foi desenvolvido como parte da disciplina *Processamento Gráfico:
Fundamentos* (Unisinos, 2026/2), no **Trabalho Grau B**, com prazo de entrega em
**25/11/2026**. O objetivo é aplicar, na prática, conceitos de processamento
digital de imagens e vídeo — filtragem no domínio espacial, suavização, realce,
detecção de bordas, combinação sequencial de filtros, aritmética entre imagens,
seleção de canais de cor, composição com transparência (canal alpha), animação
por spritesheet e detecção facial com Haar Cascades — em uma aplicação gráfica
interativa.

O aplicativo usa uma interface gráfica dedicada em **Tkinter (`ttk`) + Pillow**,
operando em dois modos alternáveis sem fechar o programa:

- **Modo Foto** (`--imagem` ou botão *Abrir Foto...*): carrega uma imagem do
  disco via diálogo nativo, permite selecionar o canal de cor (`Todos`, `R`, `G`
  ou `B`), montar um pipeline sequencial com 12 filtros (exibindo descrição e
  parâmetros **antes** de adicionar cada filtro), aplicar 4 operações aritméticas
  com uma segunda imagem, ativar filtros/adesivos de detecção facial, posicionar
  stickers estáticos e stickers animados por spritesheet com o mouse, salvar o
  resultado e restaurar ao original.
- **Modo Vídeo** (`--video` ou botão *Ligar Webcam*): captura a webcam padrão ao
  vivo com o mesmo pipeline completo (canal, múltiplos filtros em sequência,
  operações com segunda imagem, detecção facial em tempo real, stickers
  estáticos/animados, salvar quadro e restaurar).

**Stack escolhida:** Python 3.12+ + [OpenCV 4.x](https://docs.opencv.org/4.x/)
(`opencv-python>=4.10,<5`) + NumPy + [Pillow](https://python-pillow.org/) +
Tkinter (`ttk`), gerenciados pelo [`uv`](https://docs.astral.sh/uv/).

---

## Estrutura do Projeto

```
GrauB/
├── pyproject.toml                       # metadados do projeto e dependências
├── uv.lock                              # lockfile das dependências (uv)
├── README.md                            # este arquivo
└── grau_b/
    ├── __init__.py                      # versão do pacote (__version__)
    ├── app.py                           # ponto de entrada e CLI
    ├── modes.py                         # orquestração que abre a GUI
    ├── gui.py                           # interface gráfica Tkinter + Pillow
    ├── io_utils.py                      # carregar / salvar / resetar imagem
    ├── canais.py                        # seleção de canal de cor (todos / R / G / B)
    ├── filtros.py                       # 12 filtros + pipeline sequencial (EtapaFiltro)
    ├── operacoes.py                     # 4 operações aritméticas entre duas imagens
    ├── face.py                          # detecção facial (Haar Cascade) e efeitos no rosto
    ├── fundo.py                         # segmentação de pessoa (ONNX) para blur e fundo virtual
    ├── stickers.py                      # 5 stickers RGBA + 3 stickers animados (spritesheet)
    └── assets/
        ├── cascades/
        │   └── haarcascade_frontalface_default.xml
        ├── models/
        │   └── human_segmentation_pphumanseg_2023mar.onnx
        └── spritesheets/
            ├── estrela_pulsante.png     # spritesheet RGBA 640×80 (8 quadros de 80×80)
            ├── coracao_batendo.png      # spritesheet RGBA 640×80 (8 quadros de 80×80)
            └── arco_iris_girando.png    # spritesheet RGBA 640×80 (8 quadros de 80×80)
```

---

## Informações Técnicas

- **Linguagem:** Python 3.12+ (`requires-python = ">=3.12"` no `pyproject.toml`)
- **Bibliotecas:** `opencv-python>=4.10,<5` (OpenCV 4.x + NumPy), `pillow`, `tkinter` (`ttk`)
- **Gerenciador de ambiente:** [`uv`](https://docs.astral.sh/uv/) (`pyproject.toml` + `uv.lock`)

### Instalação

```bash
cd PG2026-2/src/Trabalho/GrauB
uv sync
```

### Como rodar

```bash
# Abre a interface gráfica pronta para escolher uma foto ou ligar a webcam
uv run python -m grau_b.app

# Modo Foto: abre direto com a imagem informada
uv run python -m grau_b.app --imagem caminho/para/imagem.png

# Modo Vídeo: abre direto com a webcam ligada (índice 0)
uv run python -m grau_b.app --video

# Ajuda e versão
uv run python -m grau_b.app --help
uv run python -m grau_b.app --version
```

### Pipeline de processamento

A cada quadro (foto ou webcam), a imagem passa nesta ordem:

1. **Operação aritmética** (se houver uma ativa com a 2ª imagem carregada, redimensionada automaticamente);
2. **Seleção de canal** (`Todos`, `R`, `G` ou `B`);
3. **Combinação sequencial de filtros** (`EtapaFiltro` 1 → 2 → ... → N, cada um com seu parâmetro);
4. **Detecção facial (Haar Cascade)** — aplica *Gatinho*, *Óculos Escuros* ou *Desfoque no Rosto* (e caixa de detecção opcional);
5. **Stickers posicionados** — 5 estáticos + 3 animados por spritesheet `.png` RGBA, na ordem dos cliques.

---

## Checklist de Requisitos

### Requisitos obrigatórios do enunciado

- [x] **Modo Foto** — carrega imagem do disco (`--imagem` ou diálogo nativo *Abrir Foto...*), processa, salva e reseta.
- [x] **Modo Vídeo (webcam)** — captura contínua da webcam padrão em tempo real; iniciar/parar pelo botão *Ligar/Desligar Webcam* ou `--video`.
- [x] **No mínimo 10 filtros de imagem** — implementados **12** em `grau_b/filtros.py`.
- [x] **No mínimo 1 filtro de suavização** — **3** (`blur_media`, `blur_gaussiano`, `blur_mediana`), com kernel ajustável (3 a 15).
- [x] **No mínimo 1 filtro de realce** — **2** (`realce_nitidez` com intensidade ajustável e `equalizacao` de histograma).
- [x] **Seleção de canais (inteira ou R/G/B)** — `grau_b/canais.py` (`todos`, `R`, `G`, `B`).
- [x] **Descrição dos filtros e parâmetros configuráveis** — exibidos na interface gráfica **antes** de adicionar o filtro ao pipeline.
- [x] **No mínimo 3 operações aritméticas entre duas imagens** — **4** em `grau_b/operacoes.py` (`soma`, `subtracao`, `multiplicacao`, `mescla`).
- [x] **No mínimo 5 stickers com canal alpha** — **5** estáticos + **3** animados por spritesheet (todos RGBA com alpha blending).
- [x] **Posicionar sticker com clique do mouse** — clique sobre a área da imagem (funciona em Foto e Vídeo).
- [x] **Salvar e Resetar** — diálogo nativo *Salvar...* (sem a interface) e botão *Restaurar Tudo*.

### Extras opcionais implementados (+1.0 ponto)

- [x] **Interface Gráfica Externa** — implementada com **Tkinter (`ttk`) + Pillow** em `grau_b/gui.py`, com painel lateral rolável, diálogos nativos de arquivo e atalhos (`Ctrl+O`, `Ctrl+S`, `Ctrl+R`, `ESC`).
- [x] **Combinação de Filtros** — lista ordenada editável (`aplicar_pipeline_filtros` em `grau_b/filtros.py`), permitindo empilhar múltiplos filtros em sequência, reordenar (`↑`/`↓`), editar o parâmetro de cada etapa individualmente e remover etapas.
- [x] **Stickers Animados por Spritesheet** — 3 spritesheets `.png` RGBA (`estrela_pulsante.png`, `coracao_batendo.png`, `arco_iris_girando.png`, 8 quadros cada) carregados e recortados em `grau_b/stickers.py`, com animação contínua em Foto e Vídeo.
- [x] **Filtros e Stickers com Detecção de Face** — `grau_b/face.py` utilizando OpenCV Haar Cascades (`haarcascade_frontalface_default.xml`), com 3 modos (*Filtro de Gatinho*, *Óculos Escuros* e *Desfoque no Rosto*) + opção de exibir a caixa de detecção, em Foto e Vídeo.

---

## Filtros, Operações, Efeitos Faciais e Stickers

### Filtros (12)

| `id` | Nome | Categoria | Parâmetro |
| --- | --- | --- | --- |
| `negativo` | Negativo | cor | — |
| `cinza` | Escala de cinza | cor | — |
| `blur_media` | Desfoque por média | **suavização** | `kernel` (3–15, passo 2) |
| `blur_gaussiano` | Desfoque Gaussiano | **suavização** | `kernel` (3–15, passo 2) |
| `blur_mediana` | Desfoque de mediana | **suavização** | `kernel` (3–15, passo 2) |
| `realce_nitidez` | Realce de nitidez | **realce** | `intensidade` (1–5) |
| `bordas_sobel` | Bordas (Sobel) | bordas | — |
| `bordas_canny` | Bordas (Canny) | bordas | `limiar` (10–200, passo 10) |
| `relevo` | Relevo (Emboss) | estilo | — |
| `binarizacao` | Binarização | cor | `limiar` (0–255, passo 8) |
| `equalizacao` | Equalização de histograma | **realce** | — |
| `sepia` | Sépia | estilo | — |

### Operações aritméticas (4)

- **Soma** (`cv2.add`), **Subtração** (`cv2.subtract`), **Multiplicação** (`cv2.multiply` normalizada) e **Mescla** (`cv2.addWeighted` 50/50).

### Efeitos de Detecção Facial (Haar Cascade)

- **Filtro de Gatinho** (orelhas acima da testa + focinho e bigodes proporcionais à face)
- **Óculos Escuros** (posicionados na altura dos olhos)
- **Desfoque no Rosto** (blur gaussiano localizado sobre cada face)
- **Caixa de detecção** (checkbox opcional)

### Stickers (5 estáticos + 3 animados por spritesheet)

- Estáticos (RGBA 80×80): `Estrela`, `Coração`, `Círculo`, `Triângulo`, `Selo OK`
- Animados por spritesheet `.png` (RGBA 640×80, 8 quadros): `Estrela Pulsante [Anim]`, `Coração Batendo [Anim]`, `Arco-Íris Girando [Anim]`

---

## Referências e Créditos

- **OpenCV 4.x** — processamento de imagem/vídeo e classificador oficial `haarcascade_frontalface_default.xml`: <https://docs.opencv.org/4.x/> · <https://github.com/opencv/opencv/tree/master/data/haarcascades>
- **Pillow** — integração de frames OpenCV/NumPy com `ImageTk.PhotoImage`: <https://python-pillow.org/>
- **NumPy** — manipulação matricial de imagens: <https://numpy.org/>
- **`uv`** — gerenciamento de ambiente e dependências: <https://docs.astral.sh/uv/>
