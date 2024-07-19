#!/usr/bin/env python3
"""
Check if the YAML files are well-formed and if the referenced executables exist.
"""

import pathlib
import platform
import sys

import yaml

if __name__ == "__main__":
    ostype = platform.system()
    if ostype == "Windows":
        modsuffix = ".exe"
    else:
        arch = platform.machine()
        modsuffix = f"_{ostype}_{arch}"
    top = pathlib.Path(sys.argv[0]).parent.parent
    for f in top.joinpath("YAML").glob("*.yaml"):
        print(f"checking {f.name}...")
        with f.open("r", encoding="utf-8") as fd:
            data = fd.read()
        d = yaml.safe_load(data)
        modname = next(iter(d.keys()))
        exe = top.joinpath("MODULES", modname + modsuffix)
        if not exe.is_file():
            raise FileNotFoundError(f"File {exe.name} referenced in {f.name} not found")
    for f in top.joinpath("YAML", "subwindow").glob("*.yaml"):
        print(f"checking subwindow/{f.name}...")
        with f.open("r", encoding="utf-8") as fd:
            data = fd.read()
        d = yaml.safe_load(data)
    print("OK")
