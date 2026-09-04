#!/usr/bin/env python3
"""
Gera os assets do Mini-Mario (Grau A) a partir do pack CC0
"Pixel Platformer" da Kenney (https://kenney.nl/assets/pixel-platformer).

O pack distribui sprites individuais de 18x18 (tiles) e 24x24 (personagens);
este script os combina em:
  - spritesheets em grade regular (player.png, enemy.png, coin.png)
  - imagens únicas (platform.png, flag.png)
  - camadas de parallax 960x540 (sky.png, hills.png, trees.png)

Uso: python3 make_assets.py <caminho/para/kenney_pixel-platformer> <pasta_de_saida>
"""
import math
import sys
from PIL import Image

TILE = 18   # tamanho dos tiles do cenário no pack
CHAR = 24   # tamanho dos personagens no pack


def load_tile(root, index, size=TILE):
    return Image.open(f"{root}/Tiles/tile_{index:04d}.png").convert("RGBA")


def load_char(root, index):
    return Image.open(f"{root}/Tiles/Characters/tile_{index:04d}.png").convert("RGBA")


def scale_nearest(img, target):
    return img.resize((target, target), Image.NEAREST)


def tint(img, color, amount):
    """Mistura o RGB da imagem com uma cor sólida, PRESERVANDO o alfa
    (perspectiva aérea / coesão sem criar fundos opacos)."""
    r, g, b, a = img.split()
    solid = Image.new("RGBA", img.size, color)
    blended = Image.blend(img.convert("RGB"), solid.convert("RGB"), amount)
    out = blended.convert("RGBA")
    out.putalpha(a)
    return out


def compose_sheets(root, out):
    # ---- player.png: grade 2 colunas x 3 linhas, células de 32x32 ----
    #   linha 0: parado (1 frame)   -> char 0
    #   linha 1: andando (2 frames) -> char 0, char 1
    #   linha 2: pulo (1 frame)     -> char 1
    cell = 32
    sheet = Image.new("RGBA", (cell * 2, cell * 3), (0, 0, 0, 0))
    idle = scale_nearest(load_char(root, 0), cell)
    walk = scale_nearest(load_char(root, 1), cell)
    sheet.paste(idle, (0, 0), idle)
    sheet.paste(idle, (0, cell), idle)
    sheet.paste(walk, (cell, cell), walk)
    sheet.paste(walk, (0, cell * 2), idle)
    sheet.save(f"{out}/player.png")

    # ---- enemy.png: 2 frames de andar (linha 0) ----
    sheet = Image.new("RGBA", (cell * 2, cell), (0, 0, 0, 0))
    for i, idx in enumerate((19, 20)):
        frame = scale_nearest(load_char(root, idx), cell)
        sheet.paste(frame, (i * cell, 0), frame)
    sheet.save(f"{out}/enemy.png")

    # ---- coin.png: 4 frames "girando" (linha 0) ----
    # O giro é simulado espremendo horizontalmente o sprite da moeda.
    coin = scale_nearest(load_tile(root, 151), cell)
    widths = (32, 18, 6, 18)
    sheet = Image.new("RGBA", (cell * 4, cell), (0, 0, 0, 0))
    for i, w in enumerate(widths):
        frame = coin.resize((w, cell), Image.NEAREST)
        sheet.paste(frame, (i * cell + (cell - w) // 2, 0), frame)
    sheet.save(f"{out}/coin.png")


def compose_scene_sprites(root, out):
    # ---- platform.png: bloco de grama (esticado pela model matrix) ----
    scale_nearest(load_tile(root, 1), 32).save(f"{out}/platform.png")

    # ---- flag.png: mastro + bandeira desenhados (32x64) ----
    flag = Image.new("RGBA", (32, 64), (0, 0, 0, 0))
    px = flag.load()
    for y in range(4, 64):                       # mastro
        for x in range(26, 30):
            px[x, y] = (90, 105, 136, 255)
    px[29, 63] = (60, 70, 96, 255)               # base
    for y in range(6, 24):                       # bandeira vermelha
        for x in range(6, 26):
            px[x, y] = (229, 80, 57, 255)
    for y in range(8, 22):                       # brilho da bandeira
        for x in range(8, 24):
            px[x, y] = (240, 100, 70, 255)
    flag.save(f"{out}/flag.png")


def brighten(img, amount):
    """Empurra o RGB da imagem para branco, preservando o alfa."""
    r, g, b, a = img.split()
    r = r.point(lambda v: v + (255 - v) * amount)
    g = g.point(lambda v: v + (255 - v) * amount)
    b = b.point(lambda v: v + (255 - v) * amount)
    out = Image.merge("RGBA", (r, g, b, a))
    return out


def compose_parallax(root, out):
    W, H = 960, 540
    SKY = (109, 170, 229, 255)  # azul do céu (combina com o glClearColor)

    # ---- sky.png: céu plano + nuvens espalhadas ----
    sky = Image.new("RGBA", (W, H), SKY)
    for (x, y, s, alpha) in [(80, 60, 6, 220), (420, 110, 4, 190),
                             (700, 50, 7, 230), (260, 200, 3, 170),
                             (820, 190, 4, 180)]:
        cloud = brighten(load_tile(root, 153 if s % 2 else 154), 0.7)
        cloud.putalpha(cloud.split()[3].point(lambda a: a * alpha // 255))
        cloud = cloud.resize((TILE * s, TILE * s), Image.NEAREST)
        sky.paste(cloud, (x, y), cloud)
    sky.save(f"{out}/sky.png")

    # ---- hills.png: montes distantes (blocos verdes com perspectiva aérea) ----
    hills = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    block = load_tile(root, 36)
    mounds = [(0, 400, 260, 140), (180, 440, 240, 100), (370, 370, 300, 170),
              (620, 430, 260, 110), (830, 390, 200, 150)]
    for (x, y, w, h) in mounds:
        mound = Image.new("RGBA", (w, h))
        tile_top = tint(scale_nearest(block, 36), SKY, 0.35)
        tile = tint(block, SKY, 0.35).resize((36, 36), Image.NEAREST)
        mound.paste(tile_top, (0, 0))
        for yy in range(36, h, 36):
            mound.paste(tile, (0, yy))
        for xx in range(36, w, 36):
            mound.paste(tile_top, (xx, 0))
            for yy in range(36, h, 36):
                mound.paste(tile, (xx, yy))
        hills.paste(mound, (x, y))
    hills.save(f"{out}/hills.png")

    # ---- trees.png: vegetação próxima (linha do chão, mais escura) ----
    trees = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    scenery = [(126, 4, 40), (99, 3, 140), (128, 3, 250), (126, 5, 350),
               (124, 4, 470), (99, 3, 560), (126, 4, 650), (128, 3, 740),
               (124, 4, 820), (126, 5, 900)]
    for (idx, scale, x) in scenery:
        sprite = tint(load_tile(root, idx), (40, 70, 90, 255), 0.30)
        sprite = sprite.resize((TILE * scale, TILE * scale), Image.NEAREST)
        trees.paste(sprite, (x, H - sprite.height), sprite)
    trees.save(f"{out}/trees.png")


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else "."
    out = sys.argv[2] if len(sys.argv) > 2 else "assets"
    compose_sheets(root, out)
    compose_scene_sprites(root, out)
    compose_parallax(root, out)
    print("Assets gerados em", out)


if __name__ == "__main__":
    main()
