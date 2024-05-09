:mod:`!pprint` --- Data pretty printer
======================================

.. module:: pprint
   :synopsis: Data pretty printer.

.. moduleauthor:: Fred L. Drake, Jr. <fdrake@acm.org>
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>

**Source code:** :source:`Lib/pprint.py`

--------------

The :mod:`pprint` module provides a capability to "pretty-print" arbitrary
Python data structures in a form which can be used as input to the interpreter.
If the formatted structures include objects which are not fundamental Python
types, the representation may not be loadable.  This may be the case if objects
such as files, sockets or classes are included, as well as many other
objects which are not representable as Python literals.

The formatted representation keeps objects on a single line if it can, and
breaks them onto multiple lines if they don't fit within the allowed width,
adjustable by the *width* parameter defaulting to 80 characters.

Dictionaries are sorted by key before the display is computed.

.. versionchanged:: 3.9
   Added support for pretty-printing :class:`types.SimpleNamespace`.

.. versionchanged:: 3.10
   Added support for pretty-printing :class:`dataclasses.dataclass`.

.. _pprint-functions:

Functions
---------

.. function:: pp(object, *args, sort_dicts=False, **kwargs)

   Prints the formatted representation of *object* followed by a newline.
   If *sort_dicts* is false (the default), dictionaries will be displayed with
   their keys in insertion order, otherwise the dict keys will be sorted.
   *args* and *kwargs* will be passed to :func:`~pprint.pprint` as formatting
   parameters.

      >>> import pprint
      >>> stuff = ['spam', 'eggs', 'lumberjack', 'knights', 'ni']
      >>> stuff.insert(0, stuff)
      >>> pprint.pp(stuff)
      [<Recursion on list with id=...>,
       'spam',
       'eggs',
       'lumberjack',
       'knights',
       'ni']

   .. versionadded:: 3.8


.. function:: pprint(object, stream=None, indent=1, width=80, depth=None, *, \
                     compact=False, sort_dicts=True, underscore_numbers=False)

   Prints the formatted representation of *object* on *stream*, followed by a
   newline.  If *stream* is ``None``, :data:`sys.stdout` is used. This may be used
   in the interactive interpreter instead of the :func:`print` function for
   inspecting values (you can even reassign ``print = pprint.pprint`` for use
   within a scope).

   The configuration parameters *stream*, *indent*, *width*, *depth*,
   *compact*, *sort_dicts* and *underscore_numbers* are passed to the
   :class:`PrettyPrinter` constructor and their meanings are as
   described in its documentation below.

   Note that *sort_dicts* is ``True`` by default and you might want to use
   :func:`~pprint.pp` instead where it is ``False`` by default.

.. function:: pformat(object, indent=1, width=80, depth=None, *, \
                      compact=False, sort_dicts=True, underscore_numbers=False)

   Return the formatted representation of *object* as a string.  *indent*,
   *width*, *depth*, *compact*, *sort_dicts* and *underscore_numbers* are
   passed to the :class:`PrettyPrinter` constructor as formatting parameters
   and their meanings are as described in its documentation below.


.. function:: isreadable(object)

   .. index:: pair: built-in function; eval

   Determine if the formatted representation of *object* is "readable", or can be
   used to reconstruct the value using :func:`eval`.  This always returns ``False``
   for recursive objects.

      >>> pprint.isreadable(stuff)
      False


.. function:: isrecursive(object)

   Determine if *object* requires a recursive representation.  This function is
   subject to the same limitations as noted in :func:`saferepr` below and may raise an
   :exc:`RecursionError` if it fails to detect a recursive object.


.. function:: saferepr(object)

   Return a string representation of *object*, protected against recursion in
   some common data structures, namely instances of :class:`dict`, :class:`list`
   and :class:`tuple` or subclasses whose ``__repr__`` has not been overridden.  If the
   representation of object exposes a recursive entry, the recursive reference
   will be represented as ``<Recursion on typename with id=number>``.  The
   representation is not otherwise formatted.

   >>> pprint.saferepr(stuff)
   "[<Recursion on list with id=...>, 'spam', 'eggs', 'lumberjack', 'knights', 'ni']"

