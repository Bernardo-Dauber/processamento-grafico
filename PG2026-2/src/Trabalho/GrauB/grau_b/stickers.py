"""Stickers com canal alpha desenhados programaticamente.

Cada sticker é uma imagem RGBA (`numpy.ndarray`, `dtype=uint8`, 4 canais,
80×80 px) gerada em memória com OpenCV/NumPy — sem arquivos externos. O canal
alpha vale 255 dentro da forma desenhada e 0 no restante, o que permite
posicioná-la sobre a imagem do Modo Foto com blending por transparência.

Convenção de canais das imagens deste módulo: **RGBA** (o `alpha` é o quarto
canal). As imagens do app são **BGR**; `aplicar_sticker` faz a conversão.

O conjunto é fixo e exposto por `listar_stickers()`; `proximo_sticker` percorre
o ciclo na ordem em que os stickers aparecem na lista.

Uso direto (sem abrir janela), a partir de `PG2026-2/src/Trabalho/GrauB`:

    uv run python -c "import numpy as np; from grau_b.stickers import aplicar_sticker, listar_stickers, obter_sticker, proximo_sticker; base = np.zeros((100, 100, 3), np.uint8); print([s.id for s in listar_stickers()]); print(obter_sticker('estrela').imagem.shape, obter_sticker('estrela').imagem.dtype); print([proximo_sticker('estrela'), proximo_sticker('selo_ok')]); saida = aplicar_sticker(base, obter_sticker('estrela').imagem, (0, 0)); print(saida.shape, saida.dtype)"
"""

from __future__ import annotations

import math
from dataclasses import dataclass

import cv2
import numpy as np

#: Lado (em pixels) de todos os stickers gerados.
TAMANHO_STICKER = 80

# Cores no espaço RGB (a conversão para BGR acontece em `aplicar_sticker`).
_COR_AMARELO = (255, 214, 0)
_COR_VERMELHO = (214, 45, 45)
_COR_AZUL = (40, 90, 200)
_COR_AZUL_ESCURO = (20, 50, 120)
_COR_VERDE = (40, 170, 80)
_COR_VERDE_ESCURO = (20, 120, 55)
_COR_BRANCO = (255, 255, 255)

