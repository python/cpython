"""conf_platform — Platform identity, MACHDEP, framework, host platform.

Detects MACHDEP from uname or cross-compile host triplet; sets up
host_prefix for cross targets; seeds iOS cross-compilation tools
(AR/CC/CPP/CXX); handles --enable-universalsdk, --with-universal-archs,
--with-framework-name, --enable-framework (Darwin and iOS variants),
--with-app-store-compliance; and computes _PYTHON_HOST_PLATFORM for
cross-compilation.
"""

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
            pyconf.define(
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


WITH_C_LOCALE_COERCION = pyconf.arg_with(
    "c-locale-coercion",
    help="enable C locale coercion to a UTF-8 based locale (default is yes)",
    default="yes",
)


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
    v.define_xopen_source = True
    sys_rel = f"{v.ac_sys_system}/{v.ac_sys_release}"
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
        pyconf.define(
            "_BSD_SOURCE",
            1,
            "Define on OpenBSD to activate all library features",
        )
    elif sys_rel.startswith("OpenBSD/"):
        pyconf.define(
            "_BSD_SOURCE",
            1,
            "Define on OpenBSD to activate all library features",
        )
    elif pyconf.fnmatch_any(sys_rel, ["NetBSD/1.5*", "NetBSD/1.6*"]):
        v.define_xopen_source = False
    elif sys_rel.startswith("SunOS/"):
        v.define_xopen_source = False
    elif sys_rel.startswith("OpenUNIX/8.0.0") or pyconf.fnmatch(
        sys_rel, "UnixWare/7.1.[0-4]*"
    ):
        v.define_xopen_source = False
    elif sys_rel.startswith("SCO_SV/3.2"):
        v.define_xopen_source = False
    elif pyconf.fnmatch_any(
        sys_rel, ["Darwin/[6789].*", "Darwin/[12][0-9].*"]
    ):
        v.define_xopen_source = False
    elif sys_rel.startswith("iOS/"):
        v.define_xopen_source = False
    elif sys_rel.startswith("QNX/6.3.2"):
        v.define_xopen_source = False
    elif sys_rel.startswith("VxWorks/"):
        v.define_xopen_source = False
    elif v.ac_sys_system.startswith(("hp", "HP")):
        v.define_xopen_source = False

    if v.define_xopen_source:
        pyconf.define(
            "_XOPEN_SOURCE",
            800,
            "Define to the level of X/Open that your system supports",
        )
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
        v.MULTIARCH = pyconf.cmd_output([v.CC, "--print-multiarch"])

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


def _define_maxlogname():
    pyconf.define(
        "HAVE_MAXLOGNAME",
        1,
        "Define if you have the 'MAXLOGNAME' constant.",
    )


def _define_ut_namesize():
    pyconf.define(
        "HAVE_UT_NAMESIZE",
        1,
        "Define if you have the 'HAVE_UT_NAMESIZE' constant.",
    )


def _define_pr_set_vma_anon_name():
    pyconf.define(
        "HAVE_PR_SET_VMA_ANON_NAME",
        1,
        "Define if you have the 'PR_SET_VMA_ANON_NAME' constant.",
    )


def check_declarations(v):
    # ---------------------------------------------------------------------------
    # Declaration / constant checks
    # ---------------------------------------------------------------------------

    pyconf.check_decl(
        "MAXLOGNAME",
        on_found=_define_maxlogname,
        extra_includes=["sys/param.h"],
    )

    pyconf.check_decl(
        "UT_NAMESIZE",
        on_found=_define_ut_namesize,
        extra_includes=["utmp.h"],
    )

    # musl libc redefines struct prctl_mm_map and conflicts with linux/prctl.h
    if v.ac_cv_libc != "musl":
        pyconf.check_decl(
            "PR_SET_VMA_ANON_NAME",
            on_found=_define_pr_set_vma_anon_name,
            extra_includes=["linux/prctl.h", "sys/prctl.h"],
        )


def check_pty_and_misc_funcs(v):
    # ---------------------------------------------------------------------------
    # PTY functions: openpty, login_tty, forkpty
    # ---------------------------------------------------------------------------

    if not pyconf.check_func("openpty"):
        if pyconf.check_lib("util", "openpty"):
            pyconf.define("HAVE_OPENPTY")
            v.LIBS = f"{v.LIBS} -lutil"
        elif pyconf.check_lib("bsd", "openpty"):
            pyconf.define("HAVE_OPENPTY")
            v.LIBS = f"{v.LIBS} -lbsd"

    if pyconf.search_libs("login_tty", ["util"], required=False):
        pyconf.define(
            "HAVE_LOGIN_TTY",
            1,
            "Define to 1 if you have the `login_tty' function.",
        )

    if not pyconf.check_func("forkpty"):
        if pyconf.check_lib("util", "forkpty"):
            pyconf.define("HAVE_FORKPTY")
            v.LIBS = f"{v.LIBS} -lutil"
        elif pyconf.check_lib("bsd", "forkpty"):
            pyconf.define("HAVE_FORKPTY")
            v.LIBS = f"{v.LIBS} -lbsd"

    # namespace functions
    pyconf.check_funcs(["setns", "unshare"])

    # long file support
    pyconf.check_funcs(
        ["fseek64", "fseeko", "fstatvfs", "ftell64", "ftello", "statvfs"]
    )

    pyconf.replace_funcs(["dup2"])

    # getpgrp / setpgrp argument style
    if pyconf.check_func("getpgrp"):
        if pyconf.compile_check(includes=["unistd.h"], body="getpgrp(0);"):
            pyconf.define(
                "GETPGRP_HAVE_ARG",
                1,
                "Define if getpgrp() must be called as getpgrp(0).",
            )

    if pyconf.check_func("setpgrp"):
        if pyconf.compile_check(includes=["unistd.h"], body="setpgrp(0,0);"):
            pyconf.define(
                "SETPGRP_HAVE_ARG",
                1,
                "Define if setpgrp() must be called as setpgrp(0, 0).",
            )


def check_clock_functions(v):
    # ---------------------------------------------------------------------------
    # Clock functions
    # ---------------------------------------------------------------------------

    if not pyconf.check_func("clock_gettime"):
        if pyconf.check_lib("rt", "clock_gettime"):
            v.LIBS = f"{v.LIBS} -lrt"
            pyconf.define("HAVE_CLOCK_GETTIME", 1)
            pyconf.define(
                "TIMEMODULE_LIB",
                "rt",
                "Library needed by timemodule.c: librt may be needed for clock_gettime()",
            )
            v.TIMEMODULE_LIB = "-lrt"
    v.export("TIMEMODULE_LIB")

    if not pyconf.check_func("clock_getres"):
        if pyconf.check_lib("rt", "clock_getres"):
            pyconf.define("HAVE_CLOCK_GETRES", 1)

    # clock_settime: avoid on Android and iOS (crashes when unprivileged)
    if v.ac_sys_system not in ("Linux-android", "iOS"):
        if not pyconf.check_func("clock_settime"):
            if pyconf.check_lib("rt", "clock_settime"):
                pyconf.define("HAVE_CLOCK_SETTIME", 1)

    # clock_nanosleep: avoid on Android < API 23 (wrong return value on signal)
    android_api = int(v.ANDROID_API_LEVEL or "0")
    skip_clock_nanosleep = (
        v.ac_sys_system == "Linux-android" and android_api < 23
    )
    if not skip_clock_nanosleep:
        if not pyconf.check_func("clock_nanosleep"):
            if pyconf.check_lib("rt", "clock_nanosleep"):
                pyconf.define("HAVE_CLOCK_NANOSLEEP", 1)

    if not pyconf.check_func("nanosleep"):
        if pyconf.check_lib("rt", "nanosleep"):
            pyconf.define("HAVE_NANOSLEEP", 1)


def check_structs(v):
    # ---------------------------------------------------------------------------
    # Struct checks
    # ---------------------------------------------------------------------------

    pyconf.check_struct_tm()
    pyconf.check_struct_timezone()
    for m in (
        "struct stat.st_rdev",
        "struct stat.st_blksize",
        "struct stat.st_flags",
        "struct stat.st_gen",
        "struct stat.st_birthtime",
        "struct stat.st_blocks",
    ):
        pyconf.check_member(m, includes=["sys/types.h", "sys/stat.h"])

    pyconf.check_members(
        ["struct passwd.pw_gecos", "struct passwd.pw_passwd"],
        includes=["sys/types.h", "pwd.h"],
    )

    # Issue #21085: In Cygwin, siginfo_t does not have si_band field.
    pyconf.check_member("siginfo_t.si_band", includes=["signal.h"])

    # struct statx members added after Linux 4.11
    if v.ac_cv_func_statx is True:
        for m in (
            "struct statx.stx_mnt_id",
            "struct statx.stx_dio_mem_align",
            "struct statx.stx_subvol",
            "struct statx.stx_atomic_write_unit_min",
            "struct statx.stx_dio_read_offset_align",
            "struct statx.stx_atomic_write_unit_max_opt",
        ):
            pyconf.check_member(m, includes=["linux/stat.h"])

    # altzone in time.h
    pyconf.checking("for time.h that defines altzone")
    ac_cv_header_time_altzone = pyconf.compile_check(
        includes=["time.h"],
        body="return altzone;",
        cache_var="ac_cv_header_time_altzone",
    )
    pyconf.result(ac_cv_header_time_altzone)
    if ac_cv_header_time_altzone:
        pyconf.define(
            "HAVE_ALTZONE", 1, "Define this if your time.h defines altzone."
        )

    # struct addrinfo
    pyconf.checking("for addrinfo")
    ac_cv_struct_addrinfo = pyconf.compile_check(
        includes=["netdb.h"],
        body="struct addrinfo a",
        cache_var="ac_cv_struct_addrinfo",
    )
    pyconf.result(ac_cv_struct_addrinfo)
    if ac_cv_struct_addrinfo:
        pyconf.define("HAVE_ADDRINFO", 1, "struct addrinfo (netdb.h)")

    # struct sockaddr_storage
    pyconf.checking("for sockaddr_storage")
    ac_cv_struct_sockaddr_storage = pyconf.compile_check(
        includes=["sys/types.h", "sys/socket.h"],
        body="struct sockaddr_storage s",
        cache_var="ac_cv_struct_sockaddr_storage",
    )
    pyconf.result(ac_cv_struct_sockaddr_storage)
    if ac_cv_struct_sockaddr_storage:
        pyconf.define(
            "HAVE_SOCKADDR_STORAGE",
            1,
            "struct sockaddr_storage (sys/socket.h)",
        )

    # struct sockaddr_alg
    pyconf.checking("for sockaddr_alg")
    ac_cv_struct_sockaddr_alg = pyconf.compile_check(
        includes=["sys/types.h", "sys/socket.h", "linux/if_alg.h"],
        body="struct sockaddr_alg s",
        cache_var="ac_cv_struct_sockaddr_alg",
    )
    pyconf.result(ac_cv_struct_sockaddr_alg)
    if ac_cv_struct_sockaddr_alg:
        pyconf.define(
            "HAVE_SOCKADDR_ALG", 1, "struct sockaddr_alg (linux/if_alg.h)"
        )


def check_aix_pipe_buf(v):
    # ---------------------------------------------------------------------------
    # AIX: PIPE_BUF
    # ---------------------------------------------------------------------------

    if v.ac_sys_system.startswith("AIX"):
        pyconf.define(
            "HAVE_BROKEN_PIPE_BUF",
            1,
            "Define if the system reports an invalid PIPE_BUF value.",
        )


def check_posix_shmem(v):
    # ---------------------------------------------------------------------------
    # POSIX shared memory (_posixshmem)
    # ---------------------------------------------------------------------------

    v.POSIXSHMEM_CFLAGS = "-I$(srcdir)/Modules/_multiprocessing"
    v.POSIXSHMEM_LIBS = ""
    with pyconf.save_env():
        shm_open_result = pyconf.search_libs("shm_open", ["rt"])
        posixshmem_libs = "-lrt" if shm_open_result == "-lrt" else ""
        have_posix_shmem = pyconf.check_func(
            "shm_open", headers=["sys/mman.h"]
        ) and pyconf.check_func("shm_unlink", headers=["sys/mman.h"])
    v.POSIXSHMEM_LIBS = posixshmem_libs
    v.have_posix_shmem = have_posix_shmem
    v.export("POSIXSHMEM_CFLAGS")
    v.export("POSIXSHMEM_LIBS")


def setup_c_locale_coercion():
    """Handle --with-c-locale-coercion option."""
    with_c_locale_coercion = WITH_C_LOCALE_COERCION.process_value(None)
    if with_c_locale_coercion != "no":
        pyconf.define(
            "PY_COERCE_C_LOCALE",
            1,
            "Define if you want to coerce the C locale to a UTF-8 based locale",
        )


def check_posix_functions(v):
    """Check for Android function blocking and POSIX function availability."""
    # Android function blocking
    ANDROID_API_LEVEL = int(v.ANDROID_API_LEVEL or 0)
    if v.ac_sys_system == "Linux-android":
        blocked = (
            "chroot initgroups setegid seteuid setgid sethostname "
            "setregid setresgid setresuid setreuid setuid "
            "sem_open sem_unlink"
        ).split()
        if ANDROID_API_LEVEL < 23:
            blocked.append("fchmodat")
        for name in blocked:
            pyconf.cache[f"ac_cv_func_{name}"] = False

    # POSIX function checks
    posix_funcs = """
    accept4 alarm bind_textdomain_codeset chmod chown clearenv
    clock closefrom close_range confstr
    copy_file_range ctermid dladdr dup dup3 execv explicit_bzero explicit_memset
    faccessat fchmod fchmodat fchown fchownat fdopendir fdwalk fexecve
    fork fork1 fpathconf fstatat ftime ftruncate futimens futimes futimesat
    gai_strerror getegid geteuid getgid getgrent getgrgid getgrgid_r
    getgrnam_r getgrouplist gethostname getitimer getloadavg getlogin getlogin_r
    getpeername getpgid getpid getppid getpriority _getpty
    getpwent getpwnam_r getpwuid getpwuid_r getresgid getresuid getrusage getsid getspent
    getspnam getuid getwd grantpt if_nameindex initgroups kill killpg lchown linkat
    lockf lstat lutimes madvise mbrtowc memrchr mkdirat mkfifo mkfifoat
    mknod mknodat mktime mmap mremap nice openat opendir pathconf pause pipe
    pipe2 plock poll ppoll posix_fadvise posix_fallocate posix_openpt posix_spawn posix_spawnp
    posix_spawn_file_actions_addclosefrom_np
    pread preadv preadv2 process_vm_readv
    pthread_cond_timedwait_relative_np pthread_condattr_setclock pthread_init
    pthread_kill pthread_get_name_np pthread_getname_np pthread_set_name_np
    pthread_setname_np pthread_getattr_np
    ptsname ptsname_r pwrite pwritev pwritev2 readlink readlinkat readv realpath renameat
    rtpSpawn sched_get_priority_max sched_rr_get_interval sched_setaffinity
    sched_setparam sched_setscheduler sem_clockwait sem_getvalue sem_open
    sem_timedwait sem_unlink sendfile setegid seteuid setgid sethostname
    setitimer setlocale setpgid setpgrp setpriority setregid setresgid
    setresuid setreuid setsid setuid setvbuf shutdown sigaction sigaltstack
    sigfillset siginterrupt sigpending sigrelse sigtimedwait sigwait
    sigwaitinfo snprintf splice strftime strlcpy strsignal symlinkat sync
    sysconf tcgetpgrp tcsetpgrp tempnam timegm times tmpfile
    tmpnam tmpnam_r truncate ttyname_r umask uname unlinkat unlockpt utimensat utimes vfork
    wait wait3 wait4 waitid waitpid wcscoll wcsftime wcsxfrm wmemcmp writev
    """.split()

    pyconf.check_funcs(posix_funcs)

    # Linux-only: statx
    if v.ac_sys_system.startswith("Linux"):
        pyconf.check_func("statx")

    # lchmod: force off on Linux
    machdep = v.MACHDEP
    if machdep != "linux":
        pyconf.check_func("lchmod")

    # iOS: block certain functions
    if v.ac_sys_system != "iOS":
        pyconf.check_funcs(["getentropy", "getgroups", "system"])


def check_special_functions(v):
    """Check dirfd, PY_CHECK_FUNC equivalents, flock, unsetenv, socket libs, chflags."""
    # dirfd
    pyconf.checking("whether dirfd is declared")
    if pyconf.compile_check(
        preamble="#include <sys/types.h>\n#include <dirent.h>",
        body="void *p = dirfd;",
    ):
        pyconf.result("yes")
        pyconf.define(
            "HAVE_DIRFD",
            1,
            "Define if you have the 'dirfd' function or macro.",
        )
    else:
        pyconf.result("no")

    # PY_CHECK_FUNC equivalents — take address to verify usability
    pyconf.check_func("chroot", headers=["unistd.h"])
    pyconf.check_func("link", headers=["unistd.h"])
    pyconf.check_func("symlink", headers=["unistd.h"])
    pyconf.check_func("fchdir", headers=["unistd.h"])
    pyconf.check_func("fsync", headers=["unistd.h"])
    pyconf.check_func("fdatasync", headers=["unistd.h"])
    pyconf.check_func(
        "epoll_create", headers=["sys/epoll.h"], define="HAVE_EPOLL"
    )
    pyconf.check_func("epoll_create1", headers=["sys/epoll.h"])
    pyconf.check_func("kqueue", headers=["sys/types.h", "sys/event.h"])
    pyconf.check_func("prlimit", headers=["sys/time.h", "sys/resource.h"])
    pyconf.check_func(
        "_dyld_shared_cache_contains_path",
        headers=["mach-o/dyld.h"],
        define="HAVE_DYLD_SHARED_CACHE_CONTAINS_PATH",
    )
    pyconf.check_func(
        "memfd_create",
        headers=["sys/mman.h", "sys/memfd.h"],
        conditional_headers=["HAVE_SYS_MMAN_H", "HAVE_SYS_MEMFD_H"],
    )
    pyconf.check_func(
        "eventfd",
        headers=["sys/eventfd.h"],
        conditional_headers=["HAVE_SYS_EVENTFD_H"],
    )
    pyconf.check_func(
        "timerfd_create",
        headers=["sys/timerfd.h"],
        conditional_headers=["HAVE_SYS_TIMERFD_H"],
        define="HAVE_TIMERFD_CREATE",
    )
    pyconf.check_func("ctermid_r", headers=["stdio.h"])
    pyconf.check_func("getpagesize", headers=["unistd.h"])

    # flock: check declaration then library
    pyconf.checking("for flock declaration")
    if pyconf.compile_check(
        preamble="#include <sys/file.h>",
        body="void *p = flock;",
    ):
        pyconf.result("yes")
        if not pyconf.check_func("flock"):
            if pyconf.check_lib("bsd", "flock"):
                v.FCNTL_LIBS = "-lbsd"
    else:
        pyconf.result("no")
    if not v.is_set("FCNTL_LIBS"):
        v.FCNTL_LIBS = ""
    v.export("FCNTL_LIBS")

    # broken unsetenv
    pyconf.checking("for broken unsetenv")
    if pyconf.compile_check(
        preamble="#include <stdlib.h>",
        body='int res = unsetenv("DUMMY");',
    ):
        pyconf.result("no")
    else:
        pyconf.result("yes")
        pyconf.define(
            "HAVE_BROKEN_UNSETENV",
            1,
            "Define if 'unsetenv' does not return an int.",
        )

    # SOCKET_LIBS: check for inet_aton / hstrerror in libc vs. -lresolv
    v.SOCKET_LIBS = ""
    if not pyconf.check_lib("c", "inet_aton"):
        if pyconf.check_lib("resolv", "inet_aton"):
            v.SOCKET_LIBS = "-lresolv"
    if not v.SOCKET_LIBS:
        if not pyconf.check_lib("c", "hstrerror"):
            if pyconf.check_lib("resolv", "hstrerror"):
                v.SOCKET_LIBS = "-lresolv"
    v.export("SOCKET_LIBS")

    # chflags / lchflags
    pyconf.checking("for chflags")
    ac_cv_have_chflags = pyconf.run_check(
        "#include <sys/stat.h>\n#include <unistd.h>\n"
        "int main(int argc, char *argv[]) {\n"
        "  if(chflags(argv[0], 0) != 0) return 1;\n"
        "  return 0;\n"
        "}\n",
        default="cross",
    )
    if ac_cv_have_chflags == "cross":
        ac_cv_have_chflags = pyconf.check_func("chflags")
    pyconf.result(ac_cv_have_chflags)
    if ac_cv_have_chflags:
        pyconf.define(
            "HAVE_CHFLAGS",
            1,
            "Define to 1 if you have the 'chflags' function.",
        )

    pyconf.checking("for lchflags")
    ac_cv_have_lchflags = pyconf.run_check(
        "#include <sys/stat.h>\n#include <unistd.h>\n"
        "int main(int argc, char *argv[]) {\n"
        "  if(lchflags(argv[0], 0) != 0) return 1;\n"
        "  return 0;\n"
        "}\n",
        default="cross",
    )
    if ac_cv_have_lchflags == "cross":
        ac_cv_have_lchflags = pyconf.check_func("lchflags")
    pyconf.result(ac_cv_have_lchflags)
    if ac_cv_have_lchflags:
        pyconf.define(
            "HAVE_LCHFLAGS",
            1,
            "Define to 1 if you have the 'lchflags' function.",
        )


def check_headers(v):
    """Check for STDC_HEADERS and all required system headers."""
    pyconf.define(
        "STDC_HEADERS", 1, "Define to 1 if you have the ANSI C header files."
    )

    pyconf.check_headers(
        # Standard C headers (AC_INCLUDES_DEFAULT in autoconf)
        "stdio.h",
        "stdlib.h",
        "string.h",
        "inttypes.h",
        "stdint.h",
        "strings.h",
        "unistd.h",
        # Other headers
        "alloca.h",
        "asm/types.h",
        "bluetooth.h",
        "conio.h",
        "direct.h",
        "dlfcn.h",
        "endian.h",
        "errno.h",
        "fcntl.h",
        "grp.h",
        "io.h",
        "langinfo.h",
        "libintl.h",
        "libutil.h",
        "linux/auxvec.h",
        "sys/auxv.h",
        "linux/fs.h",
        "linux/limits.h",
        "linux/memfd.h",
        "linux/netfilter_ipv4.h",
        "linux/random.h",
        "linux/soundcard.h",
        "linux/sched.h",
        "linux/tipc.h",
        "linux/wait.h",
        "netdb.h",
        "net/ethernet.h",
        "netinet/in.h",
        "netpacket/packet.h",
        "poll.h",
        "process.h",
        "pthread.h",
        "pty.h",
        "sched.h",
        "setjmp.h",
        "shadow.h",
        "signal.h",
        "spawn.h",
        "stropts.h",
        "sys/audioio.h",
        "sys/bsdtty.h",
        "sys/devpoll.h",
        "sys/endian.h",
        "sys/epoll.h",
        "sys/event.h",
        "sys/eventfd.h",
        "sys/file.h",
        "sys/ioctl.h",
        "sys/kern_control.h",
        "sys/loadavg.h",
        "sys/lock.h",
        "sys/memfd.h",
        "sys/mkdev.h",
        "sys/mman.h",
        "sys/modem.h",
        "sys/param.h",
        "sys/pidfd.h",
        "sys/poll.h",
        "sys/random.h",
        "sys/resource.h",
        "sys/select.h",
        "sys/sendfile.h",
        "sys/socket.h",
        "sys/soundcard.h",
        "sys/stat.h",
        "sys/statvfs.h",
        "sys/sys_domain.h",
        "sys/syscall.h",
        "sys/sysmacros.h",
        "sys/termio.h",
        "sys/time.h",
        "sys/times.h",
        "sys/timerfd.h",
        "sys/types.h",
        "sys/uio.h",
        "sys/un.h",
        "sys/utsname.h",
        "sys/wait.h",
        "sys/xattr.h",
        "sysexits.h",
        "syslog.h",
        "termios.h",
        "util.h",
        "utime.h",
        "utmp.h",
    )
    # AC_HEADER_DIRENT: check for dirent.h
    pyconf.check_headers("dirent.h")

    # AC_HEADER_MAJOR: check sys/mkdev.h then sys/sysmacros.h for major/minor/makedev
    if pyconf.check_header("sys/mkdev.h"):
        pyconf.define(
            "MAJOR_IN_MKDEV",
            1,
            "Define to 1 if major, minor, and makedev are declared in <sys/mkdev.h>.",
        )
    elif pyconf.check_header("sys/sysmacros.h"):
        pyconf.define(
            "MAJOR_IN_SYSMACROS",
            1,
            "Define to 1 if major, minor, and makedev are declared in <sys/sysmacros.h>.",
        )

    save_CFLAGS = v.CFLAGS
    v.CFLAGS = f"-std=c99 {v.CFLAGS}"
    pyconf.check_headers("bluetooth/bluetooth.h")
    v.CFLAGS = save_CFLAGS

    pyconf.check_headers(
        "net/if.h",
        prologue="""
    #include <stdio.h>
    #include <stdlib.h>
    #include <stddef.h>
    #ifdef HAVE_SYS_SOCKET_H
    # include <sys/socket.h>
    #endif
    """,
    )

    pyconf.check_headers(
        "linux/netlink.h",
        "netlink/netlink.h",
        prologue="""
    #ifdef HAVE_ASM_TYPES_H
    #include <asm/types.h>
    #endif
    #ifdef HAVE_SYS_SOCKET_H
    #include <sys/socket.h>
    #endif
    """,
    )

    pyconf.check_headers(
        "linux/qrtr.h",
        prologue="""
    #ifdef HAVE_ASM_TYPES_H
    #include <asm/types.h>
    #endif
    #ifdef HAVE_SYS_SOCKET_H
    #include <sys/socket.h>
    #endif
    """,
    )

    pyconf.check_headers(
        "linux/vm_sockets.h",
        prologue="""
    #ifdef HAVE_SYS_SOCKET_H
    #include <sys/socket.h>
    #endif
    """,
    )

    pyconf.check_headers(
        "linux/can.h",
        "linux/can/bcm.h",
        "linux/can/isotp.h",
        "linux/can/j1939.h",
        "linux/can/raw.h",
        "netcan/can.h",
        prologue="""
    #ifdef HAVE_SYS_SOCKET_H
    #include <sys/socket.h>
    #endif
    """,
    )


def check_types_and_macros(v):
    """Check clock_t, makedev, le64toh, large-file support, and standard type macros."""
    pyconf.check_type(
        "clock_t",
        fallback_define=(
            "clock_t",
            "long",
            "Define to 'long' if <time.h> does not define clock_t.",
        ),
        headers=["time.h"],
    )

    if pyconf.link_check(
        """
    #if defined(MAJOR_IN_MKDEV)
    #include <sys/mkdev.h>
    #elif defined(MAJOR_IN_SYSMACROS)
    #include <sys/sysmacros.h>
    #else
    #include <sys/types.h>
    #endif
    """,
        "makedev(0, 0)",
        checking="for makedev",
    ):
        pyconf.define(
            "HAVE_MAKEDEV", 1, "Define this if you have the makedev macro."
        )

    if pyconf.link_check(
        """
    #ifdef HAVE_ENDIAN_H
    #include <endian.h>
    #elif defined(HAVE_SYS_ENDIAN_H)
    #include <sys/endian.h>
    #endif
    """,
        "le64toh(1)",
        checking="for le64toh",
    ):
        pyconf.define("HAVE_HTOLE64", 1, "Define this if you have le64toh()")

    # --- Large-file support ---
    use_lfs = not v.ac_sys_system.startswith("GNU")

    if use_lfs:
        if f"{v.ac_sys_system}/{v.ac_sys_release}".startswith("AIX"):
            pyconf.define(
                "_LARGE_FILES",
                1,
                "This must be defined on AIX systems to enable large file support.",
            )
        pyconf.define(
            "_LARGEFILE_SOURCE",
            1,
            "This must be defined on some systems to enable large file support.",
        )
        pyconf.define(
            "_FILE_OFFSET_BITS",
            64,
            "This must be set to 64 on some systems to enable large file support.",
        )

    pyconf.shell(
        "cat >> confdefs.h <<\\EOF\n"
        "#if defined(SCO_DS)\n#undef _OFF_T\n#endif\nEOF\n",
        exports=[],
    )

    # --- Type checks ---
    pyconf.macro("AC_TYPE_MODE_T", [])
    pyconf.macro("AC_TYPE_OFF_T", [])
    pyconf.macro("AC_TYPE_PID_T", [])
    pyconf.define_unquoted(
        "RETSIGTYPE",
        "void",
        "assume C89 semantics that RETSIGTYPE is always void",
    )
    pyconf.macro("AC_TYPE_SIZE_T", [])
    pyconf.macro("AC_TYPE_UID_T", [])

    pyconf.check_type("ssize_t", headers=["sys/types.h"])
    if pyconf.check_type("__uint128_t"):
        pyconf.define(
            "HAVE_GCC_UINT128_T",
            1,
            "Define if your compiler provides __uint128_t",
        )
