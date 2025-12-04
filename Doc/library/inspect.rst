:mod:`!inspect` --- Inspect live objects
========================================

.. testsetup:: *

   import inspect
   from inspect import *

.. module:: inspect
   :synopsis: Extract information and source code from live objects.

.. moduleauthor:: Ka-Ping Yee <ping@lfw.org>
.. sectionauthor:: Ka-Ping Yee <ping@lfw.org>

**Source code:** :source:`Lib/inspect.py`

--------------

The :mod:`inspect` module provides several useful functions to help get
information about live objects such as modules, classes, methods, functions,
tracebacks, frame objects, and code objects.  For example, it can help you
examine the contents of a class, retrieve the source code of a method, extract
and format the argument list for a function, or get all the information you need
to display a detailed traceback.

There are four main kinds of services provided by this module: type checking,
getting source code, inspecting classes and functions, and examining the
interpreter stack.


.. _inspect-types:

Types and members
-----------------

The :func:`getmembers` function retrieves the members of an object such as a
class or module. The functions whose names begin with "is" are mainly
provided as convenient choices for the second argument to :func:`getmembers`.
They also help you determine when you can expect to find the following special
attributes (see :ref:`import-mod-attrs` for module attributes):

.. this function name is too big to fit in the ascii-art table below
.. |coroutine-origin-link| replace:: :func:`sys.set_coroutine_origin_tracking_depth`

