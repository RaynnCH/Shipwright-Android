#!/usr/bin/env python3
"""Generates the launcher icon (Triforce + Fairy Ocarina) into Android/app/src/main/res.

Run from the repository root: python3 Android/icon-assets/generate_icon.py
"""
import math
import os
import random
from PIL import Image, ImageChops, ImageDraw, ImageFilter

S = 2048          # master canvas
SS = 2            # supersampling for crisp edges
RES = os.path.join(os.path.dirname(__file__), '..', 'app', 'src', 'main', 'res')


def lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(len(a)))


def vgrad(size, top, bottom):
    w, h = size
    img = Image.new('RGBA', size)
    px = img.load()
    for y in range(h):
        c = lerp(top, bottom, y / max(1, h - 1))
        for x in range(w):
            px[x, y] = c
    return img


def masked(gradient, mask):
    out = Image.new('RGBA', gradient.size, (0, 0, 0, 0))
    out.paste(gradient, (0, 0), mask)
    return out


def background():
    img = Image.new('RGBA', (S, S))
    px = img.load()
    cx, cy = S * 0.5, S * 0.42
    inner, outer = (36, 112, 120, 255), (10, 28, 52, 255)   # teal glow into deep Hyrule night
    for y in range(S):
        for x in range(S):
            d = min(1.0, math.hypot(x - cx, y - cy) / (S * 0.75))
            px[x, y] = lerp(inner, outer, d ** 0.8)
    rnd = random.Random(64)
    dr = ImageDraw.Draw(img)
    for _ in range(70):
        x, y = rnd.randrange(S), rnd.randrange(S)
        r = rnd.choice((2, 3, 3, 4, 6))
        a = rnd.randrange(110, 230)
        dr.ellipse((x - r, y - r, x + r, y + r), fill=(255, 250, 225, a))
    return img


def triangle(cx, top_y, side):
    h = side * math.sqrt(3) / 2
    return [(cx, top_y), (cx - side / 2, top_y + h), (cx + side / 2, top_y + h)]


def foreground():
    W = S * SS
    layer = Image.new('RGBA', (W, W), (0, 0, 0, 0))

    # --- Triforce (gold): three triangles, centred in the adaptive-icon safe zone ---
    a = 470 * SS                      # side of one small triangle
    gap = 30 * SS
    h = a * math.sqrt(3) / 2
    cx, y0 = W / 2, 380 * SS
    tri_mask = Image.new('L', (W, W), 0)
    md = ImageDraw.Draw(tri_mask)
    md.polygon(triangle(cx, y0, a), fill=255)
    md.polygon(triangle(cx - a / 2 - gap / 2, y0 + h + gap * 0.87, a), fill=255)
    md.polygon(triangle(cx + a / 2 + gap / 2, y0 + h + gap * 0.87, a), fill=255)
    glow = masked(Image.new('RGBA', (W, W), (255, 214, 90, 255)), tri_mask).filter(ImageFilter.GaussianBlur(45 * SS))
    glow.putalpha(glow.getchannel('A').point(lambda v: int(v * 0.7)))
    layer = Image.alpha_composite(layer, glow)
    gold = vgrad((W, W), (255, 240, 160, 255), (226, 158, 28, 255))
    layer = Image.alpha_composite(layer, masked(gold, tri_mask))
    edge = ImageChops.subtract(tri_mask, tri_mask.filter(ImageFilter.MinFilter(9 * SS + 1)))
    layer = Image.alpha_composite(layer, masked(Image.new('RGBA', (W, W), (150, 92, 10, 255)), edge))

    # --- Fairy Ocarina, drawn upright on its own canvas, then tilted ---
    L = 1500 * SS
    oc = Image.new('RGBA', (L, L), (0, 0, 0, 0))
    o = L / 2
    body = Image.new('L', (L, L), 0)
    bd = ImageDraw.Draw(body)
    bd.ellipse((o - 360 * SS, o - 215 * SS, o + 330 * SS, o + 215 * SS), fill=255)            # bulb
    bd.polygon([(o + 120 * SS, o - 150 * SS), (o + 470 * SS, o - 95 * SS),
                (o + 470 * SS, o + 55 * SS), (o + 120 * SS, o + 120 * SS)], fill=255)          # mouthpiece
    bd.ellipse((o + 430 * SS, o - 100 * SS, o + 510 * SS, o + 60 * SS), fill=255)             # rounded tip
    body = body.filter(ImageFilter.GaussianBlur(2 * SS)).point(lambda v: 255 if v > 127 else 0)
    outline = ImageChops.subtract(body.filter(ImageFilter.MaxFilter(17 * SS + 1)), body)
    oc = Image.alpha_composite(oc, masked(Image.new('RGBA', (L, L), (8, 20, 60, 255)), outline))
    blue = vgrad((L, L), (150, 205, 255, 255), (34, 78, 196, 255))
    oc = Image.alpha_composite(oc, masked(blue, body))
    hl = Image.new('L', (L, L), 0)
    ImageDraw.Draw(hl).ellipse((o - 300 * SS, o - 170 * SS, o + 20 * SS, o - 60 * SS), fill=170)
    hl = ImageChops.multiply(hl.filter(ImageFilter.GaussianBlur(28 * SS)), body)
    oc = Image.alpha_composite(oc, masked(Image.new('RGBA', (L, L), (255, 255, 255, 255)), hl))
    d = ImageDraw.Draw(oc)
    for hx, hy, r in ((-230, 20, 52), (-110, 85, 52), (10, 85, 52), (130, 20, 52), (-150, -80, 40), (-40, -100, 40)):
        x, y = o + hx * SS, o + hy * SS
        d.ellipse((x - r * SS, y - r * SS, x + r * SS, y + r * SS), fill=(10, 24, 74, 255))
        d.arc((x - r * SS * 0.7, y - r * SS * 0.7, x + r * SS * 0.7, y + r * SS * 0.7), 200, 290,
              fill=(90, 140, 235, 255), width=5 * SS)
    d.ellipse((o + 250 * SS, o - 50 * SS, o + 400 * SS, o + 30 * SS), fill=(10, 24, 74, 255))  # window
    oc = oc.rotate(32, resample=Image.BICUBIC)
    shadow = oc.getchannel('A').filter(ImageFilter.GaussianBlur(26 * SS)).point(lambda v: int(v * 0.5))
    sh = Image.new('RGBA', (L, L), (0, 10, 30, 255))
    sh.putalpha(shadow)
    px_, py_ = int(W / 2 - L / 2 + 20 * SS), int(1330 * SS - L / 2)
    layer.alpha_composite(sh, (px_, py_ + 24 * SS))
    layer.alpha_composite(oc, (px_, py_))

    def star(cx_, cy_, r):
        pts = []
        for i in range(8):
            ang = math.pi / 4 * i - math.pi / 2
            rr = r if i % 2 == 0 else r * 0.22
            pts.append((cx_ + rr * math.cos(ang), cy_ + rr * math.sin(ang)))
        return pts
    sd = ImageDraw.Draw(layer)
    sd.polygon(star(W * 0.80, 1000 * SS, 80 * SS), fill=(255, 250, 220, 255))
    sd.polygon(star(W * 0.22, 1480 * SS, 50 * SS), fill=(255, 250, 220, 230))
    return layer.resize((S, S), Image.LANCZOS)


