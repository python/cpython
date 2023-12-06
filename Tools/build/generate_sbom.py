"""Tool for generating Software Bill of Materials (SBOM) for Python's dependencies"""

import re
import hashlib
import json
import glob
import pathlib
import typing

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
    "mpdecimal": PackageFiles(
        include=["Modules/_decimal/libmpdec/**"]
    ),
    "expat": PackageFiles(
        include=["Modules/expat/**"]
    ),
    "pip": PackageFiles(
        include=["Lib/ensurepip/_bundled/pip-23.3.1-py3-none-any.whl"]
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
    )
}


def spdx_id(value: str) -> str:
    """Encode a value into characters that are valid in an SPDX ID"""
    return re.sub(r"[^a-zA-Z0-9.\-]+", "-", value)


def main() -> None:
    root_dir = pathlib.Path(__file__).parent.parent.parent
    sbom_path = root_dir / "Misc/sbom.spdx.json"
    sbom_data = json.loads(sbom_path.read_bytes())

    # Make a bunch of assertions about the SBOM data to ensure it's consistent.
    assert {package["name"] for package in sbom_data["packages"]} == set(PACKAGE_TO_FILES)
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
            paths = sorted(glob.glob(include, root_dir=root_dir, recursive=True))
            assert paths, include  # Make sure that every value returns something!

            for path in paths:
                # Skip directories and excluded files
                if not (root_dir / path).is_file() or path in exclude:
                    continue

                # SPDX requires SHA1 to be used for files, but we provide SHA256 too.
                data = (root_dir / path).read_bytes()
                checksum_sha1 = hashlib.sha1(data).hexdigest()
                checksum_sha256 = hashlib.sha256(data).hexdigest()

                file_spdx_id = spdx_id(f"SPDXRef-FILE-{path}")
                sbom_files.append({
                    "SPDXID": file_spdx_id,
                    "fileName": path,
                    "checksums": [
                        {"algorithm": "SHA1", "checksumValue": checksum_sha1},
                        {"algorithm": "SHA256", "checksumValue": checksum_sha256}
                    ]
                })

                # Tie each file back to its respective package.
                sbom_relationships.append({
                    "spdxElementId": package_spdx_id,
                    "relatedSpdxElement": file_spdx_id,
                    "relationshipType": "CONTAINS"
                })

    # Update the SBOM on disk
    sbom_data["files"] = sbom_files
    sbom_data["relationships"] = sbom_relationships
    sbom_path.write_text(json.dumps(sbom_data, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
