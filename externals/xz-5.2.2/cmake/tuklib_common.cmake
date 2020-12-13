#
# tuklib_common.cmake - common functions and macros for tuklib_*.cmake files
#
# Author: Lasse Collin
#
# This file has been put into the public domain.
# You can do whatever you want with this file.
#

function(tuklib_add_definitions TARGET_OR_ALL DEFINITIONS)
    # DEFINITIONS may be an empty string/list but it's fine here. There is
    # no need to quote ${DEFINITIONS} as empty arguments are fine here.
    if(TARGET_OR_ALL STREQUAL "ALL")
        add_compile_definitions(${DEFINITIONS})
    else()
        target_compile_definitions("${TARGET_OR_ALL}" PRIVATE ${DEFINITIONS})
    endif()
endfunction()

function(tuklib_add_definition_if TARGET_OR_ALL VAR)
    if(${VAR})
        tuklib_add_definitions("${TARGET_OR_ALL}" "${VAR}")
    endif()
endfunction()

# This is an over-simplified version of AC_USE_SYSTEM_EXTENSIONS in Autoconf
# or gl_USE_SYSTEM_EXTENSIONS in gnulib.
macro(tuklib_use_system_extensions TARGET_OR_ALL)
    if(NOT WIN32)
        # FIXME? The Solaris-specific __EXTENSIONS__ should be conditional
        #        even on Solaris. See gnulib: git log m4/extensions.m4.
        # FIXME? gnulib and autoconf.git has lots of new stuff.
        tuklib_add_definitions("${TARGET_OR_ALL}"
            _GNU_SOURCE
            __EXTENSIONS__
            _POSIX_PTHREAD_SEMANTICS
            _TANDEM_SOURCE
            _ALL_SOURCE
        )

        list(APPEND CMAKE_REQUIRED_DEFINITIONS
            -D_GNU_SOURCE
            -D__EXTENSIONS__
            -D_POSIX_PTHREAD_SEMANTICS
            -D_TANDEM_SOURCE
            -D_ALL_SOURCE
        )
    endif()
endmacro()