+-----------------+-------------------+---------------------------+
| Type            | Attribute         | Description               |
+=================+===================+===========================+
| class           | __doc__           | documentation string      |
+-----------------+-------------------+---------------------------+
|                 | __name__          | name with which this      |
|                 |                   | class was defined         |
+-----------------+-------------------+---------------------------+
|                 | __qualname__      | qualified name            |
+-----------------+-------------------+---------------------------+
|                 | __module__        | name of module in which   |
|                 |                   | this class was defined    |
+-----------------+-------------------+---------------------------+
|                 | __type_params__   | A tuple containing the    |
|                 |                   | :ref:`type parameters     |
|                 |                   | <type-params>` of         |
|                 |                   | a generic class           |
+-----------------+-------------------+---------------------------+
| method          | __doc__           | documentation string      |
+-----------------+-------------------+---------------------------+
|                 | __name__          | name with which this      |
|                 |                   | method was defined        |
+-----------------+-------------------+---------------------------+
|                 | __qualname__      | qualified name            |
+-----------------+-------------------+---------------------------+
|                 | __func__          | function object           |
|                 |                   | containing implementation |
|                 |                   | of method                 |
+-----------------+-------------------+---------------------------+
|                 | __self__          | instance to which this    |
|                 |                   | method is bound, or       |
|                 |                   | ``None``                  |
+-----------------+-------------------+---------------------------+
|                 | __module__        | name of module in which   |
|                 |                   | this method was defined   |
+-----------------+-------------------+---------------------------+
| function        | __doc__           | documentation string      |
+-----------------+-------------------+---------------------------+
|                 | __name__          | name with which this      |
|                 |                   | function was defined      |
+-----------------+-------------------+---------------------------+
|                 | __qualname__      | qualified name            |
+-----------------+-------------------+---------------------------+
|                 | __code__          | code object containing    |
|                 |                   | compiled function         |
|                 |                   | :term:`bytecode`          |
+-----------------+-------------------+---------------------------+
|                 | __defaults__      | tuple of any default      |
|                 |                   | values for positional or  |
|                 |                   | keyword parameters        |
+-----------------+-------------------+---------------------------+
|                 | __kwdefaults__    | mapping of any default    |
|                 |                   | values for keyword-only   |
|                 |                   | parameters                |
+-----------------+-------------------+---------------------------+
|                 | __globals__       | global namespace in which |
|                 |                   | this function was defined |
+-----------------+-------------------+---------------------------+
|                 | __builtins__      | builtins namespace        |
+-----------------+-------------------+---------------------------+
|                 | __annotations__   | mapping of parameters     |
|                 |                   | names to annotations;     |
|                 |                   | ``"return"`` key is       |
|                 |                   | reserved for return       |
|                 |                   | annotations.              |
+-----------------+-------------------+---------------------------+
|                 | __type_params__   | A tuple containing the    |
|                 |                   | :ref:`type parameters     |
|                 |                   | <type-params>` of         |
|                 |                   | a generic function        |
+-----------------+-------------------+---------------------------+
|                 | __module__        | name of module in which   |
|                 |                   | this function was defined |
+-----------------+-------------------+---------------------------+
| traceback       | tb_frame          | frame object at this      |
|                 |                   | level                     |
+-----------------+-------------------+---------------------------+
|                 | tb_lasti          | index of last attempted   |
|                 |                   | instruction in bytecode   |
+-----------------+-------------------+---------------------------+
|                 | tb_lineno         | current line number in    |
|                 |                   | Python source code        |
+-----------------+-------------------+---------------------------+
|                 | tb_next           | next inner traceback      |
|                 |                   | object (called by this    |
|                 |                   | level)                    |
+-----------------+-------------------+---------------------------+
| frame           | f_back            | next outer frame object   |
|                 |                   | (this frame's caller)     |
+-----------------+-------------------+---------------------------+
|                 | f_builtins        | builtins namespace seen   |
|                 |                   | by this frame             |
+-----------------+-------------------+---------------------------+
|                 | f_code            | code object being         |
|                 |                   | executed in this frame    |
+-----------------+-------------------+---------------------------+
|                 | f_globals         | global namespace seen by  |
|                 |                   | this frame                |
+-----------------+-------------------+---------------------------+
|                 | f_lasti           | index of last attempted   |
|                 |                   | instruction in bytecode   |
+-----------------+-------------------+---------------------------+
|                 | f_lineno          | current line number in    |
|                 |                   | Python source code        |
+-----------------+-------------------+---------------------------+
|                 | f_locals          | local namespace seen by   |
|                 |                   | this frame                |
+-----------------+-------------------+---------------------------+
|                 | f_generator       | returns the generator or  |
|                 |                   | coroutine object that     |
|                 |                   | owns this frame, or       |
|                 |                   | ``None`` if the frame is  |
|                 |                   | of a regular function     |
+-----------------+-------------------+---------------------------+
|                 | f_trace           | tracing function for this |
|                 |                   | frame, or ``None``        |
+-----------------+-------------------+---------------------------+
|                 | f_trace_lines     | indicate whether a        |
|                 |                   | tracing event is          |
|                 |                   | triggered for each source |
|                 |                   | source line               |
+-----------------+-------------------+---------------------------+
|                 | f_trace_opcodes   | indicate whether          |
|                 |                   | per-opcode events are     |
|                 |                   | requested                 |
+-----------------+-------------------+---------------------------+
|                 | clear()           | used to clear all         |
|                 |                   | references to local       |
|                 |                   | variables                 |
+-----------------+-------------------+---------------------------+
| code            | co_argcount       | number of arguments (not  |
|                 |                   | including keyword only    |
|                 |                   | arguments, \* or \*\*     |
|                 |                   | args)                     |
+-----------------+-------------------+---------------------------+
|                 | co_code           | string of raw compiled    |
|                 |                   | bytecode                  |
+-----------------+-------------------+---------------------------+
|                 | co_cellvars       | tuple of names of cell    |
|                 |                   | variables (referenced by  |
|                 |                   | containing scopes)        |
+-----------------+-------------------+---------------------------+
|                 | co_consts         | tuple of constants used   |
|                 |                   | in the bytecode           |
+-----------------+-------------------+---------------------------+
|                 | co_filename       | name of file in which     |
|                 |                   | this code object was      |
|                 |                   | created                   |
+-----------------+-------------------+---------------------------+
|                 | co_firstlineno    | number of first line in   |
|                 |                   | Python source code        |
+-----------------+-------------------+---------------------------+
|                 | co_flags          | bitmap of ``CO_*`` flags, |
|                 |                   | read more :ref:`here      |
|                 |                   | <inspect-module-co-flags>`|
+-----------------+-------------------+---------------------------+
|                 | co_lnotab         | encoded mapping of line   |
|                 |                   | numbers to bytecode       |
|                 |                   | indices                   |
+-----------------+-------------------+---------------------------+
|                 | co_freevars       | tuple of names of free    |
|                 |                   | variables (referenced via |
|                 |                   | a function's closure)     |
+-----------------+-------------------+---------------------------+
|                 | co_posonlyargcount| number of positional only |
|                 |                   | arguments                 |
+-----------------+-------------------+---------------------------+
|                 | co_kwonlyargcount | number of keyword only    |
|                 |                   | arguments (not including  |
|                 |                   | \*\* arg)                 |
+-----------------+-------------------+---------------------------+
|                 | co_name           | name with which this code |
|                 |                   | object was defined        |
+-----------------+-------------------+---------------------------+
|                 | co_qualname       | fully qualified name with |
|                 |                   | which this code object    |
|                 |                   | was defined               |
+-----------------+-------------------+---------------------------+
|                 | co_names          | tuple of names other      |
|                 |                   | than arguments and        |
|                 |                   | function locals           |
+-----------------+-------------------+---------------------------+
|                 | co_nlocals        | number of local variables |
+-----------------+-------------------+---------------------------+
|                 | co_stacksize      | virtual machine stack     |
|                 |                   | space required            |
+-----------------+-------------------+---------------------------+
|                 | co_varnames       | tuple of names of         |
|                 |                   | arguments and local       |
|                 |                   | variables                 |
+-----------------+-------------------+---------------------------+
|                 | co_lines()        | returns an iterator that  |
|                 |                   | yields successive         |
|                 |                   | bytecode ranges           |
+-----------------+-------------------+---------------------------+
|                 | co_positions()    | returns an iterator of    |
|                 |                   | source code positions for |
|                 |                   | each bytecode instruction |
+-----------------+-------------------+---------------------------+
|                 | replace()         | returns a copy of the     |
|                 |                   | code object with new      |
|                 |                   | values                    |
+-----------------+-------------------+---------------------------+
| generator       | __name__          | name                      |
+-----------------+-------------------+---------------------------+
|                 | __qualname__      | qualified name            |
+-----------------+-------------------+---------------------------+
|                 | gi_frame          | frame                     |
+-----------------+-------------------+---------------------------+
|                 | gi_running        | is the generator running? |
+-----------------+-------------------+---------------------------+
|                 | gi_suspended      | is the generator          |
|                 |                   | suspended?                |
+-----------------+-------------------+---------------------------+
|                 | gi_code           | code                      |
+-----------------+-------------------+---------------------------+
|                 | gi_yieldfrom      | object being iterated by  |
|                 |                   | ``yield from``, or        |
|                 |                   | ``None``                  |
+-----------------+-------------------+---------------------------+
| async generator | __name__          | name                      |
+-----------------+-------------------+---------------------------+
|                 | __qualname__      | qualified name            |
+-----------------+-------------------+---------------------------+
|                 | ag_await          | object being awaited on,  |
|                 |                   | or ``None``               |
+-----------------+-------------------+---------------------------+
|                 | ag_frame          | frame                     |
+-----------------+-------------------+---------------------------+
|                 | ag_running        | is the generator running? |
+-----------------+-------------------+---------------------------+
|                 | ag_code           | code                      |
+-----------------+-------------------+---------------------------+
| coroutine       | __name__          | name                      |
+-----------------+-------------------+---------------------------+
|                 | __qualname__      | qualified name            |
+-----------------+-------------------+---------------------------+
|                 | cr_await          | object being awaited on,  |
|                 |                   | or ``None``               |
+-----------------+-------------------+---------------------------+
|                 | cr_frame          | frame                     |
+-----------------+-------------------+---------------------------+
|                 | cr_running        | is the coroutine running? |
+-----------------+-------------------+---------------------------+
|                 | cr_code           | code                      |
+-----------------+-------------------+---------------------------+
|                 | cr_origin         | where coroutine was       |
|                 |                   | created, or ``None``. See |
|                 |                   | |coroutine-origin-link|   |
+-----------------+-------------------+---------------------------+
| builtin         | __doc__           | documentation string      |
+-----------------+-------------------+---------------------------+
|                 | __name__          | original name of this     |
|                 |                   | function or method        |
+-----------------+-------------------+---------------------------+
|                 | __qualname__      | qualified name            |
+-----------------+-------------------+---------------------------+
|                 | __self__          | instance to which a       |
|                 |                   | method is bound, or       |
|                 |                   | ``None``                  |
+-----------------+-------------------+---------------------------+

.. versionchanged:: 3.5

   Add ``__qualname__`` and ``gi_yieldfrom`` attributes to generators.

   The ``__name__`` attribute of generators is now set from the function
   name, instead of the code name, and it can now be modified.

.. versionchanged:: 3.7

   Add ``cr_origin`` attribute to coroutines.

.. versionchanged:: 3.10

   Add ``__builtins__`` attribute to functions.

.. versionchanged:: 3.14

   Add ``f_generator`` attribute to frames.

.. function:: getmembers(object[, predicate])

   Return all the members of an object in a list of ``(name, value)``
   pairs sorted by name. If the optional *predicate* argument—which will be
   called with the ``value`` object of each member—is supplied, only members
   for which the predicate returns a true value are included.

   .. note::

      :func:`getmembers` will only return class attributes defined in the
      metaclass when the argument is a class and those attributes have been
      listed in the metaclass' custom :meth:`~object.__dir__`.


.. function:: getmembers_static(object[, predicate])

    Return all the members of an object in a list of ``(name, value)``
    pairs sorted by name without triggering dynamic lookup via the descriptor
    protocol, __getattr__ or __getattribute__. Optionally, only return members
    that satisfy a given predicate.

    .. note::

        :func:`getmembers_static` may not be able to retrieve all members
        that getmembers can fetch (like dynamically created attributes)
        and may find members that getmembers can't (like descriptors
        that raise AttributeError). It can also return descriptor objects
        instead of instance members in some cases.

    .. versionadded:: 3.11


.. function:: getmodulename(path)

   Return the name of the module named by the file *path*, without including the
   names of enclosing packages. The file extension is checked against all of
   the entries in :func:`importlib.machinery.all_suffixes`. If it matches,
   the final path component is returned with the extension removed.
   Otherwise, ``None`` is returned.

   Note that this function *only* returns a meaningful name for actual
   Python modules - paths that potentially refer to Python packages will
   still return ``None``.

   .. versionchanged:: 3.3
      The function is based directly on :mod:`importlib`.


.. function:: ismodule(object)

   Return ``True`` if the object is a module.


.. function:: isclass(object)

   Return ``True`` if the object is a class, whether built-in or created in Python
   code.


.. function:: ismethod(object)

   Return ``True`` if the object is a bound method written in Python.


.. function:: ispackage(object)

   Return ``True`` if the object is a :term:`package`.

   .. versionadded:: 3.14


.. function:: isfunction(object)

   Return ``True`` if the object is a Python function, which includes functions
   created by a :term:`lambda` expression.


.. function:: isgeneratorfunction(object)

   Return ``True`` if the object is a Python generator function.

   .. versionchanged:: 3.8
      Functions wrapped in :func:`functools.partial` now return ``True`` if the
      wrapped function is a Python generator function.

   .. versionchanged:: 3.13
      Functions wrapped in :func:`functools.partialmethod` now return ``True``
      if the wrapped function is a Python generator function.

.. function:: isgenerator(object)

   Return ``True`` if the object is a generator.


.. function:: iscoroutinefunction(object)

   Return ``True`` if the object is a :term:`coroutine function` (a function
   defined with an :keyword:`async def` syntax), a :func:`functools.partial`
   wrapping a :term:`coroutine function`, or a sync function marked with
   :func:`markcoroutinefunction`.

   .. versionadded:: 3.5

   .. versionchanged:: 3.8
      Functions wrapped in :func:`functools.partial` now return ``True`` if the
      wrapped function is a :term:`coroutine function`.

   .. versionchanged:: 3.12
      Sync functions marked with :func:`markcoroutinefunction` now return
      ``True``.

   .. versionchanged:: 3.13
      Functions wrapped in :func:`functools.partialmethod` now return ``True``
      if the wrapped function is a :term:`coroutine function`.


.. function:: markcoroutinefunction(func)

   Decorator to mark a callable as a :term:`coroutine function` if it would not
   otherwise be detected by :func:`iscoroutinefunction`.

   This may be of use for sync functions that return a :term:`coroutine`, if
   the function is passed to an API that requires :func:`iscoroutinefunction`.

   When possible, using an :keyword:`async def` function is preferred. Also
   acceptable is calling the function and testing the return with
   :func:`iscoroutine`.

   .. versionadded:: 3.12


.. function:: iscoroutine(object)

   Return ``True`` if the object is a :term:`coroutine` created by an
   :keyword:`async def` function.

   .. versionadded:: 3.5


.. function:: isawaitable(object)

   Return ``True`` if the object can be used in :keyword:`await` expression.

   Can also be used to distinguish generator-based coroutines from regular
   generators:

   .. testcode::

      import types

      def gen():
          yield
      @types.coroutine
      def gen_coro():
          yield

      assert not isawaitable(gen())
      assert isawaitable(gen_coro())

   .. versionadded:: 3.5


.. function:: isasyncgenfunction(object)

   Return ``True`` if the object is an :term:`asynchronous generator` function,
   for example:

   .. doctest::

      >>> async def agen():
      ...     yield 1
      ...
      >>> inspect.isasyncgenfunction(agen)
      True

   .. versionadded:: 3.6

   .. versionchanged:: 3.8
      Functions wrapped in :func:`functools.partial` now return ``True`` if the
      wrapped function is an :term:`asynchronous generator` function.

   .. versionchanged:: 3.13
      Functions wrapped in :func:`functools.partialmethod` now return ``True``
      if the wrapped function is a :term:`coroutine function`.

.. function:: isasyncgen(object)

   Return ``True`` if the object is an :term:`asynchronous generator iterator`
   created by an :term:`asynchronous generator` function.

   .. versionadded:: 3.6

.. function:: istraceback(object)

   Return ``True`` if the object is a traceback.


.. function:: isframe(object)

   Return ``True`` if the object is a frame.


.. function:: iscode(object)

   Return ``True`` if the object is a code.


.. function:: isbuiltin(object)

   Return ``True`` if the object is a built-in function or a bound built-in method.


.. function:: ismethodwrapper(object)

   Return ``True`` if the type of object is a :class:`~types.MethodWrapperType`.

   These are instances of :class:`~types.MethodWrapperType`, such as :meth:`~object.__str__`,
   :meth:`~object.__eq__` and :meth:`~object.__repr__`.

   .. versionadded:: 3.11


.. function:: isroutine(object)

   Return ``True`` if the object is a user-defined or built-in function or method.


.. function:: isabstract(object)

   Return ``True`` if the object is an abstract base class.


.. function:: ismethoddescriptor(object)

   Return ``True`` if the object is a method descriptor, but not if
   :func:`ismethod`, :func:`isclass`, :func:`isfunction` or :func:`isbuiltin`
   are true.

   This, for example, is true of ``int.__add__``.  An object passing this test
   has a :meth:`~object.__get__` method, but not a :meth:`~object.__set__`
   method or a :meth:`~object.__delete__` method.  Beyond that, the set of
   attributes varies.  A :attr:`~definition.__name__` attribute is usually
   sensible, and :attr:`~definition.__doc__` often is.

   Methods implemented via descriptors that also pass one of the other tests
   return ``False`` from the :func:`ismethoddescriptor` test, simply because the
   other tests promise more -- you can, e.g., count on having the
   :attr:`~method.__func__` attribute (etc) when an object passes
   :func:`ismethod`.

   .. versionchanged:: 3.13
      This function no longer incorrectly reports objects with :meth:`~object.__get__`
      and :meth:`~object.__delete__`, but not :meth:`~object.__set__`, as being method
      descriptors (such objects are data descriptors, not method descriptors).


.. function:: isdatadescriptor(object)

   Return ``True`` if the object is a data descriptor.

   Data descriptors have a :attr:`~object.__set__` or a :attr:`~object.__delete__` method.
   Examples are properties (defined in Python), getsets, and members.  The
   latter two are defined in C and there are more specific tests available for
   those types, which is robust across Python implementations.  Typically, data
   descriptors will also have :attr:`~definition.__name__` and :attr:`!__doc__` attributes
   (properties, getsets, and members have both of these attributes), but this is
   not guaranteed.


.. function:: isgetsetdescriptor(object)

   Return ``True`` if the object is a getset descriptor.

   .. impl-detail::

      getsets are attributes defined in extension modules via
      :c:type:`PyGetSetDef` structures.  For Python implementations without such
      types, this method will always return ``False``.


.. function:: ismemberdescriptor(object)

   Return ``True`` if the object is a member descriptor.

   .. impl-detail::

      Member descriptors are attributes defined in extension modules via
      :c:type:`PyMemberDef` structures.  For Python implementations without such
      types, this method will always return ``False``.


.. _inspect-source:

Retrieving source code
----------------------

.. function:: getdoc(object, *, inherit_class_doc=True, fallback_to_class_doc=True)

   Get the documentation string for an object, cleaned up with :func:`cleandoc`.
   If the documentation string for an object is not provided:

   * if the object is a class and *inherit_class_doc* is true (by default),
     retrieve the documentation string from the inheritance hierarchy;
   * if the object is a method, a property or a descriptor, retrieve
     the documentation string from the inheritance hierarchy;
   * otherwise, if *fallback_to_class_doc* is true (by default), retrieve
     the documentation string from the class of the object.

   Return ``None`` if the documentation string is invalid or missing.

   .. versionchanged:: 3.5
      Documentation strings are now inherited if not overridden.

   .. versionchanged:: 3.15
      Added parameters *inherit_class_doc* and *fallback_to_class_doc*.

      Documentation strings on :class:`~functools.cached_property`
      objects are now inherited if not overriden.


.. function:: getcomments(object)

   Return in a single string any lines of comments immediately preceding the
   object's source code (for a class, function, or method), or at the top of the
   Python source file (if the object is a module).  If the object's source code
   is unavailable, return ``None``.  This could happen if the object has been
   defined in C or the interactive shell.


.. function:: getfile(object)

   Return the name of the (text or binary) file in which an object was defined.
   This will fail with a :exc:`TypeError` if the object is a built-in module,
   class, or function.


.. function:: getmodule(object)

   Try to guess which module an object was defined in. Return ``None``
   if the module cannot be determined.


.. function:: getsourcefile(object)

   Return the name of the Python source file in which an object was defined
   or ``None`` if no way can be identified to get the source.  This
   will fail with a :exc:`TypeError` if the object is a built-in module, class, or
   function.


.. function:: getsourcelines(object)

   Return a list of source lines and starting line number for an object. The
   argument may be a module, class, method, function, traceback, frame, or code
   object.  The source code is returned as a list of the lines corresponding to the
   object and the line number indicates where in the original source file the first
   line of code was found.  An :exc:`OSError` is raised if the source code cannot
   be retrieved.
   A :exc:`TypeError` is raised if the object is a built-in module, class, or
   function.

   .. versionchanged:: 3.3
      :exc:`OSError` is raised instead of :exc:`IOError`, now an alias of the
      former.


.. function:: getsource(object)

   Return the text of the source code for an object. The argument may be a module,
   class, method, function, traceback, frame, or code object.  The source code is
   returned as a single string.  An :exc:`OSError` is raised if the source code
   cannot be retrieved.
   A :exc:`TypeError` is raised if the object is a built-in module, class, or
   function.

   .. versionchanged:: 3.3
      :exc:`OSError` is raised instead of :exc:`IOError`, now an alias of the
      former.


.. function:: cleandoc(doc)

   Clean up indentation from docstrings that are indented to line up with blocks
   of code.

   All leading whitespace is removed from the first line.  Any leading whitespace
   that can be uniformly removed from the second line onwards is removed.  Empty
   lines at the beginning and end are subsequently removed.  Also, all tabs are
   expanded to spaces.


.. _inspect-signature-object:

Introspecting callables with the Signature object
-------------------------------------------------

.. versionadded:: 3.3

The :class:`Signature` object represents the call signature of a callable object
and its return annotation. To retrieve a :class:`!Signature` object,
use the :func:`!signature`
function.

.. function:: signature(callable, *, follow_wrapped=True, globals=None, locals=None, eval_str=False, annotation_format=Format.VALUE)

   Return a :class:`Signature` object for the given *callable*:

   .. doctest::

      >>> from inspect import signature
      >>> def foo(a, *, b:int, **kwargs):
      ...     pass

      >>> sig = signature(foo)

      >>> str(sig)
      '(a, *, b: int, **kwargs)'

      >>> str(sig.parameters['b'])
      'b: int'

      >>> sig.parameters['b'].annotation
      <class 'int'>

   Accepts a wide range of Python callables, from plain functions and classes to
   :func:`functools.partial` objects.

   If some of the annotations are strings (e.g., because
   ``from __future__ import annotations`` was used), :func:`signature` will
   attempt to automatically un-stringize the annotations using
   :func:`annotationlib.get_annotations`.  The
   *globals*, *locals*, and *eval_str* parameters are passed
   into :func:`!annotationlib.get_annotations` when resolving the
   annotations; see the documentation for :func:`!annotationlib.get_annotations`
   for instructions on how to use these parameters. A member of the
   :class:`annotationlib.Format` enum can be passed to the
   *annotation_format* parameter to control the format of the returned
   annotations. For example, use
   ``annotation_format=annotationlib.Format.STRING`` to return annotations in string
   format.

   Raises :exc:`ValueError` if no signature can be provided, and
   :exc:`TypeError` if that type of object is not supported.  Also,
   if the annotations are stringized, and *eval_str* is not false,
   the ``eval()`` call(s) to un-stringize the annotations in :func:`annotationlib.get_annotations`
   could potentially raise any kind of exception.

   A slash (/) in the signature of a function denotes that the parameters prior
   to it are positional-only. For more info, see
   :ref:`the FAQ entry on positional-only parameters <faq-positional-only-arguments>`.

   .. versionchanged:: 3.5
      The *follow_wrapped* parameter was added.
      Pass ``False`` to get a signature of
      *callable* specifically (``callable.__wrapped__`` will not be used to
      unwrap decorated callables.)

   .. versionchanged:: 3.10
      The *globals*, *locals*, and *eval_str* parameters were added.

   .. versionchanged:: 3.14
      The *annotation_format* parameter was added.

   .. note::

      Some callables may not be introspectable in certain implementations of
      Python.  For example, in CPython, some built-in functions defined in
      C provide no metadata about their arguments.

   .. impl-detail::

      If the passed object has a :attr:`!__signature__` attribute,
      we may use it to create the signature.
      The exact semantics are an implementation detail and are subject to
      unannounced changes. Consult the source code for current semantics.


.. class:: Signature(parameters=None, *, return_annotation=Signature.empty)

   A :class:`!Signature` object represents the call signature of a function
   and its return
   annotation.  For each parameter accepted by the function it stores a
   :class:`Parameter` object in its :attr:`parameters` collection.

   The optional *parameters* argument is a sequence of :class:`Parameter`
   objects, which is validated to check that there are no parameters with
   duplicate names, and that the parameters are in the right order, i.e.
   positional-only first, then positional-or-keyword, and that parameters with
   defaults follow parameters without defaults.

   The optional *return_annotation* argument can be an arbitrary Python object.
   It represents the "return" annotation of the callable.

   :class:`!Signature` objects are *immutable*.  Use :meth:`Signature.replace` or
   :func:`copy.replace` to make a modified copy.

   .. versionchanged:: 3.5
      :class:`!Signature` objects are now picklable and :term:`hashable`.

   .. attribute:: Signature.empty

      A special class-level marker to specify absence of a return annotation.

   .. attribute:: Signature.parameters

      An ordered mapping of parameters' names to the corresponding
      :class:`Parameter` objects.  Parameters appear in strict definition
      order, including keyword-only parameters.

      .. versionchanged:: 3.7
         Python only explicitly guaranteed that it preserved the declaration
         order of keyword-only parameters as of version 3.7, although in practice
         this order had always been preserved in Python 3.

   .. attribute:: Signature.return_annotation

      The "return" annotation for the callable.  If the callable has no "return"
      annotation, this attribute is set to :attr:`Signature.empty`.

   .. method:: Signature.bind(*args, **kwargs)

      Create a mapping from positional and keyword arguments to parameters.
      Returns :class:`BoundArguments` if ``*args`` and ``**kwargs`` match the
      signature, or raises a :exc:`TypeError`.

   .. method:: Signature.bind_partial(*args, **kwargs)

      Works the same way as :meth:`Signature.bind`, but allows the omission of
      some required arguments (mimics :func:`functools.partial` behavior.)
      Returns :class:`BoundArguments`, or raises a :exc:`TypeError` if the
      passed arguments do not match the signature.

   .. method:: Signature.replace(*[, parameters][, return_annotation])

      Create a new :class:`Signature` instance based on the instance
      :meth:`replace` was invoked on.
      It is possible to pass different *parameters* and/or
      *return_annotation* to override the corresponding properties of the base
      signature.  To remove ``return_annotation`` from the copied
      :class:`!Signature`, pass in
      :attr:`Signature.empty`.

      .. doctest::

         >>> def test(a, b):
         ...     pass
         ...
         >>> sig = signature(test)
         >>> new_sig = sig.replace(return_annotation="new return anno")
         >>> str(new_sig)
         "(a, b) -> 'new return anno'"

      :class:`Signature` objects are also supported by the generic function
      :func:`copy.replace`.

   .. method:: format(*, max_width=None, quote_annotation_strings=True)

      Create a string representation of the :class:`Signature` object.

      If *max_width* is passed, the method will attempt to fit
      the signature into lines of at most *max_width* characters.
      If the signature is longer than *max_width*,
      all parameters will be on separate lines.

      If *quote_annotation_strings* is False, :term:`annotations <annotation>`
      in the signature are displayed without opening and closing quotation
      marks if they are strings. This is useful if the signature was created with the
      :attr:`~annotationlib.Format.STRING` format or if
      ``from __future__ import annotations`` was used.

      .. versionadded:: 3.13

      .. versionchanged:: 3.14
         The *unquote_annotations* parameter was added.

   .. classmethod:: Signature.from_callable(obj, *, follow_wrapped=True, globals=None, locals=None, eval_str=False)

       Return a :class:`Signature` (or its subclass) object for a given callable
       *obj*.

       This method simplifies subclassing of :class:`Signature`:

       .. testcode::

          class MySignature(Signature):
              pass
          sig = MySignature.from_callable(sum)
          assert isinstance(sig, MySignature)

       Its behavior is otherwise identical to that of :func:`signature`.

       .. versionadded:: 3.5

       .. versionchanged:: 3.10
         The *globals*, *locals*, and *eval_str* parameters were added.


.. class:: Parameter(name, kind, *, default=Parameter.empty, annotation=Parameter.empty)

   :class:`!Parameter` objects are *immutable*.
   Instead of modifying a :class:`!Parameter` object,
   you can use :meth:`Parameter.replace` or :func:`copy.replace` to create a modified copy.

   .. versionchanged:: 3.5
      Parameter objects are now picklable and :term:`hashable`.

   .. attribute:: Parameter.empty

      A special class-level marker to specify absence of default values and
      annotations.

   .. attribute:: Parameter.name

      The name of the parameter as a string.  The name must be a valid
      Python identifier.

      .. impl-detail::

         CPython generates implicit parameter names of the form ``.0`` on the
         code objects used to implement comprehensions and generator
         expressions.

         .. versionchanged:: 3.6
            These parameter names are now exposed by this module as names like
            ``implicit0``.

   .. attribute:: Parameter.default

      The default value for the parameter.  If the parameter has no default
      value, this attribute is set to :attr:`Parameter.empty`.

   .. attribute:: Parameter.annotation

      The annotation for the parameter.  If the parameter has no annotation,
      this attribute is set to :attr:`Parameter.empty`.

   .. attribute:: Parameter.kind

      Describes how argument values are bound to the parameter.  The possible
      values are accessible via :class:`Parameter` (like ``Parameter.KEYWORD_ONLY``),
      and support comparison and ordering, in the following order:

      .. tabularcolumns:: |l|L|

      +------------------------+----------------------------------------------+
      |    Name                | Meaning                                      |
      +========================+==============================================+
      | *POSITIONAL_ONLY*      | Value must be supplied as a positional       |
      |                        | argument. Positional only parameters are     |
      |                        | those which appear before a ``/`` entry (if  |
      |                        | present) in a Python function definition.    |
      +------------------------+----------------------------------------------+
      | *POSITIONAL_OR_KEYWORD*| Value may be supplied as either a keyword or |
      |                        | positional argument (this is the standard    |
      |                        | binding behaviour for functions implemented  |
      |                        | in Python.)                                  |
      +------------------------+----------------------------------------------+
      | *VAR_POSITIONAL*       | A tuple of positional arguments that aren't  |
      |                        | bound to any other parameter. This           |
      |                        | corresponds to a ``*args`` parameter in a    |
      |                        | Python function definition.                  |
      +------------------------+----------------------------------------------+
      | *KEYWORD_ONLY*         | Value must be supplied as a keyword argument.|
      |                        | Keyword only parameters are those which      |
      |                        | appear after a ``*`` or ``*args`` entry in a |
      |                        | Python function definition.                  |
      +------------------------+----------------------------------------------+
      | *VAR_KEYWORD*          | A dict of keyword arguments that aren't bound|
      |                        | to any other parameter. This corresponds to a|
      |                        | ``**kwargs`` parameter in a Python function  |
      |                        | definition.                                  |
      +------------------------+----------------------------------------------+

      Example: print all keyword-only arguments without default values:

      .. doctest::

         >>> def foo(a, b, *, c, d=10):
         ...     pass

         >>> sig = signature(foo)
         >>> for param in sig.parameters.values():
         ...     if (param.kind == param.KEYWORD_ONLY and
         ...                        param.default is param.empty):
         ...         print('Parameter:', param)
         Parameter: c

   .. attribute:: Parameter.kind.description

      Describes an enum value of :attr:`Parameter.kind`.

      .. versionadded:: 3.8

      Example: print all descriptions of arguments:

      .. doctest::

         >>> def foo(a, b, *, c, d=10):
         ...     pass

         >>> sig = signature(foo)
         >>> for param in sig.parameters.values():
         ...     print(param.kind.description)
         positional or keyword
         positional or keyword
         keyword-only
         keyword-only

   .. method:: Parameter.replace(*[, name][, kind][, default][, annotation])

      Create a new :class:`Parameter` instance based on the instance replaced was invoked
      on.  To override a :class:`!Parameter` attribute, pass the corresponding
      argument.  To remove a default value or/and an annotation from a
      :class:`!Parameter`, pass :attr:`Parameter.empty`.

      .. doctest::

         >>> from inspect import Parameter
         >>> param = Parameter('foo', Parameter.KEYWORD_ONLY, default=42)
         >>> str(param)
         'foo=42'

         >>> str(param.replace()) # Will create a shallow copy of 'param'
         'foo=42'

         >>> str(param.replace(default=Parameter.empty, annotation='spam'))
         "foo: 'spam'"

      :class:`Parameter` objects are also supported by the generic function
      :func:`copy.replace`.

   .. versionchanged:: 3.4
      In Python 3.3 :class:`Parameter` objects were allowed to have ``name`` set
      to ``None`` if their ``kind`` was set to ``POSITIONAL_ONLY``.
      This is no longer permitted.

.. class:: BoundArguments

   Result of a :meth:`Signature.bind` or :meth:`Signature.bind_partial` call.
   Holds the mapping of arguments to the function's parameters.

   .. attribute:: BoundArguments.arguments

      A mutable mapping of parameters' names to arguments' values.
      Contains only explicitly bound arguments.  Changes in :attr:`arguments`
      will reflect in :attr:`args` and :attr:`kwargs`.

      Should be used in conjunction with :attr:`Signature.parameters` for any
      argument processing purposes.

      .. note::

         Arguments for which :meth:`Signature.bind` or
         :meth:`Signature.bind_partial` relied on a default value are skipped.
         However, if needed, use :meth:`BoundArguments.apply_defaults` to add
         them.

      .. versionchanged:: 3.9
         :attr:`arguments` is now of type :class:`dict`. Formerly, it was of
         type :class:`collections.OrderedDict`.

   .. attribute:: BoundArguments.args

      A tuple of positional arguments values.  Dynamically computed from the
      :attr:`arguments` attribute.

   .. attribute:: BoundArguments.kwargs

      A dict of keyword arguments values.  Dynamically computed from the
      :attr:`arguments` attribute.  Arguments that can be passed positionally
      are included in :attr:`args` instead.

   .. attribute:: BoundArguments.signature

      A reference to the parent :class:`Signature` object.

   .. method:: BoundArguments.apply_defaults()

      Set default values for missing arguments.

      For variable-positional arguments (``*args``) the default is an
      empty tuple.

      For variable-keyword arguments (``**kwargs``) the default is an
      empty dict.

      .. doctest::

         >>> def foo(a, b='ham', *args): pass
         >>> ba = inspect.signature(foo).bind('spam')
         >>> ba.apply_defaults()
         >>> ba.arguments
         {'a': 'spam', 'b': 'ham', 'args': ()}

      .. versionadded:: 3.5

   The :attr:`args` and :attr:`kwargs` properties can be used to invoke
   functions:

   .. testcode::

      def test(a, *, b):
          ...

      sig = signature(test)
      ba = sig.bind(10, b=20)
      test(*ba.args, **ba.kwargs)


.. seealso::

   :pep:`362` - Function Signature Object.
      The detailed specification, implementation details and examples.


.. _inspect-classes-functions:

Classes and functions
---------------------

.. function:: getclasstree(classes, unique=False)

   Arrange the given list of classes into a hierarchy of nested lists. Where a
   nested list appears, it contains classes derived from the class whose entry
   immediately precedes the list.  Each entry is a 2-tuple containing a class and a
   tuple of its base classes.  If the *unique* argument is true, exactly one entry
   appears in the returned structure for each class in the given list.  Otherwise,
   classes using multiple inheritance and their descendants will appear multiple
   times.


.. function:: getfullargspec(func)

   Get the names and default values of a Python function's parameters.  A
   :term:`named tuple` is returned:

   ``FullArgSpec(args, varargs, varkw, defaults, kwonlyargs, kwonlydefaults,
   annotations)``

   *args* is a list of the positional parameter names.
   *varargs* is the name of the ``*`` parameter or ``None`` if arbitrary
   positional arguments are not accepted.
   *varkw* is the name of the ``**`` parameter or ``None`` if arbitrary
   keyword arguments are not accepted.
   *defaults* is an *n*-tuple of default argument values corresponding to the
   last *n* positional parameters, or ``None`` if there are no such defaults
   defined.
   *kwonlyargs* is a list of keyword-only parameter names in declaration order.
   *kwonlydefaults* is a dictionary mapping parameter names from *kwonlyargs*
   to the default values used if no argument is supplied.
   *annotations* is a dictionary mapping parameter names to annotations.
   The special key ``"return"`` is used to report the function return value
   annotation (if any).

   Note that :func:`signature` and
   :ref:`Signature Object <inspect-signature-object>` provide the recommended
   API for callable introspection, and support additional behaviours (like
   positional-only arguments) that are sometimes encountered in extension module
   APIs. This function is retained primarily for use in code that needs to
   maintain compatibility with the Python 2 ``inspect`` module API.

   .. versionchanged:: 3.4
      This function is now based on :func:`signature`, but still ignores
      ``__wrapped__`` attributes and includes the already bound first
      parameter in the signature output for bound methods.

   .. versionchanged:: 3.6
      This method was previously documented as deprecated in favour of
      :func:`signature` in Python 3.5, but that decision has been reversed
      in order to restore a clearly supported standard interface for
      single-source Python 2/3 code migrating away from the legacy
      :func:`!getargspec` API.

   .. versionchanged:: 3.7
      Python only explicitly guaranteed that it preserved the declaration
      order of keyword-only parameters as of version 3.7, although in practice
      this order had always been preserved in Python 3.


.. function:: getargvalues(frame)

   Get information about arguments passed into a particular frame.  A
   :term:`named tuple` ``ArgInfo(args, varargs, keywords, locals)`` is
   returned. *args* is a list of the argument names.  *varargs* and *keywords*
   are the names of the ``*`` and ``**`` arguments or ``None``.  *locals* is the
   locals dictionary of the given frame.

   .. note::
      This function was inadvertently marked as deprecated in Python 3.5.


.. function:: formatargvalues(args[, varargs, varkw, locals, formatarg, formatvarargs, formatvarkw, formatvalue])

   Format a pretty argument spec from the four values returned by
   :func:`getargvalues`.  The format\* arguments are the corresponding optional
   formatting functions that are called to turn names and values into strings.

   .. note::
      This function was inadvertently marked as deprecated in Python 3.5.


.. function:: getmro(cls)

   Return a tuple of class cls's base classes, including cls, in method resolution
   order.  No class appears more than once in this tuple. Note that the method
   resolution order depends on cls's type.  Unless a very peculiar user-defined
   metatype is in use, cls will be the first element of the tuple.


.. function:: getcallargs(func, /, *args, **kwds)

   Bind the *args* and *kwds* to the argument names of the Python function or
   method *func*, as if it was called with them. For bound methods, bind also the
   first argument (typically named ``self``) to the associated instance. A dict
   is returned, mapping the argument names (including the names of the ``*`` and
   ``**`` arguments, if any) to their values from *args* and *kwds*. In case of
   invoking *func* incorrectly, i.e. whenever ``func(*args, **kwds)`` would raise
   an exception because of incompatible signature, an exception of the same type
   and the same or similar message is raised. For example:

   .. doctest::

      >>> from inspect import getcallargs
      >>> def f(a, b=1, *pos, **named):
      ...     pass
      ...
      >>> getcallargs(f, 1, 2, 3) == {'a': 1, 'named': {}, 'b': 2, 'pos': (3,)}
      True
      >>> getcallargs(f, a=2, x=4) == {'a': 2, 'named': {'x': 4}, 'b': 1, 'pos': ()}
      True
      >>> getcallargs(f)
      Traceback (most recent call last):
      ...
      TypeError: f() missing 1 required positional argument: 'a'

   .. versionadded:: 3.2

   .. deprecated:: 3.5
      Use :meth:`Signature.bind` and :meth:`Signature.bind_partial` instead.


.. function:: getclosurevars(func)

   Get the mapping of external name references in a Python function or
   method *func* to their current values. A
   :term:`named tuple` ``ClosureVars(nonlocals, globals, builtins, unbound)``
   is returned. *nonlocals* maps referenced names to lexical closure
   variables, *globals* to the function's module globals and *builtins* to
   the builtins visible from the function body. *unbound* is the set of names
   referenced in the function that could not be resolved at all given the
   current module globals and builtins.

   :exc:`TypeError` is raised if *func* is not a Python function or method.

   .. versionadded:: 3.3


.. function:: unwrap(func, *, stop=None)

   Get the object wrapped by *func*. It follows the chain of :attr:`__wrapped__`
   attributes returning the last object in the chain.

   *stop* is an optional callback accepting an object in the wrapper chain
   as its sole argument that allows the unwrapping to be terminated early if
   the callback returns a true value. If the callback never returns a true
   value, the last object in the chain is returned as usual. For example,
   :func:`signature` uses this to stop unwrapping if any object in the
   chain has a ``__signature__`` attribute defined.

   :exc:`ValueError` is raised if a cycle is encountered.

   .. versionadded:: 3.4


.. function:: get_annotations(obj, *, globals=None, locals=None, eval_str=False, format=annotationlib.Format.VALUE)

   Compute the annotations dict for an object.

   This is an alias for :func:`annotationlib.get_annotations`; see the documentation
   of that function for more information.

   .. caution::

      This function may execute arbitrary code contained in annotations.
      See :ref:`annotationlib-security` for more information.

   .. versionadded:: 3.10

   .. versionchanged:: 3.14
      This function is now an alias for :func:`annotationlib.get_annotations`.
      Calling it as ``inspect.get_annotations`` will continue to work.


.. _inspect-stack:

The interpreter stack
---------------------

Some of the following functions return
:class:`FrameInfo` objects. For backwards compatibility these objects allow
tuple-like operations on all attributes except ``positions``. This behavior
is considered deprecated and may be removed in the future.

.. class:: FrameInfo

   .. attribute:: frame

      The :ref:`frame object <frame-objects>` that the record corresponds to.

   .. attribute:: filename

      The file name associated with the code being executed by the frame this record
      corresponds to.

   .. attribute:: lineno

      The line number of the current line associated with the code being
      executed by the frame this record corresponds to.

   .. attribute:: function

      The function name that is being executed by the frame this record corresponds to.

   .. attribute:: code_context

      A list of lines of context from the source code that's being executed by the frame
      this record corresponds to.

   .. attribute:: index

      The index of the current line being executed in the :attr:`code_context` list.

   .. attribute:: positions

      A :class:`dis.Positions` object containing the start line number, end line
      number, start column offset, and end column offset associated with the
      instruction being executed by the frame this record corresponds to.

   .. versionchanged:: 3.5
      Return a :term:`named tuple` instead of a :class:`tuple`.

   .. versionchanged:: 3.11
      :class:`!FrameInfo` is now a class instance
      (that is backwards compatible with the previous :term:`named tuple`).


.. class:: Traceback

   .. attribute:: filename

      The file name associated with the code being executed by the frame this traceback
      corresponds to.

   .. attribute:: lineno

      The line number of the current line associated with the code being
      executed by the frame this traceback corresponds to.

   .. attribute:: function

      The function name that is being executed by the frame this traceback corresponds to.

   .. attribute:: code_context

      A list of lines of context from the source code that's being executed by the frame
      this traceback corresponds to.

   .. attribute:: index

      The index of the current line being executed in the :attr:`code_context` list.

   .. attribute:: positions

      A :class:`dis.Positions` object containing the start line number, end
      line number, start column offset, and end column offset associated with
      the instruction being executed by the frame this traceback corresponds
      to.

   .. versionchanged:: 3.11
      :class:`!Traceback` is now a class instance
      (that is backwards compatible with the previous :term:`named tuple`).


.. note::

   Keeping references to frame objects, as found in the first element of the frame
   records these functions return, can cause your program to create reference
   cycles.  Once a reference cycle has been created, the lifespan of all objects
   which can be accessed from the objects which form the cycle can become much
   longer even if Python's optional cycle detector is enabled.  If such cycles must
   be created, it is important to ensure they are explicitly broken to avoid the
   delayed destruction of objects and increased memory consumption which occurs.

   Though the cycle detector will catch these, destruction of the frames (and local
   variables) can be made deterministic by removing the cycle in a
   :keyword:`finally` clause.  This is also important if the cycle detector was
   disabled when Python was compiled or using :func:`gc.disable`.  For example::

      def handle_stackframe_without_leak():
          frame = inspect.currentframe()
          try:
              # do something with the frame
          finally:
              del frame

   If you want to keep the frame around (for example to print a traceback
   later), you can also break reference cycles by using the
   :meth:`frame.clear` method.

The optional *context* argument supported by most of these functions specifies
the number of lines of context to return, which are centered around the current
line.


.. function:: getframeinfo(frame, context=1)

   Get information about a frame or traceback object.  A :class:`Traceback` object
   is returned.

   .. versionchanged:: 3.11
      A :class:`Traceback` object is returned instead of a named tuple.

.. function:: getouterframes(frame, context=1)

   Get a list of :class:`FrameInfo` objects for a frame and all outer frames.
   These frames represent the calls that lead to the creation of *frame*. The
   first entry in the returned list represents *frame*; the last entry
   represents the outermost call on *frame*'s stack.

   .. versionchanged:: 3.5
      A list of :term:`named tuples <named tuple>`
      ``FrameInfo(frame, filename, lineno, function, code_context, index)``
      is returned.

   .. versionchanged:: 3.11
      A list of :class:`FrameInfo` objects is returned.

.. function:: getinnerframes(traceback, context=1)

   Get a list of :class:`FrameInfo` objects for a traceback's frame and all
   inner frames.  These frames represent calls made as a consequence of *frame*.
   The first entry in the list represents *traceback*; the last entry represents
   where the exception was raised.

   .. versionchanged:: 3.5
      A list of :term:`named tuples <named tuple>`
      ``FrameInfo(frame, filename, lineno, function, code_context, index)``
      is returned.

   .. versionchanged:: 3.11
      A list of :class:`FrameInfo` objects is returned.

.. function:: currentframe()

   Return the frame object for the caller's stack frame.

   .. impl-detail::

      This function relies on Python stack frame support in the interpreter,
      which isn't guaranteed to exist in all implementations of Python.  If
      running in an implementation without Python stack frame support this
      function returns ``None``.


.. function:: stack(context=1)

   Return a list of :class:`FrameInfo` objects for the caller's stack.  The
   first entry in the returned list represents the caller; the last entry
   represents the outermost call on the stack.

   .. versionchanged:: 3.5
      A list of :term:`named tuples <named tuple>`
      ``FrameInfo(frame, filename, lineno, function, code_context, index)``
      is returned.

   .. versionchanged:: 3.11
      A list of :class:`FrameInfo` objects is returned.

.. function:: trace(context=1)

   Return a list of :class:`FrameInfo` objects for the stack between the current
   frame and the frame in which an exception currently being handled was raised
   in.  The first entry in the list represents the caller; the last entry
   represents where the exception was raised.

   .. versionchanged:: 3.5
      A list of :term:`named tuples <named tuple>`
      ``FrameInfo(frame, filename, lineno, function, code_context, index)``
      is returned.

   .. versionchanged:: 3.11
      A list of :class:`FrameInfo` objects is returned.

Fetching attributes statically
------------------------------

Both :func:`getattr` and :func:`hasattr` can trigger code execution when
fetching or checking for the existence of attributes. Descriptors, like
properties, will be invoked and :meth:`~object.__getattr__` and
:meth:`~object.__getattribute__`
may be called.

For cases where you want passive introspection, like documentation tools, this
can be inconvenient. :func:`getattr_static` has the same signature as :func:`getattr`
but avoids executing code when it fetches attributes.

.. function:: getattr_static(obj, attr, default=None)

   Retrieve attributes without triggering dynamic lookup via the
   descriptor protocol, :meth:`~object.__getattr__`
   or :meth:`~object.__getattribute__`.

   Note: this function may not be able to retrieve all attributes
   that getattr can fetch (like dynamically created attributes)
   and may find attributes that getattr can't (like descriptors
   that raise AttributeError). It can also return descriptors objects
   instead of instance members.

   If the instance :attr:`~object.__dict__` is shadowed by another member (for
   example a property) then this function will be unable to find instance
   members.

   .. versionadded:: 3.2

