"""conf_platform — Platform identity and configuration.

Detects MACHDEP, ac_sys_system, and ac_sys_release from uname or a
cross-compile host triplet; sets up host_prefix and _PYTHON_HOST_PLATFORM;
decides whether to define _XOPEN_SOURCE/_POSIX_C_SOURCE per platform;
computes PLATFORM_TRIPLET, MULTIARCH, SOABI_PLATFORM, and the PEP 11
support tier; checks endianness and sets SOABI/EXT_SUFFIX/LDVERSION.
"""

from __future__ import annotations

import pyconf


def setup_machdep(v):
    """Detect MACHDEP from uname or cross-compile host triplet."""
    pyconf.env_var("MACHDEP", "name for machine-dependent library files")
    pyconf.checking("MACHDEP")

    v.ac_sys_system = ""
    v.ac_sys_release = ""
    if not v.MACHDEP:
        if v.cross_compiling:
            if pyconf.fnmatch(v.host, "*-*-linux-android*"):
                v.ac_sys_system = "Linux-android"
            elif pyconf.fnmatch(v.host, "*-*-linux*"):
                v.ac_sys_system = "Linux"
            elif pyconf.fnmatch(v.host, "*-*-cygwin*"):
                v.ac_sys_system = "Cygwin"
            elif pyconf.fnmatch(v.host, "*-apple-ios*"):
                v.ac_sys_system = "iOS"
            elif pyconf.fnmatch(v.host, "*-*-darwin*"):
                v.ac_sys_system = "Darwin"
            elif pyconf.fnmatch(v.host, "*-gnu"):
                v.ac_sys_system = "GNU"
            elif pyconf.fnmatch(v.host, "*-*-vxworks*"):
                v.ac_sys_system = "VxWorks"
            elif pyconf.fnmatch(v.host, "*-*-emscripten"):
                v.ac_sys_system = "Emscripten"
            elif pyconf.fnmatch(v.host, "*-*-wasi*"):
                v.ac_sys_system = "WASI"
            else:
                v.MACHDEP = "unknown"
                pyconf.error(f"error: cross build not supported for {v.host}")
            v.ac_sys_release = ""
        else:
            v.ac_sys_system = pyconf.cmd_output(["uname", "-s"])
            if v.ac_sys_system in ("AIX", "UnixWare", "OpenUNIX"):
                v.ac_sys_release = pyconf.cmd_output(["uname", "-v"])
            else:
                v.ac_sys_release = pyconf.cmd_output(["uname", "-r"])

        ac_md_system = (
            v.ac_sys_system.replace("/", "").replace(" ", "").lower()
        )
        ac_md_release = v.ac_sys_release.replace("/", "").replace(" ", "")
        ac_md_release = pyconf.sed(ac_md_release, r"^[A-Z]\.", "")
        ac_md_release = ac_md_release.split(".")[0]
        v.MACHDEP = ac_md_system + ac_md_release

        if v.MACHDEP.startswith("aix"):
            v.MACHDEP = "aix"
        elif v.MACHDEP.startswith("freebsd"):
            v.MACHDEP = "freebsd"
        elif v.MACHDEP.startswith("linux-android"):
            v.MACHDEP = "android"
        elif v.MACHDEP.startswith("linux"):
            v.MACHDEP = "linux"
        elif v.MACHDEP.startswith("cygwin"):
            v.MACHDEP = "cygwin"
        elif v.MACHDEP.startswith("darwin"):
            v.MACHDEP = "darwin"
        elif v.MACHDEP == "":
            v.MACHDEP = "unknown"

        if v.ac_sys_system == "SunOS":
            SUNOS_VERSION = pyconf.sed(
                v.ac_sys_release, r"\.(\d)$", r".0\1"
            ).replace(".", "")
            pyconf.define_unquoted(
                "Py_SUNOS_VERSION",
                SUNOS_VERSION,
                "The version of SunOS/Solaris as reported by `uname -r' without the dot.",
            )

    pyconf.result(f'"{v.MACHDEP}"')
    v.export("MACHDEP")


