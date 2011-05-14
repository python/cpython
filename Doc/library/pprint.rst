
:mod:`pprint` --- Data pretty printer
=====================================

.. module:: pprint
   :synopsis: Data pretty printer.
.. moduleauthor:: Fred L. Drake, Jr. <fdrake@acm.org>
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>


The :mod:`pprint` module provides a capability to "pretty-print" arbitrary
Python data structures in a form which can be used as input to the interpreter.
If the formatted structures include objects which are not fundamental Python
types, the representation may not be loadable.  This may be the case if objects
such as files, sockets, classes, or instances are included, as well as many
other built-in objects which are not representable as Python constants.

The formatted representation keeps objects on a single line if it can, and
breaks them onto multiple lines if they don't fit within the allowed width.
Construct :class:`PrettyPrinter` objects explicitly if you need to adjust the
width constraint.

Dictionaries are sorted by key before the display is computed.

The :mod:`pprint` module defines one class:

.. First the implementation class:


.. class:: PrettyPrinter(...)

   Construct a :class:`PrettyPrinter` instance.  This constructor understands
   several keyword parameters.  An output stream may be set using the *stream*
   keyword; the only method used on the stream object is the file protocol's
   :meth:`write` method.  If not specified, the :class:`PrettyPrinter` adopts
   ``sys.stdout``.  Three additional parameters may be used to control the
   formatted representation.  The keywords are *indent*, *depth*, and *width*.  The
   amount of indentation added for each recursive level is specified by *indent*;
   the default is one.  Other values can cause output to look a little odd, but can
   make nesting easier to spot.  The number of levels which may be printed is
   controlled by *depth*; if the data structure being printed is too deep, the next
   contained level is replaced by ``...``.  By default, there is no constraint on
   the depth of the objects being formatted.  The desired output width is
   constrained using the *width* parameter; the default is 80 characters.  If a
   structure cannot be formatted within the constrained width, a best effort will
   be made.

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
      >>> tup = ('spam', ('eggs', ('lumberjack', ('knights', ('ni', ('dead',
      ... ('parrot', ('fresh fruit',))))))))
      >>> pp = pprint.PrettyPrinter(depth=6)
      >>> pp.pprint(tup)
      ('spam', ('eggs', ('lumberjack', ('knights', ('ni', ('dead', (...)))))))

The :class:`PrettyPrinter` class supports several derivative functions:

.. Now the derivative functions:

.. function:: pformat(object[, indent[, width[, depth]]])

   Return the formatted representation of *object* as a string.  *indent*, *width*
   and *depth* will be passed to the :class:`PrettyPrinter` constructor as
   formatting parameters.


.. function:: pprint(object[, stream[, indent[, width[, depth]]]])

   Prints the formatted representation of *object* on *stream*, followed by a
   newline.  If *stream* is omitted, ``sys.stdout`` is used.  This may be used
   in the interactive interpreter instead of the :func:`print` function for
   inspecting values (you can even reassign ``print = pprint.pprint`` for use
   within a scope).  *indent*, *width* and *depth* will be passed to the
   :class:`PrettyPrinter` constructor as formatting parameters.

      >>> import pprint
      >>> stuff = ['spam', 'eggs', 'lumberjack', 'knights', 'ni']
      >>> stuff.insert(0, stuff)
      >>> pprint.pprint(stuff)
      [<Recursion on list with id=...>,
       'spam',
       'eggs',
       'lumberjack',
       'knights',
       'ni']


.. function:: isreadable(object)

   .. index:: builtin: eval

   Determine if the formatted representation of *object* is "readable," or can be
   used to reconstruct the value using :func:`eval`.  This always returns ``False``
   for recursive objects.

      >>> pprint.isreadable(stuff)
      False


.. function:: isrecursive(object)

   Determine if *object* requires a recursive representation.


One more support function is also defined:

.. function:: saferepr(object)

   Return a string representation of *object*, protected against recursive data
   structures.  If the representation of *object* exposes a recursive entry, the
   recursive reference will be represented as ``<Recursion on typename with
   id=number>``.  The representation is not otherwise formatted.

   >>> pprint.saferepr(stuff)
   "[<Recursion on list with id=...>, 'spam', 'eggs', 'lumberjack', 'knights', 'ni']"


.. _prettyprinter-objects:

PrettyPrinter Objects
---------------------

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

   .. index:: builtin: eval

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
   should be ``True``.  Recursive calls to the :meth:`format` method should add
   additional entries for containers to this dictionary.  The third argument,
   *maxlevels*, gives the requested limit to recursion; this will be ``0`` if there
   is no requested limit.  This argument should be passed unmodified to recursive
   calls. The fourth argument, *level*, gives the current level; recursive calls
   should be passed a value less than that of the current call.


