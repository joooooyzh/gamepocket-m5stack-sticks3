#include <Arduino.h>
#include <M5Unified.h>

#include "shiba_game.h"

#include "crying_shiba_sprite.h"
#include "game_background_image.h"
#include "home_image.h"
#include "level_clear_images.h"
#include "shiba_sprite.h"

namespace {

constexpr uint16_t kSky = 0x967F;
constexpr uint16_t kSkyTop = 0x5DDF;
constexpr uint16_t kGrass = 0x7EEA;
constexpr uint16_t kGrassDark = 0x35A4;
constexpr uint16_t kWood = 0xBC44;
constexpr uint16_t kWoodDark = 0x6200;
constexpr uint16_t kDirt = 0xA2E2;
constexpr uint16_t kDirtDark = 0x6961;
constexpr uint16_t kInk = 0x28C3;
constexpr uint16_t kCream = 0xFFF3;
constexpr uint16_t kRed = 0xE8E4;
constexpr uint16_t kRedDark = 0x9002;
constexpr uint16_t kLeaf = 0x4E67;
constexpr uint16_t kGold = 0xFEA0;
constexpr uint16_t kGoldLight = 0xFF66;
constexpr uint16_t kSpike = 0xF7FF;
constexpr uint16_t kFlag = 0x4E67;

constexpr int kLevelLengthMultiplier = 5;
constexpr int kMaxPlatforms = 24 * kLevelLengthMultiplier;
constexpr int kMaxCoins = 20 * kLevelLengthMultiplier;
constexpr int kMaxSpikes = 16 * kLevelLengthMultiplier;
constexpr int kLevelCount = 3;
constexpr float kGravity = 0.34f;
constexpr float kMoveAccel = 0.34f;
constexpr float kAirAccel = 0.22f;
constexpr float kFriction = 0.86f;
constexpr float kAirFriction = 0.97f;
constexpr float kJumpVelocity = -5.95f;
constexpr int kBallRadius = 9;
constexpr uint32_t kJumpBufferMs = 260;
constexpr uint32_t kCoyoteMs = 140;

struct Rect {
  int x;
  int y;
  int w;
  int h;
};

struct Coin {
  int x;
  int y;
  bool taken;
};

struct Level {
  const char* name;
  int width;
  int platformCount;
  int coinCount;
  int spikeCount;
  Rect platforms[kMaxPlatforms];
  Coin coins[kMaxCoins];
  Rect spikes[kMaxSpikes];
  int startX;
  int startY;
  int goalX;
};

Level levels[kLevelCount] = {
    {
        "SNOW", 1180, 14, 14, 8,
        {{0, 118, 155, 18}, {178, 104, 74, 12}, {286, 92, 70, 12}, {390, 114, 96, 12},
         {526, 98, 72, 12}, {640, 118, 120, 18}, {72, 70, 52, 10}, {468, 66, 52, 10},
         {602, 54, 58, 10}, {770, 100, 82, 12}, {900, 86, 70, 12}, {1012, 108, 88, 12},
         {1110, 118, 90, 18}, {942, 52, 52, 10}},
        {{52, 94, false}, {205, 82, false}, {318, 70, false}, {432, 92, false},
         {558, 76, false}, {92, 48, false}, {492, 44, false}, {632, 32, false},
         {700, 96, false}, {806, 78, false}, {932, 64, false}, {1050, 86, false},
         {1150, 96, false}, {968, 30, false}},
        {{154, 125, 22, 11}, {254, 125, 24, 11}, {358, 125, 28, 11}, {492, 125, 25, 11},
         {608, 125, 22, 11}, {760, 125, 26, 11}, {870, 125, 24, 11}, {1000, 125, 28, 11}},
        24, 88, 1136,
    },
    {
        "SEA", 1420, 18, 16, 11,
        {{0, 118, 120, 18}, {152, 106, 62, 12}, {248, 88, 62, 12}, {346, 72, 56, 12},
         {438, 104, 82, 12}, {558, 88, 58, 12}, {654, 70, 54, 12}, {748, 100, 72, 12},
         {856, 118, 90, 18}, {68, 62, 44, 10}, {514, 48, 46, 10}, {710, 42, 48, 10},
         {970, 96, 64, 12}, {1068, 78, 58, 12}, {1160, 58, 54, 12}, {1250, 94, 76, 12},
         {1360, 118, 90, 18}, {1128, 34, 42, 10}},
        {{78, 96, false}, {178, 84, false}, {276, 66, false}, {374, 50, false},
         {474, 82, false}, {586, 66, false}, {682, 48, false}, {782, 78, false},
         {94, 40, false}, {536, 26, false}, {734, 20, false}, {1000, 74, false},
         {1096, 56, false}, {1188, 36, false}, {1286, 72, false}, {1400, 96, false}},
        {{122, 125, 26, 11}, {218, 125, 28, 11}, {314, 125, 30, 11}, {524, 125, 30, 11},
         {622, 125, 28, 11}, {714, 125, 28, 11}, {824, 125, 28, 11}, {942, 125, 28, 11},
         {1038, 125, 30, 11}, {1218, 125, 30, 11}, {1328, 125, 28, 11}},
        24, 88, 1378,
    },
    {
        "LAKE", 1680, 22, 18, 14,
        {{0, 118, 110, 18}, {142, 100, 58, 12}, {232, 80, 54, 12}, {318, 62, 50, 12},
         {402, 94, 70, 12}, {506, 110, 52, 12}, {598, 88, 52, 12}, {688, 68, 52, 12},
         {780, 96, 62, 12}, {882, 76, 54, 12}, {980, 118, 112, 18}, {66, 54, 40, 10},
         {472, 42, 42, 10}, {842, 40, 44, 10}, {1116, 104, 58, 12}, {1204, 84, 52, 12},
         {1290, 64, 48, 12}, {1374, 92, 58, 12}, {1468, 72, 52, 12}, {1560, 102, 58, 12},
         {1640, 118, 90, 18}, {1328, 36, 42, 10}},
        {{70, 96, false}, {170, 78, false}, {258, 58, false}, {344, 40, false},
         {436, 72, false}, {532, 88, false}, {624, 66, false}, {714, 46, false},
         {810, 74, false}, {908, 54, false}, {494, 20, false}, {864, 18, false},
         {1142, 82, false}, {1230, 62, false}, {1316, 42, false}, {1402, 70, false},
         {1494, 50, false}, {1590, 80, false}},
        {{112, 125, 28, 11}, {202, 125, 28, 11}, {288, 125, 28, 11}, {370, 125, 30, 11},
         {560, 125, 30, 11}, {650, 125, 30, 11}, {742, 125, 30, 11}, {844, 125, 30, 11},
         {938, 125, 32, 11}, {1090, 125, 30, 11}, {1176, 125, 28, 11}, {1260, 125, 28, 11},
         {1434, 125, 30, 11}, {1530, 125, 32, 11}},
        24, 88, 1638,
    },
};

enum class GameState {
  Home,
  Playing,
  Dying,
  LevelClear,
  Victory,
};

struct Ball {
  float x;
  float y;
  float vx;
  float vy;
  bool onGround;
};

M5Canvas canvas(&M5.Display);
Ball ball;
GameState state = GameState::Home;
int screenW = 0;
int screenH = 0;
int levelIndex = 0;
int cameraX = 0;
int score = 0;
int bestScore = 0;
uint32_t stateStarted = 0;
uint32_t lastFrame = 0;
uint32_t jumpQueuedUntil = 0;
uint32_t groundedUntil = 0;
bool audioEnabled = false;
bool exitRequested = false;
bool layoutsExtended = false;

void playTone(uint16_t freq, uint16_t ms, uint8_t channel = 0) {
  if (audioEnabled) {
    M5.Speaker.tone(freq, ms, channel, true);
  }
}

Level& level() {
  return levels[levelIndex];
}

void extendLevelLayouts() {
  if (layoutsExtended) {
    return;
  }

  for (int level = 0; level < kLevelCount; ++level) {
    Level& l = levels[level];
    const int baseWidth = l.width;
    const int baseGoalX = l.goalX;
    const int basePlatformCount = l.platformCount;
    const int baseCoinCount = l.coinCount;
    const int baseSpikeCount = l.spikeCount;

    for (int repeat = 1; repeat < kLevelLengthMultiplier; ++repeat) {
      const int offsetX = baseWidth * repeat;

      for (int i = 0; i < basePlatformCount; ++i) {
        Rect copy = l.platforms[i];
        copy.x += offsetX;
        l.platforms[l.platformCount++] = copy;
      }

      for (int i = 0; i < baseCoinCount; ++i) {
        Coin copy = l.coins[i];
        copy.x += offsetX;
        copy.taken = false;
        l.coins[l.coinCount++] = copy;
      }

      for (int i = 0; i < baseSpikeCount; ++i) {
        Rect copy = l.spikes[i];
        copy.x += offsetX;
        l.spikes[l.spikeCount++] = copy;
      }
    }

    l.width = baseWidth * kLevelLengthMultiplier;
    l.goalX = baseGoalX + baseWidth * (kLevelLengthMultiplier - 1);
  }

  layoutsExtended = true;
}

void setState(GameState next) {
  state = next;
  stateStarted = millis();
}

void resetLevel(bool keepScore) {
  Level& l = level();
  for (int i = 0; i < l.coinCount; ++i) {
    l.coins[i].taken = false;
  }
  if (!keepScore) {
    score = 0;
  }
  ball.x = l.startX;
  ball.y = l.startY;
  ball.vx = 0.0f;
  ball.vy = 0.0f;
  ball.onGround = false;
  jumpQueuedUntil = 0;
  groundedUntil = 0;
  cameraX = 0;
}

bool overlaps(float ax, float ay, float aw, float ah, const Rect& r) {
  return ax < r.x + r.w && ax + aw > r.x && ay < r.y + r.h && ay + ah > r.y;
}

void emitTelemetry(const char* stateName, bool force = false) {
  static uint32_t lastTelemetry = 0;
  uint32_t now = millis();
  if (!force && now - lastTelemetry < 100) {
    return;
  }
  lastTelemetry = now;
  Serial.print("DJ:{\"state\":\"");
  Serial.print(stateName);
  Serial.print("\",\"score\":");
  Serial.print(score);
  Serial.print(",\"best\":");
  Serial.print(bestScore);
  Serial.print(",\"level\":");
  Serial.print(levelIndex + 1);
  Serial.print(",\"alt\":\"");
  Serial.print(level().name);
  Serial.print("\",\"screen\":[");
  Serial.print(screenW);
  Serial.print(',');
  Serial.print(screenH);
  Serial.print("],\"player\":[");
  Serial.print(static_cast<int>(ball.x - cameraX - kBallRadius));
  Serial.print(',');
  Serial.print(static_cast<int>(ball.y - kBallRadius));
  Serial.println("]}");
}

void drawGradientSky() {
  for (int y = 0; y < screenH; ++y) {
    uint8_t mix = static_cast<uint8_t>((y * 31) / max(1, screenH - 1));
    uint16_t color = y < screenH / 2 ? kSkyTop : kSky;
    if (mix % 5 == 0) {
      color = y < screenH / 2 ? 0x65FF : 0xA6BF;
    }
    canvas.drawFastHLine(0, y, screenW, color);
  }
  canvas.fillCircle(screenW - 25, 22, 10, 0xFFE8);
  canvas.fillCircle(screenW - 28, 19, 3, 0xFFF7);
}

void drawCloud(int x, int y) {
  canvas.fillCircle(x, y + 5, 7, kCream);
  canvas.fillCircle(x + 10, y, 9, kCream);
  canvas.fillCircle(x + 22, y + 6, 7, kCream);
  canvas.fillRoundRect(x - 2, y + 6, 32, 8, 4, kCream);
}

void drawDistantScenery() {
  int shift1 = (cameraX / 9) % 180;
  for (int x = -shift1 - 40; x < screenW + 60; x += 90) {
    canvas.fillTriangle(x, 96, x + 44, 47, x + 92, 96, 0x8D94);
    canvas.fillTriangle(x + 18, 96, x + 58, 58, x + 104, 96, 0x6C90);
    canvas.fillTriangle(x + 33, 59, x + 44, 47, x + 57, 61, kCream);
    canvas.fillTriangle(x + 48, 69, x + 58, 58, x + 72, 70, 0xF7FF);
    canvas.drawFastHLine(x + 36, 66, 16, 0xD6DF);
    canvas.drawFastHLine(x + 51, 76, 18, 0xD6DF);
  }
  int houseX = 176 - (cameraX / 4) % 260;
  canvas.fillRect(houseX, 72, 34, 22, 0xE5A6);
  canvas.fillTriangle(houseX - 4, 72, houseX + 17, 54, houseX + 38, 72, 0xB1C3);
  canvas.fillRect(houseX + 6, 80, 7, 14, kWoodDark);
  canvas.fillRect(houseX + 22, 80, 7, 7, 0x867F);
  for (int x = -((cameraX / 3) % 22); x < screenW + 20; x += 22) {
    canvas.fillRect(x, 101, 3, 15, kWoodDark);
    canvas.drawFastHLine(x - 5, 106, 18, kWood);
    canvas.drawFastHLine(x - 5, 112, 18, kWood);
  }
}

void drawPlatform(const Rect& r) {
  int x = r.x - cameraX;
  if (x + r.w < -8 || x > screenW + 8 || r.w <= 0) {
    return;
  }

  if (levelIndex == 0) {
    canvas.fillRoundRect(x - 1, r.y - 1, r.w + 2, r.h + 2, 4, 0x5A20);
    canvas.fillRoundRect(x, r.y + 4, r.w, max(3, r.h - 4), 3, 0x9362);
    canvas.fillRoundRect(x, r.y, r.w, min(7, r.h), 4, 0xF7FF);
    canvas.drawFastHLine(x + 2, r.y + 2, max(0, r.w - 4), TFT_WHITE);
    for (int px = x + 10; px < x + r.w - 4; px += 18) {
      canvas.drawFastVLine(px, r.y + 8, max(0, r.h - 8), 0x6200);
      canvas.drawPixel(px + 5, r.y + 3, 0xBDF7);
    }
    if (r.h > 13) {
      canvas.fillRect(x + 1, r.y + 12, max(0, r.w - 2), r.h - 12, 0x7A61);
    }
    return;
  }

  if (levelIndex == 1) {
    canvas.fillRoundRect(x - 1, r.y - 1, r.w + 2, r.h + 2, 4, 0x9AA0);
    canvas.fillRoundRect(x, r.y, r.w, r.h, 4, 0xF5C8);
    canvas.fillRoundRect(x, r.y, r.w, min(6, r.h), 4, 0xFFE9);
    canvas.drawFastHLine(x + 2, r.y + 2, max(0, r.w - 4), 0xFFFF);
    for (int px = x + 9; px < x + r.w - 4; px += 20) {
      canvas.fillCircle(px, r.y + min(8, r.h - 2), 2, 0xCBA6);
      canvas.drawPixel(px + 5, r.y + min(5, r.h - 2), 0x5EDF);
    }
    if (r.h > 13) {
      canvas.fillRect(x + 1, r.y + 10, max(0, r.w - 2), r.h - 10, 0xDCA5);
    }
    return;
  }

  canvas.fillRoundRect(x - 1, r.y - 1, r.w + 2, r.h + 2, 3, 0x51A0);
  canvas.fillRoundRect(x, r.y, r.w, r.h, 3, 0xD4A2);
  for (int py = r.y + 1; py < r.y + r.h - 1; py += 5) {
    canvas.drawFastHLine(x + 2, py, max(0, r.w - 4), 0xF5C6);
  }
  for (int px = x + 10; px < x + r.w - 4; px += 18) {
    canvas.drawFastVLine(px, r.y, r.h, 0x7A61);
    canvas.drawFastVLine(px + 1, r.y, r.h, 0xF6CA);
  }
  canvas.drawFastHLine(x + 2, r.y + 2, max(0, r.w - 4), 0xFFEE);
  if (r.h > 13) {
    for (int px = x + 6; px < x + r.w; px += 24) {
      canvas.drawLine(px, r.y + 10, px + 8, r.y + 15, 0x6200);
      canvas.drawLine(px + 8, r.y + 10, px, r.y + 15, 0x6200);
    }
  }
}

void drawSpike(const Rect& s) {
  int x = s.x - cameraX;
  if (x + s.w < -8 || x > screenW + 8) {
    return;
  }
  for (int px = 0; px < s.w; px += 8) {
    canvas.fillTriangle(x + px, s.y + s.h, x + px + 4, s.y, x + px + 8, s.y + s.h, kSpike);
    canvas.drawTriangle(x + px, s.y + s.h, x + px + 4, s.y, x + px + 8, s.y + s.h, 0xAD55);
    canvas.drawLine(x + px + 3, s.y + 3, x + px + 5, s.y + s.h - 2, TFT_WHITE);
  }
}

void drawCoin(const Coin& c) {
  if (c.taken) {
    return;
  }
  int x = c.x - cameraX;
  if (x < -10 || x > screenW + 10) {
    return;
  }
  int bob = static_cast<int>((millis() / 180) % 4) - 1;
  int y = c.y + bob;
  canvas.fillCircle(x + 1, y + 1, 6, 0xB3A0);
  canvas.fillCircle(x, y, 6, kGold);
  canvas.fillCircle(x - 1, y - 1, 3, 0xFFE0);
  canvas.drawCircle(x, y, 6, 0x9A80);
  canvas.drawFastVLine(x, y - 4, 8, 0xCBA0);
  if (levelIndex == 0) {
    canvas.drawFastHLine(x - 5, y - 7, 10, 0xDFFF);
    canvas.drawPixel(x - 4, y - 5, TFT_WHITE);
    canvas.drawPixel(x + 4, y - 5, TFT_WHITE);
  } else if (levelIndex == 1) {
    canvas.drawLine(x - 5, y + 4, x - 2, y + 2, 0x5EDF);
    canvas.drawLine(x - 2, y + 2, x + 1, y + 4, 0x5EDF);
    canvas.drawLine(x + 1, y + 4, x + 5, y + 2, 0x5EDF);
    canvas.drawPixel(x + 4, y - 4, TFT_WHITE);
  } else {
    canvas.fillCircle(x - 3, y - 3, 1, 0xFBE0);
    canvas.drawFastHLine(x - 4, y + 5, 8, 0xFD20);
  }
}

void drawGoal() {
  int gx = level().goalX - cameraX;
  if (gx < -30 || gx > screenW + 20) {
    return;
  }
  if (levelIndex == 0) {
    canvas.fillRect(gx - 1, 51, 3, 68, 0x6A20);
    canvas.fillRoundRect(gx - 5, 47, 10, 7, 3, 0xF7FF);
    canvas.fillRoundRect(gx + 2, 56, 30, 19, 4, 0x867F);
    canvas.fillTriangle(gx + 18, 75, gx + 32, 75, gx + 32, 91, 0x867F);
    canvas.drawFastHLine(gx + 6, 62, 15, kCream);
    canvas.drawFastHLine(gx + 10, 66, 7, kCream);
    canvas.drawFastVLine(gx + 13, 59, 12, kCream);
    canvas.fillCircle(gx + 25, 66, 3, kGold);
    return;
  }

  if (levelIndex == 1) {
    canvas.fillRect(gx - 1, 52, 3, 67, 0x9AA0);
    canvas.fillCircle(gx, 51, 3, 0xF5C8);
    canvas.fillRoundRect(gx + 2, 56, 31, 19, 4, 0x05BF);
    canvas.fillTriangle(gx + 19, 75, gx + 33, 75, gx + 33, 91, 0x05BF);
    canvas.drawLine(gx + 6, 66, gx + 12, 62, kCream);
    canvas.drawLine(gx + 12, 62, gx + 18, 66, kCream);
    canvas.drawLine(gx + 18, 66, gx + 25, 62, kCream);
    canvas.fillCircle(gx + 25, 67, 3, kGold);
    canvas.fillCircle(gx + 5, 82, 2, 0xF5C8);
    return;
  }

  canvas.fillRect(gx - 1, 51, 3, 68, 0x7A61);
  canvas.drawFastVLine(gx + 2, 51, 68, 0xF6CA);
  canvas.fillRoundRect(gx + 3, 56, 30, 19, 4, 0xFCA0);
  canvas.fillTriangle(gx + 19, 75, gx + 33, 75, gx + 33, 91, 0xE340);
  canvas.drawFastHLine(gx + 7, 64, 20, 0xFFE0);
  canvas.drawLine(gx + 7, 69, gx + 26, 61, 0x9A80);
  canvas.fillCircle(gx + 25, 67, 3, kGold);
  canvas.drawLine(gx - 4, 76, gx + 5, 80, 0x6200);
  canvas.drawLine(gx + 5, 76, gx - 4, 80, 0x6200);
}

void drawBall() {
  int x = static_cast<int>(ball.x - cameraX);
  int y = static_cast<int>(ball.y);
  canvas.fillEllipse(x, y + kBallRadius + 3, 10, 3, 0x59E2);
  canvas.setSwapBytes(true);
  canvas.pushImage(x - kShibaSpriteWidth / 2, y - kShibaSpriteHeight / 2,
                   kShibaSpriteWidth, kShibaSpriteHeight, kShibaSprite,
                   kShibaSpriteTransparent);
  canvas.setSwapBytes(false);
}

void drawOutlinedText(int x, int y, const char* text, uint16_t fill, uint16_t outline) {
  canvas.setTextDatum(TL_DATUM);
  canvas.setTextColor(outline);
  canvas.setCursor(x - 1, y);
  canvas.print(text);
  canvas.setCursor(x + 1, y);
  canvas.print(text);
  canvas.setCursor(x, y - 1);
  canvas.print(text);
  canvas.setCursor(x, y + 1);
  canvas.print(text);
  canvas.setTextColor(fill);
  canvas.setCursor(x, y);
  canvas.print(text);
}

void drawCenteredOutlinedText(int centerX, int y, const char* text,
                              uint16_t fill, uint16_t outline) {
  drawOutlinedText(centerX - canvas.textWidth(text) / 2, y, text, fill, outline);
}

void drawHud() {
  char text[32];
  canvas.setTextSize(1);

  canvas.fillRoundRect(2, 2, 80, 18, 5, kWoodDark);
  canvas.fillRoundRect(5, 4, 74, 13, 4, kWood);
  canvas.drawRoundRect(6, 5, 72, 11, 3, kGoldLight);
  canvas.fillCircle(9, 9, 2, kGold);
  canvas.fillCircle(75, 9, 2, kGold);
  snprintf(text, sizeof(text), "L%d %s", levelIndex + 1, level().name);
  drawOutlinedText(16, 7, text, kCream, kInk);

  snprintf(text, sizeof(text), "C%03d", score);
  int hudW = max(62, canvas.textWidth(text) + 23);
  int hudX = screenW - hudW - 2;
  canvas.fillRoundRect(hudX, 2, hudW, 18, 5, kWoodDark);
  canvas.fillRoundRect(hudX + 3, 4, hudW - 6, 13, 4, kWood);
  canvas.drawRoundRect(hudX + 4, 5, hudW - 8, 11, 3, kGoldLight);
  canvas.fillCircle(hudX + 12, 10, 5, kGold);
  canvas.drawCircle(hudX + 12, 10, 5, 0xCBA0);
  canvas.drawFastVLine(hudX + 12, 6, 8, 0xCBA0);
  drawOutlinedText(hudX + 22, 7, text, kCream, kInk);
}

void drawPanel(int x, int y, int w, int h) {
  canvas.fillRoundRect(x, y, w, h, 7, kWoodDark);
  canvas.fillRoundRect(x + 3, y + 3, w - 6, h - 6, 5, kWood);
  canvas.drawRoundRect(x + 5, y + 5, w - 10, h - 10, 4, 0xDCE6);
}

void drawCozyTitleScene() {
  drawGradientSky();
  drawDistantScenery();
  Rect ground = {0, 118, screenW, 18};
  int savedCamera = cameraX;
  cameraX = 0;
  drawPlatform(ground);
  cameraX = savedCamera;
  for (int x = 18; x < screenW; x += 36) {
    canvas.fillCircle(x, 112, 2, 0xF81F);
    canvas.fillCircle(x + 5, 114, 2, 0xFFE0);
  }
}

void drawCentered(const char* line1, const char* line2, const char* line3 = nullptr) {
  drawCozyTitleScene();
  drawPanel(26, 30, screenW - 52, 70);
  canvas.setTextDatum(MC_DATUM);
  canvas.setTextColor(kCream);
  canvas.setTextSize(2);
  canvas.drawString(line1, screenW / 2, 50);
  canvas.setTextSize(1);
  canvas.drawString(line2, screenW / 2, 73);
  if (line3 != nullptr) {
    canvas.drawString(line3, screenW / 2, 88);
  }
  canvas.setTextDatum(TL_DATUM);
  canvas.pushSprite(0, 0);
}

void drawWorld() {
  int maxBgScroll = max(0, kGameBackgroundWidth - screenW);
  int maxWorldScroll = max(1, level().width - screenW);
  int bgScroll = constrain((cameraX * maxBgScroll) / maxWorldScroll, 0, maxBgScroll);
  canvas.setSwapBytes(true);
  canvas.pushImage(-bgScroll, (screenH - kGameBackgroundHeight) / 2,
                   kGameBackgroundWidth, kGameBackgroundHeight,
                   kGameBackgroundImages[constrain(levelIndex, 0, kGameBackgroundCount - 1)]);
  canvas.setSwapBytes(false);
  for (int i = 0; i < level().platformCount; ++i) {
    drawPlatform(level().platforms[i]);
  }
  for (int i = 0; i < level().spikeCount; ++i) {
    drawSpike(level().spikes[i]);
  }
  for (int i = 0; i < level().coinCount; ++i) {
    drawCoin(level().coins[i]);
  }
  drawGoal();
  drawBall();
  drawHud();
}

void drawCryingShibaFailure() {
  drawWorld();
  drawPanel(42, 24, screenW - 84, 88);

  int cx = screenW / 2;
  canvas.setSwapBytes(true);
  canvas.pushImage(cx - kCryingShibaSpriteWidth / 2, 31,
                   kCryingShibaSpriteWidth, kCryingShibaSpriteHeight,
                   kCryingShibaSprite, kCryingShibaSpriteTransparent);
  canvas.setSwapBytes(false);

  canvas.fillRoundRect(cx - 48, 94, 96, 22, 6, kWoodDark);
  canvas.fillRoundRect(cx - 44, 97, 88, 16, 5, kWood);
  canvas.drawRoundRect(cx - 42, 99, 84, 12, 4, kGoldLight);
  canvas.fillCircle(cx - 38, 105, 2, kGold);
  canvas.fillCircle(cx + 38, 105, 2, kGold);
  canvas.setTextSize(1);
  drawCenteredOutlinedText(cx, 102, "TRY AGAIN!", kCream, kInk);
  canvas.pushSprite(0, 0);
}

void drawLevelClearScreen() {
  int imageIndex = constrain(levelIndex, 0, kLevelClearImageCount - 1);
  canvas.fillScreen(TFT_BLACK);
  canvas.setSwapBytes(true);
  canvas.pushImage((screenW - kLevelClearImageWidth) / 2,
                   (screenH - kLevelClearImageHeight) / 2,
                   kLevelClearImageWidth, kLevelClearImageHeight,
                   kLevelClearImages[imageIndex]);
  canvas.setSwapBytes(false);
  drawPanel(45, screenH - 29, screenW - 90, 24);
  canvas.setTextDatum(MC_DATUM);
  canvas.setTextSize(1);
  canvas.setTextColor(kCream);
  canvas.drawString(levelIndex == kLevelCount - 1 ? "B HOME   A REPLAY" : "A NEXT   B HOME",
                    screenW / 2, screenH - 17);
  canvas.setTextDatum(TL_DATUM);
  canvas.pushSprite(0, 0);
}

void drawHome() {
  canvas.fillScreen(TFT_BLACK);
  canvas.setSwapBytes(true);
  canvas.pushImage((screenW - kHomeImageWidth) / 2, (screenH - kHomeImageHeight) / 2,
                   kHomeImageWidth, kHomeImageHeight, kHomeImage);
  canvas.setSwapBytes(false);

  drawPanel(77, screenH - 42, 86, 15);
  canvas.setTextDatum(MC_DATUM);
  canvas.setTextColor(kGold);
  canvas.setTextSize(1);
  char label[22];
  snprintf(label, sizeof(label), "L%d  %s", levelIndex + 1, level().name);
  canvas.drawString(label, screenW / 2, screenH - 33);
  canvas.setTextDatum(TL_DATUM);
  canvas.pushSprite(0, 0);
}

void die() {
  playTone(220, 220);
  setState(GameState::Dying);
}

void finishLevel() {
  score += 25;
  bestScore = max(bestScore, score);
  playTone(1319, 120, 0);
  playTone(1760, 180, 1);
  setState(levelIndex == kLevelCount - 1 ? GameState::Victory : GameState::LevelClear);
}

void readInput() {
  float ax = 0.0f;
  float ay = 0.0f;
  float az = 0.0f;
  float move = 0.0f;
  if (M5.Imu.isEnabled() && M5.Imu.getAccel(&ax, &ay, &az)) {
#if defined(DEVIL_JUMP_STICKS3)
    move = -ax * 2.4f;
#else
    move = ax;
#endif
  }
  if (abs(move) < 0.05f) {
    move = 0.0f;
  }

  float accel = ball.onGround ? kMoveAccel : kAirAccel;
  ball.vx += move * accel * 5.8f;

  if (M5.BtnA.wasPressed() || M5.BtnA.isPressed()) {
    jumpQueuedUntil = millis() + kJumpBufferMs;
  }
}

void jumpIfQueued() {
  uint32_t now = millis();
  if (jumpQueuedUntil != 0 && now <= jumpQueuedUntil && (ball.onGround || now <= groundedUntil)) {
    ball.vy = kJumpVelocity;
    ball.onGround = false;
    jumpQueuedUntil = 0;
    groundedUntil = 0;
    playTone(988, 55);
  }
}

void updatePhysics() {
  readInput();

  ball.vy += kGravity;
  ball.vy = min(ball.vy, 7.0f);
  ball.x += ball.vx;

  float half = kBallRadius;
  for (int i = 0; i < level().platformCount; ++i) {
    const Rect& r = level().platforms[i];
    if (!overlaps(ball.x - half, ball.y - half, half * 2, half * 2, r)) {
      continue;
    }
    if (ball.vx > 0.0f) {
      ball.x = r.x - half;
    } else if (ball.vx < 0.0f) {
      ball.x = r.x + r.w + half;
    }
    ball.vx *= -0.18f;
  }

  float previousY = ball.y;
  ball.y += ball.vy;
  ball.onGround = false;
  for (int i = 0; i < level().platformCount; ++i) {
    const Rect& r = level().platforms[i];
    if (!overlaps(ball.x - half, ball.y - half, half * 2, half * 2, r)) {
      continue;
    }
    if (ball.vy >= 0.0f && previousY + half <= r.y + 2) {
      ball.y = r.y - half;
      ball.vy = 0.0f;
      ball.onGround = true;
      groundedUntil = millis() + kCoyoteMs;
    } else if (ball.vy < 0.0f && previousY - half >= r.y + r.h - 2) {
      ball.y = r.y + r.h + half;
      ball.vy = 0.8f;
    }
  }

  ball.vx *= ball.onGround ? kFriction : kAirFriction;
  jumpIfQueued();

  ball.vx = constrain(ball.vx, -5.4f, 5.4f);
  ball.x = constrain(ball.x, static_cast<float>(kBallRadius), static_cast<float>(level().width - kBallRadius));

  for (int i = 0; i < level().coinCount; ++i) {
    Coin& c = level().coins[i];
    if (!c.taken && abs(static_cast<int>(ball.x) - c.x) < 13 && abs(static_cast<int>(ball.y) - c.y) < 14) {
      c.taken = true;
      score += 10;
      bestScore = max(bestScore, score);
      playTone(1568, 45, 1);
    }
  }

  for (int i = 0; i < level().spikeCount; ++i) {
    if (overlaps(ball.x - 6, ball.y - 5, 12, 11, level().spikes[i])) {
      die();
      return;
    }
  }

  if (ball.y > screenH + 22) {
    die();
    return;
  }
  if (ball.x > level().goalX) {
    finishLevel();
    return;
  }

  int target = static_cast<int>(ball.x) - screenW / 2;
  cameraX += (target - cameraX) / 4;
  cameraX = constrain(cameraX, 0, max(0, level().width - screenW));
}

const char* stateName() {
  switch (state) {
    case GameState::Home:
      return "home";
    case GameState::Playing:
      return "play";
    case GameState::Dying:
      return "dying";
    case GameState::LevelClear:
      return "clear";
    case GameState::Victory:
      return "victory";
  }
  return "home";
}

void startGame() {
  resetLevel(false);
  playTone(880, 80);
  setState(GameState::Playing);
}

}  // namespace

