import argparse
import contextlib
import functools
import os
try:
    from os import process_cpu_count as cpu_count
except ImportError:
    from os import cpu_count
import pathlib
import shutil
import subprocess
import sys
import sysconfig
import tempfile

CHECKOUT = pathlib.Path(__file__).parent.parent.parent
CROSS_BUILD_DIR = CHECKOUT / "cross-build"
BUILD_DIR = CROSS_BUILD_DIR / "build"
HOST_TRIPLE = "wasm32-wasi"


def section(working_dir):
    """Print out a visible section header based on a working directory."""
    try:
        tput_output = subprocess.check_output(["tput", "cols"], encoding="utf-8")
        terminal_width = int(tput_output.strip())
    except subprocess.CalledProcessError:
        terminal_width = 80
    print("#" * terminal_width)
    print("📁", working_dir)


def subdir(working_dir, *, clean_ok=False):
    """Decorator to change to a working directory."""
    def decorator(func):
        @functools.wraps(func)
        def wrapper(context):
            if clean_ok and context.clean and working_dir.exists():
                print(f"Deleting {working_dir} (--clean)...")
                shutil.rmtree(working_dir)

            working_dir.mkdir(parents=True, exist_ok=True)
            section(working_dir)

            with contextlib.chdir(working_dir):
                return func(context, working_dir)

        return wrapper

    return decorator

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
        print(f"Logging output to {stdout.name} (--quiet)...")

    subprocess.check_call(command, **kwargs, stdout=stdout, stderr=stderr)


def build_platform():
    """The name of the build/host platform."""
    # Can also be found via `config.guess`.`
    return sysconfig.get_config_var("BUILD_GNU_TYPE")


@subdir(CHECKOUT)
def prep_checkout(context, working_dir):
    """Prepare the source checkout for cross-compiling."""
    # Without `Setup.local`, in-place execution fails to realize it's in a
    # build tree/checkout (the dreaded "No module named 'encodings'" error).
    local_setup = working_dir / "Modules" / "Setup.local"
    if local_setup.exists():
        print("Modules/Setup.local already exists ...")
    else:
        print("Touching Modules/Setup.local ...")
        local_setup.touch()


@subdir(BUILD_DIR, clean_ok=True)
def configure_build_python(context, working_dir):
    """Configure the build/host Python."""
    configure = [os.path.relpath(CHECKOUT / 'configure', working_dir)]
    if context.args:
        configure.extend(context.args)

    call(configure, quiet=context.quiet)


@subdir(BUILD_DIR)
def make_build_python(context, working_dir):
    """Make/build the build/host Python."""
    call(["make", "--jobs", str(cpu_count()), "all"],
            quiet=context.quiet)

    binary = working_dir / "python"
    if not binary.is_file():
        binary = binary.with_suffix(".exe")
        if not binary.is_file():
            raise FileNotFoundError(f"Unable to find `python(.exe)` in {working_dir}")
    cmd = [binary, "-c",
            "import sys; "
            "print(f'{sys.version_info.major}.{sys.version_info.minor}')"]
    version = subprocess.check_output(cmd, encoding="utf-8").strip()

    print(f"{binary} {version}")


def compile_host_python(context):
    """Compile the build/host Python.

    Returns the path to the new interpreter and it's major.minor version.
    """
    section(BUILD_DIR)

    configure = [os.path.relpath(CHECKOUT / 'configure', BUILD_DIR), "-C"]
    if context.debug:
        configure.append("--with-pydebug")

    if context.platform not in {"all", "build"}:
        print("Skipping build (--platform=host)...")
    else:
        if context.clean and BUILD_DIR.exists():
            print(f"Deleting {BUILD_DIR} (--clean)...")
            shutil.rmtree(BUILD_DIR)

        BUILD_DIR.mkdir(parents=True, exist_ok=True)

        with contextlib.chdir(BUILD_DIR):
            call(configure, quiet=context.quiet)
            call(["make", "--jobs", str(cpu_count()), "all"],
                 quiet=context.quiet)

    binary = BUILD_DIR / "python"
    if not binary.is_file():
        binary = binary.with_suffix(".exe")
        if not binary.is_file():
            raise FileNotFoundError(f"Unable to find `python(.exe)` in {BUILD_DIR}")
    cmd = [binary, "-c",
            "import sys; "
            "print(f'{sys.version_info.major}.{sys.version_info.minor}')"]
    version = subprocess.check_output(cmd, encoding="utf-8").strip()

    print(f"Python {version} @ {binary}")

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

    section(build_dir)

    if context.clean and build_dir.exists():
        print(f"Deleting {build_dir} (--clean)...")
        shutil.rmtree(build_dir)

    build_dir.mkdir(exist_ok=True)

    config_site = os.fsdecode(CHECKOUT / "Tools" / "wasm" / "config.site-wasm32-wasi")
    # Use PYTHONPATH to include sysconfig data (which must be anchored to the
    # WASI guest's / directory.
    guest_build_dir = build_dir.relative_to(CHECKOUT)
    sysconfig_data = f"{guest_build_dir}/build/lib.wasi-wasm32-{version}"
    if context.debug:
        sysconfig_data += "-pydebug"

    host_runner = context.host_runner.format(GUEST_DIR="/",
                                             HOST_DIR=CHECKOUT,
                                             ENV_VAR_NAME="PYTHONPATH",
                                             ENV_VAR_VALUE=f"/{sysconfig_data}",
                                             PYTHON_WASM=build_dir / "python.wasm")
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
        if context.threads:
            configure.append("--with-wasm-pthreads")
        configure_env = os.environ | env_additions | wasi_sdk_env(context)
        call(configure, env=configure_env, quiet=context.quiet)
        call(["make", "--jobs", str(cpu_count()), "all"],
             env=os.environ | env_additions,
             quiet=context.quiet)


    if not (CHECKOUT / sysconfig_data).exists():
        raise FileNotFoundError(f"Unable to find {sysconfig_data}; "
                                 "check if build Python is a different build type")

    exec_script = build_dir / "python.sh"
    with exec_script.open("w", encoding="utf-8") as file:
        file.write(f'#!/bin/sh\nexec {host_runner} "$@"\n')
    exec_script.chmod(0o755)
    print(f"Created {exec_script} ... ", end="")
    sys.stdout.flush()
    subprocess.check_call([exec_script, "--version"])


