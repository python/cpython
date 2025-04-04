:mod:`!annotationlib` --- Functionality for introspecting annotations
=====================================================================

.. module:: annotationlib
   :synopsis: Functionality for introspecting annotations


**Source code:** :source:`Lib/annotationlib.py`

.. testsetup:: default

   import annotationlib
   from annotationlib import *

--------------

The :mod:`!annotationlib` module provides tools for introspecting
:term:`annotations <annotation>` on modules, classes, and functions.

Annotations are :ref:`lazily evaluated <lazy-evaluation>` and often contain
forward references to objects that are not yet defined when the annotation
is created. This module provides a set of low-level tools that can be used to retrieve annotations in a reliable way, even
in the presence of forward references and other edge cases.

This module supports retrieving annotations in three main formats
(see :class:`Format`), each of which works best for different use cases:

* :attr:`~Format.VALUE` evaluates the annotations and returns their value.
  This is most straightforward to work with, but it may raise errors,
  for example if the annotations contain references to undefined names.
* :attr:`~Format.FORWARDREF` returns :class:`ForwardRef` objects
  for annotations that cannot be resolved, allowing you to inspect the
  annotations without evaluating them. This is useful when you need to
  work with annotations that may contain unresolved forward references.
* :attr:`~Format.STRING` returns the annotations as a string, similar
  to how it would appear in the source file. This is useful for documentation
  generators that want to display annotations in a readable way.

The :func:`get_annotations` function is the main entry point for
retrieving annotations. Given a function, class, or module, it returns
an annotations dictionary in the requested format. This module also provides
functionality for working directly with the :term:`annotate function`
that is used to evaluate annotations, such as :func:`get_annotate_function`
and :func:`call_annotate_function`, as well as the
:func:`call_evaluate_function` function for working with
:term:`evaluate functions <evaluate function>`.


.. seealso::

   :pep:`649` proposed the current model for how annotations work in Python.

   :pep:`749` expanded on various aspects of :pep:`649` and introduced the
   :mod:`!annotationlib` module.

   :ref:`annotations-howto` provides best practices for working with
   annotations.

   :pypi:`typing-extensions` provides a backport of :func:`get_annotations`
   that works on earlier versions of Python.

Annotation semantics
--------------------

The way annotations are evaluated has changed over the history of Python 3,
and currently still depends on a :ref:`future import <future>`.
There have been execution models for annotations:

* *Stock semantics* (default in Python 3.0 through 3.13; see :pep:`3107`
  and :pep:`526`): Annotations are evaluated eagerly, as they are
  encountered in the source code.
* *Stringified annotations* (used with ``from __future__ import annotations``
  in Python 3.7 and newer; see :pep:`563`): Annotations are stored as
  strings only.
* *Deferred evaluation* (default in Python 3.14 and newer; see :pep:`649` and
  :pep:`749`): Annotations are evaluated lazily, only when they are accessed.

As an example, consider the following program::

   def func(a: Cls) -> None:
       print(a)

   class Cls: pass

   print(func.__annotations__)

This will behave as follows:

* Under stock semantics (Python 3.13 and earlier), it will throw a
  :exc:`NameError` at the line where ``func`` is defined,
  because ``Cls`` is an undefined name at that point.
* Under stringified annotations (if ``from __future__ import annotations``
  is used), it will print ``{'a': 'Cls', 'return': 'None'}``.
* Under deferred evaluation (Python 3.14 and later), it will print
  ``{'a': <class 'Cls'>, 'return': None}``.

Stock semantics were used when function annotations were first introduced
in Python 3.0 (by :pep:`3107`) because this was the simplest, most obvious
way to implement annotations. The same execution model was used when variable
annotations were introduced in Python 3.6 (by :pep:`526`). However,
stock semantics caused problems when using annotations as type hints,
such as a need to refer to names that are not yet defined when the
annotation is encountered. In addition, there were performance problems
with executing annotations at module import time. Therefore, in Python 3.7,
:pep:`563` introduced the ability to store annotations as strings using the
``from __future__ import annotations`` syntax. The plan at the time was to
eventually make this behavior the default, but a problem appeared:
stringified annotations are more difficult to process for those who
introspect annotations at runtime. An alternative proposal, :pep:`649`,
introduced the third execution model, deferred evaluation, and was implemented
in Python 3.14. Stringified annotations are still used if
``from __future__ import annotations`` is present, but this behavior will
eventually be removed.

Classes
-------

