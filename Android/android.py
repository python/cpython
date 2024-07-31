#!/usr/bin/env python3

import argparse
from glob import glob
import os
import re
import shutil
import subprocess
import sys
import sysconfig
from os.path import basename, relpath
from pathlib import Path
from tempfile import TemporaryDirectory

SCRIPT_NAME = Path(__file__).name
CHECKOUT = Path(__file__).resolve().parent.parent
CROSS_BUILD_DIR = CHECKOUT / "cross-build"


def delete_glob(pattern):
    # Path.glob doesn't accept non-relative patterns.
    for path in glob(str(pattern)):
        path = Path(path)
        print(f"Deleting {path} ...")
        if path.is_dir() and not path.is_symlink():
            shutil.rmtree(path)
        else:
            path.unlink()


def subdir(name, *, clean=None):
    path = CROSS_BUILD_DIR / name
    if clean:
        delete_glob(path)
    if not path.exists():
        if clean is None:
            sys.exit(
                f"{path} does not exist. Create it by running the appropriate "
                f"`configure` subcommand of {SCRIPT_NAME}.")
        else:
            path.mkdir(parents=True)
    return path


def run(command, *, host=None, **kwargs):
    env = os.environ.copy()
    if host:
        env_script = CHECKOUT / "Android/android-env.sh"
        env_output = subprocess.run(
            f"set -eu; "
            f"HOST={host}; "
            f"PREFIX={subdir(host)}/prefix; "
            f". {env_script}; "
            f"export",
            check=True, shell=True, text=True, stdout=subprocess.PIPE
        ).stdout

        for line in env_output.splitlines():
            # We don't require every line to match, as there may be some other
            # output from installing the NDK.
            if match := re.search(
                "^(declare -x |export )?(\\w+)=['\"]?(.*?)['\"]?$", line
            ):
                key, value = match[2], match[3]
                if env.get(key) != value:
                    print(line)
                    env[key] = value

        if env == os.environ:
            raise ValueError(f"Found no variables in {env_script.name} output:\n"
                             + env_output)

    print(">", " ".join(map(str, command)))
    try:
        subprocess.run(command, check=True, env=env, **kwargs)
    except subprocess.CalledProcessError as e:
        sys.exit(e)


def build_python_path():
    """The path to the build Python binary."""
    build_dir = subdir("build")
    binary = build_dir / "python"
    if not binary.is_file():
        binary = binary.with_suffix(".exe")
        if not binary.is_file():
            raise FileNotFoundError("Unable to find `python(.exe)` in "
                                    f"{build_dir}")

    return binary


def configure_build_python(context):
    os.chdir(subdir("build", clean=context.clean))

    command = [relpath(CHECKOUT / "configure")]
    if context.args:
        command.extend(context.args)
    run(command)


def make_build_python(context):
    os.chdir(subdir("build"))
    run(["make", "-j", str(os.cpu_count())])


def unpack_deps(host):
    deps_url = "https://github.com/beeware/cpython-android-source-deps/releases/download"
    for name_ver in ["bzip2-1.0.8-1", "libffi-3.4.4-2", "openssl-3.0.13-1",
                     "sqlite-3.45.1-0", "xz-5.4.6-0"]:
        filename = f"{name_ver}-{host}.tar.gz"
        download(f"{deps_url}/{name_ver}/{filename}")
        run(["tar", "-xf", filename])
        os.remove(filename)


def download(url, target_dir="."):
    out_path = f"{target_dir}/{basename(url)}"
    run(["curl", "-Lf", "-o", out_path, url])
    return out_path


def configure_host_python(context):
    host_dir = subdir(context.host, clean=context.clean)

    prefix_dir = host_dir / "prefix"
    if not prefix_dir.exists():
        prefix_dir.mkdir()
        os.chdir(prefix_dir)
        unpack_deps(context.host)

    build_dir = host_dir / "build"
    build_dir.mkdir(exist_ok=True)
    os.chdir(build_dir)

    command = [
        # Basic cross-compiling configuration
        relpath(CHECKOUT / "configure"),
        f"--host={context.host}",
        f"--build={sysconfig.get_config_var('BUILD_GNU_TYPE')}",
        f"--with-build-python={build_python_path()}",
        "--without-ensurepip",

        # Android always uses a shared libpython.
        "--enable-shared",
        "--without-static-libpython",

        # Dependent libraries. The others are found using pkg-config: see
        # android-env.sh.
        f"--with-openssl={prefix_dir}",
    ]

    if context.args:
        command.extend(context.args)
    run(command, host=context.host)


