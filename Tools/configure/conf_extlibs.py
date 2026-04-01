"""conf_extlibs — Expat, FFI, sqlite3, Tcl/Tk detection.

Handles --with-system-expat; detects libffi via pkg-config or manual
probing (including Apple-specific libffi); detects sqlite3 via pkg-config
or manual probing with --enable-loadable-sqlite-extensions; and detects
Tcl/Tk libraries and headers via pkg-config or manual search.
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
        ffi_h = (
            f"{sdkroot}/usr/include/ffi/ffi.h"
            if sdkroot
            else "/usr/include/ffi/ffi.h"
        )
        if pyconf.path_is_file(ffi_h):
            # use ffi from SDK root
            v.have_libffi = True
            v.LIBFFI_CFLAGS = (
                f"-I{sdkroot}/usr/include/ffi -DUSING_APPLE_OS_LIBFFI=1"
            )
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
            if pyconf.check_header("ffi.h") and pyconf.check_lib(
                "ffi", "ffi_call"
            ):
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
            pyconf.check_func("ffi_prep_cif_var", headers=["ffi.h"])
            pyconf.check_func("ffi_prep_closure_loc", headers=["ffi.h"])
            pyconf.check_func("ffi_closure_alloc", headers=["ffi.h"])

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

    if pyconf.check_header(
        "sqlite3.h", extra_cflags=check_cflags, autodefine=False
    ):
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
                _x11_cflags, _x11_libs = _split_pkg_flags(parts)
                v.TCLTK_CFLAGS += " " + _x11_cflags
                v.TCLTK_LIBS += " " + _x11_libs

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
        has_create = pyconf.check_func("uuid_create")
        has_enc_be = pyconf.check_func("uuid_enc_be")
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
            save_CPPFLAGS = v.CPPFLAGS
            save_LIBS = v.LIBS
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
            v.CPPFLAGS = save_CPPFLAGS
            v.LIBS = save_LIBS

    if have_uuid == "missing":
        if pyconf.check_headers("uuid/uuid.h"):
            if pyconf.check_func("uuid_generate_time"):
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

    # configure_5.ac checks "$HAVE_UUID_GENERATE_TIME_SAFE" = "1" which is a shell var
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


WITH_READLINE = pyconf.arg_with(
    "readline",
    help="use libedit for backend or disable readline module",
)


def check_readline(v):
    # Check for libreadline and libedit
    # - libreadline provides "readline/readline.h" header and "libreadline"
    #   shared library. pkg-config file is readline.pc
    # - libedit provides "editline/readline.h" header and "libedit" shared
    #   library. pkg-config file is libedit.pc
    # - editline is not supported ("readline.h" and "libeditline" shared library)
    #
    # NOTE: In the past we checked if readline needs an additional termcap
    # library (tinfo ncursesw ncurses termcap). We now assume that libreadline
    # or readline.pc provide correct linker information.

    pyconf.define_template(
        "WITH_EDITLINE", "Define to build the readline module against libedit."
    )

    rl_raw = WITH_READLINE.value_or("readline")
    if rl_raw in ("editline", "edit"):
        with_readline = "edit"
    elif rl_raw in ("yes", "readline"):
        with_readline = "readline"
    elif rl_raw == "no":
        with_readline = False
    else:
        pyconf.error(
            "proper usage is --with(out)-readline[=editline|readline|no]"
        )

    if with_readline == "readline":
        pkg = pyconf.pkg_check_modules("LIBREADLINE", "readline")
        if pkg:
            v.LIBREADLINE = "readline"
            v.READLINE_CFLAGS = v.LIBREADLINE_CFLAGS
            v.READLINE_LIBS = v.LIBREADLINE_LIBS
        else:
            rl_found = False
            with pyconf.save_env():
                v.CPPFLAGS = f"{v.CPPFLAGS} {v.LIBREADLINE_CFLAGS}".strip()
                v.LIBS = f"{v.LIBS} {v.LIBREADLINE_LIBS}".strip()
                if pyconf.check_header("readline/readline.h"):
                    if pyconf.check_lib("readline", "readline"):
                        rl_found = True
                        rl_cflags = v.LIBREADLINE_CFLAGS or ""
                        rl_libs = v.LIBREADLINE_LIBS or "-lreadline"
            if rl_found:
                v.LIBREADLINE = "readline"
                v.READLINE_CFLAGS = rl_cflags
                v.READLINE_LIBS = rl_libs
            else:
                with_readline = False

    if with_readline == "edit":
        pkg = pyconf.pkg_check_modules("LIBEDIT", "libedit")
        if pkg:
            pyconf.define("WITH_EDITLINE", 1)
            v.LIBREADLINE = "edit"
            v.READLINE_CFLAGS = v.LIBEDIT_CFLAGS
            v.READLINE_LIBS = v.LIBEDIT_LIBS
        else:
            edit_found = False
            with pyconf.save_env():
                v.CPPFLAGS = f"{v.CPPFLAGS} {v.LIBEDIT_CFLAGS}".strip()
                v.LIBS = f"{v.LIBS} {v.LIBEDIT_LIBS}".strip()
                if pyconf.check_header("editline/readline.h"):
                    if pyconf.check_lib("edit", "readline"):
                        edit_found = True
                        edit_cflags = v.LIBEDIT_CFLAGS or ""
                        edit_libs = v.LIBEDIT_LIBS or "-ledit"
            if edit_found:
                v.LIBREADLINE = "edit"
                pyconf.define("WITH_EDITLINE", 1)
                v.READLINE_CFLAGS = edit_cflags
                v.READLINE_LIBS = edit_libs
            else:
                with_readline = False

    # pyconfig.h defines _XOPEN_SOURCE=700
    v.export(
        "READLINE_CFLAGS",
        v.READLINE_CFLAGS.replace("-D_XOPEN_SOURCE=600", "").strip(),
    )
    v.export("READLINE_LIBS")
    v.export("LIBREADLINE")

    v.with_readline = with_readline
    pyconf.checking("how to link readline")
    if with_readline is False:
        pyconf.result("no")
    else:
        pyconf.result(
            f"{with_readline} (CFLAGS: {v.READLINE_CFLAGS}, "
            f"LIBS: {v.READLINE_LIBS})"
        )

        with pyconf.save_env():
            v.CPPFLAGS = f"{v.CPPFLAGS} {v.READLINE_CFLAGS}".strip()
            v.LIBS = f"{v.LIBS} {v.READLINE_LIBS}".strip()

            # Decide which header set to use based on WITH_EDITLINE
            use_editline = with_readline == "edit"
            if use_editline:
                rl_includes = ["stdio.h", "editline/readline.h"]
            else:
                rl_includes = [
                    "stdio.h",
                    "readline/readline.h",
                    "readline/history.h",
                ]

            # check for readline 2.2
            if pyconf.check_decl(
                "rl_completion_append_character", extra_includes=rl_includes
            ):
                pyconf.define(
                    "HAVE_RL_COMPLETION_APPEND_CHARACTER",
                    1,
                    "Define if you have readline 2.2",
                )

            if pyconf.check_decl(
                "rl_completion_suppress_append", extra_includes=rl_includes
            ):
                pyconf.define(
                    "HAVE_RL_COMPLETION_SUPPRESS_APPEND",
                    1,
                    "Define if you have rl_completion_suppress_append",
                )

            # check for readline 4.0
            librl = v.LIBREADLINE or "readline"
            pyconf.checking(f"for rl_pre_input_hook in -l{librl}")
            ac_cv_rl_pre_input_hook = pyconf.link_check(
                includes=rl_includes,
                body="void *x = rl_pre_input_hook",
                cache_var="ac_cv_readline_rl_pre_input_hook",
            )
            pyconf.result(ac_cv_rl_pre_input_hook)
            if ac_cv_rl_pre_input_hook:
                pyconf.define(
                    "HAVE_RL_PRE_INPUT_HOOK",
                    1,
                    "Define if you have readline 4.0",
                )

            # also in 4.0
            pyconf.checking(
                f"for rl_completion_display_matches_hook in -l{librl}"
            )
            ac_cv_rl_completion_display = pyconf.link_check(
                includes=rl_includes,
                body="void *x = rl_completion_display_matches_hook",
                cache_var="ac_cv_readline_rl_completion_display_matches_hook",
            )
            pyconf.result(ac_cv_rl_completion_display)
            if ac_cv_rl_completion_display:
                pyconf.define(
                    "HAVE_RL_COMPLETION_DISPLAY_MATCHES_HOOK",
                    1,
                    "Define if you have readline 4.0",
                )

            # also in 4.0, but not in editline
            pyconf.checking(f"for rl_resize_terminal in -l{librl}")
            ac_cv_rl_resize_terminal = pyconf.link_check(
                includes=rl_includes,
                body="void *x = rl_resize_terminal",
                cache_var="ac_cv_readline_rl_resize_terminal",
            )
            pyconf.result(ac_cv_rl_resize_terminal)
            if ac_cv_rl_resize_terminal:
                pyconf.define(
                    "HAVE_RL_RESIZE_TERMINAL",
                    1,
                    "Define if you have readline 4.0",
                )

            # check for readline 4.2
            pyconf.checking(f"for rl_completion_matches in -l{librl}")
            ac_cv_rl_completion_matches = pyconf.link_check(
                includes=rl_includes,
                body="void *x = rl_completion_matches",
                cache_var="ac_cv_readline_rl_completion_matches",
            )
            pyconf.result(ac_cv_rl_completion_matches)
            if ac_cv_rl_completion_matches:
                pyconf.define(
                    "HAVE_RL_COMPLETION_MATCHES",
                    1,
                    "Define if you have readline 4.2",
                )

            # also in readline 4.2
            if pyconf.check_decl(
                "rl_catch_signals", extra_includes=rl_includes
            ):
                pyconf.define(
                    "HAVE_RL_CATCH_SIGNAL",
                    1,
                    "Define if you can turn off readline's signal handling.",
                )

            pyconf.checking(f"for append_history in -l{librl}")
            ac_cv_rl_append_history = pyconf.link_check(
                includes=rl_includes,
                body="void *x = append_history",
                cache_var="ac_cv_readline_append_history",
            )
            pyconf.result(ac_cv_rl_append_history)
            if ac_cv_rl_append_history:
                pyconf.define(
                    "HAVE_RL_APPEND_HISTORY",
                    1,
                    "Define if readline supports append_history",
                )

            # in readline as well as newer editline (April 2023)
            pyconf.check_type("rl_compdisp_func_t", extra_includes=rl_includes)

            # Some editline versions declare rl_startup_hook as taking no args, others
            # declare it as taking 2.
            pyconf.checking("if rl_startup_hook takes arguments")
            preamble_parts = []
            for h in rl_includes:
                preamble_parts.append(f"#include <{h}>")
            preamble = "\n".join(preamble_parts)
            ac_cv_rl_startup_hook_args = pyconf.compile_check(
                preamble=preamble
                + "\nextern int test_hook_func(const char *text, int state);",
                body="rl_startup_hook=test_hook_func;",
                cache_var="ac_cv_readline_rl_startup_hook_takes_args",
            )
            pyconf.result(ac_cv_rl_startup_hook_args)
            if ac_cv_rl_startup_hook_args:
                pyconf.define(
                    "Py_RL_STARTUP_HOOK_TAKES_ARGS",
                    1,
                    "Define if rl_startup_hook takes arguments",
                )


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
            if pyconf.check_header("zlib.h"):
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
            _found_bz2 = pyconf.check_header("bzlib.h") and pyconf.check_lib(
                "bz2", "BZ2_bzCompress"
            )
        v.have_bzip2 = _found_bz2
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
            _found_lzma = pyconf.check_header("lzma.h") and pyconf.check_lib(
                "lzma", "lzma_easy_encoder"
            )
        v.have_liblzma = _found_lzma
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
            _found_zstd = False
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
                        _found_zstd = True
                else:
                    pyconf.result("no")
        v.have_libzstd = _found_zstd
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


def _check_curses(v, lib, panel_lib):
    """Detect a curses library pair via pkg-config, with link-test fallback."""
    lib_upper = lib.upper()
    panel_upper = panel_lib.upper()
    pkg = pyconf.pkg_check_modules("CURSES", lib)
    if pkg:
        pyconf.define(
            f"HAVE_{lib_upper}",
            1,
            f"Define if you have the '{lib}' library",
        )
        v.have_curses = True
        pkg2 = pyconf.pkg_check_modules("PANEL", panel_lib)
        if pkg2:
            pyconf.define(
                f"HAVE_{panel_upper}",
                1,
                f"Define if you have the '{panel_lib}' library",
            )
            v.have_panel = True
        else:
            v.have_panel = False
    else:
        v.have_curses = False


def check_curses(v):
    # check for ncursesw/ncurses and panelw/panel
    # NOTE: old curses is not detected.

    # Check for ncursesw/panelw first. If that fails, try ncurses/panel.
    _check_curses(v, "ncursesw", "panelw")
    if not v.have_curses:
        _check_curses(v, "ncurses", "panel")

    # Curses header and link fallback; also curses function probes
    with pyconf.save_env():
        v.CPPFLAGS = f"{v.CPPFLAGS} {v.CURSES_CFLAGS} {v.PANEL_CFLAGS}".strip()
        pyconf.check_headers(
            "ncursesw/curses.h",
            "ncursesw/ncurses.h",
            "ncursesw/panel.h",
            "ncurses/curses.h",
            "ncurses/ncurses.h",
            "ncurses/panel.h",
            "curses.h",
            "ncurses.h",
            "panel.h",
        )

        # Check that we're able to link with crucial curses/panel functions. This
        # also serves as a fallback in case pkg-config failed.
        v.LIBS = f"{v.LIBS} {v.CURSES_LIBS} {v.PANEL_LIBS}".strip()
        ac_cv_search_initscr = pyconf.search_libs(
            "initscr", ["ncursesw", "ncurses"], required=False
        )
        if not ac_cv_search_initscr:
            v.have_curses = False
        else:
            if not v.have_curses:
                v.have_curses = True
                if not v.CURSES_LIBS:
                    v.CURSES_LIBS = (
                        ac_cv_search_initscr
                        if ac_cv_search_initscr != "none required"
                        else ""
                    )
        ac_cv_search_update_panels = pyconf.search_libs(
            "update_panels", ["panelw", "panel"], required=False
        )
        if not ac_cv_search_update_panels:
            v.have_panel = False
        else:
            if not v.have_panel:
                v.have_panel = True
                if not v.PANEL_LIBS:
                    v.PANEL_LIBS = (
                        ac_cv_search_update_panels
                        if ac_cv_search_update_panels != "none required"
                        else ""
                    )

        # Capture persistent values before save_env restores them
        curses_have = v.have_curses
        panel_have = v.have_panel
        curses_libs = v.CURSES_LIBS
        panel_libs = v.PANEL_LIBS
        curses_cflags = v.CURSES_CFLAGS
        panel_cflags = v.PANEL_CFLAGS

        # Note: autoconf's condition here is effectively always-true due to
        # a quoting bug (test "have_curses" != "no"), so the function checks
        # always run when any curses header is available.
        # Issue #25720: ncurses has introduced the NCURSES_OPAQUE symbol making opaque
        # structs since version 5.7.  If the macro is defined as zero before including
        # [n]curses.h, ncurses will expose fields of the structs regardless of the
        # configuration.
        _have_any_curses_h = False
        for h in (
            "HAVE_NCURSESW_NCURSES_H",
            "HAVE_NCURSESW_CURSES_H",
            "HAVE_NCURSES_NCURSES_H",
            "HAVE_NCURSES_CURSES_H",
            "HAVE_NCURSES_H",
            "HAVE_CURSES_H",
        ):
            if pyconf.is_defined(h):
                _have_any_curses_h = True
                break
        if _have_any_curses_h:
            # remove _XOPEN_SOURCE macro from curses cflags. pyconfig.h sets
            # the macro to 700.
            curses_cflags = curses_cflags.replace(
                "-D_XOPEN_SOURCE=600", ""
            ).strip()
            panel_cflags = panel_cflags.replace(
                "-D_XOPEN_SOURCE=600", ""
            ).strip()
            if v.ac_sys_system == "Darwin":
                # On macOS, there is no separate /usr/lib/libncursesw nor libpanelw.
                # System-supplied ncurses combines libncurses/libpanel and supports wide
                # characters, so we can use it like ncursesw.
                # If a locally-supplied version of libncursesw is found, we will use that.
                # There should also be a libpanelw.
                # _XOPEN_SOURCE defines are usually excluded for macOS, but we need
                # _XOPEN_SOURCE_EXTENDED here for ncurses wide char support.
                curses_cflags = (
                    f"{curses_cflags} -D_XOPEN_SOURCE_EXTENDED=1".strip()
                )

            # _CURSES_INCLUDES: conditionally include the right curses header
            # (mirrors the _CURSES_INCLUDES M4 macro in configure_8.ac lines 47-63)
            CURSES_INCLUDES = """\
