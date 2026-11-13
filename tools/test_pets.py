#!/usr/bin/env python3
"""Check pet assets, skeletal animation, move reviews and optional hidden OpenGL rendering.

Run tools/build.py first. The checks use the same compiler and cached libraries.
"""
import argparse
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys

from build import BACKENDS
from build_assimp import build_assimp
from setup_engine import ROOT


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--render", action="store_true")
    parser.add_argument("--no-engine", action="store_true")
    parser.add_argument("--compiler", default=os.environ.get("CXX", "g++" if os.name == "nt" else "c++"))
    parser.add_argument("--cc", default=os.environ.get("CC", "gcc" if os.name == "nt" else "cc"))
    args = parser.parse_args()
    compiler = shutil.which(args.compiler) or shutil.which("C:/MinGW/bin/g++.exe")
    if not compiler:
        raise RuntimeError("Compilador C++ não encontrado")
    os.environ["PATH"] = str(Path(compiler).parent) + os.pathsep + os.environ.get("PATH", "")
    cc = shutil.which(args.cc)
    if not cc:
        raise RuntimeError("Compilador C não encontrado")
    system = platform.system()
    build = ROOT / "build" / system.lower()
    output = build / "bin"
    if not output.is_dir():
        raise RuntimeError("Execute python tools/build.py primeiro")
    includes, library = build_assimp(compiler, cc, build / "obj" / "assimp")
    for folder in ("shaders", "assets/scenarios", "assets/objects/pets"):
        shutil.copytree(ROOT / folder, output / folder, dirs_exist_ok=True)
    sources = [ROOT / p for p in ("tests/pets.cpp", "tests/pet_render.cpp", "src/jogo.cpp", "src/stockfish.cpp")]
    sources += sorted((ROOT / "src/graphics").glob("*.cpp"))
    sources += sorted((ROOT / "src/pieces").glob("*.cpp"))
    sources += [ROOT / "vendor/imgui" / p for p in ("imgui.cpp", "imgui_draw.cpp", "imgui_tables.cpp",
        "imgui_widgets.cpp", "backends/imgui_impl_glfw.cpp", "backends/imgui_impl_opengl3.cpp")]
    executable = output / ("pet_checks.exe" if os.name == "nt" else "pet_checks")
    command = [compiler, "-std=c++17", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-pthread"]
    command += ["-I" + str(ROOT / p) for p in ("include", "src", "vendor/chess-library", "vendor/glfw/include",
        "vendor/glfw/deps", "vendor/imgui", "vendor/imgui/backends")]
    command += ["-I" + str(p) for p in includes]
    command += [str(p) for p in sources]
    command += [str(p) for p in sorted((build / "obj/glfw").glob("*.o"))]
    command += [str(library)] + BACKENDS[system][2]
    if system == "Windows":
        command += ["-static"]
    command += ["-o", str(executable)]
    subprocess.run(command, check=True)
    flags = (["--no-engine"] if args.no_engine else []) + (["--render"] if args.render else [])
    return subprocess.call([str(executable)] + flags)


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print("ERRO:", error, file=sys.stderr)
        sys.exit(1)
