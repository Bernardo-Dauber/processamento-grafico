# Operações aritméticas entre duas imagens

## Objetivo
Implementar as operações matemáticas entre duas imagens exigidas pelo enunciado (≥3): soma, subtração, multiplicação e mescla ponderada, acessíveis a partir do Modo Foto ao carregar uma segunda imagem.

## Contexto/ADR
Depende de `scaffold-grau-b` (carregamento/salvamento de imagem) e `pipeline-filtros-canais` (padrão de registro usado para filtros, reaproveitado aqui para as operações).

Do enunciado: pelo menos 3 operações aritméticas entre duas imagens. Operações acordadas com o dono do projeto (4, folga sobre o mínimo de 3):

| id | nome | descrição | base OpenCV |
| --- | --- | --- | --- |
| `soma` | Soma | Soma os valores de pixel das duas imagens, saturando em 255 | `cv2.add` |
| `subtracao` | Subtração | Subtrai os valores de pixel da segunda imagem da primeira, saturando em 0 | `cv2.subtract` |
| `multiplicacao` | Multiplicação | Multiplica os valores de pixel normalizados das duas imagens | `cv2.multiply` com normalização |
| `mescla` | Mescla (blend) | Combina as duas imagens por média ponderada (50/50 por padrão) | `cv2.addWeighted` |

Como as duas imagens de entrada podem ter dimensões diferentes, a segunda imagem é redimensionada para o tamanho da primeira antes de qualquer operação (comportamento simples e documentado, não um requisito do enunciado em si).

## Critérios de aceite
- [ ] `grau_b/operacoes.py` define um registro de operações nos mesmos moldes de `filtros.py` (`id`, `nome`, `descricao`, `funcao(imagem_a, imagem_b) -> imagem`), com as 4 operações da tabela, cada uma redimensionando `imagem_b` para o tamanho de `imagem_a` antes de operar.
- [ ] `listar_operacoes()` e `aplicar_operacao(id, imagem_a, imagem_b)` são expostos por `operacoes.py`.
- [ ] No Modo Foto, uma tecla (ex. `o`) abre um prompt no terminal pedindo o caminho da segunda imagem (`input()`); carregada com sucesso via `io_utils.carregar_imagem`, outra tecla (ex. `m`) cicla pelas 4 operações, imprimindo a descrição da operação atual e exibindo o resultado na janela no lugar da imagem corrente.
- [ ] Se o caminho da segunda imagem for inválido, o app imprime erro no terminal e continua rodando no estado anterior (sem crash, sem fechar a janela).
- [ ] A tecla `r` (reset, já existente) volta ao estado de imagem original de entrada, descartando o resultado da operação com a segunda imagem.
- [ ] `soma`/`subtracao` usam saturação (sem overflow/wraparound); `multiplicacao` normaliza antes de multiplicar para não saturar tudo em branco; `mescla` usa peso 0.5/0.5 por padrão.

## Arquivos e módulos
- `PG2026-2/src/Trabalho/GrauB/grau_b/operacoes.py`: registro das 4 operações, `listar_operacoes()`, `aplicar_operacao(id, imagem_a, imagem_b)`, `proxima_operacao(id_atual)`.
- `PG2026-2/src/Trabalho/GrauB/grau_b/modes.py`: `modo_foto` ganha estado da segunda imagem e da operação atual; tecla `o` (carregar segunda imagem via `input()`), tecla `m` (ciclar operação e aplicar), integração com o `r` existente para resetar.

## Passos
1. Implementar `operacoes.py`: as 4 funções puras (`imagem_a`, `imagem_b` já do mesmo tamanho dentro da função, o redimensionamento acontece antes de chamar a função registrada, dentro de `aplicar_operacao`), registro e `proxima_operacao`.
2. Atualizar `modo_foto` em `modes.py`: tecla `o` pede o caminho via `input()` no terminal (a janela OpenCV continua aberta), carrega e guarda a segunda imagem; tecla `m` cicla operação, aplica e substitui a imagem exibida pelo resultado; tratamento de erro quando a segunda imagem não existe ou está ausente ao apertar `m`.
3. Garantir que o resultado da operação passa pelo mesmo pipeline de canal/filtro já existente (ou documentar explicitamente, se a decisão de design for aplicar a operação como última etapa, substituindo a imagem de trabalho) — a escolha de projeto deve ser coerente e descrita no `README.md`.
4. Testar sem abrir janela interativa: chamar `aplicar_operacao(id, imagem_a, imagem_b)` direto via `uv run python -c "..."` para cada uma das 4 operações com duas imagens de tamanhos diferentes, confirmando o redimensionamento automático e que a saída tem a forma esperada. Não rodar o Modo Foto em terminal não interativo esperando as teclas `o`/`m`/`q` — não há tecla real disponível e o processo trava. A confirmação visual dos 4 resultados na janela é tarefa do QA (seção "Verificações manuais"), não do executor.

## Fora de escopo
- Operações entre vídeo e imagem, ou entre dois streams de vídeo (o enunciado pede entre duas imagens; não se estende ao Modo Vídeo aqui).
- Peso configurável da mescla via CLI/teclado (fixo em 50/50; ajustável como bônus).
- Qualquer filtro ou seleção de canal aplicado à segunda imagem antes da operação (a segunda imagem entra "crua", só redimensionada).

## Como validar
- install: `cd PG2026-2/src/Trabalho/GrauB && uv sync`
- test: `python3 -m compileall -q PG2026-2/src/Trabalho/GrauB`

Verificações manuais (o executor ignora; o QA lê):
- Carregar duas imagens de tamanhos diferentes no Modo Foto, confirmar o redimensionamento automático e os 4 resultados (`soma`, `subtracao`, `multiplicacao`, `mescla`) visualmente plausíveis.
- Confirmar que um caminho de segunda imagem inválido não derruba o app.
- Confirmar que `r` volta à imagem original mesmo depois de uma operação aplicada.
