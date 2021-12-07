# Python WebAssembly (WASM) build

This directory contains configuration and helpers to facilitate cross
compile CPython to WebAssembly (WASM).

## wasm32-emscripten build

Cross compiling to wasm32-emscripten platform needs [Emscripten](https://emscripten.org/)
tool chain and a build Python interpreter.

### Compile a build Python interpreter

```
mkdir -p builddir/build
pushd builddir/build
../../configure -C
make -j$(nproc)
popd
```

### Fetch and build additional emscripten ports

```
embuilder build zlib
```

### Cross compile to wasm32-emscripten

```
mkdir -p builddir/emscripten
pushd builddir/emscripten

CONFIG_SITE=../../Tools/wasm/config.site-wasm32-emscripten \
  emconfigure ../../configure -C \
    --host=wasm32-unknown-emscripten \
    --build=$(../../config.guess) \
    --with-build-python=$(pwd)/../build/python

emmake make -j$(nproc) python.html
```

### Test in browser

Serve ``python.html`` with a local webserver and open the file in a browser.

```
python3 -m http.server
```
