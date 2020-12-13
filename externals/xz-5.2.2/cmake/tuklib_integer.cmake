#
# tuklib_integer.cmake - see tuklib_integer.m4 for description and comments
#
# Author: Lasse Collin
#
# This file has been put into the public domain.
# You can do whatever you want with this file.
#

include("${CMAKE_CURRENT_LIST_DIR}/tuklib_common.cmake")
include(TestBigEndian)
include(CheckCSourceCompiles)
include(CheckIncludeFile)
include(CheckSymbolExists)

function(tuklib_integer TARGET_OR_ALL)
    # Check for endianness. Unlike the Autoconf's AC_C_BIGENDIAN, this doesn't
    # support Apple universal binaries. The CMake module will leave the
    # variable unset so we can catch that situation here instead of continuing
    # as if we were little endian.
    test_big_endian(WORDS_BIGENDIAN)
    if(NOT DEFINED WORDS_BIGENDIAN)
        message(FATAL_ERROR "Cannot determine endianness")
    endif()
    tuklib_add_definition_if("${TARGET_OR_ALL}" WORDS_BIGENDIAN)

    # Look for a byteswapping method.
    check_c_source_compiles("
            int main(void)
            {
                __builtin_bswap16(1);
                __builtin_bswap32(1);
                __builtin_bswap64(1);
                return 0;
            }
        "
        HAVE___BUILTIN_BSWAPXX)
    if(HAVE___BUILTIN_BSWAPXX)
        tuklib_add_definitions("${TARGET_OR_ALL}" HAVE___BUILTIN_BSWAPXX)
    else()
        check_include_file(byteswap.h HAVE_BYTESWAP_H)
        if(HAVE_BYTESWAP_H)
            tuklib_add_definitions("${TARGET_OR_ALL}" HAVE_BYTESWAP_H)
            check_symbol_exists(bswap_16 byteswap.h HAVE_BSWAP_16)
            tuklib_add_definition_if("${TARGET_OR_ALL}" HAVE_BSWAP_16)
            check_symbol_exists(bswap_32 byteswap.h HAVE_BSWAP_32)
            tuklib_add_definition_if("${TARGET_OR_ALL}" HAVE_BSWAP_32)
            check_symbol_exists(bswap_64 byteswap.h HAVE_BSWAP_64)
            tuklib_add_definition_if("${TARGET_OR_ALL}" HAVE_BSWAP_64)
        else()
            check_include_file(sys/endian.h HAVE_SYS_ENDIAN_H)
            if(HAVE_SYS_ENDIAN_H)
                tuklib_add_definitions("${TARGET_OR_ALL}" HAVE_SYS_ENDIAN_H)
            else()
                check_include_file(sys/byteorder.h HAVE_SYS_BYTEORDER_H)
                tuklib_add_definition_if("${TARGET_OR_ALL}"
                                         HAVE_SYS_BYTEORDER_H)
            endif()
        endif()
    endif()

    # 16-bit and 32-bit unaligned access is fast on x86(-64),
    # big endian PowerPC, and usually on 32/64-bit ARM too.
    # There are others too and ARM could be a false match.
    #
    # Guess the default value for the option.
    # CMake's ability to give info about the target arch seems bad.
    # The the same arch can have different name depending on the OS.
    #
    # FIXME: The regex is based on guessing, not on factual information!
    #
    # NOTE: Compared to the Autoconf test, this lacks the GCC/Clang test
    #       on ARM and always assumes that unaligned is fast on ARM.
    set(FAST_UNALIGNED_GUESS OFF)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES
       "[Xx3456]86|^[Xx]64|^[Aa][Mm][Dd]64|^[Aa][Rr][Mm]|^aarch|^powerpc|^ppc")
        if(NOT WORDS_BIGENDIAN OR
           NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^powerpc|^ppc")
           set(FAST_UNALIGNED_GUESS ON)
        endif()
    endif()
    option(TUKLIB_FAST_UNALIGNED_ACCESS
           "Enable if the system supports *fast* unaligned memory access \
with 16-bit and 32-bit integers."
           "${FAST_UNALIGNED_GUESS}")
    tuklib_add_definition_if("${TARGET_OR_ALL}" TUKLIB_FAST_UNALIGNED_ACCESS)

    # Unsafe type punning:
    option(TUKLIB_USE_UNSAFE_TYPE_PUNNING
           "This introduces strict aliasing violations and \
may result in broken code. However, this might improve performance \
in some cases, especially with old compilers \
(e.g. GCC 3 and early 4.x on x86, GCC < 6 on ARMv6 and ARMv7)."
           OFF)
    tuklib_add_definition_if("${TARGET_OR_ALL}" TUKLIB_USE_UNSAFE_TYPE_PUNNING)

    # Check for GCC/Clang __builtin_assume_aligned().
    check_c_source_compiles(
        "int main(void) { __builtin_assume_aligned(\"\", 1); return 0; }"
        HAVE___BUILTIN_ASSUME_ALIGNED)
    tuklib_add_definition_if("${TARGET_OR_ALL}" HAVE___BUILTIN_ASSUME_ALIGNED)
endfunction()
