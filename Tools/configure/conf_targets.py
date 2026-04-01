"""conf_targets — Android, WASM, shared options, library names, ldlibrary.

Checks --no-as-needed linker flag and detects Android API level;
handles --enable-wasm-dynamic-linking and --enable-wasm-pthreads;
determines executable suffix (--with-suffix, BUILDEXEEXT); sets
LIBRARY, LDLIBRARY, BLDLIBRARY, LINKCC, EXPORTSYMS, GNULD; handles
--enable-shared, --with-static-libpython, --enable-profiling; computes
final LDLIBRARY/RUNSHARED for each platform; sets HOSTRUNNER for
cross-compile targets; and configures AR, ARFLAGS, INSTALL, ABIFLAGS.
"""

from __future__ import annotations

import os

import pyconf

WITH_SUFFIX = pyconf.arg_with(
    "suffix",
    metavar="SUFFIX",
    help="set executable suffix (default empty; 'yes' means '.exe')",
)
ENABLE_SHARED = pyconf.arg_enable(
    "shared",
    help="enable building a shared Python library (default is no)",
)
WITH_STATIC_LIBPYTHON = pyconf.arg_with(
    "static-libpython",
    help="do not build libpythonMAJOR.MINOR.a and do not install python.o (default is yes)",
)
ENABLE_PROFILING = pyconf.arg_enable(
    "profiling",
    help="enable C-level code profiling with gprof (default is no)",
)


def _runshared(envvar: str) -> str:
    """Build a RUNSHARED value the way autoconf does at configure time.

    Autoconf expands `pwd` and ${VAR:+:${VAR}} in the shell, so the
    Makefile gets the resolved build directory and any existing env-var
    value appended.
    """
    cwd = pyconf.getcwd()
    existing = os.environ.get(envvar, "")
    if existing:
        return f"{envvar}={cwd}:{existing}"
    return f"{envvar}={cwd}"


def setup_android_api(v):
    """Check --no-as-needed linker flag and detect Android API level."""
    v.export(
        "NO_AS_NEEDED",
        pyconf.check_linker_flag(
            "-Wl,--no-as-needed", value="-Wl,--no-as-needed"
        ),
    )

    pyconf.checking("for the Android API level")
    android_out = pyconf.run(
        "$CPP $CPPFLAGS -",
        vars={"CPP": v.CPP, "CPPFLAGS": v.CPPFLAGS},
        input=(
            "#ifdef __ANDROID__\n"
            "android_api = __ANDROID_API__\n"
            "arm_arch = __ARM_ARCH\n"
            "#else\n"
            "#error not Android\n"
            "#endif\n"
        ),
        capture_output=True,
        text=True,
    )
    v.ANDROID_API_LEVEL = ""
    arm_arch = ""
    if android_out.returncode == 0:
        for line in android_out.stdout.splitlines():
            if not line.startswith("#"):
                if line.startswith("android_api = "):
                    v.ANDROID_API_LEVEL = line.removeprefix(
                        "android_api = "
                    ).strip()
                elif line.startswith("arm_arch = "):
                    arm_arch = line.removeprefix("arm_arch = ").strip()
        pyconf.result(v.ANDROID_API_LEVEL)
        if not v.ANDROID_API_LEVEL:
            pyconf.error("Fatal: you must define __ANDROID_API__")
        pyconf.define_unquoted(
            "ANDROID_API_LEVEL", v.ANDROID_API_LEVEL, "The Android API level."
        )
        # For __android_log_write() in Python/pylifecycle.c.
        v.LIBS += " -llog"
        pyconf.checking("for the Android arm ABI")
        pyconf.result(arm_arch)
        if arm_arch == "7":
            v.BASECFLAGS += " -mfloat-abi=softfp -mfpu=vfpv3-d16"
            v.LDFLAGS += " -march=armv7-a -Wl,--fix-cortex-a8"
    else:
        pyconf.result("not Android")
    v.export("ANDROID_API_LEVEL")


def setup_exe_suffix(v):
    """Handle --with-suffix, BUILDEXEEXT, and HP-UX CC flag."""
    pyconf.checking("for --with-suffix")
    if WITH_SUFFIX.given:
        if WITH_SUFFIX.is_no():
            v.EXEEXT = ""
        elif WITH_SUFFIX.is_yes():
            v.EXEEXT = ".exe"
        else:
            v.EXEEXT = WITH_SUFFIX.value_or("")
    else:
        if v.ac_sys_system == "Emscripten":
            v.EXEEXT = ".mjs"
        elif v.ac_sys_system == "WASI":
            v.EXEEXT = ".wasm"
        else:
            v.EXEEXT = ""
    pyconf.result(v.EXEEXT)
    v.export("EXEEXT")

    v.ac_exeext = v.EXEEXT

    # --- BUILDEXEEXT ---
    v.export("BUILDEXEEXT")
    # Test whether we're running on a non-case-sensitive system
    pyconf.checking("for case-insensitive build directory")
    casedir = "CaseSensitiveTestDir"
    pyconf.mkdir_p(casedir)
    if pyconf.path_is_dir("casesensitivetestdir") and not v.EXEEXT:
        pyconf.result("yes")
        v.BUILDEXEEXT = ".exe"
    else:
        pyconf.result("no")
        v.BUILDEXEEXT = v.EXEEXT
    pyconf.rmdir(casedir)

    # HP-UX: append -Ae to CC
    if v.ac_sys_system.startswith(("hp", "HP")) and v.ac_cv_cc_name in ("cc",):
        v.CC += " -Ae"


