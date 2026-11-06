# Grau B — Processamento Gráfico (Unisinos)

Aplicação de processamento de imagem e vídeo em Python + [OpenCV](https://opencv.org/),
com interface na própria janela HighGUI do OpenCV (teclado e, mais adiante, mouse).

O app tem dois modos:

- **Modo Foto**: carrega uma imagem, aplica seleção de canal, filtros e
  operações aritméticas entre duas imagens, e permite salvar o resultado.
- **Modo Vídeo**: exibe a webcam ao vivo, com seleção de canal e filtros, e
  permite salvar o quadro exibido.

## Requisitos

- Python 3.12+
- [uv](https://docs.astral.sh/uv/) (gerencia o ambiente virtual e as dependências)
- Uma sessão gráfica (X11/Wayland) para a janela do OpenCV

## Instalação

Dentro desta pasta (`PG2026-2/src/Trabalho/GrauB/`):

```bash
uv sync
```

O comando cria o `.venv`, instala as dependências do `pyproject.toml`/`uv.lock`
(`opencv-python` e `numpy`) e instala o pacote `grau_b` em modo editável.

## Como rodar

### Modo Foto

```bash
uv run python -m grau_b.app --imagem caminho/para/imagem.png
```

Abre uma janela com a imagem. Teclas disponíveis:

| Tecla     | Ação                                                        |
| --------- | ----------------------------------------------------------- |
| `s`       | Salva a imagem atual ao lado da original, com o sufixo `_editado` (ex.: `foto_editado.png`) |
| `r`       | Restaura a imagem original carregada, descartando filtro, canal e operação |
| `f`       | Avança para o próximo filtro (a descrição aparece no terminal) |
| `c`       | Alterna o canal: `todos → R → G → B → todos`                |
| `n`       | Volta ao estado "sem filtro"                                |
| `o`       | Pede no terminal o caminho de uma segunda imagem            |
| `m`       | Avança para a próxima operação aritmética entre a imagem original e a segunda imagem |
| `q`/`ESC` | Fecha a janela e encerra o app                              |

### Modo Vídeo

```bash
uv run python -m grau_b.app --video
```

Abre a webcam padrão. Valem as teclas `s`, `f`, `c`, `n` e `q`/`ESC`; a tecla
`r` não se aplica ao vídeo.

Para ver todas as opções de linha de comando:

```bash
uv run python -m grau_b.app --help
```

### Erros

Sem `--imagem`, ou com um caminho inexistente, o app escreve uma mensagem de
erro no `stderr` e termina com código de saída diferente de zero, sem abrir
janela.

## Operações matemáticas entre duas imagens

O registro fica em `grau_b/operacoes.py`, no mesmo formato do registro de
filtros: cada operação tem `id`, `nome`, `descricao` e
`funcao(imagem_a, imagem_b) -> imagem`. Estão disponíveis quatro operações:

| `id`            | Nome          | O que faz                                                        | Base OpenCV |
| --------------- | ------------- | ---------------------------------------------------------------- | ----------- |
| `soma`          | Soma          | Soma os pixels das duas imagens, saturando em 255                | `cv2.add` |
| `subtracao`     | Subtração     | Subtrai os pixels da segunda imagem dos da primeira, saturando em 0 | `cv2.subtract` |
| `multiplicacao` | Multiplicação | Normaliza as duas imagens para `[0, 1]`, multiplica e reescala para `[0, 255]` | `cv2.multiply` |
| `mescla`        | Mescla        | Média ponderada 50/50 entre as duas imagens                      | `cv2.addWeighted` |

Decisões de projeto:

- **Redimensionamento automático**: se as imagens tiverem tamanhos diferentes,
  a segunda é redimensionada para o tamanho da primeira antes da operação.
  Não é necessário preparar os arquivos.
- **Base de cálculo**: as operações são sempre recalculadas a partir da imagem
  original e da segunda imagem carregada; apertar `m` repetidamente não
  acumula resultados anteriores.
- **Ordem do pipeline**: o resultado da operação substitui a imagem de trabalho
  e passa pelo pipeline de exibição já existente (seleção de canal e, depois,
  filtro). Ou seja, filtros e canais valem sobre o resultado da operação.
- **Erros não derrubam o app**: um caminho inválido em `o` imprime o erro no
  terminal e mantém a segunda imagem anterior (ou o estado sem segunda imagem).
  Apertar `m` sem segunda imagem apenas avisa no terminal.
- **Reset**: `r` volta à imagem original de entrada e zera filtro, canal e
  operação. A segunda imagem continua carregada e pode ser reutilizada com `m`.

## Uso direto dos módulos (sem abrir janela)

A partir de `PG2026-2/src/Trabalho/GrauB`:

```bash
uv run python -c "import numpy as np; from grau_b.operacoes import aplicar_operacao, listar_operacoes, proxima_operacao; a = np.full((4, 4, 3), 100, dtype=np.uint8); b = np.full((2, 2, 3), 50, dtype=np.uint8); print([o.id for o in listar_operacoes()]); print(aplicar_operacao('soma', a, b).shape); print(aplicar_operacao('mescla', a, b)[0, 0]); print([proxima_operacao('soma'), proxima_operacao('mescla')])"
```

O mesmo vale para o pipeline de canal e filtros:

```bash
uv run python -c "import numpy as np; from grau_b.canais import aplicar_selecao_canal, proximo_canal; from grau_b.filtros import aplicar_filtro, listar_filtros, proximo_filtro; imagem = np.full((2, 2, 3), (10, 20, 30), dtype=np.uint8); print([f.id for f in listar_filtros()]); print(aplicar_filtro('negativo', aplicar_selecao_canal(imagem, 'R'))[0, 0]); print([proximo_canal('todos'), proximo_filtro('nenhum')])"
```

## Estrutura

```
GrauB/
├── pyproject.toml     # metadados e dependências (gerenciado pelo uv)
├── uv.lock            # lockfile das dependências
├── README.md
└── grau_b/
    ├── __init__.py    # versão do pacote
    ├── app.py         # ponto de entrada e parsing de argumentos
    ├── modes.py       # Modos Foto e Vídeo (teclado e pipeline de exibição)
    ├── io_utils.py    # carregar / salvar / resetar imagem
    ├── canais.py      # seleção de canal R/G/B
    ├── filtros.py     # registro de filtros de imagem
    └── operacoes.py   # registro de operações entre duas imagens
```

## Verificação de sintaxe

```bash
python3 -m compileall -q PG2026-2/src/Trabalho/GrauB
```

Esse projeto é independente do código C++/CMake do Grau A, que continua em
`PG2026-2/src/Trabalho/GrauA/`.
