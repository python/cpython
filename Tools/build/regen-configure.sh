#!/usr/bin/env bash

set -e -x

# The check_autoconf_regen job of .github/workflows/build.yml must kept in
# sync with this script. Use the same container image than the job so the job
# doesn't need to run autoreconf in a container.
IMAGE="ghcr.io/python/autoconf:2025.01.02.12581854023"
AUTORECONF="autoreconf -ivf -Werror"

WORK_DIR="/src"

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

purge_autom4te_cache() {
    "$RUNTIME" run --rm -v "$abs_srcdir:$WORK_DIR$PATH_OPT" \
        --entrypoint sh "$IMAGE" -c "rm -rf $WORK_DIR/autom4te.cache"
}

purge_autom4te_cache
trap purge_autom4te_cache EXIT
"$RUNTIME" run --rm -u "$(id -u):$(id -g)" -v "$abs_srcdir:$WORK_DIR$PATH_OPT" "$IMAGE"
