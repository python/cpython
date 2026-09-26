#!/usr/bin/env python3

import argparse
import json
import os
import platform
import re
import shlex
import shutil
import subprocess
import sys
import sysconfig
from datetime import datetime, timezone
from glob import glob
from os.path import abspath, basename, relpath
from pathlib import Path
from subprocess import CalledProcessError
from tempfile import TemporaryDirectory

import android_run
from android_run import android_home, print_called_process_error, run


SCRIPT_NAME = Path(__file__).name
if SCRIPT_NAME.startswith("__"):
    SCRIPT_NAME = "Platforms/Android"

ANDROID_DIR = Path(__file__).resolve().parent
PYTHON_DIR = ANDROID_DIR.parent.parent
in_source_tree = (
    ANDROID_DIR.name == "Android" and (PYTHON_DIR / "pyconfig.h.in").exists()
)

ENV_SCRIPT = ANDROID_DIR / "android-env.sh"
TESTBED_DIR = ANDROID_DIR / "testbed"
CROSS_BUILD_DIR = PYTHON_DIR / "cross-build"

HOSTS = [
    "aarch64-linux-android",
    "arm-linux-androideabi",
    "i686-linux-android",
    "x86_64-linux-android",
]


def delete_glob(pattern):
    # Path.glob doesn't accept non-relative patterns.
    for path in glob(str(pattern)):
        path = Path(path)
        print(f"Deleting {path} ...")
        if path.is_dir() and not path.is_symlink():
            shutil.rmtree(path)
        else:
            path.unlink()


def subdir(*parts, create=False):
    path = CROSS_BUILD_DIR.joinpath(*parts)
    if not path.exists():
        if not create:
            sys.exit(
                f"{path} does not exist. Create it by running the appropriate "
                f"`configure` subcommand of {SCRIPT_NAME}.")
        else:
            path.mkdir(parents=True)
    return path


# Format the environment so it can be pasted into a shell.
def print_env(env):
    for key, value in sorted(env.items()):
        print(f"export {key}={shlex.quote(value)}")


def android_env(host):
    if host:
        prefix = subdir(host) / "prefix"
    else:
        prefix = ANDROID_DIR / "prefix"
        sysconfig_files = prefix.glob("lib/python*/_sysconfigdata__android_*.py")
        sysconfig_filename = next(sysconfig_files).name
        host = re.fullmatch(r"_sysconfigdata__android_(.+).py", sysconfig_filename)[1]

    env_output = subprocess.run(
        f"set -eu; "
        f"HOST={host}; "
        f"PREFIX={prefix}; "
        f". {ENV_SCRIPT}; "
        f"export",
        check=True, shell=True, stdout=subprocess.PIPE, encoding='utf-8',
    ).stdout

    env = {}
    for line in env_output.splitlines():
        # We don't require every line to match, as there may be some other
        # output from installing the NDK.
        if match := re.search(
            "^(declare -x |export )?(\\w+)=['\"]?(.*?)['\"]?$", line
        ):
            key, value = match[2], match[3]
            if os.environ.get(key) != value:
                env[key] = value

    if not env:
        raise ValueError(f"Found no variables in {ENV_SCRIPT.name} output:\n"
                         + env_output)
    return env


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
    if context.clean:
        clean("build")
    os.chdir(subdir("build", create=True))

    command = [relpath(PYTHON_DIR / "configure")]
    if context.args:
        command.extend(context.args)
    run(command)


def make_build_python(context):
    os.chdir(subdir("build"))
    run(["make", "-j", str(os.cpu_count())])


def pythoninfo_build_python(context):
    os.chdir(subdir("build"))
    run(["make", "pythoninfo"])


