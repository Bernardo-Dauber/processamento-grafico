"""Modos de operação do app: Modo Foto (imagem) e Modo Vídeo (webcam)."""

from __future__ import annotations

import cv2
import numpy as np

from grau_b.canais import CANAL_PADRAO, aplicar_selecao_canal, proximo_canal
from grau_b.filtros import ID_NENHUM, aplicar_filtro, obter_filtro, proximo_filtro
from grau_b.io_utils import carregar_imagem, resetar, salvar_frame, salvar_imagem
from grau_b.stickers import (
    STICKER_PADRAO,
    aplicar_sticker,
    obter_sticker,
    proximo_sticker,
)

NOME_JANELA = "Grau B - Modo Foto"
NOME_JANELA_VIDEO = "Grau B - Modo Vídeo"
TECLA_SALVAR = ord("s")
TECLA_RESETAR = ord("r")
TECLA_PROXIMO_FILTRO = ord("f")
TECLA_PROXIMO_CANAL = ord("c")
TECLA_SEM_FILTRO = ord("n")
TECLA_PROXIMO_STICKER = ord("t")
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

    Teclas: `s` salva a imagem exibida, `r` restaura a original (zerando
    filtro, canal e stickers posicionados), `f` avança para o próximo filtro
    (imprimindo a descrição no terminal antes de aplicá-lo), `c` avança o canal
    (`todos → R → G → B → todos`), `n` volta a "sem filtro", `t` avança para o
    próximo sticker (imprimindo o nome no terminal) e `q`/ESC encerra.

    O pipeline aplicado a cada quadro é: seleção de canal, o filtro atual e,
    por fim, os stickers já posicionados. Um clique com o botão esquerdo do
    mouse fixa o sticker atual, centrado na posição clicada, sobre a imagem de
    trabalho; cliques sucessivos acumulam stickers (a tecla `r` remove todos).
    """
    imagem_original = carregar_imagem(caminho_imagem)
    imagem_atual = resetar(imagem_original)
    filtro_atual = ID_NENHUM
    canal_atual = CANAL_PADRAO
    sticker_atual = STICKER_PADRAO
    # Cada item é (id do sticker, x, y) na ordem em que foi clicado.
    stickers_posicionados: list[tuple[str, int, int]] = []

    def ao_clicar_mouse(
        evento: int,
        x: int,
        y: int,
        _flags: int,
        _param: object,
    ) -> None:
        """Fixa o sticker atual no ponto clicado (centro do sticker)."""
        if evento == cv2.EVENT_LBUTTONDOWN:
            stickers_posicionados.append((sticker_atual, x, y))
            nome = obter_sticker(sticker_atual).nome
            print(f"Sticker '{nome}' fixado em ({x}, {y}).")

    callback_registrado = False
    try:
        while True:
            imagem_exibida = _processar_quadro(imagem_atual, filtro_atual, canal_atual)
            imagem_exibida = _aplicar_stickers(imagem_exibida, stickers_posicionados)
            cv2.imshow(NOME_JANELA, imagem_exibida)
            if not callback_registrado:
                cv2.setMouseCallback(NOME_JANELA, ao_clicar_mouse)
                callback_registrado = True
            tecla = cv2.waitKey(ESPERA_TECLA_MS) & 0xFF

            if tecla in TECLAS_SAIR:
                break
            if tecla == TECLA_SALVAR:
                destino = salvar_imagem(imagem_exibida, caminho_imagem)
                print(f"Imagem salva em: {destino}")
            elif tecla == TECLA_RESETAR:
                imagem_atual = resetar(imagem_original)
                filtro_atual = ID_NENHUM
                canal_atual = CANAL_PADRAO
                stickers_posicionados.clear()
                print(
                    "Imagem restaurada ao original "
                    "(filtro, canal e stickers zerados)."
                )
            elif tecla == TECLA_PROXIMO_FILTRO:
                filtro_atual = proximo_filtro(filtro_atual)
                _imprimir_filtro(filtro_atual)
            elif tecla == TECLA_PROXIMO_CANAL:
                canal_atual = proximo_canal(canal_atual)
                print(f"Canal selecionado: {canal_atual}")
            elif tecla == TECLA_SEM_FILTRO:
                filtro_atual = ID_NENHUM
                _imprimir_filtro(filtro_atual)
            elif tecla == TECLA_PROXIMO_STICKER:
                sticker_atual = proximo_sticker(sticker_atual)
                print(f"Sticker selecionado: {obter_sticker(sticker_atual).nome}")

            if _janela_fechada(nome_janela=NOME_JANELA):
                break
    finally:
        cv2.destroyAllWindows()


def modo_video(indice_camera: int = 0) -> None:
    """Abre a janela do Modo Vídeo e exibe a webcam ao vivo até o usuário sair.

    Teclas: `s` salva o quadro exibido, `f` avança para o próximo filtro
    (imprimindo a descrição no terminal antes de aplicá-lo), `c` avança o canal
    (`todos → R → G → B → todos`), `n` volta a "sem filtro", `q`/ESC encerra.
    A tecla `r` (restaurar original) não se aplica ao vídeo e é ignorada.

    O pipeline aplicado a cada quadro é: seleção de canal e, em seguida, o
    filtro atual.

    Levanta CameraIndisponivelError quando a câmera não pode ser aberta e
    FalhaLeituraFrameError quando um quadro não pode ser lido.
    """
    filtro_atual = ID_NENHUM
    canal_atual = CANAL_PADRAO

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

            imagem_exibida = _processar_quadro(frame, filtro_atual, canal_atual)
            cv2.imshow(NOME_JANELA_VIDEO, imagem_exibida)
            tecla = cv2.waitKey(ESPERA_TECLA_MS) & 0xFF

            if tecla in TECLAS_SAIR:
                break
            if tecla == TECLA_SALVAR:
                destino = salvar_frame(imagem_exibida)
                print(f"Quadro salvo em: {destino}")
            # TECLA_RESETAR ('r') é no-op no Modo Vídeo: não há imagem original.
            elif tecla == TECLA_PROXIMO_FILTRO:
                filtro_atual = proximo_filtro(filtro_atual)
                _imprimir_filtro(filtro_atual)
            elif tecla == TECLA_PROXIMO_CANAL:
                canal_atual = proximo_canal(canal_atual)
                print(f"Canal selecionado: {canal_atual}")
            elif tecla == TECLA_SEM_FILTRO:
                filtro_atual = ID_NENHUM
                _imprimir_filtro(filtro_atual)

            if _janela_fechada(nome_janela=NOME_JANELA_VIDEO):
                break
    finally:
        captura.release()
        cv2.destroyAllWindows()


def _processar_quadro(
    imagem: np.ndarray,
    filtro_id: str,
    canal: str,
) -> np.ndarray:
    """Aplica o pipeline de exibição: seleção de canal e, depois, o filtro."""
    imagem_canal = aplicar_selecao_canal(imagem, canal)
    return aplicar_filtro(filtro_id, imagem_canal)


def _aplicar_stickers(
    imagem: np.ndarray,
    stickers_posicionados: list[tuple[str, int, int]],
) -> np.ndarray:
    """Aplica, na ordem dos cliques, todos os stickers posicionados."""
    resultado = imagem
    for id_sticker, x, y in stickers_posicionados:
        sticker = obter_sticker(id_sticker)
        resultado = aplicar_sticker(resultado, sticker.imagem, (x, y))
    return resultado


def _imprimir_filtro(filtro_id: str) -> None:
    """Imprime no terminal o filtro atual e a descrição do que ele faz."""
    if filtro_id == ID_NENHUM:
        print("Filtro: nenhum (imagem sem processamento).")
        return

    filtro = obter_filtro(filtro_id)
    print(f"Filtro: {filtro.nome} - {filtro.descricao}")


def _janela_fechada(nome_janela: str) -> bool:
    """Indica se o usuário fechou a janela pelo botão do sistema."""
    try:
        return cv2.getWindowProperty(nome_janela, cv2.WND_PROP_VISIBLE) < 1
    except cv2.error:
        return False
