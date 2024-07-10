#!/usr/bin/env python3

import argparse
import atexit
import os
import re
import shlex
import shutil
import subprocess
import sys
import sysconfig
from os.path import basename, relpath
from pathlib import Path
from subprocess import check_output
from tempfile import TemporaryDirectory
from threading import Thread
from time import sleep


SCRIPT_NAME = Path(__file__).name
CHECKOUT = Path(__file__).resolve().parent.parent
ANDROID_DIR = CHECKOUT / "Android"
TESTBED_DIR = ANDROID_DIR / "testbed"
CROSS_BUILD_DIR = CHECKOUT / "cross-build"


try:
    android_home = os.environ['ANDROID_HOME']
except KeyError:
    sys.exit("The ANDROID_HOME environment variable is required.")

adb = Path(
    f"{android_home}/platform-tools/adb"
    + (".exe" if os.name == "nt" else "")
)
if not adb.exists():
    sys.exit(
        f"{adb} does not exist. Install the Platform Tools package using the "
        f"Android SDK manager."
    )


def delete_if_exists(path):
    if path.exists():
        print(f"Deleting {path} ...")
        shutil.rmtree(path)


def subdir(name, *, clean=None):
    path = CROSS_BUILD_DIR / name
    if clean:
        delete_if_exists(path)
    if not path.exists():
        if clean is None:
            sys.exit(
                f"{path} does not exist. Create it by running the appropriate "
                f"`configure` subcommand of {SCRIPT_NAME}.")
        else:
            path.mkdir(parents=True)
    return path


def run(command, *, host=None, env=None, **kwargs):
    if env is None:
        env = os.environ.copy()
    original_env = env.copy()

    if host:
        env_script = ANDROID_DIR / "android-env.sh"
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

        if env == original_env:
            raise ValueError(f"Found no variables in {env_script.name} output:\n"
                             + env_output)

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
    host_dir = subdir(context.host)
    os.chdir(host_dir / "build")
    run(["make", "-j", str(os.cpu_count())], host=context.host)
    run(["make", "install", f"prefix={host_dir}/prefix"], host=context.host)


def build_all(context):
    steps = [configure_build_python, make_build_python, configure_host_python,
             make_host_python]
    for step in steps:
        step(context)


def clean_all(context):
    delete_if_exists(CROSS_BUILD_DIR)


# To avoid distributing compiled artifacts without corresponding source code,
# the Gradle wrapper is not included in the CPython repository. Instead, we
# extract it from the Gradle release.
def setup_testbed(context):
    ver_long = "8.7.0"
    ver_short = ver_long.removesuffix(".0")

    for filename in ["gradlew", "gradlew.bat"]:
        out_path = download(
            f"https://raw.githubusercontent.com/gradle/gradle/v{ver_long}/{filename}",
            TESTBED_DIR)
        os.chmod(out_path, 0o755)

    with TemporaryDirectory(prefix=SCRIPT_NAME) as temp_dir:
        os.chdir(temp_dir)
        bin_zip = download(
            f"https://services.gradle.org/distributions/gradle-{ver_short}-bin.zip")
        outer_jar = f"gradle-{ver_short}/lib/plugins/gradle-wrapper-{ver_short}.jar"
        run(["unzip", bin_zip, outer_jar])
        run(["unzip", "-o", "-d", f"{TESTBED_DIR}/gradle/wrapper", outer_jar,
             "gradle-wrapper.jar"])


def list_devices():
    serials = []
    header_found = False

    lines = check_output([adb, "devices"], text=True).splitlines()
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


def wait_for_new_device(initial_devices):
    while True:
        new_devices = set(list_devices()).difference(initial_devices)
        if len(new_devices) == 0:
            sleep(1)
        elif len(new_devices) == 1:
            return new_devices.pop()
        else:
            sys.exit(f"Found more than one new device: {new_devices}")


def wait_for_uid():
    while True:
        lines = check_output(
            [adb, "shell", "pm", "list", "packages", "-U", "org.python.testbed"],
            text=True
        ).splitlines()

        if len(lines) == 0:
            sleep(1)
        elif len(lines) == 1:
            if match := re.search(r"uid:\d+", lines[0]):
                return match[1]
            else:
                raise ValueError(f"failed to parse {lines[0]!r}")
        else:
            sys.exit(f"Found more than one UID: {lines}")


def logcat_thread(context, initial_devices):
    serial = context.connected or wait_for_new_device(initial_devices)

    # Because Gradle uninstalls the app after running the tests, its UID should
    # be different every time. There's therefore no need to filter the logs by
    # timestamp or PID.
    uid = wait_for_uid()

    logcat = subprocess.Popen(
        [adb, "-s", serial, "logcat", "--uid", uid, "--format", "tag"],
        stdout=subprocess.PIPE, text=True
    )

    # This is a daemon thread, so `finally` won't work.
    atexit.register(logcat.kill)

    for line in logcat.stdout:
        if match := re.fullmatch(r"(\w)/(\w+): (.*)", line):
            level, tag, message = match.groups()
        else:
            # If the regex doesn't match, this is probably the second or
            # subsequent line of a multi-line message. Python won't produce
            # such messages, but other components might.
            level, tag, message = None, None, line

        stream = (
            sys.stderr
            if level in ["E", "F"]  # ERROR and FATAL (aka ASSERT)
            else sys.stdout
        )

        # We strip the level/tag indicator from Python's stdout and stderr, to
        # simplify automated processing of the output, e.g. a buildbot posting a
        # failure notice on a GitHub PR.
        #
        # Non-Python messages from the app are still worth keeping, as they may
        # help explain any problems.
        stream.write(
            message if tag in ["python.stdout", "python.stderr"] else line
        )

    status = logcat.wait()
    if status != 0:
        sys.exit(f"Logcat exit status {status}")


def run_testbed(context):
    if not (TESTBED_DIR / "gradlew").exists():
        setup_testbed(context)

    kwargs = dict(cwd=TESTBED_DIR)

    if context.connected:
        task_prefix = "connected"
        env = os.environ.copy()
        env["ANDROID_SERIAL"] = context.connected
        kwargs.update(env=env)
    elif context.managed:
        task_prefix = context.managed
    else:
        raise ValueError("no device argument found")

    Thread(
        target=logcat_thread, args=(context, list_devices()), daemon=True
    ).start()

    run(
        [
            "./gradlew",
            "--console", "plain",
            f"{task_prefix}DebugAndroidTest",
            "-Pandroid.testInstrumentationRunnerArguments.pythonArgs="
            + shlex.join(context.args),
        ],
        **kwargs
    )


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

    subcommands.add_parser(
        "setup-testbed", help="Download the testbed Gradle wrapper")
    test = subcommands.add_parser(
        "test", help="Run the test suite")
    device_group = test.add_mutually_exclusive_group(required=True)
    device_group.add_argument(
        "--connected", metavar="SERIAL", help="Run on a connected device. "
        "Connect it yourself, then get its serial from `adb devices`.")
    device_group.add_argument(
        "--managed", metavar="NAME", help="Run on a Gradle-managed device. "
        "These are defined in `managedDevices` in testbed/app/build.gradle.kts.")
    test.add_argument(
        "args", nargs="*", help="Extra arguments for `python -m test`")

    context = parser.parse_args()
    dispatch = {"configure-build": configure_build_python,
                "make-build": make_build_python,
                "configure-host": configure_host_python,
                "make-host": make_host_python,
                "build": build_all,
                "clean": clean_all,
                "setup-testbed": setup_testbed,
                "test": run_testbed}
    dispatch[context.subcommand](context)


if __name__ == "__main__":
    main()
