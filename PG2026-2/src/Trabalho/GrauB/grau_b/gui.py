"""Interface gráfica em Tkinter + Pillow para o Trabalho Grau B.

Reúne em uma única janela todos os requisitos obrigatórios e extras:
- Modo Foto e Modo Vídeo (webcam) alternáveis sem reiniciar o app;
- Diálogos nativos do sistema para abrir foto, carregar 2ª imagem e salvar;
- Seleção de canal (Todos, R, G, B);
- Combinação de múltiplos filtros em sequência (pipeline editável), exibindo
  descrição e parâmetros configuráveis antes de adicionar cada filtro;
- Operações aritméticas entre duas imagens;
- Detecção facial via OpenCV Haar Cascades (Gatinho, Óculos, Desfoque no Rosto
  e caixa de detecção opcional);
- Stickers estáticos e stickers animados por spritesheet (.png RGBA) em Foto e
  Vídeo, posicionados com clique do mouse.
"""

from __future__ import annotations

import os
import time
import tkinter as tk
from datetime import datetime
from tkinter import filedialog, messagebox, ttk

import cv2
import numpy as np
from PIL import Image, ImageTk

from grau_b.canais import CANAIS, CANAL_PADRAO, aplicar_selecao_canal
from grau_b.face import (
    ID_EFEITO_NENHUM,
    aplicar_efeito_facial,
    listar_efeitos_faciais,
    obter_efeito_facial,
)
from grau_b.fundo import (
    BLUR_MAXIMO,
    BLUR_MINIMO,
    BLUR_PADRAO,
    ID_FUNDO_BLUR,
    ID_FUNDO_IMAGEM,
    ID_FUNDO_NENHUM,
    ajustar_intensidade_blur,
    aplicar_efeito_fundo,
    listar_modos_fundo,
    obter_modo_fundo,
)
from grau_b.filtros import (
    EtapaFiltro,
    ajustar_parametro,
    aplicar_pipeline_filtros,
    descrever_filtro,
    listar_filtros,
    obter_filtro,
    valor_padrao,
)
from grau_b.io_utils import (
    FORMATO_CAPTURA,
    PREFIXO_CAPTURA,
    ImagemInvalidaError,
    caminho_com_sufixo,
    carregar_imagem,
)
from grau_b.operacoes import aplicar_operacao, listar_operacoes, obter_operacao
from grau_b.stickers import (
    STICKER_PADRAO,
    aplicar_sticker,
    listar_stickers,
    obter_quadro_sticker,
    obter_sticker,
)

TITULO_JANELA = "Grau B — Processamento de Imagens e Vídeo (OpenCV + Tkinter)"
INTERVALO_MS = 33  # ~30 FPS
LARGURA_PAINEL = 365
TIPOS_IMAGEM = [
    ("Imagens", "*.png *.jpg *.jpeg *.bmp *.webp *.tif *.tiff"),
    ("Todos os arquivos", "*.*"),
]


class CameraIndisponivelError(Exception):
    """Erro ao abrir o dispositivo de captura de vídeo."""


class FalhaLeituraFrameError(Exception):
    """Erro ao ler um quadro do dispositivo de captura de vídeo."""


