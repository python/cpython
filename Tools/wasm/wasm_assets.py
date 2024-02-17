#!/usr/bin/env python
"""Create a WASM asset bundle directory structure.

The WASM asset bundles are pre-loaded by the final WASM build. The bundle
contains:

- a stripped down, pyc-only stdlib zip file, e.g. {PREFIX}/lib/python311.zip
- os.py as marker module {PREFIX}/lib/python3.11/os.py
- empty lib-dynload directory, to make sure it is copied into the bundle:
    {PREFIX}/lib/python3.11/lib-dynload/.empty
"""

import argparse
import pathlib
import shutil
import sys
import sysconfig
import zipfile
from typing import Dict

# source directory
SRCDIR = pathlib.Path(__file__).parent.parent.parent.absolute()
SRCDIR_LIB = SRCDIR / "Lib"


# Library directory relative to $(prefix).
WASM_LIB = pathlib.PurePath("lib")
WASM_STDLIB_ZIP = (
    WASM_LIB / f"python{sys.version_info.major}{sys.version_info.minor}.zip"
)
WASM_STDLIB = (
    WASM_LIB / f"python{sys.version_info.major}.{sys.version_info.minor}"
)
WASM_DYNLOAD = WASM_STDLIB / "lib-dynload"


# Don't ship large files / packages that are not particularly useful at
# the moment.
OMIT_FILES = (
    # regression tests
    "test/",
    # package management
    "ensurepip/",
    "venv/",
    # other platforms
    "_aix_support.py",
    "_osx_support.py",
    # webbrowser
    "antigravity.py",
    "webbrowser.py",
    # Pure Python implementations of C extensions
    "_pydecimal.py",
    "_pyio.py",
    # concurrent threading
    "concurrent/futures/thread.py",
    # Misc unused or large files
    "pydoc_data/",
)

# Synchronous network I/O and protocols are not supported; for example,
# socket.create_connection() raises an exception:
# "BlockingIOError: [Errno 26] Operation in progress".
OMIT_NETWORKING_FILES = (
    "email/",
    "ftplib.py",
    "http/",
    "imaplib.py",
    "mailbox.py",
    "poplib.py",
    "smtplib.py",
    "socketserver.py",
    # keep urllib.parse for pydoc
    "urllib/error.py",
    "urllib/request.py",
    "urllib/response.py",
    "urllib/robotparser.py",
    "wsgiref/",
)

OMIT_MODULE_FILES = {
    "_asyncio": ["asyncio/"],
    "_curses": ["curses/"],
    "_ctypes": ["ctypes/"],
    "_decimal": ["decimal.py"],
    "_dbm": ["dbm/ndbm.py"],
    "_gdbm": ["dbm/gnu.py"],
    "_json": ["json/"],
    "_multiprocessing": ["concurrent/futures/process.py", "multiprocessing/"],
    "pyexpat": ["xml/", "xmlrpc/"],
    "readline": ["rlcompleter.py"],
    "_sqlite3": ["sqlite3/"],
    "_ssl": ["ssl.py"],
    "_tkinter": ["idlelib/", "tkinter/", "turtle.py", "turtledemo/"],
    "_zoneinfo": ["zoneinfo/"],
}

SYSCONFIG_NAMES = (
    "_sysconfigdata__emscripten_wasm32-emscripten",
    "_sysconfigdata__emscripten_wasm32-emscripten",
    "_sysconfigdata__wasi_wasm32-wasi",
    "_sysconfigdata__wasi_wasm64-wasi",
)


def get_builddir(args: argparse.Namespace) -> pathlib.Path:
    """Get builddir path from pybuilddir.txt"""
    with open("pybuilddir.txt", encoding="utf-8") as f:
        builddir = f.read()
    return pathlib.Path(builddir)


def get_sysconfigdata(args: argparse.Namespace) -> pathlib.Path:
    """Get path to sysconfigdata relative to build root"""
    assert isinstance(args.builddir, pathlib.Path)
    data_name: str = sysconfig._get_sysconfigdata_name()  # type: ignore[attr-defined]
    if not data_name.startswith(SYSCONFIG_NAMES):
        raise ValueError(
            f"Invalid sysconfig data name '{data_name}'.", SYSCONFIG_NAMES
        )
    filename = data_name + ".py"
    return args.builddir / filename


