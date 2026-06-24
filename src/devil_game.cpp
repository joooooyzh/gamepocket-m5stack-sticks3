#include <Arduino.h>
#include <M5Unified.h>

#include "devil_game.h"

#include "failure_image.h"
#include "gameover_image.h"
#include "level_backgrounds.h"
#include "player_sprite.h"
#include "score_image.h"
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
constexpr int kBulletCount = 9;
constexpr int kBaseWinScore = 200000;
constexpr float kMoveAccel = 0.24f;
constexpr float kFriction = 0.94f;
constexpr float kBulletSpeed = 6.4f;
constexpr uint32_t kShotCooldownMs = 92;
constexpr uint32_t kRocketDurationMs = 2800;
constexpr float kRocketVelocity = -8.0f;
constexpr int kRocketMinScoreBoost = 5000;
constexpr int kRocketMaxScoreBoost = 10000;
constexpr int kRocketMilestones[] = {5000, 20000, 50000};
constexpr int kRocketMilestoneCount = sizeof(kRocketMilestones) / sizeof(kRocketMilestones[0]);
constexpr uint8_t kPlatformCloud = 0;
constexpr uint8_t kPlatformGrass = 1;
constexpr uint8_t kPlatformMushroom = 2;
constexpr uint8_t kPlatformIce = 3;
constexpr uint8_t kPlatformSpace = 4;
constexpr uint8_t kPlatformCracked = 5;
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
    {1200, 0.19f, -4.70f, 3.1f, 0.13f, 0.20f, 0.38f, 0x07FF, 0x349F, 0xAEFF, kInk, "CLOUD"},
    {2600, 0.20f, -4.85f, 3.2f, 0.14f, 0.22f, 0.34f, 0xFFE0, 0x44BF, 0xFEA0, kInk, "SUNSET"},
    {4100, 0.21f, -5.00f, 3.35f, 0.15f, 0.24f, 0.30f, 0xFD20, 0x2277, 0xC390, TFT_WHITE, "DUSK"},
    {5800, 0.225f, -5.15f, 3.5f, 0.16f, 0.26f, 0.27f, 0xFFFF, 0x118E, 0x74FF, TFT_WHITE, "HIGH"},
    {7600, 0.24f, -5.30f, 3.7f, 0.17f, 0.28f, 0.24f, 0xF81F, 0x006A, 0x3B7F, TFT_WHITE, "STARS"},
    {9200, 0.255f, -5.45f, 3.9f, 0.18f, 0.31f, 0.22f, 0xFFE0, 0x0004, 0x018E, TFT_WHITE, "SPACE"},
};

constexpr int kLevelCount = sizeof(kLevels) / sizeof(kLevels[0]);

struct Platform {
  float x;
  float y;
  int w;
  bool monsterAlive;
  bool rocketActive;
  bool crumbling;
  uint8_t monsterType;
  uint8_t platformType;
  uint8_t crumbleTicks;
};

struct Player {
  float x;
  float y;
  float vx;
  float vy;
};

struct Bullet {
  float x;
  float y;
  bool active;
};

#if defined(DEVIL_JUMP_STICKS3)
struct BgmNote {
  uint16_t frequency;
  uint16_t duration;
  uint16_t gap;
};

constexpr BgmNote kRetroPlatformBgm[] = {
    {988, 72, 20},  {1175, 72, 20}, {1319, 92, 36}, {0, 36, 12},
    {988, 72, 20},  {1568, 92, 36}, {1319, 72, 24}, {1175, 92, 72},
    {880, 72, 20},  {1047, 72, 20}, {1175, 92, 36}, {0, 36, 12},
    {880, 72, 20},  {1319, 92, 36}, {1175, 72, 24}, {1047, 118, 88},
};

constexpr int kRetroPlatformBgmLength = sizeof(kRetroPlatformBgm) / sizeof(kRetroPlatformBgm[0]);
#endif

enum class DifficultyMode {
  Easy,
  Challenge,
};

int screenW = 0;
int screenH = 0;
int playerW = 10;
int playerH = 12;
int platformH = 4;
int score = 0;
int bestScore = 0;
int runBestBefore = 0;
int worldLift = 0;
int rocketsCollected = 0;
int monstersShot = 0;
int crumblesTriggered = 0;
int maxLevelReached = 1;
bool gameOver = false;
bool gameOverDrawn = false;
bool dying = false;
bool winTransition = false;
bool victory = false;
bool victoryDrawn = false;
uint32_t lastFrame = 0;
uint32_t dyingStart = 0;
uint32_t winTransitionStart = 0;
uint32_t victoryStart = 0;
uint32_t lastTelemetry = 0;
uint32_t telemetrySeq = 0;
uint32_t lastBounceSoundAt = 0;
uint32_t lastShotAt = 0;
uint32_t rocketUntil = 0;
uint32_t rocketStartAt = 0;
int rocketStartScore = 0;
int rocketTargetScore = 0;
int nextMonsterScore = 3000;
#if defined(DEVIL_JUMP_STICKS3)
uint32_t nextBgmAt = 0;
int bgmNoteIndex = 0;
bool bgmEnabled = false;
#endif
int displayedLevel = 1;
int cachedBackgroundIndex = -1;
DifficultyMode difficultyMode = DifficultyMode::Easy;
bool rocketMilestoneUsed[kRocketMilestoneCount] = {false, false, false};
bool audioEnabled = false;
bool exitRequested = false;

Player player;
Platform platforms[kPlatformCount];
Bullet bullets[kBulletCount];
M5Canvas canvas(&M5.Display);
M5Canvas backgroundCanvas(&M5.Display);

