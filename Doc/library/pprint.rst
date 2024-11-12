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

.. function:: pp(object, stream=None, indent=1, width=80, depth=None, *, \
                     compact=False, sort_dicts=False, underscore_numbers=False)

   Prints the formatted representation of *object*, followed by a newline.
   This function may be used in the interactive interpreter
   instead of the :func:`print` function for inspecting values.
   Tip: you can reassign ``print = pprint.pp`` for use within a scope.

   :param object:
      The object to be printed.

   :param stream:
      A file-like object to which the output will be written
      by calling its :meth:`!write` method.
      If ``None`` (the default), :data:`sys.stdout` is used.
   :type stream: :term:`file-like object` | None

   :param int indent:
      The amount of indentation added for each nesting level.

   :param int width:
      The desired maximum number of characters per line in the output.
      If a structure cannot be formatted within the width constraint,
      a best effort will be made.

   :param depth:
      The number of nesting levels which may be printed.
      If the data structure being printed is too deep,
      the next contained level is replaced by ``...``.
      If ``None`` (the default), there is no constraint
      on the depth of the objects being formatted.
   :type depth: int | None

   :param bool compact:
      Control the way long :term:`sequences <sequence>` are formatted.
      If ``False`` (the default),
      each item of a sequence will be formatted on a separate line,
      otherwise as many items as will fit within the *width*
      will be formatted on each output line.

   :param bool sort_dicts:
      If ``True``, dictionaries will be formatted with
      their keys sorted, otherwise
      they will be displayed in insertion order (the default).

   :param bool underscore_numbers:
      If ``True``,
      integers will be formatted with the ``_`` character for a thousands separator,
      otherwise underscores are not displayed (the default).

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

   Alias for :func:`~pprint.pp` with *sort_dicts* set to ``True`` by default,
   which would automatically sort the dictionaries' keys,
   you might want to use :func:`~pprint.pp` instead where it is ``False`` by default.


.. function:: pformat(object, indent=1, width=80, depth=None, *, \
                      compact=False, sort_dicts=True, underscore_numbers=False)

   Return the formatted representation of *object* as a string.  *indent*,
   *width*, *depth*, *compact*, *sort_dicts* and *underscore_numbers* are
   passed to the :class:`PrettyPrinter` constructor as formatting parameters
   and their meanings are as described in the documentation above.


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

.. index:: single: ...; placeholder

.. class:: PrettyPrinter(indent=1, width=80, depth=None, stream=None, *, \
                         compact=False, sort_dicts=True, underscore_numbers=False)

   Construct a :class:`PrettyPrinter` instance.

   Arguments have the same meaning as for :func:`~pprint.pp`.
   Note that they are in a different order, and that *sort_dicts* defaults to ``True``.

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


   .. versionchanged:: 3.4
      Added the *compact* parameter.

   .. versionchanged:: 3.8
      Added the *sort_dicts* parameter.

   .. versionchanged:: 3.10
      Added the *underscore_numbers* parameter.

   .. versionchanged:: 3.11
      No longer attempts to write to :data:`!sys.stdout` if it is ``None``.


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
   >>> with urlopen('https://pypi.org/pypi/sampleproject/4.0.0/json') as resp:
   ...     project_info = json.load(resp)['info']

