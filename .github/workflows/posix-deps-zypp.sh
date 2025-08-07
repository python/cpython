#!/bin/sh
zypper refresh

zypper --non-interactive install --no-confirm --auto-agree-with-licenses --force \
    pattern:devel_rpm_build pattern:devel_C_C++ make python3 \
    libabigail-tools xorg-x11-server-Xvfb xvfb-run \
    cantarell-fonts google-droid-fonts google-inconsolata-fonts dejavu-fonts \
    libffi-devel \
    xz-devel \
    bzip2 \
    zlib-devel \
    libbz2-devel \
    ncurses-devel \
    readline6-devel \
    sqlite3-devel \
    libopenssl-devel \
    gdbm-devel \
    tk-devel \
    lzma \
    lzma-devel \
    libffi-devel \
    uuid-devel \
    xvfb-run \
    lcov
