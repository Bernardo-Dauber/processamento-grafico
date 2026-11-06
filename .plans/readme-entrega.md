# README final de entrega do GrauB

## Objetivo
Substituir o `README.md` mínimo do GrauB (criado em `scaffold-grau-b`) por um README completo seguindo o template oficial do curso, documentando todos os requisitos implementados nas tarefas anteriores, pronto para a entrega de 25/11/2026.

## Contexto/ADR
Depende de todas as tarefas anteriores mescladas: `scaffold-grau-b`, `modo-video-webcam`, `pipeline-filtros-canais`, `filtros-suavizacao-realce`, `operacoes-matematicas` e `stickers`. É a última tarefa da sequência porque só pode documentar com precisão o que já está implementado e testado.

O template oficial é `PG2026-2/misc/Template_README_Projeto.md`, com as seções: Nome do Projeto, Equipe, Descrição, Estrutura, Informações Técnicas, Checklist de Requisitos, Screenshots, Referências, Comentários Finais.

O enunciado completo (`EnunciadoGrauB.pdf`) é a fonte de verdade do checklist de requisitos — cada item exigido precisa aparecer marcado (feito/não feito) com uma referência de onde está implementado no código.

## Critérios de aceite
- [ ] `PG2026-2/src/Trabalho/GrauB/README.md` segue a estrutura do `Template_README_Projeto.md` (mesmas seções, na mesma ordem), preenchido com o conteúdo real do projeto GrauB (não placeholders do template).
- [ ] A seção "Descrição" explica o propósito do projeto (trabalho acadêmico de Processamento Gráfico) e a stack escolhida (Python + OpenCV + `uv`), com a justificativa de não reusar o CMake do Grau A.
- [ ] A seção "Estrutura" lista os módulos reais em `grau_b/` (`app.py`, `modes.py`, `io_utils.py`, `canais.py`, `filtros.py`, `operacoes.py`, `stickers.py`) com uma frase sobre a responsabilidade de cada um.
- [ ] A seção "Informações Técnicas" documenta como instalar (`uv sync`) e como rodar (Modo Foto com `--imagem`, Modo Vídeo com `--video`) e os controles de teclado/mouse (`f`, `c`, `n`, `o`, `m`, `t`, `s`, `r`, `q`/`ESC`, clique para sticker).
- [ ] A seção "Checklist de Requisitos" cobre, com referência ao arquivo/função responsável, cada item do enunciado: Modo Foto, Modo Vídeo, ≥10 filtros (lista os 12 por nome), ≥1 suavização, ≥1 realce, seleção de canal (todos/R/G/B), descrição do filtro antes de aplicar, ≥3 operações aritméticas (lista as 4), ≥5 stickers com canal alpha (lista os 5), posicionamento por clique, salvar, resetar — cada item marcado como feito, com os bônus não implementados listados como "não implementado" (não omitidos).
- [ ] A seção "Referências" cita o OpenCV (`opencv-python`) e o `uv`, sem inventar fontes externas de stickers (coerente com a decisão de stickers gerados por script).
- [ ] A seção "Screenshots" existe com uma instrução clara de como o revisor humano pode gerar as próprias capturas (já que o agente que escreve este README não roda a interface gráfica) — não trava o critério de aceite em uma imagem real embutida.
- [ ] Nenhuma informação no README contradiz o comportamento real do código (o agente deve ler os módulos listados antes de escrever cada seção, não assumir comportamento do enunciado que não foi implementado).

## Arquivos e módulos
- `PG2026-2/src/Trabalho/GrauB/README.md`: reescrito por completo a partir do template e do estado real do código.
- Leitura (sem alteração): `PG2026-2/misc/Template_README_Projeto.md`, `PG2026-2/src/Trabalho/GrauB/grau_b/*.py`.

## Passos
1. Ler `PG2026-2/misc/Template_README_Projeto.md` e cada módulo em `grau_b/` para levantar o comportamento real implementado (filtros, operações, stickers, teclas, modos).
2. Escrever o README seguindo a ordem de seções do template, preenchendo cada uma com o conteúdo real do projeto.
3. Montar a seção "Checklist de Requisitos" cruzando item a item com o enunciado (filtros, canais, operações, stickers, modos, salvar/resetar) e marcando bônus não implementados como pendentes.
4. Revisar que nenhum caminho de arquivo citado no README é inventado (confirmar que cada módulo citado existe de fato no repositório).

## Fora de escopo
- Implementar qualquer funcionalidade nova ou bônus (GUI externa, encadeamento de filtros, stickers animados, detecção facial) — o README documenta o que existe, não adiciona.
- Gerar screenshots reais (o agente não tem como rodar a interface gráfica interativa); a seção de screenshots documenta como gerar, não embute imagens.
- Alterar qualquer arquivo de código em `grau_b/`.

## Como validar
- test: `python3 -m compileall -q PG2026-2/src/Trabalho/GrauB`

Verificações manuais (o executor ignora; o QA lê):
- Comparar a seção "Checklist de Requisitos" do README linha a linha com `EnunciadoGrauB.pdf` e confirmar que nenhum item obrigatório foi omitido ou descrito incorretamente.
- Confirmar que todo caminho de arquivo citado no README existe no repositório.
- Confirmar que as instruções de instalação/execução (`uv sync`, `--imagem`, `--video`) realmente funcionam seguindo exatamente o que está escrito.
