# gh-91321: Build a basic C test extension to check that the Python C API is
# compatible with C and does not emit C compiler warnings.
import os
import shlex
import sys
import sysconfig
from test import support

from setuptools import setup, Extension


SOURCE = 'extension.c'
if not support.MS_WINDOWS:
    # C compiler flags for GCC and clang
    CFLAGS = [
        # The purpose of test_cext extension is to check that building a C
        # extension using the Python C API does not emit C compiler warnings.
        '-Werror',

        # gh-116869: The Python C API must build with
        # -Werror=declaration-after-statement.
        '-Werror=declaration-after-statement',
    ]
else:
    # Don't pass any compiler flag to MSVC
    CFLAGS = []


def main():
    std = os.environ.get("CPYTHON_TEST_STD", "")
    module_name = os.environ["CPYTHON_TEST_EXT_NAME"]
    limited = bool(os.environ.get("CPYTHON_TEST_LIMITED", ""))

    cflags = list(CFLAGS)
    cflags.append(f'-DMODULE_NAME={module_name}')

    if std:
        cflags.append(f'-std={std}')

        # Remove existing -std options to only test ours
        cmd = (sysconfig.get_config_var('CC') or '')
        if cmd is not None:
            cmd = shlex.split(cmd)
            cmd = [arg for arg in cmd if not arg.startswith('-std=')]
            cmd = shlex.join(cmd)
            # CC env var overrides sysconfig CC variable in setuptools
            os.environ['CC'] = cmd

    if limited:
        version = sys.hexversion
        cflags.append(f'-DPy_LIMITED_API={version:#x}')

    for env_name in ('CC', 'CFLAGS'):
        if env_name in os.environ:
            print(f"{env_name} env var: {os.environ[env_name]!r}")
        else:
            print(f"{env_name} env var: <missing>")
    print(f"extra_compile_args: {cflags!r}")

    ext = Extension(
        module_name,
        sources=[SOURCE],
        extra_compile_args=cflags)
    setup(name=f'internal_{module_name}',
          version='0.0',
          ext_modules=[ext])


if __name__ == "__main__":
    main()