class AplicacaoGrauB:
    """Controlador da janela principal Tkinter e do pipeline de processamento."""

    def __init__(
        self,
        root: tk.Tk,
        caminho_imagem: str | None = None,
        iniciar_video: bool = False,
        indice_camera: int = 0,
    ) -> None:
        self.root = root
        self.root.title(TITULO_JANELA)
        self.root.geometry("1220x780")
        self.root.minsize(920, 620)

        # Estado de entrada (foto / webcam / 2ª imagem)
        self.indice_camera = indice_camera
        self.foto: np.ndarray | None = None
        self.caminho_foto: str | None = None
        self.captura: cv2.VideoCapture | None = None
        self.segunda: np.ndarray | None = None
        self.caminho_segunda: str | None = None

        # Estado do pipeline
        self.canal_var = tk.StringVar(value=CANAL_PADRAO)
        self.etapas_filtros: list[EtapaFiltro] = []
        self.operacao_id: str | None = None
        self.modo_fundo_id: str = ID_FUNDO_NENHUM
        self.intensidade_blur_fundo: int = BLUR_PADRAO
        self.imagem_fundo: np.ndarray | None = None
        self.caminho_fundo: str | None = None
        self.efeito_facial_id: str = ID_EFEITO_NENHUM
        self.mostrar_caixas_var = tk.BooleanVar(value=False)
        self.sticker_atual_id: str = STICKER_PADRAO
        # Lista de (id_sticker, x_orig, y_orig)
        self.stickers_posicionados: list[tuple[str, int, int]] = []

        # Estado auxiliar de renderização e cache
        self._inicio_tempo = time.monotonic()
        self._imagem_processada_sem_stickers: np.ndarray | None = None
        self._imagem_exibida_bgr: np.ndarray | None = None
        self._photo_canvas: ImageTk.PhotoImage | None = None
        self._photo_preview_sticker: ImageTk.PhotoImage | None = None
        self._sujo = True
        self._faces_ultima_contagem = 0
        self._frame_count = 0
        self._ultimas_faces: list[tuple[int, int, int, int]] = []
        # Mapeamento do canvas -> imagem original
        self._render_offset_x = 0
        self._render_offset_y = 0
        self._render_escala = 1.0
        self._render_larg = 0
        self._render_alt = 0

        # Catálogos
        self._filtros = listar_filtros()
        self._operacoes = listar_operacoes()
        self._modos_fundo = listar_modos_fundo()
        self._efeitos_faciais = listar_efeitos_faciais()
        self._stickers = listar_stickers()

        # Estado do seletor de novo filtro (antes de aplicar)
        self._filtro_selecionado_id = self._filtros[0].id
        self._valor_novo_filtro: int | None = valor_padrao(self._filtro_selecionado_id)

        self._construir_interface()
        self._configurar_atalhos()

        # Carregamento inicial (CLI)
        if caminho_imagem is not None:
            self.foto = carregar_imagem(caminho_imagem)
            self.caminho_foto = caminho_imagem
            self._atualizar_status(f"Foto carregada: {os.path.basename(caminho_imagem)}")

        if iniciar_video:
            try:
                self._ligar_webcam()
            except CameraIndisponivelError as erro:
                if self.foto is None:
                    raise
                self._atualizar_status(f"Aviso: {erro}")

        self._atualizar_preview_filtro_selecionado()
        self._atualizar_botoes_estado()
        self.root.protocol("WM_DELETE_WINDOW", self._ao_fechar)
        self.root.lift()
        self.root.attributes("-topmost", True)
        self.root.after(250, lambda: self.root.attributes("-topmost", False))
        self.root.focus_force()
        self._loop_tick()

    # ------------------------------------------------------------------
    # Construção da Interface Tkinter
    # ------------------------------------------------------------------
    def _construir_interface(self) -> None:
        estilo = ttk.Style(self.root)
        for tema in ("vista", "clam", "default"):
            if tema in estilo.theme_names():
                estilo.theme_use(tema)
                break

        self.root.columnconfigure(0, weight=1)
        self.root.columnconfigure(1, weight=0)
        self.root.rowconfigure(0, weight=1)
        self.root.rowconfigure(1, weight=0)

        # Área principal da imagem (esquerda)
        frame_canvas = ttk.Frame(self.root)
        frame_canvas.grid(row=0, column=0, sticky="nsew", padx=(8, 4), pady=(8, 4))
        frame_canvas.columnconfigure(0, weight=1)
        frame_canvas.rowconfigure(0, weight=1)

        self.canvas = tk.Canvas(
            frame_canvas,
            bg="#1e1e24",
            highlightthickness=1,
            highlightbackground="#3a3a44",
            cursor="crosshair",
        )
        self.canvas.grid(row=0, column=0, sticky="nsew")
        self.canvas.bind("<Button-1>", self._ao_clicar_canvas)
        self.canvas.bind("<Configure>", lambda _e: self._marcar_sujo())

        # Painel lateral com rolagem (direita)
        container_painel = ttk.Frame(self.root, width=LARGURA_PAINEL)
        container_painel.grid(row=0, column=1, sticky="ns", padx=(4, 8), pady=(8, 4))
        container_painel.grid_propagate(False)
        container_painel.rowconfigure(0, weight=1)
        container_painel.columnconfigure(0, weight=1)

        self._canvas_painel = tk.Canvas(
            container_painel, highlightthickness=0, width=LARGURA_PAINEL - 18
        )
        scrollbar = ttk.Scrollbar(
            container_painel, orient="vertical", command=self._canvas_painel.yview
        )
        self._frame_painel = ttk.Frame(self._canvas_painel)
        self._frame_painel.bind(
            "<Configure>",
            lambda _e: self._canvas_painel.configure(
                scrollregion=self._canvas_painel.bbox("all")
            ),
        )
        self._janela_painel = self._canvas_painel.create_window(
            (0, 0), window=self._frame_painel, anchor="nw", width=LARGURA_PAINEL - 20
        )
        self._canvas_painel.configure(yscrollcommand=scrollbar.set)
        self._canvas_painel.grid(row=0, column=0, sticky="nsew")
        scrollbar.grid(row=0, column=1, sticky="ns")

        # Rolagem com roda do mouse no painel
        self._canvas_painel.bind_all(
            "<MouseWheel>",
            lambda e: self._canvas_painel.yview_scroll(int(-1 * (e.delta / 120)), "units")
            if self._mouse_sobre_painel(e)
            else None,
        )

        self._construir_secao_arquivo(self._frame_painel)
        self._construir_secao_fundo(self._frame_painel)
        self._construir_secao_canais(self._frame_painel)
        self._construir_secao_filtros(self._frame_painel)
        self._construir_secao_operacoes(self._frame_painel)
        self._construir_secao_face(self._frame_painel)
        self._construir_secao_stickers(self._frame_painel)

        # Barra de status inferior
        barra_status = ttk.Frame(self.root, relief="sunken", padding=(8, 4))
        barra_status.grid(row=1, column=0, columnspan=2, sticky="ew")
        barra_status.columnconfigure(0, weight=1)
        self.status_fonte_var = tk.StringVar(value="Nenhuma imagem carregada")
        self.status_dica_var = tk.StringVar(
            value="Abra uma foto ou ligue a webcam para começar"
        )
        ttk.Label(barra_status, textvariable=self.status_fonte_var).grid(
            row=0, column=0, sticky="w"
        )
        ttk.Label(barra_status, textvariable=self.status_dica_var).grid(
            row=0, column=1, sticky="e"
        )

    def _mouse_sobre_painel(self, evento: tk.Event) -> bool:
        try:
            x_painel = self._canvas_painel.winfo_rootx()
            return evento.x_root >= x_painel
        except tk.TclError:
            return False

    def _construir_secao_arquivo(self, pai: ttk.Frame) -> None:
        sec = ttk.LabelFrame(pai, text="Fonte e Ações", padding=8)
        sec.pack(fill="x", padx=4, pady=(0, 6))
        sec.columnconfigure(0, weight=1)
        sec.columnconfigure(1, weight=1)

        ttk.Button(sec, text="Abrir Foto...", command=self._abrir_foto_dialogo).grid(
            row=0, column=0, sticky="ew", padx=(0, 3), pady=2
        )
        self.btn_webcam = ttk.Button(
            sec, text="Ligar Webcam", command=self._alternar_webcam
        )
        self.btn_webcam.grid(row=0, column=1, sticky="ew", padx=(3, 0), pady=2)

        self.btn_salvar = ttk.Button(sec, text="Salvar...", command=self._salvar_dialogo)
        self.btn_salvar.grid(row=1, column=0, sticky="ew", padx=(0, 3), pady=2)

        ttk.Button(sec, text="Restaurar Tudo", command=self._resetar_tudo).grid(
            row=1, column=1, sticky="ew", padx=(3, 0), pady=2
        )

    def _construir_secao_fundo(self, pai: ttk.Frame) -> None:
        sec = ttk.LabelFrame(pai, text="Fundo da Webcam (Blur / Virtual)", padding=8)
        sec.pack(fill="x", padx=4, pady=(0, 6))
        sec.columnconfigure(0, weight=1)

        self.combo_fundo_var = tk.StringVar(value=self._modos_fundo[0].nome)
        self.combo_fundo = ttk.Combobox(
            sec,
            textvariable=self.combo_fundo_var,
            values=[m.nome for m in self._modos_fundo],
            state="readonly",
        )
        self.combo_fundo.grid(row=0, column=0, sticky="ew", pady=(0, 4))
        self.combo_fundo.bind("<<ComboboxSelected>>", self._ao_selecionar_modo_fundo)

        frame_blur = ttk.Frame(sec)
        frame_blur.grid(row=1, column=0, sticky="ew", pady=(0, 4))
        frame_blur.columnconfigure(0, weight=1)
        self.lbl_blur_fundo = ttk.Label(
            frame_blur,
            text=f"Intensidade do blur: {self.intensidade_blur_fundo} ({BLUR_MINIMO}–{BLUR_MAXIMO})",
        )
        self.lbl_blur_fundo.grid(row=0, column=0, sticky="w")
        self.btn_blur_menos = ttk.Button(
            frame_blur, text="-", width=3, command=lambda: self._ajustar_blur_fundo(-1)
        )
        self.btn_blur_menos.grid(row=0, column=1, padx=2)
        self.btn_blur_mais = ttk.Button(
            frame_blur, text="+", width=3, command=lambda: self._ajustar_blur_fundo(+1)
        )
        self.btn_blur_mais.grid(row=0, column=2, padx=2)

        ttk.Button(
            sec, text="Escolher Imagem de Fundo...", command=self._escolher_imagem_fundo
        ).grid(row=2, column=0, sticky="ew", pady=(0, 3))

        self.lbl_imagem_fundo = ttk.Label(
            sec, text="Imagem de fundo: nenhuma", foreground="#555555"
        )
        self.lbl_imagem_fundo.grid(row=3, column=0, sticky="w")
        self._atualizar_controles_fundo()

    def _construir_secao_canais(self, pai: ttk.Frame) -> None:
        sec = ttk.LabelFrame(pai, text="Seleção de Canal", padding=8)
        sec.pack(fill="x", padx=4, pady=(0, 6))

        rotulos = {"todos": "Todos", "R": "Canal R", "G": "Canal G", "B": "Canal B"}
        for idx, canal in enumerate(CANAIS):
            ttk.Radiobutton(
                sec,
                text=rotulos[canal],
                value=canal,
                variable=self.canal_var,
                command=self._ao_mudar_canal,
            ).grid(row=0, column=idx, sticky="w", padx=4)

    def _construir_secao_filtros(self, pai: ttk.Frame) -> None:
        sec = ttk.LabelFrame(pai, text="Filtros (Combinação em Sequência)", padding=8)
        sec.pack(fill="x", padx=4, pady=(0, 6))
        sec.columnconfigure(0, weight=1)

        ttk.Label(sec, text="Escolher filtro (leia a descrição antes de adicionar):").grid(
            row=0, column=0, sticky="w"
        )
        self._combo_filtro_var = tk.StringVar(value=self._filtros[0].nome)
        self.combo_filtro = ttk.Combobox(
            sec,
            textvariable=self._combo_filtro_var,
            values=[f.nome for f in self._filtros],
            state="readonly",
        )
        self.combo_filtro.grid(row=1, column=0, sticky="ew", pady=(2, 4))
        self.combo_filtro.bind("<<ComboboxSelected>>", self._ao_selecionar_filtro_combo)

        # Cartão de descrição antes de aplicar
        self.lbl_desc_filtro = ttk.Label(
            sec,
            text="",
            wraplength=LARGURA_PAINEL - 54,
            justify="left",
            foreground="#333333",
        )
        self.lbl_desc_filtro.grid(row=2, column=0, sticky="ew", pady=(0, 4))

        # Controle de parâmetro do filtro selecionado (antes de adicionar)
        frame_param = ttk.Frame(sec)
        frame_param.grid(row=3, column=0, sticky="ew", pady=(0, 4))
        frame_param.columnconfigure(0, weight=1)
        self.lbl_param_novo = ttk.Label(frame_param, text="")
        self.lbl_param_novo.grid(row=0, column=0, sticky="w")
        self.btn_param_menos = ttk.Button(
            frame_param, text="-", width=3, command=lambda: self._ajustar_novo_param(-1)
        )
        self.btn_param_menos.grid(row=0, column=1, padx=2)
        self.btn_param_mais = ttk.Button(
            frame_param, text="+", width=3, command=lambda: self._ajustar_novo_param(+1)
        )
        self.btn_param_mais.grid(row=0, column=2, padx=2)

        ttk.Button(
            sec, text="+ Adicionar ao Pipeline", command=self._adicionar_filtro_pipeline
        ).grid(row=4, column=0, sticky="ew", pady=(2, 6))

        ttk.Label(sec, text="Pipeline ativo (executado de cima para baixo):").grid(
            row=5, column=0, sticky="w"
        )
        self.listbox_pipeline = tk.Listbox(sec, height=5, activestyle="dotbox")
        self.listbox_pipeline.grid(row=6, column=0, sticky="ew", pady=(2, 4))
        self.listbox_pipeline.bind("<<ListboxSelect>>", self._ao_selecionar_item_pipeline)

        # Botões de gerenciamento do pipeline
        frame_acoes_pipe = ttk.Frame(sec)
        frame_acoes_pipe.grid(row=7, column=0, sticky="ew")
        for col in range(4):
            frame_acoes_pipe.columnconfigure(col, weight=1)
        ttk.Button(frame_acoes_pipe, text="↑", command=lambda: self._mover_etapa(-1)).grid(
            row=0, column=0, sticky="ew", padx=1
        )
        ttk.Button(frame_acoes_pipe, text="↓", command=lambda: self._mover_etapa(+1)).grid(
            row=0, column=1, sticky="ew", padx=1
        )
        ttk.Button(frame_acoes_pipe, text="Remover", command=self._remover_etapa).grid(
            row=0, column=2, sticky="ew", padx=1
        )
        ttk.Button(frame_acoes_pipe, text="Limpar", command=self._limpar_pipeline).grid(
            row=0, column=3, sticky="ew", padx=1
        )

        # Ajuste do parâmetro da etapa selecionada na lista
        frame_param_etapa = ttk.Frame(sec)
        frame_param_etapa.grid(row=8, column=0, sticky="ew", pady=(4, 0))
        frame_param_etapa.columnconfigure(0, weight=1)
        self.lbl_param_etapa = ttk.Label(
            frame_param_etapa, text="Selecione uma etapa acima para ajustar", foreground="#555555"
        )
        self.lbl_param_etapa.grid(row=0, column=0, sticky="w")
        self.btn_etapa_menos = ttk.Button(
            frame_param_etapa, text="-", width=3, command=lambda: self._ajustar_etapa_param(-1)
        )
        self.btn_etapa_menos.grid(row=0, column=1, padx=2)
        self.btn_etapa_mais = ttk.Button(
            frame_param_etapa, text="+", width=3, command=lambda: self._ajustar_etapa_param(+1)
        )
        self.btn_etapa_mais.grid(row=0, column=2, padx=2)

    def _construir_secao_operacoes(self, pai: ttk.Frame) -> None:
        sec = ttk.LabelFrame(pai, text="Operações Aritméticas (2ª Imagem)", padding=8)
        sec.pack(fill="x", padx=4, pady=(0, 6))
        sec.columnconfigure(0, weight=1)

        ttk.Button(
            sec, text="Carregar 2ª Imagem...", command=self._carregar_segunda_dialogo
        ).grid(row=0, column=0, sticky="ew", pady=(0, 4))

        self.lbl_segunda = ttk.Label(sec, text="2ª imagem: nenhuma", foreground="#555555")
        self.lbl_segunda.grid(row=1, column=0, sticky="w", pady=(0, 4))

        opcoes_op = ["Nenhuma", *(op.nome for op in self._operacoes)]
        self.combo_op_var = tk.StringVar(value="Nenhuma")
        self.combo_op = ttk.Combobox(
            sec, textvariable=self.combo_op_var, values=opcoes_op, state="readonly"
        )
        self.combo_op.grid(row=2, column=0, sticky="ew", pady=(0, 4))
        self.combo_op.bind("<<ComboboxSelected>>", self._ao_selecionar_operacao)

        self.lbl_desc_op = ttk.Label(
            sec,
            text="Carregue uma 2ª imagem para habilitar operações aritméticas.",
            wraplength=LARGURA_PAINEL - 54,
            justify="left",
            foreground="#555555",
        )
        self.lbl_desc_op.grid(row=3, column=0, sticky="w")

    def _construir_secao_face(self, pai: ttk.Frame) -> None:
        sec = ttk.LabelFrame(pai, text="Detecção Facial (Haar Cascade)", padding=8)
        sec.pack(fill="x", padx=4, pady=(0, 6))
        sec.columnconfigure(0, weight=1)

        self.combo_face_var = tk.StringVar(value=self._efeitos_faciais[0].nome)
        self.combo_face = ttk.Combobox(
            sec,
            textvariable=self.combo_face_var,
            values=[e.nome for e in self._efeitos_faciais],
            state="readonly",
        )
        self.combo_face.grid(row=0, column=0, sticky="ew", pady=(0, 4))
        self.combo_face.bind("<<ComboboxSelected>>", self._ao_selecionar_face)

        self.lbl_desc_face = ttk.Label(
            sec,
            text=self._efeitos_faciais[0].descricao,
            wraplength=LARGURA_PAINEL - 54,
            justify="left",
            foreground="#555555",
        )
        self.lbl_desc_face.grid(row=1, column=0, sticky="w", pady=(0, 4))

        ttk.Checkbutton(
            sec,
            text="Mostrar caixa de detecção de rosto",
            variable=self.mostrar_caixas_var,
            command=self._marcar_sujo,
        ).grid(row=2, column=0, sticky="w")

    def _construir_secao_stickers(self, pai: ttk.Frame) -> None:
        sec = ttk.LabelFrame(pai, text="Stickers (Estáticos e Spritesheet)", padding=8)
        sec.pack(fill="x", padx=4, pady=(0, 8))
        sec.columnconfigure(1, weight=1)

        self.lbl_preview_sticker = ttk.Label(sec)
        self.lbl_preview_sticker.grid(row=0, column=0, rowspan=2, padx=(0, 8), pady=2)

        self.combo_sticker_var = tk.StringVar(value=self._stickers[0].nome)
        self.combo_sticker = ttk.Combobox(
            sec,
            textvariable=self.combo_sticker_var,
            values=[s.nome for s in self._stickers],
            state="readonly",
        )
        self.combo_sticker.grid(row=0, column=1, sticky="ew", pady=(2, 2))
        self.combo_sticker.bind("<<ComboboxSelected>>", self._ao_selecionar_sticker)

        ttk.Label(
            sec,
            text="Clique na imagem para posicionar (Foto e Vídeo)",
            wraplength=LARGURA_PAINEL - 130,
            foreground="#555555",
        ).grid(row=1, column=1, sticky="w")

        frame_btns = ttk.Frame(sec)
        frame_btns.grid(row=2, column=0, columnspan=2, sticky="ew", pady=(6, 0))
        frame_btns.columnconfigure(0, weight=1)
        frame_btns.columnconfigure(1, weight=1)
        ttk.Button(
            frame_btns, text="Desfazer último", command=self._desfazer_ultimo_sticker
        ).grid(row=0, column=0, sticky="ew", padx=(0, 3))
        ttk.Button(
            frame_btns, text="Limpar stickers", command=self._limpar_stickers
        ).grid(row=0, column=1, sticky="ew", padx=(3, 0))

    def _configurar_atalhos(self) -> None:
        self.root.bind("<Control-o>", lambda _e: self._abrir_foto_dialogo())
        self.root.bind("<Control-s>", lambda _e: self._salvar_dialogo())
        self.root.bind("<Control-r>", lambda _e: self._resetar_tudo())
        self.root.bind("<Escape>", lambda _e: self._ao_fechar())

    # ------------------------------------------------------------------
    # Eventos de Controle
    # ------------------------------------------------------------------
    @property
    def webcam_ligada(self) -> bool:
        return self.captura is not None

    def _marcar_sujo(self) -> None:
        self._sujo = True

    def _atualizar_status(self, dica: str) -> None:
        self.status_dica_var.set(dica)

    def _atualizar_botoes_estado(self) -> None:
        self.btn_webcam.configure(
            text="Desligar Webcam" if self.webcam_ligada else "Ligar Webcam"
        )
        tem_entrada = self.webcam_ligada or self.foto is not None
        self.btn_salvar.configure(state="normal" if tem_entrada else "disabled")

    def _abrir_foto_dialogo(self) -> None:
        caminho = filedialog.askopenfilename(
            title="Abrir imagem", filetypes=TIPOS_IMAGEM
        )
        if not caminho:
            return
        try:
            imagem = carregar_imagem(caminho)
        except ImagemInvalidaError as erro:
            messagebox.showerror("Erro ao abrir imagem", str(erro))
            return
        self._desligar_webcam()
        self.foto = imagem
        self.caminho_foto = caminho
        self.stickers_posicionados.clear()
        self._atualizar_botoes_estado()
        self._atualizar_status(f"Foto carregada: {os.path.basename(caminho)}")
        self._marcar_sujo()

    def _ligar_webcam(self) -> None:
        captura = cv2.VideoCapture(self.indice_camera)
        if not captura.isOpened():
            captura.release()
            raise CameraIndisponivelError(
                f"não foi possível abrir a webcam (índice {self.indice_camera}); "
                "verifique se está conectada e livre."
            )
        self.captura = captura
        if self.modo_fundo_id == ID_FUNDO_NENHUM:
            self._definir_modo_fundo(ID_FUNDO_BLUR)
        self._atualizar_botoes_estado()
        self._atualizar_status("Webcam ligada (Desfoque de Fundo ativo)")
        self._marcar_sujo()

    def _desligar_webcam(self) -> None:
        if self.captura is not None:
            self.captura.release()
            self.captura = None
        self._atualizar_botoes_estado()
        self._marcar_sujo()

    def _alternar_webcam(self) -> None:
        if self.webcam_ligada:
            self._desligar_webcam()
            self._atualizar_status(
                "Webcam desligada"
                + (" — voltou para a foto" if self.foto is not None else "")
            )
            return
        try:
            self._ligar_webcam()
        except CameraIndisponivelError as erro:
            messagebox.showwarning("Webcam indisponível", str(erro))
            self._atualizar_status("Webcam indisponível")

    def _salvar_dialogo(self) -> None:
        if self._imagem_exibida_bgr is None:
            return
        if self.webcam_ligada or self.caminho_foto is None:
            carimbo = datetime.now().strftime("%Y%m%d_%H%M%S")
            nome_sugerido = f"{PREFIXO_CAPTURA}_{carimbo}.{FORMATO_CAPTURA}"
            pasta_inicial = os.getcwd()
        else:
            sugestao = caminho_com_sufixo(self.caminho_foto)
            pasta_inicial = os.path.dirname(sugestao) or os.getcwd()
            nome_sugerido = os.path.basename(sugestao)

        destino = filedialog.asksaveasfilename(
            title="Salvar imagem processada",
            initialdir=pasta_inicial,
            initialfile=nome_sugerido,
            defaultextension=".png",
            filetypes=[("PNG", "*.png"), ("JPEG", "*.jpg *.jpeg"), ("BMP", "*.bmp")],
        )
        if not destino:
            return
        if not cv2.imwrite(destino, self._imagem_exibida_bgr):
            messagebox.showerror("Erro ao salvar", f"Não foi possível gravar '{destino}'.")
            return
        self._atualizar_status(f"Salvo em {os.path.basename(destino)}")

    def _resetar_tudo(self) -> None:
        self.canal_var.set(CANAL_PADRAO)
        self.etapas_filtros.clear()
        self._atualizar_listbox_pipeline()
        self.operacao_id = None
        self.combo_op_var.set("Nenhuma")
        self._definir_modo_fundo(ID_FUNDO_NENHUM)
        self.efeito_facial_id = ID_EFEITO_NENHUM
        self.combo_face_var.set(self._efeitos_faciais[0].nome)
        self.lbl_desc_face.configure(text=self._efeitos_faciais[0].descricao)
        self.mostrar_caixas_var.set(False)
        self.stickers_posicionados.clear()
        self._atualizar_status("Restaurado ao original")
        self._marcar_sujo()

    # -- Fundo da Webcam (Blur / Virtual) --
    def _definir_modo_fundo(self, id_modo: str) -> None:
        modo = obter_modo_fundo(id_modo)
        self.modo_fundo_id = modo.id
        self.combo_fundo_var.set(modo.nome)
        self._atualizar_controles_fundo()

    def _atualizar_controles_fundo(self) -> None:
        self.lbl_blur_fundo.configure(
            text=f"Intensidade do blur: {self.intensidade_blur_fundo} ({BLUR_MINIMO}–{BLUR_MAXIMO})"
        )
        estado_blur = "normal" if self.modo_fundo_id == ID_FUNDO_BLUR else "disabled"
        self.btn_blur_menos.configure(state=estado_blur)
        self.btn_blur_mais.configure(state=estado_blur)

    def _ao_selecionar_modo_fundo(self, _evento: object = None) -> None:
        idx = self.combo_fundo.current()
        if idx < 0:
            return
        modo = self._modos_fundo[idx]
        if modo.id == ID_FUNDO_IMAGEM and self.imagem_fundo is None:
            self._escolher_imagem_fundo()
            if self.imagem_fundo is None:
                self._definir_modo_fundo(self.modo_fundo_id)
            return
        self._definir_modo_fundo(modo.id)
        self._atualizar_status(f"Fundo: {modo.nome}")
        self._marcar_sujo()

    def _ajustar_blur_fundo(self, direcao: int) -> None:
        self.intensidade_blur_fundo = ajustar_intensidade_blur(
            self.intensidade_blur_fundo, direcao
        )
        self._atualizar_controles_fundo()
        self._atualizar_status(f"Blur de fundo: {self.intensidade_blur_fundo}")
        self._marcar_sujo()

    def _escolher_imagem_fundo(self) -> None:
        caminho = filedialog.askopenfilename(
            title="Escolher imagem de fundo", filetypes=TIPOS_IMAGEM
        )
        if not caminho:
            return
        try:
            imagem = carregar_imagem(caminho)
        except ImagemInvalidaError as erro:
            messagebox.showerror("Erro ao carregar fundo", str(erro))
            return
        self.imagem_fundo = imagem
        self.caminho_fundo = caminho
        self.lbl_imagem_fundo.configure(
            text=f"Imagem de fundo: {os.path.basename(caminho)}"
        )
        self._definir_modo_fundo(ID_FUNDO_IMAGEM)
        self._atualizar_status(f"Fundo virtual: {os.path.basename(caminho)}")
        self._marcar_sujo()

    def _ao_mudar_canal(self) -> None:
        self._atualizar_status(f"Canal: {self.canal_var.get()}")
        self._marcar_sujo()

    # -- Filtros e Pipeline --
    def _ao_selecionar_filtro_combo(self, _evento: object = None) -> None:
        idx = self.combo_filtro.current()
        if idx < 0:
            return
        filtro = self._filtros[idx]
        self._filtro_selecionado_id = filtro.id
        self._valor_novo_filtro = valor_padrao(filtro.id)
        self._atualizar_preview_filtro_selecionado()

    def _atualizar_preview_filtro_selecionado(self) -> None:
        filtro = obter_filtro(self._filtro_selecionado_id)
        self.lbl_desc_filtro.configure(
            text=descrever_filtro(filtro.id, self._valor_novo_filtro)
        )
        if filtro.parametro is None:
            self.lbl_param_novo.configure(text="Sem parâmetros configuráveis")
            self.btn_param_menos.configure(state="disabled")
            self.btn_param_mais.configure(state="disabled")
        else:
            p = filtro.parametro
            atual = p.padrao if self._valor_novo_filtro is None else self._valor_novo_filtro
            self.lbl_param_novo.configure(
                text=f"Parâmetro {p.nome}: {atual} ({p.minimo}–{p.maximo})"
            )
            self.btn_param_menos.configure(state="normal")
            self.btn_param_mais.configure(state="normal")

    def _ajustar_novo_param(self, direcao: int) -> None:
        novo = ajustar_parametro(
            self._filtro_selecionado_id, self._valor_novo_filtro, direcao
        )
        if novo is not None:
            self._valor_novo_filtro = novo
            self._atualizar_preview_filtro_selecionado()

    def _adicionar_filtro_pipeline(self) -> None:
        etapa = EtapaFiltro(self._filtro_selecionado_id, self._valor_novo_filtro)
        self.etapas_filtros.append(etapa)
        self._atualizar_listbox_pipeline(selecionar_idx=len(self.etapas_filtros) - 1)
        self._atualizar_status(f"Adicionado: {etapa.rotulo()}")
        self._marcar_sujo()

    def _atualizar_listbox_pipeline(self, selecionar_idx: int | None = None) -> None:
        self.listbox_pipeline.delete(0, tk.END)
        for i, etapa in enumerate(self.etapas_filtros, start=1):
            self.listbox_pipeline.insert(tk.END, f"{i}. {etapa.rotulo()}")
        if selecionar_idx is not None and 0 <= selecionar_idx < len(self.etapas_filtros):
            self.listbox_pipeline.selection_set(selecionar_idx)
        self._ao_selecionar_item_pipeline()

    def _indice_etapa_selecionada(self) -> int | None:
        sel = self.listbox_pipeline.curselection()
        if not sel:
            return None
        return int(sel[0])

    def _ao_selecionar_item_pipeline(self, _evento: object = None) -> None:
        idx = self._indice_etapa_selecionada()
        if idx is None or idx >= len(self.etapas_filtros):
            self.lbl_param_etapa.configure(text="Selecione uma etapa acima para ajustar")
            self.btn_etapa_menos.configure(state="disabled")
            self.btn_etapa_mais.configure(state="disabled")
            return
        etapa = self.etapas_filtros[idx]
        filtro = obter_filtro(etapa.id_filtro)
        if filtro.parametro is None:
            self.lbl_param_etapa.configure(text=f"{filtro.nome}: sem parâmetros")
            self.btn_etapa_menos.configure(state="disabled")
            self.btn_etapa_mais.configure(state="disabled")
        else:
            p = filtro.parametro
            atual = p.padrao if etapa.valor is None else etapa.valor
            self.lbl_param_etapa.configure(text=f"Etapa #{idx + 1} ({p.nome}): {atual}")
            self.btn_etapa_menos.configure(state="normal")
            self.btn_etapa_mais.configure(state="normal")

    def _ajustar_etapa_param(self, direcao: int) -> None:
        idx = self._indice_etapa_selecionada()
        if idx is None or idx >= len(self.etapas_filtros):
            return
        etapa = self.etapas_filtros[idx]
        novo = ajustar_parametro(etapa.id_filtro, etapa.valor, direcao)
        if novo is not None:
            etapa.valor = novo
            self._atualizar_listbox_pipeline(selecionar_idx=idx)
            self._marcar_sujo()

    def _mover_etapa(self, delta: int) -> None:
        idx = self._indice_etapa_selecionada()
        if idx is None:
            return
        novo_idx = idx + delta
        if 0 <= novo_idx < len(self.etapas_filtros):
            self.etapas_filtros[idx], self.etapas_filtros[novo_idx] = (
                self.etapas_filtros[novo_idx],
                self.etapas_filtros[idx],
            )
            self._atualizar_listbox_pipeline(selecionar_idx=novo_idx)
            self._marcar_sujo()

    def _remover_etapa(self) -> None:
        idx = self._indice_etapa_selecionada()
        if idx is None:
            return
        removida = self.etapas_filtros.pop(idx)
        prox = min(idx, len(self.etapas_filtros) - 1) if self.etapas_filtros else None
        self._atualizar_listbox_pipeline(selecionar_idx=prox)
        self._atualizar_status(f"Removido: {removida.rotulo()}")
        self._marcar_sujo()

    def _limpar_pipeline(self) -> None:
        self.etapas_filtros.clear()
        self._atualizar_listbox_pipeline()
        self._atualizar_status("Pipeline de filtros limpo")
        self._marcar_sujo()

    # -- Operações Aritméticas --
    def _carregar_segunda_dialogo(self) -> None:
        caminho = filedialog.askopenfilename(
            title="Carregar 2ª imagem", filetypes=TIPOS_IMAGEM
        )
        if not caminho:
            return
        try:
            imagem = carregar_imagem(caminho)
        except ImagemInvalidaError as erro:
            messagebox.showerror("Erro ao carregar 2ª imagem", str(erro))
            return
        self.segunda = imagem
        self.caminho_segunda = caminho
        self.lbl_segunda.configure(text=f"2ª imagem: {os.path.basename(caminho)}")
        self._atualizar_status(f"2ª imagem carregada: {os.path.basename(caminho)}")
        self._marcar_sujo()

    def _ao_selecionar_operacao(self, _evento: object = None) -> None:
        idx = self.combo_op.current()
        if idx <= 0:
            self.operacao_id = None
            self.lbl_desc_op.configure(text="Nenhuma operação aritmética ativa.")
            self._marcar_sujo()
            return
        if self.segunda is None:
            messagebox.showinfo(
                "2ª imagem necessária",
                "Carregue uma segunda imagem primeiro clicando em 'Carregar 2ª Imagem...'.",
            )
            self.combo_op_var.set("Nenhuma")
            self.operacao_id = None
            return
        op = self._operacoes[idx - 1]
        self.operacao_id = op.id
        self.lbl_desc_op.configure(text=op.descricao)
        self._atualizar_status(f"Operação: {op.nome}")
        self._marcar_sujo()

    # -- Detecção Facial --
    def _ao_selecionar_face(self, _evento: object = None) -> None:
        idx = self.combo_face.current()
        if idx < 0:
            return
        efeito = self._efeitos_faciais[idx]
        self.efeito_facial_id = efeito.id
        self.lbl_desc_face.configure(text=efeito.descricao)
        self._atualizar_status(f"Detecção facial: {efeito.nome}")
        self._marcar_sujo()

    # -- Stickers --
    def _ao_selecionar_sticker(self, _evento: object = None) -> None:
        idx = self.combo_sticker.current()
        if idx < 0:
            return
        sticker = self._stickers[idx]
        self.sticker_atual_id = sticker.id
        self._atualizar_status(f"Sticker: {sticker.nome}")

    def _desfazer_ultimo_sticker(self) -> None:
        if self.stickers_posicionados:
            self.stickers_posicionados.pop()
            self._atualizar_status("Último sticker removido")
            self._marcar_sujo()

    def _limpar_stickers(self) -> None:
        self.stickers_posicionados.clear()
        self._atualizar_status("Stickers limpos")
        self._marcar_sujo()

    def _ao_clicar_canvas(self, evento: tk.Event) -> None:
        if self._imagem_exibida_bgr is None or self._render_escala <= 0:
            return
        x_rel = evento.x - self._render_offset_x
        y_rel = evento.y - self._render_offset_y
        if not (0 <= x_rel < self._render_larg and 0 <= y_rel < self._render_alt):
            return
        orig_x = int(x_rel / self._render_escala)
        orig_y = int(y_rel / self._render_escala)
        self.stickers_posicionados.append((self.sticker_atual_id, orig_x, orig_y))
        nome = obter_sticker(self.sticker_atual_id).nome
        self._atualizar_status(f"Sticker '{nome}' fixado em ({orig_x}, {orig_y})")
        self._marcar_sujo()

    # ------------------------------------------------------------------
    # Loop Principal de Atualização e Renderização
    # ------------------------------------------------------------------
    def _tem_sticker_animado_ativo(self) -> bool:
        return any(
            obter_sticker(sid).animado for sid, _x, _y in self.stickers_posicionados
        )

    def _atualizar_preview_sticker(self, tempo: float) -> None:
        quadro_rgba = obter_quadro_sticker(self.sticker_atual_id, tempo)
        mini = Image.fromarray(quadro_rgba, mode="RGBA").resize(
            (54, 54), Image.Resampling.LANCZOS
        )
        self._photo_preview_sticker = ImageTk.PhotoImage(mini)
        self.lbl_preview_sticker.configure(image=self._photo_preview_sticker)

    def _obter_quadro_base(self) -> np.ndarray | None:
        if self.captura is not None:
            lido, quadro = self.captura.read()
            if lido and quadro is not None:
                return quadro
            self._desligar_webcam()
            self._atualizar_status("Webcam interrompida")
            return self.foto
        return self.foto

    def _executar_pipeline_sem_stickers(self, base: np.ndarray) -> np.ndarray:
        imagem = base
        if self.modo_fundo_id != ID_FUNDO_NENHUM:
            imagem, _ = aplicar_efeito_fundo(
                imagem,
                self.modo_fundo_id,
                imagem_fundo_bgr=self.imagem_fundo,
                intensidade_blur=self.intensidade_blur_fundo,
            )
        if self.operacao_id is not None and self.segunda is not None:
            imagem = aplicar_operacao(self.operacao_id, imagem, self.segunda)
        imagem = aplicar_selecao_canal(imagem, self.canal_var.get())
        if self.etapas_filtros:
            imagem = aplicar_pipeline_filtros(imagem, self.etapas_filtros)
        if self.efeito_facial_id != ID_EFEITO_NENHUM or self.mostrar_caixas_var.get():
            # Na webcam, re-detecta a cada 2 quadros para manter alta fluidez
            faces_cache = None
            if self.webcam_ligada and self._frame_count % 2 == 1:
                faces_cache = self._ultimas_faces
            imagem, faces = aplicar_efeito_facial(
                imagem,
                self.efeito_facial_id,
                mostrar_caixas=self.mostrar_caixas_var.get(),
                faces=faces_cache,
            )
            self._ultimas_faces = faces
            self._faces_ultima_contagem = len(faces)
        else:
            self._faces_ultima_contagem = 0
        return imagem

    def _aplicar_stickers_atuais(
        self, imagem: np.ndarray, tempo: float
    ) -> np.ndarray:
        resultado = imagem
        for sid, x, y in self.stickers_posicionados:
            quadro_rgba = obter_quadro_sticker(sid, tempo)
            resultado = aplicar_sticker(resultado, quadro_rgba, (x, y))
        return resultado

    def _renderizar_no_canvas(self, imagem_bgr: np.ndarray | None) -> None:
        cw = max(1, self.canvas.winfo_width())
        ch = max(1, self.canvas.winfo_height())
        self.canvas.delete("all")

        if imagem_bgr is None:
            self.canvas.create_text(
                cw // 2,
                ch // 2,
                text="Nenhuma imagem carregada.\nClique em 'Abrir Foto...' ou 'Ligar Webcam'.",
                fill="#b8b8c8",
                font=("Segoe UI", 12),
                justify="center",
            )
            self.status_fonte_var.set("Nenhuma imagem carregada")
            return

        alt, larg = imagem_bgr.shape[:2]
        escala = min(cw / larg, ch / alt)
        nova_larg = max(1, int(larg * escala))
        nova_alt = max(1, int(alt * escala))
        off_x = (cw - nova_larg) // 2
        off_y = (ch - nova_alt) // 2

        self._render_escala = escala
        self._render_offset_x = off_x
        self._render_offset_y = off_y
        self._render_larg = nova_larg
        self._render_alt = nova_alt

        rgb = cv2.cvtColor(imagem_bgr, cv2.COLOR_BGR2RGB)
        pil_img = Image.fromarray(rgb)
        if nova_larg != larg or nova_alt != alt:
            pil_img = pil_img.resize((nova_larg, nova_alt), Image.Resampling.BILINEAR)
        self._photo_canvas = ImageTk.PhotoImage(pil_img)
        self.canvas.create_image(off_x, off_y, anchor="nw", image=self._photo_canvas)

        fonte = (
            f"Webcam (índice {self.indice_camera})"
            if self.webcam_ligada
            else os.path.basename(self.caminho_foto or "")
        )
        info = f"{fonte}  |  {larg}×{alt}  |  Filtros: {len(self.etapas_filtros)}"
        if self.efeito_facial_id != ID_EFEITO_NENHUM or self.mostrar_caixas_var.get():
            info += f"  |  Faces: {self._faces_ultima_contagem}"
        self.status_fonte_var.set(info)

    def _loop_tick(self) -> None:
        tempo = time.monotonic() - self._inicio_tempo
        self._frame_count += 1
        self._atualizar_preview_sticker(tempo)

        precisa_pipeline = self.webcam_ligada or self._sujo
        precisa_stickers = precisa_pipeline or self._tem_sticker_animado_ativo()

        if precisa_pipeline:
            base = self._obter_quadro_base()
            if base is None:
                self._imagem_processada_sem_stickers = None
                self._imagem_exibida_bgr = None
                self._renderizar_no_canvas(None)
                self._sujo = False
            else:
                self._imagem_processada_sem_stickers = (
                    self._executar_pipeline_sem_stickers(base)
                )
                self._sujo = False

        if precisa_stickers and self._imagem_processada_sem_stickers is not None:
            self._imagem_exibida_bgr = self._aplicar_stickers_atuais(
                self._imagem_processada_sem_stickers, tempo
            )
            self._renderizar_no_canvas(self._imagem_exibida_bgr)

        self.root.after(INTERVALO_MS, self._loop_tick)

    def _ao_fechar(self) -> None:
        self._desligar_webcam()
        self.root.destroy()


def executar_gui(
    caminho_imagem: str | None = None,
    iniciar_video: bool = False,
    indice_camera: int = 0,
) -> None:
    """Inicia a janela Tkinter do aplicativo."""
    root = tk.Tk()
    AplicacaoGrauB(
        root,
        caminho_imagem=caminho_imagem,
        iniciar_video=iniciar_video,
        indice_camera=indice_camera,
    )
    root.mainloop()
