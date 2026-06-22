#include <Arduino.h>
#include <M5Unified.h>

#include "gameover_image.h"
#include "level_backgrounds.h"
#include "player_sprite.h"
#include "splash_image.h"
#include "victory_image.h"

namespace {

constexpr uint16_t kSky = 0x0D5A;
constexpr uint16_t kInk = 0x2104;
constexpr uint16_t kDanger = 0xF800;
constexpr uint16_t kDevil = 0xF986;
constexpr uint16_t kDevilDark = 0x9000;
constexpr uint16_t kHorn = 0xFEE0;
constexpr uint16_t kWing = 0xB104;

constexpr int kPlatformCount = 9;
constexpr int kBaseWinScore = 2450;
constexpr float kMoveAccel = 0.24f;
constexpr float kFriction = 0.94f;
constexpr uint16_t kConfettiColors[] = {
    0xF800, 0xFFE0, 0x07E0, 0x07FF, 0xF81F, 0xFD20, 0xFFFF,
};

struct LevelRule {
  int scoreStart;
  float gravity;
  float jumpVelocity;
  float maxVX;
  float gapMinRatio;
  float gapMaxRatio;
  float widthRatio;
  uint16_t platformColor;
  uint16_t topColor;
  uint16_t bottomColor;
  uint16_t hudColor;
  const char* altitudeName;
};

constexpr LevelRule kLevels[] = {
    {0, 0.18f, -4.55f, 3.0f, 0.12f, 0.18f, 0.42f, 0x7FE0, 0x5D9F, 0xB7FF, kInk, "SKY"},
    {180, 0.19f, -4.70f, 3.1f, 0.13f, 0.20f, 0.38f, 0x07FF, 0x349F, 0xAEFF, kInk, "CLOUD"},
    {420, 0.20f, -4.85f, 3.2f, 0.14f, 0.22f, 0.34f, 0xFFE0, 0x44BF, 0xFEA0, kInk, "SUNSET"},
    {720, 0.21f, -5.00f, 3.35f, 0.15f, 0.24f, 0.30f, 0xFD20, 0x2277, 0xC390, TFT_WHITE, "DUSK"},
    {1080, 0.225f, -5.15f, 3.5f, 0.16f, 0.26f, 0.27f, 0xFFFF, 0x118E, 0x74FF, TFT_WHITE, "HIGH"},
    {1500, 0.24f, -5.30f, 3.7f, 0.17f, 0.28f, 0.24f, 0xF81F, 0x006A, 0x3B7F, TFT_WHITE, "STARS"},
    {1980, 0.255f, -5.45f, 3.9f, 0.18f, 0.31f, 0.22f, 0xFFE0, 0x0004, 0x018E, TFT_WHITE, "SPACE"},
};

constexpr int kLevelCount = sizeof(kLevels) / sizeof(kLevels[0]);

struct Platform {
  float x;
  float y;
  int w;
};

struct Player {
  float x;
  float y;
  float vx;
  float vy;
};

enum class DifficultyMode {
  Easy,
  Standard,
  Hard,
};

int screenW = 0;
int screenH = 0;
int playerW = 10;
int playerH = 12;
int platformH = 4;
int score = 0;
int bestScore = 0;
int worldLift = 0;
bool gameOver = false;
bool gameOverDrawn = false;
bool winTransition = false;
bool victory = false;
bool victoryDrawn = false;
uint32_t lastFrame = 0;
uint32_t winTransitionStart = 0;
uint32_t victoryStart = 0;
uint32_t lastTelemetry = 0;
uint32_t telemetrySeq = 0;
uint32_t lastBounceSoundAt = 0;
int displayedLevel = 1;
int cachedBackgroundIndex = -1;
DifficultyMode difficultyMode = DifficultyMode::Hard;
int hardStars = 5;
bool audioEnabled = false;

Player player;
Platform platforms[kPlatformCount];
M5Canvas canvas(&M5.Display);
M5Canvas backgroundCanvas(&M5.Display);

void initAudio() {
  M5.Speaker.begin();
  audioEnabled = M5.Speaker.isEnabled();
  if (!audioEnabled) {
    return;
  }
  M5.Speaker.setVolume(46);
  M5.Speaker.setChannelVolume(0, 120);
  M5.Speaker.setChannelVolume(1, 110);
}

void playBounceSound() {
  if (!audioEnabled) {
    return;
  }
  uint32_t now = millis();
  if (now - lastBounceSoundAt < 90) {
    return;
  }
  lastBounceSoundAt = now;
  M5.Speaker.tone(1175, 45, 0, true);
}

void playStartSound() {
  if (audioEnabled) {
    M5.Speaker.tone(988, 70, 0, true);
  }
}

void playFailureSound() {
  if (audioEnabled) {
    M5.Speaker.tone(392, 210, 0, true);
  }
}

void playSuccessSound() {
  if (audioEnabled) {
    M5.Speaker.tone(1319, 180, 0, true);
    M5.Speaker.tone(1760, 220, 1, true);
  }
}

int levelIndexForScore(int value) {
  int index = 0;
  for (int i = 0; i < kLevelCount; ++i) {
    if (value >= kLevels[i].scoreStart) {
      index = i;
    }
  }
  return index;
}

const LevelRule& currentLevel() {
  return kLevels[levelIndexForScore(score)];
}

const char* difficultyCode() {
  if (difficultyMode == DifficultyMode::Easy) {
    return "EASY";
  }
  if (difficultyMode == DifficultyMode::Standard) {
    return "STANDARD";
  }
  return "HARD";
}

const char* difficultyShortCode() {
  if (difficultyMode == DifficultyMode::Easy) {
    return "E";
  }
  if (difficultyMode == DifficultyMode::Standard) {
    return "S";
  }
  return "H";
}

int difficultyStars() {
  if (difficultyMode == DifficultyMode::Easy) {
    return 1;
  }
  if (difficultyMode == DifficultyMode::Standard) {
    return 3;
  }
  return hardStars;
}

float gravityScale() {
  if (difficultyMode == DifficultyMode::Easy) {
    return 0.82f;
  }
  if (difficultyMode == DifficultyMode::Standard) {
    return 0.94f;
  }
  return 1.0f + (hardStars - 1) * 0.035f;
}

float jumpScale() {
  if (difficultyMode == DifficultyMode::Easy) {
    return 0.95f;
  }
  if (difficultyMode == DifficultyMode::Standard) {
    return 0.99f;
  }
  return 1.0f + (hardStars - 1) * 0.015f;
}

float maxVXScale() {
  if (difficultyMode == DifficultyMode::Easy) {
    return 0.86f;
  }
  if (difficultyMode == DifficultyMode::Standard) {
    return 0.95f;
  }
  return 1.0f + (hardStars - 1) * 0.025f;
}

float gapScale() {
  if (difficultyMode == DifficultyMode::Easy) {
    return 0.76f;
  }
  if (difficultyMode == DifficultyMode::Standard) {
    return 0.90f;
  }
  return 1.0f + (hardStars - 1) * 0.045f;
}

float widthScale() {
  if (difficultyMode == DifficultyMode::Easy) {
    return 1.34f;
  }
  if (difficultyMode == DifficultyMode::Standard) {
    return 1.12f;
  }
  return max(0.68f, 1.0f - (hardStars - 1) * 0.045f);
}

float jumpVelocityForLevel() {
  return currentLevel().jumpVelocity * jumpScale();
}

int winScoreForDifficulty() {
  if (difficultyMode == DifficultyMode::Easy) {
    return 1550;
  }
  if (difficultyMode == DifficultyMode::Standard) {
    return 2100;
  }
  return kBaseWinScore + (hardStars - 5) * 170;
}

void cycleDifficulty() {
  if (difficultyMode == DifficultyMode::Easy) {
    difficultyMode = DifficultyMode::Standard;
  } else if (difficultyMode == DifficultyMode::Standard) {
    difficultyMode = DifficultyMode::Hard;
    hardStars = 1;
  } else if (hardStars < 7) {
    ++hardStars;
  } else {
    difficultyMode = DifficultyMode::Easy;
  }
}

void difficultyLabel(char* buffer, size_t size) {
  if (difficultyMode == DifficultyMode::Easy) {
    snprintf(buffer, size, "EASY");
  } else if (difficultyMode == DifficultyMode::Standard) {
    snprintf(buffer, size, "STANDARD");
  } else {
    int pos = snprintf(buffer, size, "HARD ");
    for (int i = 0; i < hardStars && pos < static_cast<int>(size) - 1; ++i) {
      buffer[pos++] = '*';
    }
    buffer[pos] = '\0';
  }
}

int platformWidthForScore() {
  const LevelRule& level = currentLevel();
  return max(screenW / 8, static_cast<int>(screenW * level.widthRatio * widthScale()));
}

float randomXForWidth(int w) {
  return static_cast<float>(random(0, max(1, screenW - w)));
}

void spawnPlatform(Platform& p, float y) {
  p.w = platformWidthForScore();
  p.x = randomXForWidth(p.w);
  p.y = y;
}

void resetGame() {
  screenW = M5.Display.width();
  screenH = M5.Display.height();
  playerW = kPlayerSpriteWidth;
  playerH = kPlayerSpriteHeight;
  platformH = max(3, screenH / 55);

  score = 0;
  worldLift = 0;
  gameOver = false;
  gameOverDrawn = false;
  winTransition = false;
  victory = false;
  victoryDrawn = false;
  winTransitionStart = 0;
  victoryStart = 0;
  displayedLevel = 1;

  player.x = screenW * 0.5f - playerW * 0.5f;
  player.y = screenH * 0.70f;
  player.vx = 0.0f;
  player.vy = jumpVelocityForLevel();

  int baseY = screenH - 18;
  for (int i = 0; i < kPlatformCount; ++i) {
    float y = baseY - i * (screenH / 7.0f);
    spawnPlatform(platforms[i], y);
  }
  platforms[0].x = player.x - playerW;
  platforms[0].w = screenW / 2;
}

void drawTextCentered(const char* text, int y, int size, uint16_t color) {
  canvas.setTextSize(size);
  canvas.setTextColor(color, kSky);
  int textW = canvas.textWidth(text);
  canvas.setCursor((screenW - textW) / 2, y);
  canvas.print(text);
}

void emitTelemetry(const char* state, bool force = false) {
  uint32_t now = millis();
  if (!force && now - lastTelemetry < 90) {
    return;
  }
  lastTelemetry = now;

  int levelIndex = constrain(levelIndexForScore(score), 0, kLevelCount - 1);
  const LevelRule& level = kLevels[levelIndex];

  Serial.print("DJ:{\"seq\":");
  Serial.print(telemetrySeq++);
  Serial.print(",\"state\":\"");
  Serial.print(state);
  Serial.print("\",\"score\":");
  Serial.print(score);
  Serial.print(",\"best\":");
  Serial.print(bestScore);
  Serial.print(",\"level\":");
  Serial.print(levelIndex + 1);
  Serial.print(",\"alt\":\"");
  Serial.print(level.altitudeName);
  Serial.print("\",\"mode\":\"");
  Serial.print(difficultyCode());
  Serial.print("\",\"stars\":");
  Serial.print(difficultyStars());
  Serial.print(",\"win\":");
  Serial.print(winScoreForDifficulty());
  Serial.print(",\"screen\":[");
  Serial.print(screenW);
  Serial.print(',');
  Serial.print(screenH);
  Serial.print("],\"player\":[");
  Serial.print(static_cast<int>(player.x));
  Serial.print(',');
  Serial.print(static_cast<int>(player.y));
  Serial.print(',');
  Serial.print(static_cast<int>(player.vx * 100));
  Serial.print(',');
  Serial.print(static_cast<int>(player.vy * 100));
  Serial.print("],\"platforms\":[");
  for (int i = 0; i < kPlatformCount; ++i) {
    if (i > 0) {
      Serial.print(',');
    }
    Serial.print('[');
    Serial.print(static_cast<int>(platforms[i].x));
    Serial.print(',');
    Serial.print(static_cast<int>(platforms[i].y));
    Serial.print(',');
    Serial.print(platforms[i].w);
    Serial.print(']');
  }
  Serial.println("]}");
}

void drawHudText(int x, int y, const char* text, uint16_t color) {
  canvas.setTextColor(color == TFT_WHITE ? TFT_BLACK : TFT_WHITE);
  canvas.setCursor(x + 1, y + 1);
  canvas.print(text);
  canvas.setTextColor(color);
  canvas.setCursor(x, y);
  canvas.print(text);
}

void drawHud() {
  char leftTop[16];
  char bestText[16];
  char modeText[14];
  snprintf(leftTop, sizeof(leftTop), "L%d %d", displayedLevel, score);
  snprintf(bestText, sizeof(bestText), "B %d", bestScore);
  snprintf(modeText, sizeof(modeText), "%s%d", difficultyShortCode(), difficultyStars());
  const LevelRule& level = currentLevel();

  canvas.setTextSize(1);
  drawHudText(4, 4, leftTop, level.hudColor);
  drawHudText(4, 14, level.altitudeName, level.hudColor);
  drawHudText(4, 24, modeText, level.hudColor);
  drawHudText(screenW - canvas.textWidth(bestText) - 4, 4, bestText, level.hudColor);
}

void drawPlayer() {
  int px = static_cast<int>(player.x);
  int py = static_cast<int>(player.y);
  canvas.setSwapBytes(true);
  canvas.pushImage(px, py, kPlayerSpriteWidth, kPlayerSpriteHeight,
                   kPlayerSprite, kPlayerSpriteTransparent);
  canvas.setSwapBytes(false);
}

void drawAngelPlayer() {
  int px = static_cast<int>(player.x);
  int py = static_cast<int>(player.y);
  int centerX = px + playerW / 2;

  canvas.fillTriangle(px - 8, py + 15, px + 4, py + 7, px + 6, py + 27, TFT_WHITE);
  canvas.fillTriangle(px + playerW + 8, py + 15, px + playerW - 4, py + 7,
                      px + playerW - 6, py + 27, TFT_WHITE);
  canvas.drawTriangle(px - 8, py + 15, px + 4, py + 7, px + 6, py + 27, 0xFFE0);
  canvas.drawTriangle(px + playerW + 8, py + 15, px + playerW - 4, py + 7,
                      px + playerW - 6, py + 27, 0xFFE0);
  canvas.drawEllipse(centerX, py - 3, 10, 3, 0xFFE0);
  canvas.drawEllipse(centerX, py - 3, 9, 2, TFT_WHITE);
  drawPlayer();
}

void drawPlatforms() {
  uint16_t platformColor = currentLevel().platformColor;
  for (Platform& p : platforms) {
    canvas.fillRoundRect(static_cast<int>(p.x) - 1, static_cast<int>(p.y) - 1,
                         p.w + 2, platformH + 2, 2, TFT_BLACK);
    canvas.fillRoundRect(static_cast<int>(p.x), static_cast<int>(p.y),
                         p.w, platformH, 2, platformColor);
    canvas.drawFastHLine(static_cast<int>(p.x), static_cast<int>(p.y),
                         p.w, TFT_WHITE);
  }
}

void cacheBackground(int bgIndex) {
  if (bgIndex == cachedBackgroundIndex) {
    return;
  }
  cachedBackgroundIndex = bgIndex;
  backgroundCanvas.fillScreen(TFT_BLACK);
  int imageX = (screenW - kLevelBackgroundWidth) / 2;
  int imageY = (screenH - kLevelBackgroundHeight) / 2;
  backgroundCanvas.setSwapBytes(true);
  backgroundCanvas.pushImage(imageX, imageY, kLevelBackgroundWidth,
                             kLevelBackgroundHeight, kLevelBackgrounds[bgIndex]);
  backgroundCanvas.setSwapBytes(false);
}

void drawBackground() {
  int bgIndex = constrain(displayedLevel - 1, 0, kLevelBackgroundCount - 1);
  cacheBackground(bgIndex);
  void* backgroundBuffer = backgroundCanvas.getBuffer();
  void* frameBuffer = canvas.getBuffer();
  if (backgroundBuffer != nullptr && frameBuffer != nullptr) {
    memcpy(canvas.getBuffer(), backgroundBuffer, screenW * screenH * sizeof(uint16_t));
  } else {
    canvas.fillScreen(TFT_BLACK);
  }
}

void drawGameOver() {
  M5.Display.fillScreen(TFT_BLACK);
  int imageX = (screenW - kGameOverWidth) / 2;
  int imageY = (screenH - kGameOverHeight) / 2;
  M5.Display.setSwapBytes(true);
  M5.Display.pushImage(imageX, imageY, kGameOverWidth, kGameOverHeight, kGameOverImage);
  M5.Display.setSwapBytes(false);
}

void drawVictoryBase() {
  backgroundCanvas.fillScreen(TFT_BLACK);
  int imageX = (screenW - kVictoryWidth) / 2;
  int imageY = (screenH - kVictoryHeight) / 2;
  backgroundCanvas.setSwapBytes(true);
  backgroundCanvas.pushImage(imageX, imageY, kVictoryWidth, kVictoryHeight, kVictoryImage);
  backgroundCanvas.setSwapBytes(false);
  cachedBackgroundIndex = -1;
}

void drawConfettiFrame() {
  uint32_t t = millis() - victoryStart;
  for (int i = 0; i < 28; ++i) {
    int baseX = (i * 23 + 11) % max(1, screenW);
    int sway = static_cast<int>((t / 45 + i * 7) % 19) - 9;
    int x = (baseX + sway + screenW) % max(1, screenW);
    int y = (static_cast<int>(t / 10) + i * 31) % (screenH + 26) - 18;
    uint16_t color = kConfettiColors[(i + t / 220) % (sizeof(kConfettiColors) / sizeof(kConfettiColors[0]))];
    if (i % 3 == 0) {
      canvas.fillRect(x, y, 3, 6, color);
    } else if (i % 3 == 1) {
      canvas.fillRect(x, y, 6, 3, color);
    } else {
      canvas.fillTriangle(x, y, x + 4, y + 2, x + 1, y + 5, color);
    }
  }
}

void drawVictoryFrame() {
  if (!victoryDrawn) {
    drawVictoryBase();
    victoryDrawn = true;
  }

  void* baseBuffer = backgroundCanvas.getBuffer();
  void* frameBuffer = canvas.getBuffer();
  if (baseBuffer != nullptr && frameBuffer != nullptr) {
    memcpy(frameBuffer, baseBuffer, screenW * screenH * sizeof(uint16_t));
  } else {
    canvas.fillScreen(TFT_BLACK);
  }

  drawConfettiFrame();
  canvas.pushSprite(0, 0);
}

void readInput() {
  float ax = 0.0f;
  float ay = 0.0f;
  float az = 0.0f;

  if (M5.Imu.isEnabled() && M5.Imu.getAccel(&ax, &ay, &az)) {
    player.vx -= ax * kMoveAccel * 3.2f;
  }

  if (M5.BtnA.isPressed()) {
    player.vx -= kMoveAccel;
  }
  if (M5.BtnB.isPressed()) {
    player.vx += kMoveAccel;
  }

  player.vx *= kFriction;
  float maxVX = currentLevel().maxVX * maxVXScale();
  player.vx = constrain(player.vx, -maxVX, maxVX);
}

void updatePlatformsAfterScroll(float lift) {
  const LevelRule& level = currentLevel();
  for (Platform& p : platforms) {
    p.y += lift;
    if (p.y > screenH + 8) {
      float highestY = screenH;
      for (Platform& other : platforms) {
        highestY = min(highestY, other.y);
      }
      int minGap = max(10, static_cast<int>(screenH * level.gapMinRatio * gapScale()));
      int maxGap = max(minGap + 1, static_cast<int>(screenH * level.gapMaxRatio * gapScale()));
      int gap = random(minGap, maxGap);
      spawnPlatform(p, highestY - gap);
    }
  }
}

void updateGame() {
  readInput();

  float previousBottom = player.y + playerH;
  player.vy += currentLevel().gravity * gravityScale();
  player.x += player.vx;
  player.y += player.vy;

  if (player.x < -playerW) {
    player.x = screenW;
  } else if (player.x > screenW) {
    player.x = -playerW;
  }

  if (player.vy > 0.0f) {
    float currentBottom = player.y + playerH;
    for (Platform& p : platforms) {
      bool crossed = previousBottom <= p.y && currentBottom >= p.y;
      bool overlapsX = player.x + playerW > p.x && player.x < p.x + p.w;
      if (crossed && overlapsX) {
        player.y = p.y - playerH;
        player.vy = jumpVelocityForLevel();
        playBounceSound();
        break;
      }
    }
  }

  float cameraLine = screenH * 0.42f;
  if (player.y < cameraLine) {
    float lift = cameraLine - player.y;
    player.y = cameraLine;
    worldLift += static_cast<int>(lift);
    score = max(score, worldLift);
    bestScore = max(bestScore, score);
    updatePlatformsAfterScroll(lift);
  }

  if (player.y > screenH + playerH) {
    gameOver = true;
    gameOverDrawn = false;
    playFailureSound();
  }

  if (score >= winScoreForDifficulty() && !winTransition && !victory) {
    winTransition = true;
    winTransitionStart = millis();
  }
}

void renderGame() {
  displayedLevel = levelIndexForScore(score) + 1;
  drawBackground();
  drawPlatforms();
  drawPlayer();
  drawHud();
  canvas.pushSprite(0, 0);
}

void renderWinTransition() {
  uint32_t elapsed = millis() - winTransitionStart;
  float progress = min(1.0f, elapsed / 820.0f);
  float startY = screenH * 0.42f;
  player.y = startY + (-playerH - startY) * progress;
  player.x += sin(elapsed / 75.0f) * 0.18f;

  displayedLevel = levelIndexForScore(score) + 1;
  drawBackground();
  drawPlatforms();
  drawAngelPlayer();
  drawHud();
  canvas.pushSprite(0, 0);

  if (progress >= 1.0f) {
    winTransition = false;
    victory = true;
    victoryDrawn = false;
    victoryStart = millis();
    playSuccessSound();
  }
}

void showBootScreen() {
  M5.Display.fillScreen(TFT_BLACK);
  int imageX = (screenW - kSplashWidth) / 2;
  int imageY = (screenH - kSplashHeight) / 2;
  M5.Display.setSwapBytes(true);
  M5.Display.pushImage(imageX, imageY, kSplashWidth, kSplashHeight, kSplashImage);
  M5.Display.setSwapBytes(false);

  bool modeNeedsRedraw = true;
  while (true) {
    M5.update();
    emitTelemetry("home");
    if (modeNeedsRedraw) {
      char modeText[20];
      difficultyLabel(modeText, sizeof(modeText));
      M5.Display.fillRect(0, screenH - 34, screenW, 34, TFT_BLACK);
      M5.Display.setTextSize(1);
      M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
      M5.Display.setCursor(6, screenH - 31);
      M5.Display.print(modeText);
      M5.Display.setCursor(6, screenH - 18);
      M5.Display.print("A MODE  B START");
      modeNeedsRedraw = false;
    }

    if (M5.BtnA.wasPressed()) {
      cycleDifficulty();
      modeNeedsRedraw = true;
      emitTelemetry("home", true);
    }
    if (M5.BtnB.wasPressed()) {
      playStartSound();
      break;
    }
    delay(10);
  }
}

}  // namespace

