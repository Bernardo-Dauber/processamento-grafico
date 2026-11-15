"""Loop principal do app: uma janela única que alterna entre foto e webcam.

A fonte exibida é a foto carregada do disco ou, quando a webcam está ligada, o
quadro ao vivo. Filtros, canal, operações entre duas imagens e o reset valem
para as duas fontes; os stickers só para a foto (como o enunciado permite).
"""

from __future__ import annotations

import os
from dataclasses import dataclass, field

import cv2
import numpy as np

from grau_b.canais import CANAL_PADRAO, aplicar_selecao_canal, proximo_canal
from grau_b.filtros import (
    ID_NENHUM,
    ajustar_parametro,
    aplicar_filtro,
    descrever_filtro,
    obter_filtro,
    proximo_filtro,
)
from grau_b.io_utils import (
    ImagemInvalidaError,
    carregar_imagem,
    salvar_frame,
    salvar_imagem,
)
from grau_b.operacoes import aplicar_operacao, obter_operacao, proxima_operacao
from grau_b.sidebar import EstadoPainel, compor_janela
from grau_b.stickers import (
    STICKER_PADRAO,
    aplicar_sticker,
    obter_sticker,
    proximo_sticker,
)

NOME_JANELA = "Grau B - Processamento de Imagem e Vídeo"
TECLA_SALVAR = ord("s")
TECLA_RESETAR = ord("r")
TECLA_PROXIMO_FILTRO = ord("f")
TECLA_PROXIMO_CANAL = ord("c")
TECLA_SEM_FILTRO = ord("n")
TECLA_PROXIMO_STICKER = ord("t")
TECLA_SEGUNDA_IMAGEM = ord("o")
TECLA_PROXIMA_OPERACAO = ord("m")
TECLA_WEBCAM = ord("v")
TECLA_ABRIR_FOTO = ord("p")
TECLAS_AUMENTAR = {ord("+"), ord("=")}
TECLAS_DIMINUIR = {ord("-"), ord("_")}
TECLAS_SAIR = {ord("q"), 27}  # 27 == ESC

# Tempo (ms) de espera por tecla. Um valor curto mantém a janela responsiva
# aos eventos do sistema (redesenho, botão de fechar) enquanto aguarda input.
ESPERA_TECLA_MS = 30

DICA_FOTO = "Clique na imagem para posicionar o sticker"
DICA_WEBCAM = "Webcam ligada"


class CameraIndisponivelError(Exception):
    """Erro ao abrir o dispositivo de captura de vídeo."""


class FalhaLeituraFrameError(Exception):
    """Erro ao ler um quadro do dispositivo de captura de vídeo."""


@dataclass
class Sessao:
    """Estado mutável do app durante a execução."""

    indice_camera: int = 0
    foto: np.ndarray | None = None
    caminho_foto: str | None = None
    captura: cv2.VideoCapture | None = None
    segunda: np.ndarray | None = None
    filtro: str = ID_NENHUM
    #: Valor do parâmetro de cada filtro (ausente = valor padrão do filtro).
    valores: dict[str, int] = field(default_factory=dict)
    canal: str = CANAL_PADRAO
    operacao: str | None = None
    sticker: str = STICKER_PADRAO
    #: Cada item é (id do sticker, x, y) na ordem em que foi clicado.
    stickers: list[tuple[str, int, int]] = field(default_factory=list)
    dica: str = ""
    #: Resultado em cache da operação sobre a foto: (op, foto, segunda, saída).
    cache_operacao: tuple[str, np.ndarray, np.ndarray, np.ndarray] | None = None

    @property
    def webcam_ligada(self) -> bool:
        return self.captura is not None

    @property
    def valor_filtro(self) -> int | None:
        return self.valores.get(self.filtro)

    def resetar(self) -> None:
        """Volta ao estado original: sem filtro, canal, operação e stickers."""
        self.filtro = ID_NENHUM
        self.valores.clear()
        self.canal = CANAL_PADRAO
        self.operacao = None
        self.stickers.clear()
        self.cache_operacao = None


