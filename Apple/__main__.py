#!/usr/bin/env python3
##########################################################################
# Apple XCframework build script
#
# This script simplifies the process of configuring, compiling and packaging an
# XCframework for an Apple platform.
#
# At present, it only supports iOS, but it has been constructed so that it
# could be used on any Apple platform.
#
# The simplest entry point is:
#
#   $ python Apple ci iOS
#
# which will:
# * Clean any pre-existing build artefacts
# * Configure and make a Python that can be used for the build
# * Configure and make a Python for each supported iOS architecture and ABI
# * Combine the outputs of the builds from the previous step into a single
#   XCframework, merging binaries into a "fat" binary if necessary
# * Clone a copy of the testbed, configured to use the XCframework
# * Construct a tarball containing the release artefacts
# * Run the test suite using the generated XCframework.
#
# This is the complete sequence that would be needed in CI to build and test
# a candidate release artefact.
#
# Each individual step can be invoked individually - there are commands to
# clean, configure-build, make-build, configure-host, make-host, package, and
# test.
#
# There is also a build command that can be used to combine the configure and
# make steps for the build Python, an individual host, all hosts, or all
# builds.
##########################################################################
from __future__ import annotations

import argparse
import os
import platform
import re
import shlex
import shutil
import signal
import subprocess
import sys
import sysconfig
import time
from collections.abc import Callable, Sequence
from contextlib import contextmanager
from datetime import datetime, timezone
from os.path import basename, relpath
from pathlib import Path
from subprocess import CalledProcessError

EnvironmentT = dict[str, str]
ArgsT = Sequence[str | Path]

SCRIPT_NAME = Path(__file__).name
PYTHON_DIR = Path(__file__).resolve().parent.parent

CROSS_BUILD_DIR = PYTHON_DIR / "cross-build"

HOSTS: dict[str, dict[str, dict[str, str]]] = {
    # Structure of this data:
    # * Platform identifier
    #   * an XCframework slice that must exist for that platform
    #     * a host triple: the multiarch spec for that host
    "iOS": {
        "ios-arm64": {
            "arm64-apple-ios": "arm64-iphoneos",
        },
        "ios-arm64_x86_64-simulator": {
            "arm64-apple-ios-simulator": "arm64-iphonesimulator",
            "x86_64-apple-ios-simulator": "x86_64-iphonesimulator",
        },
    },
}


def subdir(name: str, create: bool = False) -> Path:
    """Ensure that a cross-build directory for the given name exists."""
    path = CROSS_BUILD_DIR / name
    if not path.exists():
        if not create:
            sys.exit(
                f"{path} does not exist. Create it by running the appropriate "
                f"`configure` subcommand of {SCRIPT_NAME}."
            )
        else:
            path.mkdir(parents=True)
    return path


def run(
    command: ArgsT,
    *,
    host: str | None = None,
    env: EnvironmentT | None = None,
    log: bool | None = True,
    **kwargs,
) -> subprocess.CompletedProcess:
    """Run a command in an Apple development environment.

    Optionally logs the executed command to the console.
    """
    kwargs.setdefault("check", True)
    if env is None:
        env = os.environ.copy()

    if host:
        host_env = apple_env(host)
        print_env(host_env)
        env.update(host_env)

    if log:
        print(">", join_command(command))
    return subprocess.run(command, env=env, **kwargs)


def join_command(args: str | Path | ArgsT) -> str:
    """Format a command so it can be copied into a shell.

    Similar to `shlex.join`, but also accepts arguments which are Paths, or a
    single string/Path outside of a list.
    """
    if isinstance(args, (str, Path)):
        return str(args)
    else:
        return shlex.join(map(str, args))


def print_env(env: EnvironmentT) -> None:
    """Format the environment so it can be pasted into a shell."""
    for key, value in sorted(env.items()):
        print(f"export {key}={shlex.quote(value)}")


def apple_env(host: str) -> EnvironmentT:
    """Construct an Apple development environment for the given host."""
    env = {
        "PATH": ":".join([
            str(PYTHON_DIR / "Apple/iOS/Resources/bin"),
            str(subdir(host) / "prefix"),
            "/usr/bin",
            "/bin",
            "/usr/sbin",
            "/sbin",
            "/Library/Apple/usr/bin",
        ]),
    }

    return env


