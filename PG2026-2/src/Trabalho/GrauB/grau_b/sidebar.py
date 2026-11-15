"""Barra lateral clicável desenhada ao lado da imagem (Modos Foto e Vídeo).

O painel é desenhado com primitivas do OpenCV (`rectangle`, `circle`,
`putText`) sobre um canvas BGR e anexado à direita da imagem processada. Mostra
o estado atual (canal, filtro, operação, sticker) e as teclas de atalho; cada
botão também registra uma região clicável (`Regiao`) para o callback de mouse.

A fonte do OpenCV (Hershey) não tem acentos, então os textos passam por
`_ascii` antes de serem desenhados.
"""

from __future__ import annotations

import textwrap
import unicodedata
from dataclasses import dataclass
from typing import Callable

import cv2
import numpy as np

from grau_b import teclas as t
from grau_b.filtros import ID_NENHUM, listar_filtros, obter_filtro
from grau_b.operacoes import listar_operacoes
from grau_b.stickers import aplicar_sticker, listar_stickers

LARGURA_PAINEL = 300
ALTURA_MINIMA = 700
LARGURA_MINIMA_AREA_IMAGEM = 420
ALTURA_BARRA_STATUS = 26

# Cores em BGR.
_FUNDO_PAINEL = (252, 252, 252)
_FUNDO_AREA = (245, 243, 241)
_FUNDO_BOTAO = (255, 255, 255)
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
_SIMBOLO_OPERACAO = {"soma": "+", "subtracao": "-", "multiplicacao": "x", "mescla": "%"}
_NOME_CURTO_FILTRO = {
    ID_NENHUM: "Sem filtro",
    "negativo": "Negativo",
    "cinza": "Escala de cinza",
    "blur_media": "Desfoque media",
    "blur_gaussiano": "Desfoque gauss.",
    "blur_mediana": "Desfoque mediana",
    "realce_nitidez": "Realce nitidez",
    "bordas_sobel": "Bordas Sobel",
    "bordas_canny": "Bordas Canny",
    "relevo": "Relevo",
    "binarizacao": "Binarizacao",
    "equalizacao": "Equalizacao",
    "sepia": "Sepia",
}

_FONTE = cv2.FONT_HERSHEY_SIMPLEX

#: Ação de um clique: (tipo, valor). Tipos: "canal", "filtro", "param",
#: "operacao", "segunda", "sticker" e "tecla" (valor = código da tecla).
Acao = tuple[str, object]


@dataclass(frozen=True)
class Regiao:
    """Retângulo clicável (coordenadas da janela composta) e sua ação."""

    x0: int
    y0: int
    x1: int
    y1: int
    acao: Acao

    def contem(self, x: int, y: int) -> bool:
        return self.x0 <= x < self.x1 and self.y0 <= y < self.y1


@dataclass
class EstadoPainel:
    """Dados exibidos pela barra lateral."""

    canal: str
    filtro_id: str
    nome_arquivo: str
    dica: str = ""
    valor_filtro: int | None = None
    #: Webcam ligada (stickers ficam indisponíveis, só valem na foto).
    webcam: bool = False
    tem_foto: bool = True
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


