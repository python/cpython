#!/usr/bin/bash

set -e -x

# Quick check to avoid running configure just to fail in the end.
if [ -f Programs/python.o ]; then
    echo "Can't do an out-of-tree build w/ an in-place build pre-existing (i.e., found Programs/python.o)" >&2
    exit 1
fi

if [ ! -f Modules/Setup.local ]; then
    touch Modules/Setup.local
fi

# TODO: check if `make` and `pkgconfig` are installed
# TODO: detect if wasmtime is installed

# Create the "build" Python.
mkdir -p builddir/build
pushd builddir/build
../../configure -C
make -s -j 4 all
export PYTHON_VERSION=`./python -c 'import sys; print(f"{sys.version_info.major}.{sys.version_info.minor}")'`
popd

# Create the "host"/WASI Python.
export CONFIG_SITE="$(pwd)/Tools/wasm/config.site-wasm32-wasi"
export HOSTRUNNER="wasmtime run --mapdir /::$(pwd) --env PYTHONPATH=/builddir/wasi/build/lib.wasi-wasm32-$PYTHON_VERSION $(pwd)/builddir/wasi/python.wasm --"

mkdir -p builddir/wasi
pushd builddir/wasi
../../Tools/wasm/wasi-env \
    ../../configure \
        -C \
        --host=wasm32-unknown-wasi \
        --build=$(../../config.guess) \
        --with-build-python=../build/python
make -s -j 4 all
# Create a helper script for executing the host/WASI Python.
printf "#!/bin/sh\nexec $HOSTRUNNER \"\$@\"\n" > run_wasi.sh
chmod 755 run_wasi.sh
./run_wasi.sh --version
popd