def delete_path(name: str) -> None:
    """Delete the named cross-build directory, if it exists."""
    path = CROSS_BUILD_DIR / name
    if path.exists():
        print(f"Deleting {path} ...")
        shutil.rmtree(path)


def all_host_triples(platform: str) -> list[str]:
    """Return all host triples for the given platform.

    The host triples are the platform definitions used as input to configure
    (e.g., "arm64-apple-ios-simulator").
    """
    triples = []
    for slice_name, slice_parts in HOSTS[platform].items():
        triples.extend(list(slice_parts))
    return triples


def clean(context: argparse.Namespace, target: str = "all") -> None:
    """The implementation of the "clean" command."""
    # If we're explicitly targeting the build, there's no platform or
    # distribution artefacts. If we're cleaning tests, we keep all built
    # artefacts. Otherwise, the built artefacts must be dirty, so we remove
    # them.
    if target not in {"build", "test"}:
        paths = ["dist", context.platform] + list(HOSTS[context.platform])
    else:
        paths = []

    if target in {"all", "build"}:
        paths.append("build")

    if target in {"all", "hosts"}:
        paths.extend(all_host_triples(context.platform))
    elif target not in {"build", "test", "package"}:
        paths.append(target)

    if target in {"all", "hosts", "test"}:
        paths.extend([
            path.name
            for path in CROSS_BUILD_DIR.glob(f"{context.platform}-testbed.*")
        ])

    for path in paths:
        delete_path(path)


def build_python_path() -> Path:
    """The path to the build Python binary."""
    build_dir = subdir("build")
    binary = build_dir / "python"
    if not binary.is_file():
        binary = binary.with_suffix(".exe")
        if not binary.is_file():
            raise FileNotFoundError(
                f"Unable to find `python(.exe)` in {build_dir}"
            )

    return binary


@contextmanager
def group(text: str):
    """A context manager that outputs a log marker around a section of a build.

    If running in a GitHub Actions environment, the GitHub syntax for
    collapsible log sections is used.
    """
    if "GITHUB_ACTIONS" in os.environ:
        print(f"::group::{text}")
    else:
        print(f"===== {text} " + "=" * (70 - len(text)))

    yield

    if "GITHUB_ACTIONS" in os.environ:
        print("::endgroup::")
    else:
        print()


@contextmanager
def cwd(subdir: Path):
    """A context manager that sets the current working directory."""
    orig = os.getcwd()
    os.chdir(subdir)
    yield
    os.chdir(orig)


def configure_build_python(context: argparse.Namespace) -> None:
    """The implementation of the "configure-build" command."""
    if context.clean:
        clean(context, "build")

    with (
        group("Configuring build Python"),
        cwd(subdir("build", create=True)),
    ):
        command = [relpath(PYTHON_DIR / "configure")]
        if context.args:
            command.extend(context.args)
        run(command)


def make_build_python(context: argparse.Namespace) -> None:
    """The implementation of the "make-build" command."""
    with (
        group("Compiling build Python"),
        cwd(subdir("build")),
    ):
        run(["make", "-j", str(os.cpu_count())])


def apple_target(host: str) -> str:
    """Return the Apple platform identifier for a given host triple."""
    for _, platform_slices in HOSTS.items():
        for slice_name, slice_parts in platform_slices.items():
            for host_triple, multiarch in slice_parts.items():
                if host == host_triple:
                    return ".".join(multiarch.split("-")[::-1])

    raise KeyError(host)


def apple_multiarch(host: str) -> str:
    """Return the multiarch descriptor for a given host triple."""
    for _, platform_slices in HOSTS.items():
        for slice_name, slice_parts in platform_slices.items():
            for host_triple, multiarch in slice_parts.items():
                if host == host_triple:
                    return multiarch

    raise KeyError(host)


