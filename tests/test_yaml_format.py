#!/usr/bin/env python3
"""
Check if the YAML files are well-formed and if the referenced executables exist.
"""

import platform
import sys
from pathlib import Path

import yaml

# exclude lists
SPECIAL_EXECUTABLES = [
    "ascii2bin",
    "cas_v40",
    "chop_phases",
    "define_direction",
    "direct_view",
    "dist_time",
    "fom",
    "gener_batch",
    "gener_bispectral",
    "gener_hkl",
    "gener_pipe",
    "guide_shape",
    "lattice_dist",
    "merge",
    "merge_spectra",
    "mirror_coating",
    "opt_sim",
    "sortiap",
    "standard_deviation",
    "surface_file",
]
OLD_MODULE_NAMES = []


def yaml_dir(top):
    vers = []
    for f in Path(top).joinpath("yaml").glob("*"):
        if (
            f.is_dir()
            and "." in f.name
            and f.joinpath("enums.yaml").is_file()
            and f.joinpath("modules").is_dir()
        ):
            vers.append(f)
    vers.sort(key=lambda d: tuple(int(x) for x in d.name.split(".", 1)))
    return vers[-1]


if __name__ == "__main__":
    ostype = platform.system()
    if ostype == "Windows":
        modsuffix = ".exe"
    else:
        arch = platform.machine()
        modsuffix = f"_{ostype}_{arch}"
    top = Path(__file__).absolute().parent.parent
    yamldir = yaml_dir(top)

    defs = yamldir.joinpath("enums.yaml").open().read()
    print("checking enums.yaml...")
    yaml.safe_load(defs)  # test format
    for f in yamldir.joinpath("definitions").glob("*.yaml"):
        print(f"checking {f.name}...")
        data = f.open().read()
        yaml.safe_load(defs + data)  # test format
        defs += data

    yaml_modules = set()
    no_category = list()
    missing_hidden = list()
    for f in yamldir.joinpath("modules").glob("*.yaml"):
        print(f"checking {f.name}...")
        with f.open("r", encoding="utf-8") as fd:
            data = fd.read()
        data = yaml.safe_load(defs + data)
        basename = f.stem
        contents = data[basename]
        exe = contents["executable"]
        yaml_modules.add(exe)
        if "category" not in contents:
            no_category.append(basename)
        if exe != basename:
            hidden = False
            for param in contents["parameters"]:
                hidden = hidden or param.get("hidden", False)
            if not hidden:
                missing_hidden.append(basename)

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
