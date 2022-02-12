#!/usr/bin/env python
"""Create a WASM asset bundle directory structure.

The WASM asset bundles are pre-loaded by the final WASM build. The bundle
contains:

- a stripped down, pyc-only stdlib zip file, e.g. {PREFIX}/lib/python311.zip
- os.py as marker module {PREFIX}/lib/python3.11/os.py
- empty lib-dynload directory, to make sure it is copied into the bundle {PREFIX}/lib/python3.11/lib-dynload/.empty
"""

import argparse
import pathlib
import shutil
import sys
import zipfile

# source directory
SRCDIR = pathlib.Path(__file__).parent.parent.parent.absolute()
SRCDIR_LIB = SRCDIR / "Lib"

# sysconfig data relative to build dir.
SYSCONFIGDATA_GLOB = "build/lib.*/_sysconfigdata_*.py"

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
    # user interfaces: TK, curses
    "curses/",
    "idlelib/",
    "tkinter/",
    "turtle.py",
    "turtledemo/",
    # package management
    "ensurepip/",
    "venv/",
    # build system
    "distutils/",
    "lib2to3/",
    # concurrency
    "concurrent/",
    "multiprocessing/",
    # deprecated
    "asyncore.py",
    "asynchat.py",
    # Synchronous network I/O and protocols are not supported; for example,
    # socket.create_connection() raises an exception:
    # "BlockingIOError: [Errno 26] Operation in progress".
    "cgi.py",
    "cgitb.py",
    "email/",
    "ftplib.py",
    "http/",
    "imaplib.py",
    "nntplib.py",
    "poplib.py",
    "smtpd.py",
    "smtplib.py",
    "socketserver.py",
    "telnetlib.py",
    # keep urllib.parse for pydoc
    "urllib/error.py",
    "urllib/request.py",
    "urllib/response.py",
    "urllib/robotparser.py",
    "wsgiref/",
    "xmlrpc/",
    # dbm / gdbm
    "dbm/",
    # other platforms
    "_aix_support.py",
    "_bootsubprocess.py",
    "_osx_support.py",
    # webbrowser
    "antigravity.py",
    "webbrowser.py",
    # ctypes
    "ctypes/",
    # Pure Python implementations of C extensions
    "_pydecimal.py",
    "_pyio.py",
    # Misc unused or large files
    "pydoc_data/",
    "msilib/",
)

# regression test sub directories
OMIT_SUBDIRS = (
    "ctypes/test/",
    "tkinter/test/",
    "unittest/test/",
)


OMIT_ABSOLUTE = {SRCDIR_LIB / name for name in OMIT_FILES}
OMIT_SUBDIRS_ABSOLUTE = tuple(str(SRCDIR_LIB / name) for name in OMIT_SUBDIRS)


def filterfunc(name: str) -> bool:
    return not name.startswith(OMIT_SUBDIRS_ABSOLUTE)


def create_stdlib_zip(
    args: argparse.Namespace, compression: int = zipfile.ZIP_DEFLATED, *, optimize: int = 0
) -> None:
    sysconfig_data = list(args.builddir.glob(SYSCONFIGDATA_GLOB))
    if not sysconfig_data:
        raise ValueError("No sysconfigdata file found")

    with zipfile.PyZipFile(
        args.wasm_stdlib_zip, mode="w", compression=compression, optimize=0
    ) as pzf:
        for entry in sorted(args.srcdir_lib.iterdir()):
            if entry.name == "__pycache__":
                continue
            if entry in OMIT_ABSOLUTE:
                continue
            if entry.name.endswith(".py") or entry.is_dir():
                # writepy() writes .pyc files (bytecode).
                pzf.writepy(entry, filterfunc=filterfunc)
        for entry in sysconfig_data:
            pzf.writepy(entry)


def path(val: str) -> pathlib.Path:
    return pathlib.Path(val).absolute()


parser = argparse.ArgumentParser()
parser.add_argument(
    "--builddir",
    help="absolute build directory",
    default=pathlib.Path(".").absolute(),
    type=path,
)
parser.add_argument(
    "--prefix", help="install prefix", default=pathlib.Path("/usr/local"), type=path
)


def main():
    args = parser.parse_args()

    relative_prefix = args.prefix.relative_to(pathlib.Path("/"))
    args.srcdir = SRCDIR
    args.srcdir_lib = SRCDIR_LIB
    args.wasm_root = args.builddir / relative_prefix
    args.wasm_stdlib_zip = args.wasm_root / WASM_STDLIB_ZIP
    args.wasm_stdlib = args.wasm_root / WASM_STDLIB
    args.wasm_dynload = args.wasm_root / WASM_DYNLOAD

    # Empty, unused directory for dynamic libs, but required for site initialization.
    args.wasm_dynload.mkdir(parents=True, exist_ok=True)
    marker = args.wasm_dynload / ".empty"
    marker.touch()
    # os.py is a marker for finding the correct lib directory.
    shutil.copy(args.srcdir_lib / "os.py", args.wasm_stdlib)
    # The rest of stdlib that's useful in a WASM context.
    create_stdlib_zip(args)
    size = round(args.wasm_stdlib_zip.stat().st_size / 1024 ** 2, 2)
    parser.exit(0, f"Created {args.wasm_stdlib_zip} ({size} MiB)\n")


if __name__ == "__main__":
    main()
