"""conf_syslibs — GDBM, DBM, pthreads setup, base libraries, libatomic.

Detects GDBM library and headers; handles --with-dbmliborder and
detects DBM backends (gdbm, ndbm, bdb) in the specified order;
configures POSIX threads (THREADOBJ, thread CFLAGS/LDFLAGS,
PTHREAD_SYSTEM_SCHED_SUPPORTED); probes for sendfile, dl, dld
libraries and backtrace/dladdr1 support; checks sem_init, intl,
AIX-specific extensions, and aligned memory access; and probes
whether libatomic is needed for <pyatomic.h>.
"""

from __future__ import annotations

import pyconf

WITH_DBMLIBORDER = pyconf.arg_with(
    "dbmliborder",
    help="override order to check db backends for dbm (default is gdbm:ndbm:bdb)",
)


def detect_gdbm(v):
    """Detect gdbm library."""
    v.have_gdbm = False
    # NOTE: gdbm does not provide a pkgconf file.
    pyconf.env_var("GDBM_CFLAGS", "C compiler flags for gdbm")
    pyconf.env_var("GDBM_LIBS", "additional linker flags for gdbm")

    pyconf.checking("for gdbm.h")
    found = pyconf.check_header("gdbm.h", extra_cflags=v.GDBM_CFLAGS)
    pyconf.result(found)
    if found:
        if pyconf.check_lib(
            "gdbm",
            "gdbm_open",
            extra_cflags=v.GDBM_CFLAGS,
            extra_libs=v.GDBM_LIBS,
        ):
            v.have_gdbm = True
            v.GDBM_LIBS = v.GDBM_LIBS or "-lgdbm"
        else:
            v.have_gdbm = False
    else:
        v.have_gdbm = False

    v.export("GDBM_CFLAGS")
    v.export("GDBM_LIBS")


