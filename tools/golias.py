#!/usr/bin/env python3
"""
golias.py — Golias Engine build tooling CLI

Usage:
    python tools/golias.py bake shaders [--backend vulkan] [--slangc PATH]
    python tools/golias.py bake <asset-type> ...

Run from the engine root directory.
"""

import sys
import argparse
from pathlib import Path

from commands import shader_baker

SCRIPT_DIR = Path(__file__).resolve().parent
ENGINE_ROOT = SCRIPT_DIR.parent

# Every module here must expose register(subparsers), which adds itself
# as a `bake <name>` subcommand. To add a new bakeable asset type later
# (textures, meshes, ...): write commands/texture_baker.py with its own
# register(subparsers) + cmd_bake_textures(args), then add it to this list.
BAKE_MODULES = [
    shader_baker,
]


def engine_root_check():
    if not (ENGINE_ROOT / "CMakeLists.txt").exists():
        print(f"Error: Cannot find CMakeLists.txt at {ENGINE_ROOT}", file=sys.stderr)
        sys.exit(1)


def main():
    engine_root_check()

    parser = argparse.ArgumentParser(
        prog="golias",
        description="Golias Engine — build tooling CLI",
    )
    sub = parser.add_subparsers(dest="command", required=True)

    p_bake = sub.add_parser("bake", help="Bake game assets (shaders, textures, ...)")
    bake_sub = p_bake.add_subparsers(dest="bake_command", required=True)

    for module in BAKE_MODULES:
        module.register(bake_sub)

    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()