:func:`getattr_static` does not resolve descriptors, for example slot descriptors or
getset descriptors on objects implemented in C. The descriptor object
is returned instead of the underlying attribute.

You can handle these with code like the following. Note that
for arbitrary getset descriptors invoking these may trigger
code execution::

   # example code for resolving the builtin descriptor types
   class _foo:
       __slots__ = ['foo']

   slot_descriptor = type(_foo.foo)
   getset_descriptor = type(type(open(__file__)).name)
   wrapper_descriptor = type(str.__dict__['__add__'])
   descriptor_types = (slot_descriptor, getset_descriptor, wrapper_descriptor)

   result = getattr_static(some_object, 'foo')
   if type(result) in descriptor_types:
       try:
           result = result.__get__()
       except AttributeError:
           # descriptors can raise AttributeError to
           # indicate there is no underlying value
           # in which case the descriptor itself will
           # have to do
           pass


Current State of Generators, Coroutines, and Asynchronous Generators
--------------------------------------------------------------------

When implementing coroutine schedulers and for other advanced uses of
generators, it is useful to determine whether a generator is currently
executing, is waiting to start or resume or execution, or has already
terminated. :func:`getgeneratorstate` allows the current state of a
generator to be determined easily.

.. function:: getgeneratorstate(generator)

   Get current state of a generator-iterator.

   Possible states are:

   * GEN_CREATED: Waiting to start execution.
   * GEN_RUNNING: Currently being executed by the interpreter.
   * GEN_SUSPENDED: Currently suspended at a yield expression.
   * GEN_CLOSED: Execution has completed.

   .. versionadded:: 3.2