# To create new builds of these dependencies, usually all that's necessary is to
# push a tag to the cpython-android-source-deps repository, and GitHub Actions
# will do the rest.
#
# If you're a member of the Python core team, and you'd like to be able to push
# these tags yourself, please contact Malcolm Smith or Russell Keith-Magee.
def unpack_deps(host, prefix_dir, cache_dir):
    os.chdir(prefix_dir)
    deps_url = "https://github.com/beeware/cpython-android-source-deps/releases/download"
    for name_ver in [
        "bzip2-1.0.8-3",
        "libffi-3.4.4-3",
        "openssl-3.5.8-0",
        "sqlite-3.53.4-0",
        "xz-5.4.6-1",
        "zstd-1.5.7-2"
    ]:
        filename = f"{name_ver}-{host}.tar.gz"
        out_path = download(f"{deps_url}/{name_ver}/{filename}", cache_dir)
        shutil.unpack_archive(out_path)


def download(url, cache_dir):
    out_path = cache_dir / basename(url)
    cache_dir.mkdir(parents=True, exist_ok=True)
    if not out_path.is_file():
        run(["curl", "-Lf", "--retry", "5", "--retry-all-errors", "-o", out_path, url])
    else:
        print(f"Using cached version of {basename(url)}")
    return out_path


