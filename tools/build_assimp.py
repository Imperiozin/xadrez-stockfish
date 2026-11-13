"""Build the pinned Assimp FBX/glTF importers with GCC/Clang, without CMake."""
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import tarfile
import urllib.request

from setup_engine import ROOT

VERSION = "5.4.3"
SHA256 = "66dfbaee288f2bc43172440a55d0235dfc7bf885dda6435c038e8000e79582cb"
URL = f"https://github.com/assimp/assimp/archive/refs/tags/v{VERSION}.tar.gz"


def source_tree():
    cache = ROOT / ".cache"
    cache.mkdir(exist_ok=True)
    source = cache / f"assimp-{VERSION}"
    marker = source / ".extracted"
    if marker.exists() and marker.read_text() == SHA256:
        return source
    archive = cache / f"assimp-{VERSION}.tar.gz"
    if not archive.exists():
        print(f"Baixando fontes do Assimp {VERSION}...", flush=True)
        temporary = archive.with_suffix(".download")
        try:
            with urllib.request.urlopen(URL, timeout=60) as response, temporary.open("wb") as output:
                shutil.copyfileobj(response, output)
            temporary.replace(archive)
        finally:
            temporary.unlink(missing_ok=True)
    if hashlib.sha256(archive.read_bytes()).hexdigest() != SHA256:
        archive.unlink()
        raise RuntimeError("Checksum do Assimp invalido; execute novamente para baixar outra copia")
    # Extract regular files explicitly, with no archive links or path traversal.
    with tarfile.open(archive, "r:gz") as package:
        for member in package:
            parts = Path(member.name).parts
            if not member.isfile() or not parts or parts[0] != f"assimp-{VERSION}":
                continue
            relative = Path(*parts[1:])
            target = (source / relative).resolve()
            if source.resolve() not in target.parents:
                raise RuntimeError("Caminho invalido no pacote Assimp")
            if relative.parts[0] not in ("code", "include", "contrib", "LICENSE"):
                continue
            target.parent.mkdir(parents=True, exist_ok=True)
            with package.extractfile(member) as input_file, target.open("wb") as output:
                shutil.copyfileobj(input_file, output)
    marker.write_text(SHA256)
    return source


def write_if_changed(path, contents):
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists() or path.read_text(encoding="utf-8") != contents:
        path.write_text(contents, encoding="utf-8")


def build_assimp(compiler, cc, build):
    source = source_tree()
    manifest = json.loads((ROOT / "tools/assimp_sources.json").read_text())
    if manifest["version"] != VERSION:
        raise RuntimeError("Manifesto de fontes do Assimp incompativel")
    generated = build / "include"
    config = (source / "include/assimp/config.h.in").read_text(encoding="utf-8")
    config = re.sub(r"^#cmakedefine .*", "/* Double precision disabled. */", config, flags=re.M)
    write_if_changed(generated / "assimp/config.h", config)
    revision = (source / "include/assimp/revision.h.in").read_text(encoding="utf-8")
    values = {"GIT_COMMIT_HASH": "0", "GIT_BRANCH": "release", "ASSIMP_VERSION_MAJOR": "5",
              "ASSIMP_VERSION_MINOR": "4", "ASSIMP_VERSION_PATCH": "3", "ASSIMP_PACKAGE_VERSION": "0",
              "CMAKE_SHARED_LIBRARY_PREFIX": "", "LIBRARY_SUFFIX": "", "CMAKE_DEBUG_POSTFIX": ""}
    for key, value in values.items():
        revision = revision.replace("@" + key + "@", value)
    write_if_changed(generated / "assimp/revision.h", revision)
    write_if_changed(generated / "zconf.h", (source / "contrib/zlib/zconf.h.included").read_text())
    includes = [generated] + [source / p for p in (
        "include", "code", ".", "contrib/zlib", "contrib/pugixml/src",
        "contrib/utf8cpp/source", "contrib/rapidjson/include", "contrib",
        "contrib/unzip", "contrib/openddlparser/include")]
    defines = manifest["defines"] + ["_LARGEFILE64_SOURCE=1"]
    flags = ["-O2", "-DNDEBUG", "-fno-strict-aliasing", "-pthread"]
    if compiler.lower().endswith(".exe"):
        flags += ["-Wa,-mbig-obj"]
    else:
        flags += ["-fPIC"]
    flags += ["-D" + d for d in defines] + ["-I" + str(p) for p in includes]
    zlib = ["adler32", "compress", "crc32", "deflate", "gzclose", "gzlib", "gzread",
            "gzwrite", "inflate", "infback", "inftrees", "inffast", "trees", "uncompr", "zutil"]
    sources = manifest["sources"] + [f"contrib/zlib/{name}.c" for name in zlib]
    # Header changes and compiler changes invalidate the cached objects.
    header_time = max(p.stat().st_mtime for base in (source, generated)
                      for p in base.rglob("*") if p.suffix in (".h", ".hpp", ".inl"))
    versions = {tool: subprocess.check_output([tool, "--version"]).decode(errors="replace")
                for tool in (compiler, cc)}

    def compile_source(relative):
        src = source / relative
        obj = build / "obj" / (relative + ".o")
        obj.parent.mkdir(parents=True, exist_ok=True)
        tool = cc if src.suffix == ".c" else compiler
        command = [tool, "-std=c99" if src.suffix == ".c" else "-std=c++17"] + flags
        command += ["-c", str(src), "-o", str(obj)]
        stamp = obj.with_suffix(".flags")
        signature = json.dumps([command, versions[tool]])
        if (not obj.exists() or not stamp.exists() or stamp.read_text() != signature or
                obj.stat().st_mtime < max(src.stat().st_mtime, header_time)):
            print("Assimp: " + relative, flush=True)
            subprocess.run(command, check=True)
            stamp.write_text(signature)
        return obj

    with ThreadPoolExecutor(max_workers=2) as pool:
        objects = list(pool.map(compile_source, sources))
    library = build / "libassimp.a"
    archive_stamp = build / "archive.flags"
    signature = json.dumps([str(p) for p in objects])
    if (not library.exists() or not archive_stamp.exists() or archive_stamp.read_text() != signature or
            any(p.stat().st_mtime > library.stat().st_mtime for p in objects)):
        ar = subprocess.check_output([compiler, "-print-prog-name=ar"], text=True).strip()
        library.unlink(missing_ok=True)
        # Small batches avoid Windows command-line limits and work with BSD ar
        # on macOS, which does not use GNU ar's response-file syntax.
        for start in range(0, len(objects), 16):
            subprocess.run([ar, "rcs", str(library)] + [str(p) for p in objects[start:start + 16]], check=True)
        archive_stamp.write_text(signature)
    return [generated, source / "include"], library