def executar(
    caminho_imagem: str | None = None,
    iniciar_video: bool = False,
    indice_camera: int = 0,
) -> None:
    """Abre a janela e processa o teclado/mouse até o usuário sair.

    Abre na foto (`caminho_imagem`) e, se `iniciar_video` for verdadeiro ou não
    houver foto, já com a webcam ligada. Teclas: `v` liga/desliga a webcam, `p`
    carrega outra foto, `f` próximo filtro, `n` sem filtro, `+`/`-` ajustam o
    parâmetro do filtro, `c` canal, `o` carrega a 2ª imagem, `m` próxima
    operação, `t` próximo sticker (clique posiciona, só na foto), `s` salva,
    `r` reseta tudo e `q`/ESC encerra.

    O pipeline por quadro é: operação (se houver) → canal → filtro → stickers
    (só na foto). Levanta `ImagemInvalidaError` se a foto não carregar,
    `CameraIndisponivelError` se não houver foto e a webcam não abrir, e
    `FalhaLeituraFrameError` se a webcam parar sem foto para voltar.
    """
    sessao = Sessao(indice_camera=indice_camera)
    if caminho_imagem is not None:
        sessao.foto = carregar_imagem(caminho_imagem)
        sessao.caminho_foto = caminho_imagem

    if iniciar_video or sessao.foto is None:
        try:
            _ligar_webcam(sessao)
        except CameraIndisponivelError as erro:
            if sessao.foto is None:
                raise
            print(f"aviso: {erro}")
            sessao.dica = "Webcam indisponivel"

    def ao_clicar_mouse(
        evento: int, x: int, y: int, _flags: int, _param: object
    ) -> None:
        """Fixa o sticker atual no ponto clicado (só na foto, dentro dela)."""
        if evento != cv2.EVENT_LBUTTONDOWN or sessao.webcam_ligada:
            return
        if sessao.foto is None:
            return
        altura, largura = sessao.foto.shape[:2]
        if x < largura and y < altura:
            sessao.stickers.append((sessao.sticker, x, y))
            nome = obter_sticker(sessao.sticker).nome
            print(f"Sticker '{nome}' fixado em ({x}, {y}).")
            sessao.dica = f"Sticker '{nome}' fixado"

    callback_registrado = False
    try:
        while True:
            base = _obter_base(sessao)
            if base is None:  # webcam falhou e voltamos para a foto
                continue
            exibida = _processar(sessao, base)
            if not sessao.webcam_ligada:
                exibida = _aplicar_stickers(exibida, sessao.stickers)
            cv2.imshow(NOME_JANELA, compor_janela(exibida, _estado_painel(sessao)))
            if not callback_registrado:
                cv2.setMouseCallback(NOME_JANELA, ao_clicar_mouse)
                callback_registrado = True
            tecla = cv2.waitKey(ESPERA_TECLA_MS) & 0xFF

            if tecla in TECLAS_SAIR:
                break
            _tratar_tecla(tecla, sessao, exibida)

            if _janela_fechada(NOME_JANELA):
                break
    finally:
        _desligar_webcam(sessao)
        cv2.destroyAllWindows()


def _obter_base(sessao: Sessao) -> np.ndarray | None:
    """Devolve a imagem de entrada do pipeline: quadro da webcam ou a foto.

    Devolve `None` quando a webcam falhou e o app voltou para a foto.
    """
    if sessao.captura is None:
        assert sessao.foto is not None
        return sessao.foto

    lido, quadro = sessao.captura.read()
    if lido and quadro is not None:
        return quadro

    mensagem = (
        f"falha ao ler um quadro da webcam (índice {sessao.indice_camera}); "
        "a captura foi interrompida"
    )
    if sessao.foto is None:
        raise FalhaLeituraFrameError(mensagem)
    print(f"aviso: {mensagem}")
    _desligar_webcam(sessao)
    sessao.dica = "Webcam falhou; voltou para a foto"
    return None