.. function:: getcoroutinestate(coroutine)

   Get current state of a coroutine object.  The function is intended to be
   used with coroutine objects created by :keyword:`async def` functions, but
   will accept any coroutine-like object that has ``cr_running`` and
   ``cr_frame`` attributes.

   Possible states are:

   * CORO_CREATED: Waiting to start execution.
   * CORO_RUNNING: Currently being executed by the interpreter.
   * CORO_SUSPENDED: Currently suspended at an await expression.
   * CORO_CLOSED: Execution has completed.

   .. versionadded:: 3.5

.. function:: getasyncgenstate(agen)

   Get current state of an asynchronous generator object.  The function is
   intended to be used with asynchronous iterator objects created by
   :keyword:`async def` functions which use the :keyword:`yield` statement,
   but will accept any asynchronous generator-like object that has
   ``ag_running`` and ``ag_frame`` attributes.

   Possible states are:

   * AGEN_CREATED: Waiting to start execution.
   * AGEN_RUNNING: Currently being executed by the interpreter.
   * AGEN_SUSPENDED: Currently suspended at a yield expression.
   * AGEN_CLOSED: Execution has completed.

   .. versionadded:: 3.12

The current internal state of the generator can also be queried. This is
mostly useful for testing purposes, to ensure that internal state is being
updated as expected:

