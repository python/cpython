"""Checked related to threading libraries."""

from __future__ import annotations

import pyconf

_PTHREAD_TEST_SRC = """
    #include <stdio.h>
    #include <pthread.h>
    void* routine(void* p){return NULL;}
    int main(void){
      pthread_t p;
      if(pthread_create(&p,NULL,routine,NULL)!=0) return 1;
      (void)pthread_detach(p);
      return 0;
    }
    """


def check_pthreads(v):
    """Detect pthreads support and CXX thread flags."""
    # On some compilers, pthreads are available without further options
    # (e.g. MacOS X). On some of these systems, the compiler will not
    # complain if unaccepted options are passed (e.g. gcc on Mac OS X).
    # So we have to see first whether pthreads are available without
    # options before we can check whether -Kpthread improves anything.
    ac_cv_pthread_is_default = pyconf.run_check(
        "whether pthreads are available without options",
        _PTHREAD_TEST_SRC,
        cross_default=False,
    )
    v.ac_cv_pthread_is_default = ac_cv_pthread_is_default

    if ac_cv_pthread_is_default:
        ac_cv_kpthread = False
        ac_cv_kthread = False
        ac_cv_pthread = False
    else:
        # -Kpthread, if available, provides the right #defines
        # and linker options to make pthread_create available
        # Some compilers won't report that they do not support -Kpthread,
        # so we need to run a program to see whether it really made the
        # function available.
        ac_cv_kpthread = pyconf.run_check_with_cc_flag(
            f"whether {v.CC} accepts -Kpthread",
            "-Kpthread",
            _PTHREAD_TEST_SRC,
            cross_default=False,
        )

    if not ac_cv_kpthread and not ac_cv_pthread_is_default:
        # -Kthread, if available, provides the right #defines
        # and linker options to make pthread_create available
        # Some compilers won't report that they do not support -Kthread,
        # so we need to run a program to see whether it really made the
        # function available.
        ac_cv_kthread = pyconf.run_check_with_cc_flag(
            f"whether {v.CC} accepts -Kthread",
            "-Kthread",
            _PTHREAD_TEST_SRC,
            cross_default=False,
        )
    else:
        ac_cv_kthread = False

    if not ac_cv_kthread and not ac_cv_pthread_is_default:
        # -pthread, if available, provides the right #defines
        # and linker options to make pthread_create available
        # Some compilers won't report that they do not support -pthread,
        # so we need to run a program to see whether it really made the
        # function available.
        ac_cv_pthread = pyconf.run_check_with_cc_flag(
            f"whether {v.CC} accepts -pthread",
            "-pthread",
            _PTHREAD_TEST_SRC,
            cross_default=False,
        )
    else:
        ac_cv_pthread = False

    v.ac_cv_kpthread = ac_cv_kpthread
    v.ac_cv_kthread = ac_cv_kthread
    v.ac_cv_pthread = ac_cv_pthread

    if v.CXX:
        # If we have set a CC compiler flag for thread support then
        # check if it works for CXX, too.
        if ac_cv_kpthread:
            cxx_flag = "-Kpthread"
            ac_cv_cxx_thread = True
        elif ac_cv_kthread:
            cxx_flag = "-Kthread"
            ac_cv_cxx_thread = True
        elif ac_cv_pthread:
            cxx_flag = "-pthread"
            ac_cv_cxx_thread = True
        else:
            cxx_flag = ""
            ac_cv_cxx_thread = False

        if ac_cv_cxx_thread:
            ac_cv_cxx_thread = pyconf.compile_link_check(
                f"whether {v.CXX} also accepts flags for thread support",
                compiler=f"{v.CXX} {cxx_flag}",
                source="void foo();int main(){foo();}void foo(){}",
            )
            # Note: autoconf saves/restores CXX here; the flag is added
            # permanently later in setup_pthreads().
            if ac_cv_cxx_thread != "yes":
                ac_cv_cxx_thread = False
    else:
        ac_cv_cxx_thread = False

    v.ac_cv_cxx_thread = ac_cv_cxx_thread


