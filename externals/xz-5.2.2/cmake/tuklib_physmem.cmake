#
# tuklib_physmem.cmake - see tuklib_physmem.m4 for description and comments
#
# NOTE: Compared tuklib_physmem.m4, this lacks support for Tru64, IRIX, and
# Linux sysinfo() (usually sysconf() is used on GNU/Linux).
#
# Author: Lasse Collin
#
# This file has been put into the public domain.
# You can do whatever you want with this file.
#

include("${CMAKE_CURRENT_LIST_DIR}/tuklib_common.cmake")
include(CheckCSourceCompiles)
include(CheckIncludeFile)

function(tuklib_physmem_internal_check)
    # Shortcut on Windows:
    if(WIN32 OR CYGWIN)
        # Nothing to do, the tuklib_physmem.c handles it.
        set(TUKLIB_PHYSMEM_DEFINITIONS "" CACHE INTERNAL "")
        return()
    endif()

    # Full check for special cases:
    check_c_source_compiles("
            #if defined(_WIN32) || defined(__CYGWIN__) || defined(__OS2__) \
                || defined(__DJGPP__) || defined(__VMS) \
                || defined(AMIGA) || defined(__AROS__) || defined(__QNX__)
            int main(void) { return 0; }
            #else
            compile error
            #endif
        "
        TUKLIB_PHYSMEM_SPECIAL)
    if(TUKLIB_PHYSMEM_SPECIAL)
        # Nothing to do, the tuklib_physmem.c handles it.
        set(TUKLIB_PHYSMEM_DEFINITIONS "" CACHE INTERNAL "")
        return()
    endif()

    # Look for AIX-specific solution before sysconf(), because the test
    # for sysconf() will pass on AIX but won't actually work
    # (sysconf(_SC_PHYS_PAGES) compiles but always returns -1 on AIX).
    check_c_source_compiles("
            #include <sys/systemcfg.h>
            int main(void)
            {
                (void)_system_configuration.physmem;
                return 0;
            }
        "
        TUKLIB_PHYSMEM_AIX)
    if(TUKLIB_PHYSMEM_AIX)
        set(TUKLIB_PHYSMEM_DEFINITIONS "TUKLIB_PHYSMEM_AIX" CACHE INTERNAL "")
        return()
    endif()

    # sysconf()
    check_c_source_compiles("
            #include <unistd.h>
            int main(void)
            {
                long i;
                i = sysconf(_SC_PAGESIZE);
                i = sysconf(_SC_PHYS_PAGES);
                return 0;
            }
        "
        TUKLIB_PHYSMEM_SYSCONF)
    if(TUKLIB_PHYSMEM_SYSCONF)
        set(TUKLIB_PHYSMEM_DEFINITIONS "TUKLIB_PHYSMEM_SYSCONF"
            CACHE INTERNAL "")
        return()
    endif()

    # sysctl()
    check_include_file(sys/param.h HAVE_SYS_PARAM_H)
    if(HAVE_SYS_PARAM_H)
        list(APPEND CMAKE_REQUIRED_DEFINITIONS -DHAVE_SYS_PARAM_H)
    endif()

    check_c_source_compiles("
            #ifdef HAVE_SYS_PARAM_H
            #   include <sys/param.h>
            #endif
            #include <sys/sysctl.h>
            int main(void)
            {
                int name[2] = { CTL_HW, HW_PHYSMEM };
                unsigned long mem;
                size_t mem_ptr_size = sizeof(mem);
                sysctl(name, 2, &mem, &mem_ptr_size, NULL, 0);
                return 0;
            }
        "
        TUKLIB_PHYSMEM_SYSCTL)
    if(TUKLIB_PHYSMEM_SYSCTL)
        if(HAVE_SYS_PARAM_H)
            set(TUKLIB_PHYSMEM_DEFINITIONS
                "HAVE_PARAM_H;TUKLIB_PHYSMEM_SYSCTL"
                CACHE INTERNAL "")
        else()
            set(TUKLIB_PHYSMEM_DEFINITIONS
                "TUKLIB_PHYSMEM_SYSCTL"
                CACHE INTERNAL "")
        endif()
        return()
    endif()

    # HP-UX
    check_c_source_compiles("
            #include <sys/param.h>
            #include <sys/pstat.h>
            int main(void)
            {
                struct pst_static pst;
                pstat_getstatic(&pst, sizeof(pst), 1, 0);
                (void)pst.physical_memory;
                (void)pst.page_size;
                return 0;
            }
        "
        TUKLIB_PHYSMEM_PSTAT_GETSTATIC)
    if(TUKLIB_PHYSMEM_PSTAT_GETSTATIC)
        set(TUKLIB_PHYSMEM_DEFINITIONS "TUKLIB_PHYSMEM_PSTAT_GETSTATIC"
            CACHE INTERNAL "")
        return()
    endif()
endfunction()

function(tuklib_physmem TARGET_OR_ALL)
    if(NOT DEFINED CACHE{TUKLIB_PHYSMEM_FOUND})
        message(STATUS "Checking how to detect the amount of physical memory")
        tuklib_physmem_internal_check()

        if(DEFINED CACHE{TUKLIB_PHYSMEM_DEFINITIONS})
            set(TUKLIB_PHYSMEM_FOUND 1 CACHE INTERNAL "")
        else()
            set(TUKLIB_PHYSMEM_FOUND 0 CACHE INTERNAL "")
            message(WARNING
                "No method to detect the amount of physical memory was found")
        endif()
    endif()

    if(TUKLIB_PHYSMEM_FOUND)
        tuklib_add_definitions("${TARGET_OR_ALL}"
                               "${TUKLIB_PHYSMEM_DEFINITIONS}")
    endif()
endfunction()
