"""conf_paths — Install paths.

Handles --with-platlibdir and computes install paths (BINLIBDEST, LIBDEST,
LIBPL, etc.) and --with-wheel-pkg-dir; checks miscellaneous runtime
features (broken nice, broken poll, working tzset).
"""

from __future__ import annotations

import pyconf

WITH_PLATLIBDIR = pyconf.arg_with(
    "platlibdir",
    help='Python library directory name (default is "lib")',
)
WITH_WHEEL_PKG_DIR = pyconf.arg_with(
    "wheel-pkg-dir",
    help="Directory of wheel packages used by ensurepip (default: none)",
)


def setup_install_paths(v):
    """Set PLATLIBDIR, LIBDEST, BINLIBDEST, LIBPL, and WHEEL_PKG_DIR."""
    # ---------------------------------------------------------------------------
    # --with-platlibdir
    # ---------------------------------------------------------------------------

    v.export("PLATLIBDIR")
    # XXX: We should probably calculate the default from libdir, if defined.
    v.PLATLIBDIR = "lib"
    pyconf.checking("for --with-platlibdir")
    with_platlibdir = WITH_PLATLIBDIR.value
    if with_platlibdir and with_platlibdir not in ("yes", "no", ""):
        pyconf.result("yes")
        v.PLATLIBDIR = with_platlibdir
    else:
        pyconf.result("no")

    v.export("LIBDEST")
    v.export("BINLIBDEST")
    v.LIBDEST = r"${prefix}/${PLATLIBDIR}/python$(VERSION)$(ABI_THREAD)"
    v.BINLIBDEST = (
        r"${exec_prefix}/${PLATLIBDIR}/python$(VERSION)$(ABI_THREAD)"
    )

    # LIBPL defined after ABIFLAGS and LDVERSION is defined.
    v.export("PY_ENABLE_SHARED")
    platform_triplet = v.PLATFORM_TRIPLET
    if not platform_triplet:
        v.export("LIBPL", r"$(LIBDEST)/config-" + v.LDVERSION)
    else:
        v.export(
            "LIBPL",
            r"$(LIBDEST)/config-" + v.LDVERSION + f"-{platform_triplet}",
        )

    # ---------------------------------------------------------------------------
    # --with-wheel-pkg-dir
    # ---------------------------------------------------------------------------

    v.export("WHEEL_PKG_DIR")
    v.WHEEL_PKG_DIR = ""
    pyconf.checking("for --with-wheel-pkg-dir")
    with_wheel = WITH_WHEEL_PKG_DIR.value
    if with_wheel and with_wheel not in ("no",):
        pyconf.result("yes")
        v.WHEEL_PKG_DIR = with_wheel
    else:
        pyconf.result("no")


def check_misc_runtime(v):
    """Check for broken nice(), broken poll(), and working tzset()."""
    # ---------------------------------------------------------------------------
    # Miscellaneous runtime checks
    # ---------------------------------------------------------------------------

    # check if nice() returns success/failure instead of the new priority
    pyconf.checking("for broken nice()")
    ac_cv_broken_nice = pyconf.run_check(
        r"""
    #include <stdlib.h>
    #include <unistd.h>
    int main(void)
    {
        int val1 = nice(1);
        if (val1 != -1 && val1 == nice(2))
            exit(0);
        exit(1);
    }
    """,
        cache_var="ac_cv_broken_nice",
        cross_compiling_default=False,
    )
    pyconf.result(ac_cv_broken_nice)
    if ac_cv_broken_nice:
        pyconf.define(
            "HAVE_BROKEN_NICE",
            1,
            "Define if nice() returns success/failure instead of the new priority.",
        )

    # check if poll() sets errno on invalid file descriptors
    pyconf.checking("for broken poll()")
    ac_cv_broken_poll = pyconf.run_check(
        r"""
    #include <poll.h>
    #include <unistd.h>

    int main(void)
    {
        struct pollfd poll_struct = { 42, POLLIN|POLLPRI|POLLOUT, 0 };
        int poll_test;
        close (42);
        poll_test = poll(&poll_struct, 1, 0);
        if (poll_test < 0)
            return 0;
        else if (poll_test == 0 && poll_struct.revents != POLLNVAL)
            return 0;
        else
            return 1;
    }
    """,
        cache_var="ac_cv_broken_poll",
        cross_compiling_default=False,
    )
    pyconf.result(ac_cv_broken_poll)
    if ac_cv_broken_poll:
        pyconf.define(
            "HAVE_BROKEN_POLL",
            1,
            "Define if poll() sets errno on invalid file descriptors.",
        )

    # check tzset(3) exists and works like we expect it to
    # We need to ensure that tzset() not only does 'something' with localtime,
    # but it works as documented in the library reference and as expected by
    # the test suite. This includes making sure that tzname is set properly if
    # tm->tm_zone does not exist since it is the alternative way of getting
    # timezone info. Red Hat 6.2 doesn't understand the southern hemisphere
    # after New Year's Day.
    pyconf.checking("for working tzset()")
    ac_cv_working_tzset = pyconf.run_check(
        r"""
    #include <stdlib.h>
    #include <time.h>
    #include <string.h>

    #if HAVE_TZNAME
    extern char *tzname[];
    #endif

    int main(void)
    {
        time_t groundhogday = 1044144000; /* GMT-based */
        time_t midyear = groundhogday + (365 * 24 * 3600 / 2);

        putenv("TZ=UTC+0");
        tzset();
        if (localtime(&groundhogday)->tm_hour != 0)
            exit(1);
    #if HAVE_TZNAME
        /* For UTC, tzname[1] is sometimes "", sometimes "   " */
        if (strcmp(tzname[0], "UTC") ||
            (tzname[1][0] != 0 && tzname[1][0] != ' '))
            exit(1);
    #endif

        putenv("TZ=EST+5EDT,M4.1.0,M10.5.0");
        tzset();
        if (localtime(&groundhogday)->tm_hour != 19)
            exit(1);
    #if HAVE_TZNAME
        if (strcmp(tzname[0], "EST") || strcmp(tzname[1], "EDT"))
            exit(1);
    #endif

        putenv("TZ=AEST-10AEDT-11,M10.5.0,M3.5.0");
        tzset();
        if (localtime(&groundhogday)->tm_hour != 11)
            exit(1);
    #if HAVE_TZNAME
        if (strcmp(tzname[0], "AEST") || strcmp(tzname[1], "AEDT"))
            exit(1);
    #endif

    #if HAVE_STRUCT_TM_TM_ZONE
        if (strcmp(localtime(&groundhogday)->tm_zone, "AEDT"))
            exit(1);
        if (strcmp(localtime(&midyear)->tm_zone, "AEST"))
            exit(1);
    #endif

        exit(0);
    }
    """,
        cache_var="ac_cv_working_tzset",
        cross_compiling_default=False,
    )
    pyconf.result(ac_cv_working_tzset)
    if ac_cv_working_tzset:
        pyconf.define(
            "HAVE_WORKING_TZSET",
            1,
            "Define if tzset() actually switches the local timezone in a meaningful way.",
        )
