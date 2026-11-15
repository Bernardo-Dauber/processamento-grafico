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

O app é uma única janela do próprio OpenCV com uma barra lateral de comandos, e
dois modos que se alternam sem reabrir o programa (tecla `v`):

- **Modo Foto** (`--imagem`): carrega uma imagem do disco, permite selecionar o
  canal de cor (imagem completa, R, G ou B), aplicar 12 filtros (com
  parâmetros ajustáveis), carregar uma segunda imagem e aplicar 4 operações
  aritméticas entre as duas, posicionar stickers com canal alpha por clique do
  mouse, salvar o resultado e restaurar tudo ao original.
- **Modo Vídeo** (`--video` ou tecla `v`): exibe a webcam padrão ao vivo, com
  canal, filtros (e parâmetros), operações com a segunda imagem, salvar quadro
  e restaurar. Os stickers existem só no Modo Foto.

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
    ├── modes.py       # loop único Foto/Vídeo (janela, teclado, mouse, pipeline)
    ├── io_utils.py    # carregar / salvar / resetar imagem e salvar quadro
    ├── canais.py      # seleção de canal de cor (todos / R / G / B)
    ├── filtros.py     # registro dos 12 filtros de imagem
    ├── operacoes.py   # registro das 4 operações aritméticas entre imagens
    ├── teclas.py      # códigos de tecla compartilhados pelo loop e pelo painel
    ├── sidebar.py     # barra lateral de comandos desenhada ao lado da imagem
    └── stickers.py    # geração dos 5 stickers RGBA e blending com alpha
```

| Arquivo | Descrição |
| --- | --- |
| `grau_b/app.py` | Ponto de entrada (`python -m grau_b.app`): parsing da linha de comando (`--imagem`, `--video`, `--version`), validação do caminho da imagem e tradução de falhas em códigos de saída. |
| `grau_b/modes.py` | `executar()`: mantém o estado da sessão (`Sessao`: foto, webcam, filtro e parâmetros, canal, operação, stickers), roda o loop de `waitKey(30)`, trata teclas e mouse e aplica o pipeline a cada quadro, vindo da foto ou da webcam. |
| `grau_b/io_utils.py` | `carregar_imagem()`, `salvar_imagem()` (sufixo `_editado` ao lado da original), `salvar_frame()` (captura com carimbo de data/hora) e `resetar()` (cópia da imagem original). |
| `grau_b/canais.py` | `aplicar_selecao_canal()` — devolve a imagem com os outros dois canais zerados — e `proximo_canal()` para o ciclo `todos → R → G → B → todos`. |
| `grau_b/filtros.py` | Registro `FILTROS` com os 12 filtros (cada um com `id`, `nome`, `descricao`, `funcao` e, quando existe, um `Parametro` configurável), mais `listar_filtros()`, `obter_filtro()`, `aplicar_filtro()`, `proximo_filtro()`, `ajustar_parametro()` e `descrever_filtro()`. |
| `grau_b/operacoes.py` | Registro `OPERACOES` com as 4 operações aritméticas entre duas imagens, redimensionamento automático da segunda imagem e `aplicar_operacao()` / `proxima_operacao()`. |
| `grau_b/sidebar.py` | `compor_janela()` anexa à direita da imagem uma barra lateral (desenhada com `cv2.rectangle`/`putText`) com listas de Canal, Filtro (nome, posição e descrição), Operação, Sticker e as teclas de atalho, além de uma barra de status com o arquivo e a última ação. Cada botão registra uma região clicável (`Regiao`) tratada pelo callback de mouse em `modes.py`; a imagem salva não inclui a barra. |
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

# Modo Vídeo: abre direto na webcam padrão (índice 0)
uv run python -m grau_b.app --video

# Foto com a webcam já ligada ('v' volta para a foto)
uv run python -m grau_b.app --imagem caminho/para/imagem.png --video

# Ajuda e versão
uv run python -m grau_b.app --help
uv run python -m grau_b.app --version
```

É preciso informar `--imagem`, `--video` ou os dois; sem nenhum, o `argparse`
encerra com erro de uso.

### Controles

Tudo o que existe no teclado também é **clicável na barra lateral**: canais, a lista
completa de filtros, os botões `-`/`+` do parâmetro, as operações (clicar de novo
desliga), "Carregar 2a imagem", os stickers e os botões Salvar, Restaurar,
Ligar/Desligar webcam, Abrir foto e Sair. Os atalhos de teclado abaixo continuam
valendo.

