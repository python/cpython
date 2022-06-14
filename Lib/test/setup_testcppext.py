# gh-91321: Build a basic C++ test extension to check that the Python C API is
# compatible with C++ and does not emit C++ compiler warnings.
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
        # Warn on old-style cast (C cast) like: (PyObject*)op
        '-Wold-style-cast',
        # Warn when using NULL rather than _Py_NULL in static inline functions
        '-Wzero-as-null-pointer-constant',
    ]
else:
    # Don't pass any compiler flag to MSVC
    CPPFLAGS = []


def main():
    cppflags = list(CPPFLAGS)
    if '-std=c++03' in sys.argv:
        sys.argv.remove('-std=c++03')
        std = 'c++03'
        name = '_testcpp03ext'
    else:
        # Python currently targets C++11
        std = 'c++11'
        name = '_testcpp11ext'

    cppflags = [*CPPFLAGS, f'-std={std}']
    cpp_ext = Extension(
        name,
        sources=[SOURCE],
        language='c++',
        extra_compile_args=cppflags)
    setup(name=name, ext_modules=[cpp_ext])


if __name__ == "__main__":
    main()
