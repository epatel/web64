#!/usr/bin/env python3
"""Run the project demos in the web64 IDE (headless Chromium) and
verify what lands on the C64 screen.

There is no local assembler or emulator: each demo's .web64proj is fed
to the real IDE at https://web64.nofs.ai/ide/ via its project file
input, started with the Start PRG button, and checked by sampling the
emulator canvas. Checks are structural (color/region/fraction based),
not golden-image based, so they tolerate palette and scaling detail:

  fixmath     draws frame/fan/circles, then a numeric self-test sets
              the border GREEN (pass) or RED (fail) -> border color
  breakout    animated game -> consecutive frames differ + colored
              brick rows present
  raytracer   deterministic dithered render (warp on) -> wait until
  raytracer-c the frame is static, then check sky dither, the sphere
              highlight, the cast shadow, and overall white fraction

Requires: playwright + pillow, plus `playwright install chromium`
(the Makefile's `make test` bootstraps all of this into .venv).

Usage: tools/run_demos.py [demo ...]     (default: all)
"""

import io
import sys
import time
from pathlib import Path

try:
    from PIL import Image
    from playwright.sync_api import sync_playwright
except ImportError as e:
    print(f"missing dependency ({e.name}) — run via `make test`", file=sys.stderr)
    sys.exit(2)

ROOT = Path(__file__).resolve().parent.parent
IDE_URL = "https://web64.nofs.ai/ide/"

# canvas is 384x272: 320x200 visible screen at offset (32, 36)
CANVAS = (384, 272)
SCREEN = (32, 36, 352, 236)


def grab(page) -> Image.Image:
    png = page.locator("canvas").first.screenshot()
    return Image.open(io.BytesIO(png)).convert("RGB").resize(CANVAS)


def region(img, x0, y0, x1, y1):
    """Crop a region given in 320x200 screen coordinates."""
    return img.crop((SCREEN[0] + x0, SCREEN[1] + y0, SCREEN[0] + x1, SCREEN[1] + y1))


def white_frac(img) -> float:
    px = list(img.convert("L").getdata())
    return sum(1 for v in px if v > 180) / len(px)


def expect(cond, what):
    print(f"  {'ok  ' if cond else 'FAIL'} {what}")
    return cond


def wait_stable(page, interval=15, timeout=420):
    """Wait until two consecutive canvas grabs are identical (program
    finished drawing and sits in its halt loop)."""
    deadline = time.time() + timeout
    prev = None
    while time.time() < deadline:
        time.sleep(interval)
        cur = grab(page).tobytes()
        if prev is not None and cur == prev:
            return True
        prev = cur
    return False


def check_fixmath(page):
    ok = wait_stable(page, interval=5, timeout=120)
    ok = expect(ok, "demo finished (screen static)") and ok
    img = grab(page)
    r, g, b = img.getpixel((8, CANVAS[1] // 2))  # left border
    ok = expect(g > r + 40 and g > b + 40, f"border green = self-test pass (rgb {r},{g},{b})") and ok
    ok = expect(white_frac(region(img, 0, 0, 320, 200)) > 0.02, "frame/fan/circles drawn") and ok
    return ok

def check_breakout(page):
    # with nobody at the paddle the ball drains all lives within
    # seconds and the game-over screen is static — poll fast to catch
    # the animation while it lasts
    animated = False
    prev = grab(page)
    for _ in range(12):
        time.sleep(1)
        cur = grab(page)
        if cur.tobytes() != prev.tobytes():
            animated = True
            break
        prev = cur
    ok = expect(animated, "game animates (frames differ)")
    img = grab(page)
    bricks = region(img, 20, 30, 300, 70).quantize(16).getcolors()
    ok = expect(bricks and len(bricks) >= 4, f"colored brick rows ({len(bricks or [])} colors)") and ok
    return ok

def check_raytracer(page):
    ok = wait_stable(page)  # full frame is minutes even under warp
    ok = expect(ok, "render finished (screen static)") and ok
    img = grab(page)
    wf = white_frac(region(img, 0, 0, 320, 200))
    ok = expect(0.20 < wf < 0.70, f"overall dither density {wf:.2f}") and ok
    sky = white_frac(region(img, 10, 5, 60, 20))
    ok = expect(0.15 < sky < 0.95, f"sky dithered {sky:.2f}") and ok
    hi = white_frac(region(img, 85, 28, 115, 48))
    ok = expect(hi > 0.5, f"sphere highlight bright {hi:.2f}") and ok
    sh = white_frac(region(img, 150, 135, 185, 155))
    ok = expect(sh < 0.15, f"cast shadow dark {sh:.2f}") and ok
    return ok


DEMOS = {
    "fixmath": check_fixmath,
    "breakout": check_breakout,
    "raytracer": check_raytracer,
    "raytracer-c": check_raytracer,
}


def boot_screen(img) -> bool:
    """True while the border still shows the C64 boot light blue."""
    r, g, b = img.getpixel((8, CANVAS[1] // 2))
    return b > 200 and 90 < r < 160 and 100 < g < 170


def start_program(page) -> bool:
    """Click Start PRG until the program actually takes over the
    screen (border leaves the boot light blue). On a cold emulator
    the IDE types SYS before BASIC reaches READY and the keystrokes
    are lost — a later retry lands."""
    for attempt in range(5):
        page.locator('button[title="Start PRG"]').click()
        deadline = time.time() + 45
        while time.time() < deadline:
            time.sleep(5)
            if not boot_screen(grab(page)):
                print(f"  ok   program started (attempt {attempt + 1})")
                return True
    return False


def run_demo(browser, name, checker) -> bool:
    print(f"\n=== {name} ===")
    proj = ROOT / "projects" / name / f"{name}.web64proj"
    page = browser.new_page(viewport={"width": 1400, "height": 900})
    try:
        page.goto(IDE_URL, wait_until="networkidle")
        page.locator('input[type=file][accept*="web64proj"]').set_input_files(proj)
        page.get_by_text("Compile ready").wait_for(timeout=30000)
        print("  ok   loaded, compile ready")
        if not expect(start_program(page), "program running (left boot screen)"):
            return False
        return checker(page)
    finally:
        page.close()


def main() -> int:
    names = sys.argv[1:] or list(DEMOS)
    for n in names:
        if n not in DEMOS:
            print(f"unknown demo '{n}' (have: {', '.join(DEMOS)})", file=sys.stderr)
            return 2
    failed = []
    with sync_playwright() as pw:
        browser = pw.chromium.launch()
        for n in names:
            try:
                if not run_demo(browser, n, DEMOS[n]):
                    failed.append(n)
            except Exception as e:
                print(f"  FAIL {type(e).__name__}: {e}")
                failed.append(n)
        browser.close()
    print(f"\n{'FAILED: ' + ', '.join(failed) if failed else 'all demos passed'}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