def unpack_deps(
    platform: str,
    host: str,
    prefix_dir: Path,
    cache_dir: Path,
) -> None:
    """Unpack binary dependencies into a provided directory.

    Downloads binaries if they aren't already present. Downloads will be stored
    in provided cache directory.

    On iOS, as a safety mechanism, any dynamic libraries will be purged from
    the unpacked dependencies.
    """
    # To create new builds of these dependencies, usually all that's necessary
    # is to push a tag to the cpython-apple-source-deps repository, and GitHub
    # Actions will do the rest.
    #
    # If you're a member of the Python core team, and you'd like to be able to
    # push these tags yourself, please contact Malcolm Smith or Russell
    # Keith-Magee.
    deps_url = "https://github.com/beeware/cpython-apple-source-deps/releases/download"
    for name_ver in [
        "BZip2-1.0.8-2",
        "libFFI-3.4.7-2",
        "OpenSSL-3.0.18-1",
        "XZ-5.6.4-2",
        "mpdecimal-4.0.0-2",
        "zstd-1.5.7-1",
    ]:
        filename = f"{name_ver.lower()}-{apple_target(host)}.tar.gz"
        archive_path = download(
            f"{deps_url}/{name_ver}/{filename}",
            target_dir=cache_dir,
        )
        shutil.unpack_archive(archive_path, prefix_dir)

    # Dynamic libraries will be preferentially linked over static;
    # On iOS, ensure that no dylibs are available in the prefix folder.
    if platform == "iOS":
        for dylib in prefix_dir.glob("**/*.dylib"):
            dylib.unlink()


def download(url: str, target_dir: Path) -> Path:
    """Download the specified URL into the given directory.

    :return: The path to the downloaded archive.
    """
    target_path = Path(target_dir).resolve()
    target_path.mkdir(exist_ok=True, parents=True)

    out_path = target_path / basename(url)
    if not Path(out_path).is_file():
        run([
            "curl",
            "-Lf",
            "--retry",
            "5",
            "--retry-all-errors",
            "-o",
            out_path,
            url,
        ])
    else:
        print(f"Using cached version of {basename(url)}")
    return out_path


def configure_host_python(
    context: argparse.Namespace,
    host: str | None = None,
) -> None:
    """The implementation of the "configure-host" command."""
    if host is None:
        host = context.host

    if context.clean:
        clean(context, host)

    host_dir = subdir(host, create=True)
    prefix_dir = host_dir / "prefix"

    with group(f"Downloading dependencies ({host})"):
        if not prefix_dir.exists():
            prefix_dir.mkdir()
            unpack_deps(context.platform, host, prefix_dir, context.cache_dir)
        else:
            print("Dependencies already installed")

    with (
        group(f"Configuring host Python ({host})"),
        cwd(host_dir),
    ):
        command = [
            # Basic cross-compiling configuration
            relpath(PYTHON_DIR / "configure"),
            f"--host={host}",
            f"--build={sysconfig.get_config_var('BUILD_GNU_TYPE')}",
            f"--with-build-python={build_python_path()}",
            "--with-system-libmpdec",
            "--enable-framework",
            # Dependent libraries.
            f"--with-openssl={prefix_dir}",
            f"LIBLZMA_CFLAGS=-I{prefix_dir}/include",
            f"LIBLZMA_LIBS=-L{prefix_dir}/lib -llzma",
            f"LIBFFI_CFLAGS=-I{prefix_dir}/include",
            f"LIBFFI_LIBS=-L{prefix_dir}/lib -lffi",
            f"LIBMPDEC_CFLAGS=-I{prefix_dir}/include",
            f"LIBMPDEC_LIBS=-L{prefix_dir}/lib -lmpdec",
            f"LIBZSTD_CFLAGS=-I{prefix_dir}/include",
            f"LIBZSTD_LIBS=-L{prefix_dir}/lib -lzstd",
        ]

        if context.args:
            command.extend(context.args)
        run(command, host=host)


def make_host_python(
    context: argparse.Namespace,
    host: str | None = None,
) -> None:
    """The implementation of the "make-host" command."""
    if host is None:
        host = context.host

    with (
        group(f"Compiling host Python ({host})"),
        cwd(subdir(host)),
    ):
        run(["make", "-j", str(os.cpu_count())], host=host)
        run(["make", "install"], host=host)


def framework_path(host_triple: str, multiarch: str) -> Path:
    """The path to a built single-architecture framework product.

    :param host_triple: The host triple (e.g., arm64-apple-ios-simulator)
    :param multiarch: The multiarch identifier (e.g., arm64-simulator)
    """
    return CROSS_BUILD_DIR / f"{host_triple}/Apple/iOS/Frameworks/{multiarch}"