.. function:: getgeneratorlocals(generator)

   Get the mapping of live local variables in *generator* to their current
   values.  A dictionary is returned that maps from variable names to values.
   This is the equivalent of calling :func:`locals` in the body of the
   generator, and all the same caveats apply.

   If *generator* is a :term:`generator` with no currently associated frame,
   then an empty dictionary is returned.  :exc:`TypeError` is raised if
   *generator* is not a Python generator object.

   .. impl-detail::

      This function relies on the generator exposing a Python stack frame
      for introspection, which isn't guaranteed to be the case in all
      implementations of Python. In such cases, this function will always
      return an empty dictionary.

   .. versionadded:: 3.3

.. function:: getcoroutinelocals(coroutine)

   This function is analogous to :func:`~inspect.getgeneratorlocals`, but
   works for coroutine objects created by :keyword:`async def` functions.

   .. versionadded:: 3.5

.. function:: getasyncgenlocals(agen)

   This function is analogous to :func:`~inspect.getgeneratorlocals`, but
   works for asynchronous generator objects created by :keyword:`async def`
   functions which use the :keyword:`yield` statement.

   .. versionadded:: 3.12


.. _inspect-module-co-flags:

Code Objects Bit Flags
----------------------

Python code objects have a :attr:`~codeobject.co_flags` attribute,
which is a bitmap of the following flags:

