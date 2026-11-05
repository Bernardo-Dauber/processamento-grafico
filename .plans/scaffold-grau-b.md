# Scaffold do projeto GrauB (Python + uv + OpenCV)

## Objetivo
Criar a estrutura inicial do projeto do Trabalho GrauB em `PG2026-2/src/Trabalho/GrauB/`: um app Python gerenciado por `uv`, com OpenCV instalado e o Modo Foto mínimo (carregar imagem de um caminho, exibir numa janela, salvar e resetar). É a base sobre a qual as tarefas seguintes (vídeo, filtros, canais, operações matemáticas, stickers) são construídas.

## Contexto/ADR
O enunciado oficial é `EnunciadoGrauB.pdf` (anexado na conversa que originou este plano): trabalho individual de Processamento Gráfico (Unisinos), prazo 25/11/2026, pedindo um app de processamento de imagem/vídeo com ≥10 filtros, seleção de canal, ≥3 operações aritméticas entre duas imagens, ≥5 stickers com canal alpha posicionáveis por clique, Modo Foto e Modo Vídeo, salvar/resetar.

Decisões tomadas com o dono do projeto (sessão de grilling de 2026-11-04):
- Stack: Python + `opencv-python`, projeto standalone (não integra ao CMake do Grau A, que exige `cmake`/`libopencv-dev` via `sudo`, indisponíveis na máquina de desenvolvimento).
- Interação mínima: HighGUI do próprio OpenCV (janela + teclado + mouse), sem framework de GUI externo (isso é bônus opcional, fora de escopo aqui).
- Gerenciamento de dependências: `uv` (`pyproject.toml` + `uv.lock`), não `pip`/`requirements.txt`.
- `PG2026-2/src/Trabalho/GrauB/` já existe no repositório e está vazia — é a raiz do novo projeto Python.
- Validação automática do projeto: só checagem de sintaxe (`compileall`), sem lint — não há test/lint reais pedidos pelo curso.

Como o `pyproject.toml`/`uv.lock` ficam numa subpasta do repositório (não na raiz do worktree), as tarefas seguintes que dependerem de instalar pacotes precisarão de um `install:` explícito no plano (o executor só detecta lockfile automaticamente na raiz). Esta tarefa cria esse lockfile pela primeira vez, então não há instalação automática a fazer antes de o agente rodar.

## Critérios de aceite
- [ ] `PG2026-2/src/Trabalho/GrauB/pyproject.toml` existe, declara o projeto (nome, versão, `requires-python`) e tem `opencv-python` como dependência.
- [ ] `PG2026-2/src/Trabalho/GrauB/uv.lock` existe e foi gerado por `uv lock` (ou `uv sync`) a partir desse `pyproject.toml`.
- [ ] A estrutura de pastas abaixo existe, com todos os arquivos `.py` sintaticamente válidos:
  - `grau_b/__init__.py`
  - `grau_b/app.py`
  - `grau_b/modes.py`
  - `grau_b/io_utils.py`
- [ ] `uv run python -m grau_b.app --help` (executado dentro de `PG2026-2/src/Trabalho/GrauB/`) imprime as opções de linha de comando sem erro e sem abrir nenhuma janela.
- [ ] Rodando `uv run python -m grau_b.app --imagem <caminho.png>` dentro de `PG2026-2/src/Trabalho/GrauB/` com uma imagem válida: abre uma janela OpenCV mostrando a imagem; a tecla `s` salva a imagem atual (com um sufixo, ex. `_editado`, no mesmo diretório da imagem de origem); a tecla `r` restaura a imagem original carregada; a tecla `q` ou `ESC` encerra o app sem erro.
- [ ] Sem argumento `--imagem`, ou com caminho inexistente, o app imprime uma mensagem de erro clara no `stderr` e sai com código diferente de zero, sem abrir janela.
- [ ] O `.gitignore` da raiz do repositório ganhou entradas para artefatos Python (`.venv/`, `__pycache__/`, `*.pyc`, `.python-version`), sem remover nenhuma entrada existente do CMake/C++.
- [ ] `PG2026-2/src/Trabalho/GrauB/README.md` existe com pelo menos: o que o projeto é, como instalar (`uv sync`) e como rodar o Modo Foto.

