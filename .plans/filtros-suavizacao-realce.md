# Os 10+ filtros (suavização, realce e demais)

## Objetivo
Popular o registro de filtros criado em `pipeline-filtros-canais` com os 12 filtros concretos que cobrem o requisito do enunciado (≥10 filtros, incluindo ao menos 1 de suavização e 1 de realce de nitidez), cada um com nome e descrição textual.

## Contexto/ADR
Depende de `pipeline-filtros-canais` já mesclado: `grau_b/filtros.py` já expõe `listar_filtros()`/`aplicar_filtro(id, imagem)`/`proximo_filtro`, e `modes.py` já trata as teclas `f`/`c`/`n` e imprime a descrição ao trocar de filtro.

Lista de filtros acordada com o dono do projeto (12 no total, substitui os 2 filtros de exemplo da tarefa anterior):

| id | nome | descrição | categoria |
| --- | --- | --- | --- |
| `negativo` | Negativo | Inverte os valores de cada pixel (255 − valor) | cor |
| `cinza` | Escala de cinza | Converte a imagem para tons de cinza | cor |
| `blur_media` | Desfoque por média | Suaviza a imagem calculando a média dos pixels vizinhos | **suavização** |
| `blur_gaussiano` | Desfoque Gaussiano | Suaviza a imagem com peso maior para pixels mais próximos do centro | **suavização** |
| `blur_mediana` | Desfoque de mediana | Suaviza removendo ruído tipo "sal e pimenta", preservando bordas | **suavização** |
| `realce_nitidez` | Realce de nitidez | Acentua bordas e detalhes (unsharp mask / kernel de nitidez) | **realce** |
| `bordas_sobel` | Bordas (Sobel) | Detecta bordas pelo gradiente de intensidade | bordas |
| `bordas_canny` | Bordas (Canny) | Detecta bordas como linhas binárias finas | bordas |
| `relevo` | Relevo (Emboss) | Simula um efeito de relevo 3D nas transições de intensidade | estilo |
| `binarizacao` | Binarização | Converte a imagem para preto e branco puro por limiar | cor |
| `equalizacao` | Equalização de histograma | Realça o contraste redistribuindo os níveis de intensidade | realce |
| `sepia` | Sépia | Aplica uma transformação de cor em tom sépia (envelhecido) | estilo |

Isso fecha ≥10 filtros (12), com 3 de suavização e 2 de realce — folga sobre o mínimo de 1 cada.

## Critérios de aceite
- [ ] `grau_b/filtros.py` tem exatamente os 12 filtros da tabela acima registrados, cada um com `id`, `nome`, `descricao` (igual ou equivalente em sentido ao texto da tabela) e `funcao(imagem) -> imagem` usando OpenCV/NumPy.
- [ ] `blur_media`, `blur_gaussiano` e `blur_mediana` usam, respectivamente, `cv2.blur`, `cv2.GaussianBlur` e `cv2.medianBlur` (ou equivalentes), com um tamanho de kernel fixo e razoável (ex. 5×5 ou 7×7) documentado na descrição ou num comentário.
- [ ] `realce_nitidez` aplica um kernel de nitidez (ex. `cv2.filter2D` com kernel tipo `[[0,-1,0],[-1,5,-1],[0,-1,0]]`) ou unsharp mask (original + (original − blur) amplificado), produzindo visivelmente mais contraste de borda que a entrada.
- [ ] `cinza`, `binarizacao`, `bordas_sobel`, `bordas_canny` e `equalizacao` preservam o número de canais esperado pelo pipeline de exibição (convertem de volta para BGR de 3 canais antes de devolver, já que `imshow`/pipeline de canal espera BGR) — nenhum filtro deve fazer o app quebrar ao trocar de filtro em sequência via tecla `f`.
- [ ] `proximo_filtro` cicla pelos 12 filtros na ordem da tabela, mais o estado "nenhum" já existente, sem repetir nem pular nenhum.
- [ ] Rodar `uv run python -m grau_b.app --imagem <img>` e apertar `f` 13 vezes (12 filtros + nenhum) percorre o ciclo completo sem erro e sem travar a janela.

## Arquivos e módulos
- `PG2026-2/src/Trabalho/GrauB/grau_b/filtros.py`: substitui os 2 filtros de exemplo pelos 12 filtros reais da tabela, cada `funcao` isolada e testável.

## Passos
1. Implementar cada filtro como uma função pura `imagem_bgr -> imagem_bgr` (sempre devolvendo 3 canais, mesmo quando o processamento interno é em escala de cinza ou binário), em `filtros.py`.
2. Registrar os 12 filtros na ordem da tabela, com `id`/`nome`/`descricao` como definidos.
3. Remover os filtros de exemplo da tarefa anterior (ou substituí-los no lugar, mantendo a mesma interface de registro).
4. Testar sem abrir janela interativa: chamar `aplicar_filtro(id, imagem)` direto para cada um dos 12 ids via `uv run python -c "..."`, comparando a imagem de saída com a de entrada (ex. `not np.array_equal(...)`) para confirmar que cada filtro produz um resultado distinto. Não rodar o Modo Foto (`--imagem`) com o ciclo de teclas `f` em terminal não interativo — não há tecla real disponível e o processo trava esperando `q`/`ESC`. A verificação visual do ciclo completo de `f` na janela é tarefa do QA (seção "Verificações manuais"), não do executor.

## Fora de escopo
- Seleção de canal (já resolvida em `pipeline-filtros-canais`) e sua interação com cada filtro além de já funcionar genericamente.
- Operações aritméticas entre duas imagens (tarefa `operacoes-matematicas`).
- Encadeamento de múltiplos filtros simultâneos (bônus).
- Parâmetros configuráveis de kernel/threshold via CLI ou teclado (os valores ficam fixos no código; ajustável como bônus futuro).

## Como validar
- install: `cd PG2026-2/src/Trabalho/GrauB && uv sync`
- test: `python3 -m compileall -q PG2026-2/src/Trabalho/GrauB`

Verificações manuais (o executor ignora; o QA lê):
- Percorrer o ciclo completo de filtros (`f` 13 vezes) numa imagem de teste e confirmar visualmente que cada um produz um efeito plausível e coerente com a descrição impressa.
- Confirmar que `blur_mediana` reduz ruído "sal e pimenta" numa imagem de teste com ruído artificial, se disponível.
- Confirmar que `realce_nitidez` deixa bordas visivelmente mais acentuadas que a imagem original.
