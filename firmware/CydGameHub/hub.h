// =============================================================================
//  hub.h - shared framework for CYD Game Hub
//  Target: ESP32-2432S028R "Cheap Yellow Display" (2.8" 320x240, resistive XPT2046 touch)
//
//  The whole sketch is ONE translation unit: every game is a header with its own
//  namespace, all included from games.h. Everything here is `static`.
//
//  Rules for game code:
//   * C++11 only (arduino-esp32 2.0.x builds with -std=gnu++11).
//   * Use imin/imax/iabs/clampi, NOT min/max/abs on mixed types (core's min/max are templates).
//   * Only TFT_eSPI fonts 1 (6x8), 2 (16px) and 4 (26px) are loaded. Scale with the size arg.
//   * No full-screen sprites (not enough RAM). Redraw only what changed; erase old, draw new.
//   * Landscape 320x240. Top HUD_H rows belong to the HUD (title, score, MENU button at top-right).
//     The playfield is y = PF_Y .. SCR_H-1. Never draw over the MENU button.
//   * Each game provides:  void init();  void tick();  void icon(int cx,int cy);
//       init() - called once when the game is opened. Must call hudBegin("Name"), reset state, draw.
//       tick() - called every GameDef::frameMs. Read `touch`, update state, draw changes.
//       icon() - draw a ~36x36 pictogram centred on (cx,cy) for the menu tile (tile is dark grey).
// =============================================================================
#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <Preferences.h>
#ifdef ARDUINO_ARCH_ESP32
#include <esp_system.h>   // esp_random()
#endif

// ---- Safety net: the ESP32 core / xtensa headers #define many short upper-case names (BR, LBEG, SAR, ...).
// Any of them that equals one of this sketch's own constants would break the build, so drop them now that all
// system headers have been read. (Undefining a name that is not defined is harmless.)
#undef AH
#undef AW
#undef AX
#undef AY
#undef BACK_FILL
#undef BACK_RIM
#undef BALL_R
#undef BAR_BG
#undef BAR_H
#undef BAR_W
#undef BAR_X
#undef BAR_Y
#undef BG
#undef BH
#undef BHGT
#undef BOARD_COL
#undef BOT
#undef BR
#undef BRIGHT
#undef BROWN
#undef BRY0
#undef BS
#undef BW
#undef BWID
#undef BX
#undef BY
#undef CAP_H
#undef CAP_M
#undef CEIL_Y
#undef CELL
#undef CH
#undef COL
#undef COLS
#undef COL_BOARD
#undef COL_EDGE
#undef COL_GRID
#undef COL_O
#undef COL_WIN
#undef COL_X
#undef CS
#undef CW
#undef CX
#undef CY
#undef DARK
#undef DARK_TXT
#undef DC
#undef DEPTH
#undef DIM
#undef DIRT
#undef DISC
#undef DONE_FILL
#undef DONE_RIM
#undef DR
#undef DROPMS
#undef DX
#undef DY
#undef END_DRAW
#undef END_LOSS
#undef END_NONE
#undef END_WIN
#undef FACE_FILL
#undef FACE_RIM
#undef FBH
#undef FBW
#undef FBX
#undef FBY
#undef FIELD
#undef FIELD_H
#undef FLAP_V
#undef FLOOR_Y
#undef F_FLAG
#undef F_MINE
#undef F_REV
#undef GAMES
#undef GAP
#undef GAPP
#undef GRASS
#undef GRAV
#undef GRID
#undef GRID_LINE
#undef GROUND
#undef GROUND_Y
#undef GX
#undef GY
#undef G_CUT
#undef G_HOLD
#undef HB
#undef HI
#undef HID_DARK
#undef HID_FILL
#undef HID_LIGHT
#undef HOLES
#undef HUD_BG
#undef INF
#undef JV
#undef KICK
#undef LAMP_GLOW
#undef LAMP_OFF
#undef LAMP_ON
#undef LAMP_RIM
#undef LINES
#undef LOSE_Y
#undef LX
#undef MAXB
#undef MAXE
#undef MAXLEN
#undef MAXOBS
#undef MAXP
#undef MAXSEQ
#undef MINES
#undef N
#undef NCELLS
#undef NCLOUD
#undef NSPECK
#undef NUMCOL
#undef NUM_GAMES
#undef ORDER
#undef OX
#undef OY
#undef PAD_COL
#undef PAD_H
#undef PAD_SPD
#undef PAD_Y
#undef PAIRS
#undef PAL
#undef PER_PAGE
#undef PH
#undef PIPE_D
#undef PIPE_L
#undef PIPE_M
#undef PIT
#undef PITCH
#undef PTS
#undef PW
#undef PX
#undef PX0
#undef PY
#undef READY_Y
#undef REV_FILL
#undef RH
#undef RIM
#undef ROUND_MS
#undef ROWCOL
#undef ROWS
#undef ROW_COL
#undef ROW_PITCH
#undef RW
#undef RX
#undef SET_RH
#undef SET_ROWS
#undef SET_Y0
#undef SH
#undef SHIPH
#undef SHIPW
#undef SHIPY
#undef SKY
#undef SPACING
#undef ST_AI
#undef ST_DROP
#undef ST_END
#undef ST_FB
#undef ST_IDLE
#undef ST_OVER
#undef ST_PLAY
#undef ST_SHOW
#undef ST_WAIT
#undef SZ
#undef TAN
#undef TILE_COL
#undef TILE_COLS
#undef TILE_GAP
#undef TILE_H
#undef TILE_ROWS
#undef TILE_W
#undef TILE_X0
#undef TILE_Y0
#undef TONE
#undef TOP
#undef VMAX
#undef WALL_Y
#undef WINSC
#undef C_BIRD
#undef C_BLACK
#undef C_BLUE
#undef C_CAC
#undef C_CLOUD
#undef C_CYAN
#undef C_DGREY
#undef C_GOLD
#undef C_GREEN
#undef C_GREY
#undef C_LGREY
#undef C_MAGENTA
#undef C_NAVY
#undef C_ORANGE
#undef C_PINK
#undef C_PURPLE
#undef C_RED
#undef C_RUN
#undef C_TEAL
#undef C_WHITE
#undef C_WING
#undef C_YELLOW

