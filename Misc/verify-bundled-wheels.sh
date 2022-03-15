#!/bin/sh

#
# Purpose: Compare checksums of bundled pip and setuptools to ones
#          published on PyPI (retrieved via the Warehouse’s JSON API).
#
# Synopsis: ./Misc/verify-bundled-wheels.sh
#
# Requirements: curl, jq
#

cd "$(dirname "$0")/.."
package_names="pip setuptools"
exit_status=0

for package_name in ${package_names}; do
    package_path=$(find Lib/ensurepip/_bundled/ -name "${package_name}*.whl")
    echo "$package_path"

    package_name_uppercase=$(echo "$package_name" | tr "[:lower:]" "[:upper:]")
    package_version=$(
        grep -Pom 1 "_${package_name_uppercase}_VERSION = \"\K[^\"]+" Lib/ensurepip/__init__.py
    )
    expected_digest=$(curl -fs "https://pypi.org/pypi/${package_name}/json" | jq --raw-output "
        .releases.\"${package_version}\"
        | .[]
        | select(.filename == \"$(basename "$package_path")\")
        | .digests.sha256
    ")
    echo "Expected digest: ${expected_digest}"

    actual_digest=$(sha256sum "$package_path" | awk '{print $1}')
    echo "Actual digest:\t ${actual_digest}"

    # The messages are formatted to be parsed by GitHub Actions.
    # https://docs.github.com/en/actions/using-workflows/workflow-commands-for-github-actions#setting-a-notice-message
    if [ "$actual_digest" = "$expected_digest" ]; then
        echo "::notice file=${package_path}::Successfully verified checksum of this wheel."
    else
        echo "::error file=${package_path}::Failed to verify checksum of this wheel."
        exit_status=1
    fi
    echo
done

exit $exit_status
