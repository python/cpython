# gh-91321: Build a basic C++ test extension to check that the Python C API is
# compatible with C++ and does not emit C++ compiler warnings.
import os
import sys
from test import support

from setuptools import setup, Extension


MS_WINDOWS = (sys.platform == 'win32')


SOURCE = support.findfile('_testcppext.cpp')
if not MS_WINDOWS:
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

    cpp_ext = Extension(
        name,
        sources=[SOURCE],
        language='c++',
        extra_compile_args=cppflags)
    setup(name='internal' + name, version='0.0', ext_modules=[cpp_ext])


if __name__ == "__main__":
    main()