.. _prettyprinter-objects:

PrettyPrinter Objects
---------------------

This module defines one class:

.. First the implementation class:


.. index:: single: ...; placeholder

.. class:: PrettyPrinter(indent=1, width=80, depth=None, stream=None, *, \
                         compact=False, sort_dicts=True, underscore_numbers=False)

   Construct a :class:`PrettyPrinter` instance.  This constructor understands
   several keyword parameters.

   *stream* (default :data:`!sys.stdout`) is a :term:`file-like object` to
   which the output will be written by calling its :meth:`!write` method.
   If both *stream* and :data:`!sys.stdout` are ``None``, then
   :meth:`~PrettyPrinter.pprint` silently returns.

   Other values configure the manner in which nesting of complex data
   structures is displayed.

   *indent* (default 1) specifies the amount of indentation added for
   each nesting level.

   *depth* controls the number of nesting levels which may be printed; if
   the data structure being printed is too deep, the next contained level
   is replaced by ``...``.  By default, there is no constraint on the
   depth of the objects being formatted.

   *width* (default 80) specifies the desired maximum number of characters per
   line in the output. If a structure cannot be formatted within the width
   constraint, a best effort will be made.

   *compact* impacts the way that long sequences (lists, tuples, sets, etc)
   are formatted. If *compact* is false (the default) then each item of a
   sequence will be formatted on a separate line.  If *compact* is true, as
   many items as will fit within the *width* will be formatted on each output
   line.

   If *sort_dicts* is true (the default), dictionaries will be formatted with
   their keys sorted, otherwise they will display in insertion order.

   If *underscore_numbers* is true, integers will be formatted with the
   ``_`` character for a thousands separator, otherwise underscores are not
   displayed (the default).

   .. versionchanged:: 3.4
      Added the *compact* parameter.

   .. versionchanged:: 3.8
      Added the *sort_dicts* parameter.

   .. versionchanged:: 3.10
      Added the *underscore_numbers* parameter.

   .. versionchanged:: 3.11
      No longer attempts to write to :data:`!sys.stdout` if it is ``None``.

      >>> import pprint
      >>> stuff = ['spam', 'eggs', 'lumberjack', 'knights', 'ni']
      >>> stuff.insert(0, stuff[:])
      >>> pp = pprint.PrettyPrinter(indent=4)
      >>> pp.pprint(stuff)
      [   ['spam', 'eggs', 'lumberjack', 'knights', 'ni'],
          'spam',
          'eggs',
          'lumberjack',
          'knights',
          'ni']
      >>> pp = pprint.PrettyPrinter(width=41, compact=True)
      >>> pp.pprint(stuff)
      [['spam', 'eggs', 'lumberjack',
        'knights', 'ni'],
       'spam', 'eggs', 'lumberjack', 'knights',
       'ni']
      >>> tup = ('spam', ('eggs', ('lumberjack', ('knights', ('ni', ('dead',
      ... ('parrot', ('fresh fruit',))))))))
      >>> pp = pprint.PrettyPrinter(depth=6)
      >>> pp.pprint(tup)
      ('spam', ('eggs', ('lumberjack', ('knights', ('ni', ('dead', (...)))))))


:class:`PrettyPrinter` instances have the following methods:


.. method:: PrettyPrinter.pformat(object)

   Return the formatted representation of *object*.  This takes into account the
   options passed to the :class:`PrettyPrinter` constructor.


.. method:: PrettyPrinter.pprint(object)

   Print the formatted representation of *object* on the configured stream,
   followed by a newline.

The following methods provide the implementations for the corresponding
functions of the same names.  Using these methods on an instance is slightly
more efficient since new :class:`PrettyPrinter` objects don't need to be
created.


