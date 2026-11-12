#!/bin/bash
# Build and run the libpython embedding smoke test.
set -euo pipefail
cd "$(dirname "$0")/../../.."

BUILD_DIR=${CROSS_BUILD_DIR:-cross-build}/wasm32-emscripten/build/python
make -C "$BUILD_DIR" web_embed_test

# Node 24 needs JSPI enabled explicitly; it is on by default afterwards.
NODE=${NODE:-node}
NODE_FLAGS=
if [ "$("$NODE" -p 'process.versions.node.split(".")[0]')" = 24 ]; then
    NODE_FLAGS=--experimental-wasm-jspi
fi

# Run from the build directory: Emscripten resolves main.data relative to cwd.
cd "$BUILD_DIR/web_embed_test"
rc=0
out=$("$NODE" $NODE_FLAGS main.js 2>&1) || rc=$?
echo "$out"
[ "$rc" -eq 0 ] && grep -q "web_embed_test: ok" <<<"$out"
