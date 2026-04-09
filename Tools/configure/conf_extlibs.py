"""conf_extlibs — external library detection for Expat, FFI, sqlite3, Tcl/Tk,
UUID, and compression (zlib, bzip2, lzma, zstd).

Handles --with-system-expat; detects libffi via pkg-config or manual probing
(including Apple SDK libffi); detects sqlite3 via pkg-config or manual probing
with --enable-loadable-sqlite-extensions; detects Tcl/Tk via pkg-config or
manual search; detects UUID libraries; probes compression libraries (zlib,
bzip2, lzma, zstd).
"""

from __future__ import annotations

import pyconf


def _split_pkg_flags(parts):
    """Split pkg-config output into (cflags, libs) strings."""
    cflags = []
    libs = []
    for p in parts:
        if p.startswith("-I") or p.startswith("-D"):
            cflags.append(p)
        else:
            libs.append(p)
    return " ".join(cflags), " ".join(libs)


WITH_SYSTEM_EXPAT = pyconf.arg_with(
    "system-expat",
    help="build pyexpat module using an installed expat library (default is no)",
    default="no",
)
ENABLE_LOADABLE_SQLITE_EXTENSIONS = pyconf.arg_enable(
    "loadable-sqlite-extensions",
    help="support loadable extensions in the sqlite3 module (default is no)",
)


def setup_expat(v):
    # Check for use of the system expat library
    with_system_expat = WITH_SYSTEM_EXPAT.process_value(None)

    if with_system_expat == "yes":
        v.LIBEXPAT_CFLAGS = ""
        v.LIBEXPAT_LDFLAGS = "-lexpat"
        v.LIBEXPAT_INTERNAL = ""
    else:
        v.LIBEXPAT_CFLAGS = r"-I$(srcdir)/Modules/expat"
        v.LIBEXPAT_LDFLAGS = r"-lm $(LIBEXPAT_A)"
        v.LIBEXPAT_INTERNAL = r"$(LIBEXPAT_HEADERS) $(LIBEXPAT_A)"

    v.export("LIBEXPAT_CFLAGS")
    v.export("LIBEXPAT_LDFLAGS")
    v.export("LIBEXPAT_INTERNAL")