| Entrada | Ação |
| --- | --- |
| `f` | Próximo filtro do ciclo (12 filtros + "nenhum"). O nome, a descrição e os parâmetros configuráveis são impressos no terminal e mostrados na barra lateral. |
| `n` | Volta ao estado "sem filtro". |
| `+` / `-` | Aumenta/diminui o parâmetro do filtro atual (kernel dos desfoques, intensidade da nitidez, limiar do Canny, limiar da binarização). Filtros sem parâmetro avisam e nada muda. |
| `c` | Canal de cor: `todos → R → G → B → todos`. |
| `o` | Pede no terminal o caminho de uma segunda imagem (a janela fica parada enquanto o `input()` espera). |
| `m` | Próxima operação aritmética entre a imagem exibida (foto ou quadro da webcam) e a segunda imagem. Sem segunda imagem, apenas avisa. |
| `t` | Próximo sticker (só na foto). |
| clique esquerdo | Fixa o sticker atual **centrado** no ponto clicado, só na foto e dentro da imagem. Cliques sucessivos acumulam. Cliques na barra lateral acionam os botões dela. |
| `v` | Liga/desliga a webcam. Desligar volta para a foto; sem foto carregada a webcam não desliga (use `p` antes). |
| `p` | Pede no terminal o caminho de uma foto e a carrega no lugar da atual. |
| `s` | Salva o que está na tela, sem a barra lateral: foto em `<nome>_editado.<ext>` ao lado da original; quadro da webcam em `captura_AAAAMMDD_HHMMSS_ffffff.png` na pasta atual. |
| `r` | Restaura tudo: zera filtro, parâmetros, canal, operação e stickers (a segunda imagem continua carregada). Vale para foto e webcam. |
| `q` / `ESC` | Fecha a janela e encerra o app. |

### Pipeline de processamento

A cada quadro, a imagem é processada nesta ordem
(`grau_b/modes.py::_processar` e `_aplicar_stickers`), igual para foto e webcam:

1. **Operação aritmética** (se houver uma ativa): o resultado de
   `operacao(imagem_de_entrada, segunda_imagem)` substitui a imagem de trabalho.
   É sempre recalculado a partir das duas entradas (na foto, com cache), então
   apertar `m` repetidamente **não** acumula resultados. A segunda imagem é
   redimensionada para o tamanho da entrada.
2. **Seleção de canal** (`canais.aplicar_selecao_canal`).
3. **Filtro**, com o parâmetro atual (`filtros.aplicar_filtro`).
4. **Stickers** já posicionados, na ordem dos cliques (só na foto).

### Erros e códigos de saída

- Nem `--imagem` nem `--video`: mensagem de uso do `argparse`, código `2`, sem
  abrir janela.
- `--imagem` apontando para arquivo inexistente: mensagem no `stderr`, código
  `2`, sem abrir janela.
- Arquivo existente mas que o OpenCV não consegue decodificar: mensagem no
  `stderr`, código `1`.
- Webcam indisponível ao iniciar: sem foto, mensagem no `stderr` e código `1`;
  com foto, o app avisa e abre na foto. Se falhar ao ligar com `v`, a barra de
  status avisa e a foto continua. Se a webcam parar durante o uso, o app volta
  para a foto (ou encerra com código `1` se não houver foto).

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
  `grau_b/app.py::main` + `grau_b/modes.py::executar` +
  `grau_b/io_utils.py::carregar_imagem`/`salvar_imagem`/`resetar`.
- [x] **Modo Vídeo (webcam)** — captura contínua da câmera padrão (índice 0),
  exibida em tempo real; **iniciar/parar** pela tecla `v` ou por `--video`.
  `grau_b/modes.py::executar` (`_ligar_webcam`/`_desligar_webcam`) +
  `grau_b/io_utils.py::salvar_frame`.
- [x] **No mínimo 10 filtros de imagem** — implementados **12**.
  `grau_b/filtros.py::FILTROS`; seleção pela tecla `f`
  (`modes.py::TECLA_PROXIMO_FILTRO` → `filtros.proximo_filtro`).