def setup_host_prefix(v):
    """Set host_prefix and host_exec_prefix for cross-compile targets."""
    if not v.host_prefix:
        v.export(
            "host_prefix",
            "/" if v.ac_sys_system == "Emscripten" else "${prefix}",
        )

    if not v.host_exec_prefix:
        v.host_exec_prefix = (
            v.host_prefix
            if v.ac_sys_system == "Emscripten"
            else "${exec_prefix}"
        )
    v.export("host_exec_prefix")


def setup_host_platform(v):
    """Set _PYTHON_HOST_PLATFORM for cross-compilation."""
    v._PYTHON_HOST_PLATFORM = ""
    if v.cross_compiling:
        host_ident = ""
        if pyconf.fnmatch(v.host, "*-*-linux*"):
            host_ident = "arm" if v.host_cpu.startswith("arm") else v.host_cpu
        elif pyconf.fnmatch(v.host, "*-*-cygwin*"):
            host_ident = ""
        elif pyconf.fnmatch(v.host, "*-apple-ios*"):
            parts = v.host.split("-")
            host_os = parts[2] if len(parts) > 2 else ""
            host_device = parts[3] if len(parts) > 3 else "os"

            pyconf.checking("iOS deployment target")
            v.IPHONEOS_DEPLOYMENT_TARGET = (
                host_os.removeprefix("ios") or "13.0"
            )
            pyconf.result(v.IPHONEOS_DEPLOYMENT_TARGET)

            if v.host_cpu == "aarch64":
                host_ident = (
                    f"{v.IPHONEOS_DEPLOYMENT_TARGET}-arm64-iphone{host_device}"
                )
            else:
                host_ident = f"{v.IPHONEOS_DEPLOYMENT_TARGET}-{v.host_cpu}-iphone{host_device}"
        elif pyconf.fnmatch(v.host, "*-*-darwin*"):
            host_ident = "arm" if v.host_cpu.startswith("arm") else v.host_cpu
        elif pyconf.fnmatch(v.host, "*-gnu"):
            host_ident = v.host_cpu
        elif pyconf.fnmatch(v.host, "*-*-vxworks*"):
            host_ident = v.host_cpu
        elif pyconf.fnmatch(v.host, "*-*-emscripten"):
            emcc_ver = pyconf.cmd_output(["emcc", "-dumpversion"]).split("-")[
                0
            ]
            host_ident = f"{emcc_ver}-{v.host_cpu}"
        elif v.host.startswith("wasm32-") or v.host.startswith("wasm64-"):
            host_ident = v.host_cpu
        else:
            v.MACHDEP = "unknown"
            pyconf.error(f"error: cross build not supported for {v.host}")

        v._PYTHON_HOST_PLATFORM = v.MACHDEP + (
            f"-{host_ident}" if host_ident else ""
        )

    # Ensure IPHONEOS_DEPLOYMENT_TARGET exists even in non-iOS builds
    if not v.is_set("IPHONEOS_DEPLOYMENT_TARGET"):
        v.IPHONEOS_DEPLOYMENT_TARGET = ""

    v.export("_PYTHON_HOST_PLATFORM")


def check_endianness_and_soabi(v):
    # ---------------------------------------------------------------------------
    # Endianness
    # ---------------------------------------------------------------------------

    pyconf.check_c_bigendian()

    # ---------------------------------------------------------------------------
    # SOABI / EXT_SUFFIX / LDVERSION
    # ---------------------------------------------------------------------------

    v.export("SOABI")  # AC_SUBST registration
    pyconf.checking("ABIFLAGS")
    pyconf.result(v.ABIFLAGS)
    pyconf.checking("SOABI")
    version_nodots = v.VERSION.replace(".", "")
    abiflags = v.ABIFLAGS
    soabi_platform = v.SOABI_PLATFORM
    v.SOABI = f"cpython-{version_nodots}{abiflags}"
    if soabi_platform:
        v.SOABI += f"-{soabi_platform}"
    pyconf.result(v.SOABI)

    if v.Py_DEBUG is True:
        v.export("ALT_SOABI")
        abiflags_no_d = abiflags.replace("d", "")
        v.ALT_SOABI = f"cpython-{version_nodots}{abiflags_no_d}"
        if soabi_platform:
            v.ALT_SOABI += f"-{soabi_platform}"
        pyconf.define_unquoted(
            "ALT_SOABI",
            f'"{v.ALT_SOABI}"',
            "Alternative SOABI used in debug build to load C extensions "
            "built in release mode",
        )

    v.export("EXT_SUFFIX")
    v.EXT_SUFFIX = f".{v.SOABI}{v.SHLIB_SUFFIX}"

    pyconf.checking("LDVERSION")
    v.export("LDVERSION", "$(VERSION)$(ABIFLAGS)")
    pyconf.result(v.LDVERSION)