def package_version(prefix_path: Path) -> str:
    """Extract the Python version being built from patchlevel.h."""
    for path in prefix_path.glob("**/patchlevel.h"):
        text = path.read_text(encoding="utf-8")
        if match := re.search(
            r'\n\s*#define\s+PY_VERSION\s+"(.+)"\s*\n', text
        ):
            version = match[1]
            # If not building against a tagged commit, add a timestamp to the
            # version. Follow the PyPA version number rules, as this will make
            # it easier to process with other tools. The version will have a
            # `+` suffix once any official release has been made; a freshly
            # forked main branch will have a version of 3.X.0a0.
            if version.endswith("a0"):
                version += "+"
            if version.endswith("+"):
                version += datetime.now(timezone.utc).strftime("%Y%m%d.%H%M%S")

            return version

    sys.exit("Unable to determine Python version being packaged.")


def lib_platform_files(dirname, names):
    """A file filter that ignores platform-specific files in lib."""
    path = Path(dirname)
    if (
        path.parts[-3] == "lib"
        and path.parts[-2].startswith("python")
        and path.parts[-1] == "lib-dynload"
    ):
        return names
    elif path.parts[-2] == "lib" and path.parts[-1].startswith("python"):
        ignored_names = {
            name
            for name in names
            if (
                name.startswith("_sysconfigdata_")
                or name.startswith("_sysconfig_vars_")
                or name == "build-details.json"
            )
        }
    elif path.parts[-1] == "lib":
        ignored_names = {
            name
            for name in names
            if name.startswith("libpython") and name.endswith(".dylib")
        }
    else:
        ignored_names = set()

    return ignored_names


def lib_non_platform_files(dirname, names):
    """A file filter that ignores anything *except* platform-specific files
    in the lib directory.
    """
    path = Path(dirname)
    if path.parts[-2] == "lib" and path.parts[-1].startswith("python"):
        return (
            set(names) - lib_platform_files(dirname, names) - {"lib-dynload"}
        )
    else:
        return set()


