import argparse
import asyncio
import fcntl
import json
import os
import plistlib
import re
import shutil
import subprocess
import sys
import tempfile
from contextlib import asynccontextmanager
from datetime import datetime
from pathlib import Path


DECODE_ARGS = ("UTF-8", "backslashreplace")

# The system log prefixes each line:
#   2025-01-17 16:14:29.090 Df iOSTestbed[23987:1fd393b4] (Python) ...
#   2025-01-17 16:14:29.090 E  iOSTestbed[23987:1fd393b4] (Python) ...

LOG_PREFIX_REGEX = re.compile(
    r"^\d{4}-\d{2}-\d{2}"  # YYYY-MM-DD
    r"\s+\d+:\d{2}:\d{2}\.\d+"  # HH:MM:SS.sss
    r"\s+\w+"  # Df/E
    r"\s+iOSTestbed\[\d+:\w+\]"  # Process/thread ID
    r"\s+\(Python\)\s"  # Logger name
)


# Work around a bug involving sys.exit and TaskGroups
# (https://github.com/python/cpython/issues/101515).
def exit(*args):
    raise MySystemExit(*args)


class MySystemExit(Exception):
    pass


class SimulatorLock:
    # An fcntl-based filesystem lock that can be used to ensure that
    def __init__(self, timeout):
        self.filename = Path(tempfile.gettempdir()) / "python-ios-testbed"
        self.timeout = timeout

        self.fd = None

    async def acquire(self):
        # Ensure the lockfile exists
        self.filename.touch(exist_ok=True)

        # Try `timeout` times to acquire the lock file, with a 1 second pause
        # between each attempt. Report status every 10 seconds.
        for i in range(0, self.timeout):
            try:
                fd = os.open(self.filename, os.O_RDWR | os.O_TRUNC, 0o644)
                fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
            except OSError:
                os.close(fd)
                if i % 10 == 0:
                    print("... waiting", flush=True)
                await asyncio.sleep(1)
            else:
                self.fd = fd
                return

        # If we reach the end of the loop, we've exceeded the allowed number of
        # attempts.
        raise ValueError("Unable to obtain lock on iOS simulator creation")

    def release(self):
        # If a lock is held, release it.
        if self.fd is not None:
            # Release the lock.
            fcntl.flock(self.fd, fcntl.LOCK_UN)
            os.close(self.fd)
            self.fd = None


# All subprocesses are executed through this context manager so that no matter
# what happens, they can always be cancelled from another task, and they will
# always be cleaned up on exit.
@asynccontextmanager
async def async_process(*args, **kwargs):
    process = await asyncio.create_subprocess_exec(*args, **kwargs)
    try:
        yield process
    finally:
        if process.returncode is None:
            # Allow a reasonably long time for Xcode to clean itself up,
            # because we don't want stale emulators left behind.
            timeout = 10
            process.terminate()
            try:
                await asyncio.wait_for(process.wait(), timeout)
            except TimeoutError:
                print(
                    f"Command {args} did not terminate after {timeout} seconds "
                    f" - sending SIGKILL"
                )
                process.kill()

                # Even after killing the process we must still wait for it,
                # otherwise we'll get the warning "Exception ignored in __del__".
                await asyncio.wait_for(process.wait(), timeout=1)


async def async_check_output(*args, **kwargs):
    async with async_process(
        *args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, **kwargs
    ) as process:
        stdout, stderr = await process.communicate()
        if process.returncode == 0:
            return stdout.decode(*DECODE_ARGS)
        else:
            raise subprocess.CalledProcessError(
                process.returncode,
                args,
                stdout.decode(*DECODE_ARGS),
                stderr.decode(*DECODE_ARGS),
            )


