# CYD Game Hub

15 touch games for the **2.8" resistive-touch Cheap Yellow Display** (ESP32-2432S028R), with a one-click
**browser flasher** (Chrome / Edge, Web Serial, no drivers or IDE needed beyond the USB-serial driver).

Snake, Flap, Breakout, Pong, Invaders, Tetris, 2048, Simon, Whack-a-mole, Runner, Tic-Tac-Toe, Connect 4,
Memory, Minesweeper, Lights Out. High scores and settings are saved on the board.

## Get a flashable site (pick one)

The firmware has to be compiled once; this repo does that for you two ways.

### A. Locally (Windows / macOS / Linux)

1. Install [arduino-cli](https://arduino.github.io/arduino-cli/) (Windows: `winget install ArduinoSA.CLI`) and Python 3.
2. `pip install esptool` (only used as a fallback when arduino-cli doesn't emit a merged image)
3. `python tools/build.py`  - installs the ESP32 core 2.0.17 + libraries, builds both display variants,
   writes `web/firmware/*.bin` and `web/manifest-*.json`.
4. `python tools/serve.py`  - opens http://localhost:8000. Pick your board, click flash.

### B. On GitHub (gives you a public flasher URL)

Push this folder to a GitHub repo, then **Settings -> Pages -> Source: GitHub Actions**. The included workflow
(`.github/workflows/build.yml`) compiles both variants and publishes `web/`. Share the Pages URL; anyone can flash.

## Which board variant?

| Variant | Boards | Display driver |
|---|---|---|
| `ili9341` | the common single micro-USB CYD | `ILI9341_2_DRIVER` |
| `st7789`  | newer revision with USB-C + micro-USB | `ST7789_DRIVER` |

If you see a white / garbled screen, flash the other variant. Inverted colours: **Settings -> Invert colours**.
TFT_eSPI is configured entirely through build flags in `tools/build.py` (no `User_Setup.h` editing).

## Using it

* Tap a tile to play; swipe or `<` `>` for more pages; **MENU** (top right) leaves a game.
* **Settings**: sound, brightness, screen rotation (separate touch calibration per orientation), invert colours,
  touch calibration, clear high scores. Hold **BOOT** while powering up to force touch calibration.
* First boot asks you to touch three crosshairs (a stylus is more accurate on a resistive panel).

## Layout

```
firmware/CydGameHub/   CydGameHub.ino (menu, settings, main loop), hub.h (touch, HUD, sound, scores),
                       games.h (registry), game_*.h (one namespace per game)
web/                   index.html + manifests; firmware/*.bin appear after a build
tools/build.py         build both variants -> web/
tools/serve.py         local https-less server for Web Serial (localhost)
tools/check_syntax.sh  quick host syntax check against stub headers
tools/sim/             host fuzz test: runs every game under ASan/UBSan with scripted touches (tools/sim/run.sh)
```

## Adding a game

Write `game_x.h` with `namespace X { static void init(); static void tick(); static void icon(int cx, int cy); }`
(see the rules at the top of `hub.h` and `game_snake.h` as a template), `#include` it in `games.h`, add one row to
`GAMES[]`. Menu pages and the score table grow automatically.

## Status

Developed without access to the ESP32 toolchain or a physical board: the code is syntax-checked and fuzz-tested on a
PC (every game, 7 touch scenarios, AddressSanitizer + UBSan, no findings), but **not yet run on real hardware**.
Likely first-run tweaks: display variant / inversion, and touch calibration.