def detect_libffi(v):
    # Detect libffi and check ffi function availability
    v.have_libffi = "missing"
    v.LIBFFI_CFLAGS = ""
    v.LIBFFI_LIBS = ""
    v.MODULE__CTYPES_MALLOC_CLOSURE = ""

    if v.ac_sys_system == "Darwin":
        sdkroot = v.SDKROOT
        ffi_inc = (
            f"{sdkroot}/usr/include/ffi" if sdkroot else "/usr/include/ffi"
        )
        found_darwin_ffi = False
        with pyconf.save_env():
            v.CFLAGS = f"-I{ffi_inc} {v.CFLAGS}".strip()
            if pyconf.check_header("ffi.h"):
                if pyconf.check_lib("ffi", "ffi_call"):
                    found_darwin_ffi = True
        if found_darwin_ffi:
            # use ffi from SDK root
            v.have_libffi = True
            v.LIBFFI_CFLAGS = f"-I{ffi_inc} -DUSING_APPLE_OS_LIBFFI=1"
            v.LIBFFI_LIBS = "-lffi"

    if v.have_libffi == "missing":
        # Try pkg-config
        pkg = pyconf.find_prog("pkg-config")
        if pkg:
            status, output = pyconf.cmd_status(
                [pkg, "--cflags", "--libs", "libffi"]
            )
            if status == 0:
                v.have_libffi = True
                parts = output.split()
                v.LIBFFI_CFLAGS, v.LIBFFI_LIBS = _split_pkg_flags(parts)
        if v.have_libffi == "missing":
            # Fall back: check header + lib directly
            pyconf.checking("for ffi.h")
            found_ffi_h = pyconf.check_header("ffi.h")
            pyconf.result(found_ffi_h)
            if found_ffi_h and pyconf.check_lib("ffi", "ffi_call"):
                v.have_libffi = True
                v.LIBFFI_CFLAGS = ""
                v.LIBFFI_LIBS = "-lffi"
            else:
                v.have_libffi = False

    if v.have_libffi is True:
        ctypes_malloc_closure = False
        # when do we need USING_APPLE_OS_LIBFFI?
        if v.ac_sys_system in ("Darwin", "iOS"):
            ctypes_malloc_closure = True
        elif v.ac_sys_system == "sunos5":
            v.LIBFFI_LIBS += " -mimpure-text"

        if ctypes_malloc_closure:
            v.MODULE__CTYPES_MALLOC_CLOSURE = "_ctypes/malloc_closure.c"
            v.LIBFFI_CFLAGS += " -DUSING_MALLOC_CLOSURE_DOT_C=1"

        # HAVE_LIBDL: for dlopen (gh-76828)
        if v.ac_cv_lib_dl_dlopen is True:
            v.LIBFFI_LIBS += " -ldl"

        # Check ffi function availability (with FFI flags in CFLAGS/LIBS,
        # mirroring autoconf's WITH_SAVE_ENV around these checks)
        with pyconf.save_env():
            v.CFLAGS = f"{v.CFLAGS} {v.LIBFFI_CFLAGS}".strip()
            v.LIBS = f"{v.LIBS} {v.LIBFFI_LIBS}".strip()
            pyconf.checking("for ffi_prep_cif_var")
            pyconf.result(
                pyconf.check_func("ffi_prep_cif_var", headers=["ffi.h"])
            )
            pyconf.checking("for ffi_prep_closure_loc")
            pyconf.result(
                pyconf.check_func("ffi_prep_closure_loc", headers=["ffi.h"])
            )
            pyconf.checking("for ffi_closure_alloc")
            pyconf.result(
                pyconf.check_func("ffi_closure_alloc", headers=["ffi.h"])
            )

    v.export("MODULE__CTYPES_MALLOC_CLOSURE")
    v.export("LIBFFI_CFLAGS")
    v.export("LIBFFI_LIBS")

    # Check for libffi complex double support
    # This is a workaround, since FFI_TARGET_HAS_COMPLEX_TYPE was defined in libffi v3.2.1,
    # but real support was provided only in libffi v3.3.0.
    # See https://github.com/python/cpython/issues/125206 for more details.
    pyconf.checking("libffi has complex type support")
    ac_cv_ffi_complex_double_supported = pyconf.run_check(
        """
        #include <complex.h>
        #include <ffi.h>
        int z_is_expected(double complex z) {
            const double complex expected = 1.25 - 0.5 * I;
            return z == expected;
        }
        int main(void) {
            double complex z = 1.25 - 0.5 * I;
            ffi_type *args[1] = {&ffi_type_complex_double};
            void *values[1] = {&z};
            ffi_cif cif;
            if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1,
                &ffi_type_sint, args) != FFI_OK)
                return 2;
            ffi_arg rc;
            ffi_call(&cif, FFI_FN(z_is_expected), &rc, values);
            return !rc;
        }
        """,
        extra_cflags=v.LIBFFI_CFLAGS,
        extra_libs=v.LIBFFI_LIBS,
        default=False,
    )
    pyconf.result(ac_cv_ffi_complex_double_supported)
    if ac_cv_ffi_complex_double_supported:
        pyconf.define(
            "_Py_FFI_SUPPORT_C_COMPLEX",
            1,
            "Defined if _Complex C type can be used with libffi.",
        )


