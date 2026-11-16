# CPython Pixi packages

This directory contains definitions for [Pixi
packages](https://pixi.sh/latest/reference/pixi_manifest/#the-package-section) which can
be built from the CPython source code.

Downstream developers can make use of these packages by adding them as Git dependencies
in a [Pixi workspace](https://pixi.sh/latest/first_workspace/), like:

```toml
[dependencies]
python.git = "https://github.com/python/cpython"
python.subdirectory = "Tools/pixi-packages"
python.flags = ["asan"]
```

This is particularly useful when developers need to build CPython from source
(for example, for an ASan or TSan-instrumented build), as it does not require any manual
clone or build steps. Instead, Pixi will automatically handle both the build
and installation of the package.

Each package variant carries a 'flag' to enable selection of that variant — see
[the Pixi docs](https://pixi.prefix.dev/latest/concepts/package_specifications/#extras-and-flags)
for details of how to use this. Currently defined package variants:

- `default`
- `freethreading`
- `asan`: ASan-instrumented build
- `tsan_freethreading`: TSan-instrumented free-threading build

## Maintenance

- Keep the `version` field in `variants.yaml` up to date with the
  Python version
- Update `build.sh` for any breaking changes in the `configure` and `make` workflow

## Opportunities for future improvement

- More package variants (such as UBSan)
- Support for Windows

## Troubleshooting

TSan builds may crash on Linux with
```
FATAL: ThreadSanitizer: unexpected memory mapping 0x7977bd072000-0x7977bd500000
```
To fix it, try reducing `mmap_rnd_bits`:

```console
$ sudo sysctl vm.mmap_rnd_bits
vm.mmap_rnd_bits = 32  # too high for TSan
$ sudo sysctl vm.mmap_rnd_bits=28  # reduce it
vm.mmap_rnd_bits = 28
```
