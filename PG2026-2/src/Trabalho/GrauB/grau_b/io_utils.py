"""Leitura, escrita e restauração de imagens usadas pelos modos do app."""

from __future__ import annotations

import os
from datetime import datetime

import cv2
import numpy as np

SUFIXO_EDITADO = "_editado"
PREFIXO_CAPTURA = "captura"
FORMATO_CAPTURA = "png"


class ImagemInvalidaError(Exception):
    """Erro ao carregar um arquivo de imagem (inexistente ou formato inválido)."""


def carregar_imagem(caminho: str) -> np.ndarray:
    """Carrega uma imagem colorida do disco.

    Levanta ImagemInvalidaError quando o OpenCV não consegue decodificar o arquivo.
    """
    imagem = cv2.imread(caminho, cv2.IMREAD_COLOR)
    if imagem is None:
        raise ImagemInvalidaError(
            f"não foi possível carregar a imagem '{caminho}' "
            "(arquivo inexistente ou formato não suportado)"
        )
    return imagem


def caminho_com_sufixo(caminho_origem: str, sufixo: str = SUFIXO_EDITADO) -> str:
    """Monta o caminho de saída ao lado da imagem de origem, antes da extensão."""
    base, extensao = os.path.splitext(caminho_origem)
    return f"{base}{sufixo}{extensao}"


def salvar_imagem(
    imagem: np.ndarray,
    caminho_origem: str,
    sufixo: str = SUFIXO_EDITADO,
) -> str:
    """Salva a imagem ao lado do arquivo de origem e devolve o caminho gravado."""
    destino = caminho_com_sufixo(caminho_origem, sufixo)
    if not cv2.imwrite(destino, imagem):
        raise OSError(f"não foi possível salvar a imagem em '{destino}'")
    return destino


def salvar_frame(
    frame: np.ndarray,
    prefixo: str = PREFIXO_CAPTURA,
) -> str:
    """Salva um quadro de vídeo na pasta atual e devolve o caminho gravado.

    O nome inclui um carimbo de data/hora (com microssegundos) para não
    sobrescrever capturas anteriores.
    """
    carimbo = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
    destino = f"{prefixo}_{carimbo}.{FORMATO_CAPTURA}"
    if not cv2.imwrite(destino, frame):
        raise OSError(f"não foi possível salvar o quadro em '{destino}'")
    return destino


def resetar(imagem_original: np.ndarray) -> np.ndarray:
    """Devolve uma cópia da imagem original, descartando qualquer edição."""
    return imagem_original.copy()
