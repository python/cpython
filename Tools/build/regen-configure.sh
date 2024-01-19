#!/usr/bin/env bash

set -e -x

# Manifest corresponds with tag '269'
# See https://github.com/tiran/cpython_autoconf container
CPYTHON_AUTOCONF_MANIFEST="sha256:f370fee95eefa3d57b00488bce4911635411fa83e2d293ced8cf8a3674ead939"

abs_srcdir=$(cd "$(dirname "$0")/../.."; pwd)

if podman --version &>/dev/null; then
    RUNTIME="podman"
elif docker --version &>/dev/null; then
    RUNTIME="docker"
else
    echo "$@ needs either Podman or Docker container runtime." >&2
    exit 1
fi

PATH_OPT=""
if command -v selinuxenabled >/dev/null && selinuxenabled; then
    PATH_OPT=":Z"
fi

"$RUNTIME" run --rm --pull=missing -v "$abs_srcdir:/src$PATH_OPT" "quay.io/tiran/cpython_autoconf@$CPYTHON_AUTOCONF_MANIFEST"
