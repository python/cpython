# PC/layout script

This document provides an overview of the `PC/layout` script, which is used to
convert from a build tree to a ready-to-run directory of files. It's the
equivalent of `make install` when using the Makefile. We use it for official
binary releases, but it also supports a lot of configuration and can be used for
whatever your custom scenario should look like.

The script never builds anything. It reads files from an existing build tree
(`-b`) and the source tree (`-s`), then copies or zips them into a layout. It
does not require the Python being laid out to be runnable, and it can target a
different version from the one running the script (see [Build source
options](#build-source-options)).

## Quick start

Run the script as a directory with any Python 3 interpreter. From the
repository root:

```
python PC/layout --help
```

A useful invocation names an output directory and a preset. The build
directory is inferred from the running interpreter (`-b` overrides it), but a
`--preset-<name>` (or explicit `--include-<name>` options) is required;
otherwise the result is a bare, minimal layout that is rarely useful.

```
python PC/layout --copy dist --preset-default
```

### Output options

The script produces one or both of the following outputs. At least one is
required for the run to do anything useful.

* `--copy <dir>` copies the layout into a directory.
* `--zip <file>` writes the layout into a ZIP file.

Additional output-related options:

* `--catalog <file>` writes a `.cdf` catalog definition for the non-binary
  files, used later to produce a signed catalog. Compilation and signing are
  separate steps.
* `--log <file>` writes all operations to a log file.
* `-t`, `--temp <dir>` sets the temporary working directory used for generated
  files (ZIP library, `._pth`, extracted pip, generated JSON). When omitted, a
  temporary directory is created automatically. If `--copy` targets a Dev
  Drive, an adjacent temp directory on the same drive is used for speed.
* `-v` increases verbosity. Repeat for more detail.

### Layout options

Optional components are selected with `--include-<name>` flags. Presets group
these into the configurations we ship (see [Layouts](#layouts)). For a quick
throwaway layout, `-a`/`--include-all` turns on every option (except those
noted below as not affected by `--include-all`); it is intended for debugging
rather than for producing a real distribution, since it mixes components that
are never shipped together. The main component flags are:

* `--include-stable` the stable ABI DLL (`python3.dll`).
* `--include-pip` pip, extracted from the bundled wheels in
  `Lib/ensurepip/_bundled`.
* `--include-pip-user` a `pip.ini` defaulting to `--user` installs.
* `--include-tcltk` Tcl, Tk and tkinter.
* `--include-idle` IDLE. Implies `--include-tcltk`.
* `--include-tests` the test suite.
* `--include-tools` scripts from `Tools`.
* `--include-venv` venv and ensurepip, plus the venv launcher executables.
* `--include-dev` headers and `.lib` files.
* `--include-symbols` `.pdb` symbol files.
* `--include-underpth` a `python<XY>._pth` file. Not affected by
  `--include-all`.
* `--include-launchers` extra launcher executables (used with
  `--include-appxmanifest`).
* `--include-appxmanifest` an APPX/MSIX manifest.
* `--include-props` a `python.props` MSBuild file.
* `--include-nuspec` a `python.nuspec` NuGet spec.
* `--include-chm` the CHM documentation.
* `--include-html-doc` the HTML documentation.
* `--include-freethreaded` free-threaded binaries. Not affected by
  `--include-all`.
* `--include-alias`, `--include-alias3`, `--include-alias3x` aliased
  entry-point executables (`python.exe`, `python3.exe`, `python3.x.exe` and
  their `w` variants).
* `--include-install-json`, `--include-install-embed-json`,
  `--include-install-test-json` a PyManager `__install__.json` for the
  respective distribution.
* `--include-builddetails-json` a [PEP 739](https://peps.python.org/pep-0739/)
  `build-details.json`.

Layout behavior flags:

* `-d`, `--debug` lays out the debug build (files with the `_d` suffix). The
  debug binaries must already have been built (for example with
  `PCbuild/build.bat -d`); the script never builds them.
* `-p`, `--precompile` includes `.pyc` files (at optimization levels 0, 1 and
  2) instead of, or in addition to, `.py` files. The bytecode is compiled
  during layout, in-process, using the `py_compile` module of the interpreter
  running the script. It does not launch the laid-out runtime and does not check
  its version, so it assumes the running interpreter matches the target version;
  compiling with a mismatched version produces `.pyc` files the target cannot
  use.
* `-z`, `--zip-lib` puts the standard library into `python<XY>.zip` rather than
  a `Lib` directory.
* `--flat-dlls` places extension modules and DLLs next to the executables
  rather than in a `DLLs` directory.
* `--include-cat <file>` includes an already-built `.cat` catalog file. This
  only copies the named file into the layout; it does not build or sign the
  catalog and requires no external tooling. Producing the `.cat` from a `.cdf`
  (see `--catalog`) is a separate step.

### Source options

These options tell the script where to read from and what it is laying out.

* `-s`, `--source <dir>` the repository root. Defaults to the repository
  containing the script.
* `-b`, `--build <dir>` the build directory containing the compiled binaries.
  Defaults to the directory containing the running interpreter.
* `--arch <win32|amd64|arm32|arm64>` the target architecture. When omitted, it
  is inferred by reading the PE header of a binary in the build directory.
* `--doc-build <dir>` the documentation build directory. Defaults to
  `<source>/Doc/build`.

The following environment variables are read when set:

* `PYTHON_HEXVERSION` / `PYTHONINCLUDE` override version detection when the
  target version cannot be inferred by reading `Include/patchlevel.h` from the
  source tree. Set one of these when laying out a version other than the one
  running the script (see below).
* `TCL_LIBRARY` locates the Tcl library when `--include-tcltk` is set. If
  unset, the script falls back to `TCL_LIBRARY.env` in the build directory.
* `PYTHON_NUSPEC_VERSION`, `PYTHON_PROPS_PLATFORM` fill in the generated
  `python.nuspec` and `python.props` files. When unset, the version falls back
  to the detected version number and the platform falls back to a value derived
  from the target architecture.
* `APPX_DATA_PUBLISHER`, `APPX_DATA_WINVER`, `APPX_DATA_SHA256` fill in the
  generated APPX manifest. When unset, they fall back to defaults: a placeholder
  publisher certificate name, a `MaxVersionTested` of `10.0.22000.0`, and no
  SCCD hash. These defaults are fine for local use but are not suitable for a
  signed, Store-ready package.

## Layouts

This section lists the common presets, which reflect our supported
configurations. Each is selected with `--preset-<name>` and expands to a fixed
set of `--include-<name>` options. They should help you understand which to use
for your scenario, and they are useful for figuring out where to put a new file
that you've just added to CPython.

A preset can be combined with additional `--include-<name>` options to add
components on top of it; the options are cumulative.

### Development Kit (`--preset-default`)

```
python PC/layout --copy dist --preset-default
```

The development kit layout. This is the closest equivalent to a full install
and contains the interpreter, the standard library in `Lib`, extension modules
in `DLLs`, headers and import libraries (`--include-dev`), symbols, pip, tcltk,
IDLE, venv, the test suite, HTML docs, the stable ABI DLL, aliases and a
`build-details.json`.

### Python install manager (`--preset-pymanager`, `--preset-pymanager-test`)

Layouts for the [Python install manager](https://github.com/python/pymanager).
This is now the primary way upstream CPython is distributed on Windows. Both
resemble the development kit but add a PyManager `__install__.json`.

`pymanager-test` additionally includes the test suite and symbols;
`--preset-pymanager-test` is what produces the `py install PythonTest\`
packages.

### Embeddable package (`--preset-embed`)

```
python PC/layout --zip embed.zip --preset-embed
```

The embeddable package: a minimal, self-contained runtime intended to be
bundled inside another application. The standard library is placed in a ZIP
(`--zip-lib`), DLLs are flattened next to the executables (`--flat-dlls`), the
library is precompiled, and a `._pth` file (`--underpth`) is included to fix the
search path and isolate the runtime from user site directories. It omits pip,
tcltk, IDLE, the tests and development files.

`--preset-embed --include-install-embed-json` is what produces the
`py install PythonEmbed\` packages.

### NuGet package (`--preset-nuget`)

The NuGet package layout. Includes development files, pip, the stable ABI,
venv, aliases, a `build-details.json`, and the `python.props` and
`python.nuspec` files that describe the package to MSBuild and NuGet.

### APPX package (`--preset-appx`)

The APPX/MSIX package layout for the Windows Store distribution. Includes the
stable ABI, pip, tcltk, IDLE, venv, development files, launchers, aliases and an
APPX manifest. This is no longer used for upstream CPython releases.

### Windows IoT Core (`--preset-iot`)

A minimal layout for Windows IoT Core: aliases, the stable ABI and pip. This is
not used for upstream CPython releases.

## Build source options

The `-b` and `-s` options let you lay out a build that is separate from the
interpreter running the script, including a different Python version.

* `-b`/`--build` points at the build tree with the compiled binaries. It does
  not need to match the interpreter running the script.
* `-s`/`--source` points at the source tree that supplies `Lib`, `Include`,
  `Tools` and version information.

The target version is normally read from `Include/patchlevel.h` in the source
tree. When laying out a version that differs from the running interpreter and
the source tree is not the matching one, set `PYTHON_HEXVERSION` (a hex version
such as `0x030f00a0`) or `PYTHONINCLUDE` (a directory containing
`patchlevel.h`) so the correct DLL names, suffixes and version numbers are used.
The script validates the inferred version against `patchlevel.h` and stops with
an error if they disagree.

## Tool structure

The script is a package under `PC/layout`. `main.py` drives argument parsing,
builds the file list (`get_layout`), generates derived files
(`generate_source_files`) and copies or zips the result (`copy_files`). Support
modules live in `PC/layout/support`:

* `options.py` defines the `OPTIONS` and `PRESETS` tables and generates the
  `--include-*` and `--preset-*` command-line arguments. Add new options and
  presets here.
* `constants.py` derives version numbers, DLL names and related constants from
  `patchlevel.h` or the version environment variables.
* `filesets.py` provides the file-matching helpers (`FileStemSet`,
  `FileNameSet`, `FileSuffixSet`) and the `rglob` globbing helper used
  throughout `main.py`.
* `arch.py` infers the target architecture from a binary's PE header.
* `logging.py` configures logging and tracks whether errors occurred.
* `pip.py` extracts pip from the bundled wheels and lists the pip files.
* `catalog.py` generates the `.cdf`/`.cat` catalog definitions for signing.
* `props.py` generates `python.props` (NuGet/MSBuild).
* `nuspec.py` generates `python.nuspec` (NuGet).
* `appxmanifest.py` generates APPX/MSIX manifests.
* `pymanager.py` generates the PyManager `__install__.json`.
* `builddetails.py` generates the PEP 739 `build-details.json`.

### Classifying new files

`main.py` collects files by globbing shared directories -- the build output
directory (`-b`) for binaries and the source `Lib`, `Include` and `Tools`
directories -- and then filters them. A new file added to any of these is
usually picked up with no change here: build extension modules (`*.pyd`) land in
`DLLs` (or alongside the executables under `--flat-dlls`), other build DLLs land
next to them, and files under `Lib` are mirrored into the layout's `Lib`.

Changes are only needed when a new file belongs to an optional component, so it
must be routed to the right `--include-<name>` option rather than always
copied. The routing is driven by a set of file-matching tables near the top of
`main.py` (built from the `FileStemSet`, `FileNameSet` and `FileSuffixSet`
helpers in `support/filesets.py`). To classify a new file, add its name or
pattern to the matching table.

Tables that gate files on an optional component (the file is only included when
the corresponding option is set):

* `TEST_PYDS_ONLY`, `TEST_DLLS_ONLY`, `TEST_DIRS_ONLY` -- test-only extension
  modules, DLLs and `Lib` directories; gated on `--include-tests`. Extension
  module names starting with `_test` are already matched; add any other
  test-only build module here (see also the note in `PCbuild/readme.txt`).
* `TCLTK_PYDS_ONLY`, `TCLTK_DLLS_ONLY`, `TCLTK_DIRS_ONLY`, `TCLTK_FILES_ONLY` --
  Tcl/Tk/tkinter extension modules (`_tkinter`), DLLs (`tcl*`, `tk*`, `zlib1`),
  `Lib` directories (`tkinter`, `turtledemo`) and files (`turtle.py`); gated on
  `--include-tcltk`.
* `IDLE_DIRS_ONLY` -- IDLE's `Lib` directory (`idlelib`); gated on
  `--include-idle`.
* `VENV_DIRS_ONLY` -- `venv` and `ensurepip` `Lib` directories; gated on
  `--include-venv`.

Tables that exclude files outright:

* `EXCLUDE_FROM_DLLS` -- build DLLs that are handled elsewhere and must not be
  copied by the generic DLL glob (`python*`, `pyshellext`, `vcruntime*`).
* `EXCLUDE_FROM_LIB` -- never copied into `Lib` (`*.pyc`, `__pycache__`,
  `*.pickle`).
* `EXCLUDE_FROM_COMPILE` -- `Lib` files that must not be precompiled
  (`badsyntax_*`, `bad_*`).
* `EXCLUDE_FROM_CATALOG` -- suffixes omitted from the signing catalog because
  they are signed separately (`.exe`, `.pyd`, `.dll`).

Special cases in the debug and binary handling:

* `REQUIRED_DLLS` (`libcrypto*`, `libssl*`, `libffi*`) -- always copied from the
  build directory even for a release layout. Most build DLLs are filtered by
  their `_d` suffix to match the debug or release build being laid out; these
  dependencies do not follow that naming and would otherwise be dropped, so they
  are matched here to force inclusion. Add a new external dependency DLL here if
  it does not use the `_d` debug-suffix convention.

When deciding where a new file goes, match it to the component it belongs to and
add it to the corresponding table. If it belongs in every layout and follows the
normal conventions, no change is needed.


## Contributing guidelines

Contributions are welcome. The script supports a broad range of scenarios, and
extending it for a new one is encouraged. Because the same code produces the
official releases across every supported version and architecture, additions
have to respect a few constraints so that existing configurations keep working.

The script runs in an unusual context, and new code must hold to the following
assumptions:

* Do not assume the Python being laid out can be launched. The script must run
  correctly using only the interpreter running it, which may differ from the
  target.
* Do not assume the layout version matches the version running the script.
  Derive version-dependent behavior from the detected version rather than from
  `sys.version` of the running interpreter. (Precompilation with `--precompile`
  is the one deliberate exception, and it assumes a matching interpreter.)
* The script may be used to lay out an older version, so behavior changes have
  to be preserved, either behind an option or gated on a version check, rather
  than replaced outright.
* The script never builds anything. Everything it emits must already exist in
  the build or source tree.

When adding features, keep the surface area aligned with upstream:

* New `--include-<name>` options for further customization are fine.
* Do not add presets beyond those used by upstream. Presets describe the
  configurations we ship, so a new one implies a new supported release
  artifact.
* Route new files through the classification tables (see [Classifying new
  files](#classifying-new-files)) rather than adding one-off special cases.
