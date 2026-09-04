#!/bin/sh
apt-get update

apt-get -yq --no-install-recommends install \
    build-essential \
    pkg-config \
    cmake \
    curl \
    gdb \
    lcov \
    libb2-dev \
    libbz2-dev \
    libffi-dev \
    libgdbm-compat-dev \
    libgdbm-dev \
    liblzma-dev \
    libmpdec-dev \
    libncurses5-dev \
    libreadline6-dev \
    libsqlite3-dev \
    libssl-dev \
    libzstd-dev \
    strace \
    tk-dev \
    uuid-dev \
    xvfb \
    zlib1g-dev
