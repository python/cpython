"""Compares checksums for wheels in :mod:`ensurepip` against the Cheeseshop."""

import hashlib
import json
from pathlib import Path
import re
from urllib.request import urlopen

PACKAGE_NAMES = ("pip", "setuptools")
ROOT = Path(__file__).parent.parent.parent / "Lib/ensurepip"
WHEEL_DIR = ROOT / "_bundled"
ENSURE_PIP_INIT_PY_TEXT = (ROOT / "__init__.py").read_text(encoding="utf-8")

# Contains names of successfully verified packages
verified_packages = {*()}


def error(file, message):
    print(f"::error file={file}::{message}\n")


for package_name in PACKAGE_NAMES:
    # Find the package on disk
    package_path = next(WHEEL_DIR.glob(f"{package_name}*.whl"), None)
    if package_path is None:
        continue

    print(f"Verifying checksum for {package_path}.")

    # Find the version of the package used by ensurepip
    package_version_match = re.search(
        f'_{package_name.upper()}_VERSION = "([^"]+)',
        ENSURE_PIP_INIT_PY_TEXT
    )
    if package_version_match is None:
        error(package_path, f"No {package_name} version found in Lib/ensurepip/__init__.py.")
        continue
    package_version = package_version_match[1]

    # Get the SHA 256 digest from the Cheeseshop
    try:
        raw_text = urlopen(f"https://pypi.org/pypi/{package_name}/json").read()
    except (OSError, ValueError) as error:
        error(package_path, f"Could not fetch JSON metadata for {package_name}.")
        continue

    expected_digest = ""
    release_files = json.loads(raw_text)["releases"][package_version]
    for release_info in release_files:
        if package_path.name != release_info["filename"]:
            continue
        expected_digest = release_info["digests"].get("sha256", "")
    if expected_digest == "":
        error(package_path, f"No digest for {package_name} found from PyPI.")
        continue

    # Compute the SHA 256 digest of the wheel on disk
    actual_digest = hashlib.sha256(package_path.read_bytes()).hexdigest()

    print(f"Expected digest: {expected_digest}")
    print(f"Actual digest:   {actual_digest}")

    # The messages are formatted to be parsed by GitHub Actions.
    # https://docs.github.com/en/actions/using-workflows/workflow-commands-for-github-actions#setting-a-notice-message
    if actual_digest == expected_digest:
        print(f"::notice file={package_path}::"
              f"Successfully verified the checksum of the {package_name} wheel.\n")
        verified_packages.add(package_name)
    else:
        error(package_path, f"Failed to verify the checksum of the {package_name} wheel.")

# If we verified all packages, the set of package names and the set of verified
# packages will be equal.
if {*PACKAGE_NAMES} == verified_packages:
    raise SystemExit(0)

# Otherwise we failed to verify all the packages.
raise SystemExit(1)