def create_stdlib_zip(
    args: argparse.Namespace,
    *,
    optimize: int = 0,
) -> None:
    def filterfunc(filename: str) -> bool:
        pathname = pathlib.Path(filename).resolve()
        return pathname not in args.omit_files_absolute

    with zipfile.PyZipFile(
        args.wasm_stdlib_zip,
        mode="w",
        compression=args.compression,
        optimize=optimize,
    ) as pzf:
        if args.compresslevel is not None:
            pzf.compresslevel = args.compresslevel
        pzf.writepy(args.sysconfig_data)
        for entry in sorted(args.srcdir_lib.iterdir()):
            entry = entry.resolve()
            if entry.name == "__pycache__":
                continue
            if entry.name.endswith(".py") or entry.is_dir():
                # writepy() writes .pyc files (bytecode).
                pzf.writepy(entry, filterfunc=filterfunc)


def detect_extension_modules(args: argparse.Namespace) -> Dict[str, bool]:
    modules = {}

    # disabled by Modules/Setup.local ?
    with open(args.buildroot / "Makefile") as f:
        for line in f:
            if line.startswith("MODDISABLED_NAMES="):
                disabled = line.split("=", 1)[1].strip().split()
                for modname in disabled:
                    modules[modname] = False
                break

    # disabled by configure?
    with open(args.sysconfig_data) as f:
        data = f.read()
    loc: Dict[str, Dict[str, str]] = {}
    exec(data, globals(), loc)

    for key, value in loc["build_time_vars"].items():
        if not key.startswith("MODULE_") or not key.endswith("_STATE"):
            continue
        if value not in {"yes", "disabled", "missing", "n/a"}:
            raise ValueError(f"Unsupported value '{value}' for {key}")

        modname = key[7:-6].lower()
        if modname not in modules:
            modules[modname] = value == "yes"
    return modules


def path(val: str) -> pathlib.Path:
    return pathlib.Path(val).absolute()


parser = argparse.ArgumentParser()
parser.add_argument(
    "--buildroot",
    help="absolute path to build root",
    default=pathlib.Path(".").absolute(),
    type=path,
)
parser.add_argument(
    "--prefix",
    help="install prefix",
    default=pathlib.Path("/usr/local"),
    type=path,
)


def main() -> None:
    args = parser.parse_args()

    relative_prefix = args.prefix.relative_to(pathlib.Path("/"))
    args.srcdir = SRCDIR
    args.srcdir_lib = SRCDIR_LIB
    args.wasm_root = args.buildroot / relative_prefix
    args.wasm_stdlib_zip = args.wasm_root / WASM_STDLIB_ZIP
    args.wasm_stdlib = args.wasm_root / WASM_STDLIB
    args.wasm_dynload = args.wasm_root / WASM_DYNLOAD

    # bpo-17004: zipimport supports only zlib compression.
    # Emscripten ZIP_STORED + -sLZ4=1 linker flags results in larger file.
    args.compression = zipfile.ZIP_DEFLATED
    args.compresslevel = 9

    args.builddir = get_builddir(args)
    args.sysconfig_data = get_sysconfigdata(args)
    if not args.sysconfig_data.is_file():
        raise ValueError(f"sysconfigdata file {args.sysconfig_data} missing.")

    extmods = detect_extension_modules(args)
    omit_files = list(OMIT_FILES)
    if sysconfig.get_platform().startswith("emscripten"):
        omit_files.extend(OMIT_NETWORKING_FILES)
    for modname, modfiles in OMIT_MODULE_FILES.items():
        if not extmods.get(modname):
            omit_files.extend(modfiles)

    args.omit_files_absolute = {
        (args.srcdir_lib / name).resolve() for name in omit_files
    }

    # Empty, unused directory for dynamic libs, but required for site initialization.
    args.wasm_dynload.mkdir(parents=True, exist_ok=True)
    marker = args.wasm_dynload / ".empty"
    marker.touch()
    # os.py is a marker for finding the correct lib directory.
    shutil.copy(args.srcdir_lib / "os.py", args.wasm_stdlib)
    # The rest of stdlib that's useful in a WASM context.
    create_stdlib_zip(args)
    size = round(args.wasm_stdlib_zip.stat().st_size / 1024**2, 2)
    parser.exit(0, f"Created {args.wasm_stdlib_zip} ({size} MiB)\n")


if __name__ == "__main__":
    main()
