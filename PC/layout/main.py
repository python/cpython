"""
Generates a layout of Python for Windows from a build.

See python make_layout.py --help for usage.
"""

__author__ = "Steve Dower <steve.dower@python.org>"
__version__ = "3.8"

import argparse
import functools
import os
import re
import shutil
import subprocess
import sys
import tempfile
import zipfile

from pathlib import Path

if __name__ == "__main__":
    # Started directly, so enable relative imports
    __path__ = [str(Path(__file__).resolve().parent)]

from .support.appxmanifest import *
from .support.catalog import *
from .support.constants import *
from .support.filesets import *
from .support.logging import *
from .support.options import *
from .support.pip import *
from .support.props import *
from .support.nuspec import *

BDIST_WININST_FILES_ONLY = FileNameSet("wininst-*", "bdist_wininst.py")
BDIST_WININST_STUB = "PC/layout/support/distutils.command.bdist_wininst.py"

TEST_PYDS_ONLY = FileStemSet("xxlimited", "_ctypes_test", "_test*")
TEST_DIRS_ONLY = FileNameSet("test", "tests")

IDLE_DIRS_ONLY = FileNameSet("idlelib")

TCLTK_PYDS_ONLY = FileStemSet("tcl*", "tk*", "_tkinter")
TCLTK_DIRS_ONLY = FileNameSet("tkinter", "turtledemo")
TCLTK_FILES_ONLY = FileNameSet("turtle.py")

VENV_DIRS_ONLY = FileNameSet("venv", "ensurepip")

EXCLUDE_FROM_PYDS = FileStemSet("python*", "pyshellext", "vcruntime*")
EXCLUDE_FROM_LIB = FileNameSet("*.pyc", "__pycache__", "*.pickle")
EXCLUDE_FROM_PACKAGED_LIB = FileNameSet("readme.txt")
EXCLUDE_FROM_COMPILE = FileNameSet("badsyntax_*", "bad_*")
EXCLUDE_FROM_CATALOG = FileSuffixSet(".exe", ".pyd", ".dll")

REQUIRED_DLLS = FileStemSet("libcrypto*", "libssl*", "libffi*")

LIB2TO3_GRAMMAR_FILES = FileNameSet("Grammar.txt", "PatternGrammar.txt")

PY_FILES = FileSuffixSet(".py")
PYC_FILES = FileSuffixSet(".pyc")
CAT_FILES = FileSuffixSet(".cat")
CDF_FILES = FileSuffixSet(".cdf")

DATA_DIRS = FileNameSet("data")

TOOLS_DIRS = FileNameSet("scripts", "i18n", "pynche", "demo", "parser")
TOOLS_FILES = FileSuffixSet(".py", ".pyw", ".txt")


def copy_if_modified(src, dest):
    try:
        dest_stat = os.stat(dest)
    except FileNotFoundError:
        do_copy = True
    else:
        src_stat = os.stat(src)
        do_copy = (
            src_stat.st_mtime != dest_stat.st_mtime
            or src_stat.st_size != dest_stat.st_size
        )

    if do_copy:
        shutil.copy2(src, dest)


def get_lib_layout(ns):
    def _c(f):
        if f in EXCLUDE_FROM_LIB:
            return False
        if f.is_dir():
            if f in TEST_DIRS_ONLY:
                return ns.include_tests
            if f in TCLTK_DIRS_ONLY:
                return ns.include_tcltk
            if f in IDLE_DIRS_ONLY:
                return ns.include_idle
            if f in VENV_DIRS_ONLY:
                return ns.include_venv
        else:
            if f in TCLTK_FILES_ONLY:
                return ns.include_tcltk
            if f in BDIST_WININST_FILES_ONLY:
                return ns.include_bdist_wininst
        return True

    for dest, src in rglob(ns.source / "Lib", "**/*", _c):
        yield dest, src

    if not ns.include_bdist_wininst:
        src = ns.source / BDIST_WININST_STUB
        yield Path("distutils/command/bdist_wininst.py"), src


