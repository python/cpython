"""
Generates a layout of Python for Windows from a build.
"""

__author__ = "Jeremy Paige <jeremyp@activestate.com>"
__version__ = "2.7"

import argparse
import os
import shutil
import sys
import tempfile
import zipfile

from glob import glob

if __name__ == "__main__":
    # Started directly, so enable relative imports
    __path__ = [os.path.dirname(os.path.realpath(__file__))]

from .support.catalog import *
from .support.constants import *
from .support.filesets import *
from .support._logging import *
from .support.options import *
from .support.pip import *
from .support.props import *
from .support.nuspec import *

TEST_PYDS_ONLY = FileStemSet("xxlimited", "xxlimited_35", "_ctypes_test", "_test*")
TEST_DIRS_ONLY = FileNameSet("test", "tests")

IDLE_DIRS_ONLY = FileNameSet("idlelib")

TCLTK_PYDS_ONLY = FileStemSet("tcl*", "tk*", "_tkinter")
TCLTK_DIRS_ONLY = FileNameSet("tkinter", "turtledemo")
TCLTK_FILES_ONLY = FileNameSet("turtle.py")

EXCLUDE_FROM_PYDS = FileStemSet("python*", "pyshellext")
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

TOOLS_DIRS = FileNameSet("scripts", "i18n", "parser")
TOOLS_FILES = FileSuffixSet(".py", ".pyw", ".txt")


def copy_if_modified(src, dest):
    try:
        dest_stat = os.stat(dest)
    except WindowsError:
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
        if os.path.isdir(f):
            if f in TEST_DIRS_ONLY:
                return ns.include_tests
            if f in TCLTK_DIRS_ONLY:
                return ns.include_tcltk
            if f in IDLE_DIRS_ONLY:
                return ns.include_idle
        else:
            if f in TCLTK_FILES_ONLY:
                return ns.include_tcltk
        return True

    for dest, src in rglob(os.path.join(ns.source, "Lib"), "**/*", _c):
        yield dest, src


def get_tcltk_lib(ns):
    if not ns.include_tcltk:
        return

    tcl_lib = os.getenv("TCL_LIBRARY")
    if not tcl_lib or not os.path.isdir(tcl_lib):
        try:
            with open(os.path.join(ns.build, "TCL_LIBRARY.env"), "r") as f:
                tcl_lib = f.read().strip()
        except IOError:
            pass
        if not tcl_lib or not os.path.isdir(tcl_lib):
            log_warning("Failed to find TCL_LIBRARY")
            return

    for dest, src in glob(os.path.join(os.path.dirname(tcl_lib), "**/*")):
        yield "tcl/{}".format(dest), src


