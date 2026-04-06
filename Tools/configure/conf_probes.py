"""conf_probes — Feature detection probes for headers, types, functions, and structs.

Checks STDC_HEADERS and all required system headers; checks clock_t,
makedev, le64toh, large-file support, and standard type macros; checks
POSIX function availability (including Android function blocking); checks
dirfd, epoll, kqueue, flock, unsetenv, socket libs, chflags; checks
declarations (MAXLOGNAME, UT_NAMESIZE, PR_SET_VMA_ANON_NAME); checks
struct members (stat, passwd, siginfo_t, statx, addrinfo, sockaddr);
handles --with-c-locale-coercion; checks AIX PIPE_BUF quirk.
"""

from __future__ import annotations

import pyconf

DISABLE_EPOLL = pyconf.arg_enable(
    "epoll", help="disable epoll (default is yes if supported)"
)

WITH_C_LOCALE_COERCION = pyconf.arg_with(
    "c-locale-coercion",
    help="enable C locale coercion to a UTF-8 based locale (default is yes)",
    default="yes",
)


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
        "Define if you have the 'UT_NAMESIZE' constant.",
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

    pyconf.checking("whether MAXLOGNAME is declared")
    found = pyconf.check_decl(
        "MAXLOGNAME",
        extra_includes=["sys/param.h"],
    )
    pyconf.result(found)
    if found:
        _define_maxlogname()

    # AC_CHECK_DECLS always defines HAVE_DECL_<NAME> to 0 or 1.
    pyconf.checking("whether UT_NAMESIZE is declared")
    found = pyconf.check_decl(
        "UT_NAMESIZE",
        define_name="HAVE_DECL_UT_NAMESIZE",
        extra_includes=["utmp.h"],
    )
    pyconf.result(found)
    if found:
        _define_ut_namesize()

    # musl libc redefines struct prctl_mm_map and conflicts with linux/prctl.h
    if v.ac_cv_libc != "musl":
        pyconf.checking("whether PR_SET_VMA_ANON_NAME is declared")
        found = pyconf.check_decl(
            "PR_SET_VMA_ANON_NAME",
            define_name="HAVE_DECL_PR_SET_VMA_ANON_NAME",
            extra_includes=["linux/prctl.h", "sys/prctl.h"],
        )
        pyconf.result(found)
        if found:
            _define_pr_set_vma_anon_name()


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
        pyconf.checking("for statx")
        pyconf.result(pyconf.check_func("statx"))

    # Force lchmod off for Linux. Linux disallows changing the mode of symbolic
    # links. Some libc implementations have a stub lchmod implementation that always
    # returns an error.
    machdep = v.MACHDEP
    if machdep != "linux":
        pyconf.checking("for lchmod")
        pyconf.result(pyconf.check_func("lchmod"))

    # iOS defines some system methods that can be linked (so they are
    # found by configure), but either raise a compilation error (because the
    # header definition prevents usage - autoconf doesn't use the headers), or
    # raise an error if used at runtime. Force these symbols off.
    if v.ac_sys_system != "iOS":
        pyconf.check_funcs(["getentropy", "getgroups", "system"])