def create_xcframework(platform: str) -> str:
    """Build an XCframework from the component parts for the platform.

    :return: The version number of the Python version that was packaged.
    """
    package_path = CROSS_BUILD_DIR / platform
    try:
        package_path.mkdir()
    except FileExistsError:
        raise RuntimeError(
            f"{platform} XCframework already exists; do you need to run "
            "with --clean?"
        ) from None

    frameworks = []
    # Merge Frameworks for each component SDK. If there's only one architecture
    # for the SDK, we can use the compiled Python.framework as-is. However, if
    # there's more than architecture, we need to merge the individual built
    # frameworks into a merged "fat" framework.
    for slice_name, slice_parts in HOSTS[platform].items():
        # Some parts are the same across all slices, so we use can any of the
        # host frameworks as the source for the merged version. Use the first
        # one on the list, as it's as representative as any other.
        first_host_triple, first_multiarch = next(iter(slice_parts.items()))
        first_framework = (
            framework_path(first_host_triple, first_multiarch)
            / "Python.framework"
        )

        if len(slice_parts) == 1:
            # The first framework is the only framework, so copy it.
            print(f"Copying framework for {slice_name}...")
            frameworks.append(first_framework)
        else:
            print(f"Merging framework for {slice_name}...")
            slice_path = CROSS_BUILD_DIR / slice_name
            slice_framework = slice_path / "Python.framework"
            slice_framework.mkdir(exist_ok=True, parents=True)

            # Copy the Info.plist
            shutil.copy(
                first_framework / "Info.plist",
                slice_framework / "Info.plist",
            )

            # Copy the headers
            shutil.copytree(
                first_framework / "Headers",
                slice_framework / "Headers",
            )

            # Create the "fat" library binary for the slice
            run(
                ["lipo", "-create", "-output", slice_framework / "Python"]
                + [
                    (
                        framework_path(host_triple, multiarch)
                        / "Python.framework/Python"
                    )
                    for host_triple, multiarch in slice_parts.items()
                ]
            )

            # Add this merged slice to the list to be added to the XCframework
            frameworks.append(slice_framework)

    print()
    print("Build XCframework...")
    cmd = [
        "xcodebuild",
        "-create-xcframework",
        "-output",
        package_path / "Python.xcframework",
    ]
    for framework in frameworks:
        cmd.extend(["-framework", framework])

    run(cmd)

    # Extract the package version from the merged framework
    version = package_version(package_path / "Python.xcframework")
    version_tag = ".".join(version.split(".")[:2])

    # On non-macOS platforms, each framework in XCframework only contains the
    # headers, libPython, plus an Info.plist. Other resources like the standard
    # library and binary shims aren't allowed to live in framework; they need
    # to be copied in separately.
    print()
    print("Copy additional resources...")
    has_common_stdlib = False
    for slice_name, slice_parts in HOSTS[platform].items():
        # Some parts are the same across all slices, so we can any of the
        # host frameworks as the source for the merged version.
        first_host_triple, first_multiarch = next(iter(slice_parts.items()))
        first_path = framework_path(first_host_triple, first_multiarch)
        first_framework = first_path / "Python.framework"

        slice_path = package_path / f"Python.xcframework/{slice_name}"
        slice_framework = slice_path / "Python.framework"

        # Copy the binary helpers
        print(f" - {slice_name} binaries")
        shutil.copytree(first_path / "bin", slice_path / "bin")

        # Copy the include path (a symlink to the framework headers)
        print(f" - {slice_name} include files")
        shutil.copytree(
            first_path / "include",
            slice_path / "include",
            symlinks=True,
        )

        # Copy in the cross-architecture pyconfig.h
        shutil.copy(
            PYTHON_DIR / f"Apple/{platform}/Resources/pyconfig.h",
            slice_framework / "Headers/pyconfig.h",
        )

        print(f" - {slice_name} shared library")
        # Create a simlink for the fat library
        shared_lib = slice_path / f"lib/libpython{version_tag}.dylib"
        shared_lib.parent.mkdir()
        shared_lib.symlink_to("../Python.framework/Python")

        print(f" - {slice_name} architecture-specific files")
        for host_triple, multiarch in slice_parts.items():
            print(f"   - {multiarch} standard library")
            arch, _ = multiarch.split("-", 1)

            if not has_common_stdlib:
                print("     - using this architecture as the common stdlib")
                shutil.copytree(
                    framework_path(host_triple, multiarch) / "lib",
                    package_path / "Python.xcframework/lib",
                    ignore=lib_platform_files,
                    symlinks=True,
                )
                has_common_stdlib = True

            shutil.copytree(
                framework_path(host_triple, multiarch) / "lib",
                slice_path / f"lib-{arch}",
                ignore=lib_non_platform_files,
                symlinks=True,
            )

            # Copy the host's pyconfig.h to an architecture-specific name.
            arch = multiarch.split("-")[0]
            host_path = (
                CROSS_BUILD_DIR
                / host_triple
                / "Apple/iOS/Frameworks"
                / multiarch
            )
            host_framework = host_path / "Python.framework"
            shutil.copy(
                host_framework / "Headers/pyconfig.h",
                slice_framework / f"Headers/pyconfig-{arch}.h",
            )

            # Apple identifies certain libraries as "security risks"; if you
            # statically link those libraries into a Framework, you become
            # responsible for providing a privacy manifest for that framework.
            xcprivacy_file = {
                "OpenSSL": subdir(host_triple)
                / "prefix/share/OpenSSL.xcprivacy"
            }
            print(f"   - {multiarch} xcprivacy files")
            for module, lib in [
                ("_hashlib", "OpenSSL"),
                ("_ssl", "OpenSSL"),
            ]:
                shutil.copy(
                    xcprivacy_file[lib],
                    slice_path
                    / f"lib-{arch}/python{version_tag}"
                    / f"lib-dynload/{module}.xcprivacy",
                )

    print(" - build tools")
    shutil.copytree(
        PYTHON_DIR / "Apple/testbed/Python.xcframework/build",
        package_path / "Python.xcframework/build",
    )

    return version


def package(context: argparse.Namespace) -> None:
    """The implementation of the "package" command."""
    if context.clean:
        clean(context, "package")

    with group("Building package"):
        # Create an XCframework
        version = create_xcframework(context.platform)

        # Clone testbed
        print()
        run([
            sys.executable,
            "Apple/testbed",
            "clone",
            "--platform",
            context.platform,
            "--framework",
            CROSS_BUILD_DIR / context.platform / "Python.xcframework",
            CROSS_BUILD_DIR / context.platform / "testbed",
        ])

        # Build the final archive
        archive_name = (
            CROSS_BUILD_DIR
            / "dist"
            / f"python-{version}-{context.platform}-XCframework"
        )

        print()
        print("Create package archive...")
        shutil.make_archive(
            str(CROSS_BUILD_DIR / archive_name),
            format="gztar",
            root_dir=CROSS_BUILD_DIR / context.platform,
            base_dir=".",
        )
        print()
        print(f"{archive_name.relative_to(PYTHON_DIR)}.tar.gz created.")