class _Painel:
    """Canvas do painel mais a lista de regiões clicáveis que ele gera."""

    def __init__(self, altura: int) -> None:
        self.img = np.full((altura, LARGURA_PAINEL, 3), _FUNDO_PAINEL, dtype=np.uint8)
        cv2.line(self.img, (0, 0), (0, altura), _BORDA, 1)
        self.regioes: list[Regiao] = []  # coordenadas locais ao painel

    def titulo(self, x: int, y: int, larg: int, titulo: str, atalhos: str = "") -> int:
        _texto(self.img, titulo.upper(), x, y, _TEXTO_MUDO, 0.38)
        if atalhos:
            _texto(
                self.img,
                atalhos,
                x + larg - _largura_texto(atalhos, 0.38),
                y,
                _TEXTO_SUAVE,
                0.38,
            )
        return y + 8

    def botao(
        self,
        x: int,
        y: int,
        larg: int,
        altura: int,
        rotulo: str,
        acao: Acao | None,
        ativo: bool = False,
        recuo: int = 8,
        icone: Callable[[np.ndarray, int, int], None] | None = None,
        escala: float = 0.4,
    ) -> None:
        fundo = _ATIVO_FUNDO if ativo else _FUNDO_BOTAO
        borda = _ATIVO_BORDA if ativo else _BORDA
        cv2.rectangle(self.img, (x, y), (x + larg, y + altura), fundo, -1)
        cv2.rectangle(self.img, (x, y), (x + larg, y + altura), borda, 1)
        if icone is not None:
            icone(self.img, x + 13, y + altura // 2)
            recuo = 26
        cor = _ATIVO_TEXTO if ativo else _TEXTO_SUAVE
        _texto(self.img, rotulo, x + recuo, y + altura // 2 + 4, cor, escala)
        if acao is not None:
            self.regioes.append(Regiao(x, y, x + larg, y + altura, acao))

    def grade(
        self,
        x: int,
        y: int,
        larg: int,
        itens: list[tuple[str, Acao, bool, Callable | None]],
        colunas: int = 2,
        altura: int = 24,
        espaco: int = 4,
        escala: float = 0.4,
    ) -> int:
        """Desenha botões em grade; o último, se sobrar sozinho, ocupa a linha."""
        largura_item = (larg - espaco * (colunas - 1)) // colunas
        for i, (rotulo, acao, ativo, icone) in enumerate(itens):
            linha, coluna = divmod(i, colunas)
            sozinho = i == len(itens) - 1 and coluna == 0 and colunas > 1
            self.botao(
                x + coluna * (largura_item + espaco),
                y + linha * (altura + espaco),
                larg if sozinho else largura_item,
                altura,
                rotulo,
                acao,
                ativo,
                icone=icone,
                escala=escala,
            )
        linhas = (len(itens) + colunas - 1) // colunas
        return y + linhas * (altura + espaco)


def _icone_canal(canal: str) -> Callable[[np.ndarray, int, int], None]:
    def desenhar(canvas: np.ndarray, cx: int, cy: int) -> None:
        if canal == "todos":
            for i, cor in enumerate(_COR_CANAL.values()):
                cv2.ellipse(
                    canvas, (cx, cy), (6, 6), 0, i * 120, (i + 1) * 120, cor, -1,
                    cv2.LINE_AA,
                )
        else:
            cv2.circle(canvas, (cx, cy), 6, _COR_CANAL[canal], -1, cv2.LINE_AA)

    return desenhar


def _icone_simbolo(simbolo: str) -> Callable[[np.ndarray, int, int], None]:
    def desenhar(canvas: np.ndarray, cx: int, cy: int) -> None:
        _texto(canvas, simbolo, cx - 4, cy + 5, _TEXTO_SUAVE, 0.5, 2)

    return desenhar


def _icone_sticker(imagem: np.ndarray) -> Callable[[np.ndarray, int, int], None]:
    miniatura = cv2.resize(imagem, (16, 16), interpolation=cv2.INTER_AREA)

    def desenhar(canvas: np.ndarray, cx: int, cy: int) -> None:
        canvas[:] = aplicar_sticker(canvas, miniatura, (cx, cy))

    return desenhar


def _secao_canal(p: _Painel, x: int, y: int, larg: int, canal: str) -> int:
    y = p.titulo(x, y, larg, "Canal", "[c]")
    itens = [
        (nome, ("canal", id_canal), id_canal == canal, _icone_canal(id_canal))
        for id_canal, nome in _NOME_CANAL.items()
    ]
    return p.grade(x, y, larg, itens)


def _secao_filtro(p: _Painel, x: int, y: int, larg: int, estado: EstadoPainel) -> int:
    y = p.titulo(x, y, larg, "Filtro", "[f] proximo  [n] limpa")
    itens = [
        (
            _NOME_CURTO_FILTRO[id_filtro],
            ("filtro", id_filtro),
            id_filtro == estado.filtro_id,
            None,
        )
        for id_filtro in [ID_NENHUM, *(f.id for f in listar_filtros())]
    ]
    y = p.grade(x, y, larg, itens, altura=22, escala=0.38)
    return _cartao_filtro(p, x, y, larg, estado) + 14


def _cartao_filtro(
    p: _Painel, x: int, y: int, larg: int, estado: EstadoPainel
) -> int:
    """Descrição do filtro atual e, se houver, o parâmetro com botões [-] [+]."""
    altura = 66
    cv2.rectangle(p.img, (x, y), (x + larg, y + altura), _FUNDO_BOTAO, -1)
    cv2.rectangle(p.img, (x, y), (x + larg, y + altura), _BORDA, 1)
    if estado.filtro_id == ID_NENHUM:
        descricao, parametro = "Imagem sem processamento.", None
    else:
        filtro = obter_filtro(estado.filtro_id)
        descricao, parametro = filtro.descricao, filtro.parametro
    for i, linha in enumerate(textwrap.wrap(_ascii(descricao), width=46)[:2]):
        _texto(p.img, linha, x + 8, y + 16 + i * 14, _TEXTO_SUAVE, 0.38)
    base = y + altura - 9
    if estado.filtro_id == ID_NENHUM:
        return y + altura
    if parametro is None:
        _texto(p.img, "Sem parametros configuraveis", x + 8, base, _TEXTO_MUDO, 0.38)
        return y + altura
    atual = parametro.padrao if estado.valor_filtro is None else estado.valor_filtro
    rotulo = f"{parametro.nome}: {atual} ({parametro.minimo}-{parametro.maximo})"
    _texto(p.img, rotulo, x + 8, base, _ATIVO_TEXTO, 0.42)
    p.botao(x + larg - 56, y + altura - 24, 24, 18, "-", ("param", -1), recuo=9, escala=0.5)
    p.botao(x + larg - 28, y + altura - 24, 24, 18, "+", ("param", +1), recuo=8, escala=0.5)
    return y + altura


def _secao_operacao(p: _Painel, x: int, y: int, larg: int, estado: EstadoPainel) -> int:
    y = p.titulo(x, y, larg, "Operacao com 2a imagem", "[o] [m]")
    itens = [
        (
            operacao.nome,
            ("operacao", operacao.id),
            operacao.id == estado.operacao_id,
            _icone_simbolo(_SIMBOLO_OPERACAO.get(operacao.id, "?")),
        )
        for operacao in listar_operacoes()
    ]
    y = p.grade(x, y, larg, itens, escala=0.38)
    rotulo = "Trocar 2a imagem" if estado.segunda_carregada else "Carregar 2a imagem"
    p.botao(x, y, larg, 22, rotulo, ("segunda", None), escala=0.4)
    return y + 22 + 14


def _secao_sticker(p: _Painel, x: int, y: int, larg: int, atual: str) -> int:
    y = p.titulo(x, y, larg, "Sticker", "[t]")
    itens = [
        (s.nome, ("sticker", s.id), s.id == atual, _icone_sticker(s.imagem))
        for s in listar_stickers()
    ]
    return p.grade(x, y, larg, itens) + 10


def _secao_acoes(p: _Painel, x: int, y: int, larg: int, estado: EstadoPainel) -> int:
    itens = [
        ("Salvar [s]", ("tecla", t.SALVAR), False, None),
        ("Restaurar [r]", ("tecla", t.RESETAR), False, None),
        (
            "Desligar webcam [v]" if estado.webcam else "Ligar webcam [v]",
            ("tecla", t.WEBCAM),
            estado.webcam,
            None,
        ),
        ("Abrir foto [p]", ("tecla", t.ABRIR_FOTO), False, None),
        ("Sair [q]", ("tecla", t.SAIR), False, None),
    ]
    return p.grade(x, y, larg, itens, escala=0.38)


def _desenhar_painel(altura: int, estado: EstadoPainel) -> _Painel:
    p = _Painel(altura)
    x, larg = 12, LARGURA_PAINEL - 24
    y = 22
    y = _secao_canal(p, x, y, larg, estado.canal) + 10
    y = _secao_filtro(p, x, y + 6, larg, estado)
    y = _secao_operacao(p, x, y + 6, larg, estado)
    if not estado.webcam:
        y = _secao_sticker(p, x, y + 6, larg, estado.sticker_id)
    _secao_acoes(p, x, y + 6, larg, estado)
    return p


def compor_janela(
    imagem: np.ndarray, estado: EstadoPainel
) -> tuple[np.ndarray, list[Regiao]]:
    """Devolve a janela composta e as regiões clicáveis do painel.

    A imagem fica no canto superior esquerdo (origem em (0, 0)), então cliques
    com `x < largura` e `y < altura` continuam mapeando para ela. As regiões
    vêm em coordenadas da janela composta.
    """
    alt_img, larg_img = imagem.shape[:2]
    altura = max(alt_img + ALTURA_BARRA_STATUS, ALTURA_MINIMA)
    largura_area = max(larg_img, LARGURA_MINIMA_AREA_IMAGEM)

    area = np.full((altura, largura_area, 3), _FUNDO_AREA, dtype=np.uint8)
    area[:alt_img, :larg_img] = imagem

    y_barra = altura - ALTURA_BARRA_STATUS
    cv2.rectangle(area, (0, y_barra), (largura_area, altura), _FUNDO_BOTAO, -1)
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

    painel = _desenhar_painel(altura, estado)
    regioes = [
        Regiao(
            r.x0 + largura_area, r.y0, r.x1 + largura_area, r.y1, r.acao
        )
        for r in painel.regioes
    ]
    return np.hstack([area, painel.img]), regioes