// ------------------------------------------------------------------ geometry
#define SCR_W 320
#define SCR_H 240
#define HUD_H 22
#define PF_Y  HUD_H
#define PF_H  (SCR_H - HUD_H)

// ------------------------------------------------------------------ CYD pins
#define PIN_TOUCH_CLK  25
#define PIN_TOUCH_MOSI 32
#define PIN_TOUCH_MISO 39
#define PIN_TOUCH_CS   33
#define PIN_SPEAKER    26
#define PIN_BACKLIGHT  21

// ------------------------------------------------------------------ colours (RGB565)
#define RGB565(r, g, b) ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xFF) >> 3)))
static const uint16_t C_BLACK  = 0x0000;
static const uint16_t C_WHITE  = 0xFFFF;
static const uint16_t C_RED    = 0xF800;
static const uint16_t C_GREEN  = 0x07E0;
static const uint16_t C_BLUE   = 0x001F;
static const uint16_t C_YELLOW = 0xFFE0;
static const uint16_t C_CYAN   = 0x07FF;
static const uint16_t C_MAGENTA= 0xF81F;
static const uint16_t C_ORANGE = 0xFD20;
static const uint16_t C_GREY   = 0x8410;
static const uint16_t C_LGREY  = 0xC618;
static const uint16_t C_DGREY  = 0x2104;
static const uint16_t C_NAVY   = 0x000F;
static const uint16_t C_PURPLE = 0x8010;
static const uint16_t C_PINK   = 0xFC9F;
static const uint16_t C_GOLD   = 0xFEA0;
static const uint16_t C_TEAL   = 0x0410;

