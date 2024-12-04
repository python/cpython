import argparse
import asyncio
import json
import plistlib
import shutil
import subprocess
import sys
from contextlib import asynccontextmanager
from datetime import datetime
from pathlib import Path


DECODE_ARGS = ("UTF-8", "backslashreplace")


# Work around a bug involving sys.exit and TaskGroups
# (https://github.com/python/cpython/issues/101515).
def exit(*args):
    raise MySystemExit(*args)


class MySystemExit(Exception):
    pass


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
                process.returncode, args,
                stdout.decode(*DECODE_ARGS), stderr.decode(*DECODE_ARGS)
            )


# Return a list of UDIDs associated with booted simulators
async def list_devices():
    # List the testing simulators, in JSON format
    raw_json = await async_check_output(
        "xcrun", "simctl", "--set", "testing", "list", "-j"
    )
    json_data = json.loads(raw_json)

    # Filter out the booted iOS simulators
    return [
        simulator["udid"]
        for runtime, simulators in json_data['devices'].items()
        for simulator in simulators
        if runtime.split(".")[-1].startswith("iOS")
        and simulator['state'] == "Booted"
    ]


async def find_device(initial_devices):
    while True:
        new_devices = set(await list_devices()).difference(initial_devices)
        if len(new_devices) == 0:
            await asyncio.sleep(1)
        elif len(new_devices) == 1:
            udid = new_devices.pop()
            print(f"{datetime.now():%Y%m%d %H%M%S}: New test simulator detected")
            print(f"UDID: {udid}")
            return udid
        else:
            exit(f"Found more than one new device: {new_devices}")


async def log_stream_task(initial_devices):
    # Wait up to 5 minutes for the build to complete and the simulator to boot.
    udid = await asyncio.wait_for(find_device(initial_devices), 5*60)

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
        )
    ]

    async with async_process(
        *args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    ) as process:
        while line := (await process.stdout.readline()).decode(*DECODE_ARGS):
            sys.stdout.write(line)


async def xcode_test(location, simulator):
    # Run the test suite on the named simulator
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
        str(location / "DerivedData",)
    ]
    async with async_process(
        *args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    ) as process:
        while line := (await process.stdout.readline()).decode(*DECODE_ARGS):
            sys.stdout.write(line)

        status = await asyncio.wait_for(process.wait(), timeout=1)
        exit(status)


def create_testbed(location: Path, framework: Path, apps: list[Path]) -> None:
    if location.exists():
        print(f"{location} already exists; aborting without creating project.")
        sys.exit(10)

    print("Copying template testbed project...")
    shutil.copytree(Path(__file__).parent, location)

    if framework.suffix == ".xcframework":
        print("Installing XCFramework...")
        xc_framework_path = location / "Python.xcframework"
        shutil.rmtree(xc_framework_path)
        shutil.copytree(framework, xc_framework_path)
    else:
        print("Installing simulator Framework...")
        sim_framework_path = (
            location
            / "Python.xcframework"
            / "ios-arm64_x86_64-simulator"
        )
        shutil.rmtree(sim_framework_path)
        shutil.copytree(framework, sim_framework_path)

    for app in apps:
        print(f"Installing app {app!r}...")
        shutil.copytree(app, location / "iOSTestbed/app/{app.name}")

    print(f"Testbed project created in {location}")


def update_plist(testbed_path, args):
    # Add the test runner arguments to the testbed's Info.plist file.
    info_plist = testbed_path / "iOSTestbed" / "iOSTestbed-Info.plist"
    with info_plist.open("rb") as f:
        info = plistlib.load(f)

    info["TestArgs"] = args

    with info_plist.open("wb") as f:
        plistlib.dump(info, f)


async def run_testbed(simulator: str, args: list[str]):
    location = Path(__file__).parent
    print("Updating plist...")
    update_plist(location, args)

    # Get the list of devices that are booted at the start of the test run.
    # The simulator started by the test suite will be detected as the new
    # entry that appears on the device list.
    initial_devices = await list_devices()

    try:
        async with asyncio.TaskGroup() as tg:
            tg.create_task(log_stream_task(initial_devices))
            tg.create_task(xcode_test(location, simulator))
    except* MySystemExit as e:
        raise SystemExit(*e.exceptions[0].args) from None
    except* subprocess.CalledProcessError as e:
        # Extract it from the ExceptionGroup so it can be handled by `main`.
        raise e.exceptions[0]


def main():
    parser = argparse.ArgumentParser(
        prog="testbed",
        description=(
            "Manages the process of testing a Python project in the iOS simulator"
        )
    )

    subcommands = parser.add_subparsers(dest="subcommand")

    create = subcommands.add_parser(
        "create",
        description=(
            "Clone the testbed project, copying in an iOS Python framework and"
            "any specified application code."
        ),
        help="Create a new testbed project"
    )
    create.add_argument(
        "--framework",
        required=True,
        help=(
            "The location of the XCFramework (or simulator-only slice of an XCFramework) "
            "to use when running the testbed"
        )
    )
    create.add_argument(
        "--app",
        dest="apps",
        action="append",
        default=[],
        help="The location of any code to include in the testbed project",
    )
    create.add_argument(
        "location",
        help="The path where the testbed will be created."
    )

    run = subcommands.add_parser(
        "run",
        usage='%(prog)s [-h] [--simulator SIMULATOR] -- <test arg> [<test arg> ...]',
        description=(
            "Run a testbed project. The arguments provided after `--` will be passed to "
            "the running iOS process as if they were arguments to `python -m`."
        ),
        help="Run a testbed project",
    )
    run.add_argument(
        "--simulator",
        default="iPhone SE (3rd Generation)",
        help="The name of the simulator to use (default: 'iPhone SE (3rd Generation)')",
    )

    try:
        pos = sys.argv.index("--")
        testbed_args = sys.argv[1:pos]
        test_args = sys.argv[pos+1:]
    except ValueError:
        testbed_args = sys.argv[1:]
        test_args = []

    context = parser.parse_args(testbed_args)

    if context.subcommand == "create":
        create_testbed(
            location=Path(context.location),
            framework=Path(context.framework),
            apps=[Path(app) for app in context.apps],
        )
    elif context.subcommand == "run":
        if test_args:
            asyncio.run(
                run_testbed(
                    simulator=context.simulator,
                    args=test_args
                )
            )
        else:
            print(f"Must specify test arguments (e.g., {sys.argv[0]} run -- test)")
            print()
            parser.print_help(sys.stderr)
            sys.exit(2)
    else:
        parser.print_help(sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
