#!/usr/bin/env python3
"""Prepara um Stockfish oficial local. Requer apenas Python 3.9+ e sua stdlib."""
import argparse
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import tarfile
import tempfile
import urllib.request
import zipfile

ASSETS = {
    "windows-x64": "stockfish-windows-x86-64.zip",
    "linux-x64": "stockfish-ubuntu-x86-64.tar",
    "macos-x64": "stockfish-macos-x86-64.tar",
    "macos-arm64": "stockfish-macos-m1-apple-silicon.tar",
}
VERSION = "17.1"
TAG = "sf_17.1"  # Versao fixa para builds reproduziveis.
BASE = f"https://github.com/official-stockfish/Stockfish/releases/download/{TAG}/"
ROOT = Path(__file__).resolve().parents[1]


def platform_asset():
    system, machine = platform.system(), platform.machine().lower()
    x64 = machine in ("amd64", "x86_64", "x64")
    arm64 = machine in ("arm64", "aarch64")
    if system == "Windows" and x64:
        return "windows-x64", "stockfish-windows-x86-64.zip", "stockfish.exe"
    if system == "Linux" and x64:
        return "linux-x64", "stockfish-ubuntu-x86-64.tar", "stockfish"
    if system == "Darwin" and x64:
        return "macos-x64", "stockfish-macos-x86-64.tar", "stockfish"
    if system == "Darwin" and arm64:
        return "macos-arm64", "stockfish-macos-m1-apple-silicon.tar", "stockfish"
    raise RuntimeError(
        f"Download automatico indisponivel para {system}/{machine}. "
        "Use --engine /caminho/do/stockfish para fornecer um motor compativel."
    )


def verify_engine(path):
    try:
        result = subprocess.run(
            [str(path)], input="uci\nquit\n", capture_output=True,
            text=True, errors="replace", timeout=20,
            cwd=str(path.parent),
        )
    except (OSError, subprocess.TimeoutExpired) as exc:
        raise RuntimeError(f"Nao foi possivel iniciar {path}: {exc}") from exc
    if result.returncode or "uciok" not in result.stdout or "Stockfish" not in result.stdout:
        raise RuntimeError("O motor nao respondeu ao handshake UCI como Stockfish. "
                           "Confira SO, arquitetura e dependencias do executavel.\n" +
                           (result.stdout + result.stderr)[-1500:])


def install_archive(archive, destination):
    # Copia arquivos regulares individualmente, sem extrair caminhos ou symlinks.
    if archive.suffix == ".zip":
        with zipfile.ZipFile(archive) as z:
            for member in z.infolist():
                if member.is_dir():
                    continue
                parts = Path(member.filename).parts
                if ".." in parts or Path(member.filename).is_absolute():
                    raise RuntimeError("Caminho inesperado no pacote do motor.")
                target = destination.joinpath(*parts)
                target.parent.mkdir(parents=True, exist_ok=True)
                with z.open(member) as src, target.open("wb") as dst:
                    shutil.copyfileobj(src, dst)
    else:
        with tarfile.open(archive) as t:
            for member in t:
                if not member.isfile():
                    continue
                parts = Path(member.name).parts
                if ".." in parts or Path(member.name).is_absolute():
                    raise RuntimeError("Caminho inesperado no pacote do motor.")
                target = destination.joinpath(*parts)
                target.parent.mkdir(parents=True, exist_ok=True)
                with t.extractfile(member) as src, target.open("wb") as dst:
                    shutil.copyfileobj(src, dst)


