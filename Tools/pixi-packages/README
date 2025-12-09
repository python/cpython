## CPython Pixi Packages

This directory contains definitions for [Pixi packages](https://pixi.sh/latest/reference/pixi_manifest/#the-package-section)
which can be built from the CPython source code.

Downstream developers can make use of these packages by adding them as git dependencies in a
[Pixi workspace](https://pixi.sh/latest/first_workspace/), like:

```toml
[dependencies]
python = { git = "https://github.com/python/cpython", subdirectory = "Tools/pixi-packages/asan" }
```

This is particularly useful when developers need to build CPython from source
(for example, for an ASan-instrumented build), as it does not require any manual
cloning or building steps. Instead, Pixi will automatically handle both the building
and installation of the package.

Each package definition is contained in a subdirectory, but they share the build script
`build.sh` in this directory. Currently defined package variants:

- `default`
- `asan`: ASan-instrumented build with `PYTHON_ASAN=1`

### Maintenance

- the `version` fields in each `recipe.yaml` should be kept up to date with the Python version
- dependency requirements should be kept up to date in each `recipe.yaml`
- `build.sh` should be updated for any breaking changes in the `configure` and `make` workflow

### Opportunities for future improvement

- more package variants (e.g. TSan, UBSan)
- support for Windows
- using a single `pixi.toml` and `recipe.yaml` for all package variants is blocked on https://github.com/prefix-dev/pixi/issues/4599
- a workaround can be removed from the build script once https://github.com/prefix-dev/rattler-build/issues/2012 is resolved
