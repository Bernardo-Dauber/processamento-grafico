"""Registro de operações aritméticas entre duas imagens do app.

Cada operação é descrita por `Operacao` (`id`, `nome`, `descricao`, `funcao`),
onde `funcao(imagem_a, imagem_b) -> imagem` recebe duas imagens BGR de 3 canais
(`numpy.ndarray`) já com o mesmo tamanho e devolve uma nova imagem BGR de 3
canais. O redimensionamento da segunda imagem para o tamanho da primeira é
responsabilidade de `aplicar_operacao`, não das funções registradas.

Semântica dos resultados:

- `soma` e `subtracao` usam as operações saturantes do OpenCV, então valores
  fora de `[0, 255]` são truncados nos extremos (sem wraparound).
- `multiplicacao` normaliza as duas imagens para `[0, 1]` antes de multiplicar
  e reescala para `[0, 255]`, evitando que quase tudo sature em branco.
- `mescla` é a média ponderada 50/50 (`cv2.addWeighted`).

As operações são aplicadas sobre a imagem original e a segunda imagem
carregada; um resultado anterior não é realimentado. O Modo Foto aplica o
pipeline de canal/filtro sobre o resultado da operação para exibição.

Uso direto (sem abrir janela), a partir de `PG2026-2/src/Trabalho/GrauB`:

    uv run python -c "import numpy as np; from grau_b.operacoes import aplicar_operacao, listar_operacoes, proxima_operacao; a = np.full((4, 4, 3), 100, dtype=np.uint8); b = np.full((2, 2, 3), 50, dtype=np.uint8); print([o.id for o in listar_operacoes()]); print(aplicar_operacao('soma', a, b).shape); print(aplicar_operacao('mescla', a, b)[0, 0]); print([proxima_operacao('soma'), proxima_operacao('mescla')])"
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Callable

import cv2
import numpy as np


@dataclass(frozen=True)
class Operacao:
    """Descrição de uma operação: identificador, rótulos e a transformação."""

    id: str
    nome: str
    descricao: str
    funcao: Callable[[np.ndarray, np.ndarray], np.ndarray]


def _soma(imagem_a: np.ndarray, imagem_b: np.ndarray) -> np.ndarray:
    """Soma pixel a pixel, saturando em 255."""
    return cv2.add(imagem_a, imagem_b)


def _subtracao(imagem_a: np.ndarray, imagem_b: np.ndarray) -> np.ndarray:
    """Subtrai `imagem_b` de `imagem_a`, saturando em 0."""
    return cv2.subtract(imagem_a, imagem_b)


def _multiplicacao(imagem_a: np.ndarray, imagem_b: np.ndarray) -> np.ndarray:
    """Multiplica os valores normalizados e devolve o resultado em `[0, 255]`."""
    normalizada_a = imagem_a.astype(np.float32) / 255.0
    normalizada_b = imagem_b.astype(np.float32) / 255.0
    produto = cv2.multiply(normalizada_a, normalizada_b)
    return np.clip(produto * 255.0, 0, 255).astype(np.uint8)


def _mescla(imagem_a: np.ndarray, imagem_b: np.ndarray) -> np.ndarray:
    """Combina as duas imagens por média ponderada 50/50."""
    return cv2.addWeighted(imagem_a, 0.5, imagem_b, 0.5, 0.0)


#: Registro ordenado de operações. A ordem é a do ciclo da tecla `m`.
OPERACOES: tuple[Operacao, ...] = (
    Operacao(
        id="soma",
        nome="Soma",
        descricao="Soma os valores de pixel das duas imagens, saturando em 255.",
        funcao=_soma,
    ),
    Operacao(
        id="subtracao",
        nome="Subtração",
        descricao=(
            "Subtrai os valores de pixel da segunda imagem dos da primeira, "
            "saturando em 0."
        ),
        funcao=_subtracao,
    ),
    Operacao(
        id="multiplicacao",
        nome="Multiplicação",
        descricao=(
            "Multiplica os valores de pixel normalizados das duas imagens."
        ),
        funcao=_multiplicacao,
    ),
    Operacao(
        id="mescla",
        nome="Mescla",
        descricao="Combina as duas imagens por média ponderada (50/50).",
        funcao=_mescla,
    ),
)


def listar_operacoes() -> list[Operacao]:
    """Devolve as operações registradas, na ordem do ciclo da tecla `m`."""
    return list(OPERACOES)


def obter_operacao(id_operacao: str) -> Operacao:
    """Devolve a operação registrada com o `id` informado.

    Levanta `ValueError` quando o `id` não está registrado.
    """
    for operacao in OPERACOES:
        if operacao.id == id_operacao:
            return operacao
    raise ValueError(f"operação desconhecida: {id_operacao!r}")


def aplicar_operacao(
    id_operacao: str,
    imagem_a: np.ndarray,
    imagem_b: np.ndarray,
) -> np.ndarray:
    """Aplica a operação `id_operacao` entre as duas imagens.

    `imagem_b` é redimensionada para o tamanho de `imagem_a` antes da operação,
    então as duas entradas podem ter dimensões diferentes. Devolve uma nova
    imagem BGR de 3 canais. Levanta `ValueError` para um `id` desconhecido.
    """
    operacao = obter_operacao(id_operacao)
    altura_a, largura_a = imagem_a.shape[:2]
    imagem_b_ajustada = cv2.resize(
        imagem_b,
        (largura_a, altura_a),
        interpolation=cv2.INTER_LINEAR,
    )
    return operacao.funcao(imagem_a, imagem_b_ajustada)


def proxima_operacao(id_atual: str | None) -> str:
    """Devolve o próximo `id` do ciclo `soma → subtração → multiplicação → mescla`.

    `None` é tratado como "antes do primeiro", devolvendo `soma`; um `id`
    desconhecido levanta `ValueError`.
    """
    ids = [operacao.id for operacao in OPERACOES]

    if id_atual is None:
        return ids[0]
    if id_atual not in ids:
        raise ValueError(f"operação desconhecida: {id_atual!r}")

    return ids[(ids.index(id_atual) + 1) % len(ids)]
