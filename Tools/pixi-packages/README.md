# CPython Pixi packages

This directory contains definitions for [Pixi
packages](https://pixi.sh/latest/reference/pixi_manifest/#the-package-section) which can
be built from the CPython source code.

Downstream developers can make use of these packages by adding them as Git dependencies
in a [Pixi workspace](https://pixi.sh/latest/first_workspace/), like:

```toml
[dependencies]
python.git = "https://github.com/python/cpython"
python.subdirectory = "Tools/pixi-packages/asan"
```

This is particularly useful when developers need to build CPython from source
(for example, for an ASan or TSan-instrumented build), as it does not require any manual
clone or build steps. Instead, Pixi will automatically handle both the build
and installation of the package.

Each package definition is contained in a subdirectory, but they share the build script
`build.sh` in this directory. Currently defined package variants:

- `default`
- `freethreading`
- `asan`: ASan-instrumented build
- `tsan-freethreading`: TSan-instrumented free-threading build

## Maintenance

- Keep the `abi_tag` and `version` fields in each `variants.yaml` up to date with the
  Python version
- Update `build.sh` for any breaking changes in the `configure` and `make` workflow

## Opportunities for future improvement

- More package variants (such as UBSan)
- Support for Windows
- Using a single `pixi.toml` and `recipe.yaml` for all package variants is blocked on
  [pixi-build-backends#532](https://github.com/prefix-dev/pixi-build-backends/pull/532)
  and [pixi#5248](https://github.com/prefix-dev/pixi/issues/5248)

## Troubleshooting

TSan builds may crash on Linux with
```
FATAL: ThreadSanitizer: unexpected memory mapping 0x7977bd072000-0x7977bd500000
```
To fix it, try reducing `mmap_rnd_bits`:

```bash
$ sudo sysctl vm.mmap_rnd_bits
vm.mmap_rnd_bits = 32  # too high for TSan
$ sudo sysctl vm.mmap_rnd_bits=28  # reduce it
vm.mmap_rnd_bits = 28
```