def get_tcltk_lib(ns):
    if not ns.include_tcltk:
        return

    tcl_lib = os.getenv("TCL_LIBRARY")
    if not tcl_lib or not os.path.isdir(tcl_lib):
        try:
            with open(ns.build / "TCL_LIBRARY.env", "r", encoding="utf-8-sig") as f:
                tcl_lib = f.read().strip()
        except FileNotFoundError:
            pass
        if not tcl_lib or not os.path.isdir(tcl_lib):
            log_warning("Failed to find TCL_LIBRARY")
            return

    for dest, src in rglob(Path(tcl_lib).parent, "**/*"):
        yield "tcl/{}".format(dest), src


def get_layout(ns):
    def in_build(f, dest="", new_name=None):
        n, _, x = f.rpartition(".")
        n = new_name or n
        src = ns.build / f
        if ns.debug and src not in REQUIRED_DLLS:
            if not src.stem.endswith("_d"):
                src = src.parent / (src.stem + "_d" + src.suffix)
            if not n.endswith("_d"):
                n += "_d"
                f = n + "." + x
        yield dest + n + "." + x, src
        if ns.include_symbols:
            pdb = src.with_suffix(".pdb")
            if pdb.is_file():
                yield dest + n + ".pdb", pdb
        if ns.include_dev:
            lib = src.with_suffix(".lib")
            if lib.is_file():
                yield "libs/" + n + ".lib", lib

    if ns.include_appxmanifest:
        yield from in_build("python_uwp.exe", new_name="python{}".format(VER_DOT))
        yield from in_build("pythonw_uwp.exe", new_name="pythonw{}".format(VER_DOT))
        # For backwards compatibility, but we don't reference these ourselves.
        yield from in_build("python_uwp.exe", new_name="python")
        yield from in_build("pythonw_uwp.exe", new_name="pythonw")
    else:
        yield from in_build("python.exe", new_name="python")
        yield from in_build("pythonw.exe", new_name="pythonw")

    yield from in_build(PYTHON_DLL_NAME)

    if ns.include_launchers and ns.include_appxmanifest:
        if ns.include_pip:
            yield from in_build("python_uwp.exe", new_name="pip{}".format(VER_DOT))
        if ns.include_idle:
            yield from in_build("pythonw_uwp.exe", new_name="idle{}".format(VER_DOT))

    if ns.include_stable:
        yield from in_build(PYTHON_STABLE_DLL_NAME)

    for dest, src in rglob(ns.build, "vcruntime*.dll"):
        yield dest, src

    yield "LICENSE.txt", ns.build / "LICENSE.txt"

    for dest, src in rglob(ns.build, ("*.pyd", "*.dll")):
        if src.stem.endswith("_d") != bool(ns.debug) and src not in REQUIRED_DLLS:
            continue
        if src in EXCLUDE_FROM_PYDS:
            continue
        if src in TEST_PYDS_ONLY and not ns.include_tests:
            continue
        if src in TCLTK_PYDS_ONLY and not ns.include_tcltk:
            continue

        yield from in_build(src.name, dest="" if ns.flat_dlls else "DLLs/")

    if ns.zip_lib:
        zip_name = PYTHON_ZIP_NAME
        yield zip_name, ns.temp / zip_name
    else:
        for dest, src in get_lib_layout(ns):
            yield "Lib/{}".format(dest), src

        if ns.include_venv:
            yield from in_build("venvlauncher.exe", "Lib/venv/scripts/nt/", "python")
            yield from in_build("venvwlauncher.exe", "Lib/venv/scripts/nt/", "pythonw")

    if ns.include_tools:

        def _c(d):
            if d.is_dir():
                return d in TOOLS_DIRS
            return d in TOOLS_FILES

        for dest, src in rglob(ns.source / "Tools", "**/*", _c):
            yield "Tools/{}".format(dest), src

    if ns.include_underpth:
        yield PYTHON_PTH_NAME, ns.temp / PYTHON_PTH_NAME

    if ns.include_dev:

        for dest, src in rglob(ns.source / "Include", "**/*.h"):
            yield "include/{}".format(dest), src
        src = ns.source / "PC" / "pyconfig.h"
        yield "include/pyconfig.h", src

    for dest, src in get_tcltk_lib(ns):
        yield dest, src

    if ns.include_pip:
        for dest, src in get_pip_layout(ns):
            if not isinstance(src, tuple) and (
                src in EXCLUDE_FROM_LIB or src in EXCLUDE_FROM_PACKAGED_LIB
            ):
                continue
            yield dest, src

    if ns.include_chm:
        for dest, src in rglob(ns.doc_build / "htmlhelp", PYTHON_CHM_NAME):
            yield "Doc/{}".format(dest), src

    if ns.include_html_doc:
        for dest, src in rglob(ns.doc_build / "html", "**/*"):
            yield "Doc/html/{}".format(dest), src

    if ns.include_props:
        for dest, src in get_props_layout(ns):
            yield dest, src

    if ns.include_nuspec:
        for dest, src in get_nuspec_layout(ns):
            yield dest, src

    for dest, src in get_appx_layout(ns):
        yield dest, src

    if ns.include_cat:
        if ns.flat_dlls:
            yield ns.include_cat.name, ns.include_cat
        else:
            yield "DLLs/{}".format(ns.include_cat.name), ns.include_cat