## Arquivos e módulos
- `PG2026-2/src/Trabalho/GrauB/pyproject.toml`: metadados do projeto e dependência `opencv-python`.
- `PG2026-2/src/Trabalho/GrauB/uv.lock`: lockfile gerado pelo `uv`.
- `PG2026-2/src/Trabalho/GrauB/README.md`: instruções mínimas de instalação/uso (expandido na tarefa final `readme-entrega`).
- `PG2026-2/src/Trabalho/GrauB/grau_b/__init__.py`: vazio ou com versão do pacote.
- `PG2026-2/src/Trabalho/GrauB/grau_b/app.py`: ponto de entrada (`python -m grau_b.app`), parsing de argumentos (`argparse`), despacho para Modo Foto (Modo Vídeo é adicionado na tarefa `modo-video-webcam`).
- `PG2026-2/src/Trabalho/GrauB/grau_b/modes.py`: função/classe `modo_foto(caminho_imagem)` com o loop de janela, teclado (`s`, `r`, `q`/`ESC`).
- `PG2026-2/src/Trabalho/GrauB/grau_b/io_utils.py`: `carregar_imagem(caminho)`, `salvar_imagem(imagem, caminho_origem)`, `resetar(imagem_original)`.
- `.gitignore` (raiz do repositório): adiciona entradas Python ao final do arquivo, sem tocar nas seções de CMake/C++.

## Passos
1. Criar `PG2026-2/src/Trabalho/GrauB/pyproject.toml` com `uv init --no-workspace` (ou escrita manual equivalente) e adicionar a dependência com `uv add opencv-python`; isso já gera `uv.lock`.
2. Criar o pacote `grau_b/` com `__init__.py` vazio.
3. Implementar `io_utils.py`: `carregar_imagem` (lê com `cv2.imread`, lança erro claro se `None`), `salvar_imagem` (grava com `cv2.imwrite` usando um sufixo no nome do arquivo de origem), `resetar` (devolve cópia da imagem original).
4. Implementar `modes.py`: `modo_foto(caminho)` que carrega a imagem, abre uma janela (`cv2.imshow`), entra em loop de `cv2.waitKey`, trata `s`/`r`/`q`/`ESC`, e fecha a janela ao sair (`cv2.destroyAllWindows`).
5. Implementar `app.py`: `argparse` com `--imagem` (obrigatório por enquanto, Modo Vídeo entra na próxima tarefa), validação do caminho antes de chamar `modo_foto`, mensagens de erro em `stderr` e código de saída não-zero quando inválido.
6. Atualizar o `.gitignore` da raiz do repositório com as entradas Python.
7. Escrever o `README.md` mínimo do projeto GrauB.
8. Rodar manualmente `uv run python -m grau_b.app --imagem <alguma imagem de teste>` e confirmar abertura de janela, salvar e resetar antes de finalizar.

## Fora de escopo
- Modo Vídeo / webcam (tarefa `modo-video-webcam`).
- Qualquer filtro, seleção de canal ou operação matemática (tarefas seguintes).
- Stickers (tarefa `stickers`).
- README final no formato do `Template_README_Projeto.md` do curso (tarefa `readme-entrega`).
- GUI externa, encadeamento de filtros, stickers animados, detecção facial (bônus, não pedidos agora).

## Como validar
- test: `python3 -m compileall -q PG2026-2/src/Trabalho/GrauB`

Verificações manuais (o executor ignora; o QA lê):
- Confirmar que `PG2026-2/src/Trabalho/GrauB/uv.lock` foi commitado junto com o `pyproject.toml`.
- Rodar `uv run python -m grau_b.app --imagem <caminho>` dentro da pasta do projeto e confirmar visualmente a janela, o salvar (`s`) e o reset (`r`).
- Confirmar que nenhuma dependência de C++/CMake foi adicionada ou alterada.