def _tratar_tecla(tecla: int, sessao: Sessao, exibida: np.ndarray) -> None:
    """Executa a ação da tecla pressionada sobre a sessão."""
    if tecla == TECLA_SALVAR:
        _salvar(sessao, exibida)
    elif tecla == TECLA_RESETAR:
        sessao.resetar()
        sessao.dica = "Restaurado: filtro, canal, operacao e stickers"
        print("Restaurado ao original (filtro, canal, operação e stickers zerados).")
    elif tecla == TECLA_WEBCAM:
        _alternar_webcam(sessao)
    elif tecla == TECLA_ABRIR_FOTO:
        imagem, caminho = _pedir_imagem("Caminho da foto")
        if imagem is not None:
            sessao.foto, sessao.caminho_foto = imagem, caminho
            sessao.stickers.clear()
            sessao.cache_operacao = None
            sessao.dica = "Foto carregada"
    elif tecla == TECLA_SEGUNDA_IMAGEM:
        imagem, _ = _pedir_imagem("Caminho da segunda imagem")
        if imagem is not None:
            sessao.segunda = imagem
            sessao.cache_operacao = None
            sessao.dica = "2a imagem carregada: use [m]"
            print("Pressione 'm' para aplicar uma operação.")
    elif tecla == TECLA_PROXIMA_OPERACAO:
        if sessao.segunda is None:
            sessao.dica = "Carregue a 2a imagem com [o]"
            print(
                "Nenhuma segunda imagem carregada; pressione 'o' e informe o "
                "caminho antes de aplicar uma operação."
            )
        else:
            sessao.operacao = proxima_operacao(sessao.operacao)
            _imprimir_operacao(sessao.operacao)
    elif tecla == TECLA_PROXIMO_FILTRO:
        sessao.filtro = proximo_filtro(sessao.filtro)
        _imprimir_filtro(sessao)
    elif tecla == TECLA_SEM_FILTRO:
        sessao.filtro = ID_NENHUM
        _imprimir_filtro(sessao)
    elif tecla in TECLAS_AUMENTAR or tecla in TECLAS_DIMINUIR:
        _ajustar_parametro(sessao, +1 if tecla in TECLAS_AUMENTAR else -1)
    elif tecla == TECLA_PROXIMO_CANAL:
        sessao.canal = proximo_canal(sessao.canal)
        print(f"Canal selecionado: {sessao.canal}")
    elif tecla == TECLA_PROXIMO_STICKER:
        sessao.sticker = proximo_sticker(sessao.sticker)
        print(f"Sticker selecionado: {obter_sticker(sessao.sticker).nome}")


def _processar(sessao: Sessao, base: np.ndarray) -> np.ndarray:
    """Pipeline de exibição: operação (se houver) → canal → filtro."""
    imagem = base
    if sessao.operacao is not None and sessao.segunda is not None:
        imagem = _aplicar_operacao(sessao, base)
    return _processar_quadro(imagem, sessao.filtro, sessao.canal, sessao.valor_filtro)


def _aplicar_operacao(sessao: Sessao, base: np.ndarray) -> np.ndarray:
    """Calcula a operação entre `base` e a 2ª imagem (com cache para a foto)."""
    assert sessao.operacao is not None and sessao.segunda is not None
    eh_foto = base is sessao.foto
    cache = sessao.cache_operacao
    if (
        eh_foto
        and cache is not None
        and cache[0] == sessao.operacao
        and cache[1] is base
        and cache[2] is sessao.segunda
    ):
        return cache[3]
    resultado = aplicar_operacao(sessao.operacao, base, sessao.segunda)
    if eh_foto:
        sessao.cache_operacao = (sessao.operacao, base, sessao.segunda, resultado)
    return resultado


def _processar_quadro(
    imagem: np.ndarray,
    filtro_id: str,
    canal: str,
    valor_filtro: int | None = None,
) -> np.ndarray:
    """Aplica seleção de canal e, depois, o filtro (com seu parâmetro)."""
    imagem_canal = aplicar_selecao_canal(imagem, canal)
    return aplicar_filtro(filtro_id, imagem_canal, valor_filtro)


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


def _estado_painel(sessao: Sessao) -> EstadoPainel:
    if sessao.webcam_ligada:
        nome_arquivo = f"Webcam (indice {sessao.indice_camera})"
    else:
        nome_arquivo = os.path.basename(sessao.caminho_foto or "")
    padrao = DICA_WEBCAM if sessao.webcam_ligada else DICA_FOTO
    return EstadoPainel(
        canal=sessao.canal,
        filtro_id=sessao.filtro,
        valor_filtro=sessao.valor_filtro,
        nome_arquivo=nome_arquivo,
        dica=sessao.dica or padrao,
        webcam=sessao.webcam_ligada,
        tem_foto=sessao.foto is not None,
        operacao_id=sessao.operacao,
        segunda_carregada=sessao.segunda is not None,
        sticker_id=sessao.sticker,
    )