# Select a simulator device to use.
async def select_simulator_device():
    # List the testing simulators, in JSON format
    raw_json = await async_check_output(
        "xcrun", "simctl", "--set", "testing", "list", "-j"
    )
    json_data = json.loads(raw_json)

    # Any device will do; we'll look for "SE" devices - but the name isn't
    # consistent over time. Older Xcode versions will use "iPhone SE (Nth
    # generation)"; As of 2025, they've started using "iPhone 16e".
    #
    # When Xcode is updated after a new release, new devices will be available
    # and old ones will be dropped from the set available on the latest iOS
    # version. Select the one with the highest minimum runtime version - this
    # is an indicator of the "newest" released device, which should always be
    # supported on the "most recent" iOS version.
    se_simulators = sorted(
        (devicetype["minRuntimeVersion"], devicetype["name"])
        for devicetype in json_data["devicetypes"]
        if devicetype["productFamily"] == "iPhone"
        and (
            ("iPhone " in devicetype["name"] and devicetype["name"].endswith("e"))
            or "iPhone SE " in devicetype["name"]
        )
    )

    return se_simulators[-1][1]


# Return a list of UDIDs associated with booted simulators
async def list_devices():
    try:
        # List the testing simulators, in JSON format
        raw_json = await async_check_output(
            "xcrun", "simctl", "--set", "testing", "list", "-j"
        )
        json_data = json.loads(raw_json)

        # Filter out the booted iOS simulators
        return [
            simulator["udid"]
            for runtime, simulators in json_data["devices"].items()
            for simulator in simulators
            if runtime.split(".")[-1].startswith("iOS") and simulator["state"] == "Booted"
        ]
    except subprocess.CalledProcessError as e:
        # If there's no ~/Library/Developer/XCTestDevices folder (which is the
        # case on fresh installs, and in some CI environments), `simctl list`
        # returns error code 1, rather than an empty list. Handle that case,
        # but raise all other errors.
        if e.returncode == 1:
            return []
        else:
            raise


async def find_device(initial_devices, lock):
    while True:
        new_devices = set(await list_devices()).difference(initial_devices)
        if len(new_devices) == 0:
            await asyncio.sleep(1)
        elif len(new_devices) == 1:
            udid = new_devices.pop()
            print(f"{datetime.now():%Y-%m-%d %H:%M:%S}: New test simulator detected")
            print(f"UDID: {udid}", flush=True)
            lock.release()
            return udid
        else:
            exit(f"Found more than one new device: {new_devices}")


async def log_stream_task(initial_devices, lock):
    # Wait up to 5 minutes for the build to complete and the simulator to boot.
    udid = await asyncio.wait_for(find_device(initial_devices, lock), 5 * 60)

    # Stream the iOS device's logs, filtering out messages that come from the
    # XCTest test suite (catching NSLog messages from the test method), or
    # Python itself (catching stdout/stderr content routed to the system log
    # with config->use_system_logger).
    args = [
        "xcrun",
        "simctl",
        "--set",
        "testing",
        "spawn",
        udid,
        "log",
        "stream",
        "--style",
        "compact",
        "--predicate",
        (
            'senderImagePath ENDSWITH "/iOSTestbedTests.xctest/iOSTestbedTests"'
            ' OR senderImagePath ENDSWITH "/Python.framework/Python"'
        ),
    ]

    async with async_process(
        *args,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    ) as process:
        suppress_dupes = False
        while line := (await process.stdout.readline()).decode(*DECODE_ARGS):
            # Strip the prefix from each log line
            line = LOG_PREFIX_REGEX.sub("", line)
            # The iOS log streamer can sometimes lag; when it does, it outputs
            # a warning about messages being dropped... often multiple times.
            # Only print the first of these duplicated warnings.
            if line.startswith("=== Messages dropped "):
                if not suppress_dupes:
                    suppress_dupes = True
                    sys.stdout.write(line)
            else:
                suppress_dupes = False
                sys.stdout.write(line)
            sys.stdout.flush()


async def xcode_test(location, simulator, verbose):
    # Run the test suite on the named simulator
    print("Starting xcodebuild...", flush=True)
    args = [
        "xcodebuild",
        "test",
        "-project",
        str(location / "iOSTestbed.xcodeproj"),
        "-scheme",
        "iOSTestbed",
        "-destination",
        f"platform=iOS Simulator,name={simulator}",
        "-resultBundlePath",
        str(location / f"{datetime.now():%Y%m%d-%H%M%S}.xcresult"),
        "-derivedDataPath",
        str(location / "DerivedData"),
    ]
    if not verbose:
        args += ["-quiet"]

    async with async_process(
        *args,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    ) as process:
        while line := (await process.stdout.readline()).decode(*DECODE_ARGS):
            sys.stdout.write(line)
            sys.stdout.flush()

        status = await asyncio.wait_for(process.wait(), timeout=1)
        exit(status)


