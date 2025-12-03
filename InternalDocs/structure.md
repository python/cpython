# CPython source code

This section gives an overview of CPython's code structure and provides
a summary of file locations for modules and built-ins.


## Source code layout

For a Python module, the typical layout is:

* `Lib/<module>.py`
* `Modules/_<module>.c` (if there's also a C accelerator module)
* `Lib/test/test_<module>.py`
* `Doc/library/<module>.rst`

For an extension module, the typical layout is:

* `Modules/<module>module.c`
* `Lib/test/test_<module>.py`
* `Doc/library/<module>.rst`

For builtin types, the typical layout is:

* `Objects/<builtin>object.c`
* `Lib/test/test_<builtin>.py`
* [`Doc/library/stdtypes.rst`](../Doc/library/stdtypes.rst)

For builtin functions, the typical layout is:

* [`Python/bltinmodule.c`](../Python/bltinmodule.c)
* [`Lib/test/test_builtin.py`](../Lib/test/test_builtin.py)
* [`Doc/library/functions.rst`](../Doc/library/functions.rst)

Some exceptions to these layouts are:

* built-in type `int` is at [`Objects/longobject.c`](../Objects/longobject.c)
* built-in type `str` is at [`Objects/unicodeobject.c`](../Objects/unicodeobject.c)
* built-in module `sys` is at [`Python/sysmodule.c`](../Python/sysmodule.c)
* built-in module `marshal` is at [`Python/marshal.c`](../Python/marshal.c)
* Windows-only module `winreg` is at [`PC/winreg.c`](../PC/winreg.c)


## Additional references

The CPython code base is constantly changing and evolving.
Here's a sample of references about CPython's architecture aimed at
building your understanding of CPython internals and its evolution:


### Current references

| Title                                                                                                                        | Brief                                                | Author           | Version |
|------------------------------------------------------------------------------------------------------------------------------|------------------------------------------------------|------------------|---------|
| [A guide from parser to objects, observed using GDB](https://hackmd.io/s/ByMHBMjFe)                                          | Code walk from Parser, AST, Sym Table and Objects    | Louie Lu         | 3.7.a0  |
| [Green Tree Snakes](https://greentreesnakes.readthedocs.io/en/latest/)                                                       | The missing Python AST docs                          | Thomas Kluyver   | 3.6     |
| [Yet another guided tour of CPython](https://paper.dropbox.com/doc/Yet-another-guided-tour-of-CPython-XY7KgFGn88zMNivGJ4Jzv) | A guide for how CPython REPL works                   | Guido van Rossum | 3.5     |
| [Python Asynchronous I/O Walkthrough](https://www.youtube.com/playlist?list=PLpEcQSRWP2IjVRlTUptdD05kG-UkJynQT)              | How CPython async I/O, generator and coroutine works | Philip Guo       | 3.5     |
| [Coding Patterns for Python Extensions](https://pythonextensionpatterns.readthedocs.io/en/latest/)                           | Reliable patterns of coding Python Extensions in C   | Paul Ross        | 3.9+    |
| [Your Guide to the CPython Source Code](https://realpython.com/cpython-source-code-guide/)                                   | Your Guide to the CPython Source Code                | Anthony Shaw     | 3.8     |


### Historical references

| Title                                                                                                                                                         | Brief                                             | Author          | Version |
|---------------------------------------------------------------------------------------------------------------------------------------------------------------|---------------------------------------------------|-----------------|---------|
| [Python's Innards Series](https://tech.blog.aknin.name/category/my-projects/pythons-innards/)                                                                 | ceval, objects, pystate and miscellaneous topics  | Yaniv Aknin     | 3.1     |
| [Eli Bendersky's Python Internals](https://eli.thegreenplace.net/tag/python-internals)                                                                        | Objects, Symbol tables and miscellaneous topics   | Eli Bendersky   | 3.x     |
| [A guide from parser to objects, observed using Eclipse](https://docs.google.com/document/d/1nzNN1jeNCC_bg1LADCvtTuGKvcyMskV1w8Ad2iLlwoI/)                    | Code walk from Parser, AST, Sym Table and Objects | Prashanth Raghu | 2.7.12  |
| [CPython internals: A ten-hour codewalk through the Python interpreter source code](https://www.youtube.com/playlist?list=PLzV58Zm8FuBL6OAv1Yu6AwXZrnsFbbR0S) | Code walk from source code to generators          | Philip Guo      | 2.7.8   |