def detect_dbm(v):
    """Detect ndbm, gdbm_compat, libdb, and handle --with-dbmliborder."""
    v.have_ndbm = False
    v.dbm_ndbm = ""
    v.have_gdbm_compat = False

    # Only search for dbm_open if ndbm.h is present (mirrors AC_CHECK_HEADERS guard)
    # Wrap in save_env() to avoid leaking -lgdbm_compat into LIBS
    # (matches autoconf's WITH_SAVE_ENV).
    # Check for _dbmmodule.c dependencies: ndbm, gdbm_compat, libdb
    ac_cv_search_dbm_open = False
    pyconf.checking("for ndbm.h")
    found_ndbm = pyconf.check_header("ndbm.h")
    pyconf.result(found_ndbm)
    if found_ndbm:
        with pyconf.save_env():
            ac_cv_search_dbm_open = pyconf.search_libs(
                "dbm_open", ["ndbm", "gdbm_compat"], required=False
            )
    if ac_cv_search_dbm_open and ac_cv_search_dbm_open is not False:
        if (
            "ndbm" in ac_cv_search_dbm_open
            or "gdbm_compat" in ac_cv_search_dbm_open
        ):
            v.dbm_ndbm = ac_cv_search_dbm_open
            v.have_ndbm = True
        elif ac_cv_search_dbm_open == "none required":
            v.dbm_ndbm = ""
            v.have_ndbm = True
        else:
            v.have_ndbm = False

    # Check for gdbm/ndbm.h and gdbm-ndbm.h (matches configure.ac AC_CACHE_CHECK pattern)
    # "gdbm-ndbm.h" and "gdbm/ndbm.h" are both normalized to "gdbm_ndbm_h"
    pyconf.checking("for gdbm/ndbm.h")
    ac_cv_header_gdbm_slash_ndbm_h = pyconf.check_header(
        "gdbm/ndbm.h", autodefine=False
    )
    pyconf.result(ac_cv_header_gdbm_slash_ndbm_h)
    if ac_cv_header_gdbm_slash_ndbm_h:
        pyconf.define(
            "HAVE_GDBM_NDBM_H",
            1,
            "Define to 1 if you have the <gdbm/ndbm.h> header file.",
        )

    pyconf.checking("for gdbm-ndbm.h")
    ac_cv_header_gdbm_dash_ndbm_h = pyconf.check_header(
        "gdbm-ndbm.h", autodefine=False
    )
    pyconf.result(ac_cv_header_gdbm_dash_ndbm_h)
    if ac_cv_header_gdbm_dash_ndbm_h:
        pyconf.define(
            "HAVE_GDBM_DASH_NDBM_H",
            1,
            "Define to 1 if you have the <gdbm-ndbm.h> header file.",
        )

    if ac_cv_header_gdbm_slash_ndbm_h or ac_cv_header_gdbm_dash_ndbm_h:
        with pyconf.save_env():
            r = pyconf.search_libs("dbm_open", ["gdbm_compat"], required=False)
        if r and r is not False:
            v.have_gdbm_compat = True
        else:
            v.have_gdbm_compat = False

    # Check for libdb >= 5 with dbm_open()
    # db.h re-defines the name of the function
    ac_cv_have_libdb = False
    pyconf.checking("for db.h")
    found_db_h = pyconf.check_header("db.h")
    pyconf.result(found_db_h)
    if found_db_h:
        if pyconf.link_check(
            "#define DB_DBM_HSEARCH 1\n"
            "#include <db.h>\n"
            "#if DB_VERSION_MAJOR < 5\n"
            '#  error "DB_VERSION_MAJOR < 5 is not supported."\n'
            "#endif\n"
            "int main(void) { DBM *dbm = dbm_open(NULL, 0, 0); return 0; }\n",
            extra_libs="-ldb",
        ):
            ac_cv_have_libdb = True
            pyconf.define(
                "HAVE_LIBDB",
                1,
                "Define to 1 if you have the `db' library (-ldb).",
            )

    # --with-dbmliborder
    dbm_raw = WITH_DBMLIBORDER.value
    with_dbmliborder = (
        dbm_raw if dbm_raw and dbm_raw != "yes" else "gdbm:ndbm:bdb"
    )
    pyconf.checking("for --with-dbmliborder")

    for db in with_dbmliborder.split(":"):
        if db not in ("ndbm", "gdbm", "bdb"):
            pyconf.fatal(
                "proper usage is --with-dbmliborder=db1:db2:... (gdbm:ndbm:bdb)"
            )
    pyconf.result(with_dbmliborder)

    pyconf.checking("for _dbm module CFLAGS and LIBS")
    v.DBM_CFLAGS = ""
    v.DBM_LIBS = ""
    have_dbm = False

    for db in with_dbmliborder.split(":"):
        if db == "ndbm" and v.have_ndbm:
            v.DBM_CFLAGS = "-DUSE_NDBM"
            v.DBM_LIBS = v.dbm_ndbm
            have_dbm = True
            break
        elif db == "gdbm" and v.have_gdbm_compat:
            v.DBM_CFLAGS = "-DUSE_GDBM_COMPAT"
            v.DBM_LIBS = "-lgdbm_compat"
            have_dbm = True
            break
        elif db == "bdb" and ac_cv_have_libdb:
            v.DBM_CFLAGS = "-DUSE_BERKDB"
            v.DBM_LIBS = "-ldb"
            have_dbm = True
            break
    pyconf.result(f"{v.DBM_CFLAGS} {v.DBM_LIBS}".strip() or "(none)")
    v.have_dbm = have_dbm
    v.with_dbmliborder = with_dbmliborder
    # have_gdbm_dbmliborder: gdbm is in dbmliborder and gdbm is available
    v.have_gdbm_dbmliborder = (
        "gdbm" in with_dbmliborder.split(":") and v.have_gdbm is True
    )

    v.export("DBM_CFLAGS")
    v.export("DBM_LIBS")