namespace ShibaGame {

void begin() {
  exitRequested = false;
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;
#if defined(DEVIL_JUMP_STICKS3)
  cfg.fallback_board = m5::board_t::board_M5StickS3;
#endif
  M5.begin(cfg);
  M5.Display.setRotation(1);
  screenW = M5.Display.width();
  screenH = M5.Display.height();
  canvas.createSprite(screenW, screenH);
  canvas.setTextDatum(TL_DATUM);
  M5.Speaker.begin();
  audioEnabled = M5.Speaker.isEnabled();
  if (audioEnabled) {
    M5.Speaker.setVolume(72);
  }
  extendLevelLayouts();
  resetLevel(false);
  setState(GameState::Home);
}

void update() {
  M5.update();
  uint32_t now = millis();

  if (state == GameState::Home) {
    drawHome();
    emitTelemetry("home");
    if (M5.BtnB.wasPressed()) {
      levelIndex = (levelIndex + 1) % kLevelCount;
      resetLevel(false);
      playTone(660, 45);
      emitTelemetry("home", true);
    }
    if (M5.BtnA.wasPressed()) {
      startGame();
    }
    delay(20);
    return;
  }

  if (state == GameState::Dying) {
    drawCryingShibaFailure();
    emitTelemetry(stateName());
    if (M5.BtnB.wasPressed()) {
      exitRequested = true;
      playTone(494, 60);
      delay(20);
      return;
    }
    if (now - stateStarted > 760) {
      resetLevel(true);
      setState(GameState::Playing);
    }
    delay(20);
    return;
  }

  if (state == GameState::LevelClear) {
    drawLevelClearScreen();
    emitTelemetry(stateName());
    if (M5.BtnB.wasPressed()) {
      exitRequested = true;
      playTone(494, 60);
      delay(20);
      return;
    }
    if (M5.BtnA.wasPressed()) {
      levelIndex = (levelIndex + 1) % kLevelCount;
      resetLevel(true);
      setState(GameState::Playing);
    }
    delay(20);
    return;
  }

  if (state == GameState::Victory) {
    drawLevelClearScreen();
    emitTelemetry(stateName());
    if (M5.BtnB.wasPressed()) {
      exitRequested = true;
      playTone(494, 60);
      delay(20);
      return;
    }
    if (M5.BtnA.wasPressed()) {
      resetLevel(false);
      setState(GameState::Playing);
    }
    delay(20);
    return;
  }

  if (now - lastFrame < 16) {
    delay(1);
    return;
  }
  if (M5.BtnB.wasPressed()) {
    exitRequested = true;
    playTone(494, 60);
    delay(20);
    return;
  }
  lastFrame = now;
  updatePhysics();
  drawWorld();
  canvas.pushSprite(0, 0);
  emitTelemetry(stateName());
}

bool wantsExit() {
  return exitRequested;
}

}  // namespace ShibaGame
