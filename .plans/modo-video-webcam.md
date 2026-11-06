# Modo Vídeo (webcam) do GrauB

## Objetivo
Adicionar o Modo Vídeo ao app GrauB: captura contínua da webcam, exibida em janela, reutilizando a infraestrutura de janela/teclado já criada no Modo Foto. É o segundo requisito obrigatório do enunciado (Modo Foto + Modo Vídeo) e depende do scaffold da tarefa `scaffold-grau-b`.

## Contexto/ADR
Depende de `scaffold-grau-b` já ter sido mesclado: `grau_b/app.py`, `grau_b/modes.py` e `grau_b/io_utils.py` existem, `pyproject.toml`/`uv.lock` já estão na pasta do projeto.

Do enunciado: o Modo Vídeo captura da webcam continuamente; os stickers (tarefa `stickers`) só são posicionáveis por clique no Modo Foto — no Modo Vídeo os filtros/canais/operações se aplicam ao quadro atual, mas a colocação interativa de stickers não é exigida aqui.

Como o `uv.lock` está em `PG2026-2/src/Trabalho/GrauB/` (subpasta do repositório), o executor não o detecta automaticamente na raiz do worktree — por isso esta tarefa (e as seguintes) usam `install:` explícito.

## Critérios de aceite
- [ ] `uv run python -m grau_b.app --video` (dentro de `PG2026-2/src/Trabalho/GrauB/`) abre a webcam padrão (índice 0), mostra o fluxo de vídeo em uma janela em tempo real, e encerra de forma limpa com `q`/`ESC`, liberando a captura (`cv2.VideoCapture.release()`).
- [ ] Se a webcam não puder ser aberta (`cv2.VideoCapture.isOpened()` falso), o app imprime um erro claro em `stderr` e sai com código diferente de zero, sem travar.
- [ ] `--imagem` e `--video` são mutuamente exclusivos na CLI; passar os dois, ou nenhum dos dois, gera erro de uso claro (`argparse` com grupo mutuamente exclusivo e `required=True`).
- [ ] A tecla `s` no Modo Vídeo salva o quadro atual como arquivo de imagem (ex. `captura_<timestamp>.png` na pasta atual); a tecla `r` não se aplica ao vídeo (sem crash: é ignorada ou documentada como no-op) — sem reset de "imagem original" no Modo Vídeo.
- [ ] O loop de captura não deixa a janela travada (sem vídeo novo) por mais de um frame perdido consecutivo sem mensagem de erro — qualquer falha de leitura de frame encerra o app com mensagem clara, não em loop infinito silencioso.

## Arquivos e módulos
- `PG2026-2/src/Trabalho/GrauB/grau_b/app.py`: adiciona `--video` como alternativa a `--imagem` (grupo mutuamente exclusivo), despacha para `modo_video`.
- `PG2026-2/src/Trabalho/GrauB/grau_b/modes.py`: adiciona `modo_video(indice_camera=0)` com loop de captura, exibição e teclado (`s`, `q`/`ESC`).
- `PG2026-2/src/Trabalho/GrauB/grau_b/io_utils.py`: adiciona `salvar_frame(frame)` (nome de arquivo com timestamp), reaproveitada por `modo_video`.

## Passos
1. Implementar `salvar_frame` em `io_utils.py` (timestamp via `datetime`, grava com `cv2.imwrite`).
2. Implementar `modo_video` em `modes.py`: abre `cv2.VideoCapture(indice_camera)`, valida `isOpened()`, loop `read()` → `imshow()` → `waitKey()`, trata `s` (salva frame), `q`/`ESC` (encerra), falha de leitura de frame (encerra com mensagem), e sempre chama `release()`/`destroyAllWindows()` ao final (inclusive em caminho de erro).
3. Atualizar `app.py`: grupo mutuamente exclusivo `--imagem`/`--video` em `argparse`, despacho para `modo_foto` ou `modo_video`.
4. Se for testar a janela de vídeo em terminal, sempre envolver o comando com `timeout` (ex. `timeout 5 uv run python -m grau_b.app --video; echo exit=$?`) — não há tecla real disponível neste ambiente para `q`/`ESC`, então rodar sem timeout trava o processo indefinidamente. Documentar no corpo da PR se a máquina de validação não tiver webcam, caso em que a verificação de `isOpened() == False` com mensagem de erro é o caminho testável. A verificação manual completa com tecla real pressionada por uma pessoa é tarefa do QA (seção "Verificações manuais" abaixo), não do executor.

## Fora de escopo
- Qualquer filtro, canal, operação matemática ou sticker aplicado ao vídeo (tarefas seguintes aplicam suas funções tanto a foto quanto a vídeo, mas a composição final é feita quando cada uma for implementada).
- Seleção de câmera por índice diferente de 0 via CLI (pode ser adicionado como bônus, não pedido agora).
- Gravação de vídeo em arquivo (só captura de frame único com `s`).

## Como validar
- install: `cd PG2026-2/src/Trabalho/GrauB && uv sync`
- test: `python3 -m compileall -q PG2026-2/src/Trabalho/GrauB`

Verificações manuais (o executor ignora; o QA lê):
- Rodar `uv run python -m grau_b.app --video` numa máquina com webcam e confirmar abertura, captura com `s` e saída limpa com `q`.
- Rodar `uv run python -m grau_b.app --imagem x.png --video` e confirmar que a CLI recusa com mensagem clara.
- Confirmar que nenhuma janela fica órfã (processo não encerra) após `q`/`ESC`.