void initAudio() {
  M5.Speaker.begin();
  audioEnabled = M5.Speaker.isEnabled();
  if (!audioEnabled) {
    return;
  }
#if defined(DEVIL_JUMP_STICKS3)
  M5.Speaker.setVolume(88);
  M5.Speaker.setChannelVolume(0, 120);
  M5.Speaker.setChannelVolume(1, 200);
#else
  M5.Speaker.setVolume(46);
  M5.Speaker.setChannelVolume(0, 120);
  M5.Speaker.setChannelVolume(1, 110);
#endif
}

#if defined(DEVIL_JUMP_STICKS3)
void startBgm() {
  if (!audioEnabled) {
    return;
  }
  bgmEnabled = true;
}

void stopBgm() {
  bgmEnabled = false;
  if (audioEnabled) {
    M5.Speaker.stop(0);
  }
}

void updateBgm() {
  if (!audioEnabled || !bgmEnabled) {
    return;
  }

  uint32_t now = millis();
  if (now < nextBgmAt) {
    return;
  }

  const BgmNote& note = kRetroPlatformBgm[bgmNoteIndex];
  if (note.frequency > 0) {
    M5.Speaker.tone(note.frequency, note.duration, 0, true);
  } else {
    M5.Speaker.stop(0);
  }
  nextBgmAt = now + note.duration + note.gap;
  bgmNoteIndex = (bgmNoteIndex + 1) % kRetroPlatformBgmLength;
}

void resetBgm() {
  bgmNoteIndex = 0;
  nextBgmAt = 0;
}
#endif

void playBounceSound() {
  if (!audioEnabled) {
    return;
  }
  uint32_t now = millis();
  if (now - lastBounceSoundAt < 90) {
    return;
  }
  lastBounceSoundAt = now;
#if defined(DEVIL_JUMP_STICKS3)
  M5.Speaker.tone(1976, 58, 1, true);
#else
  M5.Speaker.tone(1175, 45, 0, true);
#endif
}

void playShootSound() {
  if (audioEnabled) {
#if defined(DEVIL_JUMP_STICKS3)
    M5.Speaker.tone(2637, 28, 1, true);
#else
    M5.Speaker.tone(1568, 24, 0, true);
#endif
  }
}

void playMonsterHitSound() {
  if (audioEnabled) {
#if defined(DEVIL_JUMP_STICKS3)
    M5.Speaker.tone(784, 48, 1, true);
#else
    M5.Speaker.tone(988, 45, 0, true);
#endif
  }
}

void playRocketSound() {
  if (audioEnabled) {
#if defined(DEVIL_JUMP_STICKS3)
    M5.Speaker.tone(1760, 90, 1, true);
    M5.Speaker.tone(2637, 150, 0, true);
#else
    M5.Speaker.tone(1760, 120, 0, true);
#endif
  }
}

void playStartSound() {
  if (audioEnabled) {
#if defined(DEVIL_JUMP_STICKS3)
    M5.Speaker.tone(1319, 90, 0, true);
#else
    M5.Speaker.tone(988, 70, 0, true);
#endif
  }
}

void playFailureSound() {
  if (audioEnabled) {
#if defined(DEVIL_JUMP_STICKS3)
    stopBgm();
    M5.Speaker.tone(330, 260, 0, true);
#else
    M5.Speaker.tone(392, 210, 0, true);
#endif
  }
}

