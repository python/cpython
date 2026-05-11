#!/bin/sh
apt-get update

apt-get -yq --no-install-recommends install \
    build-essential \
    pkg-config \
    cmake \
    gdb \
    lcov \
    libb2-dev \
    libbz2-dev \
    libffi-dev \
    libgdbm-dev \
    libgdbm-compat-dev \
    liblzma-dev \
    libncurses5-dev \
    libreadline6-dev \
    libsqlite3-dev \
    libssl-dev \
    libzstd-dev \
    lzma \
    lzma-dev \
    strace \
    tk-dev \
    uuid-dev \
    xvfb \
    zlib1g-dev

# Workaround missing libmpdec-dev on ubuntu 24.04 by building mpdecimal
# from source. ppa:ondrej/php (launchpad.net) are unreliable 
# (https://status.canonical.com) so fetch the tarball directly
# from the upstream host.
# https://www.bytereef.org/mpdecimal/
MPDECIMAL_VERSION=4.0.1
curl -fsSL "https://www.bytereef.org/software/mpdecimal/releases/mpdecimal-${MPDECIMAL_VERSION}.tar.gz" \
    | tar -xz -C /tmp
(cd "/tmp/mpdecimal-${MPDECIMAL_VERSION}" \
    && ./configure --prefix=/usr/local \
    && make -j"$(nproc)" \
    && make install)
ldconfig
