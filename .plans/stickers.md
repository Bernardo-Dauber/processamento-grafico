# Stickers com canal alpha posicionáveis por clique

## Objetivo
Implementar a geração de ao menos 5 stickers com canal alpha (transparência) e permitir posicioná-los na imagem do Modo Foto por clique do mouse, conforme exigido pelo enunciado.

## Contexto/ADR
Depende de `scaffold-grau-b` (Modo Foto, janela, teclado) e `pipeline-filtros-canais` (padrão de estado por quadro já estabelecido em `modo_foto`). Não depende de `operacoes-matematicas` nem `filtros-suavizacao-realce`, mas é implementado depois deles na sequência para não competir por atenção/revisão em paralelo no mesmo repositório.

Do enunciado: ≥5 stickers com canal alpha, posicionáveis via clique do mouse, exclusivamente no Modo Foto (não no Modo Vídeo). Decisão do dono do projeto: os stickers são gerados por script próprio (desenhados programaticamente com OpenCV/NumPy), não baixados de bancos de imagem — evita depender de arquivos externos e de questões de licença/atribuição no README final.

5 stickers acordados (formas simples, geometria 2D, cada uma com canal alpha preenchendo só a forma e transparente no resto):

| id | nome | descrição visual |
| --- | --- | --- |
| `estrela` | Estrela | Estrela de 5 pontas preenchida, amarela |
| `coracao` | Coração | Coração preenchido, vermelho |
| `circulo` | Círculo | Círculo preenchido com borda, azul |
| `triangulo` | Triângulo | Triângulo preenchido, verde |
| `selo_ok` | Selo OK | Círculo com um "check" (✓) desenhado dentro, branco sobre verde |

## Critérios de aceite
- [ ] `grau_b/stickers.py` gera os 5 stickers acima como imagens RGBA (NumPy, `dtype=uint8`, 4 canais) em memória, por função (ex. `gerar_estrela()`, `gerar_coracao()`, ...), cada uma com tamanho fixo razoável (ex. 80×80 px) e o canal alpha em 0 fora da forma e 255 dentro.
- [ ] `listar_stickers()` expõe os 5 stickers com `id`/`nome`/imagem RGBA; `proximo_sticker(id_atual)` cicla entre eles.
- [ ] `aplicar_sticker(imagem_base_bgr, sticker_rgba, posicao_xy) -> imagem_bgr`: faz o blending alpha corretamente (recorte nas bordas quando a posição deixa o sticker parcialmente fora da imagem, sem erro de índice).
- [ ] No Modo Foto, uma tecla (ex. `t`) cicla pelo sticker atual (imprime o nome no terminal); um clique do mouse esquerdo na janela (callback `cv2.setMouseCallback`) fixa o sticker atual na posição clicada, sobre a imagem de trabalho atual (depois de canal/filtro/operação, se algum estiver ativo).
- [ ] Múltiplos cliques adicionam múltiplos stickers acumulados (não substituem o anterior); a tecla `r` (reset) remove todos os stickers aplicados junto com o resto do estado, voltando à imagem original.
- [ ] Clicar próximo da borda da imagem (sticker parcialmente fora) não lança exceção nem derruba o app.

## Arquivos e módulos
- `PG2026-2/src/Trabalho/GrauB/grau_b/stickers.py`: as 5 funções geradoras, `listar_stickers()`, `proximo_sticker`, `aplicar_sticker`.
- `PG2026-2/src/Trabalho/GrauB/grau_b/modes.py`: `modo_foto` ganha callback de mouse (`cv2.setMouseCallback`), estado do sticker atual e lista de stickers já posicionados (posição + id), tecla `t` para ciclar, integração com `r` para limpar.

## Passos
1. Implementar as 5 funções geradoras de sticker em `stickers.py` usando `cv2.ellipse`/`cv2.fillPoly`/`cv2.circle`/`cv2.putText` sobre um array RGBA inicializado transparente, desenhando a forma e o alpha correspondente.
2. Implementar `aplicar_sticker` com blending alpha padrão (`resultado = base*(1-alpha) + sticker_rgb*alpha`), cuidando do recorte quando a posição está perto da borda.
3. Atualizar `modo_foto` em `modes.py`: callback de mouse que, no evento `cv2.EVENT_LBUTTONDOWN`, adiciona `(sticker_atual_id, x, y)` a uma lista de stickers posicionados; a cada quadro, a imagem de trabalho recebe todos os stickers da lista aplicados em ordem antes do `imshow`; tecla `t` cicla o sticker atual e imprime o nome; `r` limpa a lista de stickers junto com o reset existente.
4. Testar sem abrir janela interativa: chamar `aplicar_sticker(imagem, sticker_id, x, y)` direto via `uv run python -c "..."` para cada um dos 5 stickers em várias posições, inclusive perto das 4 bordas e cantos, confirmando ausência de erro (shape/dtype preservados). Não simular cliques de mouse em terminal não interativo — não há como injetar eventos reais de mouse neste ambiente. A confirmação visual do blending e do clique real é tarefa do QA (seção "Verificações manuais"), não do executor.

## Fora de escopo
- Stickers animados (bônus, não pedido agora).
- Arrastar ou remover um sticker individual já posicionado (só adicionar e resetar tudo).
- Redimensionar ou rotacionar o sticker no momento do clique (tamanho fixo por sticker).
- Posicionamento de sticker no Modo Vídeo (o enunciado restringe isso ao Modo Foto).

## Como validar
- install: `cd PG2026-2/src/Trabalho/GrauB && uv sync`
- test: `python3 -m compileall -q PG2026-2/src/Trabalho/GrauB`

Verificações manuais (o executor ignora; o QA lê):
- Rodar o Modo Foto, ciclar pelos 5 stickers com `t` e clicar em diferentes posições, confirmando visualmente a forma e a transparência de cada um.
- Clicar perto das 4 bordas/cantos da imagem e confirmar que não há erro nem recorte incorreto grosseiro.
- Confirmar que `r` remove todos os stickers posicionados.