_CENTRO = (TAMANHO_STICKER // 2, TAMANHO_STICKER // 2)


@dataclass(frozen=True)
class Sticker:
    """Sticker disponível: identificador, rótulo e a imagem RGBA."""

    id: str
    nome: str
    imagem: np.ndarray


def _tela_rgb() -> np.ndarray:
    """Cria a tela RGB preta (transparente) sobre a qual o sticker é pintado."""
    return np.zeros((TAMANHO_STICKER, TAMANHO_STICKER, 3), dtype=np.uint8)


def _tela_alpha() -> np.ndarray:
    """Cria a máscara de alpha (0 = transparente, 255 = opaco)."""
    return np.zeros((TAMANHO_STICKER, TAMANHO_STICKER), dtype=np.uint8)


def _compor_rgba(rgb: np.ndarray, alpha: np.ndarray) -> np.ndarray:
    """Monta a imagem RGBA final a partir do RGB pintado e da máscara alpha."""
    rgba = np.zeros((TAMANHO_STICKER, TAMANHO_STICKER, 4), dtype=np.uint8)
    rgba[:, :, :3] = rgb
    rgba[:, :, 3] = alpha
    return rgba


def _pontos_estrela(
    raio_externo: float = 34.0,
    raio_interno: float = 14.0,
) -> np.ndarray:
    """Calcula os 10 vértices de uma estrela de 5 pontas apontando para cima."""
    centro_x, centro_y = _CENTRO
    pontos: list[list[int]] = []
    for indice in range(10):
        # Começa no topo (-90°) e alterna entre raio externo e interno.
        angulo = -math.pi / 2 + indice * math.pi / 5
        raio = raio_externo if indice % 2 == 0 else raio_interno
        x = centro_x + raio * math.cos(angulo)
        y = centro_y + raio * math.sin(angulo)
        pontos.append([round(x), round(y)])
    return np.array(pontos, dtype=np.int32)


def _pontos_coracao() -> np.ndarray:
    """Calcula o contorno de um coração pela curva paramétrica clássica.

    A curva `x = 16·sen³(t)`, `y = 13·cos(t) − 5·cos(2t) − 2·cos(3t) − cos(4t)`
    é amostrada, normalizada para o tamanho do sticker e espelhada no eixo Y
    (a coordenada matemática cresce para cima; a da imagem, para baixo).
    """
    amostras = 120
    xs: list[float] = []
    ys: list[float] = []
    for indice in range(amostras):
        t = 2 * math.pi * indice / amostras
        xs.append(16 * math.sin(t) ** 3)
        ys.append(
            13 * math.cos(t)
            - 5 * math.cos(2 * t)
            - 2 * math.cos(3 * t)
            - math.cos(4 * t)
        )

    margem = 6.0
    largura = max(xs) - min(xs)
    altura = max(ys) - min(ys)
    escala = (TAMANHO_STICKER - 2 * margem) / max(largura, altura)

    centro_x, centro_y = _CENTRO
    meio_x = (max(xs) + min(xs)) / 2
    meio_y = (max(ys) + min(ys)) / 2
    pontos = [
        [
            round(centro_x + (x - meio_x) * escala),
            round(centro_y - (y - meio_y) * escala),
        ]
        for x, y in zip(xs, ys)
    ]
    return np.array(pontos, dtype=np.int32)


def gerar_estrela() -> np.ndarray:
    """Estrela de 5 pontas amarela, preenchida."""
    rgb = _tela_rgb()
    alpha = _tela_alpha()
    pontos = _pontos_estrela()
    cv2.fillPoly(rgb, [pontos], _COR_AMARELO)
    cv2.fillPoly(alpha, [pontos], 255)
    return _compor_rgba(rgb, alpha)


def gerar_coracao() -> np.ndarray:
    """Coração vermelho, preenchido."""
    rgb = _tela_rgb()
    alpha = _tela_alpha()
    pontos = _pontos_coracao()
    cv2.fillPoly(rgb, [pontos], _COR_VERMELHO)
    cv2.fillPoly(alpha, [pontos], 255)
    return _compor_rgba(rgb, alpha)


def gerar_circulo() -> np.ndarray:
    """Círculo azul preenchido com uma borda mais escura."""
    rgb = _tela_rgb()
    alpha = _tela_alpha()
    raio = 34
    cv2.circle(alpha, _CENTRO, raio, 255, thickness=-1)
    cv2.circle(rgb, _CENTRO, raio, _COR_AZUL, thickness=-1)
    cv2.circle(rgb, _CENTRO, raio, _COR_AZUL_ESCURO, thickness=4, lineType=cv2.LINE_AA)
    return _compor_rgba(rgb, alpha)


def gerar_triangulo() -> np.ndarray:
    """Triângulo equilátero verde, preenchido."""
    rgb = _tela_rgb()
    alpha = _tela_alpha()
    pontos = np.array(
        [
            [40, 8],   # vértice superior
            [72, 66],  # vértice inferior direito
            [8, 66],   # vértice inferior esquerdo
        ],
        dtype=np.int32,
    )
    cv2.fillPoly(rgb, [pontos], _COR_VERDE)
    cv2.fillPoly(alpha, [pontos], 255)
    return _compor_rgba(rgb, alpha)


def gerar_selo_ok() -> np.ndarray:
    """Selo: círculo verde com um "check" branco desenhado dentro."""
    rgb = _tela_rgb()
    alpha = _tela_alpha()
    raio = 34
    check = np.array([[25, 41], [35, 53], [56, 27]], dtype=np.int32)
    cv2.circle(alpha, _CENTRO, raio, 255, thickness=-1)
    cv2.circle(rgb, _CENTRO, raio, _COR_VERDE, thickness=-1)
    cv2.circle(rgb, _CENTRO, raio, _COR_VERDE_ESCURO, thickness=3, lineType=cv2.LINE_AA)
    cv2.polylines(rgb, [check], isClosed=False, color=_COR_BRANCO, thickness=6,
                  lineType=cv2.LINE_AA)
    return _compor_rgba(rgb, alpha)


#: Registro ordenado de stickers. A ordem é a do ciclo da tecla `t`.
STICKERS: tuple[Sticker, ...] = (
    Sticker(id="estrela", nome="Estrela", imagem=gerar_estrela()),
    Sticker(id="coracao", nome="Coração", imagem=gerar_coracao()),
    Sticker(id="circulo", nome="Círculo", imagem=gerar_circulo()),
    Sticker(id="triangulo", nome="Triângulo", imagem=gerar_triangulo()),
    Sticker(id="selo_ok", nome="Selo OK", imagem=gerar_selo_ok()),
)

#: Sticker selecionado quando o Modo Foto é aberto.
STICKER_PADRAO = STICKERS[0].id


def listar_stickers() -> list[Sticker]:
    """Devolve os stickers disponíveis, na ordem do ciclo da tecla `t`."""
    return list(STICKERS)


def obter_sticker(id_sticker: str) -> Sticker:
    """Devolve o sticker registrado com o `id` informado.

    Levanta `ValueError` quando o `id` não está registrado.
    """
    for sticker in STICKERS:
        if sticker.id == id_sticker:
            return sticker
    raise ValueError(f"sticker desconhecido: {id_sticker!r}")


def proximo_sticker(id_atual: str | None) -> str:
    """Devolve o próximo `id` do ciclo de stickers.

    `None` devolve o primeiro sticker da lista. Um `id` desconhecido levanta
    `ValueError`.
    """
    ids = [sticker.id for sticker in STICKERS]
    if id_atual is None:
        return ids[0]
    if id_atual not in ids:
        raise ValueError(f"sticker desconhecido: {id_atual!r}")
    return ids[(ids.index(id_atual) + 1) % len(ids)]


def aplicar_sticker(
    imagem_base_bgr: np.ndarray,
    sticker_rgba: np.ndarray,
    posicao_xy: tuple[int, int],
) -> np.ndarray:
    """Desenha o sticker sobre a imagem, com blending por alpha.

    `posicao_xy` é o **centro** do sticker em coordenadas da imagem base.
    Devolve uma nova imagem BGR com o sticker sobreposto; a entrada não é
    modificada.

    O recorte é limitado aos limites da imagem: quando o centro está perto
    de uma borda (ou fora dela), só a parte visível do sticker é aplicada, sem
    erro de índice. Se não houver sobreposição, devolve uma cópia da base.

    Levanta `ValueError` se a base não for uma imagem BGR de 3 canais ou se o
    sticker não for RGBA de 4 canais.
    """
    if imagem_base_bgr.ndim != 3 or imagem_base_bgr.shape[2] != 3:
        raise ValueError(
            "a imagem base deve ser BGR de 3 canais; "
            f"recebida com forma {imagem_base_bgr.shape}"
        )
    if sticker_rgba.ndim != 3 or sticker_rgba.shape[2] != 4:
        raise ValueError(
            "o sticker deve ser RGBA de 4 canais; "
            f"recebido com forma {sticker_rgba.shape}"
        )

    resultado = imagem_base_bgr.copy()
    altura_base, largura_base = imagem_base_bgr.shape[:2]
    altura_sticker, largura_sticker = sticker_rgba.shape[:2]
    centro_x, centro_y = int(posicao_xy[0]), int(posicao_xy[1])

    # Coordenadas do canto superior esquerdo do sticker na imagem base.
    x_inicio = centro_x - largura_sticker // 2
    y_inicio = centro_y - altura_sticker // 2

    # Interseção entre o retângulo do sticker e a imagem.
    x_ini = max(x_inicio, 0)
    y_ini = max(y_inicio, 0)
    x_fim = min(x_inicio + largura_sticker, largura_base)
    y_fim = min(y_inicio + altura_sticker, altura_base)

    if x_ini >= x_fim or y_ini >= y_fim:
        return resultado

    # Recorte correspondente no sticker.
    sticker_x_ini = x_ini - x_inicio
    sticker_y_ini = y_ini - y_inicio
    recorte = sticker_rgba[
        sticker_y_ini : sticker_y_ini + (y_fim - y_ini),
        sticker_x_ini : sticker_x_ini + (x_fim - x_ini),
    ]

    # RGBA (RGB + alpha) -> BGR + alpha, em ponto flutuante para o blending.
    recorte_bgr = cv2.cvtColor(recorte, cv2.COLOR_RGBA2BGR).astype(np.float32) / 255.0
    alpha = recorte[:, :, 3].astype(np.float32) / 255.0
    alpha = alpha[:, :, np.newaxis]

    regiao = resultado[y_ini:y_fim, x_ini:x_fim].astype(np.float32) / 255.0
    misturado = regiao * (1.0 - alpha) + recorte_bgr * alpha
    resultado[y_ini:y_fim, x_ini:x_fim] = np.clip(
        misturado * 255.0, 0, 255
    ).astype(np.uint8)
    return resultado
