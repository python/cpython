.. _library-intro:

************
Introduction
************

The "Python library" contains several different kinds of components.

It contains data types that would normally be considered part of the "core" of a
language, such as numbers and lists.  For these types, the Python language core
defines the form of literals and places some constraints on their semantics, but
does not fully define the semantics.  (On the other hand, the language core does
define syntactic properties like the spelling and priorities of operators.)

The library also contains built-in functions and exceptions --- objects that can
be used by all Python code without the need of an :keyword:`import` statement.
Some of these are defined by the core language, but many are not essential for
the core semantics and are only described here.

The bulk of the library, however, consists of a collection of modules. There are
many ways to dissect this collection.  Some modules are written in C and built
in to the Python interpreter; others are written in Python and imported in
source form.  Some modules provide interfaces that are highly specific to
Python, like printing a stack trace; some provide interfaces that are specific
to particular operating systems, such as access to specific hardware; others
provide interfaces that are specific to a particular application domain, like
the World Wide Web. Some modules are available in all versions and ports of
Python; others are only available when the underlying system supports or
requires them; yet others are available only when a particular configuration
option was chosen at the time when Python was compiled and installed.

This manual is organized "from the inside out:" it first describes the built-in
functions, data types and exceptions, and finally the modules, grouped in
chapters of related modules.

This means that if you start reading this manual from the start, and skip to the
next chapter when you get bored, you will get a reasonable overview of the
available modules and application areas that are supported by the Python
library.  Of course, you don't *have* to read it like a novel --- you can also
browse the table of contents (in front of the manual), or look for a specific
function, module or term in the index (in the back).  And finally, if you enjoy
learning about random subjects, you choose a random page number (see module
:mod:`random`) and read a section or two.  Regardless of the order in which you
read the sections of this manual, it helps to start with chapter
:ref:`built-in-funcs`, as the remainder of the manual assumes familiarity with
this material.

Let the show begin!


.. _availability:

Notes on availability
=====================

* An "Availability: Unix" note means that this function is commonly found on
  Unix systems.  It does not make any claims about its existence on a specific
  operating system.

* If not separately noted, all functions that claim "Availability: Unix" are
  supported on macOS, which builds on a Unix core.

* If an availability note contains both a minimum Kernel version and a minimum
  libc version, then both conditions must hold. For example a feature with note
  *Availability: Linux >= 3.17 with glibc >= 2.27* requires both Linux 3.17 or
  newer and glibc 2.27 or newer.

.. _wasm-availability:

Notes on WebAssembly platforms
==============================

The WebAssembly platforms ``wasm32-emscripten`` and ``wasm32-wasi`` provide a
subset of POSIX APIs. WASM runtimes and browsers are sandboxed and have
limited access to the host and external resources. Any standard library module
that uses processes, threading, networking (:mod:`socket`), signals, or
other forms of inter-process communication (IPC), is either not available
or may not work as on other Unix-like systems. File I/O, file system, and Unix
permission-related functions are restricted, too. Emscripten does not permit
blocking operations like blocking network I/O and :func:`~time.sleep`.

The properties and behavior of Python on WebAssembly platforms depend on the
`Emscripten`_ or `WASI`_-SDK version, WASM runtimes (browser, NodeJS, `wasmtime`_),
and Python build time flags. WebAssembly, Emscripten, and WASI are evolving.
Some features like networking may be supported in the future.

For Python in the browser, users should consider `Pyodide`_ or `PyScript`_.
PyScript is built on top of Pyodide, which itself is built on top of
CPython and Emscripten. Pyodide provides access to browsers' JavaScript and
DOM APIs as well as limited networking capabilities with JavaScript's
``XMLHttpRequest`` and ``Fetch`` APIs.

.. TODO: update with information from Tools/wasm/README.md

* Process-related APIs are not available or always fail with an error. That
  includes APIs that spawn new processes (:func:`~os.fork`,
  :func:`~os.execve`), wait for processes (:func:`~os.waitpid`), send signals
  (:func:`~os.kill`), or otherwise interact with processes. The
  :mod:`subprocess` is importable but does not work.

* The :mod:`socket` module is available, but is dysfunctional.

* Some functions are stubs that either don't do anything and always return some
  hardcoded value.

* Functions related to file descriptors, file permissions, file ownership, and
  links are limited and don't support some operations.

.. _Emscripten: https://emscripten.org/
.. _WASI: https://wasi.dev/
.. _wasmtime: https://wasmtime.dev/
.. _Pyodide: https://pyodide.org/
.. _PyScript: https://pyscript.net/
