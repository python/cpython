# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.

"""Shared configuration for the Nanvix CPython build system.

Centralizes constants, defaults, and path helpers previously spread
across defaults.mk, common.mk, and the various test-*.mk files.
"""

from __future__ import annotations

import os
import platform
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Platform defaults
# ---------------------------------------------------------------------------

DOCKER_IMAGE = "nanvix/toolchain:latest-minimal"
DEFAULT_PLATFORM = "microvm"
DEFAULT_PROCESS_MODE = "standalone"
DEFAULT_MEMORY_SIZE = "256mb"
DEFAULT_INSTALL_PREFIX = "/sysroot"

# Python version string — centralized to avoid shotgun surgery if updated.
PYTHON_VERSION = "3.12"
PYTHON_LIB_DIR = f"python{PYTHON_VERSION}"

# ELF suffix for cross-compiled binaries
EXE = ".elf"

# The sysconfig data module name baked into the build.
# Matches _sysconfigdata_{ABIFLAGS}_{MACHDEP}_{MULTIARCH} from configure.
# Must be passed as _PYTHON_SYSCONFIGDATA_NAME inside the guest VM so that
# sysconfig can find the module regardless of how sys.platform resolves
# at runtime in the guest kernel.
SYSCONFIGDATA_NAME = "_sysconfigdata__nanvix_"

# ---------------------------------------------------------------------------
# Toolchain
# ---------------------------------------------------------------------------

TOOLCHAIN_TRIPLET = "i686-nanvix"
TOOLCHAIN_DEFAULT_PATH = "/opt/nanvix"

# Docker-internal paths
DOCKER_TOOLCHAIN_PATH = "/opt/nanvix"
DOCKER_SYSROOT_PATH = "/mnt/sysroot"
DOCKER_WORKSPACE_PATH = "/mnt/workspace"


def toolchain_paths(
    toolchain: str | Path,
    sysroot: str | Path,
) -> dict[str, Path]:
    """Return resolved paths to toolchain binaries and libraries."""
    tc = Path(toolchain)
    sr = Path(sysroot)
    return {
        "cc": tc / "bin" / f"{TOOLCHAIN_TRIPLET}-gcc",
        "cxx": tc / "bin" / f"{TOOLCHAIN_TRIPLET}-g++",
        "ld": tc / "bin" / f"{TOOLCHAIN_TRIPLET}-ld",
        "ar": tc / "bin" / f"{TOOLCHAIN_TRIPLET}-ar",
        "ranlib": tc / "bin" / f"{TOOLCHAIN_TRIPLET}-ranlib",
        "strip": tc / "bin" / f"{TOOLCHAIN_TRIPLET}-strip",
        "build_python": tc / "bin" / "python3",
        "libc": tc / f"{TOOLCHAIN_TRIPLET}" / "lib" / "libc.a",
        "libm": tc / f"{TOOLCHAIN_TRIPLET}" / "lib" / "libm.a",
        "libposix": sr / "lib" / "libposix.a",
        "libz": sr / "lib" / "libz.a",
        "libsqlite3": sr / "lib" / "libsqlite3.a",
        "libssl": sr / "lib" / "libssl.a",
        "libcrypto": sr / "lib" / "libcrypto.a",
    }


def configure_env(toolchain: str | Path, sysroot: str | Path) -> dict[str, str]:
    """Return the environment dict for ./configure."""
    tp = toolchain_paths(toolchain, sysroot)
    sr = Path(sysroot)
    return {
        "CC": str(tp["cc"]),
        "CXX": str(tp["cxx"]),
        "LD": str(tp["ld"]),
        "AR": str(tp["ar"]),
        "RANLIB": str(tp["ranlib"]),
        "CFLAGS": (
            f"-O3 -fomit-frame-pointer -fno-unwind-tables "
            f"-fno-asynchronous-unwind-tables -I{sr}/include"
        ),
        "CFLAGS_NODIST": "-fno-semantic-interposition",
        "LDFLAGS": (
            f"-L{sr}/lib -T{sr}/lib/user.ld "
            f"-Wl,--allow-multiple-definition -no-pie "
            f"-Wl,--export-dynamic -Wl,--no-dynamic-linker"
        ),
        "LIBS": (
            f"-Wl,--start-group {tp['libposix']} {tp['libc']} {tp['libm']} "
            f"-lsqlite3 -lssl -lcrypto -lz -lbz2 -lffi -Wl,--end-group"
        ),
        "LIBSQLITE3_LIBS": f"-L{sr}/lib -lsqlite3",
        "LIBSQLITE3_CFLAGS": f"-I{sr}/include",
        "ZLIB_LIBS": f"-L{sr}/lib -lz",
        "ZLIB_CFLAGS": f"-I{sr}/include",
        "BZIP2_LIBS": f"-L{sr}/lib -lbz2",
        "BZIP2_CFLAGS": f"-I{sr}/include",
        "LIBFFI_LIBS": f"-L{sr}/lib -lffi",
        "LIBFFI_CFLAGS": f"-I{sr}/include",
    }


