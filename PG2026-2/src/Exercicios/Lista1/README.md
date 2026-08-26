# Lista de Exercícios 1 — Introdução à OpenGL Moderna

Primitivas Gráficas, *Shaders* & *Buffers* — Processamento Gráfico: Fundamentos (Unisinos)

## Como compilar e executar

A partir da raiz do repositório:

```bash
cmake -S . -B build
cmake --build build -j
```

Cada exercício vira um executável dentro de `build/`:

```bash
./build/1        # Exercício 1
./build/2        # Exercício 2
./build/3        # Exercício 3
./build/4        # Exercício 4
```

Os exercícios 1 e 2 aceitam, opcionalmente, o modo/forma inicial como argumento
de linha de comando (ex.: `./build/2 6` abre direto na estrela). Em todos eles,
`ESC` fecha a janela.

## Arquivos

| Arquivo | Exercício |
|---|---|
| `1/1.cpp` | Dois triângulos, nas 4 formas de desenho |
| `2/2.cpp` | Círculo paramétrico e as formas derivadas dele |
| `3/3.cpp` | Triângulo com cor por vértice (atributos no VBO) |
| `4/4.cpp` | Desenho reproduzido com primitivas (casa) |
| `4/desenho-referencia.png` | O desenho em papel quadriculado que o `4.cpp` reproduz |

---

## Exercício 1 — Dois triângulos

Dois triângulos que compartilham o vértice central `(0,0)`, formando uma
"gravata borboleta". Um único VBO com 6 vértices atende aos quatro itens: o que
muda entre eles é **apenas a primitiva** informada na chamada de desenho.

| Tecla | Item | Chamada de desenho |
|---|---|---|
| `1` | a) Apenas com o polígono preenchido | `glDrawArrays(GL_TRIANGLES, 0, 6)` |
| `2` | b) Apenas com contorno | `glDrawArrays(GL_LINE_LOOP, 0, 3)` + `glDrawArrays(GL_LINE_LOOP, 3, 3)` |
| `3` | c) Apenas como pontos | `glDrawArrays(GL_POINTS, 0, 6)` |
| `4` | d) Com as 3 formas juntas | as três chamadas acima, em sequência |

O contorno usa **dois** `GL_LINE_LOOP` em vez de um só porque são dois contornos
fechados independentes — um único laço com os 6 vértices ligaria o vértice `v2`
ao `v3` e fecharia a figura por fora.

No modo "apenas pontos" aparecem 5 pontos, e não 6: os vértices `v2` e `v3` estão
ambos em `(0,0)`, e portanto se sobrepõem na tela.

## Exercício 2 — Círculo e formas derivadas

Os vértices são gerados pela **equação paramétrica do círculo**:

```
x(t) = cx + raio * cos(t)
y(t) = cy + raio * sen(t)        t de 0 a 2π
```

e organizados como um leque de triângulos (`GL_TRIANGLE_FAN`): o primeiro vértice
é o centro, e os demais percorrem o arco, de modo que cada par de pontos
consecutivos forma um triângulo com o centro.

Variando só o **número de pontos** e o **intervalo angular** do mesmo gerador
(`gerarArco`), saem quase todas as formas pedidas:

| Tecla | Forma | Como |
|---|---|---|
| `1` | Círculo | 100 pontos, volta completa |
| `2` | a) Octógono | 8 pontos, volta completa |
| `3` | b) Pentágono | 5 pontos, começando em 90° para a ponta ficar para cima |
| `4` | c) Pac-man | volta incompleta, de 35° a 325° — o arco que falta é a boca |
| `5` | d) Fatia de pizza | um arco estreito, de 20° a 70° |
| `6` | e) **DESAFIO 1**: estrela | raios alternados: pontas no raio externo, vales no interno |
| `7` | f) **DESAFIO 2**: espiral | `r(t) = a·t` — o raio cresce junto com o ângulo |

Dois detalhes valem nota:

- **Proporção da tela.** A janela é 800x600, mas o espaço normalizado (NDC) vai de
  -1 a 1 nos dois eixos. Sem corrigir, o círculo sairia achatado como uma elipse;
  por isso o deslocamento em `x` é multiplicado por `altura/largura` (0,75).
- **A espiral é uma curva aberta**, não uma área preenchida. Ela é a única forma
  desenhada com `GL_LINE_STRIP` e sem vértice central — as demais usam
  `GL_TRIANGLE_FAN`.

Na estrela, a razão de 0,382 entre o raio interno e o externo é a que deixa os
lados das pontas alinhados, como numa estrela de 5 pontas desenhada à mão.

## Exercício 3 — Triângulo com cor por vértice

