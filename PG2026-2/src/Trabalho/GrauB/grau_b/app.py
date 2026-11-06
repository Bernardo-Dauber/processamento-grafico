"""Ponto de entrada do Trabalho Grau B.

Uso:
    python -m grau_b.app --imagem caminho/para/imagem.png
    python -m grau_b.app --video
"""

from __future__ import annotations

import argparse
import os
import sys

from grau_b import __version__
from grau_b.io_utils import ImagemInvalidaError
from grau_b.modes import (
    CameraIndisponivelError,
    FalhaLeituraFrameError,
    modo_foto,
    modo_video,
)

CODIGO_ERRO_USO = 2
CODIGO_ERRO_EXECUCAO = 1

INDICE_CAMERA_PADRAO = 0


def montar_parser() -> argparse.ArgumentParser:
    """Cria o parser de argumentos do app."""
    parser = argparse.ArgumentParser(
        prog="grau_b.app",
        description=(
            "Trabalho Grau B de Processamento Gráfico: modos Foto (imagem) e "
            "Vídeo (webcam) em janelas OpenCV."
        ),
    )
    grupo_modos = parser.add_mutually_exclusive_group(required=True)
    grupo_modos.add_argument(
        "--imagem",
        metavar="CAMINHO",
        help=(
            "abre a imagem em Modo Foto. "
            "Teclas na janela: 's' salva, 'r' restaura a original, 'q'/ESC sai."
        ),
    )
    grupo_modos.add_argument(
        "--video",
        action="store_true",
        help=(
            "abre a webcam padrão (índice 0) em Modo Vídeo. "
            "Teclas na janela: 's' salva o quadro atual, 'q'/ESC sai."
        ),
    )
    parser.add_argument(
        "--version",
        action="version",
        version=f"%(prog)s {__version__}",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    """Executa o app e devolve o código de saída."""
    parser = montar_parser()
    args = parser.parse_args(argv)

    if args.video:
        return _executar_modo_video()

    if not os.path.isfile(args.imagem):
        print(
            f"erro: arquivo de imagem não encontrado: '{args.imagem}'",
            file=sys.stderr,
        )
        return CODIGO_ERRO_USO

    try:
        modo_foto(args.imagem)
    except ImagemInvalidaError as erro:
        print(f"erro: {erro}", file=sys.stderr)
        return CODIGO_ERRO_EXECUCAO

    return 0


def _executar_modo_video() -> int:
    """Executa o Modo Vídeo e traduz falhas de captura em código de erro."""
    try:
        modo_video(indice_camera=INDICE_CAMERA_PADRAO)
    except (CameraIndisponivelError, FalhaLeituraFrameError) as erro:
        print(f"erro: {erro}", file=sys.stderr)
        return CODIGO_ERRO_EXECUCAO
    except OSError as erro:
        print(f"erro: {erro}", file=sys.stderr)
        return CODIGO_ERRO_EXECUCAO

    return 0


if __name__ == "__main__":
    sys.exit(main())
