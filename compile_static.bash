#!/usr/bin/env bash

export LIBS="-lc -lgcc"
export LDFLAGS="-static"
export LINKFORSHARED=" "

export MODULE_BUILDTYPE="static"
export CONFIG_SITE="config.site-static"

CROSS_COMPILE_PREFIX="${CROSS_COMPILE_PREFIX:-x86_64-linux-gnu}"

./configure \
	-C \
	--disable-test-modules \
	--with-ensurepip=no \
	--without-decimal-contextvar \
	--build=x86_64-pc-linux-gnu \
	--host="${CROSS_COMPILE_PREFIX}" \
	--with-build-python=/usr/bin/python3.12 \
	--disable-ipv6 \
	--disable-shared

make -j $(nproc)