def setup_pthreads(v):
    """Finalize CC/CXX/LIBS/posix_threads based on pthread detection results."""
    v.posix_threads = False

    ac_cv_pthread_is_default = v.ac_cv_pthread_is_default
    ac_cv_kpthread = v.ac_cv_kpthread
    ac_cv_kthread = v.ac_cv_kthread
    ac_cv_pthread = v.ac_cv_pthread
    ac_cv_cxx_thread = v.ac_cv_cxx_thread
    unistd_defines_pthreads = (
        ""  # only set in fallback branch; empty == "not checked"
    )

    if ac_cv_pthread_is_default:
        # Defining _REENTRANT on system with POSIX threads should not hurt.
        pyconf.define("_REENTRANT")
        v.posix_threads = True
        if v.ac_sys_system == "SunOS":
            v.CFLAGS += " -D_REENTRANT"
    elif ac_cv_kpthread:
        v.CC += " -Kpthread"
        if ac_cv_cxx_thread:
            v.CXX += " -Kpthread"
        v.posix_threads = True
    elif ac_cv_kthread:
        v.CC += " -Kthread"
        if ac_cv_cxx_thread:
            v.CXX += " -Kthread"
        v.posix_threads = True
    elif ac_cv_pthread:
        v.CC += " -pthread"
        if ac_cv_cxx_thread:
            v.CXX += " -pthread"
        v.posix_threads = True
    else:
        # Full fallback: probe pthread_create in various libs
        pyconf.define("_REENTRANT")

        # According to the POSIX spec, a pthreads implementation must
        # define _POSIX_THREADS in unistd.h. Some apparently don't
        # (e.g. gnu pth with pthread emulation)
        unistd_defines_pthreads = False
        pyconf.checking("for _POSIX_THREADS in unistd.h")
        unistd_defines_pthreads = pyconf.check_define(
            "unistd.h", "_POSIX_THREADS"
        )
        pyconf.result(unistd_defines_pthreads)

        prog = (
            "#include <stdio.h>\n#include <stdlib.h>\n#include <pthread.h>\n"
            "void *start_routine(void *arg) { exit(0); }\n"
            "int main(void) { pthread_create(NULL, NULL, start_routine, NULL); return 0; }\n"
        )
        # Just looking for pthread_create in libpthread is not enough:
        # on HP/UX, pthread.h renames pthread_create to a different symbol name.
        # So we really have to include pthread.h, and then link.
        pyconf.checking("for pthread_create in -lpthread")
        if pyconf.link_check(prog, extra_libs="-lpthread"):
            pyconf.result("yes")
            v.LIBS += " -lpthread"
            v.posix_threads = True
        else:
            pyconf.result("no")
            if pyconf.check_func("pthread_detach"):
                v.posix_threads = True
            elif pyconf.check_lib("pthreads", "pthread_create"):
                v.posix_threads = True
                v.LIBS += " -lpthreads"
            elif pyconf.check_lib("c_r", "pthread_create"):
                v.posix_threads = True
                v.LIBS += " -lc_r"
            elif pyconf.check_lib("pthread", "__pthread_create_system"):
                v.posix_threads = True
                v.LIBS += " -lpthread"
            elif pyconf.check_lib("cma", "pthread_create"):
                v.posix_threads = True
                v.LIBS += " -lcma"
            elif v.ac_sys_system == "WASI":
                v.posix_threads = "stub"
            else:
                pyconf.fatal("could not find pthreads on your system")

        if pyconf.check_lib("mpc", "usconfig"):
            v.LIBS += " -lmpc"

    if v.posix_threads is True:
        # Only define _POSIX_THREADS if we went through the fallback branch and
        # unistd.h doesn't define it there.  For the other branches (kpthread,
        # kthread, -pthread flag, pthread_is_default) the system headers already
        # expose threads, so we must NOT define _POSIX_THREADS.
        if unistd_defines_pthreads is False:
            pyconf.define(
                "_POSIX_THREADS",
                1,
                "Define if you have POSIX threads, and your system does not define that.",
            )

        # Bug 662787: Using semaphores causes unexplicable hangs on Solaris 8.
        sr = f"{v.ac_sys_system}/{v.ac_sys_release}"
        if sr == "SunOS/5.6":
            pyconf.define(
                "HAVE_PTHREAD_DESTRUCTOR",
                1,
                "Defined for Solaris 2.6 bug in pthread header.",
            )
        elif (
            sr in ("SunOS/5.8",)
            or v.ac_sys_system == "AIX"
            or v.ac_sys_system == "NetBSD"
        ):
            pyconf.define(
                "HAVE_BROKEN_POSIX_SEMAPHORES",
                1,
                "Define if the Posix semaphores do not work on your system",
            )

        # PTHREAD_SCOPE_SYSTEM check
        pyconf.checking("if PTHREAD_SCOPE_SYSTEM is supported")
        ac_cv_pthread_system_supported = pyconf.run_check(
            "#include <stdio.h>\n#include <pthread.h>\n"
            "void *foo(void *parm) { return NULL; }\n"
            "int main(void) {\n"
            "  pthread_attr_t attr; pthread_t id;\n"
            "  if (pthread_attr_init(&attr)) return -1;\n"
            "  if (pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM)) return -1;\n"
            "  if (pthread_create(&id, &attr, foo, NULL)) return -1;\n"
            "  if (pthread_join(id, NULL)) return -1;\n"
            "  return 0;\n"
            "}\n",
            default=False,
        )
        pyconf.result(ac_cv_pthread_system_supported)
        if ac_cv_pthread_system_supported:
            pyconf.define(
                "PTHREAD_SYSTEM_SCHED_SUPPORTED",
                1,
                "Defined if PTHREAD_SCOPE_SYSTEM supported.",
            )

        if pyconf.check_func("pthread_sigmask"):
            if v.ac_sys_system.startswith("CYGWIN"):
                pyconf.define(
                    "HAVE_BROKEN_PTHREAD_SIGMASK",
                    1,
                    "Define if pthread_sigmask() does not work on your system.",
                )
        pyconf.check_func("pthread_getcpuclockid")

    if v.posix_threads == "stub":
        pyconf.define(
            "HAVE_PTHREAD_STUBS",
            1,
            "Define if platform requires stubbed pthreads support",
        )


