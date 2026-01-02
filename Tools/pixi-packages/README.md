# CPython Pixi packages

This directory contains definitions for [Pixi packages](https://pixi.sh/latest/reference/pixi_manifest/#the-package-section)
which can be built from the CPython source code.

Downstream developers can make use of these packages by adding them as Git dependencies in a
[Pixi workspace](https://pixi.sh/latest/first_workspace/), like:

```toml
[dependencies]
python = { git = "https://github.com/python/cpython", subdirectory = "Tools/pixi-packages/asan" }
```

This is particularly useful when developers need to build CPython from source
(for example, for an ASan or TSan-instrumented build), as it does not require any manual
clone or build steps. Instead, Pixi will automatically handle both the build
and installation of the package.

Each package definition is contained in a subdirectory, but they share the build script
`build.sh` in this directory. Currently defined package variants:

- `default`
- `free-threading`
- `asan`: ASan-instrumented build
- `tsan-free-threading`: TSan-instrumented free-threading build

## Maintenance

- Keep the `version` fields in each `recipe.yaml` up to date with the Python version
- Keep the dependency requirements up to date in each `recipe.yaml`
- Update `build.sh` for any breaking changes in the `configure` and `make` workflow

## Opportunities for future improvement

- More package variants (such as UBSan)
- Support for Windows
- Using a single `pixi.toml` and `recipe.yaml` for all package variants is blocked on https://github.com/prefix-dev/pixi/issues/4599
- A workaround can be removed from the build script once https://github.com/prefix-dev/rattler-build/issues/2012 is resolved
