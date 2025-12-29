# Mypy path symlinks

This directory stores symlinks to standard library modules and packages
that are fully type-annotated and ready to be used in type checking of
the rest of the stdlib or Tools/ and so on.

## Why this is necessary
Due to most of the standard library being untyped, we prefer not to
point mypy directly at `Lib/` for type checking.  Additionally, mypy
as a tool does not support shadowing typing-related standard libraries
like `types`, `typing`, and `collections.abc`.

So instead, we set `mypy_path` to include this directory,
which only links modules and packages we know are safe to be
type-checked themselves and used as dependencies.  See
`Lib/_pyrepl/mypy.ini` for a usage example.

## I want to add a new type-checked module
Add it to `typed-stdlib.txt` and run `make_symlinks.py --symlink`.

## I don't see any symlinks in this directory
The symlinks in this directory are skipped in source tarballs
in Python releases. This ensures they don't end up in the SBOM. To
recreate them, run the `make_symlinks.py --symlink` script.
