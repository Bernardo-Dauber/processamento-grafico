"""Ponto de entrada do Trabalho Grau B.

Uso:
    python -m grau_b.app
    python -m grau_b.app --imagem caminho/para/imagem.png
    python -m grau_b.app --video
    python -m grau_b.app --imagem caminho/para/imagem.png --video
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
    executar,
)

CODIGO_ERRO_USO = 2
CODIGO_ERRO_EXECUCAO = 1

INDICE_CAMERA_PADRAO = 0


def montar_parser() -> argparse.ArgumentParser:
    """Cria o parser de argumentos do app."""
    parser = argparse.ArgumentParser(
        prog="grau_b.app",
        description=(
            "Trabalho Grau B de Processamento Gráfico: interface gráfica Tkinter "
            "para edição de imagens e vídeo com OpenCV."
        ),
    )
    parser.add_argument(
        "--imagem",
        metavar="CAMINHO",
        help="abre a imagem indicada ao iniciar (Modo Foto).",
    )
    parser.add_argument(
        "--video",
        action="store_true",
        help="liga a webcam padrão (índice 0) ao iniciar (Modo Vídeo).",
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

    if args.imagem is not None and not os.path.isfile(args.imagem):
        print(
            f"erro: arquivo de imagem não encontrado: '{args.imagem}'",
            file=sys.stderr,
        )
        return CODIGO_ERRO_USO

    try:
        executar(
            args.imagem,
            iniciar_video=args.video,
            indice_camera=INDICE_CAMERA_PADRAO,
        )
    except ImagemInvalidaError as erro:
        print(f"erro: {erro}", file=sys.stderr)
        return CODIGO_ERRO_EXECUCAO
    except (CameraIndisponivelError, FalhaLeituraFrameError, OSError) as erro:
        print(f"erro: {erro}", file=sys.stderr)
        return CODIGO_ERRO_EXECUCAO

    return 0


if __name__ == "__main__":
    sys.exit(main())