#define NCURSES_OPAQUE 0
#if defined(HAVE_NCURSESW_NCURSES_H)
#  include <ncursesw/ncurses.h>
#elif defined(HAVE_NCURSESW_CURSES_H)
#  include <ncursesw/curses.h>
#elif defined(HAVE_NCURSES_NCURSES_H)
#  include <ncurses/ncurses.h>
#elif defined(HAVE_NCURSES_CURSES_H)
#  include <ncurses/curses.h>
#elif defined(HAVE_NCURSES_H)
#  include <ncurses.h>
#elif defined(HAVE_CURSES_H)
#  include <curses.h>
#endif
"""

            # On Solaris, term.h requires curses.h
            pyconf.check_header("term.h", extra_includes=CURSES_INCLUDES)

            # On HP/UX 11.0, mvwdelch is a block with a return statement
            if pyconf.compile_check(
                preamble=CURSES_INCLUDES,
                body="int rtn; rtn = mvwdelch(0,0,0);",
            ):
                pyconf.define(
                    "MVWDELCH_IS_EXPRESSION",
                    1,
                    "Define if mvwdelch in curses.h is an expression.",
                )

            if pyconf.compile_check(
                preamble=CURSES_INCLUDES, body="WINDOW *w; w->_flags = 0;"
            ):
                pyconf.define(
                    "WINDOW_HAS_FLAGS",
                    1,
                    "Define if WINDOW in curses.h offers a field _flags.",
                )

            curses_funcs = [
                "is_pad",
                "is_term_resized",
                "resize_term",
                "resizeterm",
                "immedok",
                "syncok",
                "wchgat",
                "filter",
                "has_key",
                "typeahead",
                "use_env",
            ]
            for fn in curses_funcs:
                if pyconf.compile_check(
                    description=f"for curses function {fn}",
                    preamble=CURSES_INCLUDES
                    + f"""
#ifndef {fn}
void *x={fn};
#endif
""",
                ):
                    pyconf.define(
                        f"HAVE_CURSES_{fn.upper()}",
                        1,
                        f"Define if you have the '{fn}' function.",
                    )

    # Re-apply persistent curses values captured inside save_env block
    v.have_curses = curses_have
    v.have_panel = panel_have
    v.CURSES_LIBS = curses_libs
    v.PANEL_LIBS = panel_libs
    v.CURSES_CFLAGS = curses_cflags
    v.PANEL_CFLAGS = panel_cflags
    v.export("CURSES_LIBS")
    v.export("CURSES_CFLAGS")
    v.export("PANEL_LIBS")
    v.export("PANEL_CFLAGS")
