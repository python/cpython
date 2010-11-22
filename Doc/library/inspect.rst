:mod:`inspect` --- Inspect live objects
=======================================

.. module:: inspect
   :synopsis: Extract information and source code from live objects.
.. moduleauthor:: Ka-Ping Yee <ping@lfw.org>
.. sectionauthor:: Ka-Ping Yee <ping@lfw.org>


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
class or module. The sixteen functions whose names begin with "is" are mainly
provided as convenient choices for the second argument to :func:`getmembers`.
They also help you determine when you can expect to find the following special
attributes:

+-----------+-----------------+---------------------------+
| Type      | Attribute       | Description               |
+===========+=================+===========================+
| module    | __doc__         | documentation string      |
+-----------+-----------------+---------------------------+
|           | __file__        | filename (missing for     |
|           |                 | built-in modules)         |
+-----------+-----------------+---------------------------+
| class     | __doc__         | documentation string      |
+-----------+-----------------+---------------------------+
|           | __module__      | name of module in which   |
|           |                 | this class was defined    |
+-----------+-----------------+---------------------------+
| method    | __doc__         | documentation string      |
+-----------+-----------------+---------------------------+
|           | __name__        | name with which this      |
|           |                 | method was defined        |
+-----------+-----------------+---------------------------+
|           | __func__        | function object           |
|           |                 | containing implementation |
|           |                 | of method                 |
+-----------+-----------------+---------------------------+
|           | __self__        | instance to which this    |
|           |                 | method is bound, or       |
|           |                 | ``None``                  |
+-----------+-----------------+---------------------------+
| function  | __doc__         | documentation string      |
+-----------+-----------------+---------------------------+
|           | __name__        | name with which this      |
|           |                 | function was defined      |
+-----------+-----------------+---------------------------+
|           | __code__        | code object containing    |
|           |                 | compiled function         |
|           |                 | :term:`bytecode`          |
+-----------+-----------------+---------------------------+
|           | __defaults__    | tuple of any default      |
|           |                 | values for arguments      |
+-----------+-----------------+---------------------------+
|           | __globals__     | global namespace in which |
|           |                 | this function was defined |
+-----------+-----------------+---------------------------+
| traceback | tb_frame        | frame object at this      |
|           |                 | level                     |
+-----------+-----------------+---------------------------+
|           | tb_lasti        | index of last attempted   |
|           |                 | instruction in bytecode   |
+-----------+-----------------+---------------------------+
|           | tb_lineno       | current line number in    |
|           |                 | Python source code        |
+-----------+-----------------+---------------------------+
|           | tb_next         | next inner traceback      |
|           |                 | object (called by this    |
|           |                 | level)                    |
+-----------+-----------------+---------------------------+
| frame     | f_back          | next outer frame object   |
|           |                 | (this frame's caller)     |
+-----------+-----------------+---------------------------+
|           | f_builtins      | builtins namespace seen   |
|           |                 | by this frame             |
+-----------+-----------------+---------------------------+
|           | f_code          | code object being         |
|           |                 | executed in this frame    |
+-----------+-----------------+---------------------------+
|           | f_globals       | global namespace seen by  |
|           |                 | this frame                |
+-----------+-----------------+---------------------------+
|           | f_lasti         | index of last attempted   |
|           |                 | instruction in bytecode   |
+-----------+-----------------+---------------------------+
|           | f_lineno        | current line number in    |
|           |                 | Python source code        |
+-----------+-----------------+---------------------------+
|           | f_locals        | local namespace seen by   |
|           |                 | this frame                |
+-----------+-----------------+---------------------------+
|           | f_restricted    | 0 or 1 if frame is in     |
|           |                 | restricted execution mode |
+-----------+-----------------+---------------------------+
|           | f_trace         | tracing function for this |
|           |                 | frame, or ``None``        |
+-----------+-----------------+---------------------------+
| code      | co_argcount     | number of arguments (not  |
|           |                 | including \* or \*\*      |
|           |                 | args)                     |
+-----------+-----------------+---------------------------+
|           | co_code         | string of raw compiled    |
|           |                 | bytecode                  |
+-----------+-----------------+---------------------------+
|           | co_consts       | tuple of constants used   |
|           |                 | in the bytecode           |
+-----------+-----------------+---------------------------+
|           | co_filename     | name of file in which     |
|           |                 | this code object was      |
|           |                 | created                   |
+-----------+-----------------+---------------------------+
|           | co_firstlineno  | number of first line in   |
|           |                 | Python source code        |
+-----------+-----------------+---------------------------+
|           | co_flags        | bitmap: 1=optimized ``|`` |
|           |                 | 2=newlocals ``|`` 4=\*arg |
|           |                 | ``|`` 8=\*\*arg           |
+-----------+-----------------+---------------------------+
|           | co_lnotab       | encoded mapping of line   |
|           |                 | numbers to bytecode       |
|           |                 | indices                   |
+-----------+-----------------+---------------------------+
|           | co_name         | name with which this code |
|           |                 | object was defined        |
+-----------+-----------------+---------------------------+
|           | co_names        | tuple of names of local   |
|           |                 | variables                 |
+-----------+-----------------+---------------------------+
|           | co_nlocals      | number of local variables |
+-----------+-----------------+---------------------------+
|           | co_stacksize    | virtual machine stack     |
|           |                 | space required            |
+-----------+-----------------+---------------------------+
|           | co_varnames     | tuple of names of         |
|           |                 | arguments and local       |
|           |                 | variables                 |
+-----------+-----------------+---------------------------+
| builtin   | __doc__         | documentation string      |
+-----------+-----------------+---------------------------+
|           | __name__        | original name of this     |
|           |                 | function or method        |
+-----------+-----------------+---------------------------+
|           | __self__        | instance to which a       |
|           |                 | method is bound, or       |
|           |                 | ``None``                  |
+-----------+-----------------+---------------------------+


.. function:: getmembers(object[, predicate])

   Return all the members of an object in a list of (name, value) pairs sorted by
   name.  If the optional *predicate* argument is supplied, only members for which
   the predicate returns a true value are included.

   .. note::

      :func:`getmembers` does not return metaclass attributes when the argument
      is a class (this behavior is inherited from the :func:`dir` function).


.. function:: getmoduleinfo(path)

   Returns a :term:`named tuple` ``ModuleInfo(name, suffix, mode,
   module_type)`` of values that describe how Python will interpret the file
   identified by *path* if it is a module, or ``None`` if it would not be
   identified as a module.  The return tuple is ``(name, suffix, mode, mtype)``,
   where *name* is the name of the module without the name of any enclosing
   package, *suffix* is the trailing part of the file name (which may not be a
   dot-delimited extension), *mode* is the :func:`open` mode that would be used
   (``'r'`` or ``'rb'``), and *mtype* is an integer giving the type of the
   module.  *mtype* will have a value which can be compared to the constants
   defined in the :mod:`imp` module; see the documentation for that module for
   more information on module types.


.. function:: getmodulename(path)

   Return the name of the module named by the file *path*, without including the
   names of enclosing packages.  This uses the same algorithm as the interpreter
   uses when searching for modules.  If the name cannot be matched according to the
   interpreter's rules, ``None`` is returned.


.. function:: ismodule(object)

   Return true if the object is a module.


.. function:: isclass(object)

   Return true if the object is a class, whether built-in or created in Python
   code.


.. function:: ismethod(object)

   Return true if the object is a bound method written in Python.


.. function:: isfunction(object)

   Return true if the object is a Python function, which includes functions
   created by a :term:`lambda` expression.


.. function:: isgeneratorfunction(object)

   Return true if the object is a Python generator function.


.. function:: isgenerator(object)

   Return true if the object is a generator.


.. function:: istraceback(object)

   Return true if the object is a traceback.


.. function:: isframe(object)

   Return true if the object is a frame.


.. function:: iscode(object)

   Return true if the object is a code.


.. function:: isbuiltin(object)

   Return true if the object is a built-in function or a bound built-in method.


.. function:: isroutine(object)

   Return true if the object is a user-defined or built-in function or method.


.. function:: isabstract(object)

   Return true if the object is an abstract base class.


.. function:: ismethoddescriptor(object)

   Return true if the object is a method descriptor, but not if
   :func:`ismethod`, :func:`isclass`, :func:`isfunction` or :func:`isbuiltin`
   are true.

   This, for example, is true of ``int.__add__``.  An object passing this test
   has a :attr:`__get__` attribute but not a :attr:`__set__` attribute, but
   beyond that the set of attributes varies.  :attr:`__name__` is usually
   sensible, and :attr:`__doc__` often is.

   Methods implemented via descriptors that also pass one of the other tests
   return false from the :func:`ismethoddescriptor` test, simply because the
   other tests promise more -- you can, e.g., count on having the
   :attr:`__func__` attribute (etc) when an object passes :func:`ismethod`.


.. function:: isdatadescriptor(object)

   Return true if the object is a data descriptor.

   Data descriptors have both a :attr:`__get__` and a :attr:`__set__` attribute.
   Examples are properties (defined in Python), getsets, and members.  The
   latter two are defined in C and there are more specific tests available for
   those types, which is robust across Python implementations.  Typically, data
   descriptors will also have :attr:`__name__` and :attr:`__doc__` attributes
   (properties, getsets, and members have both of these attributes), but this is
   not guaranteed.


.. function:: isgetsetdescriptor(object)

   Return true if the object is a getset descriptor.

   .. impl-detail::

      getsets are attributes defined in extension modules via
      :c:type:`PyGetSetDef` structures.  For Python implementations without such
      types, this method will always return ``False``.


.. function:: ismemberdescriptor(object)

   Return true if the object is a member descriptor.

   .. impl-detail::

      Member descriptors are attributes defined in extension modules via
      :c:type:`PyMemberDef` structures.  For Python implementations without such
      types, this method will always return ``False``.


.. _inspect-source:

Retrieving source code
----------------------

.. function:: getdoc(object)

   Get the documentation string for an object, cleaned up with :func:`cleandoc`.


.. function:: getcomments(object)

   Return in a single string any lines of comments immediately preceding the
   object's source code (for a class, function, or method), or at the top of the
   Python source file (if the object is a module).


.. function:: getfile(object)

   Return the name of the (text or binary) file in which an object was defined.
   This will fail with a :exc:`TypeError` if the object is a built-in module,
   class, or function.


.. function:: getmodule(object)

   Try to guess which module an object was defined in.


.. function:: getsourcefile(object)

   Return the name of the Python source file in which an object was defined.  This
   will fail with a :exc:`TypeError` if the object is a built-in module, class, or
   function.


.. function:: getsourcelines(object)

   Return a list of source lines and starting line number for an object. The
   argument may be a module, class, method, function, traceback, frame, or code
   object.  The source code is returned as a list of the lines corresponding to the
   object and the line number indicates where in the original source file the first
   line of code was found.  An :exc:`IOError` is raised if the source code cannot
   be retrieved.


.. function:: getsource(object)

   Return the text of the source code for an object. The argument may be a module,
   class, method, function, traceback, frame, or code object.  The source code is
   returned as a single string.  An :exc:`IOError` is raised if the source code
   cannot be retrieved.


.. function:: cleandoc(doc)

   Clean up indentation from docstrings that are indented to line up with blocks
   of code.  Any whitespace that can be uniformly removed from the second line
   onwards is removed.  Also, all tabs are expanded to spaces.


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


.. function:: getargspec(func)

   Get the names and default values of a Python function's arguments. A
   :term:`named tuple` ``ArgSpec(args, varargs, keywords,
   defaults)`` is returned. *args* is a list of
   the argument names. *varargs* and *varkw* are the names of the ``*`` and
   ``**`` arguments or ``None``. *defaults* is a tuple of default argument
   values or None if there are no default arguments; if this tuple has *n*
   elements, they correspond to the last *n* elements listed in *args*.

   .. deprecated:: 3.0
      Use :func:`getfullargspec` instead, which provides information about
      keyword-only arguments and annotations.


.. function:: getfullargspec(func)

   Get the names and default values of a Python function's arguments.  A
   :term:`named tuple` is returned:

   ``FullArgSpec(args, varargs, varkw, defaults, kwonlyargs, kwonlydefaults,
   annotations)``

   *args* is a list of the argument names.  *varargs* and *varkw* are the names
   of the ``*`` and ``**`` arguments or ``None``.  *defaults* is an n-tuple of
   the default values of the last n arguments.  *kwonlyargs* is a list of
   keyword-only argument names.  *kwonlydefaults* is a dictionary mapping names
   from kwonlyargs to defaults.  *annotations* is a dictionary mapping argument
   names to annotations.

   The first four items in the tuple correspond to :func:`getargspec`.


.. function:: getargvalues(frame)

   Get information about arguments passed into a particular frame.  A
   :term:`named tuple` ``ArgInfo(args, varargs, keywords, locals)`` is
   returned. *args* is a list of the argument names.  *varargs* and *varkw* are
   the names of the ``*`` and ``**`` arguments or ``None``.  *locals* is the
   locals dictionary of the given frame.


.. function:: formatargspec(args[, varargs, varkw, defaults, formatarg, formatvarargs, formatvarkw, formatvalue])

   Format a pretty argument spec from the four values returned by
   :func:`getargspec`.  The format\* arguments are the corresponding optional
   formatting functions that are called to turn names and values into strings.


.. function:: formatargvalues(args[, varargs, varkw, locals, formatarg, formatvarargs, formatvarkw, formatvalue])

   Format a pretty argument spec from the four values returned by
   :func:`getargvalues`.  The format\* arguments are the corresponding optional
   formatting functions that are called to turn names and values into strings.


.. function:: getmro(cls)

   Return a tuple of class cls's base classes, including cls, in method resolution
   order.  No class appears more than once in this tuple. Note that the method
   resolution order depends on cls's type.  Unless a very peculiar user-defined
   metatype is in use, cls will be the first element of the tuple.


.. function:: getcallargs(func[, *args][, **kwds])

   Bind the *args* and *kwds* to the argument names of the Python function or
   method *func*, as if it was called with them. For bound methods, bind also the
   first argument (typically named ``self``) to the associated instance. A dict
   is returned, mapping the argument names (including the names of the ``*`` and
   ``**`` arguments, if any) to their values from *args* and *kwds*. In case of
   invoking *func* incorrectly, i.e. whenever ``func(*args, **kwds)`` would raise
   an exception because of incompatible signature, an exception of the same type
   and the same or similar message is raised. For example::

    >>> from inspect import getcallargs
    >>> def f(a, b=1, *pos, **named):
    ...     pass
    >>> getcallargs(f, 1, 2, 3)
    {'a': 1, 'named': {}, 'b': 2, 'pos': (3,)}
    >>> getcallargs(f, a=2, x=4)
    {'a': 2, 'named': {'x': 4}, 'b': 1, 'pos': ()}
    >>> getcallargs(f)
    Traceback (most recent call last):
    ...
    TypeError: f() takes at least 1 argument (0 given)

   .. versionadded:: 3.2


.. _inspect-stack:

The interpreter stack
---------------------

When the following functions return "frame records," each record is a tuple of
six items: the frame object, the filename, the line number of the current line,
the function name, a list of lines of context from the source code, and the
index of the current line within that list.

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

The optional *context* argument supported by most of these functions specifies
the number of lines of context to return, which are centered around the current
line.


.. function:: getframeinfo(frame, context=1)

   Get information about a frame or traceback object.  A :term:`named tuple`
   ``Traceback(filename, lineno, function, code_context, index)`` is returned.


.. function:: getouterframes(frame, context=1)

   Get a list of frame records for a frame and all outer frames.  These frames
   represent the calls that lead to the creation of *frame*. The first entry in the
   returned list represents *frame*; the last entry represents the outermost call
   on *frame*'s stack.


.. function:: getinnerframes(traceback, context=1)

   Get a list of frame records for a traceback's frame and all inner frames.  These
   frames represent calls made as a consequence of *frame*.  The first entry in the
   list represents *traceback*; the last entry represents where the exception was
   raised.


.. function:: currentframe()

   Return the frame object for the caller's stack frame.

   .. impl-detail::

      This function relies on Python stack frame support in the interpreter,
      which isn't guaranteed to exist in all implementations of Python.  If
      running in an implementation without Python stack frame support this
      function returns ``None``.


.. function:: stack(context=1)

   Return a list of frame records for the caller's stack.  The first entry in the
   returned list represents the caller; the last entry represents the outermost
   call on the stack.


.. function:: trace(context=1)

   Return a list of frame records for the stack between the current frame and the
   frame in which an exception currently being handled was raised in.  The first
   entry in the list represents the caller; the last entry represents where the
   exception was raised.


Fetching attributes statically
------------------------------

Both :func:`getattr` and :func:`hasattr` can trigger code execution when
fetching or checking for the existence of attributes. Descriptors, like
properties, will be invoked and :meth:`__getattr__` and :meth:`__getattribute__`
may be called.

For cases where you want passive introspection, like documentation tools, this
can be inconvenient. `getattr_static` has the same signature as :func:`getattr`
but avoids executing code when it fetches attributes.

.. function:: getattr_static(obj, attr, default=None)

   Retrieve attributes without triggering dynamic lookup via the
   descriptor protocol, `__getattr__` or `__getattribute__`.

   Note: this function may not be able to retrieve all attributes
   that getattr can fetch (like dynamically created attributes)
   and may find attributes that getattr can't (like descriptors
   that raise AttributeError). It can also return descriptors objects
   instead of instance members.

   .. versionadded:: 3.2

The only known case that can cause `getattr_static` to trigger code execution,
and cause it to return incorrect results (or even break), is where a class uses
:data:`~object.__slots__` and provides a `__dict__` member using a property or
descriptor. If you find other cases please report them so they can be fixed
or documented.

`getattr_static` does not resolve descriptors, for example slot descriptors or
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


Current State of a Generator
----------------------------

When implementing coroutine schedulers and for other advanced uses of
generators, it is useful to determine whether a generator is currently
executing, is waiting to start or resume or execution, or has already
terminated. func:`getgeneratorstate` allows the current state of a
generator to be determined easily.

.. function:: getgeneratorstate(generator)

    Get current state of a generator-iterator.

    Possible states are:
      GEN_CREATED: Waiting to start execution.
      GEN_RUNNING: Currently being executed by the interpreter.
      GEN_SUSPENDED: Currently suspended at a yield expression.
      GEN_CLOSED: Execution has completed.

   .. versionadded:: 3.2