def check_special_functions(v):
    """Check dirfd, PY_CHECK_FUNC equivalents, flock, unsetenv, socket libs, chflags."""
    # AC_CHECK_DECL([dirfd], [AC_DEFINE([HAVE_DIRFD])], [], [dirent.h])
    # Uses AC_CHECK_DECL (singular) — only define when found, never define to 0.
    pyconf.checking("whether dirfd is declared")
    has_dirfd = pyconf.check_decl(
        "dirfd",
        includes=["sys/types.h", "dirent.h"],
    )
    pyconf.result("yes" if has_dirfd else "no")
    if has_dirfd:
        pyconf.define(
            "HAVE_DIRFD",
            1,
            "Define if you have the 'dirfd' function or macro.",
        )

    # PY_CHECK_FUNC equivalents — take address to verify usability
    for _func in ("chroot", "link", "symlink", "fchdir", "fsync", "fdatasync"):
        pyconf.checking(f"for {_func}")
        pyconf.result(pyconf.check_func(_func, headers=["unistd.h"]))
    disable_epoll = DISABLE_EPOLL.is_no()
    pyconf.checking("for --disable-epoll")
    pyconf.result("yes" if disable_epoll else "no")
    if not disable_epoll:
        pyconf.checking("for epoll_create")
        pyconf.result(
            pyconf.check_func(
                "epoll_create", headers=["sys/epoll.h"], define="HAVE_EPOLL"
            )
        )
        pyconf.checking("for epoll_create1")
        pyconf.result(
            pyconf.check_func("epoll_create1", headers=["sys/epoll.h"])
        )
    pyconf.checking("for kqueue")
    pyconf.result(
        pyconf.check_func("kqueue", headers=["sys/types.h", "sys/event.h"])
    )
    pyconf.checking("for prlimit")
    pyconf.result(
        pyconf.check_func("prlimit", headers=["sys/time.h", "sys/resource.h"])
    )
    pyconf.checking("for _dyld_shared_cache_contains_path")
    pyconf.result(
        pyconf.check_func(
            "_dyld_shared_cache_contains_path",
            headers=["mach-o/dyld.h"],
            define="HAVE_DYLD_SHARED_CACHE_CONTAINS_PATH",
        )
    )
    pyconf.checking("for memfd_create")
    pyconf.result(
        pyconf.check_func(
            "memfd_create",
            headers=["sys/mman.h", "sys/memfd.h"],
            conditional_headers=["HAVE_SYS_MMAN_H", "HAVE_SYS_MEMFD_H"],
        )
    )
    pyconf.checking("for eventfd")
    pyconf.result(
        pyconf.check_func(
            "eventfd",
            headers=["sys/eventfd.h"],
            conditional_headers=["HAVE_SYS_EVENTFD_H"],
        )
    )
    pyconf.checking("for timerfd_create")
    pyconf.result(
        pyconf.check_func(
            "timerfd_create",
            headers=["sys/timerfd.h"],
            conditional_headers=["HAVE_SYS_TIMERFD_H"],
            define="HAVE_TIMERFD_CREATE",
        )
    )
    pyconf.checking("for ctermid_r")
    pyconf.result(pyconf.check_func("ctermid_r", headers=["stdio.h"]))
    pyconf.checking("for getpagesize")
    pyconf.result(pyconf.check_func("getpagesize", headers=["unistd.h"]))

    # flock: check declaration then library
    # Linking with libbsd may be necessary on AIX for flock function.
    pyconf.checking("for flock declaration")
    if pyconf.compile_check(
        preamble="#include <sys/file.h>",
        body="void *p = flock;",
    ):
        pyconf.result("yes")
        pyconf.checking("for flock")
        have_flock = pyconf.check_func("flock")
        pyconf.result(have_flock)
        if not have_flock:
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
    # On some systems (e.g. Solaris), hstrerror and inet_aton are in -lresolv
    # On others, they are in the C library, so we take no action
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
    # On Tru64, chflags seems to be present, but calling it will exit Python
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
        pyconf.checking("for chflags")
        ac_cv_have_chflags = pyconf.check_func("chflags")
        pyconf.result(ac_cv_have_chflags)
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
        pyconf.checking("for lchflags")
        ac_cv_have_lchflags = pyconf.check_func("lchflags")
        pyconf.result(ac_cv_have_lchflags)
    pyconf.result(ac_cv_have_lchflags)
    if ac_cv_have_lchflags:
        pyconf.define(
            "HAVE_LCHFLAGS",
            1,
            "Define to 1 if you have the 'lchflags' function.",
        )


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
        pyconf.checking(f"for {m}")
        found = pyconf.check_member(m, includes=["sys/types.h", "sys/stat.h"])
        pyconf.result(found)

    pyconf.check_members(
        ["struct passwd.pw_gecos", "struct passwd.pw_passwd"],
        includes=["sys/types.h", "pwd.h"],
    )

    # Issue #21085: In Cygwin, siginfo_t does not have si_band field.
    pyconf.checking("for siginfo_t.si_band")
    found = pyconf.check_member("siginfo_t.si_band", includes=["signal.h"])
    pyconf.result(found)

    # Some systems have the definitions of the mask bits without having the
    # corresponding members in struct statx.  Check for members added after Linux
    # 4.11 (when statx itself was added).
    if v.ac_cv_func_statx is True:
        for m in (
            "struct statx.stx_mnt_id",
            "struct statx.stx_dio_mem_align",
            "struct statx.stx_subvol",
            "struct statx.stx_atomic_write_unit_min",
            "struct statx.stx_dio_read_offset_align",
            "struct statx.stx_atomic_write_unit_max_opt",
        ):
            pyconf.checking(f"for {m}")
            pyconf.result(pyconf.check_member(m))

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


