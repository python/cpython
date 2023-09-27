import argparse
import contextlib
import os
import pathlib
import subprocess
import sysconfig

CHECKOUT = pathlib.Path(".").absolute()
CROSS_BUILD_DIR = CHECKOUT / "cross-build"
HOST_TRIPLE = "wasm32-unknown-wasi"

# - Make sure `Modules/Setup.local` exists
# - Make sure the necessary build tools are installed:
#   - `make`
#   - `pkg-config` (on Linux)
# - Create the build Python
#   - `mkdir -p builddir/build`
#   - `pushd builddir/build`
#   - Get the build platform
#     - Python: `sysconfig.get_config_var("BUILD_GNU_TYPE")`
#     - Shell: `../../config.guess`
#   - `../../configure -C`
#   - `make all`
#   - ```PYTHON_VERSION=`./python -c 'import sys; print(f"{sys.version_info.major}.{sys.version_info.minor}")'` ```
#   - `popd`
# - Create the host/WASI Python
#   - `mkdir builddir/wasi`
#   - `pushd builddir/wasi`
#   - `../../Tools/wasm/wasi-env ../../configure -C --host=wasm32-unknown-wasi --build=$(../../config.guess) --with-build-python=../build/python`
#     - `CONFIG_SITE=../../Tools/wasm/config.site-wasm32-wasi`
#     - `HOSTRUNNER="wasmtime run --mapdir /::$(dirname $(dirname $(pwd))) --env PYTHONPATH=/builddir/wasi/build/lib.wasi-wasm32-$PYTHON_VERSION $(pwd)/python.wasm --"`
#       - Maps the source checkout to `/` in the WASI runtime
#       - Stdlib gets loaded from `/Lib`
#       - Gets `_sysconfigdata__wasi_wasm32-wasi.py` on to `sys.path` via `PYTHONPATH`
#     - Set by `wasi-env`
#       - `WASI_SDK_PATH`
#       - `WASI_SYSROOT`; enough for `--sysroot`?
#       - `CC`
#       - `CPP`
#       - `CXX`
#       - `LDSHARED`
#       - `AR`
#       - `RANLIB`
#       - `CFLAGS`
#       - `LDFLAGS`
#       - `PKG_CONFIG_PATH`
#       - `PKG_CONFIG_LIBDIR`
#       - `PKG_CONFIG_SYSROOT_DIR`
#       - `PATH`
#   - `make all`
# - Create `run_wasi.sh`

def build_platform():
    """The name of the build/host platform."""
    # Can also be found via `config.guess`.`
    return sysconfig.get_config_var("BUILD_GNU_TYPE")


def compile_host_python():
    """Compile the build/host Python.

    Returns the path to the new interpreter and it's major.minor version.
    """
    build_dir = CROSS_BUILD_DIR / "build"
    build_dir.mkdir(parents=True, exist_ok=True)


    with contextlib.chdir(build_dir):
        subprocess.check_call([CHECKOUT / "configure", "-C"])
        subprocess.check_call(["make", "all"])
        # XXX
    #   - ```PYTHON_VERSION=`./python -c 'import sys; print(f"{sys.version_info.major}.{sys.version_info.minor}")'` ```
    # XXX Check if the binary is named `python` on macOS
    return build_dir / "python", XXX


def find_wasi_sdk():
    """Find the path to wasi-sdk."""
    if wasi_sdk_path := os.environ.get("WASI_SDK_PATH"):
        return pathlib.Path(wasi_sdk_path)
    elif (default_path := pathlib.Path("/opt/wasi-sdk")).exists():
        return default_path


def main():
    parser = argparse.ArgumentParser()
    subcommands = parser.add_subparsers(dest="subcommand")
    build = subcommands.add_parser("build")
    build.add_argument("--wasi-sdk", type=pathlib.Path, dest="wasi_sdk_path",
                       default=find_wasi_sdk(),
                       help="Path to wasi-sdk; defaults to "
                            "$WASI_SDK_PATH or /opt/wasi-sdk")

    args = parser.parse_args()
    if not args.wasi_sdk_path or not args.wasi_sdk_path.exists():
        raise ValueError("wasi-sdk not found; see https://github.com/WebAssembly/wasi-sdk")

    compile_host_python()


if  __name__ == "__main__":
    main()