def setup_xopen_source(v):
    """Decide whether to define _XOPEN_SOURCE and related macros."""
    # Some systems cannot stand _XOPEN_SOURCE being defined at all; they
    # disable features if it is defined, without any means to access these
    # features as extensions. For these systems, we skip the definition of
    # _XOPEN_SOURCE. Before adding a system to the list to gain access to
    # some feature, make sure there is no alternative way to access this
    # feature. Also, when using wildcards, make sure you have verified the
    # need for not defining _XOPEN_SOURCE on all systems matching the
    # wildcard, and that the wildcard does not include future systems
    # (which may remove their limitations).
    v.define_xopen_source = True
    sys_rel = f"{v.ac_sys_system}/{v.ac_sys_release}"
    # On OpenBSD, select(2) is not available if _XOPEN_SOURCE is defined,
    # even though select is a POSIX function. Reported by J. Ribbens.
    # Reconfirmed for OpenBSD 3.3 by Zachary Hamm, for 3.4 by Jason Ish.
    # In addition, Stefan Krah confirms that issue #1244610 exists through
    # OpenBSD 4.6, but is fixed in 4.7.
    if pyconf.fnmatch_any(
        sys_rel,
        [
            "OpenBSD/2.*",
            "OpenBSD/3.*",
            "OpenBSD/4.0*",
            "OpenBSD/4.1*",
            "OpenBSD/4.2*",
            "OpenBSD/4.3*",
            "OpenBSD/4.4*",
            "OpenBSD/4.5*",
            "OpenBSD/4.6*",
        ],
    ):
        v.define_xopen_source = False
        # OpenBSD undoes our definition of __BSD_VISIBLE if _XOPEN_SOURCE is
        # also defined. This can be overridden by defining _BSD_SOURCE
        # As this has a different meaning on Linux, only define it on OpenBSD
        pyconf.define(
            "_BSD_SOURCE",
            1,
            "Define on OpenBSD to activate all library features",
        )
    elif sys_rel.startswith("OpenBSD/"):
        # OpenBSD undoes our definition of __BSD_VISIBLE if _XOPEN_SOURCE is
        # also defined. This can be overridden by defining _BSD_SOURCE
        # As this has a different meaning on Linux, only define it on OpenBSD
        pyconf.define(
            "_BSD_SOURCE",
            1,
            "Define on OpenBSD to activate all library features",
        )
    # Defining _XOPEN_SOURCE on NetBSD version prior to the introduction of
    # _NETBSD_SOURCE disables certain features (eg. setgroups). Reported by
    # Marc Recht
    elif pyconf.fnmatch_any(sys_rel, ["NetBSD/1.5*", "NetBSD/1.6*"]):
        v.define_xopen_source = False
    # From the perspective of Solaris, _XOPEN_SOURCE is not so much a
    # request to enable features supported by the standard as a request
    # to disable features not supported by the standard.  The best way
    # for Python to use Solaris is simply to leave _XOPEN_SOURCE out
    # entirely and define __EXTENSIONS__ instead.
    elif sys_rel.startswith("SunOS/"):
        v.define_xopen_source = False
    # On UnixWare 7, u_long is never defined with _XOPEN_SOURCE,
    # but used in /usr/include/netinet/tcp.h. Reported by Tim Rice.
    # Reconfirmed for 7.1.4 by Martin v. Loewis.
    elif sys_rel.startswith("OpenUNIX/8.0.0") or pyconf.fnmatch(
        sys_rel, "UnixWare/7.1.[0-4]*"
    ):
        v.define_xopen_source = False
    # On OpenServer 5, u_short is never defined with _XOPEN_SOURCE,
    # but used in struct sockaddr.sa_family. Reported by Tim Rice.
    elif sys_rel.startswith("SCO_SV/3.2"):
        v.define_xopen_source = False
    # On MacOS X 10.2, a bug in ncurses.h means that it craps out if
    # _XOPEN_EXTENDED_SOURCE is defined. Apparently, this is fixed in 10.3, which
    # identifies itself as Darwin/7.*
    # On Mac OS X 10.4, defining _POSIX_C_SOURCE or _XOPEN_SOURCE
    # disables platform specific features beyond repair.
    # On Mac OS X 10.3, defining _POSIX_C_SOURCE or _XOPEN_SOURCE
    # has no effect, don't bother defining them
    elif pyconf.fnmatch_any(
        sys_rel, ["Darwin/[6789].*", "Darwin/[12][0-9].*"]
    ):
        v.define_xopen_source = False
    # On iOS, defining _POSIX_C_SOURCE also disables platform specific features.
    elif sys_rel.startswith("iOS/"):
        v.define_xopen_source = False
    # On QNX 6.3.2, defining _XOPEN_SOURCE prevents netdb.h from
    # defining NI_NUMERICHOST.
    elif sys_rel.startswith("QNX/6.3.2"):
        v.define_xopen_source = False
    # On VxWorks, defining _XOPEN_SOURCE causes compile failures
    # in network headers still using system V types.
    elif sys_rel.startswith("VxWorks/"):
        v.define_xopen_source = False
    # On HP-UX, defining _XOPEN_SOURCE to 600 or greater hides
    # chroot() and other functions
    elif v.ac_sys_system.startswith(("hp", "HP")):
        v.define_xopen_source = False

    if v.define_xopen_source:
        # X/Open 8, incorporating POSIX.1-2024
        pyconf.define(
            "_XOPEN_SOURCE",
            800,
            "Define to the level of X/Open that your system supports",
        )
        # On Tru64 Unix 4.0F, defining _XOPEN_SOURCE also requires
        # definition of _XOPEN_SOURCE_EXTENDED and _POSIX_C_SOURCE, or else
        # several APIs are not declared. Since this is also needed in some
        # cases for HP-UX, we define it globally.
        pyconf.define(
            "_XOPEN_SOURCE_EXTENDED",
            1,
            "Define to activate Unix95-and-earlier features",
        )
        pyconf.define_unquoted(
            "_POSIX_C_SOURCE",
            "202405L",
            "Define to activate features from IEEE Std 1003.1-2024",
        )

    if v.ac_sys_system.startswith(("hp", "HP")):
        # On HP-UX mbstate_t requires _INCLUDE__STDC_A1_SOURCE
        pyconf.define(
            "_INCLUDE__STDC_A1_SOURCE",
            1,
            "Define to include mbstate_t for mbrtowc",
        )


