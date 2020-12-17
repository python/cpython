#############################################################################
#
# Very limited CMake support for building some parts of XZ Utils
#
# For now, this is indented to be useful to build static or shared liblzma
# on Windows with MSVC (to avoid the need to maintain Visual Studio project
# files). Building liblzma on a few other platforms should work too but it
# is somewhat experimental and not as portable as using ./configure.
#
# On some platforms this builds also xz and xzdec, but these are
# highly experimental and meant for testing only:
#   - No large file support on those 32-bit platforms that need it
#   - No replacement getopt_long(), libc must have it
#   - No sandboxing support
#   - No translations
#   - No xz symlinks are installed
#
# Other missing things:
#   - No xzgrep or other scripts or their symlinks
#   - No tests (no test failures either!)
#
# NOTE: Even if the code compiles without warnings, the end result may be
# different than via ./configure. Specifically, the list of #defines
# may be different (if so, probably this CMakeLists.txt got them wrong).
#
# This file provides the following installation components (if you only
# need liblzma, install only its components!):
#   - liblzma_Runtime
#   - liblzma_Development
#   - xz (on some platforms only)
#   - xzdec (on some platforms only)
#
# To find the target liblzma::liblzma from other packages, use the CONFIG
# option with find_package() to avoid a conflict with the FindLibLZMA module
# with case-insensitive file systems. For example, to require liblzma 5.2.5
# or a newer compatible version:
#
#     find_package(liblzma 5.2.5 REQUIRED CONFIG)
#     target_link_libraries(my_application liblzma::liblzma)
#
#############################################################################
#
# Author: Lasse Collin
#
# This file has been put into the public domain.
# You can do whatever you want with this file.
#
#############################################################################

cmake_minimum_required(VERSION 3.13...3.16 FATAL_ERROR)

include(CheckSymbolExists)
include(CheckStructHasMember)
include(cmake/tuklib_integer.cmake)
include(cmake/tuklib_cpucores.cmake)
include(cmake/tuklib_physmem.cmake)
include(cmake/tuklib_progname.cmake)
include(cmake/tuklib_mbstr.cmake)

# Get the package version from version.h into XZ_VERSION variable.
file(READ src/liblzma/api/lzma/version.h XZ_VERSION)
string(REGEX REPLACE
"^.*\n\
#define LZMA_VERSION_MAJOR ([0-9]+)\n\
#define LZMA_VERSION_MINOR ([0-9]+)\n\
#define LZMA_VERSION_PATCH ([0-9]+)\n\
.*$"
       "\\1.\\2.\\3" XZ_VERSION "${XZ_VERSION}")

# Among other things, this gives us variables xz_VERSION and xz_VERSION_MAJOR.
project(xz VERSION "${XZ_VERSION}" LANGUAGES C)

# Definitions common to all targets:
add_compile_definitions(
    # Package info:
    PACKAGE_NAME="XZ Utils"
    PACKAGE_BUGREPORT="lasse.collin@tukaani.org"
    PACKAGE_URL="https://tukaani.org/xz/"

    # Features:
    HAVE_CHECK_CRC32
    HAVE_CHECK_CRC64
    HAVE_CHECK_SHA256
    HAVE_DECODERS
    HAVE_DECODER_ARM
    HAVE_DECODER_ARMTHUMB
    HAVE_DECODER_DELTA
    HAVE_DECODER_IA64
    HAVE_DECODER_LZMA1
    HAVE_DECODER_LZMA2
    HAVE_DECODER_POWERPC
    HAVE_DECODER_SPARC
    HAVE_DECODER_X86
    HAVE_ENCODERS
    HAVE_ENCODER_ARM
    HAVE_ENCODER_ARMTHUMB
    HAVE_ENCODER_DELTA
    HAVE_ENCODER_IA64
    HAVE_ENCODER_LZMA1
    HAVE_ENCODER_LZMA2
    HAVE_ENCODER_POWERPC
    HAVE_ENCODER_SPARC
    HAVE_ENCODER_X86
    HAVE_MF_BT2
    HAVE_MF_BT3
    HAVE_MF_BT4
    HAVE_MF_HC3
    HAVE_MF_HC4

    # Standard headers and types are available:
    HAVE_STDBOOL_H
    HAVE__BOOL
    HAVE_STDINT_H
    HAVE_INTTYPES_H

    # Disable assert() checks when no build type has been specified. Non-empty
    # build types like "Release" and "Debug" handle this by default.
    $<$<CONFIG:>:NDEBUG>
)

