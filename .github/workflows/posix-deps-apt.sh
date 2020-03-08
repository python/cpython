#!/bin/sh
apt-get update

apt-get -yq install \
    build-essential \
    zlib1g-dev \
    libbz2-dev \
    liblzma-dev \
    libncurses5-dev \
    libreadline6-dev \
    libsqlite3-dev \
    libssl-dev \
    libgdbm-dev \
    tk-dev \
    lzma \
    lzma-dev \
    liblzma-dev \
    libffi-dev \
    uuid-dev \
    xvfb \
    lcov