.. method:: PrettyPrinter.isreadable(object)

   .. index:: pair: built-in function; eval

   Determine if the formatted representation of the object is "readable," or can be
   used to reconstruct the value using :func:`eval`.  Note that this returns
   ``False`` for recursive objects.  If the *depth* parameter of the
   :class:`PrettyPrinter` is set and the object is deeper than allowed, this
   returns ``False``.


.. method:: PrettyPrinter.isrecursive(object)

   Determine if the object requires a recursive representation.

This method is provided as a hook to allow subclasses to modify the way objects
are converted to strings.  The default implementation uses the internals of the
:func:`saferepr` implementation.


.. method:: PrettyPrinter.format(object, context, maxlevels, level)

   Returns three values: the formatted version of *object* as a string, a flag
   indicating whether the result is readable, and a flag indicating whether
   recursion was detected.  The first argument is the object to be presented.  The
   second is a dictionary which contains the :func:`id` of objects that are part of
   the current presentation context (direct and indirect containers for *object*
   that are affecting the presentation) as the keys; if an object needs to be
   presented which is already represented in *context*, the third return value
   should be ``True``.  Recursive calls to the :meth:`.format` method should add
   additional entries for containers to this dictionary.  The third argument,
   *maxlevels*, gives the requested limit to recursion; this will be ``0`` if there
   is no requested limit.  This argument should be passed unmodified to recursive
   calls. The fourth argument, *level*, gives the current level; recursive calls
   should be passed a value less than that of the current call.


.. _pprint-example:

Example
-------

To demonstrate several uses of the :func:`~pprint.pp` function and its parameters,
let's fetch information about a project from `PyPI <https://pypi.org>`_::

   >>> import json
   >>> import pprint
   >>> from urllib.request import urlopen
   >>> with urlopen('https://pypi.org/pypi/sampleproject/json') as resp:
   ...     project_info = json.load(resp)['info']

In its basic form, :func:`~pprint.pp` shows the whole object::

   >>> pprint.pp(project_info)
   {'author': 'The Python Packaging Authority',
    'author_email': 'pypa-dev@googlegroups.com',
    'bugtrack_url': None,
    'classifiers': ['Development Status :: 3 - Alpha',
                    'Intended Audience :: Developers',
                    'License :: OSI Approved :: MIT License',
                    'Programming Language :: Python :: 2',
                    'Programming Language :: Python :: 2.6',
                    'Programming Language :: Python :: 2.7',
                    'Programming Language :: Python :: 3',
                    'Programming Language :: Python :: 3.2',
                    'Programming Language :: Python :: 3.3',
                    'Programming Language :: Python :: 3.4',
                    'Topic :: Software Development :: Build Tools'],
    'description': 'A sample Python project\n'
                   '=======================\n'
                   '\n'
                   'This is the description file for the project.\n'
                   '\n'
                   'The file should use UTF-8 encoding and be written using '
                   'ReStructured Text. It\n'
                   'will be used to generate the project webpage on PyPI, and '
                   'should be written for\n'
                   'that purpose.\n'
                   '\n'
                   'Typical contents for this file would include an overview of '
                   'the project, basic\n'
                   'usage examples, etc. Generally, including the project '
                   'changelog in here is not\n'
                   'a good idea, although a simple "What\'s New" section for the '
                   'most recent version\n'
                   'may be appropriate.',
    'description_content_type': None,
    'docs_url': None,
    'download_url': 'UNKNOWN',
    'downloads': {'last_day': -1, 'last_month': -1, 'last_week': -1},
    'home_page': 'https://github.com/pypa/sampleproject',
    'keywords': 'sample setuptools development',
    'license': 'MIT',
    'maintainer': None,
    'maintainer_email': None,
    'name': 'sampleproject',
    'package_url': 'https://pypi.org/project/sampleproject/',
    'platform': 'UNKNOWN',
    'project_url': 'https://pypi.org/project/sampleproject/',
    'project_urls': {'Download': 'UNKNOWN',
                     'Homepage': 'https://github.com/pypa/sampleproject'},
    'release_url': 'https://pypi.org/project/sampleproject/1.2.0/',
    'requires_dist': None,
    'requires_python': None,
    'summary': 'A sample Python project',
    'version': '1.2.0'}

