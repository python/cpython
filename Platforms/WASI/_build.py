__lazy_modules__ = [
    "os",
    "pathlib",
    "shutil",
    "subprocess",
    "sys",
]

import os
import pathlib
import shutil
import subprocess
import sys

try:
    from os import process_cpu_count as cpu_count
except ImportError:
    from os import cpu_count

import _shared

WASMTIME_VAR_NAME = "WASMTIME"
WASMTIME_HOST_RUNNER_VAR = f"{{{WASMTIME_VAR_NAME}}}"


@_shared.subdir("build_python_path", clean_ok=True)
def configure_build_python(context, working_dir):
    """Configure the build/host Python."""
    configure = [os.path.relpath(context.checkout / "configure", working_dir)]
    configure.extend(context.configure_args)

    context.call(configure)


@_shared.subdir("build_python_path")
def make_build_python(context, _working_dir):
    """Make/build the build Python."""
    context.call(["make", "--jobs", str(cpu_count()), "all"])

    binary = context.build_python_interpreter
    cmd = [
        binary,
        "-c",
        "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}')",
    ]
    version = subprocess.check_output(cmd, encoding="utf-8").strip()

    _shared.log("🎉", f"{binary} {version}")


def wasi_sdk_env(context):
    """Calculate environment variables for building with wasi-sdk."""
    wasi_sdk_path = context.wasi_sdk_path
    sysroot = wasi_sdk_path / "share" / "wasi-sysroot"
    env = {
        "CC": "clang",
        "CPP": "clang-cpp",
        "CXX": "clang++",
        "AR": "llvm-ar",
        "RANLIB": "ranlib",
    }

    for env_var, binary_name in list(env.items()):
        env[env_var] = os.fsdecode(wasi_sdk_path / "bin" / binary_name)

    for compiler in ["CC", "CPP", "CXX"]:
        env[compiler] += f" --target={context.host_triple}"
        if not wasi_sdk_path.name.startswith("wasi-sdk"):
            env[compiler] += f" --sysroot={sysroot}"

    env["PKG_CONFIG_PATH"] = ""
    env["PKG_CONFIG_LIBDIR"] = os.pathsep.join(
        map(
            os.fsdecode,
            [sysroot / "lib" / "pkgconfig", sysroot / "share" / "pkgconfig"],
        )
    )
    env["PKG_CONFIG_SYSROOT_DIR"] = os.fsdecode(sysroot)

    env["WASI_SDK_PATH"] = os.fsdecode(wasi_sdk_path)
    env["WASI_SYSROOT"] = os.fsdecode(sysroot)

    env["PATH"] = os.pathsep.join([
        os.fsdecode(wasi_sdk_path / "bin"),
        os.environ["PATH"],
    ])

    return env


@_shared.subdir("wasi_build_path", clean_ok=True)
def configure_wasi_python(context, working_dir):
    """Configure the WASI/host build."""
    config_site = os.fsdecode(context.here / "config.site-wasm32-wasi")

    wasi_build_dir = working_dir.relative_to(context.checkout)

    args = {
        "WASMTIME": "wasmtime",
        "ARGV0": f"/{wasi_build_dir}/python.wasm",
        "CHECKOUT": os.fsdecode(context.checkout),
        "WASMTIME_CONFIG_PATH": os.fsdecode(context.here / "wasmtime.toml"),
    }
    # Check dynamically for wasmtime in case it was specified manually via
    # `--host-runner`.
    if "{WASMTIME}" in context.host_runner:
        if wasmtime := shutil.which("wasmtime"):
            args["WASMTIME"] = wasmtime
        else:
            raise FileNotFoundError(
                "wasmtime not found; download from "
                "https://github.com/bytecodealliance/wasmtime"
            )
    host_runner = context.host_runner.format_map(args)
    env_additions = {
        "CONFIG_SITE": config_site,
        "HOSTRUNNER": host_runner,
        "CFLAGS": f"{os.environ.get('CFLAGS', '')} -DPY_CALL_TRAMPOLINE".strip(),
    }
    build_python = os.fsdecode(context.build_python_interpreter)
    # The path to `configure` MUST be relative, else `python.wasm` is unable
    # to find the stdlib due to Python not recognizing that it's being
    # executed from within context.checkout.
    configure = [
        os.path.relpath(context.checkout / "configure", working_dir),
        f"--host={context.host_triple}",
        f"--build={context.build_python_path.name}",
        f"--with-build-python={build_python}",
    ]
    if context.is_debug:
        configure.append("--with-pydebug")
    configure.extend(context.configure_args)
    context.call(
        configure,
        env=_shared.updated_env(env_additions | wasi_sdk_env(context)),
    )

    python_wasm = working_dir / "python.wasm"
    exec_script = working_dir / "python.sh"
    with exec_script.open("w", encoding="utf-8") as file:
        file.write(f'#!/bin/sh\nexec {host_runner} {python_wasm} "$@"\n')
    exec_script.chmod(0o755)
    _shared.log("🏃", f"Created {exec_script} (--host-runner)... ")
    sys.stdout.flush()


@_shared.subdir("wasi_build_path")
def make_wasi_python(context, working_dir):
    """Run `make` for the WASI/host build."""
    context.call(
        ["make", "--jobs", str(cpu_count()), "all"],
        env=_shared.updated_env(),
    )

    exec_script = working_dir / "python.sh"
    context.call(
        [exec_script, "-c", "import sys; print(sys.version)"], quiet=False
    )
    _shared.log(
        "🎉",
        f"Use `{exec_script.relative_to(pathlib.Path().absolute())}` "
        "to run CPython w/ the WASI host specified by --host-runner",
    )


def clean_contents(context):
    """Delete all files created by this script."""
    context.clean = True
    for path in [context.cross_build_path, context.dist_path]:
        if path.exists():
            _shared.log("🧹", f"Deleting {path} ...")
            shutil.rmtree(path)


@_shared.subdir("build_python_path")
def pythoninfo_build_python(context, working_dir):
    """Display build info of the build Python."""
    context.call(["make", "pythoninfo"])


@_shared.subdir("wasi_build_path")
def pythoninfo_wasi_python(context, working_dir):
    """Display build info of the host/WASI Python."""
    context.call(["make", "pythoninfo"])
