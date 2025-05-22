"""Tool for generating Software Bill of Materials (SBOM) for Python's dependencies"""

import glob
import hashlib
import json
import os
import re
import subprocess
import sys
import time
import typing
import urllib.error
import urllib.request
from pathlib import Path, PurePosixPath, PureWindowsPath

CPYTHON_ROOT_DIR = Path(__file__).parent.parent.parent

# Before adding a new entry to this list, double check that
# the license expression is a valid SPDX license expression:
# See: https://spdx.org/licenses
ALLOWED_LICENSE_EXPRESSIONS = {
    "Apache-2.0",
    "Apache-2.0 OR BSD-2-Clause",
    "BSD-2-Clause",
    "BSD-3-Clause",
    "CC0-1.0",
    "ISC",
    "LGPL-2.1-only",
    "MIT",
    "MPL-2.0",
    "Python-2.0.1",
}

# Properties which are required for our purposes.
REQUIRED_PROPERTIES_PACKAGE = frozenset([
    "SPDXID",
    "name",
    "versionInfo",
    "downloadLocation",
    "checksums",
    "licenseConcluded",
    "externalRefs",
    "primaryPackagePurpose",
])


class PackageFiles(typing.NamedTuple):
    """Structure for describing the files of a package"""
    include: list[str] | None
    exclude: list[str] | None = None


# SBOMS don't have a method to specify the sources of files
# so we need to do that external to the SBOM itself. Add new
# values to 'exclude' if we create new files within tracked
# directories that aren't sourced from third-party packages.
PACKAGE_TO_FILES = {
    "mpdecimal": PackageFiles(
        include=["Modules/_decimal/libmpdec/**"]
    ),
    "expat": PackageFiles(
        include=["Modules/expat/**"],
        exclude=[
            "Modules/expat/expat_config.h",
            "Modules/expat/pyexpatns.h",
            "Modules/expat/refresh.sh",
        ]
    ),
    "macholib": PackageFiles(
        include=["Lib/ctypes/macholib/**"],
        exclude=[
            "Lib/ctypes/macholib/README.ctypes",
            "Lib/ctypes/macholib/fetch_macholib",
            "Lib/ctypes/macholib/fetch_macholib.bat",
        ],
    ),
    "hacl-star": PackageFiles(
        include=["Modules/_hacl/**"],
        exclude=[
            "Modules/_hacl/refresh.sh",
            "Modules/_hacl/README.md",
            "Modules/_hacl/python_hacl_namespace.h",
        ]
    ),
}


def spdx_id(value: str) -> str:
    """Encode a value into characters that are valid in an SPDX ID"""
    return re.sub(r"[^a-zA-Z0-9.\-]+", "-", value)


def error_if(value: bool, error_message: str) -> None:
    """Prints an error if a comparison fails along with a link to the devguide"""
    if value:
        print(error_message)
        print("See 'https://devguide.python.org/developer-workflow/sbom' for more information.")
        sys.exit(1)