def check_posix_semaphores(v):
    # ---------------------------------------------------------------------------
    # POSIX semaphores
    # ---------------------------------------------------------------------------

    # For multiprocessing module, check that sem_open
    # actually works.  For FreeBSD versions <= 7.2,
    # the kernel module that provides POSIX semaphores
    # isn't loaded by default, so an attempt to call
    # sem_open results in a 'Signal 12' error.
    pyconf.checking("whether POSIX semaphores are enabled")
    ac_cv_posix_semaphores_enabled = pyconf.run_check(
        r"""
    #include <unistd.h>
    #include <fcntl.h>
    #include <stdio.h>
    #include <semaphore.h>
    #include <sys/stat.h>

    int main(void) {
        sem_t *a = sem_open("/autoconf", O_CREAT, S_IRUSR|S_IWUSR, 0);
        if (a == SEM_FAILED) {
            perror("sem_open");
            return 1;
        }
        sem_close(a);
        sem_unlink("/autoconf");
        return 0;
    }
    """,
        cache_var="ac_cv_posix_semaphores_enabled",
        cross_compiling_default=True,
    )
    pyconf.result(ac_cv_posix_semaphores_enabled)
    if not ac_cv_posix_semaphores_enabled:
        pyconf.define(
            "POSIX_SEMAPHORES_NOT_ENABLED",
            1,
            "Define if POSIX semaphores aren't enabled on your system",
        )

    pyconf.checking("for broken sem_getvalue")
    ac_cv_broken_sem_getvalue = pyconf.run_check(
        r"""
    #include <unistd.h>
    #include <fcntl.h>
    #include <stdio.h>
    #include <semaphore.h>
    #include <sys/stat.h>

    int main(void){
        sem_t *a = sem_open("/autocftw", O_CREAT, S_IRUSR|S_IWUSR, 0);
        int count;
        int res;
        if(a==SEM_FAILED){
            perror("sem_open");
            return 1;
        }
        res = sem_getvalue(a, &count);
        sem_close(a);
        sem_unlink("/autocftw");
        return res==-1 ? 1 : 0;
    }
    """,
        cache_var="ac_cv_broken_sem_getvalue",
        cross_compiling_default=True,
        on_success_return=False,
        on_failure_return=True,
    )
    pyconf.result(ac_cv_broken_sem_getvalue)
    if ac_cv_broken_sem_getvalue:
        pyconf.define(
            "HAVE_BROKEN_SEM_GETVALUE",
            1,
            "define to 1 if your sem_getvalue is broken.",
        )

    # RTLD flags (check for declarations)
    pyconf.check_decls(
        [
            "RTLD_LAZY",
            "RTLD_NOW",
            "RTLD_GLOBAL",
            "RTLD_LOCAL",
            "RTLD_NODELETE",
            "RTLD_NOLOAD",
            "RTLD_DEEPBIND",
            "RTLD_MEMBER",
        ],
        extra_includes=["dlfcn.h"],
    )