def make_host_python(context):
    # The CFLAGS and LDFLAGS set in android-env include the prefix dir, so
    # delete any previously-installed Python libs and include files to prevent
    # them being used during the build.
    host_dir = subdir(context.host)
    prefix_dir = host_dir / "prefix"
    delete_glob(f"{prefix_dir}/include/python*")
    delete_glob(f"{prefix_dir}/lib/libpython*")

    os.chdir(host_dir / "build")
    run(["make", "-j", str(os.cpu_count())], host=context.host)
    run(["make", "install", f"prefix={prefix_dir}"], host=context.host)


def build_all(context):
    steps = [configure_build_python, make_build_python, configure_host_python,
             make_host_python]
    for step in steps:
        step(context)


def clean_all(context):
    delete_glob(CROSS_BUILD_DIR)


# To avoid distributing compiled artifacts without corresponding source code,
# the Gradle wrapper is not included in the CPython repository. Instead, we
# extract it from the Gradle release.
def setup_testbed(context):
    ver_long = "8.7.0"
    ver_short = ver_long.removesuffix(".0")
    testbed_dir = CHECKOUT / "Android/testbed"

    for filename in ["gradlew", "gradlew.bat"]:
        out_path = download(
            f"https://raw.githubusercontent.com/gradle/gradle/v{ver_long}/{filename}",
            testbed_dir)
        os.chmod(out_path, 0o755)

    with TemporaryDirectory(prefix=SCRIPT_NAME) as temp_dir:
        os.chdir(temp_dir)
        bin_zip = download(
            f"https://services.gradle.org/distributions/gradle-{ver_short}-bin.zip")
        outer_jar = f"gradle-{ver_short}/lib/plugins/gradle-wrapper-{ver_short}.jar"
        run(["unzip", bin_zip, outer_jar])
        run(["unzip", "-o", "-d", f"{testbed_dir}/gradle/wrapper", outer_jar,
             "gradle-wrapper.jar"])


def main():
    parser = argparse.ArgumentParser()
    subcommands = parser.add_subparsers(dest="subcommand")
    build = subcommands.add_parser("build", help="Build everything")
    configure_build = subcommands.add_parser("configure-build",
                                             help="Run `configure` for the "
                                             "build Python")
    make_build = subcommands.add_parser("make-build",
                                        help="Run `make` for the build Python")
    configure_host = subcommands.add_parser("configure-host",
                                            help="Run `configure` for Android")
    make_host = subcommands.add_parser("make-host",
                                       help="Run `make` for Android")
    subcommands.add_parser(
        "clean", help="Delete the cross-build directory")
    subcommands.add_parser(
        "setup-testbed", help="Download the testbed Gradle wrapper")

    for subcommand in build, configure_build, configure_host:
        subcommand.add_argument(
            "--clean", action="store_true", default=False, dest="clean",
            help="Delete any relevant directories before building")
    for subcommand in build, configure_host, make_host:
        subcommand.add_argument(
            "host", metavar="HOST",
            choices=["aarch64-linux-android", "x86_64-linux-android"],
            help="Host triplet: choices=[%(choices)s]")
    for subcommand in build, configure_build, configure_host:
        subcommand.add_argument("args", nargs="*",
                                help="Extra arguments to pass to `configure`")

    context = parser.parse_args()
    dispatch = {"configure-build": configure_build_python,
                "make-build": make_build_python,
                "configure-host": configure_host_python,
                "make-host": make_host_python,
                "build": build_all,
                "clean": clean_all,
                "setup-testbed": setup_testbed}
    dispatch[context.subcommand](context)


if __name__ == "__main__":
    main()
