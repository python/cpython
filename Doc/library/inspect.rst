:mod:`inspect` --- Inspect live objects
=======================================

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
|           |                 | values for positional or  |
|           |                 | keyword parameters        |
+-----------+-----------------+---------------------------+
|           | __kwdefaults__  | mapping of any default    |
|           |                 | values for keyword-only   |
|           |                 | parameters                |
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

      :func:`getmembers` will only return class attributes defined in the
      metaclass when the argument is a class and those attributes have been
      listed in the metaclass' custom :meth:`__dir__`.


.. function:: getmoduleinfo(path)

   Returns a :term:`named tuple` ``ModuleInfo(name, suffix, mode, module_type)``
   of values that describe how Python will interpret the file identified by
   *path* if it is a module, or ``None`` if it would not be identified as a
   module.  In that tuple, *name* is the name of the module without the name of
   any enclosing package, *suffix* is the trailing part of the file name (which
   may not be a dot-delimited extension), *mode* is the :func:`open` mode that
   would be used (``'r'`` or ``'rb'``), and *module_type* is an integer giving
   the type of the module.  *module_type* will have a value which can be
   compared to the constants defined in the :mod:`imp` module; see the
   documentation for that module for more information on module types.

   .. deprecated:: 3.3
      You may check the file path's suffix against the supported suffixes
      listed in :mod:`importlib.machinery` to infer the same information.


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
      This function is now based directly on :mod:`importlib` rather than the
      deprecated :func:`getmoduleinfo`.


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
   line of code was found.  An :exc:`OSError` is raised if the source code cannot
   be retrieved.

   .. versionchanged:: 3.3
      :exc:`OSError` is raised instead of :exc:`IOError`, now an alias of the
      former.


.. function:: getsource(object)

   Return the text of the source code for an object. The argument may be a module,
   class, method, function, traceback, frame, or code object.  The source code is
   returned as a single string.  An :exc:`OSError` is raised if the source code
   cannot be retrieved.

   .. versionchanged:: 3.3
      :exc:`OSError` is raised instead of :exc:`IOError`, now an alias of the
      former.


.. function:: cleandoc(doc)

   Clean up indentation from docstrings that are indented to line up with blocks
   of code.  Any whitespace that can be uniformly removed from the second line
   onwards is removed.  Also, all tabs are expanded to spaces.


.. _inspect-signature-object:

Introspecting callables with the Signature object
-------------------------------------------------

.. versionadded:: 3.3

The Signature object represents the call signature of a callable object and its
return annotation.  To retrieve a Signature object, use the :func:`signature`
function.

.. function:: signature(callable)

   Return a :class:`Signature` object for the given ``callable``::

      >>> from inspect import signature
      >>> def foo(a, *, b:int, **kwargs):
      ...     pass

      >>> sig = signature(foo)

      >>> str(sig)
      '(a, *, b:int, **kwargs)'

      >>> str(sig.parameters['b'])
      'b:int'

      >>> sig.parameters['b'].annotation
      <class 'int'>

   Accepts a wide range of python callables, from plain functions and classes to
   :func:`functools.partial` objects.

   Raises :exc:`ValueError` if no signature can be provided, and
   :exc:`TypeError` if that type of object is not supported.

   .. note::

      Some callables may not be introspectable in certain implementations of
      Python.  For example, in CPython, some built-in functions defined in
      C provide no metadata about their arguments.


.. class:: Signature(parameters=None, \*, return_annotation=Signature.empty)

   A Signature object represents the call signature of a function and its return
   annotation.  For each parameter accepted by the function it stores a
   :class:`Parameter` object in its :attr:`parameters` collection.

   The optional *parameters* argument is a sequence of :class:`Parameter`
   objects, which is validated to check that there are no parameters with
   duplicate names, and that the parameters are in the right order, i.e.
   positional-only first, then positional-or-keyword, and that parameters with
   defaults follow parameters without defaults.

   The optional *return_annotation* argument, can be an arbitrary Python object,
   is the "return" annotation of the callable.

   Signature objects are *immutable*.  Use :meth:`Signature.replace` to make a
   modified copy.

   .. attribute:: Signature.empty

      A special class-level marker to specify absence of a return annotation.

   .. attribute:: Signature.parameters

      An ordered mapping of parameters' names to the corresponding
      :class:`Parameter` objects.

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

      Create a new Signature instance based on the instance replace was invoked
      on.  It is possible to pass different ``parameters`` and/or
      ``return_annotation`` to override the corresponding properties of the base
      signature.  To remove return_annotation from the copied Signature, pass in
      :attr:`Signature.empty`.

      ::

         >>> def test(a, b):
         ...     pass
         >>> sig = signature(test)
         >>> new_sig = sig.replace(return_annotation="new return anno")
         >>> str(new_sig)
         "(a, b) -> 'new return anno'"


