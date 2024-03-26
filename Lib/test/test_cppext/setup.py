# gh-91321: Build a basic C++ test extension to check that the Python C API is
# compatible with C++ and does not emit C++ compiler warnings.
import os
import platform
import shlex
import sysconfig
from test import support

from setuptools import setup, Extension


SOURCE = 'extension.cpp'
if not support.MS_WINDOWS:
    # C++ compiler flags for GCC and clang
    CPPFLAGS = [
        # gh-91321: The purpose of _testcppext extension is to check that building
        # a C++ extension using the Python C API does not emit C++ compiler
        # warnings
        '-Werror',
    ]
else:
    # Don't pass any compiler flag to MSVC
    CPPFLAGS = []


def main():
    cppflags = list(CPPFLAGS)
    std = os.environ.get("CPYTHON_TEST_CPP_STD", "")
    module_name = os.environ["CPYTHON_TEST_EXT_NAME"]

    cppflags = list(CPPFLAGS)
    cppflags.append(f'-DMODULE_NAME={module_name}')

    # Add -std=STD or /std:STD (MSVC) compiler flag
    if std:
        if support.MS_WINDOWS:
            cppflags.append(f'/std:{std}')
        else:
            cppflags.append(f'-std={std}')

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

    # On Windows, add PCbuild\amd64\ to include and library directories
    include_dirs = []
    library_dirs = []
    if support.MS_WINDOWS:
        srcdir = sysconfig.get_config_var('srcdir')
        machine = platform.uname().machine
        pcbuild = os.path.join(srcdir, 'PCbuild', machine)
        if os.path.exists(pcbuild):
            # pyconfig.h is generated in PCbuild\amd64\
            include_dirs.append(pcbuild)
            # python313.lib is generated in PCbuild\amd64\
            library_dirs.append(pcbuild)
            print(f"Add PCbuild directory: {pcbuild}")

    # Display information to help debugging
    for env_name in ('CC', 'CFLAGS', 'CPPFLAGS'):
        if env_name in os.environ:
            print(f"{env_name} env var: {os.environ[env_name]!r}")
        else:
            print(f"{env_name} env var: <missing>")
    print(f"extra_compile_args: {cppflags!r}")

    ext = Extension(
        module_name,
        sources=[SOURCE],
        language='c++',
        extra_compile_args=cppflags,
        include_dirs=include_dirs,
        library_dirs=library_dirs)
    setup(name=f'internal_{module_name}',
          version='0.0',
          ext_modules=[ext])


if __name__ == "__main__":
    main()