void playSuccessSound() {
  if (audioEnabled) {
#if defined(DEVIL_JUMP_STICKS3)
    stopBgm();
    M5.Speaker.tone(1568, 220, 0, true);
    M5.Speaker.tone(2093, 260, 1, true);
#else
    M5.Speaker.tone(1319, 180, 0, true);
    M5.Speaker.tone(1760, 220, 1, true);
#endif
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
  return "CHALLENGE";
}

const char* difficultyShortCode() {
  if (difficultyMode == DifficultyMode::Easy) {
    return "E";
  }
  return "C";
}

int difficultyStars() {
  return difficultyMode == DifficultyMode::Easy ? 1 : 2;
}

bool challengeMode() {
  return difficultyMode == DifficultyMode::Challenge;
}

float gravityScale() {
  if (difficultyMode == DifficultyMode::Easy) {
    return 0.82f;
  }
  return 1.05f;
}

float jumpScale() {
  if (difficultyMode == DifficultyMode::Easy) {
    return 0.95f;
  }
  return 1.02f;
}

float maxVXScale() {
  if (difficultyMode == DifficultyMode::Easy) {
    return 0.86f;
  }
  return 1.08f;
}

float gapScale() {
  if (difficultyMode == DifficultyMode::Easy) {
    return 0.76f;
  }
  return 0.86f;
}

float widthScale() {
  if (difficultyMode == DifficultyMode::Easy) {
    return 1.34f;
  }
  return 1.05f;
}

float jumpVelocityForLevel() {
  return currentLevel().jumpVelocity * jumpScale();
}

int winScoreForDifficulty() {
  return kBaseWinScore;
}

void cycleDifficulty() {
  if (difficultyMode == DifficultyMode::Easy) {
    difficultyMode = DifficultyMode::Challenge;
  } else {
    difficultyMode = DifficultyMode::Easy;
  }
}

void difficultyLabel(char* buffer, size_t size) {
  if (difficultyMode == DifficultyMode::Easy) {
    snprintf(buffer, size, "EASY");
  } else {
    snprintf(buffer, size, "CHALLENGE");
  }
}

int platformWidthForScore() {
  const LevelRule& level = currentLevel();
  return max(screenW / 8, static_cast<int>(screenW * level.widthRatio * widthScale()));
}

float randomXForWidth(int w) {
  return static_cast<float>(random(0, max(1, screenW - w)));
}

void resetBullets() {
  for (Bullet& b : bullets) {
    b.active = false;
    b.x = 0.0f;
    b.y = 0.0f;
  }
  lastShotAt = 0;
}

int scoreFromWorldLift() {
  return worldLift * (challengeMode() ? 4 : 3);
}

int lateGameRank() {
  return constrain(levelIndexForScore(score), 0, kLevelCount - 1);
}

int rocketMilestoneIndexForScore() {
  for (int i = 0; i < kRocketMilestoneCount; ++i) {
    int start = kRocketMilestones[i];
    int window = i == 0 ? 8000 : (i == 1 ? 12000 : 18000);
    if (!rocketMilestoneUsed[i] && score >= start && score < start + window) {
      return i;
    }
  }
  return -1;
}

bool shouldSpawnRocket() {
  if (!challengeMode()) {
    return false;
  }
  int milestoneIndex = rocketMilestoneIndexForScore();
  if (milestoneIndex < 0) {
    return false;
  }
  int chance = milestoneIndex == 0 ? 28 : (milestoneIndex == 1 ? 24 : 20);
  if (random(100) >= chance) {
    return false;
  }
  rocketMilestoneUsed[milestoneIndex] = true;
  return true;
}

int monsterSpawnChance() {
  if (!challengeMode() || score < 3000) {
    return 0;
  }
  int scaled = (score - 3000) / 2500;
  return min(72, 28 + scaled * 3 + lateGameRank() * 4);
}

int crackedPlatformChance() {
  if (!challengeMode() || score < 4500) {
    return 0;
  }
  int scaled = (score - 4500) / 3500;
  return min(34, 8 + scaled * 2 + lateGameRank() * 2);
}

bool monsterDue() {
  return challengeMode() && score >= nextMonsterScore;
}

void scheduleNextMonster() {
  int pressure = min(1700, max(0, score - 3000) / 22);
  int minGap = max(800, 2800 - pressure);
  int maxGap = max(minGap + 500, 4300 - pressure);
  nextMonsterScore = score + random(minGap, maxGap + 1);
}

bool anyMonsterAlive() {
  for (const Platform& p : platforms) {
    if (p.monsterAlive) {
      return true;
    }
  }
  return false;
}

void ensureVisibleMonster() {
  if (!monsterDue() || anyMonsterAlive()) {
    return;
  }

  Platform* fallback = nullptr;
  Platform* best = nullptr;
  for (Platform& p : platforms) {
    if (p.crumbling || p.rocketActive || p.w < 24 || p.y <= 34 || p.y >= screenH - 38) {
      continue;
    }
    if (!fallback) {
      fallback = &p;
    }
    if (p.y < player.y - 18) {
      best = &p;
      break;
    }
  }

  Platform* target = best ? best : fallback;
  if (!target) {
    return;
  }

  target->platformType = random(0, 5);
  target->monsterAlive = true;
  target->rocketActive = false;
  target->monsterType = random(0, 3);
  scheduleNextMonster();
}

void spawnPlatform(Platform& p, float y) {
  bool forceMonster = monsterDue() && y > 18;
  int crackChance = crackedPlatformChance();
  bool cracked = !forceMonster && y > 28 && random(100) < crackChance;

  p.w = platformWidthForScore();
  p.x = randomXForWidth(p.w);
  p.y = y;
  p.crumbling = false;
  p.crumbleTicks = 0;
  p.platformType = cracked ? kPlatformCracked : random(0, 5);

  int monsterChance = monsterSpawnChance();
  p.monsterAlive = !cracked && p.w >= 24 && y > 18 &&
                   (forceMonster || random(100) < monsterChance);
  if (p.monsterAlive && forceMonster) {
    scheduleNextMonster();
  }
  p.rocketActive = !cracked && !p.monsterAlive && p.w >= 22 && y > 26 && shouldSpawnRocket();
  p.monsterType = random(0, 3);
}

void resetGame() {
  screenW = M5.Display.width();
  screenH = M5.Display.height();
  playerW = kPlayerSpriteWidth;
  playerH = kPlayerSpriteHeight;
  platformH = max(3, screenH / 55);

  runBestBefore = bestScore;
  score = 0;
  worldLift = 0;
  rocketsCollected = 0;
  monstersShot = 0;
  crumblesTriggered = 0;
  maxLevelReached = 1;
  gameOver = false;
  gameOverDrawn = false;
  dying = false;
  winTransition = false;
  victory = false;
  victoryDrawn = false;
  dyingStart = 0;
  winTransitionStart = 0;
  victoryStart = 0;
  displayedLevel = 1;
  cachedBackgroundIndex = -1;
  rocketUntil = 0;
  rocketStartAt = 0;
  rocketStartScore = 0;
  rocketTargetScore = 0;
  nextMonsterScore = 3000;
  for (int i = 0; i < kRocketMilestoneCount; ++i) {
    rocketMilestoneUsed[i] = false;
  }
  resetBullets();

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
  platforms[0].monsterAlive = false;
  platforms[0].rocketActive = false;
  platforms[0].crumbling = false;
  platforms[0].crumbleTicks = 0;
  platforms[0].platformType = 1;
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
  Serial.print(",\"prevBest\":");
  Serial.print(runBestBefore);
  Serial.print(",\"rocketsCollected\":");
  Serial.print(rocketsCollected);
  Serial.print(",\"monstersShot\":");
  Serial.print(monstersShot);
  Serial.print(",\"crumbles\":");
  Serial.print(crumblesTriggered);
  Serial.print(",\"maxLevel\":");
  Serial.print(maxLevelReached);
  Serial.print(",\"height\":");
  Serial.print(worldLift);
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
  Serial.print("],\"rocket\":");
  Serial.print(millis() < rocketUntil ? "true" : "false");
  Serial.print(",\"platforms\":[");
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
    Serial.print(',');
    Serial.print(platforms[i].platformType);
    Serial.print(',');
    Serial.print(platforms[i].crumbling ? 1 : 0);
    Serial.print(',');
    Serial.print(platforms[i].crumbleTicks);
    Serial.print(']');
  }
  Serial.print("],\"monsters\":[");
  bool firstMonster = true;
  for (int i = 0; i < kPlatformCount; ++i) {
    if (!platforms[i].monsterAlive) {
      continue;
    }
    if (!firstMonster) {
      Serial.print(',');
    }
    firstMonster = false;
    Serial.print('[');
    Serial.print(static_cast<int>(platforms[i].x + platforms[i].w * 0.5f - 8));
    Serial.print(',');
    Serial.print(static_cast<int>(platforms[i].y - 9));
    Serial.print(',');
    Serial.print(platforms[i].monsterType);
    Serial.print(']');
  }
  Serial.print("],\"rockets\":[");
  bool firstRocket = true;
  for (int i = 0; i < kPlatformCount; ++i) {
    if (!platforms[i].rocketActive) {
      continue;
    }
    if (!firstRocket) {
      Serial.print(',');
    }
    firstRocket = false;
    Serial.print('[');
    Serial.print(static_cast<int>(platforms[i].x + platforms[i].w * 0.5f - 4));
    Serial.print(',');
    Serial.print(static_cast<int>(platforms[i].y - 21));
    Serial.print(']');
  }
  Serial.print("],\"bullets\":[");
  bool firstBullet = true;
  for (int i = 0; i < kBulletCount; ++i) {
    if (!bullets[i].active) {
      continue;
    }
    if (!firstBullet) {
      Serial.print(',');
    }
    firstBullet = false;
    Serial.print('[');
    Serial.print(static_cast<int>(bullets[i].x));
    Serial.print(',');
    Serial.print(static_cast<int>(bullets[i].y));
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
  snprintf(modeText, sizeof(modeText), "%s", difficultyShortCode());
  const LevelRule& level = currentLevel();

  canvas.setTextSize(1);
  drawHudText(4, 4, leftTop, level.hudColor);
  drawHudText(4, 14, level.altitudeName, level.hudColor);
  drawHudText(4, 24, modeText, level.hudColor);
  drawHudText(screenW - canvas.textWidth(bestText) - 4, 4, bestText, level.hudColor);
}

bool rocketActive() {
  return millis() < rocketUntil;
}

void triggerGameOver() {
  if (dying || gameOver || victory || winTransition) {
    return;
  }
  dying = true;
  dyingStart = millis();
  gameOverDrawn = false;
  rocketUntil = 0;
  playFailureSound();
}

void drawRocketAt(int x, int y, bool flameOn) {
  canvas.fillTriangle(x + 4, y - 3, x + 1, y + 3, x + 7, y + 3, 0xFFE0);
  canvas.fillRoundRect(x + 1, y + 2, 7, 14, 3, 0xF800);
  canvas.fillRoundRect(x + 3, y + 4, 3, 8, 1, 0xFD20);
  canvas.fillRect(x, y + 10, 2, 5, 0x7BEF);
  canvas.fillRect(x + 7, y + 10, 2, 5, 0x7BEF);
  if (flameOn) {
    canvas.fillTriangle(x + 2, y + 15, x + 7, y + 15, x + 4, y + 24, 0x07FF);
    canvas.fillTriangle(x + 3, y + 15, x + 6, y + 15, x + 4, y + 21, 0xFFE0);
  }
}

void drawPlayer() {
  int px = static_cast<int>(player.x);
  int py = static_cast<int>(player.y);
  if (rocketActive()) {
    drawRocketAt(px + playerW / 2 - 4, py + playerH - 18, true);
  }
  canvas.setSwapBytes(true);
  canvas.pushImage(px, py, kPlayerSpriteWidth, kPlayerSpriteHeight,
                   kPlayerSprite, kPlayerSpriteTransparent);
  canvas.setSwapBytes(false);
}

void drawMonster(const Platform& p) {
  if (!p.monsterAlive) {
    return;
  }
  int mx = static_cast<int>(p.x + p.w * 0.5f);
  int my = static_cast<int>(p.y) - 8;
  uint16_t body = p.monsterType == 0 ? 0x783F : (p.monsterType == 1 ? 0x07E6 : 0xF986);
  uint16_t belly = p.monsterType == 0 ? 0xB81F : (p.monsterType == 1 ? 0x57EA : 0xFD20);

  canvas.fillTriangle(mx, my - 14, mx - 5, my - 5, mx + 5, my - 5, kDanger);
  canvas.setTextSize(1);
  canvas.setTextColor(TFT_WHITE);
  canvas.setCursor(mx - 1, my - 14);
  canvas.print("!");
  canvas.fillRoundRect(mx - 8, my, 16, 11, 5, TFT_BLACK);
  canvas.drawRoundRect(mx - 9, my - 1, 18, 13, 5, 0xFFE0);
  canvas.fillRoundRect(mx - 7, my - 1, 14, 11, 5, body);
  canvas.fillCircle(mx, my + 7, 4, belly);
  canvas.fillTriangle(mx - 7, my + 1, mx - 4, my - 5, mx - 1, my + 1, kHorn);
  canvas.fillTriangle(mx + 1, my + 1, mx + 4, my - 5, mx + 7, my + 1, kHorn);
  canvas.fillTriangle(mx - 9, my + 6, mx - 13, my + 9, mx - 7, my + 10, body);
  canvas.fillTriangle(mx + 9, my + 6, mx + 13, my + 9, mx + 7, my + 10, body);
  canvas.fillCircle(mx - 3, my + 3, 2, TFT_WHITE);
  canvas.fillCircle(mx + 3, my + 3, 2, TFT_WHITE);
  canvas.drawPixel(mx - 3, my + 3, kInk);
  canvas.drawPixel(mx + 3, my + 3, kInk);
  canvas.drawFastHLine(mx - 3, my + 8, 7, kInk);
}

void drawBullets() {
  for (const Bullet& b : bullets) {
    if (!b.active) {
      continue;
    }
    int x = static_cast<int>(b.x);
    int y = static_cast<int>(b.y);
    canvas.fillCircle(x, y + 4, 2, 0xF800);
    canvas.fillCircle(x, y + 1, 2, 0xFD20);
    canvas.fillCircle(x, y - 1, 1, 0xFFE0);
    canvas.drawPixel(x, y + 7, 0xF800);
  }
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
  for (Platform& p : platforms) {
    int x = static_cast<int>(p.x);
    int y = static_cast<int>(p.y);
    uint16_t top = currentLevel().platformColor;
    uint16_t bottom = 0x03A3;
    switch (p.platformType) {
      case kPlatformCloud:
        top = TFT_WHITE;
        bottom = 0x7DDF;
        break;
      case kPlatformGrass:
        top = 0x7FE0;
        bottom = 0x9A60;
        break;
      case kPlatformMushroom:
        top = 0xFD20;
        bottom = 0xF800;
        break;
      case kPlatformIce:
        top = 0xBFFF;
        bottom = 0x4D7F;
        break;
      case kPlatformCracked:
        top = p.crumbling ? 0xB269 : 0xE3A9;
        bottom = 0x7286;
        break;
      default:
        top = 0xFFE0;
        bottom = 0x8010;
        break;
    }

    if (p.crumbling) {
      int fall = p.crumbleTicks * 2;
      int half = max(5, p.w / 2 - 3);
      canvas.fillRoundRect(x - 1, y + fall - 1, half + 1, platformH + 4, 2, TFT_BLACK);
      canvas.fillRoundRect(x, y + fall, half, platformH + 2, 2, top);
      canvas.fillRoundRect(x + half + 5, y + fall + 2, p.w - half - 5, platformH + 4, 2,
                           TFT_BLACK);
      canvas.fillRoundRect(x + half + 6, y + fall + 3, p.w - half - 7, platformH + 2, 2,
                           bottom);
      canvas.fillRect(x + half + 2, y + fall + 6, 2, 2, top);
      canvas.fillRect(x + p.w - 5, y + fall + 10, 2, 2, bottom);
      continue;
    }

    canvas.fillRoundRect(x - 1, y - 2, p.w + 2, platformH + 5, 3, TFT_BLACK);
    if (p.platformType == kPlatformCloud) {
      for (int i = 0; i < p.w; i += 10) {
        canvas.fillCircle(x + i + 5, y + 1, 5, TFT_WHITE);
      }
      canvas.fillRoundRect(x, y + 1, p.w, platformH + 2, 3, bottom);
      canvas.drawFastHLine(x + 2, y, max(1, p.w - 4), TFT_WHITE);
    } else if (p.platformType == kPlatformGrass) {
      canvas.fillRoundRect(x, y, p.w, platformH + 3, 2, bottom);
      canvas.fillRoundRect(x, y - 1, p.w, platformH, 2, top);
      for (int i = 3; i < p.w; i += 8) {
        canvas.drawFastVLine(x + i, y - 4, 4, top);
      }
    } else if (p.platformType == kPlatformMushroom) {
      canvas.fillRoundRect(x, y + 1, p.w, platformH + 2, 3, bottom);
      canvas.fillRoundRect(x + 2, y - 2, max(1, p.w - 4), platformH + 2, 5, top);
      canvas.drawFastHLine(x + 5, y - 1, max(1, p.w - 10), 0xFFE0);
    } else if (p.platformType == kPlatformIce) {
      canvas.fillRoundRect(x, y, p.w, platformH + 2, 2, bottom);
      for (int i = 0; i < p.w; i += 7) {
        canvas.fillTriangle(x + i, y + platformH + 2, x + i + 4, y - 2, x + i + 8,
                            y + platformH + 2, top);
      }
    } else if (p.platformType == kPlatformCracked) {
      canvas.fillRoundRect(x, y, p.w, platformH + 3, 2, bottom);
      canvas.fillRoundRect(x, y - 1, p.w, platformH + 1, 2, top);
      int mid = x + p.w / 2;
      canvas.drawLine(mid - 7, y - 1, mid - 3, y + 2, kDanger);
      canvas.drawLine(mid - 3, y + 2, mid + 1, y, kDanger);
      canvas.drawLine(mid + 1, y, mid + 6, y + 3, kDanger);
    } else {
      canvas.fillRoundRect(x, y, p.w, platformH + 2, 2, bottom);
      canvas.fillRoundRect(x, y - 1, p.w, platformH, 2, top);
      for (int i = 4; i < p.w; i += 15) {
        canvas.drawPixel(x + i, y, TFT_WHITE);
      }
    }
    if (p.rocketActive) {
      drawRocketAt(x + p.w / 2 - 4, y - 21, false);
    }
    drawMonster(p);
  }
}

void drawTinyStar(int x, int y, uint16_t color) {
  canvas.fillTriangle(x, y - 5, x - 2, y, x + 2, y, color);
  canvas.fillTriangle(x, y + 5, x - 2, y, x + 2, y, color);
  canvas.fillTriangle(x - 5, y, x, y - 2, x, y + 2, color);
  canvas.fillTriangle(x + 5, y, x, y - 2, x, y + 2, color);
}

void drawStatIcon(int x, int y, uint8_t iconType, uint16_t color) {
  if (iconType == 0) {
    drawRocketAt(x - 3, y - 8, false);
  } else if (iconType == 1) {
    canvas.fillRoundRect(x - 6, y - 5, 12, 10, 4, color);
    canvas.fillTriangle(x - 5, y - 4, x - 2, y - 9, x + 1, y - 4, kHorn);
    canvas.fillTriangle(x + 1, y - 4, x + 4, y - 9, x + 6, y - 4, kHorn);
    canvas.fillCircle(x - 2, y - 1, 1, TFT_WHITE);
    canvas.fillCircle(x + 3, y - 1, 1, TFT_WHITE);
  } else if (iconType == 2) {
    canvas.fillRoundRect(x - 8, y - 3, 7, 6, 1, color);
    canvas.fillRoundRect(x + 2, y - 4, 7, 7, 1, 0x7286);
    canvas.drawLine(x - 2, y - 3, x + 1, y + 3, kDanger);
  } else if (iconType == 3) {
    canvas.fillRect(x - 5, y - 7, 2, 13, color);
    canvas.fillTriangle(x - 3, y - 7, x + 7, y - 4, x - 3, y, kDanger);
    canvas.drawPixel(x + 1, y - 3, kInk);
  } else {
    canvas.fillTriangle(x, y - 9, x - 8, y + 6, x + 8, y + 6, color);
    canvas.fillTriangle(x, y - 4, x - 5, y + 6, x + 5, y + 6, 0x4D7F);
  }
}

void drawStatTile(int x, int y, const char* label, int value, uint8_t iconType, uint16_t iconColor) {
  canvas.fillRoundRect(x, y, 54, 33, 4, 0x1082);
  canvas.drawRoundRect(x, y, 54, 33, 4, 0x4208);
  drawStatIcon(x + 13, y + 16, iconType, iconColor);
  canvas.setTextSize(1);
  canvas.setTextColor(0xCFFF);
  canvas.setCursor(x + 25, y + 5);
  canvas.print(label);
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%d", value);
  canvas.setTextColor(TFT_WHITE);
  canvas.setCursor(x + 52 - canvas.textWidth(buffer), y + 20);
  canvas.print(buffer);
}

void drawResultPanel(const char* title) {
  int delta = score - runBestBefore;
  bool newBest = score >= bestScore && score > runBestBefore;

  canvas.fillScreen(TFT_BLACK);
  canvas.setSwapBytes(true);
  canvas.pushImage((screenW - kScoreWidth) / 2, (screenH - kScoreHeight) / 2,
                   kScoreWidth, kScoreHeight, kScoreImage);
  canvas.setSwapBytes(false);

  char scoreText[18];
  snprintf(scoreText, sizeof(scoreText), "%d", score);
  canvas.setTextSize(3);
  canvas.setTextColor(0x0000);
  canvas.setCursor((screenW - canvas.textWidth(scoreText)) / 2 + 2, 102);
  canvas.print(scoreText);
  canvas.setTextColor(0xFFE0);
  canvas.setCursor((screenW - canvas.textWidth(scoreText)) / 2, 100);
  canvas.print(scoreText);

  char compareText[18];
  if (newBest) {
    snprintf(compareText, sizeof(compareText), "NEW +%d", delta);
  } else {
    snprintf(compareText, sizeof(compareText), "BEST %d", bestScore);
  }
  canvas.setTextSize(1);
  canvas.setTextColor(newBest ? 0xFFE0 : 0xCFFF);
  canvas.setCursor((screenW - canvas.textWidth(compareText)) / 2, 174);
  canvas.print(compareText);
}

void cacheBackground(int bgIndex) {
  if (bgIndex == cachedBackgroundIndex) {
    return;
  }
  backgroundCanvas.fillScreen(TFT_BLACK);
  int imageX = (screenW - kLevelBackgroundWidth) / 2;
  int imageY = (screenH - kLevelBackgroundHeight) / 2;
  backgroundCanvas.setSwapBytes(true);
  backgroundCanvas.pushImage(imageX, imageY, kLevelBackgroundWidth,
                             kLevelBackgroundHeight, kLevelBackgrounds[bgIndex]);
  backgroundCanvas.setSwapBytes(false);
  cachedBackgroundIndex = bgIndex;
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
    canvas.setSwapBytes(true);
    canvas.pushImage((screenW - kLevelBackgroundWidth) / 2,
                     (screenH - kLevelBackgroundHeight) / 2,
                     kLevelBackgroundWidth, kLevelBackgroundHeight,
                     kLevelBackgrounds[bgIndex]);
    canvas.setSwapBytes(false);
  }
}