.. _pprint-example:

Example
-------

To demonstrate several uses of the :func:`pprint` function and its parameters,
let's fetch information about a package from PyPI::

   >>> import json
   >>> import pprint
   >>> from urllib.request import urlopen
   >>> with urlopen('http://pypi.python.org/pypi/configparser/json') as url:
   ...     http_info = url.info()
   ...     raw_data = url.read().decode(http_info.get_content_charset())
   >>> package_data = json.loads(raw_data)
   >>> result = {'headers': http_info.items(), 'body': package_data}

In its basic form, :func:`pprint` shows the whole object::

   >>> pprint.pprint(result)
   {'body': {'info': {'_pypi_hidden': False,
                      '_pypi_ordering': 12,
                      'classifiers': ['Development Status :: 4 - Beta',
                                      'Intended Audience :: Developers',
                                      'License :: OSI Approved :: MIT License',
                                      'Natural Language :: English',
                                      'Operating System :: OS Independent',
                                      'Programming Language :: Python',
                                      'Programming Language :: Python :: 2',
                                      'Programming Language :: Python :: 2.6',
                                      'Programming Language :: Python :: 2.7',
                                      'Topic :: Software Development :: Libraries',
                                      'Topic :: Software Development :: Libraries :: Python Modules'],
                      'download_url': 'UNKNOWN',
                      'home_page': 'http://docs.python.org/py3k/library/configparser.html',
                      'keywords': 'configparser ini parsing conf cfg configuration file',
                      'license': 'MIT',
                      'name': 'configparser',
                      'package_url': 'http://pypi.python.org/pypi/configparser',
                      'platform': 'any',
                      'release_url': 'http://pypi.python.org/pypi/configparser/3.2.0r3',
                      'requires_python': None,
                      'stable_version': None,
                      'summary': 'This library brings the updated configparser from Python 3.2+ to Python 2.6-2.7.',
                      'version': '3.2.0r3'},
           'urls': [{'comment_text': '',
                     'downloads': 47,
                     'filename': 'configparser-3.2.0r3.tar.gz',
                     'has_sig': False,
                     'md5_digest': '8500fd87c61ac0de328fc996fce69b96',
                     'packagetype': 'sdist',
                     'python_version': 'source',
                     'size': 32281,
                     'upload_time': '2011-05-10T16:28:50',
                     'url': 'http://pypi.python.org/packages/source/c/configparser/configparser-3.2.0r3.tar.gz'}]},
   'headers': [('Date', 'Sat, 14 May 2011 12:48:52 GMT'),
               ('Server', 'Apache/2.2.16 (Debian)'),
               ('Content-Disposition', 'inline'),
               ('Connection', 'close'),
               ('Transfer-Encoding', 'chunked'),
               ('Content-Type', 'application/json; charset="UTF-8"')]}

The result can be limited to a certain *depth* (ellipsis is used for deeper
contents)::

   >>> pprint.pprint(result, depth=3)
   {'body': {'info': {'_pypi_hidden': False,
                      '_pypi_ordering': 12,
                      'classifiers': [...],
                      'download_url': 'UNKNOWN',
                      'home_page': 'http://docs.python.org/py3k/library/configparser.html',
                      'keywords': 'configparser ini parsing conf cfg configuration file',
                      'license': 'MIT',
                      'name': 'configparser',
                      'package_url': 'http://pypi.python.org/pypi/configparser',
                      'platform': 'any',
                      'release_url': 'http://pypi.python.org/pypi/configparser/3.2.0r3',
                      'requires_python': None,
                      'stable_version': None,
                      'summary': 'This library brings the updated configparser from Python 3.2+ to Python 2.6-2.7.',
                      'version': '3.2.0r3'},
           'urls': [{...}]},
   'headers': [('Date', 'Sat, 14 May 2011 12:48:52 GMT'),
               ('Server', 'Apache/2.2.16 (Debian)'),
               ('Content-Disposition', 'inline'),
               ('Connection', 'close'),
               ('Transfer-Encoding', 'chunked'),
               ('Content-Type', 'application/json; charset="UTF-8"')]}

Additionally, maximum *width* can be suggested. If a long object cannot be
split, the specified width will be exceeded::

   >>> pprint.pprint(result['headers'], width=30)
   [('Date',
     'Sat, 14 May 2011 12:48:52 GMT'),
    ('Server',
     'Apache/2.2.16 (Debian)'),
    ('Content-Disposition',
     'inline'),
    ('Connection', 'close'),
    ('Transfer-Encoding',
     'chunked'),
    ('Content-Type',
     'application/json; charset="UTF-8"')]
