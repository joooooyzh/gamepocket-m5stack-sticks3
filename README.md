# M5Stick Jump

一个适合 M5StickC / M5StickC Plus 的小屏幕跳跃游戏，玩法类似“涂鸦跳跃”：小恶魔会自动弹跳，玩家通过倾斜设备或 A/B 按键左右移动，尽量一直向上。

游戏现在带有由 imagegen 设计的 `DEVIL JUMP` 小恶魔首页、Game Over 页面、通关 SUCCESS 页面和 7 张关卡背景。关卡越高，平台越窄、平台间距越大、跳跃和横向移动节奏越快，背景也会从蓝天逐渐跳到太空。
游戏还带有轻量 8-bit 音效：开始、踩到横杆、失败和成功时会播放短提示音。

## 硬件

- M5StickC 或 M5StickC Plus
- USB 数据线

## 编译和烧录

推荐使用 PlatformIO：

```bash
pio run -t upload
```

串口监视：

```bash
pio device monitor
```

电脑同步画面推荐使用串口桥接：

```bash
python3 tools/serial_mirror.py --serial /dev/cu.usbserial-6952522CF5 --http 8766 --host 127.0.0.1
```

然后打开：

```text
http://127.0.0.1:8766/mirror/
```

网页镜像和 `pio device monitor` 不能同时占用同一个串口。

## 操作

- 开机画面：默认困难 5 星
- 开机画面按 A：切换难度，顺序为 EASY、STANDARD、HARD 1 星到 HARD 7 星
- 开机画面按 B：开始游戏
- 左右倾斜：移动角色
- A 键：向左微调
- B 键：向右微调
- 游戏结束后按 A 或 B 回到首页，再按 B 开始
- 通关后小恶魔会先长出翅膀飞到顶部，再显示变天使的 SUCCESS 页面和撒花动画；按 A 或 B 回到首页

## 关卡

- L1：入门，平台宽，间距小
- L2：高云层，平台逐渐变窄
- L3：夕阳高度，开始更考验落点
- L4：暮色天空，间距继续变大
- L5：平流层，节奏更快
- L6：星层，平台更窄
- L7：太空，速度最快。通关分数会随难度变化，默认困难 5 星约 2450 分通关

## 难度

- EASY：平台更宽、间距更小、重力更轻，适合练习
- STANDARD：标准节奏，平台和跳跃节奏适中
- HARD：1 到 7 星，星星越多平台越窄、间距越大、重力和速度越强、通关高度越高

## 文件

- `platformio.ini`：PlatformIO 工程配置
- `src/main.cpp`：游戏主程序
- `src/splash_image.h`：首页图的 RGB565 固件数据
- `src/gameover_image.h`：Game Over 图的 RGB565 固件数据
- `src/victory_image.h`：通关 SUCCESS 图的 RGB565 固件数据
- `src/level_backgrounds.h`：7 张关卡背景的 RGB565 固件数据
- `src/player_sprite.h`：游戏中跳跃小恶魔的 RGB565 透明 sprite 数据
- `mirror/index.html`：电脑端同步显示页面
- `mirror/app.js`：通过 Web Serial 接收 M5Stick 游戏状态并重绘画面
- `mirror/styles.css`：电脑端同步显示页面样式
- `assets/devil-jump-home.png`：由 imagegen 生成的 `DEVIL JUMP` 首页设计源图
- `assets/devil-jump-gameover.png`：由 imagegen 生成的小恶魔哭泣 Game Over 设计源图
- `assets/devil-jump-victory.png`：由 imagegen 生成的小恶魔变天使通关设计源图
- `assets/player-devil.png`：由 imagegen 生成的跳跃小恶魔 sprite 源图
- `assets/player-devil-clean.png`：电脑镜像页面使用的透明抠图小恶魔 sprite
- `assets/level-*.png`：由 imagegen 生成的 7 张关卡背景源图
- `assets/little-devil-splash.png`：早期小恶魔设计源图
- `tools/convert_splash.py`：把图片转换为 M5Stick 可用头文件的脚本
- `tools/serial_mirror.py`：电脑端串口桥接服务
