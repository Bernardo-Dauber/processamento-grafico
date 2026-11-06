"""Registro de filtros de imagem do app.

Cada filtro é descrito por `Filtro` (`id`, `nome`, `descricao`, `funcao`), onde
`funcao(imagem) -> imagem` recebe e devolve uma imagem BGR de 3 canais
(`numpy.ndarray`) — necessário porque o pipeline de exibição (`imshow`) e a
seleção de canal esperam sempre 3 canais.

Além dos filtros, o registro tem o estado especial `"nenhum"` (constante
`ID_NENHUM`), equivalente a um filtro identidade: a imagem passa pelo pipeline
sem processamento. `proximo_filtro` percorre esse ciclo completo
(`nenhum → filtro 1 → ... → último filtro → nenhum`).

Este módulo entrega o mecanismo de registro/seleção; a lista final de ≥10
filtros é populada pela tarefa `filtros-suavizacao-realce` sem mudar a
interface (`listar_filtros`, `obter_filtro`, `aplicar_filtro`,
`proximo_filtro`).

Uso direto (sem abrir janela), a partir de `PG2026-2/src/Trabalho/GrauB`:

    uv run python -c "import numpy as np; from grau_b.filtros import aplicar_filtro, listar_filtros, proximo_filtro; imagem = np.full((2, 2, 3), (10, 20, 30), dtype=np.uint8); print([f.id for f in listar_filtros()]); print(aplicar_filtro('negativo', imagem)[0, 0]); print([proximo_filtro('nenhum'), proximo_filtro('cinza')])"
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Callable

import cv2
import numpy as np

#: Estado "sem filtro" do registro (filtro identidade).
ID_NENHUM = "nenhum"


@dataclass(frozen=True)
class Filtro:
    """Descrição de um filtro: identificador, rótulos e a transformação."""

    id: str
    nome: str
    descricao: str
    funcao: Callable[[np.ndarray], np.ndarray]


def _negativo(imagem: np.ndarray) -> np.ndarray:
    """Inverte os valores de cada pixel (255 - valor)."""
    return cv2.bitwise_not(imagem)


def _escala_cinza(imagem: np.ndarray) -> np.ndarray:
    """Converte para tons de cinza e devolve em BGR de 3 canais."""
    cinza = cv2.cvtColor(imagem, cv2.COLOR_BGR2GRAY)
    return cv2.cvtColor(cinza, cv2.COLOR_GRAY2BGR)


#: Registro ordenado de filtros. A ordem é a do ciclo da tecla `f`.
FILTROS: tuple[Filtro, ...] = (
    Filtro(
        id="negativo",
        nome="Negativo",
        descricao="Inverte os valores de cada pixel (255 - valor).",
        funcao=_negativo,
    ),
    Filtro(
        id="cinza",
        nome="Escala de cinza",
        descricao="Converte a imagem para tons de cinza.",
        funcao=_escala_cinza,
    ),
)


def listar_filtros() -> list[Filtro]:
    """Devolve os filtros registrados, na ordem do ciclo da tecla `f`."""
    return list(FILTROS)


def obter_filtro(id_filtro: str) -> Filtro:
    """Devolve o filtro registrado com o `id` informado.

    Levanta `ValueError` quando o `id` não está registrado (inclusive para
    `"nenhum"`, que não é um filtro de verdade).
    """
    for filtro in FILTROS:
        if filtro.id == id_filtro:
            return filtro
    raise ValueError(f"filtro desconhecido: {id_filtro!r}")


def aplicar_filtro(id_filtro: str | None, imagem: np.ndarray) -> np.ndarray:
    """Aplica o filtro `id_filtro` à imagem e devolve o resultado.

    `None` e `"nenhum"` são o estado "sem filtro": a própria imagem é devolvida
    sem processamento. Levanta `ValueError` para um `id` desconhecido.
    """
    if id_filtro is None or id_filtro == ID_NENHUM:
        return imagem
    return obter_filtro(id_filtro).funcao(imagem)


def proximo_filtro(id_atual: str | None) -> str:
    """Devolve o próximo `id` do ciclo, incluindo o estado `"nenhum"`.

    O ciclo é `nenhum → primeiro filtro → ... → último filtro → nenhum`.
    `None` é tratado como `"nenhum"`; um `id` desconhecido levanta `ValueError`.
    """
    ids = [ID_NENHUM, *(filtro.id for filtro in FILTROS)]

    atual = ID_NENHUM if id_atual is None else id_atual
    if atual not in ids:
        raise ValueError(f"filtro desconhecido: {atual!r}")

    return ids[(ids.index(atual) + 1) % len(ids)]
