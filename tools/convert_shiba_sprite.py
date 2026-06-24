from pathlib import Path

from PIL import Image, ImageOps


ROOT = Path(__file__).resolve().parents[1]
KEY_RGB565 = 0xF81F

SPRITES = [
    {
        "input": ROOT / "assets" / "shiba-ball-source.png",
        "preview": ROOT / "assets" / "shiba-ball-sprite.png",
        "output": ROOT / "src" / "shiba_sprite.h",
        "width": 28,
        "height": 28,
        "width_name": "kShibaSpriteWidth",
        "height_name": "kShibaSpriteHeight",
        "transparent_name": "kShibaSpriteTransparent",
        "array_name": "kShibaSprite",
    },
    {
        "input": ROOT / "assets" / "crying-shiba-source.png",
        "preview": ROOT / "assets" / "crying-shiba-sprite.png",
        "output": ROOT / "src" / "crying_shiba_sprite.h",
        "width": 64,
        "height": 64,
        "width_name": "kCryingShibaSpriteWidth",
        "height_name": "kCryingShibaSpriteHeight",
        "transparent_name": "kCryingShibaSpriteTransparent",
        "array_name": "kCryingShibaSprite",
    },
]


def rgb565(pixel):
    r, g, b = pixel[:3]
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def is_green_key(pixel):
    r, g, b, a = pixel
    return a < 20 or (g > 165 and r < 95 and b < 105 and g > r * 1.8 and g > b * 1.8)


def convert_sprite(spec):
    width = spec["width"]
    height = spec["height"]
    image = Image.open(spec["input"]).convert("RGBA")
    pixels = image.load()
    min_x, min_y = image.width, image.height
    max_x, max_y = 0, 0

    for y in range(image.height):
        for x in range(image.width):
            if not is_green_key(pixels[x, y]):
                min_x = min(min_x, x)
                min_y = min(min_y, y)
                max_x = max(max_x, x)
                max_y = max(max_y, y)

    cropped = image.crop((min_x, min_y, max_x + 1, max_y + 1))
    cropped = ImageOps.contain(cropped, (width - 2, height - 2), method=Image.Resampling.LANCZOS)

    sprite = Image.new("RGBA", (width, height), (255, 0, 255, 255))
    sprite.alpha_composite(cropped, ((width - cropped.width) // 2, (height - cropped.height) // 2))

    preview = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    preview.alpha_composite(cropped, ((width - cropped.width) // 2, (height - cropped.height) // 2))
    preview.save(spec["preview"])

    values = []
    for y in range(height):
        for x in range(width):
            r, g, b, a = sprite.getpixel((x, y))
            if a < 32 or is_green_key((r, g, b, a)) or (r > 225 and b > 225 and g < 80):
                values.append(KEY_RGB565)
            else:
                values.append(rgb565((r, g, b)))

    lines = [
        "#pragma once",
        "",
        "#include <Arduino.h>",
        "",
        f"constexpr int {spec['width_name']} = {width};",
        f"constexpr int {spec['height_name']} = {height};",
        f"constexpr uint16_t {spec['transparent_name']} = 0x{KEY_RGB565:04X};",
        "",
        f"const uint16_t {spec['array_name']}[] PROGMEM = {{",
    ]
    for i in range(0, len(values), 10):
        lines.append("  " + ", ".join(f"0x{value:04X}" for value in values[i : i + 10]) + ",")
    lines.extend(["};", ""])
    spec["output"].write_text("\n".join(lines), encoding="utf-8")


def main():
    for spec in SPRITES:
        convert_sprite(spec)


if __name__ == "__main__":
    main()