def detect_sqlite3(v):
    """Detect sqlite3 and handle --enable-loadable-sqlite-extensions."""
    v.have_sqlite3 = False
    v.have_supported_sqlite3 = False

    # detect sqlite3 from Emscripten emport
    # Emscripten port check — must run before defaults are set so that
    # check_emscripten_port sees empty CFLAGS/LIBS and can set them.
    pyconf.check_emscripten_port("LIBSQLITE3", "-sUSE_SQLITE3")

    # Set defaults only if not already provided (by emscripten port or user).
    if not v.LIBSQLITE3_CFLAGS:
        v.LIBSQLITE3_CFLAGS = ""
    if not v.LIBSQLITE3_LIBS:
        v.LIBSQLITE3_LIBS = "-lsqlite3"

    # pkg-config probe
    pkg = pyconf.find_prog("pkg-config")
    if pkg:
        if pyconf.cmd([pkg, "--atleast-version=3.15.2", "sqlite3"]):
            status, output = pyconf.cmd_status(
                [pkg, "--cflags", "--libs", "sqlite3"]
            )
            if status == 0:
                parts = output.split()
                v.LIBSQLITE3_CFLAGS, v.LIBSQLITE3_LIBS = _split_pkg_flags(
                    parts
                )

    # Use the real srcdir path for configure-time checks; $(srcdir) is a
    # Makefile reference that can't be used in shell commands (the shell
    # interprets it as command substitution).
    # bpo-45774/GH-29507: The CPP check in AC_CHECK_HEADER can fail on FreeBSD,
    # hence we use CPPFLAGS instead of CFLAGS.
    sqlite_inc = f" -I{pyconf.srcdir}/Modules/_sqlite"
    check_cflags = v.LIBSQLITE3_CFLAGS + sqlite_inc
    v.LIBSQLITE3_CFLAGS += r" -I$(srcdir)/Modules/_sqlite"

    pyconf.checking("for sqlite3.h")
    found_sqlite3_h = pyconf.check_header(
        "sqlite3.h", extra_cflags=check_cflags, autodefine=False
    )
    pyconf.result(found_sqlite3_h)
    if found_sqlite3_h:
        v.have_sqlite3 = True
        if pyconf.compile_check(
            preamble="#include <sqlite3.h>\n"
            "#if SQLITE_VERSION_NUMBER < 3015002\n"
            '#  error "SQLite 3.15.2 or higher required"\n'
            "#endif",
            extra_cflags=check_cflags,
        ):
            v.have_supported_sqlite3 = True
            # Check that required functions are in place. A lot of stuff may be
            # omitted with SQLITE_OMIT_* compile time defines.
            required_funcs = [
                "sqlite3_bind_double",
                "sqlite3_column_decltype",
                "sqlite3_column_double",
                "sqlite3_complete",
                "sqlite3_progress_handler",
                "sqlite3_result_double",
                "sqlite3_set_authorizer",
                "sqlite3_value_double",
            ]
            for f in required_funcs:
                if not pyconf.check_lib(
                    "sqlite3",
                    f,
                    extra_cflags=check_cflags,
                    extra_libs=v.LIBSQLITE3_LIBS,
                ):
                    v.have_supported_sqlite3 = False
                    break
            if not pyconf.check_lib(
                "sqlite3", "sqlite3_trace_v2", extra_libs=v.LIBSQLITE3_LIBS
            ):
                if not pyconf.check_lib(
                    "sqlite3", "sqlite3_trace", extra_libs=v.LIBSQLITE3_LIBS
                ):
                    v.have_supported_sqlite3 = False
            v.have_sqlite3_load_extension = pyconf.check_lib(
                "sqlite3",
                "sqlite3_load_extension",
                extra_libs=v.LIBSQLITE3_LIBS,
            )
            if pyconf.check_lib(
                "sqlite3", "sqlite3_serialize", extra_libs=v.LIBSQLITE3_LIBS
            ):
                pyconf.define(
                    "PY_SQLITE_HAVE_SERIALIZE",
                    1,
                    "Define if SQLite was compiled with the serialize API",
                )
            pyconf.define(
                "HAVE_LIBSQLITE3",
                1,
                "Define to 1 if you have the `sqlite3' library (-lsqlite3).",
            )
        else:
            v.have_supported_sqlite3 = False

    # --enable-loadable-sqlite-extensions
    enable_loadable_sqlite_extensions = (
        ENABLE_LOADABLE_SQLITE_EXTENSIONS.value_or("no")
    )
    pyconf.checking("for --enable-loadable-sqlite-extensions")
    if enable_loadable_sqlite_extensions != "no":
        if not v.have_sqlite3_load_extension:
            pyconf.result("n/a")
            pyconf.warn(
                "Your version of SQLite does not support loadable extensions"
            )
        else:
            pyconf.result("yes")
            pyconf.define(
                "PY_SQLITE_ENABLE_LOAD_EXTENSION",
                1,
                "Define to 1 to build the sqlite module with loadable extensions support.",
            )
    else:
        pyconf.result("no")

    v.export("LIBSQLITE3_CFLAGS")
    v.export("LIBSQLITE3_LIBS")


