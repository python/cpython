#!/usr/bin/env python3

import asyncio
import argparse
import json
import os
import platform
import re
import shlex
import shutil
import signal
import subprocess
import sys
import sysconfig
from asyncio import wait_for
from contextlib import asynccontextmanager
from datetime import datetime, timezone
from enum import IntEnum, auto
from glob import glob
from os.path import abspath, basename, relpath
from pathlib import Path
from subprocess import CalledProcessError
from tempfile import TemporaryDirectory


SCRIPT_NAME = Path(__file__).name
ANDROID_DIR = Path(__file__).resolve().parent
PYTHON_DIR = ANDROID_DIR.parent
in_source_tree = (
    ANDROID_DIR.name == "Android" and (PYTHON_DIR / "pyconfig.h.in").exists()
)

ENV_SCRIPT = ANDROID_DIR / "android-env.sh"
TESTBED_DIR = ANDROID_DIR / "testbed"
CROSS_BUILD_DIR = PYTHON_DIR / "cross-build"

HOSTS = ["aarch64-linux-android", "x86_64-linux-android"]
APP_ID = "org.python.testbed"
DECODE_ARGS = ("UTF-8", "backslashreplace")


try:
    android_home = Path(os.environ['ANDROID_HOME'])
except KeyError:
    sys.exit("The ANDROID_HOME environment variable is required.")

adb = Path(
    f"{android_home}/platform-tools/adb"
    + (".exe" if os.name == "nt" else "")
)

gradlew = Path(
    f"{TESTBED_DIR}/gradlew"
    + (".bat" if os.name == "nt" else "")
)

# Whether we've seen any output from Python yet.
python_started = False

# Buffer for verbose output which will be displayed only if a test fails and
# there has been no output from Python.
hidden_output = []


# Based on android/log.h in the NDK.
class LogPriority(IntEnum):
    UNKNOWN = 0
    DEFAULT = auto()
    VERBOSE = auto()
    DEBUG = auto()
    INFO = auto()
    WARN = auto()
    ERROR = auto()
    FATAL = auto()
    SILENT = auto()


def log_verbose(context, line, stream=sys.stdout):
    if context.verbose:
        stream.write(line)
    else:
        hidden_output.append((stream, line))


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


def run(command, *, host=None, env=None, log=True, **kwargs):
    kwargs.setdefault("check", True)
    if env is None:
        env = os.environ.copy()

    if host:
        host_env = android_env(host)
        print_env(host_env)
        env.update(host_env)

    if log:
        print(">", join_command(command))
    return subprocess.run(command, env=env, **kwargs)


# Format a command so it can be copied into a shell. Like shlex.join, but also
# accepts arguments which are Paths, or a single string/Path outside of a list.
def join_command(args):
    if isinstance(args, (str, Path)):
        return str(args)
    else:
        return shlex.join(map(str, args))


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
        check=True, shell=True, capture_output=True, encoding='utf-8',
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


# To create new builds of these dependencies, usually all that's necessary is to
# push a tag to the cpython-android-source-deps repository, and GitHub Actions
# will do the rest.
#
# If you're a member of the Python core team, and you'd like to be able to push
# these tags yourself, please contact Malcolm Smith or Russell Keith-Magee.
def unpack_deps(host, prefix_dir):
    os.chdir(prefix_dir)
    deps_url = "https://github.com/beeware/cpython-android-source-deps/releases/download"
    for name_ver in ["bzip2-1.0.8-3", "libffi-3.4.4-3", "openssl-3.0.18-0",
                     "sqlite-3.50.4-0", "xz-5.4.6-1", "zstd-1.5.7-1"]:
        filename = f"{name_ver}-{host}.tar.gz"
        download(f"{deps_url}/{name_ver}/{filename}")
        shutil.unpack_archive(filename)
        os.remove(filename)


def download(url, target_dir="."):
    out_path = f"{target_dir}/{basename(url)}"
    run(["curl", "-Lf", "--retry", "5", "--retry-all-errors", "-o", out_path, url])
    return out_path


