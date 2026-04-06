"""conf_terminal — readline/libedit and curses/panel library detection.

Checks readline/libedit via pkg-config or manual probing with
--with-readline; probes readline feature availability (version checks
for 2.2, 4.0, 4.2); detects ncurses/panel via pkg-config with link-test
fallback; checks curses functions and capabilities.
"""

from __future__ import annotations

import pyconf


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
                pyconf.checking("for readline/readline.h")
                found_rl_h = pyconf.check_header("readline/readline.h")
                pyconf.result(found_rl_h)
                if found_rl_h:
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
                pyconf.checking("for editline/readline.h")
                found_edit_h = pyconf.check_header("editline/readline.h")
                pyconf.result(found_edit_h)
                if found_edit_h:
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
            pyconf.checking(
                "whether rl_completion_append_character is declared"
            )
            found = pyconf.check_decl(
                "rl_completion_append_character", extra_includes=rl_includes
            )
            pyconf.result(found)
            if found:
                pyconf.define(
                    "HAVE_RL_COMPLETION_APPEND_CHARACTER",
                    1,
                    "Define if you have readline 2.2",
                )

            pyconf.checking(
                "whether rl_completion_suppress_append is declared"
            )
            found = pyconf.check_decl(
                "rl_completion_suppress_append", extra_includes=rl_includes
            )
            pyconf.result(found)
            if found:
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
            pyconf.checking("whether rl_catch_signals is declared")
            found = pyconf.check_decl(
                "rl_catch_signals", extra_includes=rl_includes
            )
            pyconf.result(found)
            if found:
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
            pyconf.checking("for rl_compdisp_func_t")
            pyconf.result(
                pyconf.check_type(
                    "rl_compdisp_func_t", extra_includes=rl_includes
                )
            )

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


def _check_curses(v, lib, panel_lib):
    """Detect a curses library pair via pkg-config, with link-test fallback."""
    v.have_curses = False
    v.have_panel = False
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
        have_any_curses_h = False
        for h in (
            "HAVE_NCURSESW_NCURSES_H",
            "HAVE_NCURSESW_CURSES_H",
            "HAVE_NCURSES_NCURSES_H",
            "HAVE_NCURSES_CURSES_H",
            "HAVE_NCURSES_H",
            "HAVE_CURSES_H",
        ):
            if pyconf.is_defined(h):
                have_any_curses_h = True
                break
        if have_any_curses_h:
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
            # (mirrors the _CURSES_INCLUDES M4 macro in configure.ac)
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
            pyconf.checking("for term.h")
            pyconf.result(
                pyconf.check_header("term.h", extra_includes=CURSES_INCLUDES)
            )

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
