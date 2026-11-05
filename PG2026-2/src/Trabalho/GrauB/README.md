# Grau B — Processamento Gráfico (Unisinos)

Aplicação de processamento de imagem e vídeo em Python + [OpenCV](https://opencv.org/),
com interface na própria janela HighGUI do OpenCV (teclado e, mais adiante, mouse).

Esta é a base do trabalho: por enquanto o app implementa apenas o **Modo Foto**
mínimo — carrega uma imagem de um caminho, exibe em uma janela e permite salvar
a imagem atual ou descartar as edições e voltar à original.

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

| Tecla   | Ação                                                        |
| ------- | ----------------------------------------------------------- |
| `s`     | Salva a imagem atual ao lado da original, com o sufixo `_editado` (ex.: `foto_editado.png`) |
| `r`     | Restaura a imagem original carregada, descartando edições   |
| `q`/`ESC` | Fecha a janela e encerra o app                            |

Para ver todas as opções de linha de comando:

```bash
uv run python -m grau_b.app --help
```

### Erros

Sem `--imagem`, ou com um caminho inexistente, o app escreve uma mensagem de
erro no `stderr` e termina com código de saída diferente de zero, sem abrir
janela.

## Estrutura

```
GrauB/
├── pyproject.toml     # metadados e dependências (gerenciado pelo uv)
├── uv.lock            # lockfile das dependências
├── README.md
└── grau_b/
    ├── __init__.py    # versão do pacote
    ├── app.py         # ponto de entrada e parsing de argumentos
    ├── modes.py       # Modos de operação (Modo Foto)
    └── io_utils.py    # carregar / salvar / resetar imagem
```

## Verificação de sintaxe

```bash
python3 -m compileall -q PG2026-2/src/Trabalho/GrauB
```

Esse projeto é independente do código C++/CMake do Grau A, que continua em
`PG2026-2/src/Trabalho/GrauA/`.
