# gh-91321: Build a basic C++ test extension to check that the Python C API is
# compatible with C++ and does not emit C++ compiler warnings.
import os
import shlex
import sys
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
    std = os.environ["CPYTHON_TEST_CPP_STD"]
    name = os.environ["CPYTHON_TEST_EXT_NAME"]

    cppflags = [*CPPFLAGS, f'-std={std}']

    # gh-105776: When "gcc -std=11" is used as the C++ compiler, -std=c11
    # option emits a C++ compiler warning. Remove "-std11" option from the
    # CC command.
    cmd = (sysconfig.get_config_var('CC') or '')
    if cmd is not None:
        cmd = shlex.split(cmd)
        cmd = [arg for arg in cmd if not arg.startswith('-std=')]
        cmd = shlex.join(cmd)
        # CC env var overrides sysconfig CC variable in setuptools
        os.environ['CC'] = cmd

    cpp_ext = Extension(
        name,
        sources=[SOURCE],
        language='c++',
        extra_compile_args=cppflags)
    setup(name='internal' + name, version='0.0', ext_modules=[cpp_ext])


if __name__ == "__main__":
    main()
