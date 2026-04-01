"""conf_init — Bootstrap, source dirs, version, pkg-config.

Handles the earliest configuration steps: platform feature macros
(NetBSD, FreeBSD, Darwin), source/build directory detection, git
metadata, canonical host triplet, build-python selection (including
cross-compilation), prefix/directory layout, VERSION/SOVERSION,
pkg-config probing, and the missing-stdlib-config option.
"""

from __future__ import annotations

import conf_modules
import pyconf

PYTHON_VERSION = "3.15"

WITH_BUILD_PYTHON = pyconf.arg_with(
    "build-python",
    metavar=f"python{PYTHON_VERSION}",
    help=(
        f"path to build python binary for cross compiling "
        f"(default: _bootstrap_python or python{PYTHON_VERSION})"
    ),
)
WITH_PKG_CONFIG = pyconf.arg_with(
    "pkg-config",
    metavar="yes|no|check",
    help="use pkg-config to detect build options (default is check)",
)
WITH_MISSING_STDLIB_CONFIG = pyconf.arg_with(
    "missing-stdlib-config",
    metavar="FILE",
    help="File with custom module error messages for missing stdlib modules",
)


def setup_platform_defines():
    """pyconfig.h platform feature macros for NetBSD, FreeBSD, Darwin."""
    # The later definition of _XOPEN_SOURCE and _POSIX_C_SOURCE disables
    # certain features on NetBSD, so we need _NETBSD_SOURCE to re-enable them.
    pyconf.define(
        "_NETBSD_SOURCE",
        1,
        "Define on NetBSD to activate all library features",
    )
    # The later definition of _XOPEN_SOURCE and _POSIX_C_SOURCE disables
    # certain features on FreeBSD, so we need __BSD_VISIBLE to re-enable them.
    pyconf.define(
        "__BSD_VISIBLE",
        1,
        "Define on FreeBSD to activate all library features",
    )
    # The later definition of _XOPEN_SOURCE and _POSIX_C_SOURCE disables
    # certain features on Mac OS X, so we need _DARWIN_C_SOURCE to re-enable them.
    pyconf.define(
        "_DARWIN_C_SOURCE",
        1,
        "Define on Darwin to activate all library features",
    )


def setup_source_dirs(v):
    """Detect source and build directories, set srcdir/abs_srcdir/abs_builddir."""
    srcdir_raw = pyconf._srcdir_arg or v.srcdir or "."
    srcdir_abs = pyconf.abspath(srcdir_raw)
    builddir = pyconf.abspath(".")
    out_of_tree = srcdir_abs != builddir
    # Match autoconf: preserve the path form that was given.  If --srcdir
    # was absolute (e.g. /src), keep it absolute; if relative, keep it
    # relative.  For in-tree builds, always use ".".
    if out_of_tree:
        srcdir = srcdir_raw
    else:
        srcdir = "."
    v.export("srcdir", srcdir)
    v.export("abs_srcdir", srcdir_abs)
    v.export("abs_builddir", builddir)
    pyconf.srcdir = srcdir_abs

    if out_of_tree:
        # If we're building out-of-tree, we need to make sure the following
        # resources get picked up before their $srcdir counterparts:
        #   Objects/ -> typeslots.inc
        #   Include/ -> Python.h
        # (A side effect of this is that these resources will automatically be
        #  regenerated when building out-of-tree, regardless of whether or not
        #  the $srcdir counterpart is up-to-date. This is an acceptable trade-off.)
        v.export("BASECPPFLAGS", "-IObjects -IInclude -IPython")
    else:
        v.export("BASECPPFLAGS", "")

    v.srcdir_abs = srcdir_abs


def setup_git_metadata(v):
    """Detect git and set GITVERSION, GITTAG, GITBRANCH."""
    git_dir = pyconf.path_join([v.srcdir_abs, ".git"])
    if pyconf.path_exists(git_dir):
        HAS_GIT = bool(pyconf.find_prog("git"))
    else:
        HAS_GIT = False

    if HAS_GIT:
        v.GITVERSION = "git --git-dir $(srcdir)/.git rev-parse --short HEAD"
        v.GITTAG = (
            "git --git-dir $(srcdir)/.git describe --all --always --dirty"
        )
        v.GITBRANCH = "git --git-dir $(srcdir)/.git name-rev --name-only HEAD"
    else:
        v.GITVERSION = ""
        v.GITTAG = ""
        v.GITBRANCH = ""
    v.export("GITVERSION")
    v.export("GITTAG")
    v.export("GITBRANCH")


def setup_canonical_host(v):
    """Run canonical host detection and validate cross-compilation settings."""
    pyconf.canonical_host()
    v.cross_compiling = pyconf.cross_compiling
    v.host = pyconf.host
    v.host_cpu = pyconf.host_cpu
    v.export("build", pyconf.build)
    v.export("host")

    if v.cross_compiling == "maybe":
        pyconf.error(
            "error: Cross compiling requires --host=HOST-TUPLE and --build=ARCH"
        )

    # pybuilddir.txt will be created by --generate-posix-vars in the Makefile
    pyconf.rm_f("pybuilddir.txt")


