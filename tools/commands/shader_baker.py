"""
commands/shader_baker.py — bake .slang shaders into backend-specific bytecode
"""

import os
import platform
import shutil
import subprocess
import sys
import time
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
ENGINE_ROOT = SCRIPT_DIR.parent.parent
SHADER_SRC_DIR = ENGINE_ROOT / "res" / "internal" / "shaders"
SHADER_EXTS = (".hlsl", ".slang")


BACKENDS = {
    "vulkan": {
        "target": "spirv",
        "profile": "spirv_1_5",
        "out_ext": ".spv",
       
        "extra": [
            "-emit-spirv-directly",
            "-fvk-use-entrypoint-name",
        ],
        "ready": True,
        "host_platforms": None,
    },
    "d3d12": {
        "target": "dxil",
        "profile": "sm_6_6",
        "out_ext": ".dxil",
        "extra": [],
        "ready": True,
        # slangc bundles dxcompiler, so DXIL can be produced from any host.
        "host_platforms": None,
    },
    "metal": {
        "target": "metallib",
        "profile": None,
        "out_ext": ".metallib",
        "extra": [],
        "ready": True,
        # Producing a compiled .metallib needs Apple's Metal toolchain (xcrun metal/metallib), which only exists on macOS.
        "host_platforms": ["Darwin"],
    },
}

DEFAULT_BACKEND = "vulkan"


def find_slangc(explicit_path: str | None) -> str:
    if explicit_path:
        if not Path(explicit_path).exists():
            print(f"Error: --slangc path does not exist: {explicit_path}", file=sys.stderr)
            sys.exit(1)
        return explicit_path

    env_path = os.environ.get("SLANGC")
    if env_path:
        return env_path

    found = shutil.which("slangc")
    if found:
        return found

    print(
        "Error: Could not find 'slangc' on PATH.\n"
        "  Install the Slang compiler (https://github.com/shader-slang/slang)\n"
        "  and either add it to PATH, set the SLANGC env var, or pass --slangc.",
        file=sys.stderr,
    )
    sys.exit(1)


def resolve_backend(name: str) -> dict:
    cfg = BACKENDS.get(name)
    if cfg is None:
        print(f"Error: Unknown backend '{name}'. Available: {', '.join(BACKENDS)}", file=sys.stderr)
        sys.exit(1)
    if not cfg["ready"]:
        print(f"Error: Backend '{name}' is registered but not implemented yet.", file=sys.stderr)
        sys.exit(1)

    host_platforms = cfg.get("host_platforms")
    if host_platforms is not None and platform.system() not in host_platforms:
        print(
            f"Error: Backend '{name}' can only be baked on {', '.join(host_platforms)} "
            f"(current host: {platform.system()}).",
            file=sys.stderr,
        )
        sys.exit(1)

    return cfg


def find_shader_sources() -> list[Path]:
    if not SHADER_SRC_DIR.is_dir():
        print(f"Error: Shader source directory not found: {SHADER_SRC_DIR}", file=sys.stderr)
        sys.exit(1)
    sources = [p for ext in SHADER_EXTS for p in SHADER_SRC_DIR.rglob(f"*{ext}")]
    return sorted(set(sources))


def is_stale(src: Path, out: Path) -> bool:
    if not out.exists():
        return True
    return src.stat().st_mtime > out.stat().st_mtime


def cmd_bake_shaders(args):
    start_time = time.time()

    backend_name = args.backend or DEFAULT_BACKEND
    backend = resolve_backend(backend_name)
    slangc = find_slangc(args.slangc)

    sources = find_shader_sources()
    if not sources:
        print(f"No {', '.join(SHADER_EXTS)} files found under {SHADER_SRC_DIR}")
        return

    # Baked bytecode lives next to the source shader (res/internal/shaders/<backend>)
    # so the engine can load it at runtime using a stable asset path.
    out_root = SHADER_SRC_DIR / backend_name
    out_root.mkdir(parents=True, exist_ok=True)

    print(f"Baking shaders  backend={backend_name}  target={backend['target']}")
    print(f"  Source: {SHADER_SRC_DIR}")
    print(f"  Output: {out_root}\n")

    baked = 0
    skipped = 0
    failed = 0

    for src in sources:
        rel = src.relative_to(SHADER_SRC_DIR)
        out_path = (out_root / rel).with_suffix(backend["out_ext"])

        if not is_stale(src, out_path):
            skipped += 1
            continue

        out_path.parent.mkdir(parents=True, exist_ok=True)

        cmd = [slangc, str(src), "-target", backend["target"]]
        if backend["profile"]:
            cmd.extend(["-profile", backend["profile"]])
        cmd.extend(backend["extra"])
        cmd.extend(["-o", str(out_path)])

        print(f"  [{backend_name}] {rel}")
        result = subprocess.run(cmd, cwd=ENGINE_ROOT, capture_output=True, text=True)

        if result.returncode != 0:
            failed += 1
            print(f"    FAILED: {rel}", file=sys.stderr)
            if result.stderr.strip():
                print(f"    {result.stderr.strip()}", file=sys.stderr)
            if out_path.exists():
                out_path.unlink()
            continue

        if result.stderr.strip():
            # slangc prints warnings to stderr even on success
            print(f"    {result.stderr.strip()}")

        baked += 1

    print()
    print(f"Baked: {baked}   Skipped (up to date): {skipped}   Failed: {failed}")

    elapsed = time.time() - start_time
    print(f"Elapsed time: {elapsed:.2f} seconds")

    if failed:
        sys.exit(1)

def register(subparsers):
    p = subparsers.add_parser("shaders", help=f"Compile {'/'.join(SHADER_EXTS)} shaders from res/internal/shaders")
    p.add_argument(
        "--backend", "-b",
        choices=list(BACKENDS.keys()),
        default=None,
        help=f"Target backend (default: {DEFAULT_BACKEND})",
    )
    p.add_argument("--slangc", help="Path to the slangc executable (default: search PATH / SLANGC env var)")
    p.set_defaults(func=cmd_bake_shaders)