void drawGameOver() {
  canvas.fillScreen(0x1082);
  drawResultPanel("SCORE");
  canvas.pushSprite(0, 0);
}

void drawDeathDevil(int cx, int cy, uint16_t body, bool knockedOut) {
  canvas.fillRoundRect(cx - 9, cy - 7, 18, 14, 6, body);
  canvas.fillTriangle(cx - 8, cy - 5, cx - 5, cy - 13, cx - 1, cy - 5, kHorn);
  canvas.fillTriangle(cx + 1, cy - 5, cx + 5, cy - 13, cx + 8, cy - 5, kHorn);
  if (knockedOut) {
    canvas.drawLine(cx - 5, cy - 2, cx - 1, cy + 2, TFT_WHITE);
    canvas.drawLine(cx - 1, cy - 2, cx - 5, cy + 2, TFT_WHITE);
    canvas.drawLine(cx + 2, cy - 2, cx + 6, cy + 2, TFT_WHITE);
    canvas.drawLine(cx + 6, cy - 2, cx + 2, cy + 2, TFT_WHITE);
  } else {
    canvas.fillCircle(cx - 4, cy - 1, 2, 0xFFE0);
    canvas.fillCircle(cx + 4, cy - 1, 2, 0xFFE0);
  }
  canvas.fillTriangle(cx - 10, cy + 1, cx - 17, cy - 5, cx - 14, cy + 7, kWing);
  canvas.fillTriangle(cx + 10, cy + 1, cx + 17, cy - 5, cx + 14, cy + 7, kWing);
}

