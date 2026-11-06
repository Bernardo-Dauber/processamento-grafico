"""Modos de operação do app: Modo Foto (imagem) e Modo Vídeo (webcam)."""

from __future__ import annotations

import cv2
import numpy as np

from grau_b.io_utils import carregar_imagem, resetar, salvar_frame, salvar_imagem

NOME_JANELA = "Grau B - Modo Foto"
NOME_JANELA_VIDEO = "Grau B - Modo Vídeo"
TECLA_SALVAR = ord("s")
TECLA_RESETAR = ord("r")
TECLAS_SAIR = {ord("q"), 27}  # 27 == ESC

# Tempo (ms) de espera por tecla. Um valor curto mantém a janela responsiva
# aos eventos do sistema (redesenho, botão de fechar) enquanto aguarda input.
ESPERA_TECLA_MS = 30


class CameraIndisponivelError(Exception):
    """Erro ao abrir o dispositivo de captura de vídeo."""


class FalhaLeituraFrameError(Exception):
    """Erro ao ler um quadro do dispositivo de captura de vídeo."""


def modo_foto(caminho_imagem: str) -> None:
    """Abre a janela do Modo Foto e processa o teclado até o usuário sair.

    Teclas: `s` salva a imagem atual, `r` restaura a original, `q`/ESC encerra.
    """
    imagem_original = carregar_imagem(caminho_imagem)
    imagem_atual = resetar(imagem_original)

    try:
        while True:
            cv2.imshow(NOME_JANELA, imagem_atual)
            tecla = cv2.waitKey(ESPERA_TECLA_MS) & 0xFF

            if tecla in TECLAS_SAIR:
                break
            if tecla == TECLA_SALVAR:
                destino = salvar_imagem(imagem_atual, caminho_imagem)
                print(f"Imagem salva em: {destino}")
            elif tecla == TECLA_RESETAR:
                imagem_atual = resetar(imagem_original)
                print("Imagem restaurada ao original.")

            if _janela_fechada(nome_janela=NOME_JANELA):
                break
    finally:
        cv2.destroyAllWindows()


def modo_video(indice_camera: int = 0) -> None:
    """Abre a janela do Modo Vídeo e exibe a webcam ao vivo até o usuário sair.

    Teclas: `s` salva o quadro atual, `q`/ESC encerra. A tecla `r` (restaurar
    original) não se aplica ao vídeo e é ignorada.

    Levanta CameraIndisponivelError quando a câmera não pode ser aberta e
    FalhaLeituraFrameError quando um quadro não pode ser lido.
    """
    captura = cv2.VideoCapture(indice_camera)
    if not captura.isOpened():
        captura.release()
        raise CameraIndisponivelError(
            f"não foi possível abrir a webcam (índice {indice_camera}); "
            "verifique se o dispositivo existe, está conectado e não está "
            "em uso por outro programa"
        )

    try:
        while True:
            lido, frame = captura.read()
            if not lido or frame is None:
                raise FalhaLeituraFrameError(
                    f"falha ao ler um quadro da webcam (índice {indice_camera}); "
                    "a captura foi interrompida"
                )

            cv2.imshow(NOME_JANELA_VIDEO, frame)
            tecla = cv2.waitKey(ESPERA_TECLA_MS) & 0xFF

            if tecla in TECLAS_SAIR:
                break
            if tecla == TECLA_SALVAR:
                destino = salvar_frame(frame)
                print(f"Quadro salvo em: {destino}")
            # TECLA_RESETAR ('r') é no-op no Modo Vídeo: não há imagem original.

            if _janela_fechada(nome_janela=NOME_JANELA_VIDEO):
                break
    finally:
        captura.release()
        cv2.destroyAllWindows()


def _janela_fechada(nome_janela: str) -> bool:
    """Indica se o usuário fechou a janela pelo botão do sistema."""
    try:
        return cv2.getWindowProperty(nome_janela, cv2.WND_PROP_VISIBLE) < 1
    except cv2.error:
        return False
