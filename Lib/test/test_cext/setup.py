# gh-91321: Build a basic C or C++ test extension module to check that the
# Python C API does not emit compiler warnings.
import os
import shlex
import sys
import sysconfig
from test import support

from setuptools import setup, Extension

SOURCE = {
    'C': 'extension.c',
    'C++': 'extension.cpp',
}

MSVC = support.MS_WINDOWS


### C flags #################################################################

if not MSVC:
    # C compiler flags for GCC and clang
    BASE_CFLAGS = [
        # The purpose of test_cext extension is to check that building a C
        # extension using the Python C API does not emit C compiler warnings.
        '-Werror',
        # Enable extra checks for header files, which:
        #  - need to be enabled somewhere inside Python headers (rather than
        #    before including Python.h)
        #  - should not be checked for user code
        '-D_Py_IS_TESTCEXT',
    ]

    # C compiler flags for GCC and clang
    PUBLIC_CFLAGS = [
        *BASE_CFLAGS,

        # gh-120593: Check the 'const' qualifier
        '-Wcast-qual',

        # Ask for strict(er) compliance with the standard
        '-pedantic-errors',
    ]
    if not support.Py_GIL_DISABLED:
        PUBLIC_CFLAGS.append(
            # gh-116869: The Python C API must be compatible with building
            # with the -Werror=declaration-after-statement compiler flag.
            '-Werror=declaration-after-statement',
        )
    INTERNAL_CFLAGS = [*BASE_CFLAGS]
else:
    # MSVC compiler flags
    BASE_CFLAGS = [
        # Treat all compiler warnings as compiler errors
        '/WX',
    ]
    PUBLIC_CFLAGS = [
        *BASE_CFLAGS,
        # Display warnings level 1 to 4
        '/W4',
    ]
    INTERNAL_CFLAGS = [
        *BASE_CFLAGS,
        # Display warnings level 1 to 3
        '/W3',
    ]


### C++ flags ###############################################################

if not MSVC:
    # C++ compiler flags for GCC and clang
    CPPFLAGS = [
        # gh-91321: The purpose of _testcppext extension is to check that building
        # a C++ extension using the Python C API does not emit C++ compiler
        # warnings
        '-Werror',
    ]

    CPPFLAGS_PEDANTIC = [
        # Ask for strict(er) compliance with the standard.
        # We cannot do this for c++03 unlimited API, since several headers in
        # Include/cpython/ use commas at end of `enum` declarations, a C++11
        # feature for which GCC has no narrower option than -Wpedantic itself.
        '-pedantic-errors',

        # We also use `long long`, a C++11 feature we can enable individually.
        '-Wno-long-long',
    ]
else:
    # MSVC compiler flags
    CPPFLAGS = [
        # Display warnings level 1 to 4
        '/W4',
        # Treat all compiler warnings as compiler errors
        '/WX',
    ]
    CPPFLAGS_PEDANTIC = []


def main():
    module_name = os.environ["CPYTHON_TEST_EXT_NAME"]
    language = os.environ.get("CPYTHON_TEST_LANG", "C")
    std = os.environ.get("CPYTHON_TEST_STD", "")
    limited = bool(os.environ.get("CPYTHON_TEST_LIMITED", ""))
    abi3t = bool(os.environ.get("CPYTHON_TEST_ABI3T", ""))
    internal = bool(int(os.environ.get("CPYTHON_TEST_INTERNAL_C_API", "0")))
    incdirs = os.environ.get("CPYTHON_TEST_EXTRA_INCDIRS", "")
    libdirs = os.environ.get("CPYTHON_TEST_EXTRA_LIBDIRS", "")
    extra_cflags = os.environ.get("CPYTHON_TEST_EXTRA_CFLAGS", "")

    source = SOURCE[language]

    if language == 'C++':
        flags = list(CPPFLAGS)
    else:
        if not internal:
            flags = list(PUBLIC_CFLAGS)
        else:
            flags = list(INTERNAL_CFLAGS)
    flags.append(f'-DMODULE_NAME={module_name}')

    # Add -std=STD or /std:STD (MSVC) compiler flag
    if std:
        if MSVC:
            flags.append(f'/std:{std}')
        else:
            flags.append(f'-std={std}')

    if language == 'C++' and (limited or (std != 'c++03') and not internal):
        # See CPPFLAGS_PEDANTIC docstring
        flags.extend(CPPFLAGS_PEDANTIC)

    # gh-105776: When "gcc -std=11" is used as the C++ compiler, -std=c11
    # option emits a C++ compiler warning. Remove "-std11" option from the
    # CC command.
    cmd = (sysconfig.get_config_var('CC') or '')
    if cmd is not None:
        if support.MS_WINDOWS:
            std_prefix = '/std'
        else:
            std_prefix = '-std'
        cmd = shlex.split(cmd)
        cmd = [arg for arg in cmd if not arg.startswith(std_prefix)]
        cmd = shlex.join(cmd)
        # CC env var overrides sysconfig CC variable in setuptools
        os.environ['CC'] = cmd

    # Define opt-in macros
    if limited:
        flags.append(f'-DPy_LIMITED_API={sys.hexversion:#x}')
    if abi3t:
        flags.append(f'-DPy_TARGET_ABI3T={sys.hexversion:#x}')
    if internal:
        flags.append('-DTEST_INTERNAL_C_API=1')
    if extra_cflags:
        flags.extend(shlex.split(extra_cflags))

    # Add additional include and library directories, typically for in-tree
    # testing where not all directories are inferred
    include_dirs = []
    library_dirs = []
    if incdirs:
        print("Add incdirs:", incdirs)
        include_dirs.extend(incdirs.split(os.pathsep))
    if libdirs:
        print("Add libdirs:", libdirs)
        library_dirs.extend(libdirs.split(os.pathsep))

    # Display information to help debugging
    print(f"Language: {language}")
    print(f"Source: {source}")
    for env_name in ('CC', 'CXX', 'CFLAGS', 'CPPFLAGS', 'CXXFLAGS'):
        if env_name in os.environ:
            print(f"{env_name} env var: {os.environ[env_name]!r}")
        else:
            print(f"{env_name} env var: <missing>")
    print(f"extra_compile_args: {flags!r}")

    ext = Extension(
        module_name,
        sources=[source],
        extra_compile_args=flags,
        include_dirs=include_dirs,
        library_dirs=library_dirs)
    setup(name=f'internal_{module_name}',
          version='0.0',
          ext_modules=[ext])


if __name__ == "__main__":
    main()
