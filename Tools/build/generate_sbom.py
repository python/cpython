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
from urllib.request import urlopen

CPYTHON_ROOT_DIR = pathlib.Path(__file__).parent.parent.parent

# Before adding a new entry to this list, double check that
# the license expression is a valid SPDX license expression:
# See: https://spdx.org/licenses
ALLOWED_LICENSE_EXPRESSIONS = {
    "MIT",
    "CC0-1.0",
    "Apache-2.0",
    "BSD-2-Clause",
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
    "originator",
    "primaryPackagePurpose",
])


class PackageFiles(typing.NamedTuple):
    """Structure for describing the files of a package"""
    include: list[str]
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
        check=False,
        stdout=subprocess.PIPE,
    )
    # 1 means matches, 0 means no matches.
    assert git_check_ignore_proc.returncode in (0, 1)

    # Return the list of paths sorted
    git_check_ignore_lines = git_check_ignore_proc.stdout.decode().splitlines()
    return sorted([line.split()[-1] for line in git_check_ignore_lines if line.startswith("::")])


def discover_pip_sbom_package(sbom_data: dict[str, typing.Any]) -> None:
    """pip is a part of a packaging ecosystem (Python, surprise!) so it's actually
    automatable to discover the metadata we need like the version and checksums
    so let's do that on behalf of our friends at the PyPA.
    """
    global PACKAGE_TO_FILES

    ensurepip_bundled_dir = CPYTHON_ROOT_DIR / "Lib/ensurepip/_bundled"
    pip_wheels = []

    # Find the hopefully one pip wheel in the bundled directory.
    for wheel_filename in os.listdir(ensurepip_bundled_dir):
        if wheel_filename.startswith("pip-"):
            pip_wheels.append(wheel_filename)
    if len(pip_wheels) != 1:
        print("Zero or multiple pip wheels detected in 'Lib/ensurepip/_bundled'")
        sys.exit(1)
    pip_wheel_filename = pip_wheels[0]

    # Add the wheel filename to the list of files so the SBOM file
    # and relationship generator can work its magic on the wheel too.
    PACKAGE_TO_FILES["pip"] = PackageFiles(
        include=[f"Lib/ensurepip/_bundled/{pip_wheel_filename}"]
    )

    # Wheel filename format puts the version right after the project name.
    pip_version = pip_wheel_filename.split("-")[1]
    pip_checksum_sha256 = hashlib.sha256(
        (ensurepip_bundled_dir / pip_wheel_filename).read_bytes()
    ).hexdigest()

    # Get pip's download location from PyPI. Check that the checksum is correct too.
    try:
        raw_text = urlopen(f"https://pypi.org/pypi/pip/{pip_version}/json").read()
        pip_release_metadata = json.loads(raw_text)
        url: dict[str, typing.Any]

        # Look for a matching artifact filename and then check
        # its remote checksum to the local one.
        for url in pip_release_metadata["urls"]:
            if url["filename"] == pip_wheel_filename:
                break
        else:
            raise ValueError(f"No matching filename on PyPI for '{pip_wheel_filename}'")
        if url["digests"]["sha256"] != pip_checksum_sha256:
            raise ValueError(f"Local pip checksum doesn't match artifact on PyPI")

        # Successfully found the download URL for the matching artifact.
        pip_download_url = url["url"]

    except (OSError, ValueError) as e:
        print(f"Couldn't fetch pip's metadata from PyPI: {e}")
        sys.exit(1)

    # Remove pip from the existing SBOM packages if it's there
    # and then overwrite its entry with our own generated one.
    sbom_data["packages"] = [
        sbom_package
        for sbom_package in sbom_data["packages"]
        if sbom_package["name"] != "pip"
    ]
    sbom_data["packages"].append(
        {
            "SPDXID": spdx_id("SPDXRef-PACKAGE-pip"),
            "name": "pip",
            "versionInfo": pip_version,
            "originator": "Organization: Python Packaging Authority",
            "licenseConcluded": "MIT",
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


def main() -> None:
    sbom_path = CPYTHON_ROOT_DIR / "Misc/sbom.spdx.json"
    sbom_data = json.loads(sbom_path.read_bytes())

    # Insert pip's SBOM metadata from the wheel.
    discover_pip_sbom_package(sbom_data)

    # Ensure all packages in this tool are represented also in the SBOM file.
    assert {package["name"] for package in sbom_data["packages"]} == set(PACKAGE_TO_FILES)

    # Make a bunch of assertions about the SBOM data to ensure it's consistent.
    for package in sbom_data["packages"]:

        # Properties and ID must be properly formed.
        assert set(package.keys()) == REQUIRED_PROPERTIES_PACKAGE
        assert package["SPDXID"] == spdx_id(f"SPDXRef-PACKAGE-{package['name']}")

        # Version must be in the download and external references.
        version = package["versionInfo"]
        assert version in package["downloadLocation"]
        assert all(version in ref["referenceLocator"] for ref in package["externalRefs"])

        # License must be on the approved list for SPDX.
        assert package["licenseConcluded"] in ALLOWED_LICENSE_EXPRESSIONS, package["licenseConcluded"]

    # Regenerate file information from current data.
    sbom_files = []
    sbom_relationships = []

    # We call 'sorted()' here a lot to avoid filesystem scan order issues.
    for name, files in sorted(PACKAGE_TO_FILES.items()):
        package_spdx_id = spdx_id(f"SPDXRef-PACKAGE-{name}")
        exclude = files.exclude or ()
        for include in sorted(files.include):

            # Find all the paths and then filter them through .gitignore.
            paths = glob.glob(include, root_dir=CPYTHON_ROOT_DIR, recursive=True)
            paths = filter_gitignored_paths(paths)
            assert paths, include  # Make sure that every value returns something!

            for path in paths:
                # Skip directories and excluded files
                if not (CPYTHON_ROOT_DIR / path).is_file() or path in exclude:
                    continue

                # SPDX requires SHA1 to be used for files, but we provide SHA256 too.
                data = (CPYTHON_ROOT_DIR / path).read_bytes()
                checksum_sha1 = hashlib.sha1(data).hexdigest()
                checksum_sha256 = hashlib.sha256(data).hexdigest()

                file_spdx_id = spdx_id(f"SPDXRef-FILE-{path}")
                sbom_files.append({
                    "SPDXID": file_spdx_id,
                    "fileName": path,
                    "checksums": [
                        {"algorithm": "SHA1", "checksumValue": checksum_sha1},
                        {"algorithm": "SHA256", "checksumValue": checksum_sha256},
                    ],
                })

                # Tie each file back to its respective package.
                sbom_relationships.append({
                    "spdxElementId": package_spdx_id,
                    "relatedSpdxElement": file_spdx_id,
                    "relationshipType": "CONTAINS",
                })

    # Update the SBOM on disk
    sbom_data["files"] = sbom_files
    sbom_data["relationships"] = sbom_relationships
    sbom_path.write_text(json.dumps(sbom_data, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