.. class:: Format

   An :class:`~enum.IntEnum` describing the formats in which annotations
   can be returned. Members of the enum, or their equivalent integer values,
   can be passed to :func:`get_annotations` and other functions in this
   module, as well as to :attr:`~object.__annotate__` functions.

   .. attribute:: VALUE
      :value: 1

      Values are the result of evaluating the annotation expressions.

   .. attribute:: FORWARDREF
      :value: 2

      Values are real annotation values (as per :attr:`Format.VALUE` format)
      for defined values, and :class:`ForwardRef` proxies for undefined
      values. Real objects may contain references to, :class:`ForwardRef`
      proxy objects.

   .. attribute:: STRING
      :value: 3

      Values are the text string of the annotation as it appears in the
      source code, up to modifications including, but not restricted to,
      whitespace normalizations and constant values optimizations.

      The exact values of these strings may change in future versions of Python.

   .. attribute:: VALUE_WITH_FAKE_GLOBALS
      :value: 4

      Special value used to signal that an annotate function is being
      evaluated in a special environment with fake globals. When passed this
      value, annotate functions should either return the same value as for
      the :attr:`Format.VALUE` format, or raise :exc:`NotImplementedError`
      to signal that they do not support execution in this environment.
      This format is only used internally and should not be passed to
      the functions in this module.

   .. versionadded:: 3.14

.. class:: ForwardRef

   A proxy object for forward references in annotations.

   Instances of this class are returned when the :attr:`~Format.FORWARDREF`
   format is used and annotations contain a name that cannot be resolved.
   This can happen when a forward reference is used in an annotation, such as
   when a class is referenced before it is defined.

   .. attribute:: __forward_arg__

      A string containing the code that was evaluated to produce the
      :class:`~ForwardRef`. The string may not be exactly equivalent
      to the original source.

   .. method:: evaluate(*, owner=None, globals=None, locals=None, type_params=None)

      Evaluate the forward reference, returning its value.

      This may throw an exception, such as :exc:`NameError`, if the forward
      reference refers to a name that cannot be resolved. The arguments to this
      method can be used to provide bindings for names that would otherwise
      be undefined.

      The *owner* parameter provides the preferred mechanism for passing scope
      information to this method. The owner of a :class:`~ForwardRef` is the
      object that contains the annotation from which the :class:`~ForwardRef`
      derives, such as a module object, type object, or function object.

      The *globals*, *locals*, and *type_params* parameters provide a more precise
      mechanism for influencing the names that are available when the :class:`~ForwardRef`
      is evaluated. *globals* and *locals* are passed to :func:`eval`, representing
      the global and local namespaces in which the name is evaluated.
      The *type_params* parameter is relevant for objects created using the native
      syntax for :ref:`generic classes <generic-classes>` and :ref:`functions <generic-functions>`.
      It is a tuple of :ref:`type parameters <type-params>` that are in scope
      while the forward reference is being evaluated. For example, if evaluating a
      :class:`~ForwardRef` retrieved from an annotation found in the class namespace
      of a generic class ``C``, *type_params* should be set to ``C.__type_params__``.

      :class:`~ForwardRef` instances returned by :func:`get_annotations`
      retain references to information about the scope they originated from,
      so calling this method with no further arguments may be sufficient to
      evaluate such objects. :class:`~ForwardRef` instances created by other
      means may not have any information about their scope, so passing
      arguments to this method may be necessary to evaluate them successfully.

   .. versionadded:: 3.14


Functions
---------

.. function:: annotations_to_string(annotations)

   Convert an annotations dict containing runtime values to a
   dict containing only strings. If the values are not already strings,
   they are converted using :func:`value_to_string`.
   This is meant as a helper for user-provided
   annotate functions that support the :attr:`~Format.STRING` format but
   do not have access to the code creating the annotations.

   For example, this is used to implement the :attr:`~Format.STRING` for
   :class:`typing.TypedDict` classes created through the functional syntax:

   .. doctest::

       >>> from typing import TypedDict
       >>> Movie = TypedDict("movie", {"name": str, "year": int})
       >>> get_annotations(Movie, format=Format.STRING)
       {'name': 'str', 'year': 'int'}

   .. versionadded:: 3.14

.. function:: call_annotate_function(annotate, format, *, owner=None)

   Call the :term:`annotate function` *annotate* with the given *format*,
   a member of the :class:`Format` enum, and return the annotations
   dictionary produced by the function.

   This helper function is required because annotate functions generated by
   the compiler for functions, classes, and modules only support
   the :attr:`~Format.VALUE` format when called directly.
   To support other formats, this function calls the annotate function
   in a special environment that allows it to produce annotations in the
   other formats. This is a useful building block when implementing
   functionality that needs to partially evaluate annotations while a class
   is being constructed.

   *owner* is the object that owns the annotation function, usually
   a function, class, or module. If provided, it is used in the
   :attr:`~Format.FORWARDREF` format to produce a :class:`ForwardRef`
   object that carries more information.

   .. seealso::

      :PEP:`PEP 649 <649#the-stringizer-and-the-fake-globals-environment>`
      contains an explanation of the implementation technique used by this
      function.

   .. versionadded:: 3.14