def is_root_directory_git_index() -> bool:
    """Checks if the root directory is a git index"""
    try:
        subprocess.check_call(
            ["git", "-C", str(CPYTHON_ROOT_DIR), "rev-parse"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
    except subprocess.CalledProcessError:
        return False
    return True


def filter_gitignored_paths(paths: list[str]) -> list[str]:
    """
    Filter out paths excluded by the gitignore file.
    The output of 'git check-ignore --non-matching --verbose' looks
    like this for non-matching (included) files:

        '::<whitespace><path>'

    And looks like this for matching (excluded) files:

        '.gitignore:9:*.a    Tools/lib.a'
    """
    # No paths means no filtering to be done.
    if not paths:
        return []

    # Filter out files in gitignore.
    # Non-matching files show up as '::<whitespace><path>'
    git_check_ignore_proc = subprocess.run(
        ["git", "check-ignore", "--verbose", "--non-matching", *paths],
        cwd=CPYTHON_ROOT_DIR,
        check=False,
        stdout=subprocess.PIPE,
    )
    # 1 means matches, 0 means no matches.
    assert git_check_ignore_proc.returncode in (0, 1)

    # Paths may or may not be quoted, Windows quotes paths.
    git_check_ignore_re = re.compile(r"^::\s+(\"([^\"]+)\"|(.+))\Z")

    # Return the list of paths sorted
    git_check_ignore_lines = git_check_ignore_proc.stdout.decode().splitlines()
    git_check_not_ignored = []
    for line in git_check_ignore_lines:
        if match := git_check_ignore_re.fullmatch(line):
            git_check_not_ignored.append(match.group(2) or match.group(3))
    return sorted(git_check_not_ignored)


def get_externals() -> list[str]:
    """
    Parses 'PCbuild/get_externals.bat' for external libraries.
    Returns a list of (git tag, name, version) tuples.
    """
    get_externals_bat_path = CPYTHON_ROOT_DIR / "PCbuild/get_externals.bat"
    externals = re.findall(
        r"set\s+libraries\s*=\s*%libraries%\s+([a-zA-Z0-9.-]+)\s",
        get_externals_bat_path.read_text()
    )
    return externals


def download_with_retries(download_location: str,
                          max_retries: int = 5,
                          base_delay: float = 2.0) -> typing.Any:
    """Download a file with exponential backoff retry."""
    for attempt in range(max_retries):
        try:
            resp = urllib.request.urlopen(download_location)
        except urllib.error.URLError as ex:
            if attempt == max_retries:
                raise ex
            time.sleep(base_delay**attempt)
        else:
            return resp


def check_sbom_packages(sbom_data: dict[str, typing.Any]) -> None:
    """Make a bunch of assertions about the SBOM package data to ensure it's consistent."""

    for package in sbom_data["packages"]:
        # Properties and ID must be properly formed.
        error_if(
            "name" not in package,
            "Package is missing the 'name' field"
        )

        # Verify that the checksum matches the expected value
        # and that the download URL is valid.
        if "checksums" not in package or "CI" in os.environ:
            download_location = package["downloadLocation"]
            resp = download_with_retries(download_location)
            error_if(resp.status != 200, f"Couldn't access URL: {download_location}'")

            package["checksums"] = [{
                "algorithm": "SHA256",
                "checksumValue": hashlib.sha256(resp.read()).hexdigest()
            }]

        missing_required_keys = REQUIRED_PROPERTIES_PACKAGE - set(package.keys())
        error_if(
            bool(missing_required_keys),
            f"Package '{package['name']}' is missing required fields: {missing_required_keys}",
        )
        error_if(
            package["SPDXID"] != spdx_id(f"SPDXRef-PACKAGE-{package['name']}"),
            f"Package '{package['name']}' has a malformed SPDXID",
        )

        # Version must be in the download and external references.
        version = package["versionInfo"]
        error_if(
            version not in package["downloadLocation"],
            f"Version '{version}' for package '{package['name']} not in 'downloadLocation' field",
        )
        error_if(
            any(version not in ref["referenceLocator"] for ref in package["externalRefs"]),
            (
                f"Version '{version}' for package '{package['name']} not in "
                f"all 'externalRefs[].referenceLocator' fields"
            ),
        )

        # HACL* specifies its expected rev in a refresh script.
        if package["name"] == "hacl-star":
            hacl_refresh_sh = (CPYTHON_ROOT_DIR / "Modules/_hacl/refresh.sh").read_text()
            hacl_expected_rev_match = re.search(
                r"expected_hacl_star_rev=([0-9a-f]{40})",
                hacl_refresh_sh
            )
            hacl_expected_rev = hacl_expected_rev_match and hacl_expected_rev_match.group(1)

            error_if(
                hacl_expected_rev != version,
                "HACL* SBOM version doesn't match value in 'Modules/_hacl/refresh.sh'"
            )

        # libexpat specifies its expected rev in a refresh script.
        if package["name"] == "libexpat":
            libexpat_refresh_sh = (CPYTHON_ROOT_DIR / "Modules/expat/refresh.sh").read_text()
            libexpat_expected_version_match = re.search(
                r"expected_libexpat_version=\"([0-9]+\.[0-9]+\.[0-9]+)\"",
                libexpat_refresh_sh
            )
            libexpat_expected_sha256_match = re.search(
                r"expected_libexpat_sha256=\"[a-f0-9]{40}\"",
                libexpat_refresh_sh
            )
            libexpat_expected_version = libexpat_expected_version_match and libexpat_expected_version_match.group(1)
            libexpat_expected_sha256 = libexpat_expected_sha256_match and libexpat_expected_sha256_match.group(1)

            error_if(
                libexpat_expected_version != version,
                "libexpat SBOM version doesn't match value in 'Modules/expat/refresh.sh'"
            )
            error_if(
                package["checksums"] != [{
                    "algorithm": "SHA256",
                    "checksumValue": libexpat_expected_sha256
                }],
                "libexpat SBOM checksum doesn't match value in 'Modules/expat/refresh.sh'"
            )

        # License must be on the approved list for SPDX.
        license_concluded = package["licenseConcluded"]
        error_if(
            license_concluded != "NOASSERTION",
            "License identifier must be 'NOASSERTION'"
        )


def create_source_sbom() -> None:
    sbom_path = CPYTHON_ROOT_DIR / "Misc/sbom.spdx.json"
    sbom_data = json.loads(sbom_path.read_bytes())

    # We regenerate all of this information. Package information
    # should be preserved though since that is edited by humans.
    sbom_data["files"] = []
    sbom_data["relationships"] = []

    # Ensure all packages in this tool are represented also in the SBOM file.
    actual_names = {package["name"] for package in sbom_data["packages"]}
    expected_names = set(PACKAGE_TO_FILES)
    error_if(
        actual_names != expected_names,
        f"Packages defined in SBOM tool don't match those defined in SBOM file: {actual_names}, {expected_names}",
    )

    check_sbom_packages(sbom_data)

    # We call 'sorted()' here a lot to avoid filesystem scan order issues.
    for name, files in sorted(PACKAGE_TO_FILES.items()):
        package_spdx_id = spdx_id(f"SPDXRef-PACKAGE-{name}")
        exclude = files.exclude or ()
        for include in sorted(files.include or ()):
            # Find all the paths and then filter them through .gitignore.
            paths = glob.glob(include, root_dir=CPYTHON_ROOT_DIR, recursive=True)
            paths = filter_gitignored_paths(paths)
            error_if(
                len(paths) == 0,
                f"No valid paths found at path '{include}' for package '{name}",
            )

            for path in paths:

                # Normalize the filename from any combination of slashes.
                path = str(PurePosixPath(PureWindowsPath(path)))

                # Skip directories and excluded files
                if not (CPYTHON_ROOT_DIR / path).is_file() or path in exclude:
                    continue

                # SPDX requires SHA1 to be used for files, but we provide SHA256 too.
                data = (CPYTHON_ROOT_DIR / path).read_bytes()
                # We normalize line-endings for consistent checksums.
                # This is a rudimentary check for binary files.
                if b"\x00" not in data:
                    data = data.replace(b"\r\n", b"\n")
                checksum_sha1 = hashlib.sha1(data).hexdigest()
                checksum_sha256 = hashlib.sha256(data).hexdigest()

                file_spdx_id = spdx_id(f"SPDXRef-FILE-{path}")
                sbom_data["files"].append({
                    "SPDXID": file_spdx_id,
                    "fileName": path,
                    "checksums": [
                        {"algorithm": "SHA1", "checksumValue": checksum_sha1},
                        {"algorithm": "SHA256", "checksumValue": checksum_sha256},
                    ],
                })

                # Tie each file back to its respective package.
                sbom_data["relationships"].append({
                    "spdxElementId": package_spdx_id,
                    "relatedSpdxElement": file_spdx_id,
                    "relationshipType": "CONTAINS",
                })

    # Update the SBOM on disk
    sbom_path.write_text(json.dumps(sbom_data, indent=2, sort_keys=True))


def create_externals_sbom() -> None:
    sbom_path = CPYTHON_ROOT_DIR / "Misc/externals.spdx.json"
    sbom_data = json.loads(sbom_path.read_bytes())

    externals = get_externals()
    externals_name_to_version = {}
    externals_name_to_git_tag = {}
    for git_tag in externals:
        name, _, version = git_tag.rpartition("-")
        externals_name_to_version[name] = version
        externals_name_to_git_tag[name] = git_tag

    # Ensure all packages in this tool are represented also in the SBOM file.
    actual_names = {package["name"] for package in sbom_data["packages"]}
    expected_names = set(externals_name_to_version)
    error_if(
        actual_names != expected_names,
        f"Packages defined in SBOM tool don't match those defined in SBOM file: {actual_names}, {expected_names}",
    )

    # Set the versionInfo and downloadLocation fields for all packages.
    for package in sbom_data["packages"]:
        package_version = externals_name_to_version[package["name"]]

        # Update the version information in all the locations.
        package["versionInfo"] = package_version
        for external_ref in package["externalRefs"]:
            if external_ref["referenceType"] != "cpe23Type":
                continue
            # Version is the fifth field of a CPE.
            cpe23ref = external_ref["referenceLocator"]
            external_ref["referenceLocator"] = re.sub(
                r"\A(cpe(?::[^:]+){4}):[^:]+:",
                fr"\1:{package_version}:",
                cpe23ref
            )

        download_location = (
            f"https://github.com/python/cpython-source-deps/archive/refs/tags/{externals_name_to_git_tag[package['name']]}.tar.gz"
        )
        download_location_changed = download_location != package["downloadLocation"]
        package["downloadLocation"] = download_location

        # If the download URL has changed we want one to get recalulated.
        if download_location_changed:
            package.pop("checksums", None)

    check_sbom_packages(sbom_data)

    # Update the SBOM on disk
    sbom_path.write_text(json.dumps(sbom_data, indent=2, sort_keys=True))


def main() -> None:
    # Don't regenerate the SBOM if we're not a git repository.
    if not is_root_directory_git_index():
        print("Skipping SBOM generation due to not being a git repository")
        return

    create_source_sbom()
    create_externals_sbom()


if __name__ == "__main__":
    main()
