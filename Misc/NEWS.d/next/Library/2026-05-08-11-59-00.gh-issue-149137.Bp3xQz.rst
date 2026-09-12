Fix :class:`importlib.metadata.PackagePath` to normalize Windows backslash
path separators from RECORD files to forward slashes. Per the packaging
specification, backslashes are valid separators on Windows, but
:class:`~pathlib.PurePosixPath` treats them as literal characters, causing
:attr:`~pathlib.PurePath.name` and :attr:`~pathlib.PurePath.parts` to return
incorrect values.