# _GNU_SOURCE and such definitions. This specific macro is special since
# it also adds the definitions to CMAKE_REQUIRED_DEFINITIONS.
tuklib_use_system_extensions(ALL)

# This is needed by liblzma and xz.
tuklib_integer(ALL)

# Check for clock_gettime(). Do this before checking for threading so
# that we know there if CLOCK_MONOTONIC is available.
if(NOT WIN32 AND NOT DEFINED CACHE{HAVE_CLOCK_GETTIME})
    check_symbol_exists(clock_gettime time.h HAVE_CLOCK_GETTIME)
    if(NOT HAVE_CLOCK_GETTIME)
        # With glibc <= 2.17 or Solaris 10 this needs librt.
        unset(HAVE_CLOCK_GETTIME CACHE)

        list(INSERT CMAKE_REQUIRED_LIBRARIES 0 rt)
        check_symbol_exists(clock_gettime time.h HAVE_CLOCK_GETTIME)

        # If it was found now, add it to all targets and keep it
        # in CMAKE_REQUIRED_LIBRARIES for further tests too.
        if(HAVE_CLOCK_GETTIME)
            link_libraries(rt)
        else()
            list(REMOVE_AT CMAKE_REQUIRED_LIBRARIES 0)
        endif()
    endif()
    if(HAVE_CLOCK_GETTIME)
        # Check if CLOCK_MONOTONIC is available for clock_gettime().
        check_symbol_exists(CLOCK_MONOTONIC time.h HAVE_DECL_CLOCK_MONOTONIC)

        # HAVE_DECL_CLOCK_MONOTONIC should always be defined to 0 or 1
        # when clock_gettime is available.
        add_compile_definitions(
            HAVE_CLOCK_GETTIME
            HAVE_DECL_CLOCK_MONOTONIC=$<BOOL:"${HAVE_DECL_CLOCK_MONOTONIC}">
        )
    endif()
endif()

# Threading support:
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
if(CMAKE_USE_WIN32_THREADS_INIT)
    add_compile_definitions(MYTHREAD_VISTA)
else()
    add_compile_definitions(MYTHREAD_POSIX)

    # Check if pthread_condattr_setclock() exists to use CLOCK_MONOTONIC.
    if(HAVE_DECL_CLOCK_MONOTONIC)
        list(INSERT CMAKE_REQUIRED_LIBRARIES 0 "${CMAKE_THREAD_LIBS_INIT}")
        check_symbol_exists(pthread_condattr_setclock pthread.h
                            HAVE_PTHREAD_CONDATTR_SETCLOCK)
        tuklib_add_definition_if(ALL HAVE_PTHREAD_CONDATTR_SETCLOCK)
    endif()
endif()

# Options for new enough GCC or Clang on any arch or operating system:
if(CMAKE_C_COMPILER_ID MATCHES GNU|Clang)
    # configure.ac has a long list but it won't be copied here:
    add_compile_options(-Wall -Wextra)
endif()


#############################################################################
# liblzma
#############################################################################

option(BUILD_SHARED_LIBS "Build liblzma as a shared library instead of static")