void drawDeathParticles(int cx, int cy, uint32_t elapsed) {
  for (int i = 0; i < 8; ++i) {
    float a = elapsed / 85.0f + i * 0.785f;
    int radius = 12 + (elapsed / 55 + i * 3) % 18;
    int x = cx + static_cast<int>(cos(a) * radius);
    int y = cy + static_cast<int>(sin(a) * radius * 0.65f);
    if (i % 3 == 0) {
      drawTinyStar(x, y, 0xFFE0);
    } else if (i % 3 == 1) {
      canvas.fillCircle(x, y, 2, 0xFD20);
    } else {
      canvas.fillRect(x, y, 3, 3, 0x8410);
    }
  }
}

void drawDyingFrame() {
  displayedLevel = levelIndexForScore(score) + 1;
  uint32_t elapsed = millis() - dyingStart;
  uint16_t flash = (elapsed / 100) % 2 == 0 ? 0xF800 : 0xFFE0;

  canvas.fillScreen(TFT_BLACK);
  canvas.setSwapBytes(true);
  canvas.pushImage((screenW - kFailureWidth) / 2, (screenH - kFailureHeight) / 2,
                   kFailureWidth, kFailureHeight, kFailureImage);
  canvas.setSwapBytes(false);

  canvas.fillRoundRect(18, screenH - 48, screenW - 36, 30, 6, 0x1082);
  canvas.drawRoundRect(18, screenH - 48, screenW - 36, 30, 6, flash);
  canvas.setTextSize(1);
  canvas.setTextColor(TFT_WHITE);
  const char* next = elapsed < 730 ? "OUCH" : "SCORE NEXT";
  canvas.setCursor((screenW - canvas.textWidth(next)) / 2, screenH - 39);
  canvas.print(next);
  uint32_t cappedElapsed = min<uint32_t>(980, elapsed);
  int progress = min(screenW - 44, static_cast<int>((screenW - 44) * cappedElapsed / 980));
  canvas.fillRoundRect(22, screenH - 27, screenW - 44, 3, 1, 0x0008);
  canvas.fillRoundRect(22, screenH - 27, progress, 3, 1, 0xFFE0);
  canvas.pushSprite(0, 0);
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
  drawResultPanel("SUCCESS");
  canvas.pushSprite(0, 0);
}

