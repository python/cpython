"""conf_modules — Stdlib module definitions, HACL.

Sets up platform-specific not-available module lists; declares
MODULE_BUILDTYPE for ~60 stdlib extension modules (always-builtin,
shared, disabled, n/a); configures HACL* cryptographic library
flags (libHacl_Hash_SHA2, etc.); and sets up remaining module
variables (LIBHACL_*, MODULE_*_LDFLAGS).
"""

from __future__ import annotations

import pyconf


def setup_module_deps(v):
    # ---------------------------------------------------------------------------
    # MODULE_DEPS_SHARED / LIBPYTHON
    # ---------------------------------------------------------------------------

    v.export("MODULE_DEPS_SHARED")
    v.export("LIBPYTHON")
    v.MODULE_DEPS_SHARED = "$(MODULE_DEPS_STATIC) $(EXPORTSYMS)"
    v.LIBPYTHON = ""

    android_api_level = v.ANDROID_API_LEVEL
    if v.PY_ENABLE_SHARED == 1 and (
        android_api_level or v.MACHDEP == "cygwin"
    ):
        v.MODULE_DEPS_SHARED = f"{v.MODULE_DEPS_SHARED} $(LDLIBRARY)"
        v.LIBPYTHON = r"$(BLDLIBRARY)"

    if v.ac_sys_system == "iOS":
        v.MODULE_DEPS_SHARED = (
            f"{v.MODULE_DEPS_SHARED} $(PYTHONFRAMEWORKDIR)/$(PYTHONFRAMEWORK)"
        )


def setup_freeze_module(v):
    """Set FREEZE_MODULE_BOOTSTRAP, FREEZE_MODULE, and related variables."""
    if v.cross_compiling:
        FREEZE_MODULE_BOOTSTRAP = (
            "$(PYTHON_FOR_FREEZE) $(srcdir)/Programs/_freeze_module.py"
        )
        FREEZE_MODULE_BOOTSTRAP_DEPS = "$(srcdir)/Programs/_freeze_module.py"
        FREEZE_MODULE = "$(FREEZE_MODULE_BOOTSTRAP)"
        FREEZE_MODULE_DEPS = "$(FREEZE_MODULE_BOOTSTRAP_DEPS)"
        PYTHON_FOR_BUILD_DEPS = ""
    else:
        FREEZE_MODULE_BOOTSTRAP = "./Programs/_freeze_module"
        FREEZE_MODULE_BOOTSTRAP_DEPS = "Programs/_freeze_module"
        FREEZE_MODULE = (
            "$(PYTHON_FOR_FREEZE) $(srcdir)/Programs/_freeze_module.py"
        )
        FREEZE_MODULE_DEPS = (
            "_bootstrap_python $(srcdir)/Programs/_freeze_module.py"
        )
        PYTHON_FOR_BUILD_DEPS = "$(BUILDPYTHON)"
    v.export("FREEZE_MODULE_BOOTSTRAP", FREEZE_MODULE_BOOTSTRAP)
    v.export("FREEZE_MODULE_BOOTSTRAP_DEPS", FREEZE_MODULE_BOOTSTRAP_DEPS)
    v.export("FREEZE_MODULE", FREEZE_MODULE)
    v.export("FREEZE_MODULE_DEPS", FREEZE_MODULE_DEPS)
    v.export("PYTHON_FOR_BUILD_DEPS", PYTHON_FOR_BUILD_DEPS)


