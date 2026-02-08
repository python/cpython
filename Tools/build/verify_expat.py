#! /usr/bin/env python3
"""
Verify that bundled libexpat files come from a verified release.
"""

from __future__ import annotations

import json
import os
import re
import tarfile
from hashlib import sha3_256
from pathlib import Path
from typing import Literal
from urllib.request import Request, urlopen

GITHUB_ACTIONS = os.getenv("GITHUB_ACTIONS") == "true"

EXPAT_REL_PATH = "Modules/expat/"
EXPAT_PATH = Path(__file__).parent.parent.parent / EXPAT_REL_PATH


def log(
    level: Literal["debug", "notice", "error"], file_path: str | None, message: str
) -> None:
    if GITHUB_ACTIONS:
        # https://docs.github.com/en/actions/using-workflows/workflow-commands-for-github-actions
        if file_path:
            message = f"::{level} file={file_path}::{message}"
        else:
            message = f"::{level}::{message}"
    print(message)


def verify_expat() -> bool:
    has_failed = False

    with open(EXPAT_PATH / "expat.h") as headers_file:
        headers = headers_file.read()
    version_pieces = re.findall(
        r"(?<=^#define XML_(?:MAJOR|MINOR|MICRO)_VERSION ).+$", headers, re.MULTILINE
    )

    expat_version = ".".join(version_pieces)
    log("debug", None, f"Verifying bundled files from libexpat {expat_version}.")

    # Offload verifying the release tag to GitHub.
    # https://docs.github.com/en/rest/git/commits?apiVersion=2022-11-28#get-a-commit
    tag_name = f"R_{'_'.join(version_pieces)}"
    tag_data_request = Request(
        f"https://api.github.com/repos/libexpat/libexpat/commits/{tag_name}",
        headers={"X-GitHub-Api-Version": "2022-11-28"},
    )
    with urlopen(tag_data_request) as tag_data_io:
        tag_data = json.load(tag_data_io)
    if tag_data["commit"]["verification"]["verified"]:
        log(
            "debug",
            None,
            f"The signature in libexpat {expat_version} is considered to be verified.",
        )
    else:
        log(
            "error",
            None,
            f"The signature in libexpat {expat_version} is not considered to be verified.",
        )
        has_failed = True

    # Download tarball from GitHub and generate hashes for files that
    # have to be bundled.
    tarball_url = (
        f"https://github.com/libexpat/libexpat/archive/refs/tags/{tag_name}.tar.gz"
    )
    expected_hashes = {}
    with urlopen(tarball_url) as tarball_io:
        with tarfile.open(fileobj=tarball_io, mode="r:gz") as tarball:
            for member in tarball:
                if re.search(
                    r"/expat/lib/\w+\.[ch]$", member.name
                ) or member.name.endswith("/COPYING"):
                    file_name = os.path.basename(member.name)
                    content = tarball.extractfile(member).read()  # type: ignore[union-attr]
                    expected_hashes[file_name] = sha3_256(content).hexdigest()

    # Compare hashes of bundled libexpat files to the actually released
    # files.
    with os.scandir(EXPAT_PATH) as expat_dir:
        for entry in expat_dir:
            with open(entry, "rb") as expat_file:
                # Skip files that are not a part of libexpat.
                if entry.name in (
                    "expat_config.h",
                    "pyexpatns.h",
                ) or entry.name.endswith((".a", ".o")):
                    continue
                # Skip a few known lines added to expat_external.h.
                elif entry.name == "expat_external.h":
                    for _ in range(4):
                        expat_file.readline()
                file_path = os.path.join(EXPAT_REL_PATH, entry.name)
                content = expat_file.read()
                if sha3_256(content).hexdigest() == expected_hashes[entry.name]:
                    log(
                        "debug",
                        file_path,
                        f"{entry.name} is the same as in libexpat {expat_version}.",
                    )
                else:
                    log(
                        "error",
                        file_path,
                        f"{entry.name} is not the same as in libexpat {expat_version}.",
                    )
                    has_failed = True
                del expected_hashes[entry.name]

    if expected_hashes:
        log("error", None, f"{expected_hashes.keys()} files were not bundled.")
        has_failed = True

    has_succeeded = not has_failed
    if has_succeeded:
        log("notice", None, "Successfully verified bundled libexpat files.")
    return has_succeeded


if __name__ == "__main__":
    raise SystemExit(0 if verify_expat() else 1)