def detect_tcltk(v):
    # Detect Tcl/Tk, use pkg-config if available
    v.have_tcltk = False
    v.TCLTK_CFLAGS = ""
    v.TCLTK_LIBS = ""

    pkg = pyconf.find_prog("pkg-config")
    if pkg:
        for q in [
            "tcl >= 8.5.12 tk >= 8.5.12",
            "tcl8.6 tk8.6",
            "tcl86 tk86",
            "tcl8.5 >= 8.5.12 tk8.5 >= 8.5.12",
            "tcl85 >= 8.5.12 tk85 >= 8.5.12",
        ]:
            if pyconf.cmd([pkg, "--exists"] + q.split()):
                status, output = pyconf.cmd_status(
                    [pkg, "--cflags", "--libs"] + q.split()
                )
                if status == 0:
                    parts = output.split()
                    v.TCLTK_CFLAGS, v.TCLTK_LIBS = _split_pkg_flags(parts)
                    break

    # FreeBSD has an X11 dependency which is not implicitly resolved.
    if v.ac_sys_system.startswith("FreeBSD") and pkg:
        if pyconf.cmd([pkg, "--exists", "x11"]):
            status, output = pyconf.cmd_status(
                [pkg, "--cflags", "--libs", "x11"]
            )
            if status == 0:
                parts = output.split()
                x11_cflags, x11_libs = _split_pkg_flags(parts)
                v.TCLTK_CFLAGS += " " + x11_cflags
                v.TCLTK_LIBS += " " + x11_libs

    # Try to link against Tcl/Tk (enforces minimum version 8.5.12)
    # Also try pkg-config if available to get flags.
    if pyconf.link_check(
        "#include <tcl.h>\n"
        "#include <tk.h>\n"
        "#if defined(TK_HEX_VERSION)\n"
        "#  if TK_HEX_VERSION < 0x0805020c\n"
        '#    error "Tk older than 8.5.12 not supported"\n'
        "#  endif\n"
        "#endif\n"
        "#if (TCL_MAJOR_VERSION < 8) || \\\n"
        "    ((TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION < 5)) || \\\n"
        "    ((TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION == 5) && (TCL_RELEASE_SERIAL < 12))\n"
        '#  error "Tcl older than 8.5.12 not supported"\n'
        "#endif\n"
        "#if (TK_MAJOR_VERSION < 8) || \\\n"
        "    ((TK_MAJOR_VERSION == 8) && (TK_MINOR_VERSION < 5)) || \\\n"
        "    ((TK_MAJOR_VERSION == 8) && (TK_MINOR_VERSION == 5) && (TK_RELEASE_SERIAL < 12))\n"
        '#  error "Tk older than 8.5.12 not supported"\n'
        "#endif\n"
        "void *x1 = Tcl_Init;\nvoid *x2 = Tk_Init;\n",
        extra_cflags=v.TCLTK_CFLAGS,
        extra_libs=v.TCLTK_LIBS,
    ):
        v.have_tcltk = True
        v.TCLTK_CFLAGS += " -Wno-strict-prototypes -DWITH_APPINIT=1"

    v.export("TCLTK_CFLAGS")
    v.export("TCLTK_LIBS")


