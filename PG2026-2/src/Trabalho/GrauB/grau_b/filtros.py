"""Registro de filtros de imagem do app.

Cada filtro é descrito por `Filtro` (`id`, `nome`, `descricao`, `funcao`), onde
`funcao(imagem) -> imagem` recebe e devolve uma imagem BGR de 3 canais
(`numpy.ndarray`) — necessário porque o pipeline de exibição (`imshow`) e a
seleção de canal esperam sempre 3 canais.

Além dos filtros, o registro tem o estado especial `"nenhum"` (constante
`ID_NENHUM`), equivalente a um filtro identidade: a imagem passa pelo pipeline
sem processamento. `proximo_filtro` percorre esse ciclo completo
(`nenhum → filtro 1 → ... → último filtro → nenhum`).

O registro cobre os 12 filtros do Grau B (cor, suavização, realce, bordas e
estilo) sobre a interface `listar_filtros`, `obter_filtro`, `aplicar_filtro` e
`proximo_filtro`.

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


#: Tamanho do kernel (lado ímpar) usado pelos três filtros de suavização.
TAMANHO_KERNEL_SUAVIZACAO = 5

#: Kernel de nitidez: realça o pixel central (5×) e subtrai os vizinhos em cruz.
KERNEL_NITIDEZ = np.array(
    [
        [0, -1, 0],
        [-1, 5, -1],
        [0, -1, 0],
    ],
    dtype=np.float32,
)

#: Kernel de relevo (emboss): diferenças em diagonal, com brilho neutro de 128.
KERNEL_RELEVO = np.array(
    [
        [-2, -1, 0],
        [-1, 1, 1],
        [0, 1, 2],
    ],
    dtype=np.float32,
)

#: Matriz de transformação de cor para o tom sépia (aplicada em BGR).
MATRIZ_SEPIA = np.array(
    [
        [0.272, 0.534, 0.131],
        [0.349, 0.686, 0.168],
        [0.393, 0.769, 0.189],
    ],
    dtype=np.float32,
)

#: Limiar fixo (0–255) da binarização.
LIMIAR_BINARIZACAO = 127

#: Limiares inferior e superior do detector de bordas de Canny.
LIMIAR_CANNY_INFERIOR = 100
LIMIAR_CANNY_SUPERIOR = 200


def _negativo(imagem: np.ndarray) -> np.ndarray:
    """Inverte os valores de cada pixel (255 - valor)."""
    return cv2.bitwise_not(imagem)


def _escala_cinza(imagem: np.ndarray) -> np.ndarray:
    """Converte para tons de cinza e devolve em BGR de 3 canais."""
    cinza = cv2.cvtColor(imagem, cv2.COLOR_BGR2GRAY)
    return cv2.cvtColor(cinza, cv2.COLOR_GRAY2BGR)


def _blur_media(imagem: np.ndarray) -> np.ndarray:
    """Suaviza calculando a média dos vizinhos num kernel 5×5."""
    kernel = (TAMANHO_KERNEL_SUAVIZACAO, TAMANHO_KERNEL_SUAVIZACAO)
    return cv2.blur(imagem, kernel)


def _blur_gaussiano(imagem: np.ndarray) -> np.ndarray:
    """Suaviza com peso gaussiano num kernel 5×5 (sigma automático)."""
    kernel = (TAMANHO_KERNEL_SUAVIZACAO, TAMANHO_KERNEL_SUAVIZACAO)
    return cv2.GaussianBlur(imagem, kernel, 0)


def _blur_mediana(imagem: np.ndarray) -> np.ndarray:
    """Suaviza com a mediana dos vizinhos num kernel 5×5 (remove sal e pimenta)."""
    return cv2.medianBlur(imagem, TAMANHO_KERNEL_SUAVIZACAO)


def _realce_nitidez(imagem: np.ndarray) -> np.ndarray:
    """Acentua bordas e detalhes com um kernel de nitidez 3×3."""
    return cv2.filter2D(imagem, -1, KERNEL_NITIDEZ)


def _bordas_sobel(imagem: np.ndarray) -> np.ndarray:
    """Detecta bordas pelo gradiente de intensidade (Sobel) e devolve BGR."""
    cinza = cv2.cvtColor(imagem, cv2.COLOR_BGR2GRAY)
    gradiente_x = cv2.Sobel(cinza, cv2.CV_64F, 1, 0, ksize=3)
    gradiente_y = cv2.Sobel(cinza, cv2.CV_64F, 0, 1, ksize=3)
    magnitude = cv2.magnitude(gradiente_x, gradiente_y)
    bordas = np.clip(magnitude, 0, 255).astype(np.uint8)
    return cv2.cvtColor(bordas, cv2.COLOR_GRAY2BGR)


def _bordas_canny(imagem: np.ndarray) -> np.ndarray:
    """Detecta bordas como linhas binárias finas (Canny) e devolve BGR."""
    cinza = cv2.cvtColor(imagem, cv2.COLOR_BGR2GRAY)
    bordas = cv2.Canny(cinza, LIMIAR_CANNY_INFERIOR, LIMIAR_CANNY_SUPERIOR)
    return cv2.cvtColor(bordas, cv2.COLOR_GRAY2BGR)


def _relevo(imagem: np.ndarray) -> np.ndarray:
    """Simula relevo 3D nas transições de intensidade (emboss)."""
    relevo = cv2.filter2D(imagem.astype(np.float32), cv2.CV_32F, KERNEL_RELEVO)
    relevo = np.clip(relevo + 128.0, 0, 255).astype(np.uint8)
    return relevo


def _binarizacao(imagem: np.ndarray) -> np.ndarray:
    """Converte para preto e branco puro por limiar fixo e devolve BGR."""
    cinza = cv2.cvtColor(imagem, cv2.COLOR_BGR2GRAY)
    _, binaria = cv2.threshold(cinza, LIMIAR_BINARIZACAO, 255, cv2.THRESH_BINARY)
    return cv2.cvtColor(binaria, cv2.COLOR_GRAY2BGR)


def _equalizacao(imagem: np.ndarray) -> np.ndarray:
    """Realça o contraste equalizando o histograma da luminância (YCrCb)."""
    ycrcb = cv2.cvtColor(imagem, cv2.COLOR_BGR2YCrCb)
    y, cr, cb = cv2.split(ycrcb)
    y_equalizado = cv2.equalizeHist(y)
    equalizada = cv2.merge((y_equalizado, cr, cb))
    return cv2.cvtColor(equalizada, cv2.COLOR_YCrCb2BGR)


def _sepia(imagem: np.ndarray) -> np.ndarray:
    """Aplica uma transformação de cor em tom sépia (envelhecido)."""
    sepia = cv2.transform(imagem.astype(np.float32), MATRIZ_SEPIA)
    return np.clip(sepia, 0, 255).astype(np.uint8)


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
    Filtro(
        id="blur_media",
        nome="Desfoque por média",
        descricao=(
            "Suaviza a imagem calculando a média dos pixels vizinhos "
            f"(kernel {TAMANHO_KERNEL_SUAVIZACAO}×{TAMANHO_KERNEL_SUAVIZACAO})."
        ),
        funcao=_blur_media,
    ),
    Filtro(
        id="blur_gaussiano",
        nome="Desfoque Gaussiano",
        descricao=(
            "Suaviza a imagem com peso maior para pixels mais próximos do centro "
            f"(kernel {TAMANHO_KERNEL_SUAVIZACAO}×{TAMANHO_KERNEL_SUAVIZACAO})."
        ),
        funcao=_blur_gaussiano,
    ),
    Filtro(
        id="blur_mediana",
        nome="Desfoque de mediana",
        descricao=(
            "Suaviza removendo ruído tipo \"sal e pimenta\", preservando bordas "
            f"(kernel {TAMANHO_KERNEL_SUAVIZACAO}×{TAMANHO_KERNEL_SUAVIZACAO})."
        ),
        funcao=_blur_mediana,
    ),
    Filtro(
        id="realce_nitidez",
        nome="Realce de nitidez",
        descricao="Acentua bordas e detalhes (unsharp mask / kernel de nitidez).",
        funcao=_realce_nitidez,
    ),
    Filtro(
        id="bordas_sobel",
        nome="Bordas (Sobel)",
        descricao="Detecta bordas pelo gradiente de intensidade.",
        funcao=_bordas_sobel,
    ),
    Filtro(
        id="bordas_canny",
        nome="Bordas (Canny)",
        descricao="Detecta bordas como linhas binárias finas.",
        funcao=_bordas_canny,
    ),
    Filtro(
        id="relevo",
        nome="Relevo (Emboss)",
        descricao="Simula um efeito de relevo 3D nas transições de intensidade.",
        funcao=_relevo,
    ),
    Filtro(
        id="binarizacao",
        nome="Binarização",
        descricao="Converte a imagem para preto e branco puro por limiar.",
        funcao=_binarizacao,
    ),
    Filtro(
        id="equalizacao",
        nome="Equalização de histograma",
        descricao="Realça o contraste redistribuindo os níveis de intensidade.",
        funcao=_equalizacao,
    ),
    Filtro(
        id="sepia",
        nome="Sépia",
        descricao="Aplica uma transformação de cor em tom sépia (envelhecido).",
        funcao=_sepia,
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
