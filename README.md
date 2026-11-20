# Processamento Gráfico: Fundamentos — 2026/2

Repositório com os trabalhos e listas de exercícios desenvolvidos para a Atividade Acadêmica **Processamento Gráfico: Fundamentos**, do curso de **Ciência da Computação — Unisinos**.

**Autor:** Bernardo Dauber

---

## Trabalhos

| Trabalho | Documentação | Tecnologias | Descrição |
| --- | --- | --- | --- |
| **Trabalho Grau A — Mini-Mario** | [README.md](PG2026-2/src/Trabalho/GrauA/README.md) | C++ / OpenGL 3.3+ (GLFW, GLAD, GLM) | Jogo 2D platformer com shaders, spritesheets animados, parallax, câmera, colisão AABB e leitura de fase via `config.txt`. |
| **Trabalho Grau B — Editor de Imagens e Vídeo** | [README.md](PG2026-2/src/Trabalho/GrauB/README.md) | Python / OpenCV / Tkinter / Pillow (`uv`) | Aplicativo interativo em modo Foto e Webcam com 12 filtros combináveis em pipeline, canais RGB, operações aritméticas, stickers estáticos e animados (spritesheets), detecção facial (Haar Cascades) e desfoque/fundo virtual na webcam. |

---

## Listas de Exercícios

| Lista | Documentação | Tecnologias | Descrição |
| --- | --- | --- | --- |
| **Lista 1** | [README.md](PG2026-2/src/Exercicios/Lista1/README.md) | C++ / OpenGL 3.3+ | Introdução à OpenGL moderna: primitivas gráficas, shaders e buffers (Exercícios 1 a 4). |
| **Lista 2** | [README.md](PG2026-2/src/Exercicios/Lista2/README.md) | C++ / OpenGL 3.3+ | Matriz de projeção ortográfica (`ortho`), mapeamento NDC → *viewport*, quadrantes e interação com mouse (Ex1 a Ex6). |
| **Lista 3** | [README.md](PG2026-2/src/Exercicios/Lista3/README.md) | C++ / OpenGL 3.3+ | Criação de triângulos a partir do clique do mouse com buffers de geometria e shaders. |

---

## Estrutura Rápida

```text
.
├── README.md                                  # Este índice
├── PG2026-2/
│   ├── CMakeLists.txt                         # Build C++/OpenGL (Listas e Grau A)
│   ├── GettingStarted.md                      # Guia de configuração do ambiente C++/OpenGL
│   └── src/
│       ├── Exercicios/
│       │   ├── Lista1/README.md
│       │   ├── Lista2/README.md
│       │   └── Lista3/README.md
│       └── Trabalho/
│           ├── GrauA/README.md
│           └── GrauB/README.md                # Projeto Python independente (uv)
└── docs/
    └── adr/0001-mundo-virtual-960x540-camera.md
```

---

## Como Executar (Resumo)

Para instruções detalhadas, controles e dependências, consulte o `README.md` de cada projeto acima.

### Projetos C++ / OpenGL (Listas 1–3 e Grau A)

```bash
cd PG2026-2
cmake -S . -B build
cmake --build build -j
```

### Trabalho Grau B (Python / OpenCV / Tkinter)

```bash
cd PG2026-2/src/Trabalho/GrauB
uv sync
uv run grau-b
```

---

## Referências e Enunciados

- [Guia de configuração do ambiente (`PG2026-2/GettingStarted.md`)](PG2026-2/GettingStarted.md)
- [README base do módulo `PG2026-2`](PG2026-2/README.md)
- [ADR 0001 — Mundo virtual 960×540 e câmera (Grau A)](docs/adr/0001-mundo-virtual-960x540-camera.md)
- Enunciados em PDF na raiz: [Lista 1](Lista1.pdf) · [Lista 2](Lista%202.pdf) · [Lista 3](Lista%203.pdf) · [Trabalho Grau A](EnunciadoGrauA-Jogo.pdf)
