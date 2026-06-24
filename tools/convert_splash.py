from pathlib import Path

from PIL import Image, ImageOps


ROOT = Path(__file__).resolve().parents[1]
WIDTH = 135
HEIGHT = 240
PLAYER_WIDTH = 30
PLAYER_HEIGHT = 34
PLAYER_KEY_RGB565 = 0xF81F

IMAGES = [
    {
        "input": ROOT / "assets" / "devil-jump-home.png",
        "output": ROOT / "src" / "splash_image.h",
        "width_name": "kSplashWidth",
        "height_name": "kSplashHeight",
        "array_name": "kSplashImage",
    },
    {
        "input": ROOT / "assets" / "devil-jump-gameover.png",
        "output": ROOT / "src" / "gameover_image.h",
        "width_name": "kGameOverWidth",
        "height_name": "kGameOverHeight",
        "array_name": "kGameOverImage",
    },
    {
        "input": ROOT / "assets" / "devil-jump-failure.png",
        "output": ROOT / "src" / "failure_image.h",
        "width_name": "kFailureWidth",
        "height_name": "kFailureHeight",
        "array_name": "kFailureImage",
    },
    {
        "input": ROOT / "assets" / "devil-jump-score-bg.png",
        "output": ROOT / "src" / "score_image.h",
        "width_name": "kScoreWidth",
        "height_name": "kScoreHeight",
        "array_name": "kScoreImage",
    },
    {
        "input": ROOT / "assets" / "devil-jump-victory.png",
        "output": ROOT / "src" / "victory_image.h",
        "width_name": "kVictoryWidth",
        "height_name": "kVictoryHeight",
        "array_name": "kVictoryImage",
    },
]

LEVEL_BACKGROUNDS = [
    ROOT / "assets" / "level-1-sky.png",
    ROOT / "assets" / "level-2-cloud.png",
    ROOT / "assets" / "level-3-sunset.png",
    ROOT / "assets" / "level-4-dusk.png",
    ROOT / "assets" / "level-5-high.png",
    ROOT / "assets" / "level-6-stars.png",
    ROOT / "assets" / "level-7-space.png",
]


def rgb565(pixel):
    r, g, b = pixel[:3]
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def image_pixels(path):
    image = Image.open(path).convert("RGB")
    image = ImageOps.fit(image, (WIDTH, HEIGHT), method=Image.Resampling.LANCZOS, centering=(0.5, 0.5))
    return [rgb565(image.getpixel((x, y))) for y in range(HEIGHT) for x in range(WIDTH)]


def player_pixels(path):
    image = Image.open(path).convert("RGB")
    key = (255, 0, 255)
    pixels = image.load()
    min_x, min_y = image.width, image.height
    max_x, max_y = 0, 0

    for y in range(image.height):
        for x in range(image.width):
            r, g, b = pixels[x, y]
            if not (r > 245 and b > 245 and g < 25):
                min_x = min(min_x, x)
                min_y = min(min_y, y)
                max_x = max(max_x, x)
                max_y = max(max_y, y)

    cropped = image.crop((min_x, min_y, max_x + 1, max_y + 1))
    cropped = ImageOps.contain(cropped, (PLAYER_WIDTH, PLAYER_HEIGHT), method=Image.Resampling.LANCZOS)
    sprite = Image.new("RGB", (PLAYER_WIDTH, PLAYER_HEIGHT), key)
    sprite.paste(cropped, ((PLAYER_WIDTH - cropped.width) // 2, (PLAYER_HEIGHT - cropped.height) // 2))

    output = []
    for y in range(PLAYER_HEIGHT):
        for x in range(PLAYER_WIDTH):
            r, g, b = sprite.getpixel((x, y))
            magenta_strength = min(r, b) - g
            if r > 210 and b > 210 and g < 90 and magenta_strength > 140:
                output.append(PLAYER_KEY_RGB565)
                continue
            if r > 160 and b > 160 and g < 120 and abs(r - b) < 70:
                output.append(PLAYER_KEY_RGB565)
                continue
            output.append(rgb565((r, g, b)))
    return output


def append_pixel_array(lines, array_name, pixels):
    lines.append(f"const uint16_t {array_name}[] PROGMEM = {{")
    for i in range(0, len(pixels), 10):
        chunk = pixels[i : i + 10]
        values = ", ".join(f"0x{value:04X}" for value in chunk)
        lines.append(f"  {values},")
    lines.append("};")
    lines.append("")


def convert(spec):
    pixels = image_pixels(spec["input"])

    lines = [
        "#pragma once",
        "",
        "#include <Arduino.h>",
        "",
        f"constexpr int {spec['width_name']} = {WIDTH};",
        f"constexpr int {spec['height_name']} = {HEIGHT};",
        "",
    ]
    append_pixel_array(lines, spec["array_name"], pixels)
    spec["output"].write_text("\n".join(lines), encoding="utf-8")


def convert_level_backgrounds():
    lines = [
        "#pragma once",
        "",
        "#include <Arduino.h>",
        "",
        f"constexpr int kLevelBackgroundWidth = {WIDTH};",
        f"constexpr int kLevelBackgroundHeight = {HEIGHT};",
        "constexpr int kLevelBackgroundCount = 7;",
        "",
    ]

    names = []
    for index, path in enumerate(LEVEL_BACKGROUNDS, start=1):
        name = f"kLevelBackground{index}"
        names.append(name)
        append_pixel_array(lines, name, image_pixels(path))

    lines.append("const uint16_t* const kLevelBackgrounds[] = {")
    for name in names:
        lines.append(f"  {name},")
    lines.append("};")
    lines.append("")
    (ROOT / "src" / "level_backgrounds.h").write_text("\n".join(lines), encoding="utf-8")


def convert_player_sprite():
    lines = [
        "#pragma once",
        "",
        "#include <Arduino.h>",
        "",
        f"constexpr int kPlayerSpriteWidth = {PLAYER_WIDTH};",
        f"constexpr int kPlayerSpriteHeight = {PLAYER_HEIGHT};",
        f"constexpr uint16_t kPlayerSpriteTransparent = 0x{PLAYER_KEY_RGB565:04X};",
        "",
    ]
    append_pixel_array(lines, "kPlayerSprite", player_pixels(ROOT / "assets" / "player-devil.png"))
    (ROOT / "src" / "player_sprite.h").write_text("\n".join(lines), encoding="utf-8")


def main():
    for spec in IMAGES:
        convert(spec)
    convert_level_backgrounds()
    convert_player_sprite()


if __name__ == "__main__":
    main()
