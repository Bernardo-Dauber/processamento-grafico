"""Orquestração da execução do app (delegando para a GUI em Tkinter)."""

from __future__ import annotations

from grau_b.gui import (
    CameraIndisponivelError,
    FalhaLeituraFrameError,
    executar_gui,
)

__all__ = [
    "CameraIndisponivelError",
    "FalhaLeituraFrameError",
    "executar",
]


def executar(
    caminho_imagem: str | None = None,
    iniciar_video: bool = False,
    indice_camera: int = 0,
) -> None:
    """Abre a interface gráfica Tkinter nos modos Foto e/ou Vídeo."""
    executar_gui(
        caminho_imagem=caminho_imagem,
        iniciar_video=iniciar_video,
        indice_camera=indice_camera,
    )