def prepare(output, custom=None):
    output = Path(output).resolve()
    directory = output / "engines"
    directory.mkdir(parents=True, exist_ok=True)
    name = "stockfish.exe" if os.name == "nt" else "stockfish"
    engine = directory / name
    if custom:
        custom = Path(custom).resolve()
        verify_engine(custom)
        if custom != engine:
            shutil.copy2(custom, engine)
        if os.name != "nt":
            engine.chmod(engine.stat().st_mode | 0o111)
        verify_engine(engine)
        print(f"Motor local pronto: {engine}", flush=True)
        return engine
    key, asset, name = platform_asset()
    marker = directory / "install.json"
    bundled = ROOT / "vendor" / "stockfish" / key
    if bundled.is_dir():
        import hashlib
        manifest = json.loads((bundled / "bundle.json").read_text(encoding="utf-8"))
        source = bundled / name
        digest = hashlib.sha256(source.read_bytes()).hexdigest()
        if digest != manifest["sha256"]:
            raise RuntimeError("Binario incluido esta corrompido. Extraia o ZIP novamente.")
        current = None
        if marker.is_file():
            try:
                current = json.loads(marker.read_text(encoding="utf-8"))
            except (ValueError, OSError):
                pass
        if not engine.is_file() or current != manifest or hashlib.sha256(engine.read_bytes()).hexdigest() != digest:
            shutil.copytree(bundled, directory, dirs_exist_ok=True)
            marker.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
        if os.name != "nt":
            engine.chmod(engine.stat().st_mode | 0o111)
        verify_engine(engine)
        print(f"Stockfish local pronto (sem download): {engine}", flush=True)
        return engine
    if engine.is_file() and marker.is_file():
        try:
            data = json.loads(marker.read_text(encoding="utf-8"))
        except (ValueError, OSError):
            data = {}
        if data.get("platform") == key and data.get("version") == VERSION:
            verify_engine(engine)
            print(f"Stockfish {VERSION} ja preparado: {engine}", flush=True)
            return engine
    # Cache de downloads permite recompilar e abrir outro target sem nova transferencia.
    cache = ROOT / ".cache" / "stockfish" / TAG
    cache.mkdir(parents=True, exist_ok=True)
    archive = cache / asset
    if not archive.is_file():
        print(f"Baixando Stockfish {VERSION} para {key} (~65-81 MB)...", flush=True)
        request = urllib.request.Request(BASE + asset, headers={"User-Agent": "xadrez-terminal-demo/1.0"})
        partial = archive.with_suffix(archive.suffix + ".part")
        try:
            with urllib.request.urlopen(request, timeout=90) as src, partial.open("wb") as dst:
                shutil.copyfileobj(src, dst)
            partial.replace(archive)
        finally:
            partial.unlink(missing_ok=True)
    with tempfile.TemporaryDirectory(prefix="stockfish-") as temporary:
        unpacked = Path(temporary)
        install_archive(archive, unpacked)
        candidates = [p for p in unpacked.rglob("stockfish-*")
                      if p.is_file() and (p.suffix == ".exe" if os.name == "nt" else p.suffix == "")]
        if len(candidates) != 1:
            raise RuntimeError(f"Executavel ambiguo ou ausente em {archive}. Apague o cache e tente novamente.")
        # Preserva licenca, fontes e redes NNUE do pacote oficial junto ao motor.
        source = candidates[0]
        shutil.copytree(source.parent, directory, dirs_exist_ok=True)
        copied = directory / source.name
        if copied != engine:
            copied.replace(engine)
        if os.name != "nt":
            engine.chmod(engine.stat().st_mode | 0o111)
        verify_engine(engine)
    marker.write_text(json.dumps({"platform": key, "version": VERSION,
                                 "url": BASE + asset}, indent=2), encoding="utf-8")
    print(f"Motor pronto: {engine}", flush=True)
    return engine


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, help="Pasta do executavel do jogo")
    parser.add_argument("--engine", help="Copiar um Stockfish local em vez de baixar")
    args = parser.parse_args()
    try:
        prepare(args.output, args.engine)
        return 0
    except (OSError, RuntimeError, ValueError, tarfile.TarError, zipfile.BadZipFile) as exc:
        print(f"ERRO: {exc}\nConfira a internet ou use --engine para um motor local.", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