void readInput() {
  float ax = 0.0f;
  float ay = 0.0f;
  float az = 0.0f;

  if (M5.Imu.isEnabled() && M5.Imu.getAccel(&ax, &ay, &az)) {
#if defined(DEVIL_JUMP_STICKS3)
    player.vx -= ay * kMoveAccel * 3.2f;
#else
    player.vx -= ax * kMoveAccel * 3.2f;
#endif
  }

  player.vx *= kFriction;
  float maxVX = currentLevel().maxVX * maxVXScale();
  player.vx = constrain(player.vx, -maxVX, maxVX);
}

void fireBullet() {
  uint32_t now = millis();
  if (now - lastShotAt < kShotCooldownMs) {
    return;
  }

  for (Bullet& b : bullets) {
    if (b.active) {
      continue;
    }
    b.active = true;
    b.x = player.x + playerW * 0.5f;
    b.y = player.y + 2;
    lastShotAt = now;
    playShootSound();
    return;
  }
}

void updateBullets() {
  if (M5.BtnA.isPressed()) {
    fireBullet();
  }

  for (Bullet& b : bullets) {
    if (!b.active) {
      continue;
    }
    b.y -= kBulletSpeed;
    if (b.y < -8) {
      b.active = false;
      continue;
    }

    for (Platform& p : platforms) {
      if (!p.monsterAlive) {
        continue;
      }
      float mx = p.x + p.w * 0.5f;
      float my = p.y - 8;
      bool hitX = b.x >= mx - 8 && b.x <= mx + 8;
      bool hitY = b.y >= my - 5 && b.y <= my + 12;
      if (hitX && hitY) {
        b.active = false;
        p.monsterAlive = false;
        score += 120;
        ++monstersShot;
        bestScore = max(bestScore, score);
        playMonsterHitSound();
        break;
      }
    }
  }
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
  for (Bullet& b : bullets) {
    if (b.active) {
      b.y += lift;
    }
  }
}

