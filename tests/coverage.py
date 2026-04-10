#!/usr/bin/env python3
"""
Check for untested modules
"""

import platform
import re
from pathlib import Path


def strip_suffix(mod):
    return mod


if __name__ == "__main__":
    # load exclude lists
    test_dir = Path(__file__).parent
    ns = dict()
    exec(test_dir.joinpath("properties.py").open().read(), ns)
    DEPRECATED = ns.get("DEPRECATED", [])
    TOOLS = ns.get("TOOLS", [])

    ostype = platform.system()
    if ostype == "Windows":
        modsuffix = ".exe"
    else:
        arch = platform.machine()
        modsuffix = f"_{ostype}_{arch}"
    all_modules = (
        set(
            m.name[: -len(modsuffix)]
            for m in test_dir.parent.joinpath("MODULES").glob("*" + modsuffix)
        )
        .difference(TOOLS)
        .difference(DEPRECATED)
    )

    RE_MOD = re.compile(r'^"?\$\{?V\}?"?/([\w\.]*)')
    test_modules = set()
    for subdir in test_dir.joinpath("module_tests").glob("*"):
        if not subdir.is_dir():
            continue
        for f in subdir.glob("*.sh"):
            for line in f.open("r"):
                m = RE_MOD.match(line)
                if m:
                    test_modules.add(strip_suffix(m.group(1)))

    deprecated = test_modules.intersection(DEPRECATED)
    if deprecated:
        print("Deprecated modules in tests:")
        print(deprecated)

    untested = all_modules.difference(test_modules)
    if untested:
        print("Untested modules:")
        print(untested)
        exit(1)