def setup_c_locale_coercion():
    """Handle --with-c-locale-coercion option."""
    with_c_locale_coercion = WITH_C_LOCALE_COERCION.process_value(None)
    if with_c_locale_coercion != "no":
        pyconf.define(
            "PY_COERCE_C_LOCALE",
            1,
            "Define if you want to coerce the C locale to a UTF-8 based locale",
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
    # On Linux, stropts.h may be empty
    pyconf.checking("whether stropts.h has I_PUSH")
    has_i_push = pyconf.compile_check(
        preamble=(
            "#ifdef HAVE_SYS_TYPES_H\n"
            "#  include <sys/types.h>\n"
            "#endif\n"
            "#include <stropts.h>\n"
        ),
        body="(void)I_PUSH",
    )
    pyconf.result(has_i_push)
    if has_i_push:
        pyconf.define(
            "HAVE_STROPTS_H",
            1,
            "Define to 1 if you have the <stropts.h> header file.",
        )

    # AC_HEADER_DIRENT: check for dirent.h
    pyconf.check_headers("dirent.h")

    # AC_HEADER_MAJOR: check sys/mkdev.h then sys/sysmacros.h for major/minor/makedev
    pyconf.checking("for sys/mkdev.h")
    found = pyconf.check_header("sys/mkdev.h")
    pyconf.result(found)
    if found:
        pyconf.define(
            "MAJOR_IN_MKDEV",
            1,
            "Define to 1 if major, minor, and makedev are declared in <sys/mkdev.h>.",
        )
    else:
        pyconf.checking("for sys/sysmacros.h")
        found = pyconf.check_header("sys/sysmacros.h")
        pyconf.result(found)
        if found:
            pyconf.define(
                "MAJOR_IN_SYSMACROS",
                1,
                "Define to 1 if major, minor, and makedev are declared in <sys/sysmacros.h>.",
            )

    # bluetooth/bluetooth.h has been known to not compile with -std=c99.
    # http://permalink.gmane.org/gmane.linux.bluez.kernel/22294
    save_CFLAGS = v.CFLAGS
    v.CFLAGS = f"-std=c99 {v.CFLAGS}"
    pyconf.check_headers("bluetooth/bluetooth.h")
    v.CFLAGS = save_CFLAGS

    # On Darwin (OS X) net/if.h requires sys/socket.h to be imported first.
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

    # On Linux, netlink.h requires asm/types.h
    # On FreeBSD, netlink.h is located in netlink/netlink.h
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

    # On Linux, qrtr.h requires asm/types.h
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

    # On Linux, can.h, can/bcm.h, can/isotp.h, can/j1939.h, can/raw.h require sys/socket.h
    # On NetBSD, netcan/can.h requires sys/socket.h
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
    pyconf.checking("for clock_t")
    pyconf.result(
        pyconf.check_type(
            "clock_t",
            fallback_define=(
                "clock_t",
                "long",
                "Define to 'long' if <time.h> does not define clock_t.",
            ),
            headers=["time.h"],
        )
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

    pyconf.checking("for ssize_t")
    pyconf.result(pyconf.check_type("ssize_t", headers=["sys/types.h"]))
    pyconf.checking("for __uint128_t")
    found = pyconf.check_type("__uint128_t")
    pyconf.result(found)
    if found:
        pyconf.define(
            "HAVE_GCC_UINT128_T",
            1,
            "Define if your compiler provides __uint128_t",
        )