def get_layout(ns):
    def in_build(f, dest="", new_name=None):
        n, _, x = f.rpartition(".")
        n = new_name or n
        src = os.path.join(ns.build, f)
        if ns.debug and src not in REQUIRED_DLLS:
            stem, suffix = os.path.splitext(src)
            if not stem.endswith("_d"):
                src = os.path.join(os.path.dirname(src), (stem + "_d" + suffix))
            if not n.endswith("_d"):
                n += "_d"
                f = n + "." + x
        yield dest + n + "." + x, src
        if ns.include_symbols:
            pdb = os.path.splitext(src)[0] + ".pdb"
            if os.path.isfile(pdb):
                yield dest + n + ".pdb", pdb
        if ns.include_dev:
            lib = os.path.splitext(src)[0] + ".lib"
            if os.path.isfile(lib):
                yield "libs/" + n + ".lib", lib

    for item in in_build("python.exe", new_name="python"):
        yield item
    for item in in_build("pythonw.exe", new_name="pythonw"):
        yield item

    for item in in_build(PYTHON_DLL_NAME):
        yield item

    yield "LICENSE", "LICENSE"

    for dest, src in rglob(ns.build, ("*.pyd", "*.dll")):
        if os.path.splitext(src)[0].endswith("_d") != bool(ns.debug) and src not in REQUIRED_DLLS:
            continue
        if src in EXCLUDE_FROM_PYDS:
            continue
        if src in TEST_PYDS_ONLY and not ns.include_tests:
            continue
        if src in TCLTK_PYDS_ONLY and not ns.include_tcltk:
            continue

        for item in in_build(os.path.basename(src), dest="" if ns.flat_dlls else "DLLs/"):
            yield item

    if ns.zip_lib:
        zip_name = PYTHON_ZIP_NAME
        yield zip_name, os.path.join(ns.temp, zip_name)
    else:
        for dest, src in get_lib_layout(ns):
            yield "Lib/{}".format(dest), src

    if ns.include_tools:

        def _c(d):
            if os.path.isdir(d):
                return d in TOOLS_DIRS
            return d in TOOLS_FILES

        for dest, src in rglob(os.path.join(ns.source, "Tools"), "**/*", _c):
            yield "Tools/{}".format(dest), src

    if ns.include_underpth:
        yield PYTHON_PTH_NAME, os.path.join(ns.temp, PYTHON_PTH_NAME)

    if ns.include_dev:

        for dest, src in rglob(os.path.join(ns.source, "Include"), "**/*.h"):
            yield "include/{}".format(dest), src
        src = os.path.join(ns.source, "PC", "pyconfig.h")
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
        for dest, src in rglob(os.path.join(ns.doc_build, "htmlhelp"), PYTHON_CHM_NAME):
            yield "Doc/{}".format(dest), src

    if ns.include_html_doc:
        for dest, src in rglob(os.path.join(ns.doc_build, "html"), "**/*"):
            yield "Doc/html/{}".format(dest), src

    if ns.include_props:
        for dest, src in get_props_layout(ns):
            yield dest, src

    if ns.include_nuspec:
        for dest, src in get_nuspec_layout(ns):
            yield dest, src

    if ns.include_cat:
        if ns.flat_dlls:
            yield os.path.basename(ns.include_cat), ns.include_cat
        else:
            yield "DLLs/{}".format(os.path.basename(ns.include_cat)), ns.include_cat


def _compile_one_py(src, dest, name):
    import py_compile

    if dest is not None:
        dest = str(dest)

    try:
        return py_compile.compile(
            str(src),
            dest,
            str(name),
            doraise=True,
        )
    except py_compile.PyCompileError:
        log_warning("Failed to compile %s", src)
        return None


# name argument added to address bpo-37641
def _py_temp_compile(src, name, ns, dest_dir=None, checked=True):
    if not ns.precompile or src not in PY_FILES or os.path.dirname(src) in DATA_DIRS:
        return None
    dest = os.path.join((dest_dir or ns.temp), (os.path.splitext(src)[0] + ".pyc"))
    return _compile_one_py(src, dest, name)


def _write_to_zip(zf, dest, src, ns):
    pyc = _py_temp_compile(src, dest, ns)
    if pyc:
        try:
            zf.write(str(pyc), dest.with_suffix(".pyc"))
        finally:
            try:
                pyc.unlink()
            except:
                log_exception("Failed to delete %s", pyc)
        return

    if src in LIB2TO3_GRAMMAR_FILES:
        from lib2to3.pgen2.driver import load_grammar

        tmp = os.path.join(ns.temp, os.path.basename(src))
        try:
            shutil.copy(src, tmp)
            load_grammar(str(tmp))
            for f in glob(os.path.join(ns.temp, os.path.splitext(src)[0] + "*.pickle")):
                zf.write(str(f), os.path.join(os.path.dirname(dest), os.path.basename(f)))
                try:
                    f.unlink()
                except:
                    log_exception("Failed to delete %s", f)
        except:
            log_exception("Failed to compile %s", src)
        finally:
            try:
                tmp.unlink()
            except:
                log_exception("Failed to delete %s", tmp)

    zf.write(str(src), str(dest))