def configure_host_python(context, host=None):
    if host is None:
        host = context.host
    if context.clean:
        clean(host)

    host_dir = subdir(host, create=True)
    prefix_dir = host_dir / "prefix"
    if not prefix_dir.exists():
        prefix_dir.mkdir()
        cache_dir = (
            Path(context.cache_dir).resolve()
            if context.cache_dir
            else CROSS_BUILD_DIR / "downloads"
        )
        unpack_deps(host, prefix_dir, cache_dir)

    os.chdir(host_dir)
    command = [
        # Basic cross-compiling configuration
        relpath(PYTHON_DIR / "configure"),
        f"--host={host}",
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

    host_env = android_env(host)
    print_env(host_env)
    run(command, env={**os.environ, **host_env})


def make_host_python(context, host=None):
    if host is None:
        host = context.host
    # The CFLAGS and LDFLAGS set in android-env include the prefix dir, so
    # delete any previous Python installation to prevent it being used during
    # the build.
    host_dir = subdir(host)
    prefix_dir = host_dir / "prefix"
    for pattern in ("include/python*", "lib/libpython*", "lib/python*"):
        delete_glob(f"{prefix_dir}/{pattern}")

    # The Android environment variables were already captured in the Makefile by
    # `configure`, and passing them again when running `make` may cause some
    # flags to be duplicated. So we don't use the `host` argument here.
    os.chdir(host_dir)
    run(["make", "-j", str(os.cpu_count())])

    # The `make install` output is very verbose and rarely useful, so
    # suppress it by default.
    run(
        ["make", "install", f"prefix={prefix_dir}"],
        capture_output=not context.verbose,
    )


def build_targets(context):
    if context.target in {"all", "build"}:
        configure_build_python(context)
        make_build_python(context)
        pythoninfo_build_python(context)

    for host in HOSTS:
        if context.target in {"all", "hosts", host}:
            configure_host_python(context, host)
            make_host_python(context, host)


def clean(host):
    delete_glob(CROSS_BUILD_DIR / host)


def clean_targets(context):
    if context.target in {"all", "build"}:
        clean("build")

    for host in HOSTS:
        if context.target in {"all", "hosts", host}:
            clean(host)


def setup_ci():
    if "GITHUB_ACTIONS" in os.environ:
        # Enable emulator hardware acceleration
        # (https://github.blog/changelog/2024-04-02-github-actions-hardware-accelerated-android-virtualization-now-available/).
        if platform.system() == "Linux":
            run(
                ["sudo", "tee", "/etc/udev/rules.d/99-kvm4all.rules"],
                input='KERNEL=="kvm", GROUP="kvm", MODE="0666", OPTIONS+="static_node=kvm"\n',
                text=True,
            )
            run(["sudo", "udevadm", "control", "--reload-rules"])
            run(["sudo", "udevadm", "trigger", "--name-match=kvm"])

        # Free up disk space by deleting unused versions of the NDK
        # (https://github.com/freakboy3742/pyspamsum/pull/108).
        for line in ENV_SCRIPT.read_text().splitlines():
            if match := re.fullmatch(r"ndk_version=(.+)", line):
                ndk_version = match[1]
                break
        else:
            raise ValueError(f"Failed to find NDK version in {ENV_SCRIPT.name}")

        for item in (android_home / "ndk").iterdir():
            if item.name[0].isdigit() and item.name != ndk_version:
                delete_glob(item)


# To avoid distributing compiled artifacts without corresponding source code,
# the Gradle wrapper is not included in the CPython repository. Instead, we
# extract it from the Gradle GitHub repository.
def setup_testbed():
    paths = ["gradlew", "gradlew.bat", "gradle/wrapper/gradle-wrapper.jar"]
    if all((TESTBED_DIR / path).exists() for path in paths):
        return

    # Any version of the wrapper can run any reasonably close version of Gradle, so this
    # doesn't need to match the Gradle version used for the build, which is specified in
    # testbed/gradle/wrapper/gradle-wrapper.properties.
    version = "9.5.0"

    for path in paths:
        out_path = TESTBED_DIR / path
        out_path.parent.mkdir(exist_ok=True)
        download(
            f"https://raw.githubusercontent.com/gradle/gradle/v{version}/{path}",
            out_path.parent,
        )
        os.chmod(out_path, 0o755)


def run_testbed(context):
    setup_ci()
    setup_testbed()
    env = os.environ.copy()

    # Ensure that CROSS_BUILD_DIR is in the Gradle environment, regardless
    # of whether it was set by environment variable or `--cross-build-dir`.
    env["CROSS_BUILD_DIR"] = CROSS_BUILD_DIR

    if context.ci_mode:
        context.args[0:0] = [
            # See _add_ci_python_opts in libregrtest/main.py.
            "-W", "error", "-bb", "-E",

            # Randomization is disabled because order-dependent failures are
            # much less likely to pass on a rerun in single-process mode.
            "-m", "test",
            f"--{context.ci_mode}-ci", "--single-process", "--no-randomize",
            "--pythoninfo",
        ]

    if not any(arg in context.args for arg in ["-c", "-m"]):
        context.args[0:0] = ["-m", "test"]

    args = ["-v"] * context.verbose
    args += android_run.unparse_device_group(context)
    for name, value in [
        ("python.sitePackages", context.site_packages),
        ("python.cwd", context.cwd),
        (
            "android.testInstrumentationRunnerArguments.pythonArgs",
            json.dumps(context.args),
        ),
    ]:
        if value:
            args += ["-P", f"{name}={value}"]
    args += [str(TESTBED_DIR)]
    run([android_run.__file__, *args], env=env, log=bool(context.verbose))


def package_version(prefix_dir):
    patchlevel_glob = f"{prefix_dir}/include/python*/patchlevel.h"
    patchlevel_paths = glob(patchlevel_glob)
    if len(patchlevel_paths) != 1:
        sys.exit(f"{patchlevel_glob} matched {len(patchlevel_paths)} paths.")

    for line in open(patchlevel_paths[0]):
        if match := re.fullmatch(r'\s*#define\s+PY_VERSION\s+"(.+)"\s*', line):
            version = match[1]
            break
    else:
        sys.exit(f"Failed to find Python version in {patchlevel_paths[0]}.")

    # If not building against a tagged commit, add a timestamp to the version.
    # Follow the PyPA version number rules, as this will make it easier to
    # process with other tools.
    if version.endswith("+"):
        version += datetime.now(timezone.utc).strftime("%Y%m%d.%H%M%S")

    return version


def package(context):
    prefix_dir = subdir(context.host, "prefix")
    version = package_version(prefix_dir)

    with TemporaryDirectory(prefix=SCRIPT_NAME.replace("/", "-")) as temp_dir:
        temp_dir = Path(temp_dir)

        # Include all tracked files from the Android directory.
        for line in run(
            ["git", "ls-files"],
            cwd=ANDROID_DIR, capture_output=True, text=True, log=False,
        ).stdout.splitlines():
            src = ANDROID_DIR / line
            # "__main__.py" is renamed "android.py" for distribution purpose
            dst = temp_dir / {
                "__main__.py": "android.py"
            }.get(line, line)
            dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src, dst, follow_symlinks=False)

        # Include anything from the prefix directory which could be useful
        # either for embedding Python in an app, or building third-party
        # packages against it.
        for rel_dir, patterns in [
            ("include", ["openssl*", "python*", "sqlite*"]),
            ("lib", ["engines-3", "libcrypto*.so", "libpython*", "libsqlite*",
                     "libssl*.so", "ossl-modules", "python*"]),
            ("lib/pkgconfig", ["*crypto*", "*ssl*", "*python*", "*sqlite*"]),
        ]:
            for pattern in patterns:
                for src in glob(f"{prefix_dir}/{rel_dir}/{pattern}"):
                    dst = temp_dir / relpath(src, prefix_dir.parent)
                    dst.parent.mkdir(parents=True, exist_ok=True)
                    if Path(src).is_dir():
                        shutil.copytree(
                            src, dst, symlinks=True,
                            ignore=lambda *args: ["__pycache__"]
                        )
                    else:
                        shutil.copy2(src, dst, follow_symlinks=False)

        # Strip debug information.
        if not context.debug:
            so_files = glob(f"{temp_dir}/**/*.so", recursive=True)
            run([android_env(context.host)["STRIP"], *so_files], log=False)

        dist_dir = subdir(context.host, "dist", create=True)
        package_path = shutil.make_archive(
            f"{dist_dir}/python-{version}-{context.host}", "gztar", temp_dir
        )
        print(f"Wrote {package_path}")
        return package_path