.. function:: call_evaluate_function(evaluate, format, *, owner=None)

   Call the :term:`evaluate function` *evaluate* with the given *format*,
   a member of the :class:`Format` enum, and return the value produced by
   the function. This is similar to :func:`call_annotate_function`,
   but the latter always returns a dictionary mapping strings to annotations,
   while this function returns a single value.

   This is intended for use with the evaluate functions generated for lazily
   evaluated elements related to type aliases and type parameters:

   * :meth:`typing.TypeAliasType.evaluate_value`, the value of type aliases
   * :meth:`typing.TypeVar.evaluate_bound`, the bound of type variables
   * :meth:`typing.TypeVar.evaluate_constraints`, the constraints of
     type variables
   * :meth:`typing.TypeVar.evaluate_default`, the default value of
     type variables
   * :meth:`typing.ParamSpec.evaluate_default`, the default value of
     parameter specifications
   * :meth:`typing.TypeVarTuple.evaluate_default`, the default value of
     type variable tuples

   *owner* is the object that owns the evaluate function, such as the type
   alias or type variable object.

   *format* can be used to control the format in which the value is returned:

   .. doctest::

      >>> type Alias = undefined
      >>> call_evaluate_function(Alias.evaluate_value, Format.VALUE)
      Traceback (most recent call last):
      ...
      NameError: name 'undefined' is not defined
      >>> call_evaluate_function(Alias.evaluate_value, Format.FORWARDREF)
      ForwardRef('undefined')
      >>> call_evaluate_function(Alias.evaluate_value, Format.STRING)
      'undefined'

   .. versionadded:: 3.14

.. function:: get_annotate_function(obj)

   Retrieve the :term:`annotate function` for *obj*. Return :const:`!None`
   if *obj* does not have an annotate function.

   This is usually equivalent to accessing the :attr:`~object.__annotate__`
   attribute of *obj*, but direct access to the attribute may return the wrong
   object in certain situations involving metaclasses. This function should be
   used instead of accessing the attribute directly.

   .. versionadded:: 3.14

.. function:: get_annotations(obj, *, globals=None, locals=None, eval_str=False, format=Format.VALUE)

   Compute the annotations dict for an object.

   *obj* may be a callable, class, module, or other object with
   :attr:`~object.__annotate__` and :attr:`~object.__annotations__` attributes.
   Passing in an object of any other type raises :exc:`TypeError`.

   The *format* parameter controls the format in which annotations are returned,
   and must be a member of the :class:`Format` enum or its integer equivalent.

   Returns a dict. :func:`!get_annotations` returns a new dict every time
   it's called; calling it twice on the same object will return two
   different but equivalent dicts.

   This function handles several details for you:

   * If *eval_str* is true, values of type :class:`!str` will
     be un-stringized using :func:`eval`. This is intended
     for use with stringized annotations
     (``from __future__ import annotations``). It is an error
     to set *eval_str* to true with formats other than :attr:`Format.VALUE`.
   * If *obj* doesn't have an annotations dict, returns an
     empty dict. (Functions and methods always have an
     annotations dict; classes, modules, and other types of
     callables may not.)
   * Ignores inherited annotations on classes, as well as annotations
     on metaclasses. If a class
     doesn't have its own annotations dict, returns an empty dict.
   * All accesses to object members and dict values are done
     using ``getattr()`` and ``dict.get()`` for safety.

   *eval_str* controls whether or not values of type :class:`!str` are
   replaced with the result of calling :func:`eval` on those values:

   * If eval_str is true, :func:`eval` is called on values of type
     :class:`!str`. (Note that :func:`!get_annotations` doesn't catch
     exceptions; if :func:`eval` raises an exception, it will unwind
     the stack past the :func:`!get_annotations` call.)
   * If *eval_str* is false (the default), values of type :class:`!str` are
     unchanged.

   *globals* and *locals* are passed in to :func:`eval`; see the documentation
   for :func:`eval` for more information. If *globals* or *locals*
   is :const:`!None`, this function may replace that value with a
   context-specific default, contingent on ``type(obj)``:

   * If *obj* is a module, *globals* defaults to ``obj.__dict__``.
   * If *obj* is a class, *globals* defaults to
     ``sys.modules[obj.__module__].__dict__`` and *locals* defaults
     to the *obj* class namespace.
   * If *obj* is a callable, *globals* defaults to
     :attr:`obj.__globals__ <function.__globals__>`,
     although if *obj* is a wrapped function (using
     :func:`functools.update_wrapper`) or a :class:`functools.partial` object,
     it is unwrapped until a non-wrapped function is found.

   Calling :func:`!get_annotations` is best practice for accessing the
   annotations dict of any object. See :ref:`annotations-howto` for
   more information on annotations best practices.

   .. doctest::

      >>> def f(a: int, b: str) -> float:
      ...     pass
      >>> get_annotations(f)
      {'a': <class 'int'>, 'b': <class 'str'>, 'return': <class 'float'>}

   .. versionadded:: 3.14

.. function:: value_to_string(value)

   Convert an arbitrary Python value to a format suitable for use by the
   :attr:`~Format.STRING` format. This calls :func:`repr` for most
   objects, but has special handling for some objects, such as type objects.

   This is meant as a helper for user-provided
   annotate functions that support the :attr:`~Format.STRING` format but
   do not have access to the code creating the annotations. It can also
   be used to provide a user-friendly string representation for other
   objects that contain values that are commonly encountered in annotations.

   .. versionadded:: 3.14