def build(context: argparse.Namespace, host: str | None = None) -> None:
    """The implementation of the "build" command."""
    if host is None:
        host = context.host

    if context.clean:
        clean(context, host)

    if host in {"all", "build"}:
        for step in [
            configure_build_python,
            make_build_python,
        ]:
            step(context)

    if host == "build":
        hosts = []
    elif host in {"all", "hosts"}:
        hosts = all_host_triples(context.platform)
    else:
        hosts = [host]

    for step_host in hosts:
        for step in [
            configure_host_python,
            make_host_python,
        ]:
            step(context, host=step_host)

    if host in {"all", "hosts"}:
        package(context)


def test(context: argparse.Namespace, host: str | None = None) -> None:  # noqa: PT028
    """The implementation of the "test" command."""
    if host is None:
        host = context.host

    if context.clean:
        clean(context, "test")

    with group(f"Test {'XCframework' if host in {'all', 'hosts'} else host}"):
        timestamp = str(time.time_ns())[:-6]
        testbed_dir = (
            CROSS_BUILD_DIR / f"{context.platform}-testbed.{timestamp}"
        )
        if host in {"all", "hosts"}:
            framework_path = (
                CROSS_BUILD_DIR / context.platform / "Python.xcframework"
            )
        else:
            build_arch = platform.machine()
            host_arch = host.split("-")[0]

            if not host.endswith("-simulator"):
                print("Skipping test suite non-simulator build.")
                return
            elif build_arch != host_arch:
                print(
                    f"Skipping test suite for an {host_arch} build "
                    f"on an {build_arch} machine."
                )
                return
            else:
                framework_path = (
                    CROSS_BUILD_DIR
                    / host
                    / f"Apple/{context.platform}"
                    / f"Frameworks/{apple_multiarch(host)}"
                )

        run([
            sys.executable,
            "Apple/testbed",
            "clone",
            "--platform",
            context.platform,
            "--framework",
            framework_path,
            testbed_dir,
        ])

        run(
            [
                sys.executable,
                testbed_dir,
                "run",
                "--verbose",
            ]
            + (
                ["--simulator", str(context.simulator)]
                if context.simulator
                else []
            )
            + [
                "--",
                "test",
                f"--{context.ci_mode}-ci",
                "--single-process",
                "--no-randomize",
                # Timeout handling requires subprocesses; explicitly setting
                # the timeout to -1 disables the faulthandler.
                "--timeout=-1",
                # Adding Python options requires the use of a subprocess to
                # start a new Python interpreter.
                "--dont-add-python-opts",
            ]
        )


def apple_sim_host(platform_name: str) -> str:
    """Determine the native simulator target for this platform."""
    for _, slice_parts in HOSTS[platform_name].items():
        for host_triple in slice_parts:
            parts = host_triple.split("-")
            if parts[0] == platform.machine() and parts[-1] == "simulator":
                return host_triple

    raise KeyError(platform_name)


