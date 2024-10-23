
# The CPython Repository

This section gives an overview of CPython's code structure and provides
a summary of file locations for modules and built-ins.

Source code layout
==================

The core interpreter implementation is in the [`Python/`](../Python) directory.
Some of the files there are generated during the build process, that will be
indicated by a comment at the top of the file.

For a Python [`module`](https://docs.python.org/3/glossary.html#term-module),
the typical layout is:

* `Lib/{<module>}.py`
* `Modules/_{<module>}.c` (if there's also a C accelerator module)
* `Lib/test/test_{<module>}.py`
* `Doc/library/{<module>}.rst`

For an [`extension module`](https://docs.python.org/3/glossary.html#term-extension-module),
the typical layout is:

* `Modules/{<module>}module.c`
* `Lib/test/test_{<module>}.py`
* `Doc/library/{<module>}.rst`

For [`bltin-types`](https://docs.python.org/3/library/stdtypes.html#bltin-types),
the typical layout is:

* `Objects/{<builtin>}object.c`
* `Lib/test/test_{<builtin>}.py`
* [`Doc/library/stdtypes.rst`](../Doc/library/stdtypes.rst)

For [`built-in-funcs`](https://docs.python.org/3/library/functions.html#built-in-funcs),
the typical layout is:

* [`Python/bltinmodule.c`](../Python/bltinmodule.c)
* [`Lib/test/test_builtin.py`](../Lib/test/test_builtin.py)
* [`Doc/library/functions.rst`](../Doc/library/functions.rst)

Some exceptions to these layouts are:

* built-in type ``int`` is at [`Objects/longobject.c`](../Objects/longobject.c)
* built-in type ``str`` is at [`Objects/unicodeobject.c`](../Objects/unicodeobject.c)
* built-in module ``sys`` is at [`Python/sysmodule.c`](../Python/sysmodule.c)
* built-in module ``marshal`` is at [`Python/marshal.c`](../Python/marshal.c)
* Windows-only module ``winreg`` is at [`PC/winreg.c`](../PC/winreg.c)