def setup_library_names(v):
    """Set LIBRARY, LDLIBRARY/BLDLIBRARY init, LINKCC, EXPORTSYMS, GNULD."""
    # LDLIBRARY is the name of the library to link against (as opposed to the
    # name of the library into which to insert object files). BLDLIBRARY is also
    # the library to link against, usually. On Mac OS X frameworks, BLDLIBRARY
    # is blank as the main program is not linked directly against LDLIBRARY.
    # LDLIBRARYDIR is the path to LDLIBRARY, which is made in a subdirectory. On
    # systems without shared libraries, LDLIBRARY is the same as LIBRARY
    # (defined in the Makefiles). On Cygwin LDLIBRARY is the import library,
    # DLLLIBRARY is the shared (i.e., DLL) library.
    #
    # RUNSHARED is used to run shared python without installed libraries
    #
    # INSTSONAME is the name of the shared library that will be use to install
    # on the system - some systems like version suffix, others don't
    #
    # LDVERSION is the shared library version number, normally the Python version
    # with the ABI build flags appended.
    v.export("LIBRARY")
    pyconf.checking("LIBRARY")
    v.LIBRARY = v.LIBRARY or "libpython$(VERSION)$(ABIFLAGS).a"
    pyconf.result(v.LIBRARY)

    # --- LDLIBRARY initial values ---
    v.export("LDLIBRARY")
    v.export("DLLLIBRARY")
    v.export("BLDLIBRARY")
    v.export("PY3LIBRARY")
    v.export("LDLIBRARYDIR")
    v.export("INSTSONAME")
    v.export("RUNSHARED")
    v.export("LDVERSION")

    v.LDLIBRARY = v.LIBRARY
    v.BLDLIBRARY = "$(LDLIBRARY)"
    v.INSTSONAME = "$(LDLIBRARY)"
    v.DLLLIBRARY = ""
    v.LDLIBRARYDIR = ""
    v.RUNSHARED = ""
    v.LDVERSION = v.VERSION
    v.PY3LIBRARY = ""

    # --- LINKCC ---
    # LINKCC is the command that links the python executable -- default is $(CC).
    # If CXX is set, and if it is needed to link a main function that was
    # compiled with CXX, LINKCC is CXX instead. Always using CXX is undesirable:
    # python might then depend on the C++ runtime
    v.export("LINKCC")
    pyconf.checking("LINKCC")
    if not v.LINKCC:
        v.LINKCC = "$(PURIFY) $(CC)"
        if v.ac_sys_system.startswith("QNX"):
            v.LINKCC = "qcc"
    pyconf.result(v.LINKCC)

    # --- EXPORTSYMS / EXPORTSFROM ---
    # EXPORTSYMS holds the list of exported symbols for AIX.
    # EXPORTSFROM holds the module name exporting symbols on AIX.
    v.export("EXPORTSYMS", "")
    v.export("EXPORTSFROM", "")
    pyconf.checking("EXPORTSYMS")
    if v.ac_sys_system.startswith("AIX"):
        v.EXPORTSYMS = "Modules/python.exp"
        v.EXPORTSFROM = "."
    pyconf.result(v.EXPORTSYMS)

    # --- GNULD: detect GNU linker ---
    # GNULD is set to "yes" if the GNU linker is used.  If this goes wrong
    # make sure we default having it set to "no": this is used by
    # distutils.unixccompiler to know if it should add --enable-new-dtags
    # to linker command lines, and failing to detect GNU ld simply results
    # in the same behaviour as before.
    v.export("GNULD")
    pyconf.checking("for GNU ld")
    ld_prog = "ld"
    if v.ac_cv_cc_name == "gcc":
        ld_prog = pyconf.cmd_output([v.CC, "-print-prog-name=ld"])
    _, ld_ver = pyconf.cmd_status([ld_prog, "-V"])
    v.GNULD = "GNU" in ld_ver
    pyconf.result(v.GNULD)


