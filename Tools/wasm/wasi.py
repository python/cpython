import argparse
import contextlib
import os
try:
    from os import process_cpu_count as cpu_count
except ImportError:
    from os import cpu_count
import pathlib
import shutil
import subprocess
import sysconfig
import tempfile

CHECKOUT = pathlib.Path(__file__).parent.parent.parent
CROSS_BUILD_DIR = CHECKOUT / "cross-build"
HOST_TRIPLE = "wasm32-wasi"


def section(working_dir):
    """Print out a visible section header based on a working directory."""
    print("#" * 80)
    print("📁", working_dir)


def call(command, *, quiet, **kwargs):
    """Execute a command.

    If 'quiet' is true, then redirect stdout and stderr to a temporary file.
    """
    print("❯", " ".join(map(str, command)))
    if not quiet:
        stdout = None
        stderr = None
    else:
        stdout = tempfile.NamedTemporaryFile("w", encoding="utf-8",
                                             delete=False,
                                             prefix="cpython-wasi-",
                                             suffix=".log")
        stderr = subprocess.STDOUT
        print(f"Logging output to {stdout.name} ...")

    subprocess.check_call(command, **kwargs, stdout=stdout, stderr=stderr)


def build_platform():
    """The name of the build/host platform."""
    # Can also be found via `config.guess`.`
    return sysconfig.get_config_var("BUILD_GNU_TYPE")


def compile_host_python(context):
    """Compile the build/host Python.

    Returns the path to the new interpreter and it's major.minor version.
    """
    build_dir = CROSS_BUILD_DIR / "build"
    build_dir.mkdir(parents=True, exist_ok=True)

    section(build_dir)

    configure = [CHECKOUT / "configure", "-C"]
    if context.debug:
        configure.append("--with-pydebug")

    if not context.build_python:
        print("Skipped via --skip-build-python ...")
    else:
        with contextlib.chdir(build_dir):
            call(configure, quiet=context.quiet)
            call(["make", "--jobs", str(cpu_count()), "all"], quiet=context.quiet)

    binary = build_dir / "python"
    cmd = [binary, "-c",
            "import sys; "
            "print(f'{sys.version_info.major}.{sys.version_info.minor}')"]
    version = subprocess.check_output(cmd, encoding="utf-8").strip()

    # XXX Check if the binary is named `python` on macOS
    return binary, version


def find_wasi_sdk():
    """Find the path to wasi-sdk."""
    if wasi_sdk_path := os.environ.get("WASI_SDK_PATH"):
        return pathlib.Path(wasi_sdk_path)
    elif (default_path := pathlib.Path("/opt/wasi-sdk")).exists():
        return default_path


def wasi_sdk_env(context):
    """Calculate environment variables for building with wasi-sdk."""
    wasi_sdk_path = context.wasi_sdk_path
    sysroot = wasi_sdk_path / "share" / "wasi-sysroot"
    env = {"CC": "clang", "CPP": "clang-cpp", "CXX": "clang++",
           "LDSHARED": "wasm-ld", "AR": "llvm-ar", "RANLIB": "ranlib"}

    for env_var, binary_name in list(env.items()):
        env[env_var] = os.fsdecode(wasi_sdk_path / "bin" / binary_name)

    if wasi_sdk_path != pathlib.Path("/opt/wasi-sdk"):
        for compiler in ["CC", "CPP", "CXX"]:
            env[compiler] += f" --sysroot={sysroot}"

    env["PKG_CONFIG_PATH"] = ""
    env["PKG_CONFIG_LIBDIR"] = os.pathsep.join(
                                map(os.fsdecode,
                                    [sysroot / "lib" / "pkgconfig",
                                     sysroot / "share" / "pkgconfig"]))
    env["PKG_CONFIG_SYSROOT_DIR"] = os.fsdecode(sysroot)

    env["WASI_SDK_PATH"] = os.fsdecode(wasi_sdk_path)
    env["WASI_SYSROOT"] = os.fsdecode(sysroot)

    env["PATH"] = os.pathsep.join([os.fsdecode(wasi_sdk_path / "bin"),
                                   os.environ["PATH"]])

    return env


def compile_wasi_python(context, build_python, version):
    """Compile the wasm32-wasi Python."""
    build_dir = CROSS_BUILD_DIR / HOST_TRIPLE
    build_dir.mkdir(exist_ok=True)

    section(build_dir)

    config_site = os.fsdecode(CHECKOUT / "Tools" / "wasm" / "config.site-wasm32-wasi")
    # Map the checkout to / to load the stdlib from /Lib. Also means paths for
    # PYTHONPATH to include sysconfig data must be anchored to the WASI
    # runtime's / directory.
    host_runner = (f"{shutil.which('wasmtime')} run "
                   f"--mapdir /::{CHECKOUT} "
                   f"--env PYTHONPATH=/{CROSS_BUILD_DIR.name}/wasi/build/lib.wasi-wasm32-{version} "
                   f"{build_dir / 'python.wasm'} --")
    env_additions = {"CONFIG_SITE": config_site, "HOSTRUNNER": host_runner,
                     # Python's first commit:
                     # Thu, 09 Aug 1990 14:25:15 +0000 (1990-08-09)
                     # https://hg.python.org/cpython/rev/3cd033e6b530
                     "SOURCE_DATE_EPOCH": "650211915"}

    with contextlib.chdir(build_dir):
        # The path to `configure` MUST be relative, else `python.wasm` is unable
        # to find the stdlib due to Python not recognizing that it's being
        # executed from within a checkout.
        configure = [os.path.relpath(CHECKOUT / 'configure', build_dir),
                     "-C",
                     f"--host={HOST_TRIPLE}",
                     f"--build={build_platform()}",
                     f"--with-build-python={build_python}"]
        if context.debug:
            configure.append("--with-pydebug")
        configure_env = os.environ | env_additions | wasi_sdk_env(context)
        call(configure, env=configure_env, quiet=context.quiet)
        call(["make", "--jobs", str(cpu_count()), "all"],
             env=os.environ | env_additions,
             quiet=context.quiet)

    exec_script = build_dir / "python.sh"
    with exec_script.open("w", encoding="utf-8") as file:
        file.write(f'#!/bin/sh\nexec {host_runner} "$@"\n')
    exec_script.chmod(0o755)


def main():
    parser = argparse.ArgumentParser()
    subcommands = parser.add_subparsers(dest="subcommand")
    build = subcommands.add_parser("build")
    build.add_argument("--wasi-sdk", type=pathlib.Path, dest="wasi_sdk_path",
                       default=find_wasi_sdk(),
                       help="Path to wasi-sdk; defaults to "
                            "$WASI_SDK_PATH or /opt/wasi-sdk")
    build.add_argument("--skip-build-python", action="store_false",
                       dest="build_python", default=True,
                       help="Skip building the build/host Python")
    build.add_argument("--quiet", action="store_true", default=False,
                       dest="quiet",
                       help="Redirect output from subprocesses to a log file")
    build.add_argument("--debug", action="store_true", default=False,
                       dest="debug",
                       help="Debug build (i.e., pydebug)")

    context = parser.parse_args()
    if not context.wasi_sdk_path or not context.wasi_sdk_path.exists():
        raise ValueError("wasi-sdk not found or specified; "
                         "see https://github.com/WebAssembly/wasi-sdk")

    build_python, version = compile_host_python(context)
    print()
    compile_wasi_python(context, build_python, version)


if  __name__ == "__main__":
    main()