def detect_uuid(v):
    """Detect UUID library support."""
    ac_cv_have_uuid_h = False
    ac_cv_have_uuid_uuid_h = False
    ac_cv_have_uuid_generate_time_safe = False
    LIBUUID_CFLAGS = v.LIBUUID_CFLAGS
    LIBUUID_LIBS = v.LIBUUID_LIBS
    have_uuid = "missing"

    if pyconf.check_headers("uuid.h"):
        pyconf.checking("for uuid_create")
        has_create = pyconf.check_func("uuid_create")
        pyconf.result(has_create)
        pyconf.checking("for uuid_enc_be")
        has_enc_be = pyconf.check_func("uuid_enc_be")
        pyconf.result(has_enc_be)
        if has_create and has_enc_be:
            have_uuid = True
            ac_cv_have_uuid_h = True

    if have_uuid == "missing":
        pkg = pyconf.pkg_check_modules("LIBUUID", "uuid >= 2.20")
        if pkg:
            have_uuid = True
            ac_cv_have_uuid_generate_time_safe = True
            ac_cv_have_uuid_h = True
            LIBUUID_CFLAGS = pkg.cflags
            LIBUUID_LIBS = pkg.libs
        else:
            with pyconf.save_env():
                v.CPPFLAGS = f"{v.CPPFLAGS} {LIBUUID_CFLAGS}".strip()
                v.LIBS = f"{v.LIBS} {LIBUUID_LIBS}".strip()
                if pyconf.check_headers("uuid/uuid.h"):
                    ac_cv_have_uuid_uuid_h = True
                    if pyconf.check_lib("uuid", "uuid_generate_time"):
                        have_uuid = True
                    if pyconf.check_lib("uuid", "uuid_generate_time_safe"):
                        have_uuid = True
                        ac_cv_have_uuid_generate_time_safe = True
                if have_uuid is True:
                    LIBUUID_LIBS = LIBUUID_LIBS or "-luuid"

    if have_uuid == "missing":
        if pyconf.check_headers("uuid/uuid.h"):
            pyconf.checking("for uuid_generate_time")
            found = pyconf.check_func("uuid_generate_time")
            pyconf.result(found)
            if found:
                have_uuid = True
                ac_cv_have_uuid_uuid_h = True

    if ac_cv_have_uuid_h:
        pyconf.define("HAVE_UUID_H", 1)
    if ac_cv_have_uuid_uuid_h:
        pyconf.define("HAVE_UUID_UUID_H", 1)
    if ac_cv_have_uuid_generate_time_safe:
        pyconf.define("HAVE_UUID_GENERATE_TIME_SAFE", 1)

    if v.ac_sys_system == "NetBSD":
        have_uuid = "missing"
        pyconf.define("HAVE_UUID_H", 0)

    if have_uuid == "missing":
        have_uuid = False
    v.have_uuid = have_uuid
    v.export("LIBUUID_CFLAGS", LIBUUID_CFLAGS)
    v.export("LIBUUID_LIBS", LIBUUID_LIBS)

    # configure.ac checks "$HAVE_UUID_GENERATE_TIME_SAFE" = "1" which is a shell var
    # that is never assigned (AC_DEFINE only writes to confdefs.h, not shell vars),
    # so this block never executes in the old configure.  Match that behaviour.
    if False and have_uuid is True and ac_cv_have_uuid_generate_time_safe:
        pyconf.checking("if uuid_generate_time_safe() node value is stable")
        uuid_node_src = """
    #include <inttypes.h>
    #include <stdint.h>
    #include <stdio.h>
    #ifdef HAVE_UUID_H
    #include <uuid.h>
    #else
    #include <uuid/uuid.h>
    #endif
    int main(void) {
        uuid_t uuid;
        (void)uuid_generate_time_safe(uuid);
        uint64_t node = 0;
        for (size_t i = 0; i < 6; i++) node |= (uint64_t)uuid[15-i] << (8*i);
        FILE *fp = fopen("conftest.out", "w");
        if (!fp) return 1;
        int rc = fprintf(fp, "%" PRIu64 "\\n", node) >= 0;
        rc |= fclose(fp);
        return rc == 0 ? 0 : 1;
    }
    """
        node1 = pyconf.run_program_output(
            uuid_node_src,
            extra_cflags=LIBUUID_CFLAGS,
            extra_libs=LIBUUID_LIBS,
        )
        node2 = pyconf.run_program_output(
            uuid_node_src,
            extra_cflags=LIBUUID_CFLAGS,
            extra_libs=LIBUUID_LIBS,
        )
        if node1 and node1 == node2:
            pyconf.define("HAVE_UUID_GENERATE_TIME_SAFE_STABLE_MAC", 1)
            pyconf.result("stable")
        else:
            pyconf.result("unstable")


