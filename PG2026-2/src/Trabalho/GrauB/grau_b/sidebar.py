"""Barra lateral de comandos desenhada ao lado da imagem (Modos Foto e Vídeo).

O painel é desenhado com primitivas do OpenCV (`rectangle`, `circle`,
`putText`) sobre um canvas BGR e anexado à direita da imagem processada. Mostra
o estado atual (canal, filtro, operação, sticker) e as teclas de atalho, em
listas com nomes por extenso.

A fonte do OpenCV (Hershey) não tem acentos, então os textos passam por
`_ascii` antes de serem desenhados.
"""

from __future__ import annotations

import textwrap
import unicodedata
from dataclasses import dataclass

import cv2
import numpy as np

from grau_b.filtros import ID_NENHUM, listar_filtros, obter_filtro
from grau_b.operacoes import listar_operacoes
from grau_b.stickers import aplicar_sticker, listar_stickers

LARGURA_PAINEL = 260
ALTURA_MINIMA = 610
LARGURA_MINIMA_AREA_IMAGEM = 420
ALTURA_BARRA_STATUS = 26

# Cores em BGR.
_FUNDO_PAINEL = (252, 252, 252)
_FUNDO_AREA = (245, 243, 241)
_FUNDO_STATUS = (255, 255, 255)
_BORDA = (214, 214, 214)
_TEXTO = (40, 40, 40)
_TEXTO_SUAVE = (105, 105, 105)
_TEXTO_MUDO = (150, 150, 150)
_ATIVO_FUNDO = (251, 238, 225)
_ATIVO_BORDA = (205, 140, 55)
_ATIVO_TEXTO = (140, 80, 20)
_COR_CANAL = {"R": (74, 75, 226), "G": (34, 153, 99), "B": (221, 138, 55)}
_NOME_CANAL = {
    "todos": "Imagem completa",
    "R": "Canal vermelho",
    "G": "Canal verde",
    "B": "Canal azul",
}
_SIMBOLO_OPERACAO = {
    "soma": "+",
    "subtracao": "-",
    "multiplicacao": "x",
    "mescla": "%",
}

_FONTE = cv2.FONT_HERSHEY_SIMPLEX


@dataclass
class EstadoPainel:
    """Dados exibidos pela barra lateral."""

    canal: str
    filtro_id: str
    nome_arquivo: str
    dica: str = ""
    modo_foto: bool = True
    operacao_id: str | None = None
    segunda_carregada: bool = False
    sticker_id: str = ""


def _ascii(texto: str) -> str:
    """Remove acentos (a fonte Hershey não os desenha)."""
    base = unicodedata.normalize("NFKD", texto)
    return "".join(c for c in base if not unicodedata.combining(c))


def _texto(
    canvas: np.ndarray,
    texto: str,
    x: int,
    y: int,
    cor: tuple[int, int, int] = _TEXTO,
    escala: float = 0.45,
    espessura: int = 1,
) -> None:
    cv2.putText(
        canvas, _ascii(texto), (x, y), _FONTE, escala, cor, espessura, cv2.LINE_AA
    )


def _largura_texto(texto: str, escala: float = 0.45) -> int:
    return cv2.getTextSize(_ascii(texto), _FONTE, escala, 1)[0][0]


def _titulo_secao(
    canvas: np.ndarray, x: int, y: int, largura: int, titulo: str, atalhos: str
) -> int:
    """Desenha o título da seção com os atalhos à direita; devolve o novo y."""
    _texto(canvas, titulo.upper(), x, y, _TEXTO_MUDO, 0.38)
    if atalhos:
        _texto(
            canvas,
            atalhos,
            x + largura - _largura_texto(atalhos, 0.38),
            y,
            _TEXTO_SUAVE,
            0.38,
        )
    return y + 8