def mono(fg):
    a = fg.getchannel('A')
    out = Image.new('RGBA', fg.size, (0, 0, 0, 0))
    out.paste((255, 255, 255, 255), (0, 0), a)
    return out


def squircle_mask(size, radius_ratio):
    m = Image.new('L', (size * 4, size * 4), 0)
    ImageDraw.Draw(m).rounded_rectangle((0, 0, size * 4 - 1, size * 4 - 1), radius=int(size * 4 * radius_ratio), fill=255)
    return m.resize((size, size), Image.LANCZOS)


def circle_mask(size):
    m = Image.new('L', (size * 4, size * 4), 0)
    ImageDraw.Draw(m).ellipse((0, 0, size * 4 - 1, size * 4 - 1), fill=255)
    return m.resize((size, size), Image.LANCZOS)


def main():
    bg = background()
    fg = foreground()
    here = os.path.dirname(os.path.abspath(__file__))
    bg.convert('RGB').save(os.path.join(here, 'ic_launcher_background.png'))
    fg.save(os.path.join(here, 'ic_launcher_foreground.png'))
    mono(fg).save(os.path.join(here, 'ic_launcher_monochrome.png'))
    full = Image.alpha_composite(bg, fg)

    nodpi = os.path.join(RES, 'drawable-nodpi')
    os.makedirs(nodpi, exist_ok=True)
    bg.convert('RGB').resize((432, 432), Image.LANCZOS).save(os.path.join(nodpi, 'ic_launcher_background.png'))
    mono(fg).resize((432, 432), Image.LANCZOS).save(os.path.join(nodpi, 'ic_launcher_monochrome.png'))

    for name, legacy, adaptive in (('mdpi', 48, 108), ('hdpi', 72, 162), ('xhdpi', 96, 216),
                                   ('xxhdpi', 144, 324), ('xxxhdpi', 192, 432)):
        d = os.path.join(RES, 'mipmap-' + name)
        os.makedirs(d, exist_ok=True)
        fg.resize((adaptive, adaptive), Image.LANCZOS).save(os.path.join(d, 'ic_launcher_foreground.webp'), quality=95)
        sq = full.resize((legacy, legacy), Image.LANCZOS)
        sq.putalpha(squircle_mask(legacy, 0.22))
        sq.save(os.path.join(d, 'ic_launcher.webp'), quality=95)
        rd = full.resize((legacy, legacy), Image.LANCZOS)
        rd.putalpha(circle_mask(legacy))
        rd.save(os.path.join(d, 'ic_launcher_round.webp'), quality=95)


if __name__ == '__main__':
    main()