The result can be limited to a certain *depth* (ellipsis is used for deeper
contents)::

   >>> pprint.pp(project_info, depth=1)
   {'author': 'The Python Packaging Authority',
    'author_email': 'pypa-dev@googlegroups.com',
    'bugtrack_url': None,
    'classifiers': [...],
    'description': 'A sample Python project\n'
                   '=======================\n'
                   '\n'
                   'This is the description file for the project.\n'
                   '\n'
                   'The file should use UTF-8 encoding and be written using '
                   'ReStructured Text. It\n'
                   'will be used to generate the project webpage on PyPI, and '
                   'should be written for\n'
                   'that purpose.\n'
                   '\n'
                   'Typical contents for this file would include an overview of '
                   'the project, basic\n'
                   'usage examples, etc. Generally, including the project '
                   'changelog in here is not\n'
                   'a good idea, although a simple "What\'s New" section for the '
                   'most recent version\n'
                   'may be appropriate.',
    'description_content_type': None,
    'docs_url': None,
    'download_url': 'UNKNOWN',
    'downloads': {...},
    'home_page': 'https://github.com/pypa/sampleproject',
    'keywords': 'sample setuptools development',
    'license': 'MIT',
    'maintainer': None,
    'maintainer_email': None,
    'name': 'sampleproject',
    'package_url': 'https://pypi.org/project/sampleproject/',
    'platform': 'UNKNOWN',
    'project_url': 'https://pypi.org/project/sampleproject/',
    'project_urls': {...},
    'release_url': 'https://pypi.org/project/sampleproject/1.2.0/',
    'requires_dist': None,
    'requires_python': None,
    'summary': 'A sample Python project',
    'version': '1.2.0'}

Additionally, maximum character *width* can be suggested. If a long object
cannot be split, the specified width will be exceeded::

   >>> pprint.pp(project_info, depth=1, width=60)
   {'author': 'The Python Packaging Authority',
    'author_email': 'pypa-dev@googlegroups.com',
    'bugtrack_url': None,
    'classifiers': [...],
    'description': 'A sample Python project\n'
                   '=======================\n'
                   '\n'
                   'This is the description file for the '
                   'project.\n'
                   '\n'
                   'The file should use UTF-8 encoding and be '
                   'written using ReStructured Text. It\n'
                   'will be used to generate the project '
                   'webpage on PyPI, and should be written '
                   'for\n'
                   'that purpose.\n'
                   '\n'
                   'Typical contents for this file would '
                   'include an overview of the project, '
                   'basic\n'
                   'usage examples, etc. Generally, including '
                   'the project changelog in here is not\n'
                   'a good idea, although a simple "What\'s '
                   'New" section for the most recent version\n'
                   'may be appropriate.',
    'description_content_type': None,
    'docs_url': None,
    'download_url': 'UNKNOWN',
    'downloads': {...},
    'home_page': 'https://github.com/pypa/sampleproject',
    'keywords': 'sample setuptools development',
    'license': 'MIT',
    'maintainer': None,
    'maintainer_email': None,
    'name': 'sampleproject',
    'package_url': 'https://pypi.org/project/sampleproject/',
    'platform': 'UNKNOWN',
    'project_url': 'https://pypi.org/project/sampleproject/',
    'project_urls': {...},
    'release_url': 'https://pypi.org/project/sampleproject/1.2.0/',
    'requires_dist': None,
    'requires_python': None,
    'summary': 'A sample Python project',
    'version': '1.2.0'}