def _compile_one_py(src, dest, name, optimize, checked=True):
    import py_compile

    if dest is not None:
        dest = str(dest)

    mode = (
        py_compile.PycInvalidationMode.CHECKED_HASH
        if checked
        else py_compile.PycInvalidationMode.UNCHECKED_HASH
    )

    try:
        return Path(
            py_compile.compile(
                str(src),
                dest,
                str(name),
                doraise=True,
                optimize=optimize,
                invalidation_mode=mode,
            )
        )
    except py_compile.PyCompileError:
        log_warning("Failed to compile {}", src)
        return None


# name argument added to address bpo-37641
def _py_temp_compile(src, name, ns, dest_dir=None, checked=True):
    if not ns.precompile or src not in PY_FILES or src.parent in DATA_DIRS:
        return None
    dest = (dest_dir or ns.temp) / (src.stem + ".pyc")
    return _compile_one_py(src, dest, name, optimize=2, checked=checked)


def _write_to_zip(zf, dest, src, ns, checked=True):
    pyc = _py_temp_compile(src, dest, ns, checked=checked)
    if pyc:
        try:
            zf.write(str(pyc), dest.with_suffix(".pyc"))
        finally:
            try:
                pyc.unlink()
            except:
                log_exception("Failed to delete {}", pyc)
        return

    if src in LIB2TO3_GRAMMAR_FILES:
        from lib2to3.pgen2.driver import load_grammar

        tmp = ns.temp / src.name
        try:
            shutil.copy(src, tmp)
            load_grammar(str(tmp))
            for f in ns.temp.glob(src.stem + "*.pickle"):
                zf.write(str(f), str(dest.parent / f.name))
                try:
                    f.unlink()
                except:
                    log_exception("Failed to delete {}", f)
        except:
            log_exception("Failed to compile {}", src)
        finally:
            try:
                tmp.unlink()
            except:
                log_exception("Failed to delete {}", tmp)

    zf.write(str(src), str(dest))


def generate_source_files(ns):
    if ns.zip_lib:
        zip_name = PYTHON_ZIP_NAME
        zip_path = ns.temp / zip_name
        if zip_path.is_file():
            zip_path.unlink()
        elif zip_path.is_dir():
            log_error(
                "Cannot create zip file because a directory exists by the same name"
            )
            return
        log_info("Generating {} in {}", zip_name, ns.temp)
        ns.temp.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zf:
            for dest, src in get_lib_layout(ns):
                _write_to_zip(zf, dest, src, ns, checked=False)

    if ns.include_underpth:
        log_info("Generating {} in {}", PYTHON_PTH_NAME, ns.temp)
        ns.temp.mkdir(parents=True, exist_ok=True)
        with open(ns.temp / PYTHON_PTH_NAME, "w", encoding="utf-8") as f:
            if ns.zip_lib:
                print(PYTHON_ZIP_NAME, file=f)
                if ns.include_pip:
                    print("packages", file=f)
            else:
                print("Lib", file=f)
                print("Lib/site-packages", file=f)
            if not ns.flat_dlls:
                print("DLLs", file=f)
            print(".", file=f)
            print(file=f)
            print("# Uncomment to run site.main() automatically", file=f)
            print("#import site", file=f)

    if ns.include_pip:
        log_info("Extracting pip")
        extract_pip_files(ns)