def setup_build_python(v):
    """Handle --with-build-python, freeze module, and PYTHON_FOR_REGEN."""
    bp_raw = WITH_BUILD_PYTHON.value
    PYTHON_FOR_REGEN: str = ""
    with_build_python = None

    if bp_raw is not None:
        pyconf.checking("for --with-build-python")
        if bp_raw == "yes":
            with_build_python = f"python{PYTHON_VERSION}"
        elif bp_raw == "no":
            pyconf.error(
                "error: invalid --with-build-python option: expected path or 'yes', not 'no'"
            )
        else:
            with_build_python = bp_raw
        if not pyconf.find_prog(with_build_python):
            pyconf.error(
                f"error: invalid or missing build python binary '{with_build_python}'"
            )
        build_python_ver = pyconf.cmd_output(
            [
                with_build_python,
                "-c",
                "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}')",
            ]
        )
        if build_python_ver != PYTHON_VERSION:
            pyconf.error(
                f"error: '{with_build_python}' has incompatible version {build_python_ver} "
                f"(expected: {PYTHON_VERSION})"
            )
        # Build Python interpreter is used for regeneration and freezing.
        PYTHON_FOR_REGEN = with_build_python
        PYTHON_FOR_FREEZE = with_build_python
        v.PYTHON_FOR_BUILD = (
            "_PYTHON_PROJECT_BASE=$(abs_builddir) "
            "_PYTHON_HOST_PLATFORM=$(_PYTHON_HOST_PLATFORM) "
            "PYTHONPATH=$(srcdir)/Lib "
            "_PYTHON_SYSCONFIGDATA_NAME=_sysconfigdata_$(ABIFLAGS)_$(MACHDEP)_$(MULTIARCH) "
            "_PYTHON_SYSCONFIGDATA_PATH=$(shell test -f pybuilddir.txt && "
            "echo $(abs_builddir)/`cat pybuilddir.txt`) " + with_build_python
        )
        pyconf.result(with_build_python)
    else:
        if v.cross_compiling:
            pyconf.error("error: Cross compiling requires --with-build-python")
        v.PYTHON_FOR_BUILD = "./$(BUILDPYTHON) -E"
        PYTHON_FOR_FREEZE = "./_bootstrap_python"
    v.export("PYTHON_FOR_BUILD")

    pyconf.checking("Python for regen version")
    if pyconf.find_prog(PYTHON_FOR_REGEN):
        pyconf.result(pyconf.cmd_output([PYTHON_FOR_REGEN, "-V"]))
    else:
        pyconf.result("missing")

    v.export("PYTHON_FOR_FREEZE", PYTHON_FOR_FREEZE)

    conf_modules.setup_freeze_module(v)

    if with_build_python is None:
        PYTHON_FOR_REGEN = pyconf.check_progs(
            [
                f"python{PYTHON_VERSION}",
                "python3.15",
                "python3.14",
                "python3.13",
                "python3.12",
                "python3.11",
                "python3.10",
                "python3",
                "python",
            ],
            default="python3",
        )
    v.export("PYTHON_FOR_REGEN", PYTHON_FOR_REGEN)


def setup_prefix_and_dirs(v):
    """Set prefix and standard autoconf directory variables."""
    v.prefix = pyconf.get_dir_arg("prefix", "/usr/local")
    if not isinstance(v.prefix, str):
        v.prefix = "/usr/local"
    # Ensure that if prefix is specified, it does not end in a slash. If
    # it does, we get path names containing '//' which is both ugly and
    # can cause trouble. Last slash shouldn't be stripped if prefix=/
    if v.prefix != "/" and v.prefix.endswith("/"):
        v.prefix = v.prefix.rstrip("/")
    v.export("prefix")

    v.export("exec_prefix", pyconf.get_dir_arg("exec_prefix", "${prefix}"))
    v.export(
        "datarootdir", pyconf.get_dir_arg("datarootdir", "${prefix}/share")
    )
    v.export("bindir", pyconf.get_dir_arg("bindir", "${exec_prefix}/bin"))
    v.export("libdir", pyconf.get_dir_arg("libdir", "${exec_prefix}/lib"))
    v.export("mandir", pyconf.get_dir_arg("mandir", "${datarootdir}/man"))
    v.export(
        "includedir", pyconf.get_dir_arg("includedir", "${prefix}/include")
    )
    # Also set uppercase aliases for use in configure.py code
    v.export("BINDIR", pyconf.get_dir_arg("bindir", "${exec_prefix}/bin"))
    v.export("LIBDIR", pyconf.get_dir_arg("libdir", "${exec_prefix}/lib"))
    v.export("MANDIR", pyconf.get_dir_arg("mandir", "${datarootdir}/man"))
    v.export(
        "INCLUDEDIR", pyconf.get_dir_arg("includedir", "${prefix}/include")
    )
    v.export("VPATH", v.srcdir)


def setup_version_and_config_args(v):
    """Set VERSION, SOVERSION, and CONFIG_ARGS."""
    v.export("VERSION", PYTHON_VERSION)
    v.export("SOVERSION", "1.0")
    # Value is computed at output time by _resolve_exports() so that
    # precious variables registered via env_var() in later files are included.
    v.export("CONFIG_ARGS", "")


def setup_pkg_config(v):
    """Handle --with-pkg-config option."""
    pkgconf_raw = WITH_PKG_CONFIG.value_or("check")

    if pkgconf_raw in ("yes", "check"):
        v.PKG_CONFIG = pyconf.find_prog("pkg-config")
    elif pkgconf_raw == "no":
        v.PKG_CONFIG = ""
    else:
        pyconf.error(
            f"error: invalid argument --with-pkg-config={pkgconf_raw}"
        )

    if pkgconf_raw == "yes" and not v.PKG_CONFIG:
        pyconf.error("error: pkg-config is required")
    pyconf.env["PKG_CONFIG"] = v.PKG_CONFIG


def setup_missing_stdlib_config(v):
    """Handle --with-missing-stdlib-config option."""
    msc_raw = WITH_MISSING_STDLIB_CONFIG.value_or("")
    v.export("MISSING_STDLIB_CONFIG", msc_raw)
