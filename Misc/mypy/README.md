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
type-checked themselves and used as dependencies.

See `Lib/_pyrepl/mypy.ini` for an example.