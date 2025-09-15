import argparse
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path


DECODE_ARGS = ("UTF-8", "backslashreplace")

# The system log prefixes each line:
#   2025-01-17 16:14:29.093742+0800 iOSTestbed[23987:1fd393b4] ...
#   2025-01-17 16:14:29.093742+0800 iOSTestbed[23987:1fd393b4] ...

LOG_PREFIX_REGEX = re.compile(
    r"^\d{4}-\d{2}-\d{2}"  # YYYY-MM-DD
    r"\s+\d+:\d{2}:\d{2}\.\d+\+\d{4}"  # HH:MM:SS.ssssss+ZZZZ
    r"\s+iOSTestbed\[\d+:\w+\]"  # Process/thread ID
)


# Select a simulator device to use.
def select_simulator_device():
    # List the testing simulators, in JSON format
    raw_json = subprocess.check_output(["xcrun", "simctl", "list", "-j"])
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
            (
                "iPhone " in devicetype["name"]
                and devicetype["name"].endswith("e")
            )
            or "iPhone SE " in devicetype["name"]
        )
    )

    return se_simulators[-1][1]


def xcode_test(location, simulator, verbose):
    # Build and run the test suite on the named simulator.
    args = [
        "-project",
        str(location / "iOSTestbed.xcodeproj"),
        "-scheme",
        "iOSTestbed",
        "-destination",
        f"platform=iOS Simulator,name={simulator}",
        "-derivedDataPath",
        str(location / "DerivedData"),
    ]
    verbosity_args = [] if verbose else ["-quiet"]

    print("Building test project...")
    subprocess.run(
        ["xcodebuild", "build-for-testing"] + args + verbosity_args,
        check=True,
    )

    print("Running test project...")
    # Test execution *can't* be run -quiet; verbose mode
    # is how we see the output of the test output.
    process = subprocess.Popen(
        ["xcodebuild", "test-without-building"] + args,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    while line := (process.stdout.readline()).decode(*DECODE_ARGS):
        # Strip the timestamp/process prefix from each log line
        line = LOG_PREFIX_REGEX.sub("", line)
        sys.stdout.write(line)
        sys.stdout.flush()

    status = process.wait(timeout=5)
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
    print(f"  Cloning {source}...", end="")
    shutil.copytree(source, target, symlinks=True)
    print(" done")

    xc_framework_path = target / "Python.xcframework"
    sim_framework_path = xc_framework_path / "ios-arm64_x86_64-simulator"
    if framework is not None:
        if framework.suffix == ".xcframework":
            print("  Installing XCFramework...", end="")
            if xc_framework_path.is_dir():
                shutil.rmtree(xc_framework_path)
            else:
                xc_framework_path.unlink(missing_ok=True)
            xc_framework_path.symlink_to(
                framework.relative_to(xc_framework_path.parent, walk_up=True)
            )
            print(" done")
        else:
            print("  Installing simulator framework...", end="")
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
            print("  Rewriting symlink to XCframework...", end="")
            orig_xc_framework_path = (
                source / xc_framework_path.readlink()
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
            print("  Rewriting symlink to simulator framework...", end="")
            # Simulator framework is a relative symlink. Rewrite the symlink
            # relative to the new location.
            orig_sim_framework_path = (
                source / "Python.XCframework" / sim_framework_path.readlink()
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
        print(f"  Installing app {app_src.name!r}...", end="")
        app_target = target / f"iOSTestbed/app/{app_src.name}"
        if app_target.is_dir():
            shutil.rmtree(app_target)
        shutil.copytree(app_src, app_target)
        print(" done")

    print(f"Successfully cloned testbed: {target.resolve()}")


def update_test_plan(testbed_path, args):
    # Modify the test plan to use the requested test arguments.
    test_plan_path = testbed_path / "iOSTestbed.xctestplan"
    with test_plan_path.open("r", encoding="utf-8") as f:
        test_plan = json.load(f)

    test_plan["defaultOptions"]["commandLineArgumentEntries"] = [
        {"argument": arg} for arg in args
    ]

    with test_plan_path.open("w", encoding="utf-8") as f:
        json.dump(test_plan, f, indent=2)


def run_testbed(simulator: str | None, args: list[str], verbose: bool = False):
    location = Path(__file__).parent
    print("Updating test plan...", end="")
    update_test_plan(location, args)
    print(" done.")

    if simulator is None:
        simulator = select_simulator_device()
    print(f"Running test on {simulator}")

    xcode_test(location, simulator=simulator, verbose=verbose)


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
            "The name of the simulator to use (eg: 'iPhone 16e'). Defaults to "
            "the most recently released 'entry level' iPhone device. Device "
            "architecture and OS version can also be specified; e.g., "
            "`--simulator 'iPhone 16 Pro,arch=arm64,OS=26.0'` would run on "
            "an ARM64 iPhone 16 Pro simulator running iOS 26.0."
        ),
    )
    run.add_argument(
        "-v",
        "--verbose",
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
            framework=Path(context.framework).resolve()
            if context.framework
            else None,
            apps=[Path(app) for app in context.apps],
        )
    elif context.subcommand == "run":
        if test_args:
            if not (
                Path(__file__).parent
                / "Python.xcframework/ios-arm64_x86_64-simulator/bin"
            ).is_dir():
                print(
                    f"Testbed does not contain a compiled iOS framework. Use "
                    f"`python {sys.argv[0]} clone ...` to create a runnable "
                    f"clone of this testbed."
                )
                sys.exit(20)

            run_testbed(
                simulator=context.simulator,
                verbose=context.verbose,
                args=test_args,
            )
        else:
            print(
                f"Must specify test arguments (e.g., {sys.argv[0]} run -- test)"
            )
            print()
            parser.print_help(sys.stderr)
            sys.exit(21)
    else:
        parser.print_help(sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
