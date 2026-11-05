"""Ponto de entrada do Trabalho Grau B.

Uso:
    python -m grau_b.app --imagem caminho/para/imagem.png
"""

from __future__ import annotations

import argparse
import os
import sys

from grau_b import __version__
from grau_b.io_utils import ImagemInvalidaError
from grau_b.modes import modo_foto

CODIGO_ERRO_USO = 2
CODIGO_ERRO_EXECUCAO = 1


def montar_parser() -> argparse.ArgumentParser:
    """Cria o parser de argumentos do app."""
    parser = argparse.ArgumentParser(
        prog="grau_b.app",
        description=(
            "Trabalho Grau B de Processamento Gráfico: carrega uma imagem e "
            "permite editá-la em uma janela OpenCV."
        ),
    )
    parser.add_argument(
        "--imagem",
        metavar="CAMINHO",
        required=True,
        help=(
            "caminho da imagem a ser aberta no Modo Foto. "
            "Teclas na janela: 's' salva, 'r' restaura a original, 'q'/ESC sai."
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


if __name__ == "__main__":
    sys.exit(main())