// ------------------------------------------------------------------ small helpers
static inline int imin(int a, int b) { return a < b ? a : b; }
static inline int imax(int a, int b) { return a > b ? a : b; }
static inline int iabs(int a) { return a < 0 ? -a : a; }
static inline int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
static inline int rnd(int n) { return (int)random((long)n); }                 // 0 .. n-1
static inline int rndRange(int lo, int hi) { return lo + (int)random((long)(hi - lo + 1)); }  // lo .. hi
static inline bool inRect(int px, int py, int x, int y, int w, int h) {
  return px >= x && px < x + w && py >= y && py < y + h;
}

// ------------------------------------------------------------------ globals
static TFT_eSPI tft = TFT_eSPI();
static Preferences prefs;

static bool    gSound  = true;
static bool    gFlip   = false;   // rotate display 180 degrees
static bool    gInvert = false;   // invertDisplay
static uint8_t gBright = 3;       // 1..4

static int  gCur = -1;            // index of the running game (-1 = none)
static bool gLowerBetter = false; // set by the menu from GameDef::lowerBetter

// ------------------------------------------------------------------ game registry entry
struct GameDef {
  const char* name;
  uint16_t color;      // accent colour of the menu tile
  uint16_t frameMs;    // tick interval, e.g. 33 = ~30 fps
  bool lowerBetter;    // true if a LOWER score is better (times, moves)
  void (*init)();
  void (*tick)();
  void (*icon)(int cx, int cy);
};

// ------------------------------------------------------------------ text helpers
// Transparent background. For changing text, erase the area first with fillRect.
static void text(const char* s, int x, int y, uint8_t font, uint16_t fg,
                 uint8_t datum = TL_DATUM, uint8_t size = 1) {
  tft.setTextSize(size);
  tft.setTextColor(fg);
  tft.setTextDatum(datum);
  tft.drawString(s, x, y, font);
  tft.setTextSize(1);
  tft.setTextDatum(TL_DATUM);
}
// Print a number centred (MC_DATUM) at x,y after clearing the box w x h centred on x,y.
static void numBox(long v, int x, int y, int w, int h, uint8_t font, uint16_t fg, uint16_t bg,
                   uint8_t size = 1) {
  char b[16];
  snprintf(b, sizeof b, "%ld", v);
  tft.fillRect(x - w / 2, y - h / 2, w, h, bg);
  text(b, x, y, font, fg, MC_DATUM, size);
}
// Clear a box then draw centred text in it.
static void textBox(const char* s, int x, int y, int w, int h, uint8_t font, uint16_t fg,
                    uint16_t bg, uint8_t size = 1) {
  tft.fillRect(x - w / 2, y - h / 2, w, h, bg);
  text(s, x, y, font, fg, MC_DATUM, size);
}
static void button(int x, int y, int w, int h, const char* label, uint16_t col,
                   uint16_t fg = C_WHITE, uint8_t font = 2) {
  tft.fillRoundRect(x, y, w, h, 6, col);
  tft.drawRoundRect(x, y, w, h, 6, C_WHITE);
  text(label, x + w / 2, y + h / 2, font, fg, MC_DATUM);
}

// ------------------------------------------------------------------ sound (speaker on GPIO26)
static uint32_t gBeepEnd = 0;
static void soundBegin() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(PIN_SPEAKER, 2000, 8);
#else
  ledcSetup(0, 2000, 8);
  ledcAttachPin(PIN_SPEAKER, 0);
#endif
}
static void toneOut(int freq) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWriteTone(PIN_SPEAKER, freq);
#else
  ledcWriteTone(0, freq);
#endif
}
// Non-blocking beep. A new beep replaces the previous one.
static void beep(int freq, int ms) {
  if (!gSound) return;
  toneOut(freq);
  gBeepEnd = millis() + (uint32_t)ms;
}
static void soundTick() {
  if (gBeepEnd != 0 && (int32_t)(millis() - gBeepEnd) >= 0) {
    toneOut(0);
    gBeepEnd = 0;
  }
}