add_library(liblzma
    src/common/mythread.h
    src/common/sysdefs.h
    src/common/tuklib_common.h
    src/common/tuklib_config.h
    src/common/tuklib_cpucores.c
    src/common/tuklib_cpucores.h
    src/common/tuklib_integer.h
    src/common/tuklib_physmem.c
    src/common/tuklib_physmem.h
    src/liblzma/api/lzma.h
    src/liblzma/api/lzma/base.h
    src/liblzma/api/lzma/bcj.h
    src/liblzma/api/lzma/block.h
    src/liblzma/api/lzma/check.h
    src/liblzma/api/lzma/container.h
    src/liblzma/api/lzma/delta.h
    src/liblzma/api/lzma/filter.h
    src/liblzma/api/lzma/hardware.h
    src/liblzma/api/lzma/index.h
    src/liblzma/api/lzma/index_hash.h
    src/liblzma/api/lzma/lzma12.h
    src/liblzma/api/lzma/stream_flags.h
    src/liblzma/api/lzma/version.h
    src/liblzma/api/lzma/vli.h
    src/liblzma/check/check.c
    src/liblzma/check/check.h
    src/liblzma/check/crc32_fast.c
    src/liblzma/check/crc32_table.c
    src/liblzma/check/crc32_table_be.h
    src/liblzma/check/crc32_table_le.h
    src/liblzma/check/crc64_fast.c
    src/liblzma/check/crc64_table.c
    src/liblzma/check/crc64_table_be.h
    src/liblzma/check/crc64_table_le.h
    src/liblzma/check/crc_macros.h
    src/liblzma/check/sha256.c
    src/liblzma/common/alone_decoder.c
    src/liblzma/common/alone_decoder.h
    src/liblzma/common/alone_encoder.c
    src/liblzma/common/auto_decoder.c
    src/liblzma/common/block_buffer_decoder.c
    src/liblzma/common/block_buffer_encoder.c
    src/liblzma/common/block_buffer_encoder.h
    src/liblzma/common/block_decoder.c
    src/liblzma/common/block_decoder.h
    src/liblzma/common/block_encoder.c
    src/liblzma/common/block_encoder.h
    src/liblzma/common/block_header_decoder.c
    src/liblzma/common/block_header_encoder.c
    src/liblzma/common/block_util.c
    src/liblzma/common/common.c
    src/liblzma/common/common.h
    src/liblzma/common/easy_buffer_encoder.c
    src/liblzma/common/easy_decoder_memusage.c
    src/liblzma/common/easy_encoder.c
    src/liblzma/common/easy_encoder_memusage.c
    src/liblzma/common/easy_preset.c
    src/liblzma/common/easy_preset.h
    src/liblzma/common/file_info.c
    src/liblzma/common/filter_buffer_decoder.c
    src/liblzma/common/filter_buffer_encoder.c
    src/liblzma/common/filter_common.c
    src/liblzma/common/filter_common.h
    src/liblzma/common/filter_decoder.c
    src/liblzma/common/filter_decoder.h
    src/liblzma/common/filter_encoder.c
    src/liblzma/common/filter_encoder.h
    src/liblzma/common/filter_flags_decoder.c
    src/liblzma/common/filter_flags_encoder.c
    src/liblzma/common/hardware_cputhreads.c
    src/liblzma/common/hardware_physmem.c
    src/liblzma/common/index.c
    src/liblzma/common/index.h
    src/liblzma/common/index_decoder.c
    src/liblzma/common/index_decoder.h
    src/liblzma/common/index_encoder.c
    src/liblzma/common/index_encoder.h
    src/liblzma/common/index_hash.c
    src/liblzma/common/memcmplen.h
    src/liblzma/common/outqueue.c
    src/liblzma/common/outqueue.h
    src/liblzma/common/stream_buffer_decoder.c
    src/liblzma/common/stream_buffer_encoder.c
    src/liblzma/common/stream_decoder.c
    src/liblzma/common/stream_decoder.h
    src/liblzma/common/stream_encoder.c
    src/liblzma/common/stream_encoder_mt.c
    src/liblzma/common/stream_flags_common.c
    src/liblzma/common/stream_flags_common.h
    src/liblzma/common/stream_flags_decoder.c
    src/liblzma/common/stream_flags_encoder.c
    src/liblzma/common/vli_decoder.c
    src/liblzma/common/vli_encoder.c
    src/liblzma/common/vli_size.c
    src/liblzma/delta/delta_common.c
    src/liblzma/delta/delta_common.h
    src/liblzma/delta/delta_decoder.c
    src/liblzma/delta/delta_decoder.h
    src/liblzma/delta/delta_encoder.c
    src/liblzma/delta/delta_encoder.h
    src/liblzma/delta/delta_private.h
    src/liblzma/lz/lz_decoder.c
    src/liblzma/lz/lz_decoder.h
    src/liblzma/lz/lz_encoder.c
    src/liblzma/lz/lz_encoder.h
    src/liblzma/lz/lz_encoder_hash.h
    src/liblzma/lz/lz_encoder_hash_table.h
    src/liblzma/lz/lz_encoder_mf.c
    src/liblzma/lzma/fastpos.h
    src/liblzma/lzma/fastpos_table.c
    src/liblzma/lzma/lzma2_decoder.c
    src/liblzma/lzma/lzma2_decoder.h
    src/liblzma/lzma/lzma2_encoder.c
    src/liblzma/lzma/lzma2_encoder.h
    src/liblzma/lzma/lzma_common.h
    src/liblzma/lzma/lzma_decoder.c
    src/liblzma/lzma/lzma_decoder.h
    src/liblzma/lzma/lzma_encoder.c
    src/liblzma/lzma/lzma_encoder.h
    src/liblzma/lzma/lzma_encoder_optimum_fast.c
    src/liblzma/lzma/lzma_encoder_optimum_normal.c
    src/liblzma/lzma/lzma_encoder_presets.c
    src/liblzma/lzma/lzma_encoder_private.h
    src/liblzma/rangecoder/price.h
    src/liblzma/rangecoder/price_table.c
    src/liblzma/rangecoder/range_common.h
    src/liblzma/rangecoder/range_decoder.h
    src/liblzma/rangecoder/range_encoder.h
    src/liblzma/simple/arm.c
    src/liblzma/simple/armthumb.c
    src/liblzma/simple/ia64.c
    src/liblzma/simple/powerpc.c
    src/liblzma/simple/simple_coder.c
    src/liblzma/simple/simple_coder.h
    src/liblzma/simple/simple_decoder.c
    src/liblzma/simple/simple_decoder.h
    src/liblzma/simple/simple_encoder.c
    src/liblzma/simple/simple_encoder.h
    src/liblzma/simple/simple_private.h
    src/liblzma/simple/sparc.c
    src/liblzma/simple/x86.c
)