void setup() {
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;
  M5.begin(cfg);

  M5.Display.setRotation(0);
  screenW = M5.Display.width();
  screenH = M5.Display.height();
  canvas.createSprite(screenW, screenH);
  backgroundCanvas.createSprite(screenW, screenH);
  canvas.setTextDatum(TL_DATUM);
  backgroundCanvas.setTextDatum(TL_DATUM);
  initAudio();

  resetGame();
  showBootScreen();
  randomSeed(static_cast<uint32_t>(esp_random()));
  resetGame();
}

void loop() {
  M5.update();

  if (winTransition) {
    renderWinTransition();
    emitTelemetry("ascend");
    delay(16);
    return;
  }

  if (victory) {
    drawVictoryFrame();
    emitTelemetry("victory");
    if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed()) {
      showBootScreen();
      resetGame();
    }
    delay(30);
    return;
  }

  if (gameOver) {
    if (!gameOverDrawn) {
      drawGameOver();
      gameOverDrawn = true;
    }
    emitTelemetry("gameover");
    if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed()) {
      showBootScreen();
      resetGame();
    }
    delay(20);
    return;
  }

  uint32_t now = millis();
  if (now - lastFrame < 16) {
    delay(1);
    return;
  }
  lastFrame = now;

  updateGame();
  renderGame();
  emitTelemetry("play");
}