// ------------------------------------------------------------------ backlight
static void backlightBegin() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(PIN_BACKLIGHT, 5000, 8);
#else
  ledcSetup(2, 5000, 8);
  ledcAttachPin(PIN_BACKLIGHT, 2);
#endif
}
static void applyBrightness() {
  static const uint8_t duty[5] = {0, 30, 90, 170, 255};
  uint8_t d = duty[clampi(gBright, 1, 4)];
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(PIN_BACKLIGHT, d);
#else
  ledcWrite(2, d);
#endif
}

// ------------------------------------------------------------------ persistent storage
static int hiGet(int id) {
  char k[8];
  snprintf(k, sizeof k, "h%d", id);
  return prefs.getInt(k, 0);
}
static void hiSet(int id, int v) {
  char k[8];
  snprintf(k, sizeof k, "h%d", id);
  prefs.putInt(k, v);
}
// Best score of the running game (0 = none yet).
static int bestScore() { return gCur >= 0 ? hiGet(gCur) : 0; }
// Offer a score for the running game. Returns true if it is a new best.
// Scores are positive ints; 0 is "no score". gLowerBetter flips the comparison.
static bool submitScore(int score) {
  if (gCur < 0 || score <= 0) return false;
  int b = hiGet(gCur);
  bool better = (b == 0) || (gLowerBetter ? (score < b) : (score > b));
  if (better) hiSet(gCur, score);
  return better;
}
static void settingsLoad() {
  gSound  = prefs.getBool("snd", true);
  gFlip   = prefs.getBool("flip", false);
  gInvert = prefs.getBool("inv", false);
  gBright = (uint8_t)clampi(prefs.getUChar("bri", 3), 1, 4);
}
static void settingsSave() {
  prefs.putBool("snd", gSound);
  prefs.putBool("flip", gFlip);
  prefs.putBool("inv", gInvert);
  prefs.putUChar("bri", gBright);
}

// ------------------------------------------------------------------ touch
// Polled once per frame by the hub just before tick(). Edge flags are valid for exactly one tick.
struct TouchState {
  bool down;         // finger currently on the screen
  bool pressed;      // became down this frame
  bool released;     // lifted this frame
  bool tap;          // (release frame) finger barely moved
  int8_t swipe;      // (release frame) 0=up 1=right 2=down 3=left, -1 = none
  int x, y;          // current position (last known position once lifted)
  int sx, sy;        // position where this touch started
  uint32_t t0;       // millis() when this touch started
  uint32_t dur;      // (release frame) how long it was held, ms
};
static TouchState touch = {false, false, false, false, -1, 160, 120, 160, 120, 0, 0};

struct TouchCal {
  int16_t swap;      // 1 if raw Y drives screen X
  int16_t ax0, ax1;  // raw value at screen x = 20 and x = 300
  int16_t ay0, ay1;  // raw value at screen y = 20 and y = 220
};
static TouchCal cal = {0, 400, 3700, 400, 3700};

static XPT2046_Touchscreen ts(PIN_TOUCH_CS);
static SPIClass touchSPI(VSPI);
static int gMiss = 0;

