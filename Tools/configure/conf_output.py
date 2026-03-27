"""conf_output — Final output generation.

Generates final output files (Makefile.pre, pyconfig.h,
Modules/Setup.*, etc.); runs makesetup to produce Modules/config.c;
and emits post-configuration warnings and notices (pkg-config,
optimizations, platform support tier).
"""

from __future__ import annotations

import warnings

import pyconf


def generate_output(v):
    # ---------------------------------------------------------------------------
    # Generate output files  (AC_CONFIG_FILES / AC_OUTPUT)
    # ---------------------------------------------------------------------------

    pyconf.config_files(
        [
            "Makefile.pre",
            "Misc/python.pc",
            "Misc/python-embed.pc",
            "Misc/python-config.sh",
            "Modules/Setup.bootstrap",
            "Modules/Setup.stdlib",
        ]
    )
    pyconf.config_files(["Modules/ld_so_aix"], chmod_x=True)
    pyconf.output()  # writes pyconfig.h and Makefile

    # ---------------------------------------------------------------------------
    # Post-output steps
    # ---------------------------------------------------------------------------

    if not pyconf.no_create:
        if not pyconf.path_exists("Modules/Setup.local"):
            pyconf.write_file(
                "Modules/Setup.local",
                "# Edit this file for local setup changes\n",
            )

        # Use the srcdir substitution value (e.g. "." for in-place,
        # absolute path for out-of-tree) to match autoconf behaviour.
        srcdir_rel = v.srcdir or "."
        if not pyconf.cmd(
            [
                "/bin/sh",
                f"{srcdir_rel}/Modules/makesetup",
                "-c",
                f"{srcdir_rel}/Modules/config.c.in",
                "-s",
                "Modules",
                "Modules/Setup.local",
                "Modules/Setup.stdlib",
                "Modules/Setup.bootstrap",
                f"{srcdir_rel}/Modules/Setup",
            ]
        ):
            pyconf.error("makesetup failed")

        pyconf.rename_file("config.c", "Modules/config.c")

    if not v.PKG_CONFIG:
        warnings.warn(
            "pkg-config is missing. Some dependencies may not be detected correctly."
        )

    if v.Py_OPT is not True and v.Py_DEBUG is not True:
        pyconf.notice("""
If you want a release build with all stable optimizations active (PGO, etc),
please run ./configure --enable-optimizations
""")

    if v.PY_SUPPORT_TIER == 0:
        cc = v.ac_cv_cc_name
        host = v.host
        warnings.warn(
            f'Platform "{host}" with compiler "{cc}" is not supported by the '
            "CPython core team, see https://peps.python.org/pep-0011/ for more information."
        )

    if not v.ac_cv_header_stdatomic_h:
        print(
            "Your compiler or platform does not have a working C11 stdatomic.h. "
            "A future version of Python may require stdatomic.h."
        )