target_include_directories(liblzma PRIVATE
    src/liblzma/api
    src/liblzma/common
    src/liblzma/check
    src/liblzma/lz
    src/liblzma/rangecoder
    src/liblzma/lzma
    src/liblzma/delta
    src/liblzma/simple
    src/common
)

target_link_libraries(liblzma Threads::Threads)

# Put the tuklib functions under the lzma_ namespace.
target_compile_definitions(liblzma PRIVATE TUKLIB_SYMBOL_PREFIX=lzma_)
tuklib_cpucores(liblzma)
tuklib_physmem(liblzma)

# While liblzma can be built without tuklib_cpucores or tuklib_physmem
# modules, the liblzma API functions lzma_cputhreads() and lzma_physmem()
# will then be useless (which isn't too bad but still unfortunate). Since
# I expect the CMake-based builds to be only used on systems that are
# supported by these tuklib modules, problems with these tuklib modules
# are considered a hard error for now. This hopefully helps to catch bugs
# in the CMake versions of the tuklib checks.
if(NOT TUKLIB_CPUCORES_FOUND OR NOT TUKLIB_PHYSMEM_FOUND)
    # Use SEND_ERROR instead of FATAL_ERROR. If someone reports a bug,
    # seeing the results of the remaining checks can be useful too.
    message(SEND_ERROR
            "tuklib_cpucores() or tuklib_physmem() failed. "
            "Unless you really are building for a system where these "
            "modules are not supported (unlikely), this is a bug in the "
            "included cmake/tuklib_*.cmake files that should be fixed. "
            "To build anyway, edit this CMakeLists.txt to ignore this error.")
endif()

# immintrin.h:
include(CheckIncludeFile)
check_include_file(immintrin.h HAVE_IMMINTRIN_H)
if(HAVE_IMMINTRIN_H)
    target_compile_definitions(liblzma PRIVATE HAVE_IMMINTRIN_H)

    # SSE2 intrinsics:
    include(CheckCSourceCompiles)
    check_c_source_compiles("
            #include <immintrin.h>
            int main(void)
            {
                __m128i x = { 0 };
                _mm_movemask_epi8(x);
                return 0;
            }
        "
        HAVE__MM_MOVEMASK_EPI8)
    tuklib_add_definition_if(liblzma HAVE__MM_MOVEMASK_EPI8)
endif()

# Support -fvisiblity=hidden when building shared liblzma.
# These lines do nothing on Windows (even under Cygwin).
# HAVE_VISIBILITY should always be defined to 0 or 1.
if(BUILD_SHARED_LIBS)
    set_target_properties(liblzma PROPERTIES C_VISIBILITY_PRESET hidden)
    target_compile_definitions(liblzma PRIVATE HAVE_VISIBILITY=1)
else()
    target_compile_definitions(liblzma PRIVATE HAVE_VISIBILITY=0)
endif()

if(WIN32)
    if(BUILD_SHARED_LIBS)
        # Add the Windows resource file for liblzma.dll.
        target_sources(liblzma PRIVATE src/liblzma/liblzma_w32res.rc)

        # Export the public API symbols with __declspec(dllexport).
        target_compile_definitions(liblzma PRIVATE DLL_EXPORT)
    else()
        # Disable __declspec(dllimport) when linking against static liblzma.
        target_compile_definitions(liblzma INTERFACE LZMA_API_STATIC)
    endif()
