#!/usr/bin/env python3

__lazy_modules__ = ["_build"]

import argparse
import os
import pathlib

import _build

HERE = pathlib.Path(__file__).parent


def main():
    default_host_runner = (
        "{WASMTIME} run "
        # Set argv0 so that getpath.py can auto-discover the sysconfig data directory.
        "--argv0 {ARGV0} "
        # Map the checkout to / to load the stdlib from /Lib.
        "--dir {CHECKOUT}::/ "
        # Flags involving --optimize, --codegen, --debug, --wasm, and --wasi can be kept
        # in a config file.
        # We are using such a file to act as defaults in case a user wants to override
        # only some of the settings themselves, make it easy to modify settings
        # post-build so that they immediately apply to the Makefile instead of having to
        # regenerate it, and allow for easy copying of the settings for anyone else who
        # may want to use them.
        "--config {WASMTIME_CONFIG_PATH}"
    )

    parser = argparse.ArgumentParser()
    subcommands = parser.add_subparsers(dest="subcommand")
    build = subcommands.add_parser("build", help="Build everything")
    configure_build = subcommands.add_parser(
        "configure-build-python", help="Run `configure` for the build Python"
    )
    make_build = subcommands.add_parser(
        "make-build-python", help="Run `make` for the build Python"
    )
    build_python = subcommands.add_parser(
        "build-python", help="Build the build Python"
    )
    configure_host = subcommands.add_parser(
        "configure-host",
        help="Run `configure` for the "
        "host/WASI (pydebug builds "
        "are inferred from the build "
        "Python)",
    )
    make_host = subcommands.add_parser(
        "make-host", help="Run `make` for the host/WASI"
    )
    build_host = subcommands.add_parser(
        "build-host", help="Build the host/WASI Python"
    )
    subcommands.add_parser(
        "clean", help="Delete files and directories created by this script"
    )
    for subcommand in (
        build,
        configure_build,
        make_build,
        build_python,
        configure_host,
        make_host,
        build_host,
    ):
        subcommand.add_argument(
            "--quiet",
            action="store_true",
            default=False,
            dest="quiet",
            help="Redirect output from subprocesses to a log file",
        )
        subcommand.add_argument(
            "--logdir",
            type=pathlib.Path,
            default=None,
            help="Directory to store log files",
        )
    for subcommand in (
        configure_build,
        configure_host,
        build_python,
        build_host,
    ):
        subcommand.add_argument(
            "--clean",
            action="store_true",
            default=False,
            dest="clean",
            help="Delete any relevant directories before building",
        )
    for subcommand in (
        build,
        configure_build,
        configure_host,
        build_python,
        build_host,
    ):
        subcommand.add_argument(
            "args", nargs="*", help="Extra arguments to pass to `configure`"
        )
    for subcommand in build, configure_host, build_host:
        subcommand.add_argument(
            "--wasi-sdk",
            type=pathlib.Path,
            dest="wasi_sdk_path",
            default=None,
            help="Path to the WASI SDK; defaults to WASI_SDK_PATH environment variable "
            "or the appropriate version found in /opt",
        )
        subcommand.add_argument(
            "--host-runner",
            action="store",
            default=default_host_runner,
            dest="host_runner",
            help="Command template for running the WASI host; defaults to "
            f"`{default_host_runner}`",
        )
    for subcommand in build, configure_host, make_host, build_host:
        subcommand.add_argument(
            "--host-triple",
            action="store",
            default=None,
            help="The target triple for the WASI host build; "
            f"defaults to the value found in {os.fsdecode(HERE / 'config.toml')}",
        )

    context = parser.parse_args()

    match context.subcommand:
        case "configure-build-python":
            _build.configure_build_python(context)
        case "make-build-python":
            _build.make_build_python(context)
        case "build-python":
            _build.configure_build_python(context)
            _build.make_build_python(context)
        case "configure-host":
            _build.configure_wasi_python(context)
        case "make-host":
            _build.make_wasi_python(context)
        case "build-host":
            _build.configure_wasi_python(context)
            _build.make_wasi_python(context)
        case "build":
            _build.configure_build_python(context)
            _build.make_build_python(context)
            _build.configure_wasi_python(context)
            _build.make_wasi_python(context)
        case "clean":
            _build.clean_contents(context)
        case _:
            raise ValueError(f"Unknown subcommand {context.subcommand!r}")


if __name__ == "__main__":
    main()
