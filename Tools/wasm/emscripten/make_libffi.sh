#!/bin/bash
set +e

export CFLAGS="-O2 -fPIC -DWASM_BIGINT"
export CXXFLAGS="$CFLAGS"

# Build paths
export CPATH="$PREFIX/include"
export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig"
export EM_PKG_CONFIG_PATH="$PKG_CONFIG_PATH"

# Specific variables for cross-compilation
export CHOST="wasm32-unknown-linux" # wasm32-unknown-emscripten

emconfigure ./configure --host=$CHOST --prefix="$PREFIX" --enable-static --disable-shared --disable-dependency-tracking \
  --disable-builddir --disable-multi-os-directory --disable-raw-api --disable-docs

make install
# Some forgotten headers?
cp fficonfig.h $PREFIX/include/
cp include/ffi_common.h $PREFIX/include/
