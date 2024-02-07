"""Tool for generating Software Bill of Materials (SBOM) for Python's dependencies"""
import os
import re
import hashlib
import json
import glob
import pathlib
import subprocess
import sys
import typing
import zipfile
from urllib.request import urlopen

CPYTHON_ROOT_DIR = pathlib.Path(__file__).parent.parent.parent

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
    # NOTE: pip's entry in this structure is automatically generated in
    # the 'discover_pip_sbom_package()' function below.
    "mpdecimal": PackageFiles(
        include=["Modules/_decimal/libmpdec/**"]
    ),
    "expat": PackageFiles(
        include=["Modules/expat/**"]
    ),
    "macholib": PackageFiles(
        include=["Lib/ctypes/macholib/**"],
        exclude=[
            "Lib/ctypes/macholib/README.ctypes",
            "Lib/ctypes/macholib/fetch_macholib",
            "Lib/ctypes/macholib/fetch_macholib.bat",
        ],
    ),
    "libb2": PackageFiles(
        include=["Modules/_blake2/impl/**"]
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


def filter_gitignored_paths(paths: list[str]) -> list[str]:
    """
    Filter out paths excluded by the gitignore file.
    The output of 'git check-ignore --non-matching --verbose' looks
    like this for non-matching (included) files:

        '::<whitespace><path>'

    And looks like this for matching (excluded) files:

        '.gitignore:9:*.a    Tools/lib.a'
    """
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

    # Return the list of paths sorted
    git_check_ignore_lines = git_check_ignore_proc.stdout.decode().splitlines()
    return sorted([line.split()[-1] for line in git_check_ignore_lines if line.startswith("::")])


def fetch_package_metadata_from_pypi(project: str, version: str, filename: str | None = None) -> tuple[str, str] | None:
    """
    Fetches the SHA256 checksum and download location from PyPI.
    If we're given a filename then we match with that, otherwise we use wheels.
    """
    # Get pip's download location from PyPI. Check that the checksum is correct too.
    try:
        raw_text = urlopen(f"https://pypi.org/pypi/{project}/{version}/json").read()
        release_metadata = json.loads(raw_text)
        url: dict[str, typing.Any]

        # Look for a matching artifact filename and then check
        # its remote checksum to the local one.
        for url in release_metadata["urls"]:
            # pip can only use Python-only dependencies, so there's
            # no risk of picking the 'incorrect' wheel here.
            if (
                (filename is None and url["packagetype"] == "bdist_wheel")
                or (filename is not None and url["filename"] == filename)
            ):
                break
        else:
            raise ValueError(f"No matching filename on PyPI for '{filename}'")

        # Successfully found the download URL for the matching artifact.
        download_url = url["url"]
        checksum_sha256 = url["digests"]["sha256"]
        return download_url, checksum_sha256

    except (OSError, ValueError) as e:
        # Fail if we're running in CI where we should have an internet connection.
        error_if(
            "CI" in os.environ,
            f"Couldn't fetch metadata for project '{project}' from PyPI: {e}"
        )
        return None


def find_ensurepip_pip_wheel() -> pathlib.Path | None:
    """Try to find the pip wheel bundled in ensurepip. If missing return None"""

    ensurepip_bundled_dir = CPYTHON_ROOT_DIR / "Lib/ensurepip/_bundled"

    pip_wheels = []
    try:
        for wheel_filename in os.listdir(ensurepip_bundled_dir):
            if wheel_filename.startswith("pip-"):
                pip_wheels.append(wheel_filename)
            else:
                print(f"Unexpected wheel in ensurepip: '{wheel_filename}'")
                sys.exit(1)

    # Ignore this error, likely caused by downstream distributors
    # deleting the 'ensurepip/_bundled' directory.
    except FileNotFoundError:
        pass

    if len(pip_wheels) == 0:
        return None
    elif len(pip_wheels) > 1:
        print("Multiple pip wheels detected in 'Lib/ensurepip/_bundled'")
        sys.exit(1)
    # Otherwise return the one pip wheel.
    return ensurepip_bundled_dir / pip_wheels[0]


def maybe_remove_pip_and_deps_from_sbom(sbom_data: dict[str, typing.Any]) -> None:
    """
    Removes pip and its dependencies from the SBOM data
    if the pip wheel is removed from ensurepip. This is done
    by redistributors of Python and pip.
    """

    # If there's a wheel we don't remove anything.
    if find_ensurepip_pip_wheel() is not None:
        return

    # Otherwise we traverse the relationships
    # to find dependent packages to remove.
    sbom_pip_spdx_id = spdx_id("SPDXRef-PACKAGE-pip")
    sbom_spdx_ids_to_remove = {sbom_pip_spdx_id}

    # Find all package SPDXIDs that pip depends on.
    for sbom_relationship in sbom_data["relationships"]:
        if (
            sbom_relationship["relationshipType"] == "DEPENDS_ON"
            and sbom_relationship["spdxElementId"] == sbom_pip_spdx_id
        ):
            sbom_spdx_ids_to_remove.add(sbom_relationship["relatedSpdxElement"])

    # Remove all the packages and relationships.
    sbom_data["packages"] = [
        sbom_package for sbom_package in sbom_data["packages"]
        if sbom_package["SPDXID"] not in sbom_spdx_ids_to_remove
    ]
    sbom_data["relationships"] = [
        sbom_relationship for sbom_relationship in sbom_data["relationships"]
        if sbom_relationship["relatedSpdxElement"] not in sbom_spdx_ids_to_remove
    ]


def discover_pip_sbom_package(sbom_data: dict[str, typing.Any]) -> None:
    """pip is a part of a packaging ecosystem (Python, surprise!) so it's actually
    automatable to discover the metadata we need like the version and checksums
    so let's do that on behalf of our friends at the PyPA. This function also
    discovers vendored packages within pip and fetches their metadata.
    """
    global PACKAGE_TO_FILES

    pip_wheel_filepath = find_ensurepip_pip_wheel()
    if pip_wheel_filepath is None:
        return  # There's no pip wheel, nothing to discover.

    # Add the wheel filename to the list of files so the SBOM file
    # and relationship generator can work its magic on the wheel too.
    PACKAGE_TO_FILES["pip"] = PackageFiles(
        include=[str(pip_wheel_filepath.relative_to(CPYTHON_ROOT_DIR))]
    )

    # Wheel filename format puts the version right after the project name.
    pip_version = pip_wheel_filepath.name.split("-")[1]
    pip_checksum_sha256 = hashlib.sha256(
        pip_wheel_filepath.read_bytes()
    ).hexdigest()

    pip_metadata = fetch_package_metadata_from_pypi(
        project="pip",
        version=pip_version,
        filename=pip_wheel_filepath.name,
    )
    # We couldn't fetch any metadata from PyPI,
    # so we give up on verifying if we're not in CI.
    if pip_metadata is None:
        return

    pip_download_url, pip_actual_sha256 = pip_metadata
    if pip_actual_sha256 != pip_checksum_sha256:
        raise ValueError("Unexpected")

    # Parse 'pip/_vendor/vendor.txt' from the wheel for sub-dependencies.
    with zipfile.ZipFile(pip_wheel_filepath) as whl:
        vendor_txt_data = whl.read("pip/_vendor/vendor.txt").decode()

        # With this version regex we're assuming that pip isn't using pre-releases.
        # If any version doesn't match we get a failure below, so we're safe doing this.
        version_pin_re = re.compile(r"^([a-zA-Z0-9_.-]+)==([0-9.]*[0-9])$")
        sbom_pip_dependency_spdx_ids = set()
        for line in vendor_txt_data.splitlines():
            line = line.partition("#")[0].strip()  # Strip comments and whitespace.
            if not line:  # Skip empty lines.
                continue

            # Non-empty lines we must be able to match.
            match = version_pin_re.match(line)
            error_if(match is None, f"Couldn't parse line from pip vendor.txt: '{line}'")
            assert match is not None  # Make mypy happy.

            # Parse out and normalize the project name.
            project_name, project_version = match.groups()
            project_name = project_name.lower()

            # At this point if pip's metadata fetch succeeded we should
            # expect this request to also succeed.
            project_metadata = (
                fetch_package_metadata_from_pypi(project_name, project_version)
            )
            assert project_metadata is not None
            project_download_url, project_checksum_sha256 = project_metadata

            # Update our SBOM data with what we received from PyPI.
            # Don't overwrite any existing values.
            sbom_project_spdx_id = spdx_id(f"SPDXRef-PACKAGE-{project_name}")
            sbom_pip_dependency_spdx_ids.add(sbom_project_spdx_id)
            for package in sbom_data["packages"]:
                if package["SPDXID"] != sbom_project_spdx_id:
                    continue

                # Only thing missing from this blob is the `licenseConcluded`,
                # that needs to be triaged by human maintainers if the list changes.
                package.update({
                    "SPDXID": sbom_project_spdx_id,
                    "name": project_name,
                    "versionInfo": project_version,
                    "downloadLocation": project_download_url,
                    "checksums": [
                        {"algorithm": "SHA256", "checksumValue": project_checksum_sha256}
                    ],
                    "externalRefs": [
                        {
                            "referenceCategory": "PACKAGE_MANAGER",
                            "referenceLocator": f"pkg:pypi/{project_name}@{project_version}",
                            "referenceType": "purl",
                        },
                    ],
                    "primaryPackagePurpose": "SOURCE"
                })
                break

            PACKAGE_TO_FILES[project_name] = PackageFiles(include=None)

    # Remove pip from the existing SBOM packages if it's there
    # and then overwrite its entry with our own generated one.
    sbom_pip_spdx_id = spdx_id("SPDXRef-PACKAGE-pip")
    sbom_data["packages"] = [
        sbom_package
        for sbom_package in sbom_data["packages"]
        if sbom_package["name"] != "pip"
    ]
    sbom_data["packages"].append(
        {
            "SPDXID": sbom_pip_spdx_id,
            "name": "pip",
            "versionInfo": pip_version,
            "originator": "Organization: Python Packaging Authority",
            "licenseConcluded": "NOASSERTION",
            "downloadLocation": pip_download_url,
            "checksums": [
                {"algorithm": "SHA256", "checksumValue": pip_checksum_sha256}
            ],
            "externalRefs": [
                {
                    "referenceCategory": "SECURITY",
                    "referenceLocator": f"cpe:2.3:a:pypa:pip:{pip_version}:*:*:*:*:*:*:*",
                    "referenceType": "cpe23Type",
                },
                {
                    "referenceCategory": "PACKAGE_MANAGER",
                    "referenceLocator": f"pkg:pypi/pip@{pip_version}",
                    "referenceType": "purl",
                },
            ],
            "primaryPackagePurpose": "SOURCE",
        }
    )
    for sbom_dep_spdx_id in sorted(sbom_pip_dependency_spdx_ids):
        sbom_data["relationships"].append({
            "spdxElementId": sbom_pip_spdx_id,
            "relatedSpdxElement": sbom_dep_spdx_id,
            "relationshipType": "DEPENDS_ON"
        })


def main() -> None:
    sbom_path = CPYTHON_ROOT_DIR / "Misc/sbom.spdx.json"
    sbom_data = json.loads(sbom_path.read_bytes())

    # Check if pip should be removed if the wheel is missing.
    # We can't reset the SBOM relationship data until checking this.
    maybe_remove_pip_and_deps_from_sbom(sbom_data)

    # We regenerate all of this information. Package information
    # should be preserved though since that is edited by humans.
    sbom_data["files"] = []
    sbom_data["relationships"] = []

    # Insert pip's SBOM metadata from the wheel.
    discover_pip_sbom_package(sbom_data)

    # Ensure all packages in this tool are represented also in the SBOM file.
    actual_names = {package["name"] for package in sbom_data["packages"]}
    expected_names = set(PACKAGE_TO_FILES)
    error_if(
        actual_names != expected_names,
        f"Packages defined in SBOM tool don't match those defined in SBOM file: {actual_names}, {expected_names}",
    )

    # Make a bunch of assertions about the SBOM data to ensure it's consistent.
    for package in sbom_data["packages"]:
        # Properties and ID must be properly formed.
        error_if(
            "name" not in package,
            "Package is missing the 'name' field"
        )
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

        # License must be on the approved list for SPDX.
        license_concluded = package["licenseConcluded"]
        error_if(
            license_concluded != "NOASSERTION",
            f"License identifier must be 'NOASSERTION'"
        )

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
                # Skip directories and excluded files
                if not (CPYTHON_ROOT_DIR / path).is_file() or path in exclude:
                    continue

                # SPDX requires SHA1 to be used for files, but we provide SHA256 too.
                data = (CPYTHON_ROOT_DIR / path).read_bytes()
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


if __name__ == "__main__":
    main()