def setup_platform_triplet(v):
    """Detect PLATFORM_TRIPLET, MULTIARCH, and SOABI_PLATFORM."""
    pyconf.checking(
        "for the platform triplet based on compiler characteristics"
    )
    triplet_result = pyconf.run(
        "$CPP $CPPFLAGS "
        + pyconf.path_join([pyconf.srcdir, "Misc/platform_triplet.c"]),
        vars={"CPP": v.CPP, "CPPFLAGS": v.CPPFLAGS},
        capture_output=True,
        text=True,
    )
    v.PLATFORM_TRIPLET = ""
    if triplet_result.returncode == 0:
        for line in triplet_result.stdout.splitlines():
            if line.startswith("PLATFORM_TRIPLET="):
                v.PLATFORM_TRIPLET = line.removeprefix(
                    "PLATFORM_TRIPLET="
                ).strip()
                break
    pyconf.result(v.PLATFORM_TRIPLET or "none")

    pyconf.checking("for multiarch")
    if (
        v.ac_sys_system.startswith("Darwin")
        or v.ac_sys_system == "iOS"
        or v.ac_sys_system.startswith("FreeBSD")
    ):
        v.MULTIARCH = ""
    else:
        rc, v.MULTIARCH = pyconf.cmd_status([v.CC, "--print-multiarch"])
        if rc != 0:
            v.MULTIARCH = ""

    if v.PLATFORM_TRIPLET and v.MULTIARCH:
        if v.PLATFORM_TRIPLET != v.MULTIARCH:
            pyconf.error(
                "error: internal configure error for the platform triplet, "
                "please file a bug report"
            )
    elif v.PLATFORM_TRIPLET and not v.MULTIARCH:
        v.MULTIARCH = v.PLATFORM_TRIPLET
    v.export("MULTIARCH")
    v.export("PLATFORM_TRIPLET")
    pyconf.result(v.MULTIARCH)

    if v.ac_sys_system == "iOS":
        v.SOABI_PLATFORM = (
            v.PLATFORM_TRIPLET.split("-")[1]
            if "-" in v.PLATFORM_TRIPLET
            else v.PLATFORM_TRIPLET
        )
    else:
        v.SOABI_PLATFORM = v.PLATFORM_TRIPLET
    v.export("SOABI_PLATFORM")

    v.export(
        "MULTIARCH_CPPFLAGS",
        f'-DMULTIARCH=\\"{v.MULTIARCH}\\"' if v.MULTIARCH else "",
    )


