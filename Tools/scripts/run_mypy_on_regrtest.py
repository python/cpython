"""
Script to run mypy on Lib/test/libregrtest.

This script is necessary due to the fact that,
if you invoke mypy directly on anything inside the Lib/ directory,
it (amusingly) thinks that everything in the stdlib is being "shadowed"
by the modules inside `Lib/`.
"""

import argparse
import os
import tempfile
import shutil
import subprocess
from pathlib import Path
from typing import TypeAlias

ReturnCode: TypeAlias = int


def run_mypy_on_libregrtest(stdlib_dir: Path) -> ReturnCode:
    stdlib_test_dir = stdlib_dir / "test"
    # Copy `Lib/test/support/` into a tempdir and point MYPYPATH towards the tempdir,
    # so that mypy can see the classes and functions defined in `Lib/test/support/`
    with tempfile.TemporaryDirectory() as td:
        td_path = Path(td)
        (td_path / "test").mkdir()
        shutil.copytree(stdlib_test_dir / "support", td_path / "test" / "support")
        mypy_command = [
            "mypy",
            "--config-file",
            "libregrtest/mypy.ini",
        ]
        result = subprocess.run(
            mypy_command, cwd=stdlib_test_dir, env=os.environ | {"MYPYPATH": td}
        )
        return result.returncode


def main() -> ReturnCode:
    parser = argparse.ArgumentParser("Script to run mypy on Lib/test/regrtest/")
    parser.add_argument(
        "--stdlib-dir",
        "-s",
        type=Path,
        required=True,
        help="path to the Lib/ dir where the Python stdlib is located",
    )
    args = parser.parse_args()
    stdlib_dir = args.stdlib_dir
    if not (stdlib_dir.exists() and stdlib_dir.is_dir()):
        parser.error(
            "--stdlib-dir must point to a directory that exists on your filesystem!"
        )
    return run_mypy_on_libregrtest(args.stdlib_dir)


if __name__ == "__main__":
    raise SystemExit(main())
