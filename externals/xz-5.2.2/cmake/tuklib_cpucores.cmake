#
# tuklib_cpucores.cmake - see tuklib_cpucores.m4 for description and comments
#
# Author: Lasse Collin
#
# This file has been put into the public domain.
# You can do whatever you want with this file.
#

include("${CMAKE_CURRENT_LIST_DIR}/tuklib_common.cmake")
include(CheckCSourceCompiles)
include(CheckIncludeFile)

function(tuklib_cpucores_internal_check)
    if(WIN32 OR CYGWIN)
        # Nothing to do, the tuklib_cpucores.c handles it.
        set(TUKLIB_CPUCORES_DEFINITIONS "" CACHE INTERNAL "")
        return()
    endif()

    # glibc-based systems (GNU/Linux and GNU/kFreeBSD) have
    # sched_getaffinity(). The CPU_COUNT() macro was added in glibc 2.9.
    # glibc 2.9 is old enough that if someone uses the code on older glibc,
    # the fallback to sysconf() should be good enough.
    #
    # NOTE: This required that _GNU_SOURCE is defined. We assume that whatever
    #       feature test macros the caller wants to use are already set in
    #       CMAKE_REQUIRED_DEFINES and in the target defines.
    check_c_source_compiles("
            #include <sched.h>
            int main(void)
            {
                cpu_set_t cpu_mask;
                sched_getaffinity(0, sizeof(cpu_mask), &cpu_mask);
                return CPU_COUNT(&cpu_mask);
            }
        "
        TUKLIB_CPUCORES_SCHED_GETAFFINITY)
    if(TUKLIB_CPUCORES_SCHED_GETAFFINITY)
        set(TUKLIB_CPUCORES_DEFINITIONS
            "TUKLIB_CPUCORES_SCHED_GETAFFINITY"
            CACHE INTERNAL "")
        return()
    endif()

    # FreeBSD has both cpuset and sysctl. Look for cpuset first because
    # it's a better approach.
    #
    # This test would match on GNU/kFreeBSD too but it would require
    # -lfreebsd-glue when linking and thus in the current form this would
    # fail on GNU/kFreeBSD. The above test for sched_getaffinity() matches
    # on GNU/kFreeBSD so the test below should never run on that OS.
    check_c_source_compiles("
            #include <sys/param.h>
            #include <sys/cpuset.h>
            int main(void)
            {
                cpuset_t set;
                cpuset_getaffinity(CPU_LEVEL_WHICH, CPU_WHICH_PID, -1,
                                   sizeof(set), &set);
                return 0;
            }
        "
        TUKLIB_CPUCORES_CPUSET)
    if(TUKLIB_CPUCORES_CPUSET)
        set(TUKLIB_CPUCORES_DEFINITIONS "HAVE_PARAM_H;TUKLIB_CPUCORES_CPUSET"
            CACHE INTERNAL "")
        return()
    endif()

    # On OS/2, both sysconf() and sysctl() pass the tests in this file,
    # but only sysctl() works. On QNX it's the opposite: only sysconf() works
    # (although it assumes that _POSIX_SOURCE, _XOPEN_SOURCE, and
    # _POSIX_C_SOURCE are undefined or alternatively _QNX_SOURCE is defined).
    #
    # We test sysctl() first and intentionally break the sysctl() test on QNX
    # so that sysctl() is never used on QNX.
    check_include_file(sys/param.h HAVE_SYS_PARAM_H)
    if(HAVE_SYS_PARAM_H)
        list(APPEND CMAKE_REQUIRED_DEFINITIONS -DHAVE_SYS_PARAM_H)
    endif()
    check_c_source_compiles("
            #ifdef __QNX__
            compile error
            #endif
            #ifdef HAVE_SYS_PARAM_H
            #   include <sys/param.h>
            #endif
            #include <sys/sysctl.h>
            int main(void)
            {
                int name[2] = { CTL_HW, HW_NCPU };
                int cpus;
                size_t cpus_size = sizeof(cpus);
                sysctl(name, 2, &cpus, &cpus_size, NULL, 0);
                return 0;
            }
        "
        TUKLIB_CPUCORES_SYSCTL)
    if(TUKLIB_CPUCORES_SYSCTL)
        if(HAVE_SYS_PARAM_H)
            set(TUKLIB_CPUCORES_DEFINITIONS
                "HAVE_PARAM_H;TUKLIB_CPUCORES_SYSCTL"
                CACHE INTERNAL "")
        else()
            set(TUKLIB_CPUCORES_DEFINITIONS
                "TUKLIB_CPUCORES_SYSCTL"
                CACHE INTERNAL "")
        endif()
        return()
    endif()

    # Many platforms support sysconf().
    check_c_source_compiles("
            #include <unistd.h>
            int main(void)
            {
                long i;
            #ifdef _SC_NPROCESSORS_ONLN
                /* Many systems using sysconf() */
                i = sysconf(_SC_NPROCESSORS_ONLN);
            #else
                /* IRIX */
                i = sysconf(_SC_NPROC_ONLN);
            #endif
                return 0;
            }
        "
        TUKLIB_CPUCORES_SYSCONF)
    if(TUKLIB_CPUCORES_SYSCONF)
        set(TUKLIB_CPUCORES_DEFINITIONS "TUKLIB_CPUCORES_SYSCONF"
            CACHE INTERNAL "")
        return()
    endif()

    # HP-UX
    check_c_source_compiles("
            #include <sys/param.h>
            #include <sys/pstat.h>
            int main(void)
            {
                struct pst_dynamic pst;
                pstat_getdynamic(&pst, sizeof(pst), 1, 0);
                (void)pst.psd_proc_cnt;
                return 0;
            }
        "
        TUKLIB_CPUCORES_PSTAT_GETDYNAMIC)
    if(TUKLIB_CPUCORES_PSTAT_GETDYNAMIC)
        set(TUKLIB_CPUCORES_DEFINITIONS "TUKLIB_CPUCORES_PSTAT_GETDYNAMIC"
            CACHE INTERNAL "")
        return()
    endif()
endfunction()

function(tuklib_cpucores TARGET_OR_ALL)
    if(NOT DEFINED CACHE{TUKLIB_CPUCORES_FOUND})
        message(STATUS
                "Checking how to detect the number of available CPU cores")
        tuklib_cpucores_internal_check()

        if(DEFINED CACHE{TUKLIB_CPUCORES_DEFINITIONS})
            set(TUKLIB_CPUCORES_FOUND 1 CACHE INTERNAL "")
        else()
            set(TUKLIB_CPUCORES_FOUND 0 CACHE INTERNAL "")
            message(WARNING
                    "No method to detect the number of CPU cores was found")
        endif()
    endif()

    if(TUKLIB_CPUCORES_FOUND)
        tuklib_add_definitions("${TARGET_OR_ALL}"
                               "${TUKLIB_CPUCORES_DEFINITIONS}")
    endif()
endfunction()