def configure_opts(
    build_python: str | Path,
    libc: str | Path,
    libm: str | Path,
    sysroot: str | Path,
    install_prefix: str = DEFAULT_INSTALL_PREFIX,
    release: bool = False,
) -> list[str]:
    """Return the ./configure option list."""
    opts = [
        "--disable-shared",
        "--build=x86_64-pc-linux-gnux32",
        f"--host={TOOLCHAIN_TRIPLET}",
        f"--with-build-python={build_python}",
    ]
    if release:
        opts.append("--disable-test-modules")
    opts.extend([
        f"--with-libc={libc}",
        f"--with-libm={libm}",
        f"--prefix={install_prefix}",
        f"--exec-prefix={install_prefix}",
        "--with-ensurepip=no",
        "--with-pkg-config=no",
        f"--with-openssl={sysroot}",
        "--disable-ipv6",
    ])
    if release:
        opts.append("--without-doc-strings")
    opts.extend([
        "--with-computed-gotos",
        "ac_cv_file__dev_ptmx=no",
        "ac_cv_file__dev_ptc=no",
        "ac_cv_pthread_is_default=yes",
        "ac_cv_pthread=yes",
        "ac_cv_kthread=no",
        "ac_cv_func_dlopen=yes",
        "ac_cv_header_dlfcn_h=yes",
    ])
    return opts


# ---------------------------------------------------------------------------
# Test module lists
# ---------------------------------------------------------------------------

# Canonical list of stdlib test modules known to pass on Nanvix.
NANVIX_TEST_LIST: list[str] = [
    "test_float", "test_complex", "test_bool", "test_struct",
    "test_int", "test_range", "test_slice", "test_memoryview", "test_bytes", "test_tuple",
    "test_builtin", "test_operator", "test_binop", "test_unary",
    "test_compare", "test_richcmp", "test_augassign", "test_contains",
    "test_grammar", "test_syntax", "test_compile", "test_compiler_assemble",
    "test_compiler_codegen", "test_ast", "test_symtable", "test_opcache",
    "test_peepholer", "test_dis", "test_code", "test_keyword", "test_tokenize",
    "test_perf_profiler",
    "test_call", "test_extcall", "test_positional_only_arg",
    "test_scope", "test_global", "test_dynamic", "test_with",
    "test_types", "test_typechecks", "test_isinstance", "test_hash",
    "test_index", "test_super", "test_property",
    "test_math", "test_cmath", "test_decimal", "test_fractions",
    "test_statistics", "test_random", "test_numeric_tower",
    "test_exception_group", "test_exceptions", "test_raise", "test_traceback",
    "test_frame", "test_contextlib", "test_contextlib_async",
    "test_pprint", "test_reprlib",
    "test_list", "test_dict",
    "test_listcomps", "test_dictcomps", "test_setcomps", "test_genexps",
    "test_set",
    "test_heapq", "test_bisect", "test_queue", "test_sort",
    "test_iter", "test_itertools", "test_iterlen",
    "test_generators", "test_generator_stop", "test_yield_from", "test_coroutines",
    "test_functools", "test_funcattrs", "test_decorators",
    "test_copy", "test_copyreg",
    "test_collections", "test_defaultdict", "test_ordered_dict", "test_deque", "test_array",
    "test_weakref", "test_weakset", "test_buffer",
    "test_genericpath", "test_posixpath", "test_ntpath", "test_pathlib",
    "test_fnmatch", "test_glob", "test_filecmp", "test_linecache", "test_stat",
    "test_memoryio", "test_bufio", "test_fileinput",
    "test_io", "test_fileio", "test_file", "test_file_eintr",
    "test_source_encoding",
    "test_os", "test_posix",
    "test_tempfile", "test_shutil",
    "test_zipfile", "test_zipapp", "test_zipimport",
    "test_tarfile", "test_gzip", "test_bz2", "test_zlib",
    "test_dbm_dumb", "test_shelve",
    "test_import", "test_pkgutil", "test_modulefinder",
]