def generate_source_files(ns):
    if ns.zip_lib:
        zip_name = PYTHON_ZIP_NAME
        zip_path = os.path.join(ns.temp, zip_name)
        if os.path.isfile(zip_path):
            zip_path.unlink()
        elif os.path.isdir(zip_path):
            log_error(
                "Cannot create zip file because a directory exists by the same name"
            )
            return
        log_info("Generating %s in %s", zip_name, ns.temp)
        if not os.path.exists(ns.temp):
            os.makedirs(ns.temp)
        with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zf:
            for dest, src in get_lib_layout(ns):
                _write_to_zip(zf, dest, src, ns)

    if ns.include_underpth:
        log_info("Generating %s in %s", PYTHON_PTH_NAME, ns.temp)
        if not os.path.exists(ns.temp):
            os.makedirs(ns.temp)
        with open(os.path.join(ns.temp, PYTHON_PTH_NAME), "w") as f:
            if ns.zip_lib:
                print PYTHON_ZIP_NAME >> f
                if ns.include_pip:
                    print "packages" >> f
            else:
                print "Lib" >> f
                print "Lib/site-packages" >> f
            if not ns.flat_dlls:
                print "DLLs" >> f
            print "." >> f
            print >> f
            print "# Uncomment to run site.main() automatically" >> f
            print "#import site" >> f

    if ns.include_pip:
        log_info("Extracting pip")
        extract_pip_files(ns)


def _create_zip_file(ns):
    if not ns.zip:
        return None

    if os.path.isfile(ns.zip):
        try:
            ns.zip.unlink()
        except OSError:
            log_exception("Unable to remove %s", ns.zip)
            sys.exit(8)
    elif os.path.isdir(ns.zip):
        log_error("Cannot create ZIP file because %s is a directory", ns.zip)
        sys.exit(8)

    if not os.path.exists(os.path.dirname(ns.zip)):
        os.makedirs(os.path.dirname(ns.zip))
    return zipfile.ZipFile(ns.zip, "w", zipfile.ZIP_DEFLATED)