def _linha(
    canvas: np.ndarray, x: int, y: int, largura: int, ativo: bool, altura: int = 24
) -> tuple[int, int]:
    """Desenha o fundo de uma linha selecionável; devolve (x_texto, y_base)."""
    fundo = _ATIVO_FUNDO if ativo else _FUNDO_STATUS
    borda = _ATIVO_BORDA if ativo else _BORDA
    cv2.rectangle(canvas, (x, y), (x + largura, y + altura), fundo, -1)
    cv2.rectangle(canvas, (x, y), (x + largura, y + altura), borda, 1)
    return x + 28, y + altura - 8


def _bolinha_canal(canvas: np.ndarray, cx: int, cy: int, canal: str) -> None:
    if canal == "todos":
        for i, cor in enumerate(_COR_CANAL.values()):
            cv2.ellipse(
                canvas, (cx, cy), (6, 6), 0, i * 120, (i + 1) * 120, cor, -1, cv2.LINE_AA
            )
    else:
        cv2.circle(canvas, (cx, cy), 6, _COR_CANAL[canal], -1, cv2.LINE_AA)


def _secao_canal(canvas: np.ndarray, x: int, y: int, larg: int, canal: str) -> int:
    y = _titulo_secao(canvas, x, y, larg, "Canal", "[c]")
    for id_canal, nome in _NOME_CANAL.items():
        ativo = id_canal == canal
        tx, ty = _linha(canvas, x, y, larg, ativo)
        _bolinha_canal(canvas, x + 14, y + 12, id_canal)
        _texto(canvas, nome, tx, ty, _ATIVO_TEXTO if ativo else _TEXTO_SUAVE)
        y += 28
    return y


def _secao_filtro(canvas: np.ndarray, x: int, y: int, larg: int, filtro_id: str) -> int:
    y = _titulo_secao(canvas, x, y, larg, "Filtro", "[f] proximo  [n] limpa")
    altura = 78
    cv2.rectangle(canvas, (x, y), (x + larg, y + altura), _FUNDO_STATUS, -1)
    cv2.rectangle(canvas, (x, y), (x + larg, y + altura), _BORDA, 1)
    if filtro_id == ID_NENHUM:
        nome, posicao, descricao = "Sem filtro", "", "Imagem sem processamento."
    else:
        filtros = listar_filtros()
        filtro = obter_filtro(filtro_id)
        indice = [f.id for f in filtros].index(filtro_id) + 1
        nome, posicao = filtro.nome, f"{indice} de {len(filtros)}"
        descricao = filtro.descricao
    _texto(canvas, nome, x + 8, y + 18, _TEXTO, 0.47)
    if posicao:
        _texto(
            canvas,
            posicao,
            x + larg - 8 - _largura_texto(posicao, 0.4),
            y + 18,
            _TEXTO_MUDO,
            0.4,
        )
    linhas = textwrap.wrap(_ascii(descricao), width=36)[:3]
    for i, linha in enumerate(linhas):
        _texto(canvas, linha, x + 8, y + 36 + i * 15, _TEXTO_SUAVE, 0.38)
    return y + altura + 14


def _secao_operacao(
    canvas: np.ndarray, x: int, y: int, larg: int, estado: EstadoPainel
) -> int:
    y = _titulo_secao(canvas, x, y, larg, "Operacao com 2a imagem", "[o] [m]")
    for operacao in listar_operacoes():
        ativo = operacao.id == estado.operacao_id
        tx, ty = _linha(canvas, x, y, larg, ativo)
        cor = _ATIVO_TEXTO if ativo else _TEXTO_SUAVE
        _texto(canvas, _SIMBOLO_OPERACAO.get(operacao.id, "?"), x + 10, ty, cor, 0.5, 2)
        _texto(canvas, operacao.nome, tx, ty, cor)
        y += 28
    if not estado.segunda_carregada:
        _texto(canvas, "Pressione [o] e informe a 2a imagem", x, y + 6, _TEXTO_MUDO, 0.37)
        y += 12
    return y + 14


