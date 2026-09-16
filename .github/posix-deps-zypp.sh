#!/bin/sh
set -eu

zypper --non-interactive refresh

# The container can have newer libraries than the repositories' matching -devel
# packages. Allow the solver to downgrade dependencies rather than cancel.
# Install build tools explicitly: build patterns pull in a full base system,
# including packages that conflict with the container's busybox replacements.
# Python 3.6 needs OpenSSL 1.1 headers rather than Leap's default OpenSSL 3.
# GDB needs the UTF-32 converters supplied by glibc-locale-base.
zypper --non-interactive install --auto-agree-with-licenses \
    --allow-downgrade --no-recommends \
    autoconf automake gcc gcc-c++ make patch pkg-config python3 \
    binutils diffutils findutils gdb glibc-locale-base gzip libtool perl tar which \
    libabigail-tools xorg-x11-server-Xvfb xvfb-run \
    cantarell-fonts google-droid-fonts google-inconsolata-fonts dejavu-fonts \
    libffi-devel \
    xz-devel \
    bzip2 \
    zlib-devel \
    libbz2-devel \
    ncurses-devel \
    readline-devel \
    sqlite3-devel \
    libopenssl-1_1-devel \
    gdbm-devel \
    tk-devel \
    libuuid-devel \
    lcov