In its basic form, :func:`~pprint.pp` shows the whole object::

   >>> pprint.pp(project_info)
   {'author': None,
   'author_email': '"A. Random Developer" <author@example.com>',
   'bugtrack_url': None,
   'classifiers': ['Development Status :: 3 - Alpha',
                  'Intended Audience :: Developers',
                  'License :: OSI Approved :: MIT License',
                  'Programming Language :: Python :: 3',
                  'Programming Language :: Python :: 3 :: Only',
                  'Programming Language :: Python :: 3.10',
                  'Programming Language :: Python :: 3.11',
                  'Programming Language :: Python :: 3.12',
                  'Programming Language :: Python :: 3.13',
                  'Programming Language :: Python :: 3.9',
                  'Topic :: Software Development :: Build Tools'],
   'description': '# A sample Python project\n'
                  '\n'
                  '![Python '
                  'Logo](https://www.python.org/static/community_logos/python-logo.png '
                  '"Sample inline image")\n'
                  '\n'
                  'A sample project that exists as an aid to the [Python '
                  'Packaging User\n'
                  "Guide][packaging guide]'s [Tutorial on Packaging and "
                  'Distributing\n'
                  'Projects][distribution tutorial].\n'
                  '\n'
                  'This project does not aim to cover best practices for Python '
                  'project\n'
                  'development as a whole. For example, it does not provide '
                  'guidance or tool\n'
                  'recommendations for version control, documentation, or '
                  'testing.\n'
                  '\n'
                  '[The source for this project is available here][src].\n'
                  '\n'
                  'The metadata for a Python project is defined in the '
                  '`pyproject.toml` file,\n'
                  'an example of which is included in this project. You should '
                  'edit this file\n'
                  'accordingly to adapt this sample project to your needs.\n'
                  '\n'
                  '----\n'
                  '\n'
                  'This is the README file for the project.\n'
                  '\n'
                  'The file should use UTF-8 encoding and can be written using\n'
                  '[reStructuredText][rst] or [markdown][md use] with the '
                  'appropriate [key set][md\n'
                  'use]. It will be used to generate the project webpage on PyPI '
                  'and will be\n'
                  'displayed as the project homepage on common code-hosting '
                  'services, and should be\n'
                  'written for that purpose.\n'
                  '\n'
                  'Typical contents for this file would include an overview of '
                  'the project, basic\n'
                  'usage examples, etc. Generally, including the project '
                  'changelog in here is not a\n'
                  "good idea, although a simple “What's New” section for the "
                  'most recent version\n'
                  'may be appropriate.\n'
                  '\n'
                  '[packaging guide]: https://packaging.python.org\n'
                  '[distribution tutorial]: '
                  'https://packaging.python.org/tutorials/packaging-projects/\n'
                  '[src]: https://github.com/pypa/sampleproject\n'
                  '[rst]: http://docutils.sourceforge.net/rst.html\n'
                  '[md]: https://tools.ietf.org/html/rfc7764#section-3.5 '
                  '"CommonMark variant"\n'
                  '[md use]: '
                  'https://packaging.python.org/specifications/core-metadata/#description-content-type-optional\n',
   'description_content_type': 'text/markdown',
   'docs_url': None,
   'download_url': None,
   'downloads': {'last_day': -1, 'last_month': -1, 'last_week': -1},
   'dynamic': None,
   'home_page': None,
   'keywords': 'sample, setuptools, development',
   'license': 'Copyright (c) 2016 The Python Packaging Authority (PyPA)  '
               'Permission is hereby granted, free of charge, to any person '
               'obtaining a copy of this software and associated documentation '
               'files (the "Software"), to deal in the Software without '
               'restriction, including without limitation the rights to use, '
               'copy, modify, merge, publish, distribute, sublicense, and/or sell '
               'copies of the Software, and to permit persons to whom the '
               'Software is furnished to do so, subject to the following '
               'conditions:  The above copyright notice and this permission '
               'notice shall be included in all copies or substantial portions of '
               'the Software.  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY '
               'OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE '
               'WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE '
               'AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT '
               'HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, '
               'WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING '
               'FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR '
               'OTHER DEALINGS IN THE SOFTWARE. ',
   'maintainer': None,
   'maintainer_email': '"A. Great Maintainer" <maintainer@example.com>',
   'name': 'sampleproject',
   'package_url': 'https://pypi.org/project/sampleproject/',
   'platform': None,
   'project_url': 'https://pypi.org/project/sampleproject/',
   'project_urls': {'Bug Reports': 'https://github.com/pypa/sampleproject/issues',
                     'Funding': 'https://donate.pypi.org',
                     'Homepage': 'https://github.com/pypa/sampleproject',
                     'Say Thanks!': 'http://saythanks.io/to/example',
                     'Source': 'https://github.com/pypa/sampleproject/'},
   'provides_extra': ['dev', 'test'],
   'release_url': 'https://pypi.org/project/sampleproject/4.0.0/',
   'requires_dist': ['peppercorn',
                     'check-manifest; extra == "dev"',
                     'coverage; extra == "test"'],
   'requires_python': '>=3.9',
   'summary': 'A sample Python project',
   'version': '4.0.0',
   'yanked': False,
   'yanked_reason': None}