def setup_shared_options(v):
    """Handle --enable-shared, --with-static-libpython, --enable-profiling."""
    pyconf.checking("for --enable-shared")
    if ENABLE_SHARED.given:
        v.enable_shared = not ENABLE_SHARED.is_no()
    else:
        v.enable_shared = v.ac_sys_system.startswith("CYGWIN")
    pyconf.result(v.enable_shared)

    v.STATIC_LIBPYTHON = 1
    pyconf.checking("for --with-static-libpython")
    if WITH_STATIC_LIBPYTHON.is_no():
        pyconf.result("no")
        v.STATIC_LIBPYTHON = 0
    else:
        pyconf.result("yes")
    v.export("STATIC_LIBPYTHON")

    pyconf.checking("for --enable-profiling")
    v.enable_profiling = (
        ENABLE_PROFILING.given and not ENABLE_PROFILING.is_no()
    )
    if v.enable_profiling:
        prof_ok = pyconf.try_link(
            "int main(void) { return 0; }", extra_flags=["-pg"]
        )
        if not prof_ok:
            v.enable_profiling = False
    pyconf.result(v.enable_profiling)

    if v.enable_profiling:
        v.BASECFLAGS = f"-pg {v.BASECFLAGS}"
        v.LDFLAGS = f"-pg {v.LDFLAGS}"


def setup_ldlibrary(v):
    """Compute final LDLIBRARY, BLDLIBRARY, RUNSHARED values."""
    pyconf.checking("LDLIBRARY")

    # Apple framework builds need more magic. LDLIBRARY is the dynamic
    # library that we build, but we do not want to link against it (we
    # will find it with a -framework option). For this reason there is an
    # extra variable BLDLIBRARY against which Python and the extension
    # modules are linked, BLDLIBRARY. This is normally the same as
    # LDLIBRARY, but empty for MacOSX framework builds. iOS does the same,
    # but uses a non-versioned framework layout.
    if v.enable_framework:
        if v.ac_sys_system == "Darwin":
            v.LDLIBRARY = (
                "$(PYTHONFRAMEWORKDIR)/Versions/$(VERSION)/$(PYTHONFRAMEWORK)"
            )
        elif v.ac_sys_system == "iOS":
            v.LDLIBRARY = "$(PYTHONFRAMEWORKDIR)/$(PYTHONFRAMEWORK)"
        else:
            pyconf.error("Unknown platform for framework build")
        v.BLDLIBRARY = ""
        v.RUNSHARED = _runshared("DYLD_FRAMEWORK_PATH")
    else:
        v.BLDLIBRARY = "$(LDLIBRARY)"

    v.export("PY_ENABLE_SHARED", 0)

    # Other platforms follow
    if v.enable_shared:
        v.PY_ENABLE_SHARED = 1
        pyconf.define(
            "Py_ENABLE_SHARED",
            1,
            "Defined if Python is built as a shared library.",
        )
        _setup_ldlibrary_shared(v)
    else:
        if v.ac_sys_system.startswith("CYGWIN"):
            v.BLDLIBRARY = "$(LIBRARY)"
            v.LDLIBRARY = "libpython$(LDVERSION).dll.a"

    pyconf.result(v.LDLIBRARY)


def _setup_ldlibrary_shared(v):
    """Set LDLIBRARY/BLDLIBRARY/RUNSHARED for --enable-shared builds."""
    if v.ac_sys_system.startswith("CYGWIN"):
        v.LDLIBRARY = "libpython$(LDVERSION).dll.a"
        v.BLDLIBRARY = "-L. -lpython$(LDVERSION)"
        v.DLLLIBRARY = "libpython$(LDVERSION).dll"
    elif v.ac_sys_system.startswith("SunOS"):
        v.LDLIBRARY = "libpython$(LDVERSION).so"
        v.BLDLIBRARY = "-Wl,-R,$(LIBDIR) -L. -lpython$(LDVERSION)"
        v.RUNSHARED = _runshared("LD_LIBRARY_PATH")
        v.INSTSONAME = f"{v.LDLIBRARY}.{v.SOVERSION}"
        if not v.Py_DEBUG:
            v.PY3LIBRARY = "libpython3.so"
    elif v.ac_sys_system.startswith(
        (
            "Linux",
            "GNU",
            "NetBSD",
            "FreeBSD",
            "DragonFly",
            "OpenBSD",
            "VxWorks",
        )
    ):
        v.LDLIBRARY = "libpython$(LDVERSION).so"
        v.BLDLIBRARY = "-L. -lpython$(LDVERSION)"
        v.RUNSHARED = _runshared("LD_LIBRARY_PATH")
        # The Android Gradle plugin will only package libraries whose names end
        # with ".so".
        if v.ac_sys_system != "Linux-android":
            v.INSTSONAME = f"{v.LDLIBRARY}.{v.SOVERSION}"
        if not v.Py_DEBUG:
            v.PY3LIBRARY = "libpython3.so"
    elif v.ac_sys_system.startswith(("hp", "HP")):
        mach = pyconf.cmd_output(["uname", "-m"])
        v.LDLIBRARY = (
            "libpython$(LDVERSION).so"
            if mach == "ia64"
            else "libpython$(LDVERSION).sl"
        )
        v.BLDLIBRARY = "-Wl,+b,$(LIBDIR) -L. -lpython$(LDVERSION)"
        v.RUNSHARED = _runshared("SHLIB_PATH")
    elif v.ac_sys_system.startswith("Darwin"):
        v.LDLIBRARY = "libpython$(LDVERSION).dylib"
        v.BLDLIBRARY = "-L. -lpython$(LDVERSION)"
        v.RUNSHARED = _runshared("DYLD_LIBRARY_PATH")
    elif v.ac_sys_system == "iOS":
        v.LDLIBRARY = "libpython$(LDVERSION).dylib"
    elif v.ac_sys_system.startswith("AIX"):
        v.LDLIBRARY = "libpython$(LDVERSION).so"
        v.RUNSHARED = _runshared("LIBPATH")