def setup_platform_na_modules(v):
    # ---------------------------------------------------------------------------
    # Platform module n/a lists  (PY_STDLIB_MOD_SET_NA)
    # ---------------------------------------------------------------------------

    sys = v.ac_sys_system

    if sys == "AIX":
        pyconf.stdlib_module_set_na(["_scproxy"])
    elif sys.startswith("VxWorks"):
        pyconf.stdlib_module_set_na(["_scproxy", "termios", "grp"])
    elif sys == "Darwin":
        pass  # _scproxy is available on macOS
    elif sys == "iOS":
        pyconf.stdlib_module_set_na(
            [
                "_curses",
                "_curses_panel",
                "_gdbm",
                "_multiprocessing",
                "_posixshmem",
                "_posixsubprocess",
                "_scproxy",
                "_tkinter",
                "grp",
                "nis",
                "readline",
                "pwd",
                "spwd",
                "syslog",
            ]
        )
    elif (
        sys.startswith("CYGWIN")
        or sys.startswith("QNX")
        or sys.startswith("FreeBSD")
    ):
        pyconf.stdlib_module_set_na(["_scproxy"])
    elif sys == "Emscripten":
        pyconf.stdlib_module_set_na(
            [
                "_curses",
                "_curses_panel",
                "_dbm",
                "_gdbm",
                "_multiprocessing",
                "_posixshmem",
                "_posixsubprocess",
                "_scproxy",
                "_tkinter",
                "_interpreters",
                "_interpchannels",
                "_interpqueues",
                "grp",
                "pwd",
                "resource",
                "syslog",
                "readline",
            ]
        )
    elif sys == "WASI":
        pyconf.stdlib_module_set_na(
            [
                "_curses",
                "_curses_panel",
                "_dbm",
                "_gdbm",
                "_multiprocessing",
                "_posixshmem",
                "_posixsubprocess",
                "_scproxy",
                "_tkinter",
                "_interpreters",
                "_interpchannels",
                "_interpqueues",
                "grp",
                "pwd",
                "resource",
                "syslog",
            ]
        )
        pyconf.stdlib_module_set_na(
            [
                "_ctypes_test",
                "_remote_debugging",
                "_testimportmultiple",
                "_testmultiphase",
                "_testsinglephase",
                "fcntl",
                "mmap",
                "termios",
                "xxlimited",
                "xxlimited_35",
            ]
        )
    else:
        pyconf.stdlib_module_set_na(["_scproxy"])

    # ---------------------------------------------------------------------------
    # MODULE_BUILDTYPE
    # ---------------------------------------------------------------------------

    host_cpu = v.host_cpu
    if host_cpu in ("wasm32", "wasm64"):
        v.export("MODULE_BUILDTYPE", "static")
    else:
        v.export("MODULE_BUILDTYPE", v.MODULE_BUILDTYPE or "shared")


def setup_stdlib_modules(v):
    # ---------------------------------------------------------------------------
    # Stdlib extension modules
    # ---------------------------------------------------------------------------

    # Bootstrap modules (Modules/Setup.bootstrap)
    pyconf.stdlib_module_simple("_io", cflags="-I$(srcdir)/Modules/_io")
    pyconf.stdlib_module_simple("time", ldflags=v.TIMEMODULE_LIB)

    # Always-enabled
    pyconf.stdlib_module_simple("array")
    pyconf.stdlib_module_simple("_math_integer")
    pyconf.stdlib_module_simple("_asyncio")
    pyconf.stdlib_module_simple("_bisect")
    pyconf.stdlib_module_simple("_csv")
    pyconf.stdlib_module_simple("_heapq")
    pyconf.stdlib_module_simple("_json")
    pyconf.stdlib_module_simple("_lsprof")
    pyconf.stdlib_module_simple("_pickle")
    pyconf.stdlib_module_simple("_posixsubprocess")
    pyconf.stdlib_module_simple("_queue")
    pyconf.stdlib_module_simple("_random")
    pyconf.stdlib_module_simple(
        "_remote_debugging",
        cflags=v.REMOTE_DEBUGGING_CFLAGS,
        ldflags=v.REMOTE_DEBUGGING_LIBS,
    )
    pyconf.stdlib_module_simple("select")
    pyconf.stdlib_module_simple("_struct")
    pyconf.stdlib_module_simple("_types")
    pyconf.stdlib_module_simple("_typing")
    pyconf.stdlib_module_simple("_interpreters")
    pyconf.stdlib_module_simple("_interpchannels")
    pyconf.stdlib_module_simple("_interpqueues")
    pyconf.stdlib_module_simple("_zoneinfo")

    # multiprocessing
    pyconf.stdlib_module(
        "_multiprocessing",
        supported=v.ac_cv_func_sem_unlink is True,
        cflags="-I$(srcdir)/Modules/_multiprocessing",
    )
    pyconf.stdlib_module(
        "_posixshmem",
        supported=bool(v.have_posix_shmem),
        cflags=v.POSIXSHMEM_CFLAGS,
        ldflags=v.POSIXSHMEM_LIBS,
    )

    # libm-dependent
    pyconf.stdlib_module_simple("_statistics", ldflags=v.LIBM)
    pyconf.stdlib_module_simple("cmath", ldflags=v.LIBM)
    pyconf.stdlib_module_simple("math", ldflags=v.LIBM)
    pyconf.stdlib_module_simple(
        "_datetime",
        ldflags=f"{v.TIMEMODULE_LIB} {v.LIBM}".strip(),
    )

    # Unix modules with platform dependencies
    pyconf.stdlib_module(
        "fcntl",
        supported=(
            v.ac_cv_header_sys_ioctl_h is True
            and v.ac_cv_header_fcntl_h is True
        ),
        ldflags=v.FCNTL_LIBS,
    )
    pyconf.stdlib_module(
        "mmap",
        supported=(
            v.ac_cv_header_sys_mman_h is True
            and v.ac_cv_header_sys_stat_h is True
        ),
    )
    pyconf.stdlib_module(
        "_socket",
        supported=(
            v.ac_cv_header_sys_socket_h is True
            and v.ac_cv_header_sys_types_h is True
            and v.ac_cv_header_netinet_in_h is True
        ),
        ldflags=v.SOCKET_LIBS,
    )

    # Platform-specific
    pyconf.stdlib_module(
        "grp",
        supported=(
            v.ac_cv_func_getgrent is True
            and (
                v.ac_cv_func_getgrgid is True
                or v.ac_cv_func_getgrgid_r is True
            )
        ),
    )
    pyconf.stdlib_module(
        "pwd",
        supported=(
            v.ac_cv_func_getpwuid is True or v.ac_cv_func_getpwuid_r is True
        ),
    )
    pyconf.stdlib_module(
        "resource", supported=v.ac_cv_header_sys_resource_h is True
    )
    pyconf.stdlib_module(
        "_scproxy",
        enabled=v.ac_sys_system == "Darwin",
        ldflags="-framework SystemConfiguration -framework CoreFoundation",
    )
    pyconf.stdlib_module("syslog", supported=v.ac_cv_header_syslog_h is True)
    pyconf.stdlib_module("termios", supported=v.ac_cv_header_termios_h is True)

    # expat / elementtree
    pyconf.stdlib_module(
        "pyexpat",
        supported=v.ac_cv_header_sys_time_h is True,
        cflags=v.LIBEXPAT_CFLAGS,
        ldflags=v.LIBEXPAT_LDFLAGS,
    )
    pyconf.stdlib_module("_elementtree", cflags=v.LIBEXPAT_CFLAGS)

    # CJK codecs
    pyconf.stdlib_module_simple("_codecs_cn")
    pyconf.stdlib_module_simple("_codecs_hk")
    pyconf.stdlib_module_simple("_codecs_iso2022")
    pyconf.stdlib_module_simple("_codecs_jp")
    pyconf.stdlib_module_simple("_codecs_kr")
    pyconf.stdlib_module_simple("_codecs_tw")
    pyconf.stdlib_module_simple("_multibytecodec")
    pyconf.stdlib_module_simple("unicodedata")


