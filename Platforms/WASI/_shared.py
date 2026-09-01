__lazy_modules__ = [
    "functools",
    "json",
    "os",
    "pathlib",
    "subprocess",
    "sysconfig",
    "tempfile",
    "tomllib",
]

import functools
import json
import os
import pathlib
import subprocess
import sysconfig
import tempfile
import tomllib


class Context:
    clean = False

    def __init__(self):
        self.here = pathlib.Path(__file__).parent

    @functools.cached_property
    def checkout(self):
        checkout = self.here
        while checkout != checkout.parent:
            if (checkout / "configure").is_file():
                return checkout
            checkout = checkout.parent
        raise FileNotFoundError(
            "Unable to find the root of the CPython checkout by looking for 'configure'"
        )

    def _pybuilddir(self, build_path):
        relative_dir = (build_path / "pybuilddir.txt").read_text().strip()
        return build_path / relative_dir

    @functools.cached_property
    def setup_local_path(self):
        return self.checkout / "Modules" / "Setup.local"

    @functools.cached_property
    def host_triple(self):
        if self._host_triple:
            return self._host_triple

        with (self.here / "config.toml").open("rb") as file:
            config = tomllib.load(file)

        return config["targets"]["host-triple"]

    @functools.cached_property
    def cross_build_path(self):
        return self.checkout / "cross-build"

    @functools.cached_property
    def build_python_path(self):
        # Build platform can also be found via `config.guess`.
        return self.cross_build_path / sysconfig.get_config_var(
            "BUILD_GNU_TYPE"
        )

    @functools.cached_property
    def build_python_interpreter(self):
        binary = self.build_python_path / "python"
        if not binary.is_file():
            binary = binary.with_suffix(".exe")
            if not binary.is_file():
                raise FileNotFoundError(
                    f"Unable to find `python(.exe)` in {self.build_python_path}"
                )

        return binary

    @functools.cached_property
    def is_debug(self):
        pybuilddir = self._pybuilddir(self.build_python_path)
        if pybuilddir.exists():
            build_details_path = pybuilddir / "build-details.json"
            build_details = json.loads(build_details_path.read_text())
            return "d" in build_details["abi"]["flags"]
        else:
            # Python 3.13 and older.
            test = "import sys, test.support; sys.exit(test.support.Py_DEBUG)"
            result = subprocess.run(
                [self.build_python_interpreter, "-c", test],
                capture_output=True,
            )
            return bool(result.returncode)

    @functools.cached_property
    def wasi_build_path(self):
        return self.cross_build_path / self.host_triple

    @functools.cached_property
    def wasi_pybuilddir(self):
        return self._pybuilddir(self.wasi_build_path)

    @functools.cached_property
    def wasi_build_details(self):
        with (self.wasi_pybuilddir / "build-details.json").open() as f:
            return json.load(f)

    @functools.cached_property
    def wasi_build_version(self):
        return self.wasi_build_details["language"]["version_info"]

    @functools.cached_property
    def wasi_sdk_path(self):
        if wasi_sdk_path := self._wasi_sdk_path:
            if not wasi_sdk_path.exists():
                raise ValueError(
                    "WASI SDK not found; "
                    "download from "
                    "https://github.com/WebAssembly/wasi-sdk and/or "
                    "specify via $WASI_SDK_PATH or --wasi-sdk"
                )
            return wasi_sdk_path

        with (self.here / "config.toml").open("rb") as file:
            config = tomllib.load(file)
        wasi_sdk_version = config["targets"]["wasi-sdk"]

        if wasi_sdk_path_env_var := os.environ.get("WASI_SDK_PATH"):
            wasi_sdk_path = pathlib.Path(wasi_sdk_path_env_var)
            if not wasi_sdk_path.exists():
                raise ValueError(
                    f"WASI SDK not found at $WASI_SDK_PATH ({wasi_sdk_path})"
                )
        else:
            opt_path = pathlib.Path("/opt")
            # WASI SDK versions have a ``.0`` suffix, but it's a constant; the WASI SDK team
            # has said they don't plan to ever do a point release and all of their Git tags
            # lack the ``.0`` suffix.
            # Starting with WASI SDK 23, the tarballs went from containing a directory named
            # ``wasi-sdk-{WASI_SDK_VERSION}.0`` to e.g.
            # ``wasi-sdk-{WASI_SDK_VERSION}.0-x86_64-linux``.
            potential_sdks = [
                path
                for path in opt_path.glob(f"wasi-sdk-{wasi_sdk_version}.0*")
                if path.is_dir()
            ]
            if not potential_sdks:
                raise ValueError(
                    f"WASI SDK {wasi_sdk_version} not found in {opt_path}"
                )
            elif len(potential_sdks) == 1:
                wasi_sdk_path = potential_sdks[0]
            elif (default_path := opt_path / "wasi-sdk").is_dir():
                wasi_sdk_path = default_path
            elif potential_sdks:
                raise ValueError(
                    f"Multiple WASI SDKs found in {opt_path} w/o knowing which to use"
                )
            else:
                raise ValueError(f"WASI SDK not found in {opt_path}")

        # Starting with WASI SDK 25, a VERSION file is included in the root
        # of the SDK directory that we can read to warn folks when they are using
        # an unsupported version.
        if (
            wasi_sdk_path
            and (version_file := wasi_sdk_path / "VERSION").is_file()
        ):
            version_details = version_file.read_text(encoding="utf-8")
            found_version = version_details.splitlines()[0]
            # Make sure there's a trailing dot to avoid false positives if somehow the
            # supported version is a prefix of the found version (e.g. `25` and `2567`).
            if not found_version.startswith(f"{wasi_sdk_version}."):
                major_version = found_version.partition(".")[0]
                log(
                    "⚠️",
                    f" Found WASI SDK {major_version}, "
                    f"but WASI SDK {wasi_sdk_version} is the supported version",
                )

        return wasi_sdk_path

    @functools.cached_property
    def log_path(self):
        if self._log_path is not None:
            return self._log_path

        return pathlib.Path(tempfile.gettempdir())


def log(emoji, message, *, spacing=None):
    """Print a notification with an emoji.

    If 'spacing' is None, calculate the spacing based on the number of code points
    in the emoji as terminals "eat" a space when the emoji has multiple code points.
    """
    if spacing is None:
        spacing = " " if len(emoji) == 1 else "  "
    print("".join([emoji, spacing, message]))
