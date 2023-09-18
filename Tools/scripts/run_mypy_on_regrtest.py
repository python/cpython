"""
Script to run mypy on Lib/test/libregrtest.

This script is necessary due to the fact that,
if you invoke mypy directly on anything inside the Lib/ directory,
it (amusingly) thinks that everything in the stdlib is being "shadowed"
by the modules inside `Lib/`.
"""

import argparse
import os
import shutil
import subprocess
import tempfile
from pathlib import Path
from typing import TypeAlias

ReturnCode: TypeAlias = int


def run_mypy_on_libregrtest(root_dir: Path) -> ReturnCode:
    test_dot_support_dir = root_dir / "Lib/" / "test" / "support"
    # Copy `Lib/test/support/` into a tempdir and point MYPYPATH towards the tempdir,
    # so that mypy can see the classes and functions defined in `Lib/test/support/`
    with tempfile.TemporaryDirectory() as td:
        td_path = Path(td)
        (td_path / "test").mkdir()
        shutil.copytree(test_dot_support_dir, td_path / "test" / "support")
        mypy_command = [
            "mypy",
            "--config-file",
            "Lib/test/libregrtest/mypy.ini",
        ]
        result = subprocess.run(
            mypy_command, cwd=root_dir, env=os.environ | {"MYPYPATH": td}
        )
        return result.returncode


def main() -> ReturnCode:
    parser = argparse.ArgumentParser("Script to run mypy on Lib/test/regrtest/")
    parser.add_argument(
        "--root-dir",
        "-r",
        type=Path,
        required=True,
        help="path to the CPython repo root",
    )
    args = parser.parse_args()
    root_dir = args.root_dir
    test_dot_support_dir = root_dir / "Lib" / "test" / "support"
    if not (test_dot_support_dir.exists() and test_dot_support_dir.is_dir()):
        parser.error("--root-dir must point to the root of a CPython clone!")
    return run_mypy_on_libregrtest(root_dir)


if __name__ == "__main__":
    raise SystemExit(main())