void updateCrumblingPlatforms() {
  for (Platform& p : platforms) {
    if (!p.crumbling) {
      continue;
    }
    ++p.crumbleTicks;
    p.y += 1.5f;
    if (p.crumbleTicks > 28) {
      p.y = screenH + 12;
    }
  }
}

bool playerOverlapsRect(float x, float y, float w, float h) {
  return player.x + playerW > x && player.x < x + w && player.y + playerH > y &&
         player.y < y + h;
}

void updatePickupsAndHazards() {
  for (Platform& p : platforms) {
    if (p.monsterAlive) {
      float mx = p.x + p.w * 0.5f - 9;
      float my = p.y - 13;
      if (playerOverlapsRect(mx, my, 18, 18)) {
        triggerGameOver();
        return;
      }
    }

    if (p.rocketActive) {
      float rx = p.x + p.w * 0.5f - 7;
      float ry = p.y - 24;
      if (playerOverlapsRect(rx, ry, 14, 24)) {
        p.rocketActive = false;
        rocketStartAt = millis();
        rocketUntil = rocketStartAt + kRocketDurationMs;
        rocketStartScore = score;
        rocketTargetScore = score + random(kRocketMinScoreBoost, kRocketMaxScoreBoost + 1);
        player.vy = kRocketVelocity;
        ++rocketsCollected;
        bestScore = max(bestScore, score);
        playRocketSound();
      }
    }
  }
}