def check_base_libraries(v):
    """Probe for sendfile, dl, dld libraries and backtrace/dladdr1 support."""
    if pyconf.check_lib("sendfile", "sendfile"):
        v.LIBS = f"-lsendfile {v.LIBS}"
    # Dynamic linking for SunOS/Solaris and SYSV
    if pyconf.check_lib("dl", "dlopen"):
        v.LIBS = f"-ldl {v.LIBS}"
        pyconf.define(
            "HAVE_LIBDL", 1, "Define to 1 if you have the `dl' library (-ldl)."
        )
    # Dynamic linking for HP-UX
    if pyconf.check_lib("dld", "shl_load"):
        v.LIBS = f"-ldld {v.LIBS}"

    # For faulthandler
    ac_cv_require_ldl = False
    # Check all three headers (must evaluate all, not short-circuit)
    eh_results = []
    for h in ("execinfo.h", "link.h", "dlfcn.h"):
        eh_results.append(pyconf.check_headers(h))
    if any(eh_results):
        if pyconf.check_funcs(["backtrace", "dladdr1"]):
            # dladdr1 requires -ldl
            ac_cv_require_ldl = True

    if ac_cv_require_ldl:
        if not pyconf.check_lib("dl", "dlopen"):
            v.LDFLAGS += " -ldl"


def check_remaining_libs(v):
    """Check sem_init, intl, AIX-specific, and aligned memory access."""
    # pthread (first!) on Linux
    pyconf.search_libs("sem_init", ["pthread", "rt", "posix4"], required=False)

    # Check if we need libintl for locale functions
    if pyconf.check_lib("intl", "textdomain"):
        pyconf.define(
            "WITH_LIBINTL",
            1,
            "Define to 1 if libintl is needed for locale functions.",
        )
        v.LIBS = f"-lintl {v.LIBS}"

    if v.ac_sys_system.startswith("AIX"):
        pyconf.checking("for genuine AIX C++ extensions support")
        if pyconf.link_check("#include <load.h>", 'loadAndInit("", 0, "")'):
            pyconf.define(
                "AIX_GENUINE_CPLUSPLUS",
                1,
                "Define for AIX if your compiler is a genuine IBM xlC/xlC_r "
                "and you want support for AIX C++ shared extension modules.",
            )
            pyconf.result("yes")
        else:
            pyconf.result("no")
        # The AIX_BUILDDATE is obtained from the kernel fileset - bos.mp64
        # BUILD_GNU_TYPE + AIX_BUILDDATE are used to construct the PEP425 tag
        # of the build system.
        pyconf.checking("for the system builddate")
        AIX_BUILDDATE = pyconf.cmd_output(
            ["sh", "-c", "lslpp -Lcq bos.mp64 | awk -F: '{ print $NF }'"]
        )
        pyconf.define_unquoted(
            "AIX_BUILDDATE",
            AIX_BUILDDATE,
            "BUILD_GNU_TYPE + AIX_BUILDDATE are used to construct the PEP425 tag "
            "of the build system.",
        )
        pyconf.result(AIX_BUILDDATE)

    ac_cv_aligned_required = pyconf.run_check(
        "aligned memory access is required",
        """
    int main(void) {
        char s[16];
        int i, *p1, *p2;
        for (i=0; i < 16; i++) s[i] = i;
        p1 = (int*)(s+1);
        p2 = (int*)(s+2);
        if (*p1 == *p2) return 1;
        return 0;
    }
    """,
        success=False,
        failure=True,
        cross_default=False if v.ac_sys_system == "Linux-android" else True,
    )
    if ac_cv_aligned_required:
        pyconf.define(
            "HAVE_ALIGNED_REQUIRED",
            1,
            "Define if aligned memory access is required",
        )