def clone_testbed(
    source: Path,
    target: Path,
    framework: Path,
    apps: list[Path],
) -> None:
    if target.exists():
        print(f"{target} already exists; aborting without creating project.")
        sys.exit(10)

    if framework is None:
        if not (
            source / "Python.xcframework/ios-arm64_x86_64-simulator/bin"
        ).is_dir():
            print(
                f"The testbed being cloned ({source}) does not contain "
                f"a simulator framework. Re-run with --framework"
            )
            sys.exit(11)
    else:
        if not framework.is_dir():
            print(f"{framework} does not exist.")
            sys.exit(12)
        elif not (
            framework.suffix == ".xcframework"
            or (framework / "Python.framework").is_dir()
        ):
            print(
                f"{framework} is not an XCframework, "
                f"or a simulator slice of a framework build."
            )
            sys.exit(13)

    print("Cloning testbed project:")
    print(f"  Cloning {source}...", end="", flush=True)
    shutil.copytree(source, target, symlinks=True)
    print(" done")

    xc_framework_path = target / "Python.xcframework"
    sim_framework_path = xc_framework_path / "ios-arm64_x86_64-simulator"
    if framework is not None:
        if framework.suffix == ".xcframework":
            print("  Installing XCFramework...", end="", flush=True)
            if xc_framework_path.is_dir():
                shutil.rmtree(xc_framework_path)
            else:
                xc_framework_path.unlink(missing_ok=True)
            xc_framework_path.symlink_to(
                framework.relative_to(xc_framework_path.parent, walk_up=True)
            )
            print(" done")
        else:
            print("  Installing simulator framework...", end="", flush=True)
            if sim_framework_path.is_dir():
                shutil.rmtree(sim_framework_path)
            else:
                sim_framework_path.unlink(missing_ok=True)
            sim_framework_path.symlink_to(
                framework.relative_to(sim_framework_path.parent, walk_up=True)
            )
            print(" done")
    else:
        if (
            xc_framework_path.is_symlink()
            and not xc_framework_path.readlink().is_absolute()
        ):
            # XCFramework is a relative symlink. Rewrite the symlink relative
            # to the new location.
            print("  Rewriting symlink to XCframework...", end="", flush=True)
            orig_xc_framework_path = (
                source
                / xc_framework_path.readlink()
            ).resolve()
            xc_framework_path.unlink()
            xc_framework_path.symlink_to(
                orig_xc_framework_path.relative_to(
                    xc_framework_path.parent, walk_up=True
                )
            )
            print(" done")
        elif (
            sim_framework_path.is_symlink()
            and not sim_framework_path.readlink().is_absolute()
        ):
            print("  Rewriting symlink to simulator framework...", end="", flush=True)
            # Simulator framework is a relative symlink. Rewrite the symlink
            # relative to the new location.
            orig_sim_framework_path = (
                source
                / "Python.XCframework"
                / sim_framework_path.readlink()
            ).resolve()
            sim_framework_path.unlink()
            sim_framework_path.symlink_to(
                orig_sim_framework_path.relative_to(
                    sim_framework_path.parent, walk_up=True
                )
            )
            print(" done")
        else:
            print("  Using pre-existing iOS framework.")

    for app_src in apps:
        print(f"  Installing app {app_src.name!r}...", end="", flush=True)
        app_target = target / f"iOSTestbed/app/{app_src.name}"
        if app_target.is_dir():
            shutil.rmtree(app_target)
        shutil.copytree(app_src, app_target)
        print(" done")

    print(f"Successfully cloned testbed: {target.resolve()}")


def update_plist(testbed_path, args):
    # Add the test runner arguments to the testbed's Info.plist file.
    info_plist = testbed_path / "iOSTestbed" / "iOSTestbed-Info.plist"
    with info_plist.open("rb") as f:
        info = plistlib.load(f)

    info["TestArgs"] = args

    with info_plist.open("wb") as f:
        plistlib.dump(info, f)