def main():
    parser = argparse.ArgumentParser()
    subcommands = parser.add_subparsers(dest="subcommand")
    build = subcommands.add_parser("build", help="Build everything")
    configure_build = subcommands.add_parser("configure-build-python",
                                             help="Run `configure` for the build Python")
    make_build = subcommands.add_parser("make-build-python",
                                        help="Run `make` for the build Python")
    for subcommand in build, configure_build, make_build:
        subcommand.add_argument("--quiet", action="store_true", default=False,
                        dest="quiet",
                        help="Redirect output from subprocesses to a log file")
    for subcommand in build, configure_build:
        subcommand.add_argument("--clean", action="store_true", default=False,
                        dest="clean",
                        help="Delete any relevant directories before building")
    # configure-build-python
    configure_build.add_argument("args", nargs="*",
                                 help="Extra arguments to pass to `configure`")
    # build
    build.add_argument("--with-pydebug", action="store_true", default=False,
                       dest="debug",
                       help="Debug build (i.e., pydebug)")
    build.add_argument("--platform", choices=["all", "build", "host"],
                       default="all",
                       help="specify which platform(s) to build for "
                            "(default is 'all')")
    build.add_argument("--wasi-sdk", type=pathlib.Path, dest="wasi_sdk_path",
                       default=find_wasi_sdk(),
                       help="Path to wasi-sdk; defaults to "
                            "$WASI_SDK_PATH or /opt/wasi-sdk")
    default_host_runner = (f"{shutil.which('wasmtime')} run "
                    # Make sure the stack size will work for a pydebug build.
                    # The 8388608 value comes from `ulimit -s` under Linux which
                    # is 8291 KiB.
                    "--wasm max-wasm-stack=8388608 "
                    # Enable thread support.
                    "--wasm threads=y --wasi threads=y "
                    # Map the checkout to / to load the stdlib from /Lib.
                    "--dir {HOST_DIR}::{GUEST_DIR} "
                    # Set PYTHONPATH to the sysconfig data.
                    "--env {ENV_VAR_NAME}={ENV_VAR_VALUE} "
                    # Path to the WASM binary.
                    "{PYTHON_WASM}")
    build.add_argument("--host-runner", action="store",
                       default=default_host_runner, dest="host_runner",
                       help="Command template for running the WebAssembly code "
                            "(default for wasmtime 14 or newer: "
                            f"`{default_host_runner}`)")
    build.add_argument("--threads", action="store_true", default=False,
                       dest="threads",
                       help="Compile with threads support (off by default as "
                            "thread support is experimental in WASI)")

    context = parser.parse_args()
    if context.subcommand == "configure-build-python":
        prep_checkout(context)
        print()
        configure_build_python(context)
    elif context.subcommand == "make-build-python":
        make_build_python(context)
    else:
        if not context.wasi_sdk_path or not context.wasi_sdk_path.exists():
            raise ValueError("wasi-sdk not found or specified; "
                            "see https://github.com/WebAssembly/wasi-sdk")

        prep_checkout(context)
        print()
        build_python, version = compile_host_python(context)
        if context.platform in {"all", "host"}:
            print()
            compile_wasi_python(context, build_python, version)


if  __name__ == "__main__":
    main()
