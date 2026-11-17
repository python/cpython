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

# Run the container as the invoking user so that generated files are owned by
# that user and not by root. Rootless runtimes already map root to the invoking
# user, so a user mapping is only needed for rootful Docker.
USER_OPT=""
case "$RUNTIME" in
    podman)
        USER_OPT="--userns=keep-id"
        ;;
    docker)
        if ! docker info --format '{{.SecurityOptions}}' | grep -q rootless; then
            USER_OPT="-u $(id -u):$(id -g)"
        fi
        ;;
esac

purge_autom4te_cache() {
    [ -d "$abs_srcdir/autom4te.cache" ] || return 0
    "$RUNTIME" run --rm -v "$abs_srcdir:$WORK_DIR$PATH_OPT" \
        "$IMAGE" sh -c "rm -rf $WORK_DIR/autom4te.cache"
}

purge_autom4te_cache
trap 'purge_autom4te_cache || true' EXIT
"$RUNTIME" run --rm $USER_OPT -v "$abs_srcdir:$WORK_DIR$PATH_OPT" "$IMAGE"