def _salvar(sessao: Sessao, exibida: np.ndarray) -> None:
    if sessao.webcam_ligada:
        destino = salvar_frame(exibida)
        print(f"Quadro salvo em: {destino}")
    else:
        assert sessao.caminho_foto is not None
        destino = salvar_imagem(exibida, sessao.caminho_foto)
        print(f"Imagem salva em: {destino}")
    sessao.dica = f"Salvo em {os.path.basename(str(destino))}"


def _ligar_webcam(sessao: Sessao) -> None:
    captura = cv2.VideoCapture(sessao.indice_camera)
    if not captura.isOpened():
        captura.release()
        raise CameraIndisponivelError(
            f"não foi possível abrir a webcam (índice {sessao.indice_camera}); "
            "verifique se o dispositivo existe, está conectado e não está "
            "em uso por outro programa"
        )
    sessao.captura = captura


def _desligar_webcam(sessao: Sessao) -> None:
    if sessao.captura is not None:
        sessao.captura.release()
        sessao.captura = None


def _alternar_webcam(sessao: Sessao) -> None:
    """Liga a webcam se estiver desligada e vice-versa (`v`)."""
    sessao.dica = ""
    if sessao.webcam_ligada:
        if sessao.foto is None:
            sessao.dica = "Carregue uma foto com [p] antes de desligar"
            print("Não há foto carregada; pressione 'p' para abrir uma.")
            return
        _desligar_webcam(sessao)
        print("Webcam desligada.")
        return
    try:
        _ligar_webcam(sessao)
    except CameraIndisponivelError as erro:
        print(f"erro: {erro}")
        sessao.dica = "Webcam indisponivel"
        return
    print("Webcam ligada.")


def _ajustar_parametro(sessao: Sessao, direcao: int) -> None:
    novo = ajustar_parametro(sessao.filtro, sessao.valor_filtro, direcao)
    if novo is None:
        sessao.dica = "Este filtro nao tem parametros"
        print("Este filtro não tem parâmetros configuráveis.")
        return
    sessao.valores[sessao.filtro] = novo
    parametro = obter_filtro(sessao.filtro).parametro
    assert parametro is not None
    sessao.dica = f"{parametro.nome}: {novo}"
    print(f"Parâmetro {parametro.nome}: {novo}")


def _imprimir_filtro(sessao: Sessao) -> None:
    """Imprime o filtro atual, a descrição e seus parâmetros configuráveis."""
    if sessao.filtro == ID_NENHUM:
        print("Filtro: nenhum (imagem sem processamento).")
        return
    nome = obter_filtro(sessao.filtro).nome
    print(f"Filtro: {nome} - {descrever_filtro(sessao.filtro, sessao.valor_filtro)}")


def _imprimir_operacao(operacao_id: str) -> None:
    """Imprime no terminal a operação atual e a descrição do que ela faz."""
    operacao = obter_operacao(operacao_id)
    print(f"Operação: {operacao.nome} - {operacao.descricao}")


def _pedir_imagem(rotulo: str) -> tuple[np.ndarray | None, str | None]:
    """Pede um caminho no terminal e carrega a imagem.

    Devolve `(None, None)` (sem levantar exceção) quando nenhum caminho é
    informado, a leitura é interrompida ou a imagem não carrega; o estado
    anterior é preservado. A janela fica parada enquanto o `input()` espera.
    """
    try:
        caminho = input(f"{rotulo}: ").strip()
    except EOFError:
        caminho = ""

    if not caminho:
        print("Nenhum caminho informado; nada foi alterado.")
        return None, None

    try:
        imagem = carregar_imagem(caminho)
    except ImagemInvalidaError as erro:
        print(f"Erro ao carregar a imagem: {erro}")
        return None, None

    print(f"Imagem carregada: {caminho} ({imagem.shape[1]}x{imagem.shape[0]}).")
    return imagem, caminho


def _janela_fechada(nome_janela: str) -> bool:
    """Indica se o usuário fechou a janela pelo botão do sistema."""
    try:
        return cv2.getWindowProperty(nome_janela, cv2.WND_PROP_VISIBLE) < 1
    except cv2.error:
        return False