- [x] **No mínimo 1 filtro de suavização** — implementados **3**:
  `blur_media` (`cv2.blur`), `blur_gaussiano` (`cv2.GaussianBlur`) e
  `blur_mediana` (`cv2.medianBlur`), todos com kernel configurável (padrão 5×5, ímpar de 3 a 15).
  `grau_b/filtros.py::_blur_media`/`_blur_gaussiano`/`_blur_mediana`.
- [x] **No mínimo 1 filtro de realce** — `realce_nitidez`
  (`cv2.filter2D` com kernel de nitidez `[[0,-1,0],[-1,5,-1],[0,-1,0]]`) e,
  adicionalmente, `equalizacao` (equalização de histograma na luminância).
  `grau_b/filtros.py::_realce_nitidez`/`_equalizacao`.
- [x] **Processar a imagem completa ou apenas um canal R/G/B** — ciclo
  `todos → R → G → B → todos` na tecla `c`; os outros dois canais são zerados.
  `grau_b/canais.py::aplicar_selecao_canal`/`proximo_canal` +
  `modes.py::TECLA_PROXIMO_CANAL`.
- [x] **Mostrar a descrição do filtro e seus parâmetros configuráveis** — ao
  trocar de filtro (`f`) o nome, a descrição e os parâmetros (nome, faixa e valor
  atual, ajustáveis com `+`/`-`) são impressos no terminal e mostrados na
  barra lateral; o mesmo vale para as operações (`m`).
  `grau_b/filtros.py::descrever_filtro`/`ajustar_parametro` +
  `grau_b/modes.py::_imprimir_filtro`/`_imprimir_operacao`.
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
  `grau_b/modes.py::executar` (callback `ao_clicar_mouse`) +
  `grau_b/stickers.py::aplicar_sticker` (blending por alpha com recorte nas
  bordas), seleção do sticker atual pela tecla `t`
  (`modes.py::TECLA_PROXIMO_STICKER`).
- [x] **Salvar o resultado** — `s` no Modo Foto grava a imagem exibida ao lado
  da original com sufixo `_editado`; `s` no Modo Vídeo grava o quadro atual como
  `captura_<timestamp>.png` na pasta corrente.
  `grau_b/io_utils.py::salvar_imagem`/`salvar_frame`.
- [x] **Resetar** — `r` (foto e webcam) zera filtro, parâmetros, canal, operação
  e stickers, voltando à imagem original.
  `grau_b/modes.py::Sessao.resetar` (`TECLA_RESETAR`).

### Filtros implementados (12)

| `id` | Nome exibido | Categoria | Base OpenCV |
| --- | --- | --- | --- |
| `negativo` | Negativo | cor | `cv2.bitwise_not` |
| `cinza` | Escala de cinza | cor | `cv2.cvtColor` |
| `blur_media` | Desfoque por média | **suavização** | `cv2.blur` (kernel 3–15, padrão 5) |
| `blur_gaussiano` | Desfoque Gaussiano | **suavização** | `cv2.GaussianBlur` (kernel 3–15, padrão 5) |
| `blur_mediana` | Desfoque de mediana | **suavização** | `cv2.medianBlur` (kernel 3–15, padrão 5) |
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
  clique ao Modo Foto; com a webcam ligada a tecla `t` e o clique não têm
  efeito e a barra lateral esconde a seção de stickers.
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
- **Um parâmetro por filtro.** Kernel (desfoques), intensidade (nitidez),
  limiar (binarização) e limiar inferior do Canny são ajustáveis com `+`/`-`;
  o limiar superior do Canny é sempre o dobro do inferior. Sobel, relevo,
  negativo, cinza, equalização e sépia não têm parâmetros.
- **Operações com a webcam recalculam por quadro.** Em resoluções altas ou
  máquinas lentas a taxa de quadros pode cair.
- **Câmera fixa no índice 0.** O Modo Vídeo sempre abre a webcam padrão; não há
  opção de escolher outro índice pela CLI, nem gravação de vídeo em arquivo
  (apenas captura de quadro único com `s`).
- **Validação automatizada é só de sintaxe.** Não há suíte de testes; o
  projeto é validado por `python3 -m compileall` e por inspeção visual/manual
  dos modos (as funções de filtro, operação e sticker também podem ser
  exercitadas sem janela, como mostrado em "Informações Técnicas").
