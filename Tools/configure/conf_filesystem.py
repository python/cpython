"""Filesystem related checks."""

from __future__ import annotations

import pyconf


def check_device_macros(v):
    # ---------------------------------------------------------------------------
    # Device macros: major, minor, makedev
    # ---------------------------------------------------------------------------

    pyconf.checking("for major, minor, and makedev")
    device_macros_code = """
    #if defined(MAJOR_IN_MKDEV)
    #include <sys/mkdev.h>
    #elif defined(MAJOR_IN_SYSMACROS)
    #include <sys/types.h>
    #include <sys/sysmacros.h>
    #else
    #include <sys/types.h>
    #endif
    """
    ac_cv_device_macros = pyconf.link_check(
        preamble=device_macros_code,
        body="makedev(major(0),minor(0));",
        cache_var="ac_cv_device_macros",
    )
    pyconf.result(ac_cv_device_macros)
    if ac_cv_device_macros:
        pyconf.define(
            "HAVE_DEVICE_MACROS",
            1,
            "Define to 1 if you have the device macros.",
        )

    # Always defined for backwards compatibility
    pyconf.define(
        "SYS_SELECT_WITH_SYS_TIME",
        1,
        "Define if  you can safely include both <sys/select.h> and <sys/time.h> "
        "(which you can't on SCO ODT 3.0).",
    )


def check_device_files(v):
    # ---------------------------------------------------------------------------
    # Device files (/dev/ptmx, /dev/ptc)
    # ---------------------------------------------------------------------------

    pyconf.notice("checking for device files")

    if v.ac_sys_system in ("Linux-android", "iOS"):
        # These platforms will never have /dev/ptmx or /dev/ptc
        v.ac_cv_file__dev_ptmx = False
        v.ac_cv_file__dev_ptc = False
    else:
        if v.cross_compiling:
            # autoconf uses ${ac_cv_file__dev_ptmx+set} — test whether the
            # variable has been provided at all (via CONFIG_SITE), not
            # whether its value is truthy.
            if not v.is_set("ac_cv_file__dev_ptmx"):
                pyconf.checking("for /dev/ptmx")
                pyconf.result("not set")
                pyconf.error(
                    "set ac_cv_file__dev_ptmx to yes/no in your CONFIG_SITE "
                    "file when cross compiling"
                )
            if not v.is_set("ac_cv_file__dev_ptc"):
                pyconf.checking("for /dev/ptc")
                pyconf.result("not set")
                pyconf.error(
                    "set ac_cv_file__dev_ptc to yes/no in your CONFIG_SITE "
                    "file when cross compiling"
                )
        else:
            pyconf.checking("for /dev/ptmx")
            v.ac_cv_file__dev_ptmx = pyconf.path_exists("/dev/ptmx")
            pyconf.result(v.ac_cv_file__dev_ptmx)
            pyconf.checking("for /dev/ptc")
            v.ac_cv_file__dev_ptc = pyconf.path_exists("/dev/ptc")
            pyconf.result(v.ac_cv_file__dev_ptc)

        if v.ac_cv_file__dev_ptmx:
            pyconf.define(
                "HAVE_DEV_PTMX",
                1,
                "Define to 1 if you have the /dev/ptmx device file.",
            )
        if v.ac_cv_file__dev_ptc:
            pyconf.define(
                "HAVE_DEV_PTC",
                1,
                "Define to 1 if you have the /dev/ptc device file.",
            )

    if v.ac_sys_system == "Darwin":
        v.LIBS = f"{v.LIBS} -framework CoreFoundation".strip()


def check_dirent(v):
    # ---------------------------------------------------------------------------
    # dirent.d_type
    # ---------------------------------------------------------------------------

    if pyconf.link_check(
        "if the dirent structure of a d_type field",
        """
#include <dirent.h>
int main(void) {
    struct dirent entry;
    return entry.d_type == DT_UNKNOWN;
}
""",
    ):
        pyconf.define(
            "HAVE_DIRENT_D_TYPE",
            1,
            "Define to 1 if the dirent structure has a d_type field",
        )
