# Pipeline de filtros e seleção de canal

## Objetivo
Criar a infraestrutura comum que as próximas tarefas (`filtros-suavizacao-realce`, `operacoes-matematicas`) vão preencher: um registro de filtros com nome/descrição/função, a seleção de canal (imagem completa ou um único canal R/G/B) e a integração dos dois ao teclado do Modo Foto e do Modo Vídeo, exibindo a descrição do filtro antes de aplicá-lo (requisito do enunciado).

## Contexto/ADR
Depende de `scaffold-grau-b` e `modo-video-webcam` já mesclados.

Do enunciado: o app precisa permitir escolher processar a imagem completa ou apenas um dos canais R, G ou B; e, antes de aplicar um filtro, mostrar uma descrição textual do que ele faz. Os filtros concretos (os ≥10, incluindo suavização e realce) são implementados na tarefa seguinte (`filtros-suavizacao-realce`); esta tarefa só constrói o mecanismo de registro, seleção e exibição — sem isso ainda não há filtro real além de um ou dois de exemplo para validar o pipeline.

Decisão do dono do projeto: a interação é via teclado/HighGUI (sem GUI externa); a descrição do filtro aparece como texto impresso no terminal e/ou sobreposto na janela (`cv2.putText`) antes de a transformação ser aplicada ao quadro exibido.

## Critérios de aceite
- [ ] `grau_b/filtros.py` define um registro (lista ou dict ordenado) de filtros, cada um com `id`, `nome`, `descricao` (texto curto) e `funcao(imagem) -> imagem`; o módulo expõe `listar_filtros()` e `aplicar_filtro(id, imagem)`.
- [ ] `grau_b/canais.py` define `aplicar_selecao_canal(imagem, canal)` com `canal` em `{"todos", "R", "G", "B"}`: para um canal específico, devolve uma imagem com os outros dois canais zerados (mesma forma/dtype da entrada); para `"todos"`, devolve a imagem inalterada.
- [ ] No Modo Foto e no Modo Vídeo, uma tecla (ex. `f`) avança para o próximo filtro do registro (ciclando), e outra tecla (ex. `c`) alterna o canal selecionado entre `todos → R → G → B → todos`; o filtro e o canal atuais se aplicam continuamente ao quadro exibido (ordem: seleção de canal primeiro, depois filtro).
- [ ] Antes de um filtro ser aplicado por trocar de filtro (tecla `f`), a descrição desse filtro é impressa no terminal (`stdout`); no mínimo dois filtros de exemplo existem no registro para este critério ser verificável sem depender da tarefa seguinte (podem ser substituídos/ampliados por `filtros-suavizacao-realce`, desde que a interface não mude).
- [ ] A tecla `n` (nenhum filtro) volta ao estado "sem filtro" (equivalente a um filtro identidade), sem quebrar o ciclo da tecla `f`.
- [ ] `grau_b/filtros.py` e `grau_b/canais.py` têm testes de uso direto documentados no README (ou docstring) que podem ser executados manualmente com `uv run python -c "..."` sem precisar de janela.

## Arquivos e módulos
- `PG2026-2/src/Trabalho/GrauB/grau_b/filtros.py`: registro de filtros, `listar_filtros()`, `aplicar_filtro(id, imagem)`, `proximo_filtro(id_atual)`.
- `PG2026-2/src/Trabalho/GrauB/grau_b/canais.py`: `aplicar_selecao_canal(imagem, canal)`, `proximo_canal(canal_atual)`.
- `PG2026-2/src/Trabalho/GrauB/grau_b/modes.py`: `modo_foto` e `modo_video` passam a manter estado de `filtro_atual`/`canal_atual`, tratam as teclas `f`/`c`/`n`, aplicam canal depois filtro a cada quadro antes do `imshow`, e imprimem a descrição ao trocar de filtro.

## Passos
1. Implementar `canais.py`: `aplicar_selecao_canal` (usa indexação de canal do array NumPy/BGR do OpenCV, zera os demais) e `proximo_canal` (ciclo fixo).
2. Implementar `filtros.py` com um registro inicial de 2 filtros de exemplo (ex. identidade e negativo simples), cada um com `id`/`nome`/`descricao`/`funcao`; `aplicar_filtro` busca pelo `id` e aplica; `proximo_filtro` cicla pela lista, incluindo o estado "nenhum" (`None`/`"nenhum"`).
3. Atualizar `modo_foto` e `modo_video` em `modes.py`: estado local de filtro/canal, tratamento das teclas `f` (próximo filtro + imprime descrição), `c` (próximo canal), `n` (nenhum filtro); pipeline por quadro: `imagem_canal = aplicar_selecao_canal(imagem, canal_atual)`; `imagem_final = aplicar_filtro(filtro_atual, imagem_canal)` se houver filtro, senão `imagem_canal`.
4. Validar sem abrir janela interativa: chamar `aplicar_filtro(id, aplicar_selecao_canal(imagem, canal))` direto via `uv run python -c "..."` combinando cada filtro com cada canal, confirmando ausência de erro. Não rodar `modo_foto`/`modo_video` em terminal não interativo trocando `f`/`c` — não há tecla real disponível e o processo trava esperando `q`/`ESC`. A confirmação visual combinada é tarefa do QA (seção "Verificações manuais"), não do executor.

## Fora de escopo
- A lista final de ≥10 filtros com suavização e realce (tarefa `filtros-suavizacao-realce` populará o registro).
- Operações aritméticas entre duas imagens (tarefa `operacoes-matematicas`).
- Encadeamento de múltiplos filtros simultâneos (bônus, não pedido agora — o pipeline aplica um filtro por vez).
- Overlay gráfico elaborado da descrição na janela (texto no terminal já satisfaz o requisito; `cv2.putText` pode ser adicionado como melhoria, não obrigatório aqui).

## Como validar
- install: `cd PG2026-2/src/Trabalho/GrauB && uv sync`
- test: `python3 -m compileall -q PG2026-2/src/Trabalho/GrauB`

Verificações manuais (o executor ignora; o QA lê):
- Rodar o Modo Foto, apertar `c` repetidas vezes e confirmar visualmente que só um canal aparece por vez (demais pretos) e volta a "todos".
- Apertar `f` repetidas vezes e confirmar a descrição impressa no terminal a cada troca.
- Confirmar que `n` volta à imagem sem filtro sem quebrar o ciclo seguinte de `f`.