def _create_zip_file(ns):
    if not ns.zip:
        return None

    if ns.zip.is_file():
        try:
            ns.zip.unlink()
        except OSError:
            log_exception("Unable to remove {}", ns.zip)
            sys.exit(8)
    elif ns.zip.is_dir():
        log_error("Cannot create ZIP file because {} is a directory", ns.zip)
        sys.exit(8)

    ns.zip.parent.mkdir(parents=True, exist_ok=True)
    return zipfile.ZipFile(ns.zip, "w", zipfile.ZIP_DEFLATED)


def copy_files(files, ns):
    if ns.copy:
        ns.copy.mkdir(parents=True, exist_ok=True)

    try:
        total = len(files)
    except TypeError:
        total = None
    count = 0

    zip_file = _create_zip_file(ns)
    try:
        need_compile = []
        in_catalog = []

        for dest, src in files:
            count += 1
            if count % 10 == 0:
                if total:
                    log_info("Processed {:>4} of {} files", count, total)
                else:
                    log_info("Processed {} files", count)
            log_debug("Processing {!s}", src)

            if isinstance(src, tuple):
                src, content = src
                if ns.copy:
                    log_debug("Copy {} -> {}", src, ns.copy / dest)
                    (ns.copy / dest).parent.mkdir(parents=True, exist_ok=True)
                    with open(ns.copy / dest, "wb") as f:
                        f.write(content)
                if ns.zip:
                    log_debug("Zip {} into {}", src, ns.zip)
                    zip_file.writestr(str(dest), content)
                continue

            if (
                ns.precompile
                and src in PY_FILES
                and src not in EXCLUDE_FROM_COMPILE
                and src.parent not in DATA_DIRS
                and os.path.normcase(str(dest)).startswith(os.path.normcase("Lib"))
            ):
                if ns.copy:
                    need_compile.append((dest, ns.copy / dest))
                else:
                    (ns.temp / "Lib" / dest).parent.mkdir(parents=True, exist_ok=True)
                    copy_if_modified(src, ns.temp / "Lib" / dest)
                    need_compile.append((dest, ns.temp / "Lib" / dest))

            if src not in EXCLUDE_FROM_CATALOG:
                in_catalog.append((src.name, src))

            if ns.copy:
                log_debug("Copy {} -> {}", src, ns.copy / dest)
                (ns.copy / dest).parent.mkdir(parents=True, exist_ok=True)
                try:
                    copy_if_modified(src, ns.copy / dest)
                except shutil.SameFileError:
                    pass

            if ns.zip:
                log_debug("Zip {} into {}", src, ns.zip)
                zip_file.write(src, str(dest))

        if need_compile:
            for dest, src in need_compile:
                compiled = [
                    _compile_one_py(src, None, dest, optimize=0),
                    _compile_one_py(src, None, dest, optimize=1),
                    _compile_one_py(src, None, dest, optimize=2),
                ]
                for c in compiled:
                    if not c:
                        continue
                    cdest = Path(dest).parent / Path(c).relative_to(src.parent)
                    if ns.zip:
                        log_debug("Zip {} into {}", c, ns.zip)
                        zip_file.write(c, str(cdest))
                    in_catalog.append((cdest.name, cdest))

        if ns.catalog:
            # Just write out the CDF now. Compilation and signing is
            # an extra step
            log_info("Generating {}", ns.catalog)
            ns.catalog.parent.mkdir(parents=True, exist_ok=True)
            write_catalog(ns.catalog, in_catalog)

    finally:
        if zip_file:
            zip_file.close()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-v", help="Increase verbosity", action="count")
    parser.add_argument(
        "-s",
        "--source",
        metavar="dir",
        help="The directory containing the repository root",
        type=Path,
        default=None,
    )
    parser.add_argument(
        "-b", "--build", metavar="dir", help="Specify the build directory", type=Path
    )
    parser.add_argument(
        "--arch",
        metavar="architecture",
        help="Specify the target architecture",
        type=str,
        default=None,
    )
    parser.add_argument(
        "--doc-build",
        metavar="dir",
        help="Specify the docs build directory",
        type=Path,
        default=None,
    )
    parser.add_argument(
        "--copy",
        metavar="directory",
        help="The name of the directory to copy an extracted layout to",
        type=Path,
        default=None,
    )
    parser.add_argument(
        "--zip",
        metavar="file",
        help="The ZIP file to write all files to",
        type=Path,
        default=None,
    )
    parser.add_argument(
        "--catalog",
        metavar="file",
        help="The CDF file to write catalog entries to",
        type=Path,
        default=None,
    )
    parser.add_argument(
        "--log",
        metavar="file",
        help="Write all operations to the specified file",
        type=Path,
        default=None,
    )
    parser.add_argument(
        "-t",
        "--temp",
        metavar="file",
        help="A temporary working directory",
        type=Path,
        default=None,
    )
    parser.add_argument(
        "-d", "--debug", help="Include debug build", action="store_true"
    )
    parser.add_argument(
        "-p",
        "--precompile",
        help="Include .pyc files instead of .py",
        action="store_true",
    )
    parser.add_argument(
        "-z", "--zip-lib", help="Include library in a ZIP file", action="store_true"
    )
    parser.add_argument(
        "--flat-dlls", help="Does not create a DLLs directory", action="store_true"
    )
    parser.add_argument(
        "-a",
        "--include-all",
        help="Include all optional components",
        action="store_true",
    )
    parser.add_argument(
        "--include-cat",
        metavar="file",
        help="Specify the catalog file to include",
        type=Path,
        default=None,
    )
    for opt, help in get_argparse_options():
        parser.add_argument(opt, help=help, action="store_true")

    ns = parser.parse_args()
    update_presets(ns)

    ns.source = ns.source or (Path(__file__).resolve().parent.parent.parent)
    ns.build = ns.build or Path(sys.executable).parent
    ns.temp = ns.temp or Path(tempfile.mkdtemp())
    ns.doc_build = ns.doc_build or (ns.source / "Doc" / "build")
    if not ns.source.is_absolute():
        ns.source = (Path.cwd() / ns.source).resolve()
    if not ns.build.is_absolute():
        ns.build = (Path.cwd() / ns.build).resolve()
    if not ns.temp.is_absolute():
        ns.temp = (Path.cwd() / ns.temp).resolve()
    if not ns.doc_build.is_absolute():
        ns.doc_build = (Path.cwd() / ns.doc_build).resolve()
    if ns.include_cat and not ns.include_cat.is_absolute():
        ns.include_cat = (Path.cwd() / ns.include_cat).resolve()
    if not ns.arch:
        ns.arch = "amd64" if sys.maxsize > 2 ** 32 else "win32"

    if ns.copy and not ns.copy.is_absolute():
        ns.copy = (Path.cwd() / ns.copy).resolve()
    if ns.zip and not ns.zip.is_absolute():
        ns.zip = (Path.cwd() / ns.zip).resolve()
    if ns.catalog and not ns.catalog.is_absolute():
        ns.catalog = (Path.cwd() / ns.catalog).resolve()

    configure_logger(ns)

    log_info(
        """OPTIONS
Source: {ns.source}
Build:  {ns.build}
Temp:   {ns.temp}
Arch:   {ns.arch}

Copy to: {ns.copy}
Zip to:  {ns.zip}
Catalog: {ns.catalog}""",
        ns=ns,
    )

    if ns.arch not in ("win32", "amd64", "arm32", "arm64"):
        log_error("--arch is not a valid value (win32, amd64, arm32, arm64)")
        return 4
    if ns.arch in ("arm32", "arm64"):
        for n in ("include_idle", "include_tcltk"):
            if getattr(ns, n):
                log_warning(f"Disabling --{n.replace('_', '-')} on unsupported platform")
                setattr(ns, n, False)

    if ns.include_idle and not ns.include_tcltk:
        log_warning("Assuming --include-tcltk to support --include-idle")
        ns.include_tcltk = True

    try:
        generate_source_files(ns)
        files = list(get_layout(ns))
        copy_files(files, ns)
    except KeyboardInterrupt:
        log_info("Interrupted by Ctrl+C")
        return 3
    except SystemExit:
        raise
    except:
        log_exception("Unhandled error")

    if error_was_logged():
        log_error("Errors occurred.")
        return 1


if __name__ == "__main__":
    sys.exit(int(main() or 0))
