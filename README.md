# M5StickS3 Mini Game Launcher / 小游戏合集

一个烧录到同一台 M5StickS3 上的双游戏合集。开机先进入竖屏游戏选择首页，每个游戏单独显示成一张大卡片；进入后会到对应游戏自己的首页。

## 游戏

- **Shiba Coin Dash / 柴犬金币冲刺**：横向金币闯关小游戏，倾斜设备控制圆滚滚柴犬，收集金币、躲避尖刺、到达旗帜过关。
- **Devil Jump / 恶魔跳跃**：竖屏涂鸦跳跃风格小游戏，小恶魔自动弹跳，倾斜或按键左右移动，尽量向上挑战更高关卡。
  新版加入 GPT Image 生成风格的新怪物、云朵/草地/蘑菇/冰晶/星空/碎裂横杠，以及可拾取的小火箭背包；碰到怪物会死亡，达到 200000 分才触发成功彩蛋页，结束后会显示本局比分详情。

## 硬件

- M5StickS3
- USB 数据线

## 编译和烧录

推荐使用 PlatformIO：

```bash
pio run -e m5sticks3 -t upload
```

如果只想先编译：

```bash
pio run -e m5sticks3
```

串口监视：

```bash
pio device monitor
```

## 操作

启动器首页：

- B：切换游戏卡片
- A：进入选中的游戏

柴犬金币冲刺：

- 柴犬首页按 A：开始，首页图中显示为 `A PLAY`
- 柴犬首页按 B：切换关卡，首页图中显示为 `B STAGE`
- 横屏握持时上下倾斜 StickS3：向上倾斜向左滚，向下倾斜向右滚
- 游戏中按 A：跳跃
- 游戏中按 B：返回游戏机首页
- 通关后按 A 进入下一关，按 B 返回游戏机首页

恶魔跳跃：

- 恶魔首页按 A：直接进入简单模式
- 恶魔首页按 B：直接进入挑战模式
- 左右倾斜：移动角色
- 简单模式只有基础跳跃；挑战模式会出现火箭、怪物和会断掉的横杠
- 挑战模式中，3000 分后怪物会按分数随机出现，分数越高越难；按 A 连续射击可以打掉怪物，跳跃碰到没打掉的怪物会失败
- 挑战模式中，火箭会在 5000、20000、50000 分附近随机出现，拾取后会向上飞并推进 5000-10000 随机分数
- 挑战模式中，裂纹横杠踩上后会突然碎掉，只能借力一次
- 关卡长度已加长，分数达到 200000 才触发成功彩蛋页
- 死亡后会先进入短暂受击状态，再进入 SCORE 分数页，显示本局分数、最好成绩对比、火箭数、击杀数、碎裂横杠数、最高关卡和高度
- 游戏中按 B：返回游戏机首页
- 游戏结束或通关后按 A 回到恶魔首页，按 B 返回游戏机首页

## 文件

- `src/main.cpp`：竖屏 GamePocket 首页和游戏选择卡片
- `src/shiba_game.cpp` / `src/shiba_game.h`：柴犬金币冲刺模块
- `src/devil_game.cpp` / `src/devil_game.h`：恶魔跳跃模块
- `src/launcher_card_images.h`：GPT Image 两张竖屏游戏卡片转换出的 135x240 固件图片数据
- `src/gamepocket_home_image.h`、`assets/gamepocket-home-v4.png`：GPT Image 生成的 GamePocket 竖屏首页，开机后按 A 进入游戏选择页
- `assets/launcher-card-shiba.png`：GPT Image 生成的柴犬游戏选择卡片源图
- `assets/launcher-card-devil.png`：GPT Image 生成的恶魔跳跃选择卡片源图
- `assets/devil-monster-bullet-style-guide.png`：GPT Image 生成的恶魔跳跃怪物和子弹风格参考图
- `assets/devil-jump-powerup-platform-spritesheet.png`：GPT Image 生成的新怪物、平台形态和小火箭素材参考图
- `assets/devil-jump-results-crumble-style-guide.png`：GPT Image 生成的碎裂横杠和比分详情结算卡参考图
- `assets/devil-jump-score-page-design.png`：GPT Image 生成的 SCORE 分数页视觉设计参考图
- `assets/devil-jump-score-bg.png`：GPT Image 生成的简化 SCORE 分数页固件背景图
- `assets/devil-jump-death-animation-design.png`：GPT Image 生成的死亡过渡动效关键帧参考图
- `assets/devil-jump-failure.png`：GPT Image 生成的恶魔跳跃失败状态固件背景图
- `src/launcher_home_image.h`、`assets/game-select-home.png`：旧横屏启动器素材，当前入口不再使用
- `src/home_image.h`、`src/game_background_image.h`、`src/level_clear_images.h`、`src/shiba_sprite.h`、`src/crying_shiba_sprite.h`：柴犬游戏素材头文件
- `assets/text-hud-style-guide.png`：GPT Image 生成的柴犬 HUD 和失败提示文字风格参考图
- `src/splash_image.h`、`src/gameover_image.h`、`src/victory_image.h`、`src/level_backgrounds.h`、`src/player_sprite.h`：恶魔跳跃素材头文件
- `tools/convert_home_image.py`：把启动器、柴犬首页、柴犬背景和通关图转换为固件头文件
- `tools/convert_shiba_sprite.py`：把柴犬 sprite 源图转换为固件头文件
- `tools/convert_splash.py`：恶魔跳跃图片转换脚本
- `tools/serial_mirror.py`：电脑端串口桥接服务，保留用于调试遥测
