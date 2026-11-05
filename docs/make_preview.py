from PIL import Image, ImageDraw, ImageFont
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FONT_PATH = "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"
FONT_CN = ImageFont.truetype(FONT_PATH, 10)
FONT_ASCII = ImageFont.truetype(FONT_PATH, 8)
FONT_TINY = ImageFont.truetype(FONT_PATH, 5)
SCALE = 4
PANEL_W, PANEL_H = 128, 64
MARGIN, GAP, LABEL_H = 22, 18, 17

screens = [
    ("开机标识（约 1.5 秒）", "splash"),
    ("主页面（统一小字体，方向位于车次和速度之间）", "main"),
    ("补充信息页（不显示 A/B 端位）", "detail"),
    ("无数据时", "empty"),
]

canvas_w = MARGIN * 2 + PANEL_W * SCALE * 2 + GAP * SCALE
canvas_h = MARGIN * 2 + LABEL_H * SCALE * 2 + PANEL_H * SCALE * 2 + GAP * SCALE
img = Image.new("RGB", (canvas_w, canvas_h), (25, 27, 30))
d = ImageDraw.Draw(img)

for idx, (label, kind) in enumerate(screens):
    col, row = idx % 2, idx // 2
    x = MARGIN + col * (PANEL_W * SCALE + GAP * SCALE)
    y = MARGIN + row * (PANEL_H * SCALE + (LABEL_H + GAP) * SCALE)
    d.text((x, y), label, fill=(225, 230, 235), font=ImageFont.truetype(FONT_PATH, 11))
    sy = y + LABEL_H * SCALE
    d.rounded_rectangle((x-3, sy-3, x+PANEL_W*SCALE+3, sy+PANEL_H*SCALE+3), radius=8,
                        fill=(8, 9, 10), outline=(100, 106, 114), width=2)
    lcd = Image.new("RGB", (PANEL_W, PANEL_H), (238, 239, 230))
    ld = ImageDraw.Draw(lcd)
    ld.rectangle((0, 0, PANEL_W-1, PANEL_H-1), outline=(40, 43, 43), width=1)

    if kind == "splash":
        ld.text((21, 9), "LBJ RX 821.2375", fill=(13, 16, 18), font=FONT_ASCII)
        ld.text((22, 28), "铁路列车预警接收", fill=(13, 16, 18), font=FONT_CN)
        ld.text((38, 49), "POCSAG 1200", fill=(13, 16, 18), font=FONT_ASCII)
    elif kind == "main":
        # Separate top status strip; history count and page only.
        ld.text((2, 1), "01/10 1/2", fill=(13, 16, 18), font=FONT_TINY)
        # All information rows use the same small-font treatment as UI_PrintStringSmall.
        ld.text((5, 9), "车K1206 上行81km/h", fill=(13, 16, 18), font=FONT_CN)
        ld.text((5, 24), "K201", fill=(13, 16, 18), font=FONT_ASCII)
        ld.text((5, 34), "型 HXD3D-00477", fill=(13, 16, 18), font=FONT_CN)
        ld.text((5, 49), "线 南昆客专", fill=(13, 16, 18), font=FONT_CN)
    elif kind == "detail":
        ld.text((2, 1), "01/10 2/2", fill=(13, 16, 18), font=FONT_TINY)
        ld.text((5, 9), "类 快速旅客列车", fill=(13, 16, 18), font=FONT_CN)
        ld.text((5, 25), "LON 104123456", fill=(13, 16, 18), font=FONT_ASCII)
        ld.text((5, 41), "LAT 25123456", fill=(13, 16, 18), font=FONT_ASCII)
    else:
        ld.text((2, 1), "00/00 1/2", fill=(13, 16, 18), font=FONT_TINY)
        ld.text((24, 12), "等待 LBJ POCSAG", fill=(13, 16, 18), font=FONT_CN)
        ld.text((24, 31), "等待列车报文", fill=(13, 16, 18), font=FONT_CN)
        ld.text((31, 51), "UP/DN HISTORY", fill=(13, 16, 18), font=FONT_TINY)

    lcd = lcd.resize((PANEL_W*SCALE, PANEL_H*SCALE), Image.Resampling.NEAREST)
    img.paste(lcd, (x, sy))

out = ROOT / "docs" / "lcd_preview.png"
img.save(out)
print(out)
