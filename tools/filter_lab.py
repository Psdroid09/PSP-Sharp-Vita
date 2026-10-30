#!/usr/bin/env python3
"""Offline PSP image scaling/sharpness comparison lab.

Requires: Pillow (python -m pip install Pillow)
No console connection, plugins, or PSP files are touched.
"""
from __future__ import annotations

import argparse
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, ImageFilter, ImageChops, ImageStat

SOURCE_SIZE = (480, 272)
SCALE = 2


def synthetic_reference() -> Image.Image:
    """Create a deterministic 480x272 pattern with edges, text and fine detail."""
    im = Image.new("RGB", SOURCE_SIZE, (24, 28, 36))
    d = ImageDraw.Draw(im)
    # Smooth gradients and broad shapes.
    for y in range(272):
        c = int(28 + 65 * y / 271)
        d.line((0, y, 479, y), fill=(c // 2, c, min(255, c + 30)))
    d.rounded_rectangle((14, 14, 222, 118), radius=9, fill=(235, 180, 65), outline=(255, 245, 210), width=2)
    d.ellipse((250, 16, 360, 126), fill=(70, 170, 220), outline=(235, 245, 255), width=3)
    d.polygon([(385, 18), (460, 120), (340, 120)], fill=(220, 85, 95), outline=(255, 235, 235))
    # Fine line pairs and checkerboard reveal blur/aliasing.
    for x in range(14):
        xx = 16 + x * 13
        d.line((xx, 142, xx, 205), fill=(245, 245, 245) if x % 2 else (20, 20, 20), width=1)
    for y in range(8):
        for x in range(16):
            col = (240, 240, 240) if (x + y) % 2 else (22, 22, 22)
            d.rectangle((225 + x * 7, 143 + y * 7, 231 + x * 7, 149 + y * 7), fill=col)
    # Diagonals and circles help reveal ringing/halos.
    for offset in range(0, 80, 5):
        d.line((15 + offset, 260, 95 + offset, 210), fill=(255, 255, 255), width=1)
    d.ellipse((365, 155, 450, 240), outline=(255, 255, 255), width=1)
    d.ellipse((375, 165, 440, 230), outline=(15, 15, 15), width=1)
    try:
        font = ImageFont.truetype("DejaVuSans.ttf", 16)
    except OSError:
        font = ImageFont.load_default()
    d.text((18, 218), "PSP SHARP TEST 480x272", font=font, fill=(255, 255, 255))
    return im


def make_variants(src: Image.Image, size: tuple[int, int]) -> dict[str, Image.Image]:
    nearest = src.resize(size, Image.Resampling.NEAREST)
    bilinear = src.resize(size, Image.Resampling.BILINEAR)
    # A conservative practical approximation, not a bit-exact shader port.
    sharp_bilinear_approx = bilinear.filter(
        ImageFilter.UnsharpMask(radius=0.55, percent=65, threshold=4)
    )
    unsharp = bilinear.filter(
        ImageFilter.UnsharpMask(radius=1.15, percent=135, threshold=3)
    )
    return {
        "Nearest neighbour": nearest,
        "Bilinear": bilinear,
        "Sharp-bilinear (mild USM approximation)": sharp_bilinear_approx,
        "Unsharp mask (stronger)": unsharp,
    }


def build_sheet(variants: dict[str, Image.Image], output: Path) -> None:
    tile_w, tile_h = next(iter(variants.values())).size
    label_h, margin, gap = 34, 18, 16
    sheet = Image.new("RGB", (margin * 2 + tile_w * 2 + gap,
                              margin * 2 + (tile_h + label_h) * 2 + gap),
                      (18, 20, 25))
    draw = ImageDraw.Draw(sheet)
    try:
        font = ImageFont.truetype("DejaVuSans.ttf", 13)
    except OSError:
        font = ImageFont.load_default()
    for idx, (name, image) in enumerate(variants.items()):
        col, row = idx % 2, idx // 2
        x = margin + col * (tile_w + gap)
        y = margin + row * (tile_h + label_h + gap)
        sheet.paste(image, (x, y + label_h))
        draw.text((x, y + 8), name, font=font, fill=(245, 245, 245))
    sheet.save(output)


def edge_indicator(image: Image.Image) -> float:
    """Mean absolute difference from a one-pixel horizontal shift; descriptive only."""
    gray = image.convert("L")
    shifted = Image.new("L", gray.size)
    shifted.paste(gray.crop((1, 0, gray.width, gray.height)), (0, 0))
    diff = ImageChops.difference(gray, shifted)
    return round(ImageStat.Stat(diff).mean[0], 3)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", nargs="?", help="Optional source image (ideally 480x272)")
    parser.add_argument("--synthetic", action="store_true", help="Use generated test pattern")
    parser.add_argument("--output", default="psp-sharp-comparison.png", help="Output comparison PNG")
    parser.add_argument("--scale", type=int, default=SCALE, choices=(2, 3, 4))
    args = parser.parse_args()

    if args.synthetic or not args.image:
        src = synthetic_reference()
        source_label = "synthetic 480x272 reference"
    else:
        src = Image.open(args.image).convert("RGB")
        source_label = str(args.image)

    target = (src.width * args.scale, src.height * args.scale)
    variants = make_variants(src, target)
    output = Path(args.output)
    build_sheet(variants, output)

    print(f"Source: {source_label} ({src.width}x{src.height})")
    print(f"Output: {output.resolve()} ({target[0]}x{target[1]} per method)")
    print("Horizontal edge indicator (higher is not automatically better):")
    for name, image in variants.items():
        print(f"  {name}: {edge_indicator(image)}")
    print("Note: edge indicator is descriptive, not an objective image-quality score.")
    print("Sharp-bilinear is represented by a mild unsharp-mask approximation, not an exact shader.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