async def run_testbed(simulator: str | None, args: list[str], verbose: bool=False):
    location = Path(__file__).parent
    print("Updating plist...", end="", flush=True)
    update_plist(location, args)
    print(" done.", flush=True)

    if simulator is None:
        simulator = await select_simulator_device()
    print(f"Running test on {simulator}", flush=True)

    # We need to get an exclusive lock on simulator creation, to avoid issues
    # with multiple simulators starting and being unable to tell which
    # simulator is due to which testbed instance. See
    # https://github.com/python/cpython/issues/130294 for details. Wait up to
    # 10 minutes for a simulator to boot.
    print("Obtaining lock on simulator creation...", flush=True)
    simulator_lock = SimulatorLock(timeout=10*60)
    await simulator_lock.acquire()
    print("Simulator lock acquired.", flush=True)

    # Get the list of devices that are booted at the start of the test run.
    # The simulator started by the test suite will be detected as the new
    # entry that appears on the device list.
    initial_devices = await list_devices()

    try:
        async with asyncio.TaskGroup() as tg:
            tg.create_task(log_stream_task(initial_devices, simulator_lock))
            tg.create_task(xcode_test(location, simulator=simulator, verbose=verbose))
    except* MySystemExit as e:
        raise SystemExit(*e.exceptions[0].args) from None
    except* subprocess.CalledProcessError as e:
        # Extract it from the ExceptionGroup so it can be handled by `main`.
        raise e.exceptions[0]
    finally:
        simulator_lock.release()


def main():
    parser = argparse.ArgumentParser(
        description=(
            "Manages the process of testing a Python project in the iOS simulator."
        ),
    )

    subcommands = parser.add_subparsers(dest="subcommand")

    clone = subcommands.add_parser(
        "clone",
        description=(
            "Clone the testbed project, copying in an iOS Python framework and"
            "any specified application code."
        ),
        help="Clone a testbed project to a new location.",
    )
    clone.add_argument(
        "--framework",
        help=(
            "The location of the XCFramework (or simulator-only slice of an "
            "XCFramework) to use when running the testbed"
        ),
    )
    clone.add_argument(
        "--app",
        dest="apps",
        action="append",
        default=[],
        help="The location of any code to include in the testbed project",
    )
    clone.add_argument(
        "location",
        help="The path where the testbed will be cloned.",
    )

    run = subcommands.add_parser(
        "run",
        usage="%(prog)s [-h] [--simulator SIMULATOR] -- <test arg> [<test arg> ...]",
        description=(
            "Run a testbed project. The arguments provided after `--` will be "
            "passed to the running iOS process as if they were arguments to "
            "`python -m`."
        ),
        help="Run a testbed project",
    )
    run.add_argument(
        "--simulator",
        help=(
            "The name of the simulator to use (eg: 'iPhone 16e'). Defaults to ",
            "the most recently released 'entry level' iPhone device."
        )
    )
    run.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Enable verbose output",
    )

    try:
        pos = sys.argv.index("--")
        testbed_args = sys.argv[1:pos]
        test_args = sys.argv[pos + 1 :]
    except ValueError:
        testbed_args = sys.argv[1:]
        test_args = []

    context = parser.parse_args(testbed_args)

    if context.subcommand == "clone":
        clone_testbed(
            source=Path(__file__).parent.resolve(),
            target=Path(context.location).resolve(),
            framework=Path(context.framework).resolve() if context.framework else None,
            apps=[Path(app) for app in context.apps],
        )
    elif context.subcommand == "run":
        if test_args:
            if not (
                Path(__file__).parent / "Python.xcframework/ios-arm64_x86_64-simulator/bin"
            ).is_dir():
                print(
                    f"Testbed does not contain a compiled iOS framework. Use "
                    f"`python {sys.argv[0]} clone ...` to create a runnable "
                    f"clone of this testbed."
                )
                sys.exit(20)

            asyncio.run(
                run_testbed(
                    simulator=context.simulator,
                    verbose=context.verbose,
                    args=test_args,
                )
            )
        else:
            print(f"Must specify test arguments (e.g., {sys.argv[0]} run -- test)")
            print()
            parser.print_help(sys.stderr)
            sys.exit(21)
    else:
        parser.print_help(sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