elseif(CMAKE_SYSTEM_NAME MATCHES "^Linux$|^FreeBSD$")
    # Symbol versioning for shared liblzma. This doesn't affect static builds.
    target_link_options(liblzma PRIVATE
        "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/liblzma.map"
    )
    set_target_properties(liblzma PROPERTIES
        LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/liblzma/liblzma.map"
        LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/common/common_w32res.rc"
    )
endif()

set_target_properties(liblzma PROPERTIES
    # At least for now the package versioning matches the rules used for
    # shared library versioning (excluding development releases) so it is
    # fine to use the package version here.
    SOVERSION "${xz_VERSION_MAJOR}"
    VERSION "${xz_VERSION}"

    # It's liblzma.so or liblzma.dll, not libliblzma.so or lzma.dll.
    # Avoid the name lzma.dll because it would conflict with LZMA SDK.
    PREFIX ""
)

# Create liblzmaConfigVersion.cmake.
#
# FIXME: SameMajorVersion is correct for stable releases but it is wrong
# for development releases where each release may have incompatible changes.
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/liblzmaConfigVersion.cmake"
    VERSION "${liblzma_VERSION}"
    COMPATIBILITY SameMajorVersion)

# Create liblzmaConfig.cmake.
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/liblzmaConfig.cmake"
"include(CMakeFindDependencyMacro)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_dependency(Threads)
include(\"\${CMAKE_CURRENT_LIST_DIR}/liblzmaTargets.cmake\")
")

# Set CMAKE_INSTALL_LIBDIR and friends.
include(GNUInstallDirs)

# Install the library binary. The INCLUDES specifies the include path that
# is exported for other projects to use but it doesn't install any files.
install(TARGETS liblzma EXPORT liblzmaTargets
        RUNTIME  DESTINATION "${CMAKE_INSTALL_BINDIR}"
                 COMPONENT liblzma_Runtime
        LIBRARY  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
                 COMPONENT liblzma_Runtime
                 NAMELINK_COMPONENT liblzma_Development
        ARCHIVE  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
                 COMPONENT liblzma_Development
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

# Install the liblzma API headers. These use a subdirectory so
# this has to be done as a separate step.
install(DIRECTORY src/liblzma/api/
        COMPONENT liblzma_Development
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        FILES_MATCHING PATTERN "*.h")

# Install the CMake files that other packages can use to find liblzma.
set(liblzma_INSTALL_CMAKEDIR
    "${CMAKE_INSTALL_LIBDIR}/cmake/liblzma"
    CACHE STRING "Path to liblzma's .cmake files")

install(EXPORT liblzmaTargets
        NAMESPACE liblzma::
        FILE liblzmaTargets.cmake
        DESTINATION "${liblzma_INSTALL_CMAKEDIR}"
        COMPONENT liblzma_Development)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/liblzmaConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/liblzmaConfigVersion.cmake"
        DESTINATION "${liblzma_INSTALL_CMAKEDIR}"
        COMPONENT liblzma_Development)


#############################################################################
# getopt_long
#############################################################################

# The command line tools needs this.
check_symbol_exists(getopt_long getopt.h HAVE_GETOPT_LONG)


#############################################################################
# xzdec
#############################################################################

if(HAVE_GETOPT_LONG)
    add_executable(xzdec
        src/common/sysdefs.h
        src/common/tuklib_common.h
        src/common/tuklib_config.h
        src/common/tuklib_exit.c
        src/common/tuklib_exit.h
        src/common/tuklib_gettext.h
        src/common/tuklib_progname.c
        src/common/tuklib_progname.h
        src/xzdec/xzdec.c
    )

    target_include_directories(xzdec PRIVATE
        src/common
        src/liblzma/api
    )

    target_link_libraries(xzdec PRIVATE liblzma)

    tuklib_progname(xzdec)

    install(TARGETS xzdec
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
                    COMPONENT xzdec)

    if(UNIX)
        install(FILES src/xzdec/xzdec.1
                DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
                COMPONENT xzdec)
    endif()
endif()


#############################################################################
# xz
#############################################################################

if(NOT MSVC AND HAVE_GETOPT_LONG)
    add_executable(xz
        src/common/mythread.h
        src/common/sysdefs.h
        src/common/tuklib_common.h
        src/common/tuklib_config.h
        src/common/tuklib_exit.c
        src/common/tuklib_exit.h
        src/common/tuklib_gettext.h
        src/common/tuklib_integer.h
        src/common/tuklib_mbstr.h
        src/common/tuklib_mbstr_fw.c
        src/common/tuklib_mbstr_width.c
        src/common/tuklib_open_stdxxx.c
        src/common/tuklib_open_stdxxx.h
        src/common/tuklib_progname.c
        src/common/tuklib_progname.h
        src/xz/args.c
        src/xz/args.h
        src/xz/coder.c
        src/xz/coder.h
        src/xz/file_io.c
        src/xz/file_io.h
        src/xz/hardware.c
        src/xz/hardware.h
        src/xz/list.c
        src/xz/list.h
        src/xz/main.c
        src/xz/main.h
        src/xz/message.c
        src/xz/message.h
        src/xz/mytime.c
        src/xz/mytime.h
        src/xz/options.c
        src/xz/options.h
        src/xz/private.h
        src/xz/signals.c
        src/xz/signals.h
        src/xz/suffix.c
        src/xz/suffix.h
        src/xz/util.c
        src/xz/util.h
    )

    target_include_directories(xz PRIVATE
        src/common
        src/liblzma/api
    )

    target_link_libraries(xz PRIVATE liblzma)

    target_compile_definitions(xz PRIVATE ASSUME_RAM=128)

    tuklib_progname(xz)
    tuklib_mbstr(xz)

    check_symbol_exists(optreset getopt.h HAVE_OPTRESET)
    tuklib_add_definition_if(xz HAVE_OPTRESET)

    check_symbol_exists(posix_fadvise fcntl.h HAVE_POSIX_FADVISE)
    tuklib_add_definition_if(xz HAVE_POSIX_FADVISE)

    # How to get file time:
    check_struct_has_member("struct stat" st_atim.tv_nsec
                            "sys/types.h;sys/stat.h"
                            HAVE_STRUCT_STAT_ST_ATIM_TV_NSEC)
    if(HAVE_STRUCT_STAT_ST_ATIM_TV_NSEC)
        tuklib_add_definitions(xz HAVE_STRUCT_STAT_ST_ATIM_TV_NSEC)
    else()
        check_struct_has_member("struct stat" st_atimespec.tv_nsec
                                "sys/types.h;sys/stat.h"
                                HAVE_STRUCT_STAT_ST_ATIMESPEC_TV_NSEC)
        if(HAVE_STRUCT_STAT_ST_ATIMESPEC_TV_NSEC)
            tuklib_add_definitions(xz HAVE_STRUCT_STAT_ST_ATIMESPEC_TV_NSEC)
        else()
            check_struct_has_member("struct stat" st_atimensec
                                    "sys/types.h;sys/stat.h"
                                    HAVE_STRUCT_STAT_ST_ATIMENSEC)
            tuklib_add_definition_if(xz HAVE_STRUCT_STAT_ST_ATIMENSEC)
        endif()
    endif()

    # How to set file time:
    check_symbol_exists(futimens "sys/types.h;sys/stat.h" HAVE_FUTIMENS)
    if(HAVE_FUTIMENS)
        tuklib_add_definitions(xz HAVE_FUTIMENS)
    else()
        check_symbol_exists(futimes "sys/time.h" HAVE_FUTIMES)
        if(HAVE_FUTIMES)
            tuklib_add_definitions(xz HAVE_FUTIMES)
        else()
            check_symbol_exists(futimesat "sys/time.h" HAVE_FUTIMESAT)
            if(HAVE_FUTIMESAT)
                tuklib_add_definitions(xz HAVE_FUTIMESAT)
            else()
                check_symbol_exists(utimes "sys/time.h" HAVE_UTIMES)
                if(HAVE_UTIMES)
                    tuklib_add_definitions(xz HAVE_UTIMES)
                else()
                    check_symbol_exists(_futime "sys/utime.h" HAVE__FUTIME)
                    if(HAVE__FUTIME)
                        tuklib_add_definitions(xz HAVE__FUTIME)
                    else()
                        check_symbol_exists(utime "utime.h" HAVE_UTIME)
                        tuklib_add_definition_if(xz HAVE_UTIME)
                    endif()
                endif()
            endif()
        endif()
    endif()

    install(TARGETS xz
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
                    COMPONENT xz)

    install(FILES src/xz/xz.1
            DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
            COMPONENT xz)
endif()
