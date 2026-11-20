"""Segmentação de pessoa e efeitos de fundo virtual (Blur e Troca de Fundo).

Utiliza o modelo oficial do OpenCV Zoo `human_segmentation_pphumanseg_2023mar.onnx`
via `cv2.dnn` (sem dependências externas adicionais) para separar pessoa e fundo
em tempo real na webcam (ou em fotos) e aplicar:

- `nenhum`: fundo original preservado;
- `blur`: desfoque gaussiano apenas no fundo, mantendo a pessoa nítida;
- `imagem`: substituição do fundo por uma imagem escolhida pelo usuário
  (redimensionada proporcionalmente em modo cover, sem distorção).
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

import cv2
import numpy as np

ID_FUNDO_NENHUM = "nenhum"
ID_FUNDO_BLUR = "blur"
ID_FUNDO_IMAGEM = "imagem"

BLUR_PADRAO = 35
BLUR_MINIMO = 11
BLUR_MAXIMO = 75
BLUR_PASSO = 8

_CAMINHO_MODELO = (
    Path(__file__).resolve().parent
    / "assets"
    / "models"
    / "human_segmentation_pphumanseg_2023mar.onnx"
)
_TAMANHO_ENTRADA = (192, 192)
_REDE: cv2.dnn.Net | None = None


@dataclass(frozen=True)
class ModoFundo:
    """Descrição de um modo de fundo virtual."""

    id: str
    nome: str
    descricao: str


MODOS_FUNDO: tuple[ModoFundo, ...] = (
    ModoFundo(
        id=ID_FUNDO_NENHUM,
        nome="Desligado",
        descricao="Mantém o fundo original da imagem ou webcam.",
    ),
    ModoFundo(
        id=ID_FUNDO_BLUR,
        nome="Desfoque de Fundo (Blur)",
        descricao="Desfoca apenas o plano de fundo, mantendo a pessoa nítida em primeiro plano.",
    ),
    ModoFundo(
        id=ID_FUNDO_IMAGEM,
        nome="Fundo Virtual (Imagem)",
        descricao="Substitui o fundo por uma imagem escolhida do disco.",
    ),
)


def listar_modos_fundo() -> list[ModoFundo]:
    """Devolve os modos de fundo disponíveis."""
    return list(MODOS_FUNDO)


def obter_modo_fundo(id_modo: str) -> ModoFundo:
    """Devolve o modo de fundo pelo `id` ou levanta `ValueError`."""
    for modo in MODOS_FUNDO:
        if modo.id == id_modo:
            return modo
    raise ValueError(f"modo de fundo desconhecido: {id_modo!r}")


def ajustar_intensidade_blur(valor_atual: int, direcao: int) -> int:
    """Ajusta o tamanho do kernel de desfoque de fundo (sempre ímpar)."""
    novo = valor_atual + direcao * BLUR_PASSO
    novo = max(BLUR_MINIMO, min(BLUR_MAXIMO, novo))
    return novo | 1


def _obter_rede() -> cv2.dnn.Net:
    """Carrega (com cache) o modelo ONNX de segmentação de pessoas."""
    global _REDE
    if _REDE is None:
        if not _CAMINHO_MODELO.is_file():
            raise FileNotFoundError(
                f"modelo de segmentação não encontrado: {_CAMINHO_MODELO}"
            )
        _REDE = cv2.dnn.readNetFromONNX(str(_CAMINHO_MODELO))
    return _REDE


def segmentar_pessoa(imagem_bgr: np.ndarray) -> np.ndarray:
    """Retorna máscara float32 `(H, W)` em `[0.0, 1.0]` (1 = pessoa, 0 = fundo)."""
    alt, larg = imagem_bgr.shape[:2]
    rgb = cv2.cvtColor(imagem_bgr, cv2.COLOR_BGR2RGB)
    reduzida = cv2.resize(rgb, _TAMANHO_ENTRADA, interpolation=cv2.INTER_AREA)
    normalizada = (reduzida.astype(np.float32) / 255.0 - 0.5) / 0.5
    blob = np.transpose(normalizada, (2, 0, 1))[np.newaxis, ...]

    rede = _obter_rede()
    rede.setInput(blob)
    saida = rede.forward()  # shape: (1, 2, 192, 192), canal 1 = pessoa
    prob_pessoa = saida[0, 1]

    # Suaviza a transição entre 0.30 e 0.70 para bordas limpas sem vazamento
    mascara = np.clip((prob_pessoa - 0.30) / 0.40, 0.0, 1.0).astype(np.float32)
    mascara = cv2.resize(mascara, (larg, alt), interpolation=cv2.INTER_LINEAR)
    mascara = cv2.GaussianBlur(mascara, (7, 7), 0)
    return mascara


def _redimensionar_cover(fundo_bgr: np.ndarray, largura: int, altura: int) -> np.ndarray:
    """Redimensiona e recorta centralmente o fundo para preencher `(largura, altura)` sem distorção."""
    alt_f, larg_f = fundo_bgr.shape[:2]
    escala = max(largura / larg_f, altura / alt_f)
    nova_larg = max(largura, int(round(larg_f * escala)))
    nova_alt = max(altura, int(round(alt_f * escala)))
    redim = cv2.resize(fundo_bgr, (nova_larg, nova_alt), interpolation=cv2.INTER_LINEAR)
    x0 = (nova_larg - largura) // 2
    y0 = (nova_alt - altura) // 2
    return redim[y0 : y0 + altura, x0 : x0 + largura]


def aplicar_efeito_fundo(
    imagem_bgr: np.ndarray,
    id_modo: str,
    imagem_fundo_bgr: np.ndarray | None = None,
    intensidade_blur: int = BLUR_PADRAO,
    mascara: np.ndarray | None = None,
) -> tuple[np.ndarray, np.ndarray | None]:
    """Aplica blur de fundo ou substituição de fundo na imagem.

    Devolve `(imagem_composta, mascara_calculada)`.
    """
    obter_modo_fundo(id_modo)
    if id_modo == ID_FUNDO_NENHUM:
        return imagem_bgr, None
    if id_modo == ID_FUNDO_IMAGEM and imagem_fundo_bgr is None:
        return imagem_bgr, None

    if mascara is None:
        mascara = segmentar_pessoa(imagem_bgr)

    alt, larg = imagem_bgr.shape[:2]
    if id_modo == ID_FUNDO_BLUR:
        k = max(BLUR_MINIMO, intensidade_blur) | 1
        # Reduz, desfoca e volta ao tamanho original: mais suave e muito mais rápido
        pequena = cv2.resize(
            imagem_bgr, (max(1, larg // 2), max(1, alt // 2)), interpolation=cv2.INTER_AREA
        )
        k_half = max(3, (k // 2) | 1)
        desfocada = cv2.GaussianBlur(pequena, (k_half, k_half), 0)
        fundo = cv2.resize(desfocada, (larg, alt), interpolation=cv2.INTER_LINEAR)
    else:
        assert imagem_fundo_bgr is not None
        fundo = _redimensionar_cover(imagem_fundo_bgr, larg, alt)

    alpha = mascara[:, :, np.newaxis]
    composta = (
        imagem_bgr.astype(np.float32) * alpha
        + fundo.astype(np.float32) * (1.0 - alpha)
    )
    return np.clip(composta, 0, 255).astype(np.uint8), mascara