# Default batch size for regrtest VM invocations.
DEFAULT_TEST_BATCH_SIZE = 4

# Per-mode test exclusions (passed to regrtest --ignore).
STANDALONE_EXCLUDE: list[str] = [
    "test_queue",      # NSKIP019: standalone 32 MB heap too small for module
    "test_itertools",  # NSKIP019: standalone 32 MB heap too small for module
    "test_functools",  # NSKIP019: standalone 32 MB heap too small for module
    "test_io",         # NSKIP019: standalone 32 MB heap too small for module
    "test_zipfile",    # NSKIP019: standalone too slow / heap too small for module
    "test_import",     # NSKIP019: standalone 32 MB heap too small for module
]

# Platform-specific nanvixd extra arguments.
PLATFORM_NANVIXD_ARGS: dict[str, list[str]] = {
    "microvm": [],
    "hyperlight": [],
}

# ---------------------------------------------------------------------------
# Sysroot trimming
# ---------------------------------------------------------------------------

# Directories removed from sysroot during ramfs trimming.
SYSROOT_TRIM_DIRS: list[str] = [
    f"lib/{PYTHON_LIB_DIR}/config-{PYTHON_VERSION}",
    f"lib/{PYTHON_LIB_DIR}/idlelib",
    f"lib/{PYTHON_LIB_DIR}/tkinter",
    f"lib/{PYTHON_LIB_DIR}/turtledemo",
    f"lib/{PYTHON_LIB_DIR}/lib2to3",
    f"lib/{PYTHON_LIB_DIR}/ensurepip",
    f"lib/{PYTHON_LIB_DIR}/pydoc_data",
    f"lib/{PYTHON_LIB_DIR}/venv",
    f"lib/{PYTHON_LIB_DIR}/site-packages",
    f"lib/{PYTHON_LIB_DIR}/__phello__",
    "include",
    "share",
    "lib/pkgconfig",
]

# Files removed from sysroot bin/ during ramfs trimming.
SYSROOT_TRIM_BIN_PATTERNS: list[str] = [
    "2to3*", "idle3*", "pydoc3*",
    "python3-config", f"python{PYTHON_VERSION}-config", "python3",
]

# ---------------------------------------------------------------------------
# Docker tar excludes (for Windows host-side source sync)
# ---------------------------------------------------------------------------

DOCKER_TAR_EXCLUDES: list[str] = [
    ".git", ".nanvix/venv", ".nanvix/cache", ".nanvix/sysroot",
    ".nanvix/buildroot", "Doc", "Lib/idlelib", "Lib/tkinter",
    "Lib/turtledemo", "Lib/ensurepip", "PC", "PCbuild",
]

# Files needing CRLF → LF normalization for autotools.
DOCKER_CRLF_FILES: list[str] = [
    "configure", "config.guess", "config.sub", "install-sh",
    "Modules/makesetup", "Modules/Setup",
    "Modules/Setup.bootstrap.in", "Modules/Setup.stdlib.in",
    "Modules/config.c.in", "Modules/ld_so_aix.in",
    "Makefile.pre.in", "pyconfig.h.in", "aclocal.m4", "configure.ac",
    "Misc/python.pc.in", "Misc/python-embed.pc.in",
    "Misc/python-config.sh.in", "Misc/python-config.in",
]

# Docker output files to copy back to host workspace.
DOCKER_OUTPUT_FILES: list[str] = [
    "python", "python.exe", "python.elf", "python.wasm", f"libpython{PYTHON_VERSION}.a",
    "pybuilddir.txt",
]

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

IS_WINDOWS = sys.platform == "win32"


def nanvixd_binary() -> str:
    """Return the nanvixd binary name for the current platform."""
    if IS_WINDOWS:
        return "nanvixd.exe"
    return "nanvixd.elf"


def mkramfs_binary() -> str:
    """Return the mkramfs binary name for the current platform."""
    if IS_WINDOWS:
        return "mkramfs.exe"
    return "mkramfs.elf"


# Windows host-native binaries needed for local test execution.
# These are downloaded from the Nanvix release page during setup.
# kernel.elf is a *guest* binary (not .exe) — nanvixd loads it directly.
WINDOWS_HOST_BINARIES: list[str] = ["nanvixd.exe", "mkramfs.exe", "kernel.elf"]


def python_binary() -> str:
    """Return the cross-compiled Python binary name."""
    return f"python{PYTHON_VERSION}"
