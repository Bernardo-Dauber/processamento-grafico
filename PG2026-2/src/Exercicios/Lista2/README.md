# Lista de Exercícios 2 — Matriz de Projeção Ortográfica e viewport

*Processamento Gráfico: Fundamentos* (Unisinos) — projeção ortográfica (window/ortho),
mapeamento NDC → *viewport* e interação com o mouse.

## Como compilar e executar

A partir da raiz do repositório:

```bash
cmake -S . -B build
cmake --build build -j
```

Cada exercício vira um executável dentro de `build/`:

```bash
./build/Ex1   # Janela do mundo (ortho) -10..10 x -10..10
./build/Ex2   # Janela do mundo 0..800 x 600..0 (Y invertido)
./build/Ex3   # Cena desenhada com a câmera 2D
./build/Ex4   # Viewport restrito a um quadrante  (teclas 1..4; padrão: sup. direito)
./build/Ex5   # Mesma cena desenhada nos 4 quadrantes
./build/Ex6   # Triângulos criados pelo clique do mouse (tecla C limpa)
```

O `Ex4` também aceita o quadrante inicial por linha de comando
(ex.: `./build/Ex4 3` abre no inferior esquerdo). Em todos, `ESC` fecha a janela.

> **Atenção (HiDPI/retina):** o `glViewport` usa o tamanho do *framebuffer*
> (`glfwGetFramebufferSize`), que pode ser maior que o tamanho da janela em
> pontos (`glfwGetWindowSize`). Os exercícios 4–6 já tratam isso.

## Arquivos

| Arquivo | Exercício |
|---|---|
| `Ex1/Ex1.cpp` | Janela do mundo `xmin=-10, xmax=10, ymin=-10, ymax=10` (+ proporção de tela) |
| `Ex2/Ex2.cpp` | Janela do mundo `xmin=0, xmax=800, ymin=600, ymax=0` (pixel, Y invertido) |
| `Ex3/Ex3.cpp` | Cena posicionada em coordenadas de pixel com a câmera 2D (teoria no fim) |
| `Ex4/Ex4.cpp` | *Viewport* desenhando a cena em apenas um quadrante (teclas `1`–`4`) |
| `Ex5/Ex5.cpp` | A mesma cena desenhada nos 4 quadrantes |
| `Ex6/Ex6.cpp` | 1 clique = 1 vértice; a cada 3 vértices, um triângulo com cor nova |

---

## Exercício 1 — Janela do mundo -10..10

A "janela do mundo" é o retângulo do espaço 2D enxergado pela câmera
ortográfica. Ela é definida pela matriz `glm::ortho(...)`, aplicada a todos os
vértices no *vertex shader* (`uniform mat4 uProjection`):

```
world (window/ortho)      projection       NDC             viewport (pixels)
    -10..10           --------------->  -1..1     ------>    800 x 600
```

Como a janela da aplicação é **800x600** (4:3) e a janela do mundo é
**quadrada** (-10..10 nos dois eixos), é preciso corrigir a proporção para as
figuras não saírem achatadas na horizontal. O código segue exatamente o
enunciado (`glm::ortho(-10, 10, -10, 10)`) e faz a correção **na geometria**:
as coordenadas x dos vértices são multiplicadas por `height/width` (0,75) — o
mesmo truque do exercício 2 da Lista 1. Com isso, 1 unidade ocupa o mesmo
número de pixels nos dois eixos **e** a janela do mundo permanece
literalmente -10..10 em ambos os eixos. A cena é a "gravata borboleta" da
Lista 1, agora dentro desta janela.

## Exercício 2 — Janela do mundo 0..800 x 600..0

Agora o mundo passa a ter **as mesmas dimensões da tela** (800x600 unidades),
com `ymin=600` e `ymax=0` — ou seja, o eixo Y é **invertido**: a origem (0,0)
fica no canto superior esquerdo e o Y cresce para baixo, como no sistema de
coordenadas da tela (e do mouse). `glm::ortho(left, right, bottom, top)` mapeia
`bottom -> NDC -1` e `top -> NDC +1`; com `bottom=600, top=0`, o mundo y=600 cai
na borda inferior e o mundo y=0 na borda superior. Cada unidade do mundo
corresponde a 1 pixel, sem correção de proporção necessária.

> Verificação no código: um vértice em `(350, 150)` significa "350 pixels para
> a direita, 150 pixels para baixo" do canto superior esquerdo.

