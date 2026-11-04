# Lista de Exercícios 3 — Criando Triângulos a partir do Clique do Mouse

*Processamento Gráfico: Fundamentos* (Unisinos) — buffers de geometria, envio de
dados para o *shader* e interação com o mouse.

## Como compilar e executar

A partir da pasta `PG2026-2` (onde fica o `CMakeLists.txt`; a raiz do
repositório git fica um nível acima):

```bash
cmake -S . -B build
cmake --build build -j
```

O exercício vira o executável `Lista3` dentro de `build/`:

```bash
./build/Lista3
```

> Se o executável não aparecer logo após criar a pasta (erro de
> `undefined reference to main`), re-execute `cmake -S . -B build`: o
> `file(GLOB CONFIGURE_DEPENDS)` do `CMakeLists.txt` precisa reavaliar a lista
> de fontes quando uma pasta nova é adicionada.

## Controles

| Entrada | Ação |
|---|---|
| Clique com o botão **esquerdo** | Cria **1 vértice** na posição do cursor |
| Tecla **C** | Limpa todos os vértices/triângulos |
| Tecla **ESC** | Fecha a janela |

A barra de título mostra a contagem de vértices, de triângulos e o FPS.

## Arquivos

| Arquivo | Descrição |
|---|---|
| `TriangulosMouse.cpp` | Programa completo (um único `.cpp` autocontido) |
| `README.md` | Este arquivo |

---

## O enunciado

1. Ao clicar na tela, cria-se **apenas 1 vértice**.
2. A cada **3 vértices** criados, forma-se um **triângulo**.
3. Cada novo triângulo deve usar uma **cor nova**.

## Como o programa resolve cada requisito

### 1 clique = 1 vértice

O clique é tratado pelo *callback* `mouse_button_callback` (registrado em
`glfwSetMouseButtonCallback`, conforme o **GLFW Input Guide**). A cada
`GLFW_PRESS` do botão esquerdo lê-se a posição do cursor com `glfwGetCursorPos`
e acrescenta-se um único vértice ao vetor.

### Mapeamento 1 unidade = 1 pixel

Segue a dica do enunciado: projeção **ortográfica** com a **janela do mundo do
mesmo tamanho da tela**. A matriz é

```cpp
glm::ortho(0.0f, 800.0f, 600.0f, 0.0f);   // left, right, bottom, top
```

`glm::ortho` mapeia `bottom → NDC -1` e `top → NDC +1`. Passando `bottom=600` e
`top=0`, o **Y do mundo cresce para baixo**, exatamente como no sistema de
coordenadas da tela. Com isso, a posição do cursor (`glfwGetCursorPos`, origem
no canto superior esquerdo, Y para baixo) é usada **diretamente** como
coordenada de vértice — 1 unidade do mundo corresponde a 1 pixel e não há
conversão alguma.

```
mundo (janela 800x600)   projeção      NDC          viewport (pixels)
   x:0..800, y:0..600   ----------->  -1..1   ---->   800 x 600
        (Y p/ baixo)     ortho         (Y p/ cima)
```

### 3 vértices = 1 triângulo, com cor nova

O vetor de vértices guarda o **índice do triângulo em construção** como
`triIndex = nVerts / 3`. Os 3 primeiros cliques pertencem ao triângulo 0, os 3
seguintes ao triângulo 1, e assim por diante — logo todos os vértices de um
mesmo triângulo recebem a **mesma cor**, e cada triângulo novo recebe uma cor
nova.

A cor é gerada pela rotação de matiz (*hue*) pelo **ângulo áureo** (~137,5°):

```cpp
float hue = fmodf((float)triIndex * 137.507764f, 360.0f);
hsvToRgb(hue, 0.75f, 1.0f, r, g, b);
```

Percorrer a roda de cores de 137,5° em 137,5° distribui as matizes de forma
quase uniforme e deixa triângulos consecutivos sempre bem distintos (é a mesma
estratégia de "espiral de cores"). A conversão HSV→RGB é feita pela função
`hsvToRgb`, com saturação 0,75 e valor 1,0.

### Buffers: um VAO único + VBO dinâmico

- **Formato intercalado (*interleaved*)**: 6 floats por vértice —
  posição `x, y, z` seguida de cor `r, g, b`.
- **Atributo 0** = posição (3 floats, *offset* 0);
  **atributo 1** = cor (3 floats, *offset* `3 * sizeof(GLfloat)`).
  *Stride* = `6 * sizeof(GLfloat)` para ambos.
- Os ponteiros de atributo são configurados **uma única vez**; como o VAO
  guarda essa "receita" (inclusive qual VBO está associado), reenviar os dados
  depois com `glBufferData(..., GL_DYNAMIC_DRAW)` não desconfigura nada.
- A cada clique o **buffer completo** é reenviado à GPU (função
  `uploadGeometry`), que é simples e suficiente para a escala do exercício.

### Desenho

- Os triângulos **completos** (múltiplos de 3 se desenhados com
  `glDrawArrays(GL_TRIANGLES, 0, 3 * n)` em uma só chamada).
- Os vértices **pendentes** (1 ou 2 cliques que ainda não fecharam triângulo)
  são desenhados com `glDrawArrays(GL_POINTS, ...)` e `glPointSize(10)`, para o
  usuário enxergar o triângulo em construção.

### HiDPI

O `glViewport` usa `glfwGetFramebufferSize` em vez de `glfwGetWindowSize`: em
telas *retina*/HiDPI o *framebuffer* é maior que a janela em pontos, e usar o
tamanho errado deixaria a cena deslocada/esticada.

---

## Notas de projeto

- **Por que a matriz de projeção é um `uniform`?** Ela transforma as
  coordenadas de mundo (pixel) para NDC no *vertex shader*; fica fixa durante a
  execução, então é enviada uma vez com `glUniformMatrix4fv`.
- **Alternativa de VAOs.** O enunciado permite "um VAO único ou diversos
  VAOs". Aqui usa-se **um VAO + um VBO dinâmico**; a variação com um VAO por
  triângulo também funcionaria (cada um com a sua própria configuração de
  atributos), ao custo de mais objetos e de uma chamada de desenho por
  triângulo.
- **Recursos liberados.** Ao final, `glDeleteVertexArrays`,
  `glDeleteBuffers`, `glDeleteProgram` e `glfwTerminate` desalocam VAO, VBO,
  programa de shader e a GLFW.
