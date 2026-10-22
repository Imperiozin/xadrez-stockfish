#!/usr/bin/env python3
"""Compila a unica aplicacao Xadrez 3D com GLFW local, sem CMake."""
import argparse
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
from setup_engine import ROOT, prepare

COMMON = "context init input monitor platform vulkan window egl_context osmesa_context null_init null_monitor null_window null_joystick".split()
BACKENDS = {
    "Windows": ("_GLFW_WIN32", "win32_init win32_joystick win32_monitor win32_window win32_time win32_thread win32_module wgl_context".split(), ["-lgdi32"]),
    "Linux": ("_GLFW_X11", "x11_init x11_monitor x11_window xkb_unicode glx_context posix_time posix_thread posix_module posix_poll linux_joystick".split(), ["-ldl", "-lm"]),
    "Darwin": ("_GLFW_COCOA", "cocoa_init cocoa_joystick cocoa_monitor cocoa_window nsgl_context macos_time posix_thread posix_module".split(), ["-framework", "Cocoa", "-framework", "IOKit", "-framework", "QuartzCore"]),
}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run",action="store_true")
    parser.add_argument("--engine")
    parser.add_argument("--output")
    parser.add_argument("--compiler",default=os.environ.get("CXX","g++" if os.name=="nt" else "c++"))
    parser.add_argument("--cc",default=os.environ.get("CC"),help="Compilador C para GLFW")
    args=parser.parse_args()
    try:
        system=platform.system()
        if system not in BACKENDS: raise RuntimeError("Plataforma nao suportada: "+system)
        compiler=shutil.which(args.compiler)
        if compiler is None and os.name=="nt": compiler=shutil.which("C:/MinGW/bin/g++.exe")
        if compiler is None: raise RuntimeError("Compilador C++ nao encontrado: "+args.compiler)
        os.environ["PATH"]=str(Path(compiler).parent)+os.pathsep+os.environ.get("PATH","")
        cc=shutil.which(args.cc or ("gcc" if system=="Windows" else "cc"))
        if cc is None: raise RuntimeError("Compilador C nao encontrado; informe --cc")
        output=Path(args.output).resolve() if args.output else ROOT/"build"/system.lower()/"bin"
        output.mkdir(parents=True,exist_ok=True)
        shutil.copytree(ROOT/"shaders",output/"shaders",dirs_exist_ok=True)
        prepare(output,args.engine)
        objects=output.parent/"obj"/"glfw"
        objects.mkdir(parents=True,exist_ok=True)
        define,sources,libraries=BACKENDS[system]
        headers=list((ROOT/"vendor/glfw/src").glob("*.h"))+list((ROOT/"vendor/glfw/include/GLFW").glob("*.h"))
        header_time=max(p.stat().st_mtime for p in headers)
        object_paths=[]
        for name in COMMON+sources:
            extension=".m" if system=="Darwin" and name.startswith(("cocoa_","nsgl_")) else ".c"
            source=ROOT/"vendor/glfw/src"/(name+extension)
            obj=objects/(name+".o")
            # Compiler/backend are recorded so a toolchain change invalidates the cache.
            stamp=objects/(name+".flags")
            flags=repr((cc,define,system))
            if not obj.exists() or not stamp.exists() or stamp.read_text()!=flags or obj.stat().st_mtime<max(source.stat().st_mtime,header_time):
                subprocess.run([cc,"-std=c99","-O2","-pthread","-D"+define,"-c",str(source),"-o",str(obj)],check=True)
                stamp.write_text(flags)
            object_paths.append(str(obj))
        executable=output/("xadrez.exe" if os.name=="nt" else "xadrez")
        sources=[ROOT/"src"/name for name in ("main.cpp","jogo.cpp","stockfish.cpp")]
        sources+=sorted((ROOT/"src/graphics").glob("*.cpp"))+sorted((ROOT/"src/pieces").glob("*.cpp"))
        command=[compiler,"-std=c++17","-O2","-Wall","-Wextra","-Wpedantic","-pthread"]
        command += ["-I"+str(ROOT/p) for p in ("include","src","vendor/chess-library","vendor/glfw/include","vendor/glfw/deps")]
        command += [str(p) for p in sources]+object_paths+libraries
        if system=="Windows":command.append("-static")
        command += ["-o",str(executable)]
        print("Compilando Xadrez 3D...",flush=True)
        subprocess.run(command,check=True)
        print("Build concluido:",executable,flush=True)
        if args.run:return subprocess.call([str(executable)])
        return 0
    except (OSError,RuntimeError,subprocess.CalledProcessError) as exc:
        print("ERRO:",exc,file=sys.stderr)
        return 1

if __name__=="__main__":sys.exit(main())