The result can be limited to a certain *depth* (ellipsis is used for deeper
contents)::

   >>> pprint.pp(project_info, depth=1)
   {'author': None,
   'author_email': '"A. Random Developer" <author@example.com>',
   'bugtrack_url': None,
   'classifiers': [...],
   'description': '# A sample Python project\n'
                  '\n'
                  '![Python '
                  'Logo](https://www.python.org/static/community_logos/python-logo.png '
                  '"Sample inline image")\n'
                  '\n'
                  'A sample project that exists as an aid to the [Python '
                  'Packaging User\n'
                  "Guide][packaging guide]'s [Tutorial on Packaging and "
                  'Distributing\n'
                  'Projects][distribution tutorial].\n'
                  '\n'
                  'This project does not aim to cover best practices for Python '
                  'project\n'
                  'development as a whole. For example, it does not provide '
                  'guidance or tool\n'
                  'recommendations for version control, documentation, or '
                  'testing.\n'
                  '\n'
                  '[The source for this project is available here][src].\n'
                  '\n'
                  'The metadata for a Python project is defined in the '
                  '`pyproject.toml` file,\n'
                  'an example of which is included in this project. You should '
                  'edit this file\n'
                  'accordingly to adapt this sample project to your needs.\n'
                  '\n'
                  '----\n'
                  '\n'
                  'This is the README file for the project.\n'
                  '\n'
                  'The file should use UTF-8 encoding and can be written using\n'
                  '[reStructuredText][rst] or [markdown][md use] with the '
                  'appropriate [key set][md\n'
                  'use]. It will be used to generate the project webpage on PyPI '
                  'and will be\n'
                  'displayed as the project homepage on common code-hosting '
                  'services, and should be\n'
                  'written for that purpose.\n'
                  '\n'
                  'Typical contents for this file would include an overview of '
                  'the project, basic\n'
                  'usage examples, etc. Generally, including the project '
                  'changelog in here is not a\n'
                  "good idea, although a simple “What's New” section for the "
                  'most recent version\n'
                  'may be appropriate.\n'
                  '\n'
                  '[packaging guide]: https://packaging.python.org\n'
                  '[distribution tutorial]: '
                  'https://packaging.python.org/tutorials/packaging-projects/\n'
                  '[src]: https://github.com/pypa/sampleproject\n'
                  '[rst]: http://docutils.sourceforge.net/rst.html\n'
                  '[md]: https://tools.ietf.org/html/rfc7764#section-3.5 '
                  '"CommonMark variant"\n'
                  '[md use]: '
                  'https://packaging.python.org/specifications/core-metadata/#description-content-type-optional\n',
   'description_content_type': 'text/markdown',
   'docs_url': None,
   'download_url': None,
   'downloads': {...},
   'dynamic': None,
   'home_page': None,
   'keywords': 'sample, setuptools, development',
   'license': 'Copyright (c) 2016 The Python Packaging Authority (PyPA)  '
               'Permission is hereby granted, free of charge, to any person '
               'obtaining a copy of this software and associated documentation '
               'files (the "Software"), to deal in the Software without '
               'restriction, including without limitation the rights to use, '
               'copy, modify, merge, publish, distribute, sublicense, and/or sell '
               'copies of the Software, and to permit persons to whom the '
               'Software is furnished to do so, subject to the following '
               'conditions:  The above copyright notice and this permission '
               'notice shall be included in all copies or substantial portions of '
               'the Software.  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY '
               'OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE '
               'WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE '
               'AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT '
               'HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, '
               'WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING '
               'FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR '
               'OTHER DEALINGS IN THE SOFTWARE. ',
   'maintainer': None,
   'maintainer_email': '"A. Great Maintainer" <maintainer@example.com>',
   'name': 'sampleproject',
   'package_url': 'https://pypi.org/project/sampleproject/',
   'platform': None,
   'project_url': 'https://pypi.org/project/sampleproject/',
   'project_urls': {...},
   'provides_extra': [...],
   'release_url': 'https://pypi.org/project/sampleproject/4.0.0/',
   'requires_dist': [...],
   'requires_python': '>=3.9',
   'summary': 'A sample Python project',
   'version': '4.0.0',
   'yanked': False,
   'yanked_reason': None}