def setup_remaining_modules(v):
    # ---------------------------------------------------------------------------
    # Remaining conditionally-enabled extension modules
    # ---------------------------------------------------------------------------

    pyconf.stdlib_module(
        "_ctypes",
        supported=v.have_libffi is True,
        cflags=f"{v.NO_STRICT_OVERFLOW_CFLAGS} {v.LIBFFI_CFLAGS}".strip(),
        ldflags=v.LIBFFI_LIBS,
    )
    pyconf.stdlib_module(
        "_curses",
        supported=v.have_curses is True,
        cflags=v.CURSES_CFLAGS,
        ldflags=v.CURSES_LIBS,
    )
    pyconf.stdlib_module(
        "_curses_panel",
        supported=(v.have_curses is True and v.have_panel is True),
        cflags=f"{v.PANEL_CFLAGS} {v.CURSES_CFLAGS}".strip(),
        ldflags=f"{v.PANEL_LIBS} {v.CURSES_LIBS}".strip(),
    )
    pyconf.stdlib_module(
        "_decimal",
        supported=v.have_mpdec is True,
        cflags=v.LIBMPDEC_CFLAGS,
        ldflags=v.LIBMPDEC_LIBS,
    )

    if v.with_system_libmpdec is False:
        pyconf.warn(
            "the bundled copy of libmpdec is scheduled for removal in Python 3.16; "
            "consider using a system installed mpdecimal library."
        )
    if v.with_system_libmpdec is True and v.have_mpdec is False:
        pyconf.warn(
            "no system libmpdec found; falling back to pure-Python version "
            "for the decimal module"
        )

    pyconf.stdlib_module(
        "_dbm",
        enabled=bool(v.with_dbmliborder),
        supported=v.have_dbm is not False,
        cflags=v.DBM_CFLAGS,
        ldflags=v.DBM_LIBS,
    )
    pyconf.stdlib_module(
        "_gdbm",
        enabled="gdbm" in v.with_dbmliborder.split(":"),
        supported=v.have_gdbm is True,
        cflags=v.GDBM_CFLAGS,
        ldflags=v.GDBM_LIBS,
    )
    pyconf.stdlib_module(
        "readline",
        supported=v.with_readline is not False,
        cflags=v.READLINE_CFLAGS,
        ldflags=v.READLINE_LIBS,
    )
    pyconf.stdlib_module(
        "_sqlite3",
        enabled=v.have_sqlite3 is True,
        supported=v.have_supported_sqlite3 is True,
        cflags=v.LIBSQLITE3_CFLAGS,
        ldflags=v.LIBSQLITE3_LIBS,
    )
    pyconf.stdlib_module(
        "_tkinter",
        supported=v.have_tcltk is True,
        cflags=v.TCLTK_CFLAGS,
        ldflags=v.TCLTK_LIBS,
    )
    pyconf.stdlib_module(
        "_uuid",
        supported=v.have_uuid is True,
        cflags=v.LIBUUID_CFLAGS,
        ldflags=v.LIBUUID_LIBS,
    )

    # Compression
    pyconf.stdlib_module(
        "zlib",
        supported=v.have_zlib is True,
        cflags=v.ZLIB_CFLAGS,
        ldflags=v.ZLIB_LIBS,
    )
    pyconf.stdlib_module_simple(
        "binascii",
        cflags=v.BINASCII_CFLAGS,
        ldflags=v.BINASCII_LIBS,
    )
    pyconf.stdlib_module(
        "_bz2",
        supported=v.have_bzip2 is True,
        cflags=v.BZIP2_CFLAGS,
        ldflags=v.BZIP2_LIBS,
    )
    pyconf.stdlib_module(
        "_lzma",
        supported=v.have_liblzma is True,
        cflags=v.LIBLZMA_CFLAGS,
        ldflags=v.LIBLZMA_LIBS,
    )
    pyconf.stdlib_module(
        "_zstd",
        supported=v.have_libzstd is True,
        cflags=v.LIBZSTD_CFLAGS,
        ldflags=v.LIBZSTD_LIBS,
    )

    # OpenSSL bindings
    openssl_cflags = v.OPENSSL_INCLUDES
    openssl_ldflags_common = (
        f"{v.OPENSSL_LDFLAGS} {v.OPENSSL_LDFLAGS_RPATH}".strip()
    )
    pyconf.stdlib_module(
        "_ssl",
        supported=v.ac_cv_working_openssl_ssl is True,
        cflags=openssl_cflags,
        ldflags=f"{openssl_ldflags_common} {v.OPENSSL_LIBS}".strip(),
    )
    pyconf.stdlib_module(
        "_hashlib",
        supported=v.ac_cv_working_openssl_hashlib is True,
        cflags=openssl_cflags,
        ldflags=f"{openssl_ldflags_common} {v.LIBCRYPTO_LIBS}".strip(),
    )

    # Test modules
    pyconf.stdlib_module(
        "_testcapi",
        enabled=v.TEST_MODULES,
        ldflags=v.LIBATOMIC,
    )
    pyconf.stdlib_module("_testclinic", enabled=v.TEST_MODULES)
    pyconf.stdlib_module("_testclinic_limited", enabled=v.TEST_MODULES)
    pyconf.stdlib_module("_testlimitedcapi", enabled=v.TEST_MODULES)
    pyconf.stdlib_module("_testinternalcapi", enabled=v.TEST_MODULES)
    pyconf.stdlib_module("_testbuffer", enabled=v.TEST_MODULES)
    pyconf.stdlib_module(
        "_testimportmultiple",
        enabled=v.TEST_MODULES,
        supported=v.ac_cv_func_dlopen is True,
    )
    pyconf.stdlib_module(
        "_testmultiphase",
        enabled=v.TEST_MODULES,
        supported=v.ac_cv_func_dlopen is True,
    )
    pyconf.stdlib_module(
        "_testsinglephase",
        enabled=v.TEST_MODULES,
        supported=v.ac_cv_func_dlopen is True,
    )
    pyconf.stdlib_module("xxsubtype", enabled=v.TEST_MODULES)
    pyconf.stdlib_module("_xxtestfuzz", enabled=v.TEST_MODULES)
    pyconf.stdlib_module(
        "_ctypes_test",
        enabled=v.TEST_MODULES,
        supported=(v.have_libffi is True and v.ac_cv_func_dlopen is True),
        cflags=v.LIBFFI_CFLAGS,
        ldflags=f"{v.LIBFFI_LIBS} {v.LIBM}".strip(),
    )
    pyconf.stdlib_module(
        "xxlimited",
        enabled=v.TEST_MODULES,
        supported=v.ac_cv_func_dlopen is True,
    )
    pyconf.stdlib_module(
        "xxlimited_35",
        enabled=v.TEST_MODULES,
        supported=v.ac_cv_func_dlopen is True,
    )

    # MODULE_BLOCK must be exported after the last stdlib_module call
    v.export("MODULE_BLOCK")


ENABLE_TEST_MODULES = pyconf.arg_enable(
    "test-modules", help="don't build nor install test modules"
)


def check_test_modules(v):
    # ---------------------------------------------------------------------------
    # --disable-test-modules
    # ---------------------------------------------------------------------------

    v.export("TEST_MODULES", not ENABLE_TEST_MODULES.is_no())