Triângulo com os vértices P1, P2 e P3 respectivamente nas cores vermelho, verde e
azul. Diferente dos exercícios 1 e 2, a cor **não** vem de uma variável `uniform`:
ela é um atributo de vértice, guardado no buffer junto com a posição.

### a) Uma possível configuração dos buffers (VBO, VAO)

Um **único VBO** com os atributos **intercalados** (*interleaved*): para cada
vértice, primeiro as 3 coordenadas, logo em seguida os 3 canais de cor — 6 floats
(24 bytes) por vértice.

```
|   P1                  |   P2                  |   P3                  |
| x  y  z | r  g  b     | x  y  z | r  g  b     | x  y  z | r  g  b     |
| 0  1  2 | 3  4  5     | 6  7  8 | 9 10 11     |12 13 14 |15 16 17     |
^         ^
offset 0  offset 12 bytes (3 * sizeof(GLfloat))

stride (distância entre dois vértices) = 6 * sizeof(GLfloat) = 24 bytes
```

O **VAO** guarda a "receita" de leitura desse VBO, com dois ponteiros de atributo:

```cpp
// atributo 0 - posição
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), (GLvoid*)0);
glEnableVertexAttribArray(0);

// atributo 1 - cor
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), (GLvoid*)(3*sizeof(GLfloat)));
glEnableVertexAttribArray(1);
```

O `stride` de 6 floats é o que faz a OpenGL "pular" a cor para chegar na posição
do próximo vértice; o `offset` de 12 bytes é o que faz o atributo 1 começar a ler
logo depois das coordenadas.

**Configuração alternativa:** usar **dois VBOs separados**, um só com as posições
e outro só com as cores. Nesse caso o `stride` de cada um seria `3*sizeof(GLfloat)`
e o `offset` de ambos seria 0; o VAO continuaria com dois ponteiros de atributo,
cada um lendo de um VBO diferente. A versão intercalada costuma ser preferida por
localidade de memória — os dados de um mesmo vértice ficam juntos, e a GPU os lê
de uma vez só.

### b) Como estes atributos seriam identificados no *vertex shader*

Pelo qualificador `layout (location = N)`, onde `N` é exatamente o mesmo número
passado como primeiro argumento de `glVertexAttribPointer` e de
`glEnableVertexAttribArray`:

```glsl
#version 400
layout (location = 0) in vec3 position;   // <-> atributo 0
layout (location = 1) in vec3 color;      // <-> atributo 1
out vec3 vertexColor;
void main()
{
    gl_Position = vec4(position.x, position.y, position.z, 1.0);
    vertexColor = color;
}
```

É esse número que amarra o dado do buffer à variável de entrada do shader. O
*vertex shader* repassa a cor adiante por uma variável de saída
(`out vec3 vertexColor`), recebida no *fragment shader* como entrada
(`in vec3 vertexColor`):

```glsl
#version 400
in vec3 vertexColor;
out vec4 color;
void main()
{
    color = vec4(vertexColor, 1.0);
}
```

Entre uma etapa e outra, o rasterizador **interpola** o valor ao longo da face do
triângulo. É por isso que o resultado é um degradê contínuo entre vermelho, verde
e azul, e não três regiões de cor chapada.

## Exercício 4 — Desenho com primitivas

O enunciado pede duas coisas: fazer o desenho em papel quadriculado e depois
reproduzi-lo com primitivas da OpenGL. O desenho de referência (feito no
computador, como o enunciado permite) está em
[`4/desenho-referencia.png`](4/desenho-referencia.png) — quadriculado, no mesmo
sistema de coordenadas NDC usado no código, com os vértices principais anotados.
`4/4.cpp` é a reprodução dele.

Uma casa com telhado, porta, janela, chão e sol. Cada parte do desenho tem o seu
**próprio VAO** e a sua **própria chamada de desenho** — 11 no total —, o que
permite usar primitivas diferentes para cada elemento:

| Primitiva | Partes |
|---|---|
| `GL_TRIANGLE_FAN` | chão, sol, corpo, porta, maçaneta, janela |
| `GL_TRIANGLES` | telhado |
| `GL_LINES` | caixilhos da janela (dois segmentos soltos formando uma cruz) |
| `GL_LINE_LOOP` | contornos do corpo, do telhado e da janela |

O céu é a própria cor de fundo (`glClearColor`), então não precisa de VAO.

Os contornos reaproveitam exatamente a mesma geometria das partes preenchidas —
como no exercício 1, o que muda é só a primitiva da chamada de desenho. Os
caixilhos usam `GL_LINES` e não `GL_LINE_STRIP` porque são dois segmentos
independentes: cada par de vértices é uma linha separada, sem ligação entre elas.

As partes são desenhadas na ordem em que são adicionadas, então o que vem depois
aparece por cima.