def ci(context: argparse.Namespace) -> None:
    """The implementation of the "ci" command.

    In "Fast" mode, this compiles the build python, and the simulator for the
    build machine's architecture; and runs the test suite with `--fast-ci`
    configuration.

    In "Slow" mode, it compiles the build python, plus all candidate
    architectures (both device and simulator); then runs the test suite with
    `--slow-ci` configuration.
    """
    clean(context, "all")
    if context.ci_mode == "slow":
        # In slow mode, build and test the full XCframework
        build(context, host="all")
        test(context, host="all")
    else:
        # In fast mode, just build the simulator platform.
        sim_host = apple_sim_host(context.platform)
        build(context, host="build")
        build(context, host=sim_host)
        test(context, host=sim_host)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "A tool for managing the build, package and test process of "
            "CPython on Apple platforms."
        ),
    )
    parser.suggest_on_error = True
    subcommands = parser.add_subparsers(dest="subcommand", required=True)

    clean = subcommands.add_parser(
        "clean",
        help="Delete all build directories",
    )

    configure_build = subcommands.add_parser(
        "configure-build", help="Run `configure` for the build Python"
    )
    subcommands.add_parser(
        "make-build", help="Run `make` for the build Python"
    )
    configure_host = subcommands.add_parser(
        "configure-host",
        help="Run `configure` for a specific platform and target",
    )
    make_host = subcommands.add_parser(
        "make-host",
        help="Run `make` for a specific platform and target",
    )
    package = subcommands.add_parser(
        "package",
        help="Create a release package for the platform",
    )
    build = subcommands.add_parser(
        "build",
        help="Build all platform targets and create the XCframework",
    )
    test = subcommands.add_parser(
        "test",
        help="Run the testbed for a specific platform",
    )
    ci = subcommands.add_parser(
        "ci",
        help="Run build, package, and test",
    )

    # platform argument
    for cmd in [clean, configure_host, make_host, package, build, test, ci]:
        cmd.add_argument(
            "platform",
            choices=HOSTS.keys(),
            help="The target platform to build",
        )

    # host triple argument
    for cmd in [configure_host, make_host]:
        cmd.add_argument(
            "host",
            help="The host triple to build (e.g., arm64-apple-ios-simulator)",
        )
    # optional host triple argument
    for cmd in [clean, build, test]:
        cmd.add_argument(
            "host",
            nargs="?",
            default="all",
            help=(
                "The host triple to build (e.g., arm64-apple-ios-simulator), "
                "or 'build' for just the build platform, or 'hosts' for all "
                "host platforms, or 'all' for the build platform and all "
                "hosts. Defaults to 'all'"
            ),
        )

    # --clean option
    for cmd in [configure_build, configure_host, build, package, test, ci]:
        cmd.add_argument(
            "--clean",
            action="store_true",
            default=False,
            dest="clean",
            help="Delete the relevant build directories first",
        )

    # --cache-dir option
    for cmd in [configure_host, build, ci]:
        cmd.add_argument(
            "--cache-dir",
            default="./cross-build/downloads",
            help="The directory to store cached downloads.",
        )

    # --simulator option
    for cmd in [test, ci]:
        cmd.add_argument(
            "--simulator",
            help=(
                "The name of the simulator to use (eg: 'iPhone 16e'). "
                "Defaults to the most recently released 'entry level' "
                "iPhone device. Device architecture and OS version can also "
                "be specified; e.g., "
                "`--simulator 'iPhone 16 Pro,arch=arm64,OS=26.0'` would "
                "run on an ARM64 iPhone 16 Pro simulator running iOS 26.0."
            ),
        )
        group = cmd.add_mutually_exclusive_group()
        group.add_argument(
            "--fast-ci",
            action="store_const",
            dest="ci_mode",
            const="fast",
            help="Add test arguments for GitHub Actions",
        )
        group.add_argument(
            "--slow-ci",
            action="store_const",
            dest="ci_mode",
            const="slow",
            help="Add test arguments for buildbots",
        )

    for subcommand in [configure_build, configure_host, build, ci]:
        subcommand.add_argument(
            "args", nargs="*", help="Extra arguments to pass to `configure`"
        )

    return parser.parse_args()


def print_called_process_error(e: subprocess.CalledProcessError) -> None:
    for stream_name in ["stdout", "stderr"]:
        content = getattr(e, stream_name)
        stream = getattr(sys, stream_name)
        if content:
            stream.write(content)
            if not content.endswith("\n"):
                stream.write("\n")

    # shlex uses single quotes, so we surround the command with double quotes.
    print(
        f'Command "{join_command(e.cmd)}" returned exit status {e.returncode}'
    )


def main() -> None:
    # Handle SIGTERM the same way as SIGINT. This ensures that if we're
    # terminated by the buildbot worker, we'll make an attempt to clean up our
    # subprocesses.
    def signal_handler(*args):
        os.kill(os.getpid(), signal.SIGINT)

    signal.signal(signal.SIGTERM, signal_handler)

    # Process command line arguments
    context = parse_args()
    dispatch: dict[str, Callable] = {
        "clean": clean,
        "configure-build": configure_build_python,
        "make-build": make_build_python,
        "configure-host": configure_host_python,
        "make-host": make_host_python,
        "package": package,
        "build": build,
        "test": test,
        "ci": ci,
    }

    try:
        dispatch[context.subcommand](context)
    except CalledProcessError as e:
        print()
        print_called_process_error(e)
        sys.exit(1)
    except RuntimeError as e:
        print()
        print(e)
        sys.exit(2)


if __name__ == "__main__":
    # Under the buildbot, stdout is not a TTY, but we must still flush after
    # every line to make sure our output appears in the correct order relative
    # to the output of our subprocesses.
    for stream in [sys.stdout, sys.stderr]:
        stream.reconfigure(line_buffering=True)

    main()
