#!/usr/bin/env python3
"""
Build the CYD Game Hub firmware (two display variants) and put the flashable images where the
web flasher expects them:

    web/firmware/cyd-gamehub-ili9341.bin   (merged image, flash at offset 0)
    web/firmware/cyd-gamehub-st7789.bin
    web/manifest-ili9341.json / web/manifest-st7789.json

Needs only `arduino-cli` on PATH (https://arduino.github.io/arduino-cli/ - on Windows: `winget install ArduinoSA.CLI`).
It installs the ESP32 core + libraries itself. Usage:   python tools/build.py [--version 1.0] [--only ili9341]
"""
import argparse, glob, json, os, shutil, subprocess, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
SKETCH = os.path.join(ROOT, "firmware", "CydGameHub")
WEB = os.path.join(ROOT, "web")
OUT = os.path.join(ROOT, "build")

CORE = "esp32:esp32"
CORE_VERSION = "2.0.17"
LIBS = ["TFT_eSPI@2.5.43", "XPT2046_Touchscreen@1.4"]
FQBN = "esp32:esp32:esp32:PartitionScheme=default,FlashMode=dio,FlashFreq=40,FlashSize=4M,UploadSpeed=921600"

# TFT_eSPI is configured purely through compiler flags (no User_Setup.h edit needed).
COMMON = {
    "USER_SETUP_LOADED": "1",
    "TFT_WIDTH": "240", "TFT_HEIGHT": "320",
    "TFT_MISO": "12", "TFT_MOSI": "13", "TFT_SCLK": "14", "TFT_CS": "15", "TFT_DC": "2", "TFT_RST": "-1",
    "TFT_BL": "21", "TFT_BACKLIGHT_ON": "HIGH",
    "USE_HSPI_PORT": "1",
    "LOAD_GLCD": "1", "LOAD_FONT2": "1", "LOAD_FONT4": "1",
    "SPI_FREQUENCY": "40000000", "SPI_READ_FREQUENCY": "20000000", "SPI_TOUCH_FREQUENCY": "2500000",
}
VARIANTS = {
    # standard single-USB CYD (ESP32-2432S028R, ILI9341)
    "ili9341": {"label": "Standard CYD (ILI9341)", "defs": {"ILI9341_2_DRIVER": "1"}},
    # CYD with USB-C + micro-USB ("2-USB" revision, ST7789)
    "st7789": {"label": "2-USB CYD (ST7789)", "defs": {"ST7789_DRIVER": "1"}},
}


def run(cmd, **kw):
    print("+", " ".join(cmd), flush=True)
    subprocess.run(cmd, check=True, **kw)


def find_cli():
    cli = shutil.which("arduino-cli")
    if not cli:
        sys.exit("arduino-cli not found on PATH. Install it from https://arduino.github.io/arduino-cli/ "
                 "(Windows: winget install ArduinoSA.CLI) and run this again.")
    return cli


def flags(defs):
    d = dict(COMMON)
    d.update(defs)
    return " ".join("-D%s=%s" % (k, v) for k, v in d.items())


def merge_fallback(build_dir, out_bin):
    """Used only if arduino-cli did not produce *.merged.bin: merge the pieces with esptool."""
    def one(pattern):
        m = glob.glob(os.path.join(build_dir, pattern))
        if not m:
            sys.exit("missing build output: " + pattern)
        return m[0]
    boot = one("*.ino.bootloader.bin")
    part = one("*.ino.partitions.bin")
    app = one("*.ino.bin")
    home = os.path.expanduser("~")
    cands = glob.glob(os.path.join(home, ".arduino15", "packages", "esp32", "hardware", "esp32", "*", "tools", "partitions", "boot_app0.bin"))
    cands += glob.glob(os.path.join(os.environ.get("LOCALAPPDATA", "/nonexistent"), "Arduino15", "packages", "esp32", "hardware", "esp32", "*", "tools", "partitions", "boot_app0.bin"))
    if not cands:
        sys.exit("boot_app0.bin not found")
    run([sys.executable, "-m", "esptool", "--chip", "esp32", "merge_bin", "-o", out_bin,
         "--flash_mode", "dio", "--flash_freq", "40m", "--flash_size", "4MB",
         "0x1000", boot, "0x8000", part, "0xe000", cands[0], "0x10000", app])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--version", default="1.0")
    ap.add_argument("--only", choices=list(VARIANTS), help="build just one variant")
    ap.add_argument("--skip-install", action="store_true", help="do not (re)install core/libraries")
    a = ap.parse_args()

    cli = find_cli()
    if not a.skip_install:
        run([cli, "core", "update-index"])
        run([cli, "core", "install", "%s@%s" % (CORE, CORE_VERSION)])
        run([cli, "lib", "install"] + LIBS)

    os.makedirs(os.path.join(WEB, "firmware"), exist_ok=True)
    with open(os.path.join(SKETCH, "version.h"), "w") as f:
        f.write('#pragma once\n#define HUB_VERSION "%s"\n' % a.version)
    for name, v in VARIANTS.items():
        if a.only and a.only != name:
            continue
        bdir = os.path.join(OUT, name)
        shutil.rmtree(bdir, ignore_errors=True)
        extra = flags(v["defs"])
        # Windows/Linux alike: the whole thing is ONE argv element, so no shell quoting is involved.
        run([cli, "compile", "--fqbn", FQBN, "--output-dir", bdir, "--warnings", "none",
             "--build-property", "compiler.cpp.extra_flags=" + extra, SKETCH])
        merged = glob.glob(os.path.join(bdir, "*.merged.bin"))
        dest = os.path.join(WEB, "firmware", "cyd-gamehub-%s.bin" % name)
        if merged:
            shutil.copyfile(merged[0], dest)
        else:
            merge_fallback(bdir, dest)
        size = os.path.getsize(dest)
        if size < 200_000:
            sys.exit("suspiciously small image (%d bytes): %s" % (size, dest))
        manifest = {
            "name": "CYD Game Hub - " + v["label"],
            "version": a.version,
            "new_install_prompt_erase": False,
            "builds": [{"chipFamily": "ESP32",
                        "parts": [{"path": "firmware/cyd-gamehub-%s.bin" % name, "offset": 0}]}],
        }
        with open(os.path.join(WEB, "manifest-%s.json" % name), "w") as f:
            json.dump(manifest, f, indent=2)
        print("OK  %-8s %7d bytes -> %s" % (name, size, dest))
    print("\nDone. Serve the flasher locally with:  python tools/serve.py")


if __name__ == "__main__":
    main()