.. class:: Parameter(name, kind, \*, default=Parameter.empty, annotation=Parameter.empty)

   Parameter objects are *immutable*.  Instead of modifying a Parameter object,
   you can use :meth:`Parameter.replace` to create a modified copy.

   .. attribute:: Parameter.empty

      A special class-level marker to specify absence of default values and
      annotations.

   .. attribute:: Parameter.name

      The name of the parameter as a string.  The name must be a valid
      Python identifier.

   .. attribute:: Parameter.default

      The default value for the parameter.  If the parameter has no default
      value, this attribute is set to :attr:`Parameter.empty`.

   .. attribute:: Parameter.annotation

      The annotation for the parameter.  If the parameter has no annotation,
      this attribute is set to :attr:`Parameter.empty`.

   .. attribute:: Parameter.kind

      Describes how argument values are bound to the parameter.  Possible values
      (accessible via :class:`Parameter`, like ``Parameter.KEYWORD_ONLY``):

      .. tabularcolumns:: |l|L|

      +------------------------+----------------------------------------------+
      |    Name                | Meaning                                      |
      +========================+==============================================+
      | *POSITIONAL_ONLY*      | Value must be supplied as a positional       |
      |                        | argument.                                    |
      |                        |                                              |
      |                        | Python has no explicit syntax for defining   |
      |                        | positional-only parameters, but many built-in|
      |                        | and extension module functions (especially   |
      |                        | those that accept only one or two parameters)|
      |                        | accept them.                                 |
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

      Example: print all keyword-only arguments without default values::

         >>> def foo(a, b, *, c, d=10):
         ...     pass

         >>> sig = signature(foo)
         >>> for param in sig.parameters.values():
         ...     if (param.kind == param.KEYWORD_ONLY and
         ...                        param.default is param.empty):
         ...         print('Parameter:', param)
         Parameter: c

   .. method:: Parameter.replace(*[, name][, kind][, default][, annotation])

      Create a new Parameter instance based on the instance replaced was invoked
      on.  To override a :class:`Parameter` attribute, pass the corresponding
      argument.  To remove a default value or/and an annotation from a
      Parameter, pass :attr:`Parameter.empty`.

      ::

         >>> from inspect import Parameter
         >>> param = Parameter('foo', Parameter.KEYWORD_ONLY, default=42)
         >>> str(param)
         'foo=42'

         >>> str(param.replace()) # Will create a shallow copy of 'param'
         'foo=42'

         >>> str(param.replace(default=Parameter.empty, annotation='spam'))
         "foo:'spam'"

    .. versionchanged:: 3.4
        In Python 3.3 Parameter objects were allowed to have ``name`` set
        to ``None`` if their ``kind`` was set to ``POSITIONAL_ONLY``.
        This is no longer permitted.

.. class:: BoundArguments

   Result of a :meth:`Signature.bind` or :meth:`Signature.bind_partial` call.
   Holds the mapping of arguments to the function's parameters.

   .. attribute:: BoundArguments.arguments

      An ordered, mutable mapping (:class:`collections.OrderedDict`) of
      parameters' names to arguments' values.  Contains only explicitly bound
      arguments.  Changes in :attr:`arguments` will reflect in :attr:`args` and
      :attr:`kwargs`.

      Should be used in conjunction with :attr:`Signature.parameters` for any
      argument processing purposes.

      .. note::

         Arguments for which :meth:`Signature.bind` or
         :meth:`Signature.bind_partial` relied on a default value are skipped.
         However, if needed, it is easy to include them.

      ::

        >>> def foo(a, b=10):
        ...     pass

        >>> sig = signature(foo)
        >>> ba = sig.bind(5)

        >>> ba.args, ba.kwargs
        ((5,), {})

        >>> for param in sig.parameters.values():
        ...     if param.name not in ba.arguments:
        ...         ba.arguments[param.name] = param.default

        >>> ba.args, ba.kwargs
        ((5, 10), {})


   .. attribute:: BoundArguments.args

      A tuple of positional arguments values.  Dynamically computed from the
      :attr:`arguments` attribute.

   .. attribute:: BoundArguments.kwargs

      A dict of keyword arguments values.  Dynamically computed from the
      :attr:`arguments` attribute.

   The :attr:`args` and :attr:`kwargs` properties can be used to invoke
   functions::

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