def setup_pep11_tier(v):
    """Determine PEP 11 support tier from host/compiler combination."""
    pyconf.checking("for PEP 11 support tier")
    host_cc = f"{v.host}/{v.ac_cv_cc_name}"
    tier1 = [
        "x86_64-*-linux-gnu/gcc",
        "x86_64-apple-darwin*/clang",
        "aarch64-apple-darwin*/clang",
        "i686-pc-windows-msvc/msvc",
        "x86_64-pc-windows-msvc/msvc",
    ]
    tier2 = [
        "aarch64-*-linux-gnu/gcc",
        "aarch64-*-linux-gnu/clang",
        "powerpc64le-*-linux-gnu/gcc",
        "wasm32-unknown-wasip1/clang",
        "x86_64-*-linux-gnu/clang",
    ]
    tier3 = [
        "aarch64-pc-windows-msvc/msvc",
        "armv7l-*-linux-gnueabihf/gcc",
        "powerpc64le-*-linux-gnu/clang",
        "s390x-*-linux-gnu/gcc",
        "x86_64-*-freebsd*/clang",
        "aarch64-apple-ios*-simulator/clang",
        "aarch64-apple-ios*/clang",
        "aarch64-*-linux-android/clang",
        "x86_64-*-linux-android/clang",
        "wasm32-*-emscripten/emcc",
    ]
    if pyconf.fnmatch_any(host_cc, tier1):
        v.PY_SUPPORT_TIER = 1
        pyconf.result(f"{host_cc} has tier 1 (supported)")
    elif pyconf.fnmatch_any(host_cc, tier2):
        v.PY_SUPPORT_TIER = 2
        pyconf.result(f"{host_cc} has tier 2 (supported)")
    elif pyconf.fnmatch_any(host_cc, tier3):
        v.PY_SUPPORT_TIER = 3
        pyconf.result(f"{host_cc} has tier 3 (partially supported)")
    else:
        v.PY_SUPPORT_TIER = 0
        pyconf.warn(f"{host_cc} is not supported")
    pyconf.define(
        "PY_SUPPORT_TIER",
        v.PY_SUPPORT_TIER,
        "PEP 11 Support tier (1, 2, 3 or 0 for unsupported)",
    )
    v.export("PY_SUPPORT_TIER")
