# Mypy path symlinks

This directory stores symlinks to standard library modules and packages
that are fully type-annotated and ready to be used in type checking of
the rest of the stdlib or Tools/ and so on.

Due to most of the standard library being untyped, we prefer not to
point mypy directly at `Lib/` for type checking.  Additionally, mypy
as a tool does not support shadowing typing-related standard libraries
like `types`, `typing`, and `collections.abc`.

So instead, we set `mypy_path` to include this directory,
which only links modules and packages we know are safe to be
type-checked themselves and used as dependencies.  See
`Lib/_pyrepl/mypy.ini` for an example.

At the same time, we don't want symlinks to be checked into the
repository as they would end up being shipped as part of the source
tarballs, which can create compatibility issues with unpacking on
operating systems without symlink support.  Additionally, those symlinks
would have to be part of the SBOM, which we don't want either.

Instead, this directory ships with a `make_symlinks.py` script, which
creates the symlinks when they are needed.  This happens automatically
on GitHub Actions.  You will have to run it manually for a local
type-checking workflow.  Note that `make distclean` removes the symlinks
to ensure that the produced distribution is clean.