def configure_host_python(context):
    if context.clean:
        clean(context.host)

    host_dir = subdir(context.host, create=True)
    prefix_dir = host_dir / "prefix"
    if not prefix_dir.exists():
        prefix_dir.mkdir()
        unpack_deps(context.host, prefix_dir)

    os.chdir(host_dir)
    command = [
        # Basic cross-compiling configuration
        relpath(PYTHON_DIR / "configure"),
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
    # delete any previous Python installation to prevent it being used during
    # the build.
    host_dir = subdir(context.host)
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


def build_all(context):
    steps = [configure_build_python, make_build_python, configure_host_python,
             make_host_python]
    for step in steps:
        step(context)


def clean(host):
    delete_glob(CROSS_BUILD_DIR / host)


def clean_all(context):
    for host in HOSTS + ["build"]:
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


def setup_sdk():
    sdkmanager = android_home / (
        "cmdline-tools/latest/bin/sdkmanager"
        + (".bat" if os.name == "nt" else "")
    )

    # Gradle will fail if it needs to install an SDK package whose license
    # hasn't been accepted, so pre-accept all licenses.
    if not all((android_home / "licenses" / path).exists() for path in [
        "android-sdk-arm-dbt-license", "android-sdk-license"
    ]):
        run(
            [sdkmanager, "--licenses"],
            text=True,
            capture_output=True,
            input="y\n" * 100,
        )

    # Gradle may install this automatically, but we can't rely on that because
    # we need to run adb within the logcat task.
    if not adb.exists():
        run([sdkmanager, "platform-tools"])


# To avoid distributing compiled artifacts without corresponding source code,
# the Gradle wrapper is not included in the CPython repository. Instead, we
# extract it from the Gradle GitHub repository.
def setup_testbed():
    paths = ["gradlew", "gradlew.bat", "gradle/wrapper/gradle-wrapper.jar"]
    if all((TESTBED_DIR / path).exists() for path in paths):
        return

    # The wrapper version isn't important, as any version of the wrapper can
    # download any version of Gradle. The Gradle version actually used for the
    # build is specified in testbed/gradle/wrapper/gradle-wrapper.properties.
    version = "8.9.0"

    for path in paths:
        out_path = TESTBED_DIR / path
        out_path.parent.mkdir(exist_ok=True)
        download(
            f"https://raw.githubusercontent.com/gradle/gradle/v{version}/{path}",
            out_path.parent,
        )
        os.chmod(out_path, 0o755)


# run_testbed will build the app automatically, but it's useful to have this as
# a separate command to allow running the app outside of this script.
def build_testbed(context):
    setup_sdk()
    setup_testbed()
    run(
        [gradlew, "--console", "plain", "packageDebug", "packageDebugAndroidTest"],
        cwd=TESTBED_DIR,
    )


# Work around a bug involving sys.exit and TaskGroups
# (https://github.com/python/cpython/issues/101515).
def exit(*args):
    raise MySystemExit(*args)


class MySystemExit(Exception):
    pass


# The `test` subcommand runs all subprocesses through this context manager so
# that no matter what happens, they can always be cancelled from another task,
# and they will always be cleaned up on exit.
@asynccontextmanager
async def async_process(*args, **kwargs):
    process = await asyncio.create_subprocess_exec(*args, **kwargs)
    try:
        yield process
    finally:
        if process.returncode is None:
            # Allow a reasonably long time for Gradle to clean itself up,
            # because we don't want stale emulators left behind.
            timeout = 10
            process.terminate()
            try:
                await wait_for(process.wait(), timeout)
            except TimeoutError:
                print(
                    f"Command {args} did not terminate after {timeout} seconds "
                    f" - sending SIGKILL"
                )
                process.kill()

                # Even after killing the process we must still wait for it,
                # otherwise we'll get the warning "Exception ignored in __del__".
                await wait_for(process.wait(), timeout=1)


async def async_check_output(*args, **kwargs):
    async with async_process(
        *args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, **kwargs
    ) as process:
        stdout, stderr = await process.communicate()
        if process.returncode == 0:
            return stdout.decode(*DECODE_ARGS)
        else:
            raise CalledProcessError(
                process.returncode, args,
                stdout.decode(*DECODE_ARGS), stderr.decode(*DECODE_ARGS)
            )


# Return a list of the serial numbers of connected devices. Emulators will have
# serials of the form "emulator-5678".
async def list_devices():
    serials = []
    header_found = False

    lines = (await async_check_output(adb, "devices")).splitlines()
    for line in lines:
        # Ignore blank lines, and all lines before the header.
        line = line.strip()
        if line == "List of devices attached":
            header_found = True
        elif header_found and line:
            try:
                serial, status = line.split()
            except ValueError:
                raise ValueError(f"failed to parse {line!r}")
            if status == "device":
                serials.append(serial)

    if not header_found:
        raise ValueError(f"failed to parse {lines}")
    return serials


async def find_device(context, initial_devices):
    if context.managed:
        print("Waiting for managed device - this may take several minutes")
        while True:
            new_devices = set(await list_devices()).difference(initial_devices)
            if len(new_devices) == 0:
                await asyncio.sleep(1)
            elif len(new_devices) == 1:
                serial = new_devices.pop()
                print(f"Serial: {serial}")
                return serial
            else:
                exit(f"Found more than one new device: {new_devices}")
    else:
        return context.connected


# An older version of this script in #121595 filtered the logs by UID instead.
# But logcat can't filter by UID until API level 31. If we ever switch back to
# filtering by UID, we'll also have to filter by time so we only show messages
# produced after the initial call to `stop_app`.
#
# We're more likely to miss the PID because it's shorter-lived, so there's a
# workaround in PythonSuite.kt to stop it being *too* short-lived.
async def find_pid(serial):
    print("Waiting for app to start - this may take several minutes")
    shown_error = False
    while True:
        try:
            # `pidof` requires API level 24 or higher. The level 23 emulator
            # includes it, but it doesn't work (it returns all processes).
            pid = (await async_check_output(
                adb, "-s", serial, "shell", "pidof", "-s", APP_ID
            )).strip()
        except CalledProcessError as e:
            # If the app isn't running yet, pidof gives no output. So if there
            # is output, there must have been some other error. However, this
            # sometimes happens transiently, especially when running a managed
            # emulator for the first time, so don't make it fatal.
            if (e.stdout or e.stderr) and not shown_error:
                print_called_process_error(e)
                print("This may be transient, so continuing to wait")
                shown_error = True
        else:
            # Some older devices (e.g. Nexus 4) return zero even when no process
            # was found, so check whether we actually got any output.
            if pid:
                print(f"PID: {pid}")
                return pid

        # Loop fairly rapidly to avoid missing a short-lived process.
        await asyncio.sleep(0.2)


async def logcat_task(context, initial_devices):
    # Gradle may need to do some large downloads of libraries and emulator
    # images. This will happen during find_device in --managed mode, or find_pid
    # in --connected mode.
    startup_timeout = 600
    serial = await wait_for(find_device(context, initial_devices), startup_timeout)
    pid = await wait_for(find_pid(serial), startup_timeout)

    # `--pid` requires API level 24 or higher.
    #
    # `--binary` mode is used in order to detect which messages end with a
    # newline, which most of the other modes don't indicate (except `--format
    # long`). For example, every time pytest runs a test, it prints a "." and
    # flushes the stream. Each "." becomes a separate log message, but we should
    # show them all on the same line.
    args = [adb, "-s", serial, "logcat", "--pid", pid,  "--binary"]
    logcat_started = False
    async with async_process(
        *args, stdout=subprocess.PIPE, stderr=None
    ) as process:
        while True:
            try:
                priority, tag, message = await read_logcat(process.stdout)
                logcat_started = True
            except asyncio.IncompleteReadError:
                break

            # Exclude high-volume messages which are rarely useful.
            if context.verbose < 2 and "from python test_syslog" in message:
                continue

            # Put high-level messages on stderr so they're highlighted in the
            # buildbot logs. This will include Python's own stderr.
            stream = sys.stderr if priority >= LogPriority.WARN else sys.stdout

            # The app's stdout and stderr should be passed through transparently
            # to our own corresponding streams.
            if tag in ["python.stdout", "python.stderr"]:
                global python_started
                python_started = True
                stream.write(message)
                stream.flush()
            else:
                # Non-Python messages add a lot of noise, but they may
                # sometimes help explain a failure. Format them in the same way
                # as `logcat --format tag`.
                formatted = f"{priority.name[0]}/{tag}: {message}"
                if not formatted.endswith("\n"):
                    formatted += "\n"
                log_verbose(context, formatted, stream)

        # If the device disconnects while logcat is running, which always
        # happens in --managed mode, some versions of adb return non-zero.
        # Distinguish this from a logcat startup error by checking whether we've
        # received any logcat messages yet.
        status = await wait_for(process.wait(), timeout=1)
        if status != 0 and not logcat_started:
            raise CalledProcessError(status, args)


# Read one binary log message from the given StreamReader. The message format is
# described at https://android.stackexchange.com/a/74660. All supported versions
# of Android use format version 2 or later.
async def read_logcat(stream):
    async def read_bytes(size):
        return await stream.readexactly(size)

    async def read_int(size):
        return int.from_bytes(await read_bytes(size), "little")

    payload_len = await read_int(2)
    if payload_len < 2:
        # 1 byte for priority, 1 byte for null terminator of tag.
        raise ValueError(f"payload length {payload_len} is too short")

    header_len = await read_int(2)
    if header_len < 4:
        raise ValueError(f"header length {header_len} is too short")
    await read_bytes(header_len - 4)  # Ignore other header fields.

    priority_int = await read_int(1)
    try:
        priority = LogPriority(priority_int)
    except ValueError:
        priority = LogPriority.UNKNOWN

    payload_fields = (await read_bytes(payload_len - 1)).split(b"\0")
    if len(payload_fields) < 2:
        raise ValueError(
            f"payload {payload!r} does not contain at least 2 "
            f"null-separated fields"
        )
    tag, message, *_ = [
        field.decode(*DECODE_ARGS) for field in payload_fields
    ]
    return priority, tag, message


def stop_app(serial):
    run([adb, "-s", serial, "shell", "am", "force-stop", APP_ID], log=False)


async def gradle_task(context):
    env = os.environ.copy()
    if context.managed:
        task_prefix = context.managed
    else:
        task_prefix = "connected"
        env["ANDROID_SERIAL"] = context.connected

    if context.ci_mode:
        context.args[0:0] = [
            # See _add_ci_python_opts in libregrtest/main.py.
            "-W", "error", "-bb", "-E",

            # Randomization is disabled because order-dependent failures are
            # much less likely to pass on a rerun in single-process mode.
            "-m", "test",
            f"--{context.ci_mode}-ci", "--single-process", "--no-randomize"
        ]

    if not any(arg in context.args for arg in ["-c", "-m"]):
        context.args[0:0] = ["-m", "test"]

    args = [
        gradlew, "--console", "plain", f"{task_prefix}DebugAndroidTest",
    ] + [
        f"-P{name}={value}"
        for name, value in [
            ("python.sitePackages", context.site_packages),
            ("python.cwd", context.cwd),
            (
                "android.testInstrumentationRunnerArguments.pythonArgs",
                json.dumps(context.args),
            ),
        ]
        if value
    ]
    if context.verbose >= 2:
        args.append("--info")
    log_verbose(context, f"> {join_command(args)}\n")

    try:
        async with async_process(
            *args, cwd=TESTBED_DIR, env=env,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        ) as process:
            while line := (await process.stdout.readline()).decode(*DECODE_ARGS):
                # Gradle may take several minutes to install SDK packages, so
                # it's worth showing those messages even in non-verbose mode.
                if line.startswith('Preparing "Install'):
                    sys.stdout.write(line)
                else:
                    log_verbose(context, line)

            status = await wait_for(process.wait(), timeout=1)
            if status == 0:
                exit(0)
            else:
                raise CalledProcessError(status, args)
    finally:
        # Gradle does not stop the tests when interrupted.
        if context.connected:
            stop_app(context.connected)


async def run_testbed(context):
    setup_ci()
    setup_sdk()
    setup_testbed()

    if context.managed:
        # In this mode, Gradle will create a device with an unpredictable name.
        # So we save a list of the running devices before starting Gradle, and
        # find_device then waits for a new device to appear.
        initial_devices = await list_devices()
    else:
        # In case the previous shutdown was unclean, make sure the app isn't
        # running, otherwise we might show logs from a previous run. This is
        # unnecessary in --managed mode, because Gradle creates a new emulator
        # every time.
        stop_app(context.connected)
        initial_devices = None

    try:
        async with asyncio.TaskGroup() as tg:
            tg.create_task(logcat_task(context, initial_devices))
            tg.create_task(gradle_task(context))
    except* MySystemExit as e:
        raise SystemExit(*e.exceptions[0].args) from None
    except* CalledProcessError as e:
        # If Python produced no output, then the user probably wants to see the
        # verbose output to explain why the test failed.
        if not python_started:
            for stream, line in hidden_output:
                stream.write(line)

        # Extract it from the ExceptionGroup so it can be handled by `main`.
        raise e.exceptions[0]


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

    with TemporaryDirectory(prefix=SCRIPT_NAME) as temp_dir:
        temp_dir = Path(temp_dir)

        # Include all tracked files from the Android directory.
        for line in run(
            ["git", "ls-files"],
            cwd=ANDROID_DIR, capture_output=True, text=True, log=False,
        ).stdout.splitlines():
            src = ANDROID_DIR / line
            dst = temp_dir / line
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
        with TemporaryDirectory(prefix=SCRIPT_NAME) as temp_dir:
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


# Handle SIGTERM the same way as SIGINT. This ensures that if we're terminated
# by the buildbot worker, we'll make an attempt to clean up our subprocesses.
def install_signal_handler():
    def signal_handler(*args):
        os.kill(os.getpid(), signal.SIGINT)

    signal.signal(signal.SIGTERM, signal_handler)


def parse_args():
    parser = argparse.ArgumentParser()
    subcommands = parser.add_subparsers(dest="subcommand", required=True)

    def add_parser(*args, **kwargs):
        parser = subcommands.add_parser(*args, **kwargs)
        parser.add_argument(
            "-v", "--verbose", action="count", default=0,
            help="Show verbose output. Use twice to be even more verbose.")
        return parser

    # Subcommands
    build = add_parser(
        "build", help="Run configure-build, make-build, configure-host and "
        "make-host")
    configure_build = add_parser(
        "configure-build", help="Run `configure` for the build Python")
    add_parser(
        "make-build", help="Run `make` for the build Python")
    configure_host = add_parser(
        "configure-host", help="Run `configure` for Android")
    make_host = add_parser(
        "make-host", help="Run `make` for Android")

    add_parser("clean", help="Delete all build directories")
    add_parser("build-testbed", help="Build the testbed app")
    test = add_parser("test", help="Run the testbed app")
    package = add_parser("package", help="Make a release package")
    ci = add_parser("ci", help="Run build, package and test")
    env = add_parser("env", help="Print environment variables")

    # Common arguments
    for subcommand in [build, configure_build, configure_host, ci]:
        subcommand.add_argument(
            "--clean", action="store_true", default=False, dest="clean",
            help="Delete the relevant build directories first")

    host_commands = [build, configure_host, make_host, package, ci]
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
    device_group = test.add_mutually_exclusive_group(required=True)
    device_group.add_argument(
        "--connected", metavar="SERIAL", help="Run on a connected device. "
        "Connect it yourself, then get its serial from `adb devices`.")
    device_group.add_argument(
        "--managed", metavar="NAME", help="Run on a Gradle-managed device. "
        "These are defined in `managedDevices` in testbed/app/build.gradle.kts.")

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
    install_signal_handler()

    # Under the buildbot, stdout is not a TTY, but we must still flush after
    # every line to make sure our output appears in the correct order relative
    # to the output of our subprocesses.
    for stream in [sys.stdout, sys.stderr]:
        stream.reconfigure(line_buffering=True)

    context = parse_args()
    dispatch = {
        "configure-build": configure_build_python,
        "make-build": make_build_python,
        "configure-host": configure_host_python,
        "make-host": make_host_python,
        "build": build_all,
        "clean": clean_all,
        "build-testbed": build_testbed,
        "test": run_testbed,
        "package": package,
        "ci": ci,
        "env": env,
    }

    try:
        result = dispatch[context.subcommand](context)
        if asyncio.iscoroutine(result):
            asyncio.run(result)
    except CalledProcessError as e:
        print_called_process_error(e)
        sys.exit(1)


def print_called_process_error(e):
    for stream_name in ["stdout", "stderr"]:
        content = getattr(e, stream_name)
        if isinstance(content, bytes):
            content = content.decode(*DECODE_ARGS)
        stream = getattr(sys, stream_name)
        if content:
            stream.write(content)
            if not content.endswith("\n"):
                stream.write("\n")

    # shlex uses single quotes, so we surround the command with double quotes.
    print(
        f'Command "{join_command(e.cmd)}" returned exit status {e.returncode}'
    )


if __name__ == "__main__":
    main()