def copy_files(files, ns):
    if ns.copy:
        if not os.path.exists(ns.copy):
            os.makedirs(ns.copy)

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
                    log_info("Processed %s of %s files", count, total)
                else:
                    log_info("Processed %s files", count)
            log_debug("Processing %s", str(src))

            if isinstance(src, tuple):
                src, content = src
                if ns.copy:
                    copy_dest = os.path.join(ns.copy, dest)
                    log_debug("Copy %s -> %s", src, copy_dest)
                    if not os.path.exists(os.path.dirname(copy_dest)):
                        os.makedirs(os.path.dirname(copy_dest))
                    with open(copy_dest, "wb") as f:
                        f.write(content)
                if ns.zip:
                    log_debug("Zip %s into %s", src, ns.zip)
                    zip_file.writestr(str(dest), content)
                continue

            if (
                ns.precompile
                and src in PY_FILES
                and src not in EXCLUDE_FROM_COMPILE
                and os.path.dirname(src) not in DATA_DIRS
                and os.path.normcase(str(dest)).startswith(os.path.normcase("Lib"))
            ):
                if ns.copy:
                    need_compile.append((dest, os.path.join(ns.copy, dest)))
                else:
                    temp_dest = os.path.join(ns.temp, "Lib", dest)
                    if not os.path.exists(os.path.dirname(temp_dest)):
                        os.makedirs(os.path.dirname(temp_dest))
                    copy_if_modified(src, temp_dest)
                    need_compile.append((dest, temp_dest))

            if src not in EXCLUDE_FROM_CATALOG:
                in_catalog.append((os.path.basename(src), src))

            if ns.copy:
                copy_dest = os.path.join(ns.copy, dest)
                log_debug("Copy %s -> %s", src, copy_dest)
                if not os.path.exists(os.path.dirname(copy_dest)):
                    os.makedirs(os.path.dirname(copy_dest))
                try:
                    copy_if_modified(src, copy_dest)
                except shutil.Error:
                    pass

            if ns.zip:
                log_debug("Zip %s into %s", src, ns.zip)
                zip_file.write(src, str(dest))

        if need_compile:
            for dest, src in need_compile:
                compiled = [
                    _compile_one_py(src, None, dest),
                    _compile_one_py(src, None, dest),
                    _compile_one_py(src, None, dest),
                ]
                for c in compiled:
                    if not c:
                        continue
                    cdest = os.path.join(os.path.dirname(dest), os.path.relpath(c, os.path.dirname(src)))
                    if ns.zip:
                        log_debug("Zip %s into %s", c, ns.zip)
                        zip_file.write(c, str(cdest))
                    in_catalog.append((os.path.basename(cdest), cdest))

        if ns.catalog:
            # Just write out the CDF now. Compilation and signing is
            # an extra step
            log_info("Generating %s", ns.catalog)
            if not os.path.exists(os.path.dirname(ns.catalog)):
                os.makedirs(os.path.dirname(ns.catalog))
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
        default=None,
    )
    parser.add_argument(
        "-b", "--build", metavar="dir", help="Specify the build directory"
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
        default=None,
    )
    parser.add_argument(
        "--copy",
        metavar="directory",
        help="The name of the directory to copy an extracted layout to",
        default=None,
    )
    parser.add_argument(
        "--zip",
        metavar="file",
        help="The ZIP file to write all files to",
        default=None,
    )
    parser.add_argument(
        "--catalog",
        metavar="file",
        help="The CDF file to write catalog entries to",
        default=None,
    )
    parser.add_argument(
        "--log",
        metavar="file",
        help="Write all operations to the specified file",
        default=None,
    )
    parser.add_argument(
        "-t",
        "--temp",
        metavar="file",
        help="A temporary working directory",
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
        default=None,
    )
    for opt, help in get_argparse_options():
        parser.add_argument(opt, help=help, action="store_true")

    ns = parser.parse_args()
    update_presets(ns)

    ns.source = ns.source or os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__))))
    ns.build = ns.build or os.path.dirname(sys.executable)
    ns.temp = ns.temp or tempfile.mkdtemp()
    ns.doc_build = ns.doc_build or os.path.join(ns.source, "Doc", "build")
    if not os.path.isabs(ns.source):
        ns.source = os.path.realpath(os.path.join(os.cwd(), ns.source))
    if not os.path.isabs(ns.build):
        ns.build = os.path.realpath(os.path.join(os.cwd(), ns.build))
    if not os.path.isabs(ns.temp):
        ns.temp = os.path.realpath(os.path.join(os.cwd(), ns.temp))
    if not os.path.isabs(ns.doc_build):
        ns.doc_build = os.path.realpath(os.path.join(os.cwd(), ns.doc_build))
    if ns.include_cat and not os.path.isabs(ns.include_cat):
        ns.include_cat = os.path.realpath(os.path.join(os.cwd(), ns.include_cat))
    if not ns.arch:
        ns.arch = "amd64" if sys.maxsize > 2 ** 32 else "win32"

    if ns.copy and not os.path.isabs(ns.copy):
        ns.copy = os.path.realpath(os.path.join(os.cwd(), ns.copy))
    if ns.zip and not os.path.isabs(ns.zip):
        ns.zip = os.path.realpath(os.path.join(os.cwd(), ns.zip))
    if ns.catalog and not os.path.isabs(ns.catalog):
        ns.catalog = os.path.realpath(os.path.join(os.cwd(), ns.catalog))

    configure_logger(ns)

    log_info(
        """OPTIONS
Source: %s
Build:  %s
Temp:   %s
Arch:   %s

Copy to: %s
Zip to:  %s
Catalog: %s""",
        ns.source,
        ns.build,
        ns.temp,
        ns.arch,
        ns.copy,
        ns.zip,
        ns.catalog,
    )

    if ns.arch not in ("win32", "amd64", "arm32", "arm64"):
        log_error("--arch is not a valid value (win32, amd64, arm32, arm64)")
        return 4
    if ns.arch in ("arm32", "arm64"):
        for n in ("include_idle", "include_tcltk"):
            if getattr(ns, n):
                log_warning("Disabling --{} on unsupported platform".format(n.replace("_", "-")))
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
