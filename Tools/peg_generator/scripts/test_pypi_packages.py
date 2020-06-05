#!/usr/bin/env python3.8

import argparse
import os
import glob
import tarfile
import zipfile
import shutil
import pathlib
import sys

from typing import Generator, Any

sys.path.insert(0, ".")

from pegen import build
from scripts import test_parse_directory

HERE = pathlib.Path(__file__).resolve().parent

argparser = argparse.ArgumentParser(
    prog="test_pypi_packages", description="Helper program to test parsing PyPI packages",
)
argparser.add_argument(
    "-t", "--tree", action="count", help="Compare parse tree to official AST", default=0
)


def get_packages() -> Generator[str, None, None]:
    all_packages = (
        glob.glob("./data/pypi/*.tar.gz")
        + glob.glob("./data/pypi/*.zip")
        + glob.glob("./data/pypi/*.tgz")
    )
    for package in all_packages:
        yield package


def extract_files(filename: str) -> None:
    savedir = os.path.join("data", "pypi")
    if tarfile.is_tarfile(filename):
        tarfile.open(filename).extractall(savedir)
    elif zipfile.is_zipfile(filename):
        zipfile.ZipFile(filename).extractall(savedir)
    else:
        raise ValueError(f"Could not identify type of compressed file {filename}")


def find_dirname(package_name: str) -> str:
    for name in os.listdir(os.path.join("data", "pypi")):
        full_path = os.path.join("data", "pypi", name)
        if os.path.isdir(full_path) and name in package_name:
            return full_path
    assert False  # This is to fix mypy, should never be reached


def run_tests(dirname: str, tree: int) -> int:
    return test_parse_directory.parse_directory(
        dirname,
        HERE / ".." / ".." / ".." / "Grammar" / "python.gram",
        HERE / ".." / ".." / ".." / "Grammar" / "Tokens",
        verbose=False,
        excluded_files=[
            "*/failset/*",
            "*/failset/**",
            "*/failset/**/*",
            "*/test2to3/*",
            "*/test2to3/**/*",
            "*/bad*",
            "*/lib2to3/tests/data/*",
        ],
        skip_actions=False,
        tree_arg=tree,
        short=True,
        mode=1,
        parser="pegen",
    )


def main() -> None:
    args = argparser.parse_args()
    tree = args.tree

    for package in get_packages():
        print(f"Extracting files from {package}... ", end="")
        try:
            extract_files(package)
            print("Done")
        except ValueError as e:
            print(e)
            continue

        print(f"Trying to parse all python files ... ")
        dirname = find_dirname(package)
        status = run_tests(dirname, tree)
        if status == 0:
            shutil.rmtree(dirname)
        else:
            print(f"Failed to parse {dirname}")


if __name__ == "__main__":
    main()
