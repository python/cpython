#
# tuklib_progname.cmake - see tuklib_progname.m4 for description and comments
#
# Author: Lasse Collin
#
# This file has been put into the public domain.
# You can do whatever you want with this file.
#

include("${CMAKE_CURRENT_LIST_DIR}/tuklib_common.cmake")
include(CheckSymbolExists)

function(tuklib_progname TARGET_OR_ALL)
    # NOTE: This glibc extension requires _GNU_SOURCE.
    check_symbol_exists(program_invocation_name errno.h
                        HAVE_DECL_PROGRAM_INVOCATION_NAME)
    tuklib_add_definition_if("${TARGET_OR_ALL}"
                             HAVE_DECL_PROGRAM_INVOCATION_NAME)
endfunction()
