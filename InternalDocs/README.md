# CPython Internals Documentation

The documentation in this folder is intended for CPython maintainers.
It describes implementation details of CPython, which should not be
assumed to be part of the Python language specification. These details
can change between any two CPython versions and should not be assumed
to hold for other implementations of the Python language.

The core dev team attempts to keep this documentation up to date. If
it is not, please report that through the
[issue tracker](https://github.com/python/cpython/issues).


Compiling Python Source Code
---

- [Guide to the parser](parser.md)

- [Compiler Design](compiler.md)

- [Changing Python's Grammar](changing_grammar.md)

Runtime Objects
---

- [Code Objects](code_objects.md)

- [Generators](generators.md)

- [Frames](frames.md)

- [String Interning](string_interning.md)

Program Execution
---

- [The Bytecode Interpreter](interpreter.md)

- [The JIT](jit.md)

- [Garbage Collector Design](garbage_collector.md)

- [Exception Handling](exception_handling.md)


Modules
---

- [asyncio](asyncio.md)