#!/usr/bin/env bash

set -e -x

# The check_generated_files job of .github/workflows/build.yml must kept in
# sync with this script. Use the same container image than the job so the job
# doesn't need to run autoreconf in a container.
IMAGE="ubuntu:22.04"
DEPENDENCIES="autotools-dev autoconf autoconf-archive pkg-config"
AUTORECONF="autoreconf -ivf -Werror"

WORK_DIR="/src"
SHELL_CMD="apt-get update && apt-get -yq install $DEPENDENCIES && cd $WORK_DIR && $AUTORECONF"

abs_srcdir=$(cd $(dirname $0)/../..; pwd)

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

"$RUNTIME" run --rm -v "$abs_srcdir:$WORK_DIR$PATH_OPT" "$IMAGE" /usr/bin/bash -c "$SHELL_CMD"