## Exercício 3 — Desenhando com a câmera 2D

Com a câmera do exercício 2, um triângulo é desenhado informando **apenas
coordenadas de pixel** (vértice `(400,150)`, base em `y=400`).

### O que acontece quando posicionamos os objetos?

A posição de cada objeto é dada **diretamente em pixels**: (0,0) é o canto
superior esquerdo, X cresce para a direita e Y para baixo. Posicionar um objeto
é dizer "em que pixel ele deve aparecer" — a projeção ortográfica mapeia cada
unidade do mundo para exatamente 1 pixel, então não há transformação, escala,
conversão ou ajuste a fazer.

### Por que é útil essa configuração?

1. **1 unidade = 1 pixel**: o layout fica previsível — o que se coloca em (x,y)
   aparece exatamente no pixel (x,y), idêntico ao que se projeta no papel.
2. **Coordenadas do mouse de graça**: `glfwGetCursorPos` já devolve o cursor
   nesse sistema (origem superior esquerda, Y para baixo); mapear clique →
   posição no mundo é a *identidade* (base do exercício 6).
3. **Sem conversões**: dispensa trocar de sistema de coordenadas ao desenhar
   UI, textos, sprites e elementos posicionados por código — padrão de jogos
   2D e aplicações de desenho/plotagem.

## Exercício 4 — Viewport em um único quadrante

O *viewport* é o retângulo da janela (em pixels) onde o framebuffer é
desenhado: `glViewport(x, y, w, h)`, com (x, y) o **canto inferior esquerdo**
do retângulo. Para restringir a cena a um quadrante, usa-se metade da largura e
metade da altura com a origem deslocada:

| Tecla | Quadrante | `glViewport` |
|---|---|---|
| `1` | superior esquerdo | `(0,   h/2, w/2, h/2)` |
| `2` | superior direito   | `(w/2, h/2, w/2, h/2)` |
| `3` | inferior esquerdo  | `(0,   0,   w/2, h/2)` |
| `4` | inferior direito   | `(w/2, 0,   w/2, h/2)` |

O padrão é o **superior direito** (tecla `2`), conforme a figura do enunciado
(ou passe `./Ex4 3` etc.). A cena é a mesma do exercício 3; os demais quadrantes
ficam com a cor de fundo. Linhas guia discretas mostram a divisão da janela.

## Exercício 5 — A mesma cena nos 4 quadrantes

A cena é desenhada **4 vezes por frame**, uma por quadrante: para cada
quadrante define-se um `glViewport` diferente e repete-se a mesma chamada de
desenho (`glDrawArrays` com o mesmo VAO). Como o mundo é 800x600 e cada
viewport tem metade do tamanho, cada quadrante recebe a cena inteira reduzida
pela metade em cada eixo — o mesmo resultado da figura do enunciado.

## Exercício 6 — Triângulos pelo clique do mouse

Cada clique com o **botão esquerdo** cria **1 vértice**; a cada 3 vértices um
triângulo é completado com uma **cor nova**:

- **Mapeamento**: projeção ortográfica com a janela do mundo do mesmo tamanho
  da tela (800x600, `glm::ortho(0, 800, 600, 0)`), então 1 unidade = 1 pixel e
  a posição do cursor (`glfwGetCursorPos` — Y para baixo) é usada diretamente
  como coordenada do vértice.
- **Buffers**: um **VAO único** com um **VBO dinâmico** `GL_DYNAMIC_DRAW`
  (interleaved: posição `x,y,z` + cor `r,g,b` por vértice). A cada clique o
  buffer completo é reenviado com `glBufferData`, e uma única chamada
  `glDrawArrays(GL_TRIANGLES, 0, 3*n)` desenha todos os triângulos.
- **Cores**: cada triângulo recebe uma cor nova gerada por rotação de matiz
  (*golden angle* ~137,5°), garantindo cores bem distintas entre triângulos
  consecutivos. Os vértices "pendentes" (1 ou 2 cliques) aparecem como pontos.
- A tecla **C** limpa a cena.

> Dica do enunciado: "Você pode criar um VAO único ou diversos VAOs". Este
> código usa um VAO único + VBO dinâmico; a variação com um VAO por triângulo
> (todos com a mesma configuração de atributos) também funciona — bastaria
> dividir o `glDrawArrays` em uma chamada por triângulo.