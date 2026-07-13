#!/usr/bin/env python3
"""Build .web64proj files from project sources.

Each project declares a manifest at projects/<name>/web64proj.json:

  {
    "main": "main.asm",              // main source: file to read, OR
    "mainName": "raytracer-c.asm",   //   virtual name + inline stub lines
    "mainStub": ["; line1", ...],    //   (for projects with no .asm at all)
    "origin": "$4000",
    "entry": "start",
    "files": ["lib/fixmath.asm"],    // virtual files (symlinks are followed)
    "c": { ... },                    // optional Web64 C config block
    "emulator": { "warpEnabled": true }  // overrides over EMULATOR_DEFAULTS
  }

Output is deterministic (no timestamps), so a rebuild with unchanged
sources is byte-identical — `--check` uses that to verify the committed
.web64proj files are in sync.

Usage:
  tools/build_web64proj.py [--check] [project ...]   (default: all)
"""

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
MANIFEST = "web64proj.json"

EMULATOR_DEFAULTS = {
    "emulatorScale": 0,
    "livePatchEnabled": True,
    "audioEnabled": False,
    "audioMuted": False,
    "audioVolume": 1,
    "sidQuality": "fastsid",
    "scanlinesEnabled": True,
    "warpEnabled": False,
    "drive9Enabled": True,
    "gamepadEnabled": False,
    "joystickPort": 2,
}


def build(project_dir: Path) -> tuple[Path, str]:
    manifest = json.loads((project_dir / MANIFEST).read_text())

    if "main" in manifest:
        main_name = manifest["main"]
        main_source = (project_dir / main_name).read_text()
    else:
        main_name = manifest["mainName"]
        main_source = "\n".join(manifest["mainStub"]) + "\n"

    proj = {
        "kind": "web64.ide.project",
        "version": 2,
        "main": {
            "fileName": main_name,
            "origin": manifest.get("origin", "$c000"),
            "source": main_source,
            "selectedEntryLabel": manifest.get("entry", "start"),
        },
        "emulator": {**EMULATOR_DEFAULTS, **manifest.get("emulator", {})},
        "files": [
            {
                "path": p,
                "name": p.rsplit("/", 1)[-1],
                "kind": "source",
                "source": (project_dir / p).read_text(),
            }
            for p in manifest.get("files", [])
        ],
    }
    if "c" in manifest:
        proj["c"] = manifest["c"]

    out = project_dir / f"{project_dir.name}.web64proj"
    return out, json.dumps(proj, indent=1) + "\n"


def main() -> int:
    args = sys.argv[1:]
    check = "--check" in args
    names = [a for a in args if not a.startswith("-")]

    project_dirs = (
        [ROOT / "projects" / n for n in names]
        if names
        else sorted(d for d in (ROOT / "projects").iterdir() if (d / MANIFEST).exists())
    )

    stale = []
    for d in project_dirs:
        if not (d / MANIFEST).exists():
            print(f"error: {d} has no {MANIFEST}", file=sys.stderr)
            return 1
        out, content = build(d)
        rel = out.relative_to(ROOT)
        if check:
            if not out.exists() or out.read_text() != content:
                stale.append(rel)
                print(f"STALE  {rel}")
            else:
                print(f"ok     {rel}")
        else:
            out.write_text(content)
            print(f"built  {rel}")

    if stale:
        print(f"\n{len(stale)} file(s) out of date — run `make proj`", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
