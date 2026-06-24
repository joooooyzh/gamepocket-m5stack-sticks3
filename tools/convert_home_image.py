from pathlib import Path

from PIL import Image, ImageOps


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_WIDTH = 240
DEFAULT_HEIGHT = 135

IMAGES = [
    {
        "input": ROOT / "assets" / "gamepocket-home-v4.png",
        "output": ROOT / "src" / "gamepocket_home_image.h",
        "width_name": "kGamePocketHomeImageWidth",
        "height_name": "kGamePocketHomeImageHeight",
        "array_name": "kGamePocketHomeImage",
        "width": 135,
        "height": 240,
    },
    {
        "input": ROOT / "assets" / "game-select-home.png",
        "output": ROOT / "src" / "launcher_home_image.h",
        "width_name": "kLauncherHomeImageWidth",
        "height_name": "kLauncherHomeImageHeight",
        "array_name": "kLauncherHomeImage",
        "width": 240,
        "height": 135,
    },
    {
        "input": [
            ROOT / "assets" / "launcher-card-shiba.png",
            ROOT / "assets" / "launcher-card-devil.png",
        ],
        "output": ROOT / "src" / "launcher_card_images.h",
        "width_name": "kLauncherCardImageWidth",
        "height_name": "kLauncherCardImageHeight",
        "array_name": "kLauncherCardImage",
        "count_name": "kLauncherCardImageCount",
        "width": 135,
        "height": 240,
    },
    {
        "input": ROOT / "assets" / "shiba-coin-dash-home.png",
        "output": ROOT / "src" / "home_image.h",
        "width_name": "kHomeImageWidth",
        "height_name": "kHomeImageHeight",
        "array_name": "kHomeImage",
        "width": 240,
        "height": 135,
    },
    {
        "input": [
            ROOT / "assets" / "level-1-snow-mountain-wide.png",
            ROOT / "assets" / "level-2-seaside-wide.png",
            ROOT / "assets" / "level-3-sunset-lake-wide.png",
        ],
        "output": ROOT / "src" / "game_background_image.h",
        "width_name": "kGameBackgroundWidth",
        "height_name": "kGameBackgroundHeight",
        "array_name": "kGameBackgroundImage",
        "count_name": "kGameBackgroundCount",
        "width": 480,
        "height": 135,
    },
    {
        "input": [
            ROOT / "assets" / "level-clear-snow.png",
            ROOT / "assets" / "level-clear-sea.png",
            ROOT / "assets" / "level-clear-lake.png",
        ],
        "output": ROOT / "src" / "level_clear_images.h",
        "width_name": "kLevelClearImageWidth",
        "height_name": "kLevelClearImageHeight",
        "array_name": "kLevelClearImage",
        "count_name": "kLevelClearImageCount",
        "width": 240,
        "height": 135,
    },
]


def rgb565(pixel):
    r, g, b = pixel[:3]
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def trim_black_border(image):
    pixels = image.load()
    min_x, min_y = image.width, image.height
    max_x, max_y = 0, 0
    for y in range(image.height):
        for x in range(image.width):
            r, g, b = pixels[x, y]
            if max(r, g, b) > 18:
                min_x = min(min_x, x)
                min_y = min(min_y, y)
                max_x = max(max_x, x)
                max_y = max(max_y, y)
    if max_x <= min_x or max_y <= min_y:
        return image
    return image.crop((min_x, min_y, max_x + 1, max_y + 1))


def append_array(lines, array_name, values):
    lines.append(f"const uint16_t {array_name}[] PROGMEM = {{")
    for i in range(0, len(values), 10):
        lines.append("  " + ", ".join(f"0x{value:04X}" for value in values[i : i + 10]) + ",")
    lines.extend(["};", ""])


def pixels_for(path, width, height):
    image = Image.open(path).convert("RGB")
    image = trim_black_border(image)
    image = ImageOps.fit(image, (width, height), method=Image.Resampling.LANCZOS, centering=(0.5, 0.5))
    return [rgb565(image.getpixel((x, y))) for y in range(height) for x in range(width)]


def convert(spec):
    width = spec.get("width", DEFAULT_WIDTH)
    height = spec.get("height", DEFAULT_HEIGHT)
    inputs = spec["input"] if isinstance(spec["input"], list) else [spec["input"]]

    lines = [
        "#pragma once",
        "",
        "#include <Arduino.h>",
        "",
        f"constexpr int {spec['width_name']} = {width};",
        f"constexpr int {spec['height_name']} = {height};",
        *( [f"constexpr int {spec['count_name']} = {len(inputs)};"] if "count_name" in spec else [] ),
        "",
    ]

    names = []
    for index, path in enumerate(inputs, start=1):
        array_name = spec["array_name"] if len(inputs) == 1 else f"{spec['array_name']}{index}"
        names.append(array_name)
        append_array(lines, array_name, pixels_for(path, width, height))

    if len(inputs) > 1:
        lines.append(f"const uint16_t* const {spec['array_name']}s[] = {{")
        for name in names:
            lines.append(f"  {name},")
        lines.extend(["};", ""])
    spec["output"].write_text("\n".join(lines), encoding="utf-8")


def main():
    for spec in IMAGES:
        convert(spec)


if __name__ == "__main__":
    main()