.. data:: CO_OPTIMIZED

   The code object is optimized, using fast locals.

.. data:: CO_NEWLOCALS

   If set, a new dict will be created for the frame's :attr:`~frame.f_locals`
   when the code object is executed.

.. data:: CO_VARARGS

   The code object has a variable positional parameter (``*args``-like).

.. data:: CO_VARKEYWORDS

   The code object has a variable keyword parameter (``**kwargs``-like).

.. data:: CO_NESTED

   The flag is set when the code object is a nested function.

.. data:: CO_GENERATOR

   The flag is set when the code object is a generator function, i.e.
   a generator object is returned when the code object is executed.

.. data:: CO_COROUTINE

   The flag is set when the code object is a coroutine function.
   When the code object is executed it returns a coroutine object.
   See :pep:`492` for more details.

   .. versionadded:: 3.5

.. data:: CO_ITERABLE_COROUTINE

   The flag is used to transform generators into generator-based
   coroutines.  Generator objects with this flag can be used in
   ``await`` expression, and can ``yield from`` coroutine objects.
   See :pep:`492` for more details.

   .. versionadded:: 3.5

.. data:: CO_ASYNC_GENERATOR

   The flag is set when the code object is an asynchronous generator
   function.  When the code object is executed it returns an
   asynchronous generator object.  See :pep:`525` for more details.

   .. versionadded:: 3.6