Additionally, maximum character *width* can be suggested. If a long object
cannot be split, the specified width will be exceeded::

   >>> pprint.pp(project_info, depth=1, width=60)
   {'author': None,
   'author_email': '"A. Random Developer" '
                  '<author@example.com>',
   'bugtrack_url': None,
   'classifiers': [...],
   'description': '# A sample Python project\n'
                  '\n'
                  '![Python '
                  'Logo](https://www.python.org/static/community_logos/python-logo.png '
                  '"Sample inline image")\n'
                  '\n'
                  'A sample project that exists as an aid to '
                  'the [Python Packaging User\n'
                  "Guide][packaging guide]'s [Tutorial on "
                  'Packaging and Distributing\n'
                  'Projects][distribution tutorial].\n'
                  '\n'
                  'This project does not aim to cover best '
                  'practices for Python project\n'
                  'development as a whole. For example, it '
                  'does not provide guidance or tool\n'
                  'recommendations for version control, '
                  'documentation, or testing.\n'
                  '\n'
                  '[The source for this project is available '
                  'here][src].\n'
                  '\n'
                  'The metadata for a Python project is '
                  'defined in the `pyproject.toml` file,\n'
                  'an example of which is included in this '
                  'project. You should edit this file\n'
                  'accordingly to adapt this sample project '
                  'to your needs.\n'
                  '\n'
                  '----\n'
                  '\n'
                  'This is the README file for the project.\n'
                  '\n'
                  'The file should use UTF-8 encoding and '
                  'can be written using\n'
                  '[reStructuredText][rst] or [markdown][md '
                  'use] with the appropriate [key set][md\n'
                  'use]. It will be used to generate the '
                  'project webpage on PyPI and will be\n'
                  'displayed as the project homepage on '
                  'common code-hosting services, and should '
                  'be\n'
                  'written for that purpose.\n'
                  '\n'
                  'Typical contents for this file would '
                  'include an overview of the project, '
                  'basic\n'
                  'usage examples, etc. Generally, including '
                  'the project changelog in here is not a\n'
                  "good idea, although a simple “What's New” "
                  'section for the most recent version\n'
                  'may be appropriate.\n'
                  '\n'
                  '[packaging guide]: '
                  'https://packaging.python.org\n'
                  '[distribution tutorial]: '
                  'https://packaging.python.org/tutorials/packaging-projects/\n'
                  '[src]: '
                  'https://github.com/pypa/sampleproject\n'
                  '[rst]: '
                  'http://docutils.sourceforge.net/rst.html\n'
                  '[md]: '
                  'https://tools.ietf.org/html/rfc7764#section-3.5 '
                  '"CommonMark variant"\n'
                  '[md use]: '
                  'https://packaging.python.org/specifications/core-metadata/#description-content-type-optional\n',
   'description_content_type': 'text/markdown',
   'docs_url': None,
   'download_url': None,
   'downloads': {...},
   'dynamic': None,
   'home_page': None,
   'keywords': 'sample, setuptools, development',
   'license': 'Copyright (c) 2016 The Python Packaging '
               'Authority (PyPA)  Permission is hereby '
               'granted, free of charge, to any person '
               'obtaining a copy of this software and '
               'associated documentation files (the '
               '"Software"), to deal in the Software without '
               'restriction, including without limitation the '
               'rights to use, copy, modify, merge, publish, '
               'distribute, sublicense, and/or sell copies of '
               'the Software, and to permit persons to whom '
               'the Software is furnished to do so, subject '
               'to the following conditions:  The above '
               'copyright notice and this permission notice '
               'shall be included in all copies or '
               'substantial portions of the Software.  THE '
               'SOFTWARE IS PROVIDED "AS IS", WITHOUT '
               'WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, '
               'INCLUDING BUT NOT LIMITED TO THE WARRANTIES '
               'OF MERCHANTABILITY, FITNESS FOR A PARTICULAR '
               'PURPOSE AND NONINFRINGEMENT. IN NO EVENT '
               'SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE '
               'LIABLE FOR ANY CLAIM, DAMAGES OR OTHER '
               'LIABILITY, WHETHER IN AN ACTION OF CONTRACT, '
               'TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN '
               'CONNECTION WITH THE SOFTWARE OR THE USE OR '
               'OTHER DEALINGS IN THE SOFTWARE. ',
   'maintainer': None,
   'maintainer_email': '"A. Great Maintainer" '
                        '<maintainer@example.com>',
   'name': 'sampleproject',
   'package_url': 'https://pypi.org/project/sampleproject/',
   'platform': None,
   'project_url': 'https://pypi.org/project/sampleproject/',
   'project_urls': {...},
   'provides_extra': [...],
   'release_url': 'https://pypi.org/project/sampleproject/4.0.0/',
   'requires_dist': [...],
   'requires_python': '>=3.9',
   'summary': 'A sample Python project',
   'version': '4.0.0',
   'yanked': False,
   'yanked_reason': None}