def _secao_sticker(canvas: np.ndarray, x: int, y: int, larg: int, atual: str) -> int:
    y = _titulo_secao(canvas, x, y, larg, "Sticker", "[t]")
    meia = (larg - 4) // 2
    for i, sticker in enumerate(listar_stickers()):
        coluna, linha_idx = i % 2, i // 2
        # O último sticker (ímpar) ocupa a linha inteira.
        ultimo_sozinho = i == len(listar_stickers()) - 1 and i % 2 == 0
        largura_item = larg if ultimo_sozinho else meia
        px = x + coluna * (meia + 4)
        py = y + linha_idx * 28
        ativo = sticker.id == atual
        _linha(canvas, px, py, largura_item, ativo)
        miniatura = cv2.resize(sticker.imagem, (18, 18), interpolation=cv2.INTER_AREA)
        canvas[:] = aplicar_sticker(canvas, miniatura, (px + 14, py + 12))
        cor = _ATIVO_TEXTO if ativo else _TEXTO_SUAVE
        _texto(canvas, sticker.nome, px + 28, py + 16, cor, 0.42)
    linhas = (len(listar_stickers()) + 1) // 2
    return y + linhas * 28 + 10


def _secao_acoes(canvas: np.ndarray, x: int, y: int, larg: int, modo_foto: bool) -> int:
    cv2.line(canvas, (x, y - 6), (x + larg, y - 6), _BORDA, 1)
    acoes = [("Salvar", "s")]
    if modo_foto:
        acoes.append(("Restaurar original", "r"))
    acoes.append(("Sair", "q / ESC"))
    for nome, tecla in acoes:
        _texto(canvas, nome, x, y + 10, _TEXTO_SUAVE)
        rotulo = f"[{tecla}]"
        _texto(canvas, rotulo, x + larg - _largura_texto(rotulo), y + 10, _TEXTO_SUAVE)
        y += 22
    return y


def _desenhar_painel(altura: int, estado: EstadoPainel) -> np.ndarray:
    painel = np.full((altura, LARGURA_PAINEL, 3), _FUNDO_PAINEL, dtype=np.uint8)
    cv2.line(painel, (0, 0), (0, altura), _BORDA, 1)
    x, larg = 12, LARGURA_PAINEL - 24
    y = 22
    y = _secao_canal(painel, x, y, larg, estado.canal) + 14
    y = _secao_filtro(painel, x, y + 6, larg, estado.filtro_id)
    if estado.modo_foto:
        y = _secao_operacao(painel, x, y + 6, larg, estado)
        y = _secao_sticker(painel, x, y + 6, larg, estado.sticker_id)
    _secao_acoes(painel, x, y + 14, larg, estado.modo_foto)
    return painel


def compor_janela(imagem: np.ndarray, estado: EstadoPainel) -> np.ndarray:
    """Devolve a imagem no canto superior esquerdo + barra de status + painel.

    A imagem mantém suas coordenadas originais (origem em (0, 0)), então
    cliques com `x < largura` e `y < altura` continuam mapeando para ela.
    """
    alt_img, larg_img = imagem.shape[:2]
    altura = max(alt_img + ALTURA_BARRA_STATUS, ALTURA_MINIMA)
    largura_area = max(larg_img, LARGURA_MINIMA_AREA_IMAGEM)

    area = np.full((altura, largura_area, 3), _FUNDO_AREA, dtype=np.uint8)
    area[:alt_img, :larg_img] = imagem

    y_barra = altura - ALTURA_BARRA_STATUS
    cv2.rectangle(area, (0, y_barra), (largura_area, altura), _FUNDO_STATUS, -1)
    cv2.line(area, (0, y_barra), (largura_area, y_barra), _BORDA, 1)
    _texto(area, estado.nome_arquivo, 10, altura - 9, _TEXTO_SUAVE, 0.42)
    if estado.dica:
        _texto(
            area,
            estado.dica,
            largura_area - 10 - _largura_texto(estado.dica, 0.42),
            altura - 9,
            _TEXTO_SUAVE,
            0.42,
        )

    return np.hstack([area, _desenhar_painel(altura, estado)])