.. data:: CO_HAS_DOCSTRING

   The flag is set when there is a docstring for the code object in
   the source code. If set, it will be the first item in
   :attr:`~codeobject.co_consts`.

   .. versionadded:: 3.14

.. data:: CO_METHOD

   The flag is set when the code object is a function defined in class
   scope.

   .. versionadded:: 3.14

.. note::
   The flags are specific to CPython, and may not be defined in other
   Python implementations.  Furthermore, the flags are an implementation
   detail, and can be removed or deprecated in future Python releases.
   It's recommended to use public APIs from the :mod:`inspect` module
   for any introspection needs.


Buffer flags
------------

.. class:: BufferFlags

   This is an :class:`enum.IntFlag` that represents the flags that
   can be passed to the :meth:`~object.__buffer__` method of objects
   implementing the :ref:`buffer protocol <bufferobjects>`.

   The meaning of the flags is explained at :ref:`buffer-request-types`.

   .. attribute:: BufferFlags.SIMPLE
   .. attribute:: BufferFlags.WRITABLE
   .. attribute:: BufferFlags.FORMAT
   .. attribute:: BufferFlags.ND
   .. attribute:: BufferFlags.STRIDES
   .. attribute:: BufferFlags.C_CONTIGUOUS
   .. attribute:: BufferFlags.F_CONTIGUOUS
   .. attribute:: BufferFlags.ANY_CONTIGUOUS
   .. attribute:: BufferFlags.INDIRECT
   .. attribute:: BufferFlags.CONTIG
   .. attribute:: BufferFlags.CONTIG_RO
   .. attribute:: BufferFlags.STRIDED
   .. attribute:: BufferFlags.STRIDED_RO
   .. attribute:: BufferFlags.RECORDS
   .. attribute:: BufferFlags.RECORDS_RO
   .. attribute:: BufferFlags.FULL
   .. attribute:: BufferFlags.FULL_RO
   .. attribute:: BufferFlags.READ
   .. attribute:: BufferFlags.WRITE

   .. versionadded:: 3.12

.. _inspect-module-cli:

Command-line interface
----------------------

The :mod:`inspect` module also provides a basic introspection capability
from the command line.

.. program:: inspect

By default, accepts the name of a module and prints the source of that
module. A class or function within the module can be printed instead by
appended a colon and the qualified name of the target object.

.. option:: --details

   Print information about the specified object rather than the source code