.. function:: getargspec(func)

   Get the names and default values of a Python function's arguments. A
   :term:`named tuple` ``ArgSpec(args, varargs, keywords, defaults)`` is
   returned. *args* is a list of the argument names. *varargs* and *keywords*
   are the names of the ``*`` and ``**`` arguments or ``None``. *defaults* is a
   tuple of default argument values or ``None`` if there are no default
   arguments; if this tuple has *n* elements, they correspond to the last
   *n* elements listed in *args*.

   .. deprecated:: 3.0
      Use :func:`getfullargspec` instead, which provides information about
      keyword-only arguments and annotations.


.. function:: getfullargspec(func)

   Get the names and default values of a Python function's arguments.  A
   :term:`named tuple` is returned:

   ``FullArgSpec(args, varargs, varkw, defaults, kwonlyargs, kwonlydefaults,
   annotations)``

   *args* is a list of the argument names.  *varargs* and *varkw* are the names
   of the ``*`` and ``**`` arguments or ``None``.  *defaults* is an *n*-tuple
   of the default values of the last *n* arguments, or ``None`` if there are no
   default arguments.  *kwonlyargs* is a list of
   keyword-only argument names.  *kwonlydefaults* is a dictionary mapping names
   from kwonlyargs to defaults.  *annotations* is a dictionary mapping argument
   names to annotations.

   The first four items in the tuple correspond to :func:`getargspec`.

   .. note::
      Consider using the new :ref:`Signature Object <inspect-signature-object>`
      interface, which provides a better way of introspecting functions.

   .. versionchanged:: 3.4
      This function is now based on :func:`signature`, but still ignores
      ``__wrapped__`` attributes and includes the already bound first
      parameter in the signature output for bound methods.


.. function:: getargvalues(frame)

   Get information about arguments passed into a particular frame.  A
   :term:`named tuple` ``ArgInfo(args, varargs, keywords, locals)`` is
   returned. *args* is a list of the argument names.  *varargs* and *keywords*
   are the names of the ``*`` and ``**`` arguments or ``None``.  *locals* is the
   locals dictionary of the given frame.


.. function:: formatargspec(args[, varargs, varkw, defaults, kwonlyargs, kwonlydefaults, annotations[, formatarg, formatvarargs, formatvarkw, formatvalue, formatreturns, formatannotations]])

   Format a pretty argument spec from the values returned by
   :func:`getargspec` or :func:`getfullargspec`.

   The first seven arguments are (``args``, ``varargs``, ``varkw``,
   ``defaults``, ``kwonlyargs``, ``kwonlydefaults``, ``annotations``). The
   other five arguments are the corresponding optional formatting functions
   that are called to turn names and values into strings. The last argument
   is an optional function to format the sequence of arguments. For example::

    >>> from inspect import formatargspec, getfullargspec
    >>> def f(a: int, b: float):
    ...     pass
    ...
    >>> formatargspec(*getfullargspec(f))
    '(a: int, b: float)'


.. function:: formatargvalues(args[, varargs, varkw, locals, formatarg, formatvarargs, formatvarkw, formatvalue])

   Format a pretty argument spec from the four values returned by
   :func:`getargvalues`.  The format\* arguments are the corresponding optional
   formatting functions that are called to turn names and values into strings.


.. function:: getmro(cls)

   Return a tuple of class cls's base classes, including cls, in method resolution
   order.  No class appears more than once in this tuple. Note that the method
   resolution order depends on cls's type.  Unless a very peculiar user-defined
   metatype is in use, cls will be the first element of the tuple.


.. function:: getcallargs(func, *args, **kwds)

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
    >>> getcallargs(f, 1, 2, 3) == {'a': 1, 'named': {}, 'b': 2, 'pos': (3,)}
    True
    >>> getcallargs(f, a=2, x=4) == {'a': 2, 'named': {'x': 4}, 'b': 1, 'pos': ()}
    True
    >>> getcallargs(f)
    Traceback (most recent call last):
    ...
    TypeError: f() missing 1 required positional argument: 'a'

   .. versionadded:: 3.2

   .. note::
      Consider using the new :meth:`Signature.bind` instead.


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

   If you want to keep the frame around (for example to print a traceback
   later), you can also break reference cycles by using the
   :meth:`frame.clear` method.

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
can be inconvenient. :func:`getattr_static` has the same signature as :func:`getattr`
but avoids executing code when it fetches attributes.

.. function:: getattr_static(obj, attr, default=None)

   Retrieve attributes without triggering dynamic lookup via the
   descriptor protocol, :meth:`__getattr__` or :meth:`__getattribute__`.

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


Current State of a Generator
----------------------------

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


.. _inspect-module-cli:

Command Line Interface
----------------------

The :mod:`inspect` module also provides a basic introspection capability
from the command line.

.. program:: inspect

By default, accepts the name of a module and prints the source of that
module. A class or function within the module can be printed instead by
appended a colon and the qualified name of the target object.

.. cmdoption:: --details

   Print information about the specified object rather than the source code