def ci(context):
    for step in [
        configure_build_python,
        make_build_python,
        pythoninfo_build_python,
        configure_host_python,
        make_host_python,
        package,
    ]:
        caption = (
            step.__name__.replace("_", " ")
            .capitalize()
            .replace("python", "Python")
        )
        print(f"::group::{caption}")
        result = step(context)
        if step is package:
            package_path = result
        print("::endgroup::")

    if (
        "GITHUB_ACTIONS" in os.environ
        and (platform.system(), platform.machine()) != ("Linux", "x86_64")
    ):
        print(
            "Skipping tests: GitHub Actions does not support the Android "
            "emulator on this platform."
        )
    else:
        with TemporaryDirectory(prefix=SCRIPT_NAME.replace("/", "-")) as temp_dir:
            print("::group::Tests")

            # Prove the package is self-contained by using it to run the tests.
            shutil.unpack_archive(package_path, temp_dir)
            launcher_args = [
                "--managed", "maxVersion", "-v", f"--{context.ci_mode}-ci"
            ]
            run(
                ["./android.py", "test", *launcher_args],
                cwd=temp_dir
            )
            print("::endgroup::")


def env(context):
    print_env(android_env(getattr(context, "host", None)))


def parse_args():
    parser = argparse.ArgumentParser()
    subcommands = parser.add_subparsers(dest="subcommand", required=True)

    def add_parser(*args, **kwargs):
        parser = subcommands.add_parser(*args, **kwargs)
        parser.add_argument(
            "--cross-build-dir",
            action="store",
            default=os.environ.get("CROSS_BUILD_DIR"),
            dest="cross_build_dir",
            type=Path,
            help=(
                "Path to the cross-build directory "
                f"(default: {CROSS_BUILD_DIR}). Can also be set "
                "with the CROSS_BUILD_DIR environment variable."
            ),
        )
        android_run.add_verbose_argument(parser)
        return parser

    # Subcommands
    build = add_parser(
        "build",
        help="Run configure and make for the selected target"
    )
    configure_build = add_parser(
        "configure-build", help="Run `configure` for the build Python")
    add_parser(
        "make-build", help="Run `make` for the build Python")
    add_parser(
        "pythoninfo-build", help="Display build info of the build Python")
    configure_host = add_parser(
        "configure-host", help="Run `configure` for Android")
    make_host = add_parser(
        "make-host", help="Run `make` for Android")

    clean = add_parser(
        "clean",
        help="Delete build directories for the selected target"
    )

    test = add_parser("test", help="Run the testbed app")
    package = add_parser("package", help="Make a release package")
    ci = add_parser("ci", help="Run build, package and test")
    env = add_parser("env", help="Print environment variables")

    # Common arguments
    # --cache-dir option
    for cmd in [configure_host, build, ci]:
        cmd.add_argument(
            "--cache-dir",
            default=os.environ.get("CACHE_DIR"),
            help="The directory to store cached downloads.",
        )

    # --clean option
    for subcommand in [build, configure_build, configure_host, ci]:
        subcommand.add_argument(
            "--clean", action="store_true", default=False, dest="clean",
            help="Delete the relevant build directories first")

    # Allow "all", "build" and "hosts" targets for some commands
    for subcommand in [clean, build]:
        subcommand.add_argument(
            "target",
            nargs="?",
            default="all",
            choices=["all", "build", "hosts"] + HOSTS,
            help=(
                "The host triplet (e.g., aarch64-linux-android), "
                "or 'build' for just the build platform, or 'hosts' for all "
                "host platforms, or 'all' for the build platform and all "
                "hosts. Defaults to 'all'"
            ),
        )

    host_commands = [configure_host, make_host, package, ci]
    if in_source_tree:
        host_commands.append(env)
    for subcommand in host_commands:
        subcommand.add_argument(
            "host", metavar="HOST", choices=HOSTS,
            help="Host triplet: choices=[%(choices)s]")

    for subcommand in [build, configure_build, configure_host, ci]:
        subcommand.add_argument("args", nargs="*",
                                help="Extra arguments to pass to `configure`")

    # Test arguments
    android_run.add_device_group(test)
    test.add_argument(
        "--site-packages", metavar="DIR", type=abspath,
        help="Directory to copy as the app's site-packages.")
    test.add_argument(
        "--cwd", metavar="DIR", type=abspath,
        help="Directory to copy as the app's working directory.")
    test.add_argument(
        "args", nargs="*", help=f"Python command-line arguments. "
        f"Separate them from {SCRIPT_NAME}'s own arguments with `--`. "
        f"If neither -c nor -m are included, `-m test` will be prepended, "
        f"which will run Python's own test suite.")

    # Package arguments.
    for subcommand in [package, ci]:
        subcommand.add_argument(
            "-g", action="store_true", default=False, dest="debug",
            help="Include debug information in package")

    # CI arguments
    for subcommand in [test, ci]:
        group = subcommand.add_mutually_exclusive_group(required=subcommand is ci)
        group.add_argument(
            "--fast-ci", action="store_const", dest="ci_mode", const="fast",
            help="Add test arguments for GitHub Actions")
        group.add_argument(
            "--slow-ci", action="store_const", dest="ci_mode", const="slow",
            help="Add test arguments for buildbots")

    return parser.parse_args()


def main():
    android_run.configure_streams()
    context = parse_args()

    # Set the CROSS_BUILD_DIR if an argument was provided
    if context.cross_build_dir:
        global CROSS_BUILD_DIR
        CROSS_BUILD_DIR = context.cross_build_dir.resolve()

    dispatch = {
        "configure-build": configure_build_python,
        "make-build": make_build_python,
        "pythoninfo-build": pythoninfo_build_python,
        "configure-host": configure_host_python,
        "make-host": make_host_python,
        "build": build_targets,
        "clean": clean_targets,
        "test": run_testbed,
        "package": package,
        "ci": ci,
        "env": env,
    }

    try:
        dispatch[context.subcommand](context)
    except CalledProcessError as e:
        print_called_process_error(e)
        sys.exit(1)


if __name__ == "__main__":
    main()