def check_compression_libraries(v):
    # Check for compression libraries
    pyconf.define_template(
        "HAVE_ZLIB_COPY", "Define if the zlib library has inflateCopy"
    )

    # detect zlib from Emscripten emport
    pyconf.check_emscripten_port("ZLIB", "-sUSE_ZLIB")
    pkg = pyconf.pkg_check_modules("ZLIB", "zlib >= 1.2.2.1")
    if pkg:
        v.have_zlib = True
        v.ZLIB_CFLAGS = pkg.cflags
        v.ZLIB_LIBS = pkg.libs
        # zlib 1.2.0 (2003) added inflateCopy
        pyconf.define("HAVE_ZLIB_COPY", 1)
    else:
        with pyconf.save_env():
            v.CPPFLAGS = f"{v.CPPFLAGS} {v.ZLIB_CFLAGS}".strip()
            v.LIBS = f"{v.LIBS} {v.ZLIB_LIBS}".strip()
            pyconf.checking("for zlib.h")
            found_zlib_h = pyconf.check_header("zlib.h")
            pyconf.result(found_zlib_h)
            if found_zlib_h:
                if pyconf.check_lib("z", "gzread"):
                    have_zlib = True
                else:
                    have_zlib = False
            else:
                have_zlib = False
            zlib_cflags = v.ZLIB_CFLAGS or ""
            zlib_libs = (v.ZLIB_LIBS or "-lz") if have_zlib else ""
            if have_zlib:
                if pyconf.check_lib("z", "inflateCopy"):
                    pyconf.define("HAVE_ZLIB_COPY", 1)
        v.have_zlib = have_zlib
        if v.have_zlib:
            v.ZLIB_CFLAGS = zlib_cflags
            v.ZLIB_LIBS = zlib_libs

    v.export("ZLIB_CFLAGS")
    v.export("ZLIB_LIBS")

    # binascii can use zlib for optimized crc32.
    if v.have_zlib:
        v.BINASCII_CFLAGS = f"-DUSE_ZLIB_CRC32 {v.ZLIB_CFLAGS}".strip()
        v.BINASCII_LIBS = v.ZLIB_LIBS
    v.export("BINASCII_CFLAGS")
    v.export("BINASCII_LIBS")

    # detect bzip2 from Emscripten emport
    pyconf.check_emscripten_port("BZIP2", "-sUSE_BZIP2")
    pkg = pyconf.pkg_check_modules("BZIP2", "bzip2")
    if pkg:
        v.have_bzip2 = True
        v.BZIP2_CFLAGS = pkg.cflags
        v.BZIP2_LIBS = pkg.libs
    else:
        with pyconf.save_env():
            v.CPPFLAGS = f"{v.CPPFLAGS} {v.BZIP2_CFLAGS}".strip()
            v.LIBS = f"{v.LIBS} {v.BZIP2_LIBS}".strip()
            pyconf.checking("for bzlib.h")
            found_bz2_h = pyconf.check_header("bzlib.h")
            pyconf.result(found_bz2_h)
            found_bz2 = found_bz2_h and pyconf.check_lib(
                "bz2", "BZ2_bzCompress"
            )
        v.have_bzip2 = found_bz2
        if v.have_bzip2:
            v.BZIP2_CFLAGS = v.BZIP2_CFLAGS or ""
            v.BZIP2_LIBS = v.BZIP2_LIBS or "-lbz2"
    v.export("BZIP2_CFLAGS")
    v.export("BZIP2_LIBS")

    # liblzma
    pkg = pyconf.pkg_check_modules("LIBLZMA", "liblzma")
    if pkg:
        v.have_liblzma = True
        v.LIBLZMA_CFLAGS = pkg.cflags
        v.LIBLZMA_LIBS = pkg.libs
    else:
        with pyconf.save_env():
            v.CPPFLAGS = f"{v.CPPFLAGS} {v.LIBLZMA_CFLAGS}".strip()
            v.LIBS = f"{v.LIBS} {v.LIBLZMA_LIBS}".strip()
            pyconf.checking("for lzma.h")
            found_lzma_h = pyconf.check_header("lzma.h")
            pyconf.result(found_lzma_h)
            found_lzma = found_lzma_h and pyconf.check_lib(
                "lzma", "lzma_easy_encoder"
            )
        v.have_liblzma = found_lzma
        if v.have_liblzma:
            v.LIBLZMA_CFLAGS = v.LIBLZMA_CFLAGS or ""
            v.LIBLZMA_LIBS = v.LIBLZMA_LIBS or "-llzma"
    v.export("LIBLZMA_CFLAGS")
    v.export("LIBLZMA_LIBS")

    # zstd 1.4.5 stabilised ZDICT_finalizeDictionary
    pkg = pyconf.pkg_check_modules("LIBZSTD", "libzstd >= 1.4.5")
    if pkg:
        v.have_libzstd = True
        v.LIBZSTD_CFLAGS = pkg.cflags
        v.LIBZSTD_LIBS = pkg.libs
    else:
        with pyconf.save_env():
            v.CPPFLAGS = f"{v.CPPFLAGS} {v.LIBZSTD_CFLAGS}".strip()
            v.CFLAGS = f"{v.CFLAGS} {v.LIBZSTD_CFLAGS}".strip()
            v.LIBS = f"{v.LIBS} {v.LIBZSTD_LIBS}".strip()
            found_zstd = False
            if pyconf.search_libs(
                "ZDICT_finalizeDictionary", ["zstd"], required=False
            ):
                # Version gate: ZSTD_VERSION_NUMBER >= 10405
                pyconf.checking("ZSTD_VERSION_NUMBER >= 1.4.5")
                ok = pyconf.compile_check(
                    includes=["zstd.h"],
                    body="""
    #if ZSTD_VERSION_NUMBER < 10405
    #  error "zstd version is too old"
    #endif
    """,
                )
                if ok:
                    pyconf.result("yes")
                    if pyconf.check_headers(["zstd.h", "zdict.h"]):
                        found_zstd = True
                else:
                    pyconf.result("no")
        v.have_libzstd = found_zstd
        if v.have_libzstd:
            v.LIBZSTD_CFLAGS = v.LIBZSTD_CFLAGS or ""
            v.LIBZSTD_LIBS = v.LIBZSTD_LIBS or "-lzstd"
    v.export("LIBZSTD_CFLAGS")
    v.export("LIBZSTD_LIBS")

    # _remote_debugging module: optional zstd compression support
    # The module always builds, but zstd compression is only available when libzstd is found
    if v.have_libzstd:
        v.REMOTE_DEBUGGING_CFLAGS = f"-DHAVE_ZSTD {v.LIBZSTD_CFLAGS}".strip()
        v.REMOTE_DEBUGGING_LIBS = v.LIBZSTD_LIBS
    else:
        v.REMOTE_DEBUGGING_CFLAGS = ""
        v.REMOTE_DEBUGGING_LIBS = ""
    v.export("REMOTE_DEBUGGING_CFLAGS")
    v.export("REMOTE_DEBUGGING_LIBS")
