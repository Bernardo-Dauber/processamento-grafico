"""Modos de operação do app (Modo Foto; Modo Vídeo entra em tarefa seguinte)."""

from __future__ import annotations

import cv2
import numpy as np

from grau_b.io_utils import carregar_imagem, resetar, salvar_imagem

NOME_JANELA = "Grau B - Modo Foto"
TECLA_SALVAR = ord("s")
TECLA_RESETAR = ord("r")
TECLAS_SAIR = {ord("q"), 27}  # 27 == ESC

# Tempo (ms) de espera por tecla. Um valor curto mantém a janela responsiva
# aos eventos do sistema (redesenho, botão de fechar) enquanto aguarda input.
ESPERA_TECLA_MS = 30


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


def _janela_fechada(nome_janela: str) -> bool:
    """Indica se o usuário fechou a janela pelo botão do sistema."""
    try:
        return cv2.getWindowProperty(nome_janela, cv2.WND_PROP_VISIBLE) < 1
    except cv2.error:
        return False
