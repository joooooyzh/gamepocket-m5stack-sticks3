#include <Arduino.h>
#include <M5Unified.h>

#include "devil_game.h"
#include "gamepocket_home_image.h"
#include "launcher_card_images.h"
#include "shiba_game.h"

namespace {

enum class LauncherGame {
  Shiba,
  Devil,
};

enum class LauncherState {
  Home,
  Menu,
  Shiba,
  Devil,
};

M5Canvas canvas(&M5.Display);
LauncherGame selectedGame = LauncherGame::Shiba;
LauncherState launcherState = LauncherState::Home;
int screenW = 0;
int screenH = 0;
bool audioEnabled = false;

void playTone(uint16_t freq, uint16_t ms, uint8_t channel = 0) {
  if (audioEnabled) {
    M5.Speaker.tone(freq, ms, channel, true);
  }
}

int selectedIndex() {
  return selectedGame == LauncherGame::Shiba ? 0 : 1;
}

void drawHome() {
  canvas.fillScreen(TFT_BLACK);
  canvas.setSwapBytes(true);
  canvas.pushImage((screenW - kGamePocketHomeImageWidth) / 2,
                   (screenH - kGamePocketHomeImageHeight) / 2,
                   kGamePocketHomeImageWidth, kGamePocketHomeImageHeight,
                   kGamePocketHomeImage);
  canvas.setSwapBytes(false);
  canvas.pushSprite(0, 0);
}

void drawLauncher() {
  canvas.fillScreen(TFT_BLACK);
  canvas.setSwapBytes(true);
  canvas.pushImage((screenW - kLauncherCardImageWidth) / 2,
                   (screenH - kLauncherCardImageHeight) / 2,
                   kLauncherCardImageWidth, kLauncherCardImageHeight,
                   kLauncherCardImages[selectedIndex()]);
  canvas.setSwapBytes(false);
  canvas.pushSprite(0, 0);
}

void enterSelectedGame() {
  playTone(1175, 70);
  if (selectedGame == LauncherGame::Shiba) {
    launcherState = LauncherState::Shiba;
    ShibaGame::begin();
    return;
  }

  launcherState = LauncherState::Devil;
  DevilGame::begin();
}

void returnToLauncher() {
  launcherState = LauncherState::Menu;
  M5.Display.setRotation(0);
  screenW = M5.Display.width();
  screenH = M5.Display.height();
  canvas.createSprite(screenW, screenH);
  canvas.setTextDatum(TL_DATUM);
  playTone(698, 55);
  drawLauncher();
}

}  // namespace

void setup() {
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
  canvas.setTextDatum(TL_DATUM);
  M5.Speaker.begin();
  audioEnabled = M5.Speaker.isEnabled();
  if (audioEnabled) {
    M5.Speaker.setVolume(70);
  }
  drawHome();
}

void loop() {
  if (launcherState == LauncherState::Shiba) {
    ShibaGame::update();
    if (ShibaGame::wantsExit()) {
      returnToLauncher();
    }
    return;
  }
  if (launcherState == LauncherState::Devil) {
    DevilGame::update();
    if (DevilGame::wantsExit()) {
      returnToLauncher();
    }
    return;
  }

  M5.update();
  if (launcherState == LauncherState::Home) {
    if (M5.BtnA.wasPressed()) {
      launcherState = LauncherState::Menu;
      playTone(1175, 70);
      drawLauncher();
    }
    delay(20);
    return;
  }

  if (M5.BtnB.wasPressed()) {
    selectedGame = selectedGame == LauncherGame::Shiba ? LauncherGame::Devil : LauncherGame::Shiba;
    playTone(880, 45);
    drawLauncher();
  }
  if (M5.BtnA.wasPressed()) {
    enterSelectedGame();
  }
  delay(20);
}
