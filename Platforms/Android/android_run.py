#!/usr/bin/env python3

import argparse
import asyncio
import os
import re
import shlex
import signal
import subprocess
import sys
from asyncio import wait_for
from contextlib import asynccontextmanager
from enum import IntEnum, auto
from pathlib import Path
from subprocess import CalledProcessError


DECODE_ARGS = ("UTF-8", "backslashreplace")

try:
    android_home = Path(os.environ['ANDROID_HOME'])
except KeyError:
    sys.exit("The ANDROID_HOME environment variable is required.")

adb = Path(
    f"{android_home}/platform-tools/adb"
    + (".exe" if os.name == "nt" else "")
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


def run(command, *, log=True, **kwargs):
    kwargs.setdefault("check", True)
    if log:
        print(">", join_command(command))
    return subprocess.run(command, **kwargs)


# Format a command so it can be copied into a shell. Like shlex.join, but also
# accepts arguments which are Paths, or a single string/Path outside of a list.
def join_command(args):
    if isinstance(args, (str, Path)):
        return str(args)
    else:
        return shlex.join(map(str, args))


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
async def find_pid(serial, app_id):
    print("Waiting for app to start - this may take several minutes")
    shown_error = False
    while True:
        try:
            # `pidof` requires API level 24 or higher. The level 23 emulator
            # includes it, but it doesn't work (it returns all processes).
            pid = (await async_check_output(
                adb, "-s", serial, "shell", "pidof", "-s", app_id
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


async def logcat_task(context, app_id, initial_devices):
    # Gradle may need to do some large downloads of libraries and emulator
    # images. This will happen during find_device in --managed mode, or find_pid
    # in --connected mode.
    startup_timeout = 600
    serial = await wait_for(find_device(context, initial_devices), startup_timeout)
    pid = await wait_for(find_pid(serial, app_id), startup_timeout)

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

    payload = await read_bytes(payload_len - 1)
    payload_fields = payload.split(b"\0")
    if len(payload_fields) < 2:
        raise ValueError(
            f"payload {payload!r} does not contain at least 2 "
            f"null-separated fields"
        )
    tag, message, *_ = [
        field.decode(*DECODE_ARGS) for field in payload_fields
    ]
    return priority, tag, message


def stop_app(serial, app_id):
    run([adb, "-s", serial, "shell", "am", "force-stop", app_id], log=False)


async def gradle_task(context, app_id):
    env = os.environ.copy()
    if context.managed:
        task_prefix = context.managed
    else:
        task_prefix = "connected"
        env["ANDROID_SERIAL"] = context.connected

    gradlew = Path(
        f"{context.project_dir}/gradlew"
        + (".bat" if os.name == "nt" else "")
    )
    args = [
        gradlew, "--console", "plain", f"{task_prefix}DebugAndroidTest"
    ]
    for prop in context.gradle_properties:
        args += ["-P", prop]
    if context.verbose >= 2:
        args.append("--info")
    log_verbose(context, f"> {join_command(args)}\n")

    try:
        async with async_process(
            *args, cwd=context.project_dir, env=env,
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
            stop_app(context.connected, app_id)


def find_app_id(project_dir):
    app_dir = project_dir / "app"
    if not app_dir.exists():
        sys.exit(f"{app_dir} does not exist")

    build_gradle_names = ["build.gradle", "build.gradle.kts"]
    for name in build_gradle_names:
        build_gradle = app_dir / name
        if build_gradle.exists():
            break
    else:
        sys.exit(f"None of {build_gradle_names} were found in {app_dir}")

    pattern = r'^\s*applicationId(\s*=\s*|\s+)"(.+)"\s*$'
    if match := re.search(pattern, build_gradle.read_text(), re.MULTILINE):
        return match[2]
    else:
        sys.exit(f"applicationId not found in {build_gradle}")


async def run_app(context):
    setup_sdk()
    app_id = find_app_id(context.project_dir)

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
        stop_app(context.connected, app_id)
        initial_devices = None

    try:
        async with asyncio.TaskGroup() as tg:
            tg.create_task(logcat_task(context, app_id, initial_devices))
            tg.create_task(gradle_task(context, app_id))
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


# Handle SIGTERM the same way as SIGINT. This ensures that if we're terminated by a
# buildbot worker, we'll receive a KeyboardInterrupt exception (which is handled
# specially by asyncio.TaskGroup) and make an attempt to clean up our subprocesses.
def install_signal_handler():
    def signal_handler(*args):
        os.kill(os.getpid(), signal.SIGINT)

    signal.signal(signal.SIGTERM, signal_handler)


def add_verbose_argument(parser):
    parser.add_argument(
        "-v", "--verbose", action="count", default=0,
        help="Show verbose output. Use twice to be even more verbose.")


def add_device_group(parser):
    device_group = parser.add_mutually_exclusive_group(required=True)
    device_group.add_argument(
        "--connected", metavar="SERIAL", help="Run on a connected device. "
        "Connect it yourself, then get its serial from `adb devices`.")
    device_group.add_argument(
        "--managed", metavar="NAME", help="Run on a Gradle-managed device. "
        "These are defined in the `managedDevices` section of build.gradle.")


def unparse_device_group(context):
    result = []
    for name in ["connected", "managed"]:
        value = getattr(context, name, None)
        if value:
            result += [f"--{name}", value]
    return result


def parse_args():
    parser = argparse.ArgumentParser(
        description="Run the test suite of an Android app"
    )
    add_verbose_argument(parser)
    add_device_group(parser)

    parser.add_argument(
        "-P", metavar="KEY=VALUE",
        dest="gradle_properties", action="append", default=[],
        help="Set a Gradle property"
    )
    parser.add_argument(
        "project_dir", type=Path, help="Gradle project directory"
    )

    return parser.parse_args()


# Under the buildbot, stdout is not a TTY, but we must still flush after
# every line to make sure our output appears in the correct order relative
# to the output of our subprocesses.
def configure_streams():
    for stream in [sys.stdout, sys.stderr]:
        stream.reconfigure(line_buffering=True)


def main():
    install_signal_handler()
    configure_streams()
    context = parse_args()

    try:
        asyncio.run(run_app(context))
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
