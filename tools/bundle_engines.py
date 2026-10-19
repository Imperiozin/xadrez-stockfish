#!/usr/bin/env python3
"""Manutencao: baixa os quatro pacotes oficiais para montar uma entrega offline."""
import hashlib
import json
from pathlib import Path
import shutil
import sys
import tempfile
import urllib.request

from setup_engine import ASSETS, BASE, ROOT, TAG, VERSION, install_archive


def main():
    cache = ROOT / ".cache" / "stockfish" / TAG
    cache.mkdir(parents=True, exist_ok=True)
    for key, asset in ASSETS.items():
        destination = ROOT / "vendor" / "stockfish" / key
        if (destination / "bundle.json").is_file():
            print(f"Incluido: {key}", flush=True)
            continue
        archive = cache / asset
        if not archive.is_file():
            print(f"Baixando {asset}...", flush=True)
            temporary = archive.with_suffix(archive.suffix + ".part")
            try:
                req = urllib.request.Request(BASE + asset, headers={"User-Agent": "xadrez-terminal-demo/1.0"})
                with urllib.request.urlopen(req, timeout=120) as src, temporary.open("wb") as dst:
                    shutil.copyfileobj(src, dst)
                temporary.replace(archive)
            finally:
                temporary.unlink(missing_ok=True)
        with tempfile.TemporaryDirectory() as temp:
            unpacked = Path(temp)
            install_archive(archive, unpacked)
            windows = key.startswith("windows")
            candidates = [p for p in unpacked.rglob("stockfish-*") if p.is_file()
                          and (p.suffix == ".exe" if windows else p.suffix == "")]
            if len(candidates) != 1:
                raise RuntimeError(f"Binario nao identificado em {asset}: {candidates}")
            source = candidates[0]
            shutil.copytree(source.parent, destination, dirs_exist_ok=True)
            name = "stockfish.exe" if windows else "stockfish"
            executable = destination / name
            (destination / source.name).replace(executable)
            if not windows:
                executable.chmod(0o755)
            manifest = {"platform": key, "version": VERSION, "url": BASE + asset,
                        "sha256": hashlib.sha256(executable.read_bytes()).hexdigest()}
            (destination / "bundle.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
            print(f"Pronto: {key} ({executable.stat().st_size:,} bytes)", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