def setup_thread_headers_and_srcdirs(v):
    # ---------------------------------------------------------------------------
    # THREADHEADERS and SRCDIRS
    # ---------------------------------------------------------------------------

    thread_headers = pyconf.glob_files(
        pyconf.path_join([pyconf.srcdir, "Python", "thread_*.h"])
    )
    header_refs = []
    for h in sorted(thread_headers):
        header_refs.append(f"$(srcdir)/{pyconf.relpath(h, pyconf.srcdir)}")
    v.export("THREADHEADERS", " ".join(header_refs))

    v.export(
        "SRCDIRS",
        "  Modules"
        "  Modules/_ctypes"
        "  Modules/_decimal"
        "  Modules/_decimal/libmpdec"
        "  Modules/_hacl"
        "  Modules/_io"
        "  Modules/_multiprocessing"
        "  Modules/_remote_debugging"
        "  Modules/_sqlite"
        "  Modules/_sre"
        "  Modules/_testcapi"
        "  Modules/_testinternalcapi"
        "  Modules/_testlimitedcapi"
        "  Modules/_xxtestfuzz"
        "  Modules/_zstd"
        "  Modules/cjkcodecs"
        "  Modules/expat"
        "  Objects"
        "  Objects/mimalloc"
        "  Objects/mimalloc/prim"
        "  Parser"
        "  Parser/tokenizer"
        "  Parser/lexer"
        "  Programs"
        "  Python"
        "  Python/frozen_modules",
    )

    pyconf.checking("for build directories")
    for dir in v.SRCDIRS.split():
        pyconf.mkdir_p(dir)
    pyconf.result("done")


def check_thread_name_maxlen(v):
    # ---------------------------------------------------------------------------
    # Thread name max length
    # ---------------------------------------------------------------------------

    # gh-59705: Maximum length in bytes of a thread name
    sys = v.ac_sys_system
    if sys.startswith("Linux"):
        v._PYTHREAD_NAME_MAXLEN = "15"  # Linux and Android
    elif sys.startswith("SunOS"):
        v._PYTHREAD_NAME_MAXLEN = "31"
    elif sys.startswith("NetBSD"):
        v._PYTHREAD_NAME_MAXLEN = "15"  # gh-131268
    elif sys in ("Darwin", "iOS"):
        v._PYTHREAD_NAME_MAXLEN = "63"
    elif sys.startswith("FreeBSD"):
        v._PYTHREAD_NAME_MAXLEN = "19"  # gh-131268
    elif sys.startswith("OpenBSD"):
        v._PYTHREAD_NAME_MAXLEN = "23"  # gh-131268
    else:
        v._PYTHREAD_NAME_MAXLEN = ""

    if v._PYTHREAD_NAME_MAXLEN:
        pyconf.define(
            "_PYTHREAD_NAME_MAXLEN",
            int(v._PYTHREAD_NAME_MAXLEN),
            "Maximum length in bytes of a thread name",
        )
    v.export("_PYTHREAD_NAME_MAXLEN")
