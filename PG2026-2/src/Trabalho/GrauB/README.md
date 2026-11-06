# Grau B — Processamento de Imagens e Vídeo com OpenCV

## Equipe

- **Bernardo Dauber** — Ciência da Computação, Unisinos — `be.daubervieira@gmail.com`

Trabalho **individual** — um único autor para código, testes manuais e documentação.

---

## Descrição do Projeto

Este programa foi desenvolvido como parte da disciplina *Processamento Gráfico:
Fundamentos* (Unisinos, 2026/2), no **Trabalho Grau B**, com prazo de entrega em
**25/11/2026**. O objetivo é aplicar, na prática, conceitos de processamento
digital de imagens e vídeo — filtragem no domínio espacial, suavização, realce,
detecção de bordas, aritmética entre imagens, seleção de canais de cor e
composição com transparência (canal alpha) — através de uma aplicação
interativa, e não de um conjunto de scripts soltos.

O app tem dois modos, ambos em janelas do próprio OpenCV:

- **Modo Foto** (`--imagem`): carrega uma imagem do disco, permite selecionar o
  canal de cor (imagem completa, R, G ou B), aplicar 12 filtros, carregar uma
  segunda imagem e aplicar 4 operações aritméticas entre as duas, posicionar
  stickers com canal alpha por clique do mouse, salvar o resultado e restaurar
  a imagem original.
- **Modo Vídeo** (`--video`): exibe a webcam padrão ao vivo, permitindo trocar
  o canal de cor e o filtro em tempo real e salvar o quadro exibido.