def setup_hostrunner(v):
    """Detect HOSTRUNNER for cross-compile targets."""
    pyconf.checking("HOSTRUNNER")
    if not v.HOSTRUNNER:
        if v.ac_sys_system == "Emscripten":
            NODE = pyconf.check_prog("node", default="node")
            v.HOSTRUNNER = NODE
            if v.host_cpu == "wasm64":
                v.HOSTRUNNER += " --experimental-wasm-memory64"
        elif v.ac_sys_system == "WASI":
            pyconf.error("HOSTRUNNER must be set when cross-compiling to WASI")
        else:
            v.HOSTRUNNER = ""
    v.export("HOSTRUNNER")
    pyconf.result(v.HOSTRUNNER)

    if v.HOSTRUNNER:
        v.PYTHON_FOR_BUILD = (
            f"_PYTHON_HOSTRUNNER='{v.HOSTRUNNER}' {v.PYTHON_FOR_BUILD}"
        )


def setup_library_deps(v):
    """Set LIBRARY_DEPS, LINK_PYTHON_DEPS, LINK_PYTHON_OBJS."""
    v.LIBRARY_DEPS = "$(PY3LIBRARY) $(EXPORTSYMS)"
    v.LINK_PYTHON_DEPS = "$(LIBRARY_DEPS)"

    if v.PY_ENABLE_SHARED == 1 or v.enable_framework:
        v.LIBRARY_DEPS = f"$(LDLIBRARY) {v.LIBRARY_DEPS}"
        if v.STATIC_LIBPYTHON == 1:
            v.LIBRARY_DEPS = f"$(LIBRARY) {v.LIBRARY_DEPS}"
        # Link Python program to the shared library
        v.LINK_PYTHON_OBJS = "$(BLDLIBRARY)"
    else:
        if v.STATIC_LIBPYTHON == 0:
            # Build Python needs object files but don't need to build
            # Python static library
            v.LINK_PYTHON_DEPS = f"{v.LIBRARY_DEPS} $(LIBRARY_OBJS)"
        v.LIBRARY_DEPS = f"$(LIBRARY) {v.LIBRARY_DEPS}"
        # Link Python program to object files
        v.LINK_PYTHON_OBJS = "$(LIBRARY_OBJS)"

    v.export("LIBRARY_DEPS")
    v.export("LINK_PYTHON_DEPS")
    v.export("LINK_PYTHON_OBJS")


def setup_build_tools(v):
    """Set AR, ARFLAGS, INSTALL, MKDIR_P, LN, ABIFLAGS."""
    # ar program
    v.export("AR", v.AR or pyconf.check_tools(["ar", "aal"], default="ar"))
    # tweak ARFLAGS only if the user didn't set it on the command line
    v.export("ARFLAGS", v.ARFLAGS or "rcs")

    # HP-UX INSTALL fallback: install -d does not work on HP-UX
    if v.MACHDEP.startswith(("hp", "HP")):
        if not os.environ.get("INSTALL"):
            os.environ["INSTALL"] = f"{v.srcdir}/install-sh -c"
    pyconf.find_install()
    pyconf.find_mkdir_p()
    v.export("MKDIR_P")

    # Not every filesystem supports hard links
    v.export(
        "LN",
        v.LN or ("ln -s" if v.ac_sys_system.startswith("CYGWIN") else "ln"),
    )

    # For calculating the .so ABI tag.
    # ABIFLAGS / ABI_THREAD: initialized empty; filled in by Part 4
    v.export("ABIFLAGS")
    v.export("ABI_THREAD")
