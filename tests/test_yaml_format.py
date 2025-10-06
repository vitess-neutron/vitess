#!/usr/bin/env python3
"""
Check if the YAML files are well-formed and if the referenced executables exist.
"""

import pathlib
import platform
import sys

import yaml

# exclude lists
SPECIAL_EXECUTABLES = [
    "merge_spectra",
    "gener_pipe",
    "visual",
    "gener_batch",
    "guide_shape",
    "fom",
    "lattice_dist",
    "cas_v40",
    "gener_hkl",
    "direct_view",
    "mirror_coating",
    "standard_deviation",
    "chop_phases",
    "opt_sim",
    "gener_bispectral",
    "ascii2bin",
    "surface_file",
    "sortiap",
    "define_direction",
]
OLD_MODULE_NAMES = []

if __name__ == "__main__":
    ostype = platform.system()
    if ostype == "Windows":
        modsuffix = ".exe"
    else:
        arch = platform.machine()
        modsuffix = f"_{ostype}_{arch}"
    top = pathlib.Path(sys.argv[0]).absolute().parent.parent

    yaml_modules = set()
    no_category = list()
    missing_hidden = list()
    for f in top.joinpath("YAML").glob("*.yaml"):
        print(f"checking {f.name}...")
        with f.open("r", encoding="utf-8") as fd:
            data = fd.read()
        d = yaml.safe_load(data)
        exe, contents = next(iter(d.items()))
        yaml_modules.add(exe)
        basename = f.stem
        if "category" not in contents[0]:
            no_category.append(basename)
        if exe != basename:
            hidden = False
            for param in contents:
                props = next(iter(param.values()))
                if isinstance(props, dict) and props.get("hidden", False):
                    hidden = True
            if not hidden:
                missing_hidden.append(basename)

    for f in top.joinpath("YAML", "subwindow").glob("*.yaml"):
        print(f"checking subwindow/{f.name}...")
        with f.open("r", encoding="utf-8") as fd:
            data = fd.read()
        d = yaml.safe_load(data)

    executables = (
        set(
            m.name[: -len(modsuffix)]
            for m in top.joinpath("MODULES").glob("*" + modsuffix)
        )
        .difference(SPECIAL_EXECUTABLES)
        .difference(OLD_MODULE_NAMES)
    )
    missing_executables = yaml_modules.difference(executables)
    missing_yaml = executables.difference(yaml_modules)

    if "-v" in sys.argv[1:]:
        print("no category:")
        print(sorted(no_category))
        print()
        print("missing hidden parameter:")
        print(sorted(missing_hidden))
        print()

    code = 0
    if missing_executables:
        code = 1
        print("Error: the following modules referenced in yaml have no executable:")
        print(missing_executables)
    if missing_yaml:
        code = 1
        print("Error: the following executables are not referenced in any yaml file:")
        print(missing_yaml)
    sys.exit(code)