**Stack escolhida: Python 3.12+ + [OpenCV](https://opencv.org/)
(`opencv-python`) + NumPy, gerenciados pelo [`uv`](https://docs.astral.sh/uv/).**
A interface é a janela *HighGUI* do OpenCV (`imshow`/`waitKey` e
`setMouseCallback`), o que dispensa qualquer framework gráfico externo: o
enunciado pede processamento de imagem/vídeo, e a janela do OpenCV já entrega
exibição, teclado e mouse.

**Por que não reutilizar o CMake do Grau A:** o Grau A é um jogo em C++/OpenGL
3.3 core (GLFW + GLAD + stb_image) cujo build depende de CMake e das
bibliotecas de desenvolvimento de OpenGL, instaladas com `sudo` — ferramentas
que não estão disponíveis na máquina usada para desenvolver este trabalho.
Além disso, o Grau B não tem shaders, transformações ou projeções: é
processamento de imagens, tarefa em que as funções prontas do OpenCV
(`cv2.blur`, `cv2.Sobel`, `cv2.addWeighted`, etc.) são o caminho natural. Por
isso o Grau B é um **projeto Python independente**, com `pyproject.toml` e
`uv.lock` próprios dentro de `PG2026-2/src/Trabalho/GrauB/`, sem nenhuma
alteração no build C++ existente.

---

## Estrutura do Projeto

```
GrauB/
├── pyproject.toml     # metadados do projeto e dependência opencv-python
├── uv.lock            # lockfile das dependências (gerado pelo uv)
├── README.md          # este arquivo
└── grau_b/
    ├── __init__.py    # versão do pacote (__version__)
    ├── app.py         # ponto de entrada e CLI
    ├── modes.py       # Modos Foto e Vídeo (janela, teclado, mouse, pipeline)
    ├── io_utils.py    # carregar / salvar / resetar imagem e salvar quadro
    ├── canais.py      # seleção de canal de cor (todos / R / G / B)
    ├── filtros.py     # registro dos 12 filtros de imagem
    ├── operacoes.py   # registro das 4 operações aritméticas entre imagens
    └── stickers.py    # geração dos 5 stickers RGBA e blending com alpha
```

| Arquivo | Descrição |
| --- | --- |
| `grau_b/app.py` | Ponto de entrada (`python -m grau_b.app`): parsing da linha de comando (`--imagem`, `--video`, `--version`), validação do caminho da imagem e tradução de falhas em códigos de saída. |
| `grau_b/modes.py` | `modo_foto()` e `modo_video()`: mantêm o estado da sessão (filtro, canal, operação, stickers), rodam o loop de `waitKey(30)`, tratam as teclas e o callback de mouse e aplicam o pipeline de exibição a cada quadro. |
| `grau_b/io_utils.py` | `carregar_imagem()`, `salvar_imagem()` (sufixo `_editado` ao lado da original), `salvar_frame()` (captura com carimbo de data/hora) e `resetar()` (cópia da imagem original). |
| `grau_b/canais.py` | `aplicar_selecao_canal()` — devolve a imagem com os outros dois canais zerados — e `proximo_canal()` para o ciclo `todos → R → G → B → todos`. |
| `grau_b/filtros.py` | Registro `FILTROS` com os 12 filtros (cada um com `id`, `nome`, `descricao` e `funcao`), mais `listar_filtros()`, `obter_filtro()`, `aplicar_filtro()` e `proximo_filtro()`. |
| `grau_b/operacoes.py` | Registro `OPERACOES` com as 4 operações aritméticas entre duas imagens, redimensionamento automático da segunda imagem e `aplicar_operacao()` / `proxima_operacao()`. |
| `grau_b/stickers.py` | Gera programaticamente os 5 stickers RGBA (80×80) em memória e expõe `listar_stickers()`, `obter_sticker()`, `proximo_sticker()` e `aplicar_sticker()` (blending por alpha, com recorte nas bordas). |
| `grau_b/__init__.py` | Versão do pacote (`__version__ = "0.1.0"`), usada por `--version`. |

---

## Informações Técnicas

- **Linguagem:** Python 3.12+ (`requires-python = ">=3.12"` no `pyproject.toml`)
- **API de processamento:** OpenCV via `opencv-python` (`>=5.0.0.93`), que traz o
  NumPy como dependência
- **Interface:** janelas *HighGUI* do OpenCV (`cv2.imshow`, `cv2.waitKey`,
  `cv2.setMouseCallback`) — é necessária uma sessão gráfica (X11/Wayland); não
  há suporte a terminal puro
- **Gerenciador de ambiente/dependências:** [`uv`](https://docs.astral.sh/uv/)
  (`pyproject.toml` + `uv.lock`), sem `pip`/`requirements.txt`
- **Plataforma-alvo/testada:** Linux (o backend gráfico usado foi o Qt embutido
  no `opencv-python`); o código não usa nada específico de sistema operacional

### Instalação

Dentro da pasta do projeto:

```bash
cd PG2026-2/src/Trabalho/GrauB
uv sync
```

O `uv sync` cria o `.venv` local, instala `opencv-python` (e NumPy) a partir do
`uv.lock` e instala o pacote `grau_b` em modo editável.

### Como rodar

```bash
# Modo Foto: abre a imagem indicada em uma janela
uv run python -m grau_b.app --imagem caminho/para/imagem.png

# Modo Vídeo: abre a webcam padrão (índice 0)
uv run python -m grau_b.app --video

# Ajuda e versão
uv run python -m grau_b.app --help
uv run python -m grau_b.app --version
```

`--imagem` e `--video` são **mutuamente exclusivos** e exatamente um deles é
obrigatório (passar os dois, ou nenhum, encerra com erro de uso do `argparse`).

### Controles — Modo Foto

| Entrada | Ação |
| --- | --- |
| `f` | Avança para o próximo filtro do ciclo (12 filtros + estado "nenhum"). A descrição do filtro é impressa no terminal antes de ele passar a valer para os quadros seguintes. |
| `c` | Avança o canal de cor: `todos → R → G → B → todos`. O canal atual é impresso no terminal. |
| `n` | Volta ao estado "sem filtro" (a imagem segue passando por canal, operação e stickers). |
| `o` | Pede no terminal o caminho de uma segunda imagem (é preciso digitar no terminal que lançou o app — a janela fica parada enquanto o `input()` espera). |
| `m` | Avança para a próxima operação aritmética entre a imagem original e a segunda imagem; a descrição da operação é impressa no terminal. Sem segunda imagem carregada, apenas avisa e não altera nada. |
| `t` | Avança para o próximo sticker; o nome é impresso no terminal. |
| clique esquerdo | Fixa o sticker atual **centrado** no ponto clicado, sobre a imagem exibida. Cliques sucessivos acumulam stickers. |
| `s` | Salva a imagem exibida (com filtro, canal, operação e stickers) em um arquivo ao lado da imagem original, com o sufixo `_editado` — ex.: `foto.png` → `foto_editado.png`. |
| `r` | Restaura a imagem original, zerando filtro, canal, operação e **todos** os stickers posicionados (a segunda imagem continua carregada e pode ser reutilizada com `m`). |
| `q` / `ESC` | Fecha a janela e encerra o app. |

### Controles — Modo Vídeo

| Entrada | Ação |
| --- | --- |
| `f` | Avança para o próximo filtro (descrição impressa no terminal). |
| `c` | Avança o canal de cor (`todos → R → G → B → todos`). |
| `n` | Volta ao estado "sem filtro". |
| `s` | Salva o quadro exibido como `captura_AAAAMMDD_HHMMSS_ffffff.png` na pasta atual. |
| `q` / `ESC` | Fecha a janela, libera a câmera e encerra o app. |

No Modo Vídeo não existem imagem original para restaurar (`r` é ignorado), nem
segunda imagem, operações (`o`/`m`) ou stickers (`t`/clique): o posicionamento
de stickers é exclusivo do Modo Foto, conforme o enunciado.

### Pipeline de processamento

A cada quadro, a imagem é processada nesta ordem
(`grau_b/modes.py::modo_foto`, `_processar_quadro` e `_aplicar_stickers`):

1. **Operação aritmética** (se houver uma ativa): o resultado de
   `operacao(imagem_original, segunda_imagem)` substitui a imagem de trabalho.
   As operações são sempre recalculadas a partir das duas imagens de entrada,
   então apertar `m` repetidamente **não** acumula resultados.
2. **Seleção de canal** (`canais.aplicar_selecao_canal`).
3. **Filtro** (`filtros.aplicar_filtro`).
4. **Stickers** já posicionados, na ordem dos cliques
   (`stickers.aplicar_sticker`).

No Modo Vídeo o pipeline é apenas canal → filtro, aplicado a cada frame da
webcam.

### Erros e códigos de saída

- Nenhum modo informado, ou `--imagem` junto com `--video`: mensagem de uso do
  `argparse`, código de saída `2`, sem abrir janela.
- `--imagem` apontando para arquivo inexistente: mensagem no `stderr`, código
  `2`, sem abrir janela.
- Arquivo existente mas que o OpenCV não consegue decodificar: mensagem no
  `stderr`, código `1`.
- Webcam indisponível (`VideoCapture.isOpened()` falso) ou falha ao ler um
  quadro: mensagem no `stderr`, código `1`, com a captura liberada.

### Verificação de sintaxe

A partir da raiz do repositório:

```bash
python3 -m compileall -q PG2026-2/src/Trabalho/GrauB
```

E, sem abrir janela, é possível conferir os registros e o pipeline:

```bash
cd PG2026-2/src/Trabalho/GrauB
uv run python -c "import numpy as np; from grau_b.filtros import listar_filtros, aplicar_filtro; from grau_b.operacoes import listar_operacoes; from grau_b.stickers import listar_stickers; from grau_b.canais import aplicar_selecao_canal; img = np.full((4, 4, 3), 100, np.uint8); print('filtros:', [f.id for f in listar_filtros()]); print('operacoes:', [o.id for o in listar_operacoes()]); print('stickers:', [s.id for s in listar_stickers()]); print('canal+filtro:', aplicar_filtro('negativo', aplicar_selecao_canal(img, 'R'))[0, 0])"
```

---

## Checklist de Requisitos

### Requisitos obrigatórios do enunciado

- [x] **Modo Foto** — carrega uma imagem do disco, exibe em janela, permite
  processar, salvar e resetar.
  `grau_b/app.py::main` + `grau_b/modes.py::modo_foto` +
  `grau_b/io_utils.py::carregar_imagem`/`salvar_imagem`/`resetar`.
- [x] **Modo Vídeo (webcam)** — captura contínua da câmera padrão (índice 0),
  exibida em tempo real e encerrada de forma limpa.
  `grau_b/app.py::_executar_modo_video` + `grau_b/modes.py::modo_video` +
  `grau_b/io_utils.py::salvar_frame`.
- [x] **No mínimo 10 filtros de imagem** — implementados **12**.
  `grau_b/filtros.py::FILTROS`; seleção pela tecla `f`
  (`modes.py::TECLA_PROXIMO_FILTRO` → `filtros.proximo_filtro`).
- [x] **No mínimo 1 filtro de suavização** — implementados **3**:
  `blur_media` (`cv2.blur`), `blur_gaussiano` (`cv2.GaussianBlur`) e
  `blur_mediana` (`cv2.medianBlur`), todos com kernel 5×5.
  `grau_b/filtros.py::_blur_media`/`_blur_gaussiano`/`_blur_mediana`.
- [x] **No mínimo 1 filtro de realce** — `realce_nitidez`
  (`cv2.filter2D` com kernel de nitidez `[[0,-1,0],[-1,5,-1],[0,-1,0]]`) e,
  adicionalmente, `equalizacao` (equalização de histograma na luminância).
  `grau_b/filtros.py::_realce_nitidez`/`_equalizacao`.
- [x] **Processar a imagem completa ou apenas um canal R/G/B** — ciclo
  `todos → R → G → B → todos` na tecla `c`; os outros dois canais são zerados.
  `grau_b/canais.py::aplicar_selecao_canal`/`proximo_canal` +
  `modes.py::TECLA_PROXIMO_CANAL`.
- [x] **Mostrar a descrição do filtro antes de aplicá-lo** — ao trocar de
  filtro (`f`) o nome e a descrição são impressos no terminal *antes* de o
  filtro passar a valer para os quadros seguintes; o mesmo vale para as
  operações (`m`) e para o estado "sem filtro" (`n`).
  `grau_b/modes.py::_imprimir_filtro`/`_imprimir_operacao` (campos `nome` e
  `descricao` de `grau_b/filtros.py::Filtro` e `grau_b/operacoes.py::Operacao`).
- [x] **No mínimo 3 operações aritméticas entre duas imagens** — implementadas
  **4** (soma, subtração, multiplicação e mescla 50/50).
  `grau_b/operacoes.py::OPERACOES`/`aplicar_operacao`; carga da segunda imagem
  pela tecla `o` (`modes.py::_carregar_segunda_imagem`) e seleção pela tecla
  `m` (`modes.py::TECLA_PROXIMA_OPERACAO`).
- [x] **No mínimo 5 stickers com canal alpha** — implementados **5**, gerados
  programaticamente como RGBA 80×80 (alpha 255 na forma, 0 no resto).
  `grau_b/stickers.py::STICKERS` (`gerar_estrela`, `gerar_coracao`,
  `gerar_circulo`, `gerar_triangulo`, `gerar_selo_ok`).
- [x] **Posicionar o sticker na imagem com o mouse** — clique esquerdo fixa o
  sticker atual centrado no ponto clicado; cliques sucessivos acumulam.
  `grau_b/modes.py::modo_foto::ao_clicar_mouse` +
  `grau_b/stickers.py::aplicar_sticker` (blending por alpha com recorte nas
  bordas), seleção do sticker atual pela tecla `t`
  (`modes.py::TECLA_PROXIMO_STICKER`).
- [x] **Salvar o resultado** — `s` no Modo Foto grava a imagem exibida ao lado
  da original com sufixo `_editado`; `s` no Modo Vídeo grava o quadro atual como
  `captura_<timestamp>.png` na pasta corrente.
  `grau_b/io_utils.py::salvar_imagem`/`salvar_frame`.
- [x] **Resetar** — `r` no Modo Foto restaura a imagem original e zera filtro,
  canal, operação e stickers posicionados.
  `grau_b/modes.py::modo_foto` (`TECLA_RESETAR`) + `grau_b/io_utils.py::resetar`.

### Filtros implementados (12)

| `id` | Nome exibido | Categoria | Base OpenCV |
| --- | --- | --- | --- |
| `negativo` | Negativo | cor | `cv2.bitwise_not` |
| `cinza` | Escala de cinza | cor | `cv2.cvtColor` |
| `blur_media` | Desfoque por média | **suavização** | `cv2.blur` (5×5) |
| `blur_gaussiano` | Desfoque Gaussiano | **suavização** | `cv2.GaussianBlur` (5×5) |
| `blur_mediana` | Desfoque de mediana | **suavização** | `cv2.medianBlur` (5×5) |
| `realce_nitidez` | Realce de nitidez | **realce** | `cv2.filter2D` (kernel de nitidez) |
| `bordas_sobel` | Bordas (Sobel) | bordas | `cv2.Sobel` + `cv2.magnitude` |
| `bordas_canny` | Bordas (Canny) | bordas | `cv2.Canny` |
| `relevo` | Relevo (Emboss) | estilo | `cv2.filter2D` (kernel de relevo) |
| `binarizacao` | Binarização | cor | `cv2.threshold` |
| `equalizacao` | Equalização de histograma | **realce** | `cv2.equalizeHist` (canal Y) |
| `sepia` | Sépia | estilo | `cv2.transform` (matriz sépia) |

Todos devolvem imagens BGR de 3 canais, então o pipeline de exibição e a
seleção de canal funcionam com qualquer filtro, em qualquer ordem.

### Operações aritméticas implementadas (4)

| `id` | Nome | O que faz | Base OpenCV |
| --- | --- | --- | --- |
| `soma` | Soma | Soma os pixels das duas imagens, saturando em 255 | `cv2.add` |
| `subtracao` | Subtração | Subtrai os pixels da segunda imagem dos da primeira, saturando em 0 | `cv2.subtract` |
| `multiplicacao` | Multiplicação | Normaliza as duas imagens para `[0, 1]`, multiplica e reescala para `[0, 255]` | `cv2.multiply` |
| `mescla` | Mescla | Média ponderada 50/50 entre as duas imagens | `cv2.addWeighted` |

A segunda imagem é redimensionada automaticamente para o tamanho da primeira
(`cv2.resize` dentro de `aplicar_operacao`), então as duas entradas podem ter
dimensões diferentes.

### Stickers implementados (5)

| `id` | Nome | Aparência | Alpha |
| --- | --- | --- | --- |
| `estrela` | Estrela | Estrela de 5 pontas amarela, preenchida (`cv2.fillPoly` sobre 10 vértices, alternando raio externo e interno) | 255 na estrela, 0 no fundo |
| `coracao` | Coração | Coração vermelho pelo contorno da curva paramétrica clássica | 255 no coração, 0 no fundo |
| `circulo` | Círculo | Círculo azul com borda mais escura | 255 no disco, 0 no fundo |
| `triangulo` | Triângulo | Triângulo equilátero verde, preenchido | 255 no triângulo, 0 no fundo |
| `selo_ok` | Selo OK | Círculo verde com um "check" branco desenhado dentro | 255 no disco, 0 no fundo |

Todos são gerados em memória por `grau_b/stickers.py` (nenhum arquivo de imagem
externo é lido) com tamanho fixo de 80×80 px.

### Bônus / itens não implementados

Os itens abaixo **não** foram implementados e estão registrados aqui para não
serem confundidos com requisitos atendidos:

- [ ] **Interface gráfica externa** (Qt, ImGui, Tkinter, etc.) — a interação
  acontece na janela HighGUI do OpenCV (teclado + clique do mouse); GUI própria
  é bônus e ficou fora do escopo.
- [ ] **Encadeamento de múltiplos filtros simultâneos** — o pipeline aplica
  **um** filtro por vez (canal → filtro → stickers).
- [ ] **Stickers animados** — os 5 stickers são estáticos, de tamanho fixo, sem
  animação, rotação ou redimensionamento no momento do clique.
- [ ] **Detecção facial** (Haar cascade / `cv2.CascadeClassifier`) — não
  implementada; nenhum filtro ou sticker é aplicado automaticamente sobre
  rostos.

---

## Screenshots / Vídeo de Gameplay

Esta seção é **opcional** no template do curso e as imagens **não** estão
embutidas neste README: o projeto foi documentado em um ambiente sem captura de
tela, então as capturas devem ser geradas por quem for avaliar (ou apresentar),
em poucos passos e com o próprio app:

1. Instale as dependências: `cd PG2026-2/src/Trabalho/GrauB && uv sync`.
2. **Modo Foto:** `uv run python -m grau_b.app --imagem caminho/para/imagem.png`.
   - aperte `f` algumas vezes para mostrar filtros diferentes (a descrição de
     cada um aparece no terminal);
   - aperte `c` para mostrar a seleção de canal (`R`, `G`, `B`, `todos`);
   - aperte `t` para escolher um sticker e clique na imagem para posicioná-lo;
   - aperte `s`: a imagem exibida é gravada ao lado da original como
     `<nome>_editado.<ext>` — esse arquivo já serve como captura do resultado.
3. **Operações aritméticas:** ainda no Modo Foto, aperte `o`, digite no
   terminal o caminho de uma segunda imagem, aperte `m` para ciclar entre
   `soma`, `subtracao`, `multiplicacao` e `mescla` e use `s` para gravar.
4. **Modo Vídeo:** `uv run python -m grau_b.app --video`, aperte `f`/`c` sobre a
   imagem da webcam e `s` para gravar o quadro atual
   (`captura_<timestamp>.png`).
5. Para capturar a **janela** (e não só a imagem processada), use a ferramenta
   de captura do sistema no momento em que a janela do OpenCV estiver aberta
   (ex.: `PrintScreen`/`Shift+PrintScreen` no GNOME, `spectacle` no KDE,
   `flameshot gui` ou `gnome-screenshot -i`); salve os arquivos como
   `screenshot_foto.png`, `screenshot_canal.png`, `screenshot_stickers.png` e
   `screenshot_video.png` e inclua-os na entrega/apresentação.

Se preferir demonstrar em vídeo, grave a tela durante a sequência acima (o
roteiro de teclas é o mesmo) — não é necessário editar nada no código.

---

## Referências e/ou créditos

- **OpenCV** — biblioteca de visão computacional usada em todos os filtros,
  operações, desenho dos stickers e na interface (HighGUI):
  <https://opencv.org/> · documentação: <https://docs.opencv.org/>
- **`opencv-python`** — pacote/wheels do OpenCV para Python (dependência
  declarada no `pyproject.toml`): <https://pypi.org/project/opencv-python/>
- **NumPy** — arrays usados como imagens (vem como dependência do
  `opencv-python`): <https://numpy.org/>
- **`uv`** — gerenciador de ambiente e dependências do projeto:
  <https://docs.astral.sh/uv/>
- **Template do README** — `PG2026-2/misc/Template_README_Projeto.md` (estrutura
  seguida neste documento)

**Créditos de assets:** este projeto **não usa nenhum asset externo**. Os 5
stickers são desenhados programaticamente em `grau_b/stickers.py` (formas
geométricas geradas com `cv2.fillPoly`/`cv2.circle`/`cv2.polylines` e uma
máscara de alpha própria), e as imagens de teste usadas durante o
desenvolvimento não fazem parte do repositório. Portanto não há banco de
imagens, pack de sprites ou licença de terceiros a atribuir.

---

## Comentários Finais

Pontos de atenção e limitações conhecidas (nenhum deles é um requisito do
enunciado que tenha ficado pendente):

- **Stickers só no Modo Foto.** O enunciado restringe o posicionamento por
  clique ao Modo Foto; no Modo Vídeo as teclas `t`/`r` e o clique não têm
  efeito.
- **Segunda imagem vem do terminal.** A tecla `o` usa `input()` no terminal que
  lançou o app; enquanto o caminho é digitado, a janela do OpenCV não é
  atualizada (comportamento esperado, não é travamento). Caminho inválido
  imprime o erro e mantém o estado anterior.
- **Operações não acumulam.** Ao apertar `m`, a operação é sempre recalculada a
  partir da imagem original e da segunda imagem carregada; o resultado anterior
  não é realimentado.
- **`r` não descarta a segunda imagem.** O reset restaura a imagem original e
  zera filtro/canal/operação/stickers, mas a segunda imagem continua carregada
  para ser reutilizada com `m`.
- **Parâmetros fixos no código.** Tamanho de kernel (5×5), limiar de
  binarização (127) e limiares do Canny (100/200) não são configuráveis pela
  interface nem pela linha de comando.
- **Câmera fixa no índice 0.** O Modo Vídeo sempre abre a webcam padrão; não há
  opção de escolher outro índice pela CLI, nem gravação de vídeo em arquivo
  (apenas captura de quadro único com `s`).
- **Validação automatizada é só de sintaxe.** Não há suíte de testes; o
  projeto é validado por `python3 -m compileall` e por inspeção visual/manual
  dos modos (as funções de filtro, operação e sticker também podem ser
  exercitadas sem janela, como mostrado em "Informações Técnicas").
