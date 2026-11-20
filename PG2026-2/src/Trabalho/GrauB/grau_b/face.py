"""Detecção facial com OpenCV Haar Cascades e efeitos localizados no rosto.

Utiliza o classificador oficial `haarcascade_frontalface_default.xml` empacotado
em `grau_b/assets/cascades/` para detectar faces em fotos e quadros de vídeo e
aplicar:

- `gatinho`: orelhas de gato acima da testa + focinho e bigodes no centro da face;
- `oculos`: óculos escuros posicionados na região dos olhos;
- `blur_rosto`: desfoque gaussiano de privacidade restrito à região do rosto.

Opcionalmente desenha as caixas delimitadoras das faces detectadas.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

import cv2
import numpy as np

from grau_b.stickers import aplicar_sticker

ID_EFEITO_NENHUM = "nenhum"
_CAMINHO_CASCADE = (
    Path(__file__).resolve().parent
    / "assets"
    / "cascades"
    / "haarcascade_frontalface_default.xml"
)


@dataclass(frozen=True)
class EfeitoFacial:
    """Descrição de um efeito aplicado sobre faces detectadas."""

    id: str
    nome: str
    descricao: str


EFEITOS_FACIAIS: tuple[EfeitoFacial, ...] = (
    EfeitoFacial(
        id=ID_EFEITO_NENHUM,
        nome="Desligado",
        descricao="Nenhum filtro ou adesivo automático de detecção facial.",
    ),
    EfeitoFacial(
        id="gatinho",
        nome="Filtro de Gatinho",
        descricao="Posiciona orelhas de gato, focinho e bigodes automaticamente em cada rosto.",
    ),
    EfeitoFacial(
        id="oculos",
        nome="Óculos Escuros",
        descricao="Posiciona óculos escuros estilizados na altura dos olhos de cada rosto.",
    ),
    EfeitoFacial(
        id="blur_rosto",
        nome="Desfoque no Rosto",
        descricao="Aplica desfoque gaussiano de privacidade apenas na região de cada rosto detectado.",
    ),
)

_CLASSIFICADOR: cv2.CascadeClassifier | None = None


def listar_efeitos_faciais() -> list[EfeitoFacial]:
    """Devolve os efeitos faciais disponíveis."""
    return list(EFEITOS_FACIAIS)


def obter_efeito_facial(id_efeito: str) -> EfeitoFacial:
    """Devolve o efeito facial pelo `id` ou levanta `ValueError`."""
    for efeito in EFEITOS_FACIAIS:
        if efeito.id == id_efeito:
            return efeito
    raise ValueError(f"efeito facial desconhecido: {id_efeito!r}")


def _obter_classificador() -> cv2.CascadeClassifier:
    """Carrega (com cache) o classificador Haar Cascade frontal."""
    global _CLASSIFICADOR
    if _CLASSIFICADOR is None:
        if not _CAMINHO_CASCADE.is_file():
            raise FileNotFoundError(
                f"arquivo Haar Cascade não encontrado: {_CAMINHO_CASCADE}"
            )
        clf = cv2.CascadeClassifier(str(_CAMINHO_CASCADE))
        if clf.empty():
            raise RuntimeError(
                f"não foi possível carregar o Haar Cascade: {_CAMINHO_CASCADE}"
            )
        _CLASSIFICADOR = clf
    return _CLASSIFICADOR


def detectar_faces(imagem_bgr: np.ndarray) -> list[tuple[int, int, int, int]]:
    """Detecta rostos frontais e devolve uma lista de retângulos `(x, y, w, h)`."""
    cinza = cv2.cvtColor(imagem_bgr, cv2.COLOR_BGR2GRAY)
    cinza = cv2.equalizeHist(cinza)
    faces = _obter_classificador().detectMultiScale(
        cinza,
        scaleFactor=1.15,
        minNeighbors=5,
        minSize=(40, 40),
    )
    if len(faces) == 0:
        return []
    return [(int(x), int(y), int(w), int(h)) for (x, y, w, h) in faces]


def _gerar_orelhas_gato(largura: int, altura: int) -> np.ndarray:
    """Gera imagem RGBA de duas orelhas de gato proporcionais ao rosto."""
    w = max(20, largura)
    h = max(16, altura)
    rgb = np.zeros((h, w, 3), dtype=np.uint8)
    alpha = np.zeros((h, w), dtype=np.uint8)

    # Orelha esquerda (externa cinza escuro, interna rosa)
    ext_esq = np.array(
        [[int(w * 0.05), h - 2], [int(w * 0.18), int(h * 0.06)], [int(w * 0.40), h - 2]],
        dtype=np.int32,
    )
    int_esq = np.array(
        [[int(w * 0.12), h - 4], [int(w * 0.20), int(h * 0.26)], [int(w * 0.33), h - 4]],
        dtype=np.int32,
    )
    # Orelha direita
    ext_dir = np.array(
        [[int(w * 0.60), h - 2], [int(w * 0.82), int(h * 0.06)], [int(w * 0.95), h - 2]],
        dtype=np.int32,
    )
    int_dir = np.array(
        [[int(w * 0.67), h - 4], [int(w * 0.80), int(h * 0.26)], [int(w * 0.88), h - 4]],
        dtype=np.int32,
    )

    cor_orelha = (55, 55, 65)
    cor_interna = (255, 160, 185)
    for ext, interno in ((ext_esq, int_esq), (ext_dir, int_dir)):
        cv2.fillPoly(rgb, [ext], cor_orelha)
        cv2.fillPoly(alpha, [ext], 255)
        cv2.fillPoly(rgb, [interno], cor_interna)

    rgba = np.zeros((h, w, 4), dtype=np.uint8)
    rgba[:, :, :3] = rgb
    rgba[:, :, 3] = alpha
    return rgba


def _gerar_focinho_bigodes(largura: int, altura: int) -> np.ndarray:
    """Gera imagem RGBA de focinho rosa e bigodes de gato."""
    w = max(24, largura)
    h = max(16, altura)
    rgb = np.zeros((h, w, 3), dtype=np.uint8)
    alpha = np.zeros((h, w), dtype=np.uint8)

    cx, cy = w // 2, h // 2
    nariz_w = max(4, int(w * 0.09))
    nariz_h = max(4, int(h * 0.22))
    nariz = np.array(
        [
            [cx - nariz_w, cy - nariz_h // 2],
            [cx + nariz_w, cy - nariz_h // 2],
            [cx, cy + nariz_h],
        ],
        dtype=np.int32,
    )
    cv2.fillPoly(rgb, [nariz], (255, 130, 170))
    cv2.fillPoly(alpha, [nariz], 255)

    esp = max(2, w // 70)
    cor_bigode = (35, 35, 40)
    # 3 bigodes de cada lado
    for dy_ini, dy_fim in ((-int(h * 0.12), -int(h * 0.28)), (0, 0), (int(h * 0.12), int(h * 0.28))):
        p1_esq = (cx - int(w * 0.14), cy + dy_ini)
        p2_esq = (int(w * 0.04), cy + dy_fim)
        p1_dir = (cx + int(w * 0.14), cy + dy_ini)
        p2_dir = (int(w * 0.96), cy + dy_fim)
        cv2.line(rgb, p1_esq, p2_esq, cor_bigode, esp, cv2.LINE_AA)
        cv2.line(alpha, p1_esq, p2_esq, 255, esp, cv2.LINE_AA)
        cv2.line(rgb, p1_dir, p2_dir, cor_bigode, esp, cv2.LINE_AA)
        cv2.line(alpha, p1_dir, p2_dir, 255, esp, cv2.LINE_AA)

    rgba = np.zeros((h, w, 4), dtype=np.uint8)
    rgba[:, :, :3] = rgb
    rgba[:, :, 3] = alpha
    return rgba


def _gerar_oculos(largura: int, altura: int) -> np.ndarray:
    """Gera imagem RGBA de óculos escuros estilizados."""
    w = max(24, largura)
    h = max(12, altura)
    rgb = np.zeros((h, w, 3), dtype=np.uint8)
    alpha = np.zeros((h, w), dtype=np.uint8)

    cor_armacao = (25, 25, 30)
    cor_lente = (35, 45, 65)
    cor_brilho = (195, 220, 245)

    # Lente esquerda e direita
    x1_l, x2_l = int(w * 0.06), int(w * 0.44)
    x1_r, x2_r = int(w * 0.56), int(w * 0.94)
    y1, y2 = int(h * 0.12), int(h * 0.88)
    esp = max(2, h // 12)

    for xa, xb in ((x1_l, x2_l), (x1_r, x2_r)):
        cv2.rectangle(rgb, (xa, y1), (xb, y2), cor_lente, -1)
        cv2.rectangle(alpha, (xa, y1), (xb, y2), 235, -1)
        cv2.rectangle(rgb, (xa, y1), (xb, y2), cor_armacao, esp)
        cv2.rectangle(alpha, (xa, y1), (xb, y2), 255, esp)
        # Reflexo diagonal na lente
        cv2.line(
            rgb,
            (xa + int((xb - xa) * 0.20), y1 + esp + 2),
            (xa + int((xb - xa) * 0.45), y2 - esp - 2),
            cor_brilho,
            max(1, esp // 2),
            cv2.LINE_AA,
        )

    # Ponte central e hastes laterais
    y_ponte = int(h * 0.36)
    cv2.line(rgb, (x2_l, y_ponte), (x1_r, y_ponte), cor_armacao, esp + 1)
    cv2.line(alpha, (x2_l, y_ponte), (x1_r, y_ponte), 255, esp + 1)
    cv2.line(rgb, (0, y_ponte), (x1_l, y_ponte), cor_armacao, esp)
    cv2.line(alpha, (0, y_ponte), (x1_l, y_ponte), 255, esp)
    cv2.line(rgb, (x2_r, y_ponte), (w - 1, y_ponte), cor_armacao, esp)
    cv2.line(alpha, (x2_r, y_ponte), (w - 1, y_ponte), 255, esp)

    rgba = np.zeros((h, w, 4), dtype=np.uint8)
    rgba[:, :, :3] = rgb
    rgba[:, :, 3] = alpha
    return rgba


def _aplicar_blur_rosto(imagem: np.ndarray, face: tuple[int, int, int, int]) -> np.ndarray:
    """Aplica desfoque gaussiano forte na região do rosto."""
    x, y, w, h = face
    alt, larg = imagem.shape[:2]
    x0, y0 = max(0, x), max(0, y)
    x1, y1 = min(larg, x + w), min(alt, y + h)
    if x0 >= x1 or y0 >= y1:
        return imagem
    roi = imagem[y0:y1, x0:x1]
    k = max(15, (min(w, h) // 4) | 1)
    imagem[y0:y1, x0:x1] = cv2.GaussianBlur(roi, (k, k), 0)
    return imagem


def aplicar_efeito_facial(
    imagem_bgr: np.ndarray,
    id_efeito: str,
    mostrar_caixas: bool = False,
    faces: list[tuple[int, int, int, int]] | None = None,
) -> tuple[np.ndarray, list[tuple[int, int, int, int]]]:
    """Detecta faces (se necessário) e aplica o efeito facial selecionado.

    Devolve `(imagem_resultante, faces_detectadas)`.
    """
    obter_efeito_facial(id_efeito)
    if id_efeito == ID_EFEITO_NENHUM and not mostrar_caixas:
        return imagem_bgr, []

    if faces is None:
        faces = detectar_faces(imagem_bgr)

    if not faces:
        return imagem_bgr, []

    resultado = imagem_bgr.copy()
    for (x, y, w, h) in faces:
        if id_efeito == "blur_rosto":
            resultado = _aplicar_blur_rosto(resultado, (x, y, w, h))
        elif id_efeito == "gatinho":
            orelhas = _gerar_orelhas_gato(int(w * 1.10), int(h * 0.45))
            resultado = aplicar_sticker(
                resultado, orelhas, (x + w // 2, y - int(h * 0.16))
            )
            focinho = _gerar_focinho_bigodes(int(w * 0.95), int(h * 0.34))
            resultado = aplicar_sticker(
                resultado, focinho, (x + w // 2, y + int(h * 0.62))
            )
        elif id_efeito == "oculos":
            oculos = _gerar_oculos(int(w * 0.96), int(h * 0.30))
            resultado = aplicar_sticker(
                resultado, oculos, (x + w // 2, y + int(h * 0.38))
            )

        if mostrar_caixas:
            cv2.rectangle(resultado, (x, y), (x + w, y + h), (40, 220, 90), 2)
            cv2.putText(
                resultado,
                "Rosto",
                (x, max(16, y - 6)),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.55,
                (40, 220, 90),
                2,
                cv2.LINE_AA,
            )

    return resultado, faces
