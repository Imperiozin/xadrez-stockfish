#!/usr/bin/env python3
"""Compila sem CMake e sem pacotes Python externos."""
import argparse
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys

from setup_engine import ROOT, prepare


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", action="store_true", help="Jogar depois do build")
    parser.add_argument("--test", action="store_true", help="Testar a API com o motor real")
    parser.add_argument("--engine", help="Stockfish local alternativo")
    parser.add_argument("--output", help="Pasta de saída alternativa")
    parser.add_argument("--compiler", default=os.environ.get("CXX", "g++"))
    args = parser.parse_args()
    output = Path(args.output).resolve() if args.output else ROOT / "build" / platform.system().lower() / "bin"
    try:
        compiler = shutil.which(args.compiler)
        if compiler is None:
            raise RuntimeError("Compilador não encontrado: " + args.compiler +
                               ". Adicione a pasta bin do MinGW/GCC ao PATH ou use --compiler caminho.")
        output.mkdir(parents=True, exist_ok=True)
        prepare(output, args.engine)
        executable = output / ("xadrez.exe" if os.name == "nt" else "xadrez")
        command = [compiler, "-std=c++17", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-pthread",
                   "-I" + str(ROOT / "vendor" / "chess-library"), "-I" + str(ROOT / "src"),
                   str(ROOT / "src" / "main.cpp"), str(ROOT / "src" / "jogo.cpp"),
                   str(ROOT / "src" / "stockfish.cpp"), str(ROOT / "src" / "autoteste.cpp"),
                   "-o", str(executable)]
        if os.name == "nt":
            command.append("-static")  # Evita depender das DLLs do MinGW ao executar.
        print("Compilando C++17...", flush=True)
        subprocess.run(command, check=True)
        print(f"Build concluído: {executable}", flush=True)
        if args.test:
            subprocess.run([str(executable), "--self-test", "--time", "50"], check=True)
        if args.run:
            return subprocess.call([str(executable)])
        return 0
    except (OSError, RuntimeError, subprocess.CalledProcessError) as exc:
        print(f"ERRO: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
