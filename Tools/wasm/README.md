# Python WebAssembly (WASM) build

This directory contains configuration and helpers to facilitate cross
compilation of CPython to WebAssembly (WASM).

## wasm32-emscripten build

Cross compiling to wasm32-emscripten platform needs the [Emscripten](https://emscripten.org/)
tool chain and a build Python interpreter.
All commands below are relative to a repository checkout.

### Compile a build Python interpreter

```shell
mkdir -p builddir/build
pushd builddir/build
../../configure -C
make -j$(nproc)
popd
```

### Fetch and build additional emscripten ports

```shell
embuilder build zlib
```

### Cross compile to wasm32-emscripten

For browser:

```shell
mkdir -p builddir/emscripten
pushd builddir/emscripten

CONFIG_SITE=../../Tools/wasm/config.site-wasm32-emscripten \
  emconfigure ../../configure -C \
    --host=wasm32-unknown-emscripten \
    --build=$(../../config.guess) \
    --with-emscripten-target=browser \
    --with-build-python=$(pwd)/../build/python

emmake make -j$(nproc)
```

For node:

```
CONFIG_SITE=../../Tools/wasm/config.site-wasm32-emscripten \
  emconfigure ../../configure -C \
    --host=wasm32-unknown-emscripten \
    --build=$(../../config.guess) \
    --with-emscripten-target=node \
    --with-build-python=$(pwd)/../build/python

emmake make -j$(nproc)
```

### Test in browser

Serve `python.html` with a local webserver and open the file in a browser.

```shell
emrun python.html
```

or

```shell
python3 -m http.server
```
