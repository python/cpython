#!/bin/bash
# Run the smoke test built by "make web_embed_test".
set -euo pipefail
cd "$(dirname "$0")/../../.."

CROSS_BUILD_DIR=${CROSS_BUILD_DIR:-cross-build}
# Run from the build directory: Emscripten resolves main.data relative to cwd.
cd "$CROSS_BUILD_DIR/wasm32-emscripten/build/python/web_embed_test"

NODE=${NODE:-node}
# Node 24 needs JSPI enabled explicitly; it is on by default afterwards.
if [ "$("$NODE" -e 'process.stdout.write(process.version.slice(1).split(".")[0])')" = "24" ]; then
    NODE_FLAGS=--experimental-wasm-jspi
else
    NODE_FLAGS=
fi

out=$("$NODE" $NODE_FLAGS main.js 2>&1) || true
echo "$out"
grep -q "web_embed_test: ok" <<<"$out"