def check_libatomic(v):
    # ---------------------------------------------------------------------------
    # libatomic probe (must happen before AC_OUTPUT)
    # ---------------------------------------------------------------------------
    # gh-109054: Check if -latomic is needed to get <pyatomic.h> atomic functions.
    # On Linux aarch64, GCC may require programs and libraries to be linked
    # explicitly to libatomic. Call _Py_atomic_or_uint64() which may require
    # libatomic __atomic_fetch_or_8(), or not, depending on the C compiler and the
    # compiler flags.
    #
    # gh-112779: On RISC-V, GCC 12 and earlier require libatomic support for 1-byte
    # and 2-byte operations, but not for 8-byte operations.
    #
    # Avoid #include <Python.h> or #include <pyport.h>. The <Python.h> header
    # requires <pyconfig.h> header which is only written below by AC_OUTPUT.
    # If the check is done after AC_OUTPUT, modifying LIBS has no effect
    # anymore.  <pyport.h> cannot be included alone, it's designed to be included
    # by <Python.h>: it expects other includes and macros to be defined.

    with pyconf.save_env():
        v.CPPFLAGS = f"{v.BASECPPFLAGS} -I. -I{pyconf.srcdir}/Include {v.CPPFLAGS}".strip()
        libatomic_needed = not pyconf.link_check(
            "whether libatomic is needed by <pyatomic.h>",
            """
// pyatomic.h needs uint64_t and Py_ssize_t types
#include <stdint.h>
#ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif
#if HAVE_SSIZE_T
typedef ssize_t Py_ssize_t;
#elif SIZEOF_VOID_P == SIZEOF_SIZE_T
typedef intptr_t Py_ssize_t;
#else
#  error "unable to define Py_ssize_t"
#endif
#include "pyatomic.h"
int main() {
    uint64_t value;
    _Py_atomic_store_uint64(&value, 2);
    if (_Py_atomic_or_uint64(&value, 8) != 2) return 1;
    if (_Py_atomic_load_uint64(&value) != 10) return 1;
    uint8_t byte = 0xb8;
    if (_Py_atomic_or_uint8(&byte, 0x2d) != 0xb8) return 1;
    if (_Py_atomic_load_uint8(&byte) != 0xbd) return 1;
    return 0;
}
""",
        )

    if libatomic_needed:
        v.LIBS = f"{v.LIBS} -latomic".strip()
        v.LIBATOMIC = v.LIBATOMIC or "-latomic"
    v.export("LIBATOMIC")


def check_stat_timestamps(v):
    # ---------------------------------------------------------------------------
    # Subsecond timestamps in struct stat
    # ---------------------------------------------------------------------------
    # Look for subsecond timestamps in struct stat
    pyconf.checking("for tv_nsec in struct stat")
    ac_cv_stat_tv_nsec = pyconf.compile_check(
        includes=["sys/stat.h"],
        body="struct stat st;\nst.st_mtim.tv_nsec = 1;",
        cache_var="ac_cv_stat_tv_nsec",
    )
    pyconf.result(ac_cv_stat_tv_nsec)
    if ac_cv_stat_tv_nsec:
        pyconf.define(
            "HAVE_STAT_TV_NSEC",
            1,
            "Define if you have struct stat.st_mtim.tv_nsec",
        )

    # Look for BSD style subsecond timestamps in struct stat
    pyconf.checking("for tv_nsec2 in struct stat")
    ac_cv_stat_tv_nsec2 = pyconf.compile_check(
        includes=["sys/stat.h"],
        body="struct stat st;\nst.st_mtimespec.tv_nsec = 1;",
        cache_var="ac_cv_stat_tv_nsec2",
    )
    pyconf.result(ac_cv_stat_tv_nsec2)
    if ac_cv_stat_tv_nsec2:
        pyconf.define(
            "HAVE_STAT_TV_NSEC2",
            1,
            "Define if you have struct stat.st_mtimensec",
        )

    # ---------------------------------------------------------------------------
    # Initialize curses/panel state -- detection happens in configure_8.py
    # (configure_7.ac lines 1429-1430)
    # ---------------------------------------------------------------------------
    v.have_curses = False
    v.have_panel = False


WITH_TZPATH = pyconf.arg_with(
    "tzpath",
    metavar="<list of absolute paths separated by pathsep>",
    help="Select the default time zone search path for zoneinfo.TZPATH",
)


def _validate_tzpath(tzpath):
    if not tzpath:
        return
    for part in tzpath.split(":"):
        if not part.startswith("/"):
            pyconf.error(
                f"--with-tzpath must contain only absolute paths, not {tzpath}"
            )


def setup_tzpath(v):
    """Handle --with-tzpath option."""
    TZPATH = "/usr/share/zoneinfo:/usr/lib/zoneinfo:/usr/share/lib/zoneinfo:/etc/zoneinfo"
    pyconf.checking("for --with-tzpath")
    tzpath_val = WITH_TZPATH.value
    if tzpath_val:
        if tzpath_val == "yes":
            pyconf.error("--with-tzpath requires a value")
        _validate_tzpath(tzpath_val)
        TZPATH = tzpath_val
        pyconf.result(f'"{TZPATH}"')
    else:
        _validate_tzpath(TZPATH)
        pyconf.result(f'"{TZPATH}"')
    v.export("TZPATH", TZPATH)