void updateGame() {
  readInput();
  updateBullets();
  updateCrumblingPlatforms();

  float previousBottom = player.y + playerH;
  if (rocketActive()) {
    player.vy = kRocketVelocity;
  } else {
    player.vy += currentLevel().gravity * gravityScale();
  }
  player.x += player.vx;
  player.y += player.vy;

  if (player.x < -playerW) {
    player.x = screenW;
  } else if (player.x > screenW) {
    player.x = -playerW;
  }

  updatePickupsAndHazards();
  if (gameOver) {
    return;
  }

  if (!rocketActive() && player.vy > 0.0f) {
    float currentBottom = player.y + playerH;
    for (Platform& p : platforms) {
      if (p.crumbling) {
        continue;
      }
      bool crossed = previousBottom <= p.y && currentBottom >= p.y;
      bool overlapsX = player.x + playerW > p.x && player.x < p.x + p.w;
      if (crossed && overlapsX) {
        player.y = p.y - playerH;
        player.vy = jumpVelocityForLevel();
        if (p.platformType == kPlatformCracked) {
          p.crumbling = true;
          p.crumbleTicks = 1;
          p.monsterAlive = false;
          p.rocketActive = false;
          ++crumblesTriggered;
        }
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
    score = max(score, scoreFromWorldLift());
    bestScore = max(bestScore, score);
    maxLevelReached = max(maxLevelReached, levelIndexForScore(score) + 1);
    updatePlatformsAfterScroll(lift);
  }

  if (rocketActive() && rocketTargetScore > rocketStartScore) {
    uint32_t elapsed = millis() - rocketStartAt;
    uint32_t cappedElapsed = min<uint32_t>(elapsed, kRocketDurationMs);
    int boostedScore =
        rocketStartScore +
        static_cast<int>((static_cast<int64_t>(rocketTargetScore - rocketStartScore) * cappedElapsed) /
                         kRocketDurationMs);
    score = max(score, boostedScore);
    bestScore = max(bestScore, score);
    maxLevelReached = max(maxLevelReached, levelIndexForScore(score) + 1);
  }

  ensureVisibleMonster();

  if (player.y > screenH + playerH) {
    triggerGameOver();
  }

  if (score >= winScoreForDifficulty() && !winTransition && !victory) {
    winTransition = true;
    winTransitionStart = millis();
#if defined(DEVIL_JUMP_STICKS3)
    stopBgm();
#endif
  }
}

void renderGame() {
  displayedLevel = levelIndexForScore(score) + 1;
  drawBackground();
  drawPlatforms();
  drawBullets();
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
  drawBullets();
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
#if defined(DEVIL_JUMP_STICKS3)
  stopBgm();
#endif
  M5.Display.fillScreen(TFT_BLACK);
  int imageX = (screenW - kSplashWidth) / 2;
  int imageY = (screenH - kSplashHeight) / 2;
  M5.Display.setSwapBytes(true);
  M5.Display.pushImage(imageX, imageY, kSplashWidth, kSplashHeight, kSplashImage);
  M5.Display.setSwapBytes(false);

  while (true) {
    M5.update();
    emitTelemetry("home");

    if (M5.BtnA.wasPressed()) {
      difficultyMode = DifficultyMode::Easy;
      playStartSound();
#if defined(DEVIL_JUMP_STICKS3)
      resetBgm();
      startBgm();
#endif
      emitTelemetry("home", true);
      break;
    }
    if (M5.BtnB.wasPressed()) {
      difficultyMode = DifficultyMode::Challenge;
      playStartSound();
#if defined(DEVIL_JUMP_STICKS3)
      resetBgm();
      startBgm();
#endif
      break;
    }
    delay(10);
  }
}

}  // namespace

namespace DevilGame {

void begin() {
  exitRequested = false;
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;
#if defined(DEVIL_JUMP_STICKS3)
  cfg.fallback_board = m5::board_t::board_M5StickS3;
#endif
  M5.begin(cfg);

  M5.Display.setRotation(0);
  screenW = M5.Display.width();
  screenH = M5.Display.height();
  canvas.createSprite(screenW, screenH);
  backgroundCanvas.createSprite(screenW, screenH);
  canvas.setTextDatum(TL_DATUM);
  backgroundCanvas.setTextDatum(TL_DATUM);
  cachedBackgroundIndex = -1;
  initAudio();
  playStartSound();

  resetGame();
  showBootScreen();
  randomSeed(static_cast<uint32_t>(esp_random()));
  resetGame();
}

void update() {
  M5.update();

  if (winTransition) {
    if (M5.BtnB.wasPressed()) {
      exitRequested = true;
      delay(20);
      return;
    }
    renderWinTransition();
    emitTelemetry("ascend");
    delay(16);
    return;
  }

  if (victory) {
    drawVictoryFrame();
    emitTelemetry("victory");
    if (M5.BtnB.wasPressed()) {
      exitRequested = true;
      delay(20);
      return;
    }
    if (M5.BtnA.wasPressed()) {
      showBootScreen();
      resetGame();
    }
    delay(30);
    return;
  }

  if (dying) {
    if (M5.BtnB.wasPressed()) {
      exitRequested = true;
      delay(20);
      return;
    }
    drawDyingFrame();
    emitTelemetry("dying");
    if (millis() - dyingStart >= 980) {
      dying = false;
      gameOver = true;
      gameOverDrawn = false;
    }
    delay(20);
    return;
  }

  if (gameOver) {
    if (!gameOverDrawn) {
      drawGameOver();
      gameOverDrawn = true;
    }
    emitTelemetry("gameover");
    if (M5.BtnB.wasPressed()) {
      exitRequested = true;
      delay(20);
      return;
    }
    if (M5.BtnA.wasPressed()) {
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
  if (M5.BtnB.wasPressed()) {
    exitRequested = true;
    delay(20);
    return;
  }
  lastFrame = now;

#if defined(DEVIL_JUMP_STICKS3)
  updateBgm();
#endif
  updateGame();
  renderGame();
  emitTelemetry("play");
}

bool wantsExit() {
  return exitRequested;
}

}  // namespace DevilGame
