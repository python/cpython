#!/bin/sh
apt-get update

apt-get -yq install \
    build-essential \
    pkg-config \
    ccache \
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

# Workaround missing libmpdec-dev on ubuntu 24.04:
# https://launchpad.net/~ondrej/+archive/ubuntu/php
# https://deb.sury.org/
sudo add-apt-repository ppa:ondrej/php
apt-get update
apt-get -yq install libmpdec-dev