static void touchBegin() {
  touchSPI.begin(PIN_TOUCH_CLK, PIN_TOUCH_MISO, PIN_TOUCH_MOSI, PIN_TOUCH_CS);
  ts.begin(touchSPI);
  ts.setRotation(0);
}
static void rawToScreen(int rx, int ry, int& sx, int& sy) {
  int a = cal.swap ? ry : rx;
  int b = cal.swap ? rx : ry;
  int da = cal.ax1 - cal.ax0;
  int db = cal.ay1 - cal.ay0;
  if (da == 0) da = 1;
  if (db == 0) db = 1;
  long x = 20L + (long)(a - cal.ax0) * 280L / da;
  long y = 20L + (long)(b - cal.ay0) * 200L / db;
  sx = clampi((int)x, 0, SCR_W - 1);
  sy = clampi((int)y, 0, SCR_H - 1);
}
static void touchClear() {
  touch.pressed = false;
  touch.released = false;
  touch.tap = false;
  touch.swipe = -1;
}
static void pollTouch() {
  touchClear();
  bool now = ts.touched();
  if (now) {
    TS_Point p = ts.getPoint();
    int sx, sy;
    rawToScreen(p.x, p.y, sx, sy);
    gMiss = 0;
    if (!touch.down) {
      touch.down = true;
      touch.pressed = true;
      touch.sx = sx;
      touch.sy = sy;
      touch.t0 = millis();
    }
    touch.x = sx;
    touch.y = sy;
  } else if (touch.down) {
    gMiss++;
    if (gMiss >= 2) {       // tolerate one dropped sample (resistive panels flicker)
      gMiss = 0;
      touch.down = false;
      touch.released = true;
      touch.dur = millis() - touch.t0;
      int dx = touch.x - touch.sx;
      int dy = touch.y - touch.sy;
      int adx = iabs(dx), ady = iabs(dy);
      if (adx < 14 && ady < 14) {
        touch.tap = true;
      } else if (adx >= 24 || ady >= 24) {
        if (adx > ady) touch.swipe = (dx > 0) ? 1 : 3;
        else           touch.swipe = (dy > 0) ? 2 : 0;
      }
    }
  }
}
// Convenience tests (use inside tick()).
static bool pressedIn(int x, int y, int w, int h) {
  return touch.pressed && inRect(touch.x, touch.y, x, y, w, h);
}
static bool tappedIn(int x, int y, int w, int h) {
  return touch.tap && inRect(touch.x, touch.y, x, y, w, h);
}

// ------------------------------------------------------------------ touch calibration
static const char* calKey() { return gFlip ? "cal3" : "cal1"; }
static bool loadCalibration() {
  TouchCal c;
  size_t got = prefs.getBytes(calKey(), &c, sizeof c);
  if (got != sizeof c) return false;
  cal = c;
  return true;
}
static void drawCross(int x, int y, uint16_t col) {
  tft.drawLine(x - 14, y, x + 14, y, col);
  tft.drawLine(x, y - 14, x, y + 14, col);
  tft.drawCircle(x, y, 7, col);
}
// Returns false (and keeps the old calibration) if nobody touched the screen for ~12 s.
static bool runCalibration() {
  static const int tx[3] = {20, 300, 20};
  static const int ty[3] = {20, 20, 220};
  int rxs[3] = {0, 0, 0};
  int rys[3] = {0, 0, 0};
  for (int i = 0; i < 3; i++) {
    tft.fillScreen(C_BLACK);
    text("TOUCH CALIBRATION", SCR_W / 2, 100, 4, C_WHITE, MC_DATUM);
    text("Press the yellow crosshair", SCR_W / 2, 130, 2, C_LGREY, MC_DATUM);
    text("(a stylus is more accurate)", SCR_W / 2, 150, 2, C_GREY, MC_DATUM);
    char b[8];
    snprintf(b, sizeof b, "%d / 3", i + 1);
    text(b, SCR_W / 2, 185, 2, C_YELLOW, MC_DATUM);
    drawCross(tx[i], ty[i], C_YELLOW);
    while (ts.touched()) delay(10);       // wait until finger is lifted
    delay(150);
    uint32_t waitStart = millis();
    while (!ts.touched()) {               // wait for a press
      delay(10);
      if (millis() - waitStart > 12000) return false;
    }
    delay(60);                            // let the reading settle
    long sumX = 0, sumY = 0;
    int n = 0;
    while (ts.touched() && n < 25) {
      TS_Point p = ts.getPoint();
      sumX += p.x;
      sumY += p.y;
      n++;
      delay(8);
    }
    if (n == 0) { i--; continue; }
    rxs[i] = (int)(sumX / n);
    rys[i] = (int)(sumY / n);
    tft.fillCircle(tx[i], ty[i], 9, C_GREEN);
    beep(1500, 60);
    while (ts.touched()) delay(10);
    delay(120);
  }
  // Is screen-X driven by raw X or raw Y? Compare the TL -> TR movement.
  if (iabs(rys[1] - rys[0]) > iabs(rxs[1] - rxs[0])) {
    cal.swap = 1;
    cal.ax0 = (int16_t)rys[0]; cal.ax1 = (int16_t)rys[1];
    cal.ay0 = (int16_t)rxs[0]; cal.ay1 = (int16_t)rxs[2];
  } else {
    cal.swap = 0;
    cal.ax0 = (int16_t)rxs[0]; cal.ax1 = (int16_t)rxs[1];
    cal.ay0 = (int16_t)rys[0]; cal.ay1 = (int16_t)rys[2];
  }
  prefs.putBytes(calKey(), &cal, sizeof cal);
  tft.fillScreen(C_BLACK);
  text("Calibration saved", SCR_W / 2, SCR_H / 2, 4, C_GREEN, MC_DATUM);
  delay(600);
  return true;
}

