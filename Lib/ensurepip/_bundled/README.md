# Upstream packaging

To populate this directory, the initial build packagers are supposed
to invoke the following command:

```console
$ python -m ensurepip.bundle
```

It will download a pre-defined version of the Pip wheel. Its SHA-256
hash is guaranteed to match the one on PyPI.

# Downstream packaging

Packagers of the downstream distributions are welcome to put an
alternative wheel version in the directory defined by the
`WHEEL_PKG_DIR` configuration setting. If this is done,

```console
$ python -m ensurepip
```

will prefer the replacement distribution package over the bundled one.
