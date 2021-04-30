.. _annotations-howto:

**************************
Annotations Best Practices
**************************

:author: Larry Hastings

.. topic:: Abstract

  This document is designed to encapsulate the best practices
  for working with annotations.  If you write Python code that
  interacts with annotations, we encourage you to follow the
  guidelines described below.

  The document is organized into three sections:
  best practices for accessing the annotations of an object
  in Python versions 3.10 and newer,
  best practices for accessing the annotations of an object
  in Python versions 3.9 and older,
  and
  other best practices
  for ``__annotations__`` that apply to any Python version.


Accessing The Annotations Dict Of An Object In Python 3.10 And Newer
====================================================================

  Python 3.10 adds a new function to the standard library:
  :func:`inspect.get_annotations`.  In Python versions 3.10
  and newer, calling this function is the best practice for
  accessing the annotations dict of any object that supports
  annotations.  This function can also "un-stringize"
  stringized annotations for you.

  If for some reason :func:`inspect.get_annotations` isn't
  viable for your use case, you may access the
  ``__annotations__`` data member manually.  Best practice
  for this changed in Python 3.10 as well: as of Python 3.10,
  ``o.__annotations__`` is guaranteed to *always* work
  on Python functions, classes, and modules.  If you're
  certain the object you're examining is one of these three
  *specific* objects, you may simply use ``o.__annotations__``
  to get at the object's annotations dict.

  However, other types of callables--for example,
  callables created by :func:`functools.partial`--may
  not have an ``__annotations__`` attribute defined.  When
  accessing the ``__annotations__`` of a possibly unknown
  object,  best practice in Python versions 3.10 and
  newer is to call :func:`getattr` with three arguments,
  for example ``getattr(o, '__annotations__', None)``.


Accessing The Annotations Dict Of An Object In Python 3.9 And Older
===================================================================

  In Python 3.9 and older, accessing the annotations dict
  of an object is much more complicated than in newer versions.
  The problem is a design flaw in these older versions of Python,
  specifically to do with class annotations.

  Best practice for accessing the annotations dict of other
  objects--functions, other callables, and modules--is the same
  as best practice for 3.10, assuming you aren't calling
  :func:`inspect.get_annotations`: you should use three-argument
  :func:`getattr` to access the object's ``__annotations__``
  attribute.

  Unfortunately, this isn't best practice for classes.  The problem
  is that, since ``__annotations__`` is optional on classes, and
  because classes can inherit attributes from their base classes,
  accessing the ``__annotations__`` attribute of a class may
  inadvertently return the annotations dict of a *base class.*
  As an example::

      class Base:
          a:int = 3
          b:str = 'abc'

      class Derived(Base):
          pass

      print(Derived.__annotations__)

  This will print the annotations dict from *``Base``*, not
  *``Derived``*.

  Your code will have to have a separate code path if the object
  you're examining is a class (``isinstance(o, type)``).
  In that case, best practice relies on an implementation detail
  of Python 3.9 and before: if a class has annotations defined,
  they are stored in the class's ``__dict__`` dictionary.  Since
  the class may or may not have annotations defined, best practice
  is to call the ``get`` method on the class dict.

  To put it all together, here is some sample code that safely
  accesses the ``__annotations__`` attribute on an arbitrary
  object in Python 3.9 and before::

      if isinstance(o, type):
          ann = o.__dict__.get('__annotations__', None)
      else:
          ann = getattr(o, '__annotations__', None)

  After running this code, ``ann`` should be either a
  dictionary or ``None``.  You're encouraged to double-check
  the type of ``ann`` using :func:`isinstance` before further
  examination.

  Note that some exotic or malformed type objects may not have
  a ``__dict__`` attribute, so for extra safety you may also wish
  to use :func:`getattr` to access ``__dict__``.


Manually Un-Stringizing Stringized Annotations
==============================================

  In situations where some annotations may be "stringized",
  and you wish to evaluate those strings to produce the
  Python values they represent, it really is best to
  call :func:`inspect.get_annotations` to do the evaluations
  for you.  Un-stringizing is a complicated process, and
  it's best to let :func:`inspect.get_annotations` do it
  for you.

  If you are using Python 3.9 or older, or if for some reason
  you cannot use :func:`inspect.get_annotations`, you'll need
  to duplicate its logic.  You're encouraged to examine the
  implementation of :func:`inspect.get_annotations` in the
  current Python version and follow a similar approach.

  In a nutshell, if you wish to evaluate a stringized annotation
  on an arbitrary object ``o``:

  * If ``o`` is a module, use ``o.__dict__`` as the
    ``globals`` when calling :func:`eval`.
  * If ``o`` is a class, use ``sys.modules[o.__module__].__dict__``
    as the ``globals``, and ``dict(vars(o))`` as the ``locals``,
    when calling :func:`eval`.
  * If ``o`` is a wrapped callable using :func:`functools.update_wrapper`,
    :func:`functools.wraps`, or :func:`functools.partial`, iteratively
    unwrap it by accessing either ``o.__wrapped__`` or ``o.func`` as
    appropriate, until you have found the root unwrapped function.
  * If ``o`` is a callable (but not a class), use
    ``o.__globals__`` as the globals when calling :func:`eval`.


Best Practices For ``__annotations__`` In Any Python Version
============================================================

  * You should avoid assigning to the ``__annotations__`` member
    of objects directly.  Let Python manage setting ``__annotations__``.

  * If you do assign directly to the ``__annotations__`` member
    of an object, you should always set it to a ``dict`` object.

  * If you directly access the ``__annotations__`` member
    of an object, you should ensure that it's a
    dictionary before attempting to examine its contents.

  * You should avoid modifying ``__annotations__`` dicts.

  * You should avoid deleting the ``__annotations__`` attribute
    of an object.
