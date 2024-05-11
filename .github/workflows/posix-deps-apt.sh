#!/bin/sh
apt-get update

# autoconf-archive is needed by autoreconf (check_generated_files job)
apt-get -yq install \
    build-essential \
    pkg-config \
    autoconf-archive \
    ccache \
    gdb \
    lcov \
    libb2-dev \
    libbz2-dev \
    libffi-dev \
    libgdbm-dev \
    libgdbm-compat-dev \
    liblzma-dev \
    libmpdec-dev \
    libncurses5-dev \
    libreadline6-dev \
    libsqlite3-dev \
    libssl-dev \
    lzma \
    lzma-dev \
    strace \
    tk-dev \
    uuid-dev \
    xvfb \
    zlib1g-dev
