"""Seleção de canal de cor aplicada antes dos filtros.

O app pode processar a imagem completa ou apenas um dos canais R, G ou B.
A seleção acontece no início do pipeline (`canais` → `filtros`): para um canal
específico, a imagem devolvida mantém somente aquele canal e zera os outros
dois, preservando forma e dtype da entrada (o `imshow` continua recebendo BGR
de 3 canais).

Convenção de canais: o OpenCV trabalha em **BGR**, então o índice 0 é B,
1 é G e 2 é R.

Uso direto (sem abrir janela), a partir de `PG2026-2/src/Trabalho/GrauB`:

    uv run python -c "import numpy as np; from grau_b.canais import aplicar_selecao_canal, proximo_canal; imagem = np.full((2, 2, 3), (10, 20, 30), dtype=np.uint8); print(aplicar_selecao_canal(imagem, 'R')[0, 0]); print(aplicar_selecao_canal(imagem, 'todos') is imagem); print([proximo_canal('todos'), proximo_canal('R'), proximo_canal('G'), proximo_canal('B')])"
"""

from __future__ import annotations

import numpy as np

CANAIS = ("todos", "R", "G", "B")
CANAL_PADRAO = "todos"

# Índice de cada canal no array BGR do OpenCV.
_INDICE_BGR = {"R": 2, "G": 1, "B": 0}


def aplicar_selecao_canal(imagem: np.ndarray, canal: str) -> np.ndarray:
    """Devolve a imagem com apenas o canal selecionado (demais canais zerados).

    `canal` deve ser um de `{"todos", "R", "G", "B"}`. Para `"todos"` a própria
    imagem de entrada é devolvida sem cópia; para um canal específico, a saída é
    uma nova imagem BGR de mesma forma e dtype, com os outros dois canais em 0.

    Levanta `ValueError` para um canal desconhecido ou para uma imagem que não
    tenha 3 canais.
    """
    if canal not in CANAIS:
        raise ValueError(
            f"canal inválido: {canal!r} (esperado um de {', '.join(CANAIS)})"
        )

    if canal == "todos":
        return imagem

    if imagem.ndim != 3 or imagem.shape[2] != 3:
        raise ValueError(
            "a seleção de canal exige uma imagem BGR de 3 canais; "
            f"recebida com forma {imagem.shape}"
        )

    indice = _INDICE_BGR[canal]
    resultado = np.zeros_like(imagem)
    resultado[:, :, indice] = imagem[:, :, indice]
    return resultado


def proximo_canal(canal_atual: str) -> str:
    """Devolve o próximo canal do ciclo `todos → R → G → B → todos`.

    Levanta `ValueError` para um canal desconhecido.
    """
    if canal_atual not in CANAIS:
        raise ValueError(
            f"canal inválido: {canal_atual!r} "
            f"(esperado um de {', '.join(CANAIS)})"
        )
    return CANAIS[(CANAIS.index(canal_atual) + 1) % len(CANAIS)]