// ------------------------------------------------------------------ HUD / overlays
static const uint16_t HUD_BG = RGB565(24, 28, 40);

// Clear the screen, draw the HUD bar with title and the MENU button (top-right).
static void hudBegin(const char* title, uint16_t bg = C_BLACK) {
  tft.fillScreen(bg);
  tft.fillRect(0, 0, SCR_W, HUD_H, HUD_BG);
  tft.drawFastHLine(0, HUD_H - 1, SCR_W, RGB565(70, 80, 100));
  text(title, 6, 3, 2, C_WHITE);
  tft.fillRoundRect(SCR_W - 50, 2, 46, HUD_H - 5, 4, RGB565(170, 40, 55));
  text("MENU", SCR_W - 27, (HUD_H - 1) / 2, 1, C_WHITE, MC_DATUM);
}
// Free-form HUD text in the middle area (x 110..265).
static void hudText(const char* s, uint16_t fg = C_YELLOW) {
  tft.fillRect(110, 1, 156, HUD_H - 3, HUD_BG);
  text(s, 188, (HUD_H - 1) / 2, 2, fg, MC_DATUM);
}
static void hudScore(long v, const char* label = "Score") {
  char b[32];
  snprintf(b, sizeof b, "%s %ld", label, v);
  hudText(b);
}
// Clear the playfield (everything below the HUD).
static void pfClear(uint16_t col = C_BLACK) { tft.fillRect(0, PF_Y, SCR_W, PF_H, col); }

// Centred dialog box with a title, optional two lines, and a hint. Used for win/lose messages.
static void overlayMsg(const char* title, uint16_t titleCol, const char* line1, const char* line2,
                       const char* hint = "Tap to continue") {
  int w = 220, h = 112;
  int x = (SCR_W - w) / 2, y = PF_Y + (PF_H - h) / 2;
  tft.fillRoundRect(x, y, w, h, 10, RGB565(14, 16, 28));
  tft.drawRoundRect(x, y, w, h, 10, titleCol);
  tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 9, titleCol);
  text(title, SCR_W / 2, y + 22, 4, titleCol, MC_DATUM);
  if (line1) text(line1, SCR_W / 2, y + 52, 2, C_WHITE, MC_DATUM);
  if (line2) text(line2, SCR_W / 2, y + 72, 2, C_GOLD, MC_DATUM);
  if (hint) text(hint, SCR_W / 2, y + 96, 1, C_GREY, MC_DATUM);
}
// "GAME OVER" box. Submits the score itself and shows the best. Call once when the game ends.
static void overlayGameOver(long score, const char* unit = "") {
  char l1[32], l2[32];
  snprintf(l1, sizeof l1, "Score: %ld%s", score, unit);
  bool nb = submitScore((int)score);
  if (nb) snprintf(l2, sizeof l2, "NEW BEST!");
  else    snprintf(l2, sizeof l2, "Best: %d%s", bestScore(), unit);
  overlayMsg("GAME OVER", C_RED, l1, l2, "Tap to play again");
}
