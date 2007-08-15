
:mod:`ConfigParser` --- Configuration file parser
=================================================

.. module:: ConfigParser
   :synopsis: Configuration file parser.
.. moduleauthor:: Ken Manheimer <klm@zope.com>
.. moduleauthor:: Barry Warsaw <bwarsaw@python.org>
.. moduleauthor:: Eric S. Raymond <esr@thyrsus.com>
.. sectionauthor:: Christopher G. Petrilli <petrilli@amber.org>


.. index::
   pair: .ini; file
   pair: configuration; file
   single: ini file
   single: Windows ini file

This module defines the class :class:`ConfigParser`.   The :class:`ConfigParser`
class implements a basic configuration file parser language which provides a
structure similar to what you would find on Microsoft Windows INI files.  You
can use this to write Python programs which can be customized by end users
easily.

.. warning::

   This library does *not* interpret or write the value-type prefixes used in the
   Windows Registry extended version of INI syntax.

The configuration file consists of sections, led by a ``[section]`` header and
followed by ``name: value`` entries, with continuations in the style of
:rfc:`822`; ``name=value`` is also accepted.  Note that leading whitespace is
removed from values. The optional values can contain format strings which refer
to other values in the same section, or values in a special ``DEFAULT`` section.
Additional defaults can be provided on initialization and retrieval.  Lines
beginning with ``'#'`` or ``';'`` are ignored and may be used to provide
comments.

For example::

   [My Section]
   foodir: %(dir)s/whatever
   dir=frob

would resolve the ``%(dir)s`` to the value of ``dir`` (``frob`` in this case).
All reference expansions are done on demand.

Default values can be specified by passing them into the :class:`ConfigParser`
constructor as a dictionary.  Additional defaults  may be passed into the
:meth:`get` method which will override all others.

Sections are normally stored in a builtin dictionary. An alternative dictionary
type can be passed to the :class:`ConfigParser` constructor. For example, if a
dictionary type is passed that sorts its keys, the sections will be sorted on
write-back, as will be the keys within each section.


.. class:: RawConfigParser([defaults[, dict_type]])

   The basic configuration object.  When *defaults* is given, it is initialized
   into the dictionary of intrinsic defaults.  When *dict_type* is given, it will
   be used to create the dictionary objects for the list of sections, for the
   options within a section, and for the default values. This class does not
   support the magical interpolation behavior.

   .. versionadded:: 2.3

   .. versionchanged:: 2.6
      *dict_type* was added.


.. class:: ConfigParser([defaults])

   Derived class of :class:`RawConfigParser` that implements the magical
   interpolation feature and adds optional arguments to the :meth:`get` and
   :meth:`items` methods.  The values in *defaults* must be appropriate for the
   ``%()s`` string interpolation.  Note that *__name__* is an intrinsic default;
   its value is the section name, and will override any value provided in
   *defaults*.

   All option names used in interpolation will be passed through the
   :meth:`optionxform` method just like any other option name reference.  For
   example, using the default implementation of :meth:`optionxform` (which converts
   option names to lower case), the values ``foo %(bar)s`` and ``foo %(BAR)s`` are
   equivalent.


.. class:: SafeConfigParser([defaults])

   Derived class of :class:`ConfigParser` that implements a more-sane variant of
   the magical interpolation feature.  This implementation is more predictable as
   well. New applications should prefer this version if they don't need to be
   compatible with older versions of Python.

   .. % XXX Need to explain what's safer/more predictable about it.

   .. versionadded:: 2.3


.. exception:: NoSectionError

   Exception raised when a specified section is not found.


.. exception:: DuplicateSectionError

   Exception raised if :meth:`add_section` is called with the name of a section
   that is already present.


.. exception:: NoOptionError

   Exception raised when a specified option is not found in the specified  section.


.. exception:: InterpolationError

   Base class for exceptions raised when problems occur performing string
   interpolation.


.. exception:: InterpolationDepthError

   Exception raised when string interpolation cannot be completed because the
   number of iterations exceeds :const:`MAX_INTERPOLATION_DEPTH`. Subclass of
   :exc:`InterpolationError`.


.. exception:: InterpolationMissingOptionError

   Exception raised when an option referenced from a value does not exist. Subclass
   of :exc:`InterpolationError`.

   .. versionadded:: 2.3


.. exception:: InterpolationSyntaxError

   Exception raised when the source text into which substitutions are made does not
   conform to the required syntax. Subclass of :exc:`InterpolationError`.

   .. versionadded:: 2.3


.. exception:: MissingSectionHeaderError

   Exception raised when attempting to parse a file which has no section headers.


.. exception:: ParsingError

   Exception raised when errors occur attempting to parse a file.


.. data:: MAX_INTERPOLATION_DEPTH

   The maximum depth for recursive interpolation for :meth:`get` when the *raw*
   parameter is false.  This is relevant only for the :class:`ConfigParser` class.


.. seealso::

   Module :mod:`shlex`
      Support for a creating Unix shell-like mini-languages which can be used as an
      alternate format for application configuration files.


.. _rawconfigparser-objects:

RawConfigParser Objects
-----------------------

:class:`RawConfigParser` instances have the following methods:


.. method:: RawConfigParser.defaults()

   Return a dictionary containing the instance-wide defaults.


.. method:: RawConfigParser.sections()

   Return a list of the sections available; ``DEFAULT`` is not included in the
   list.


.. method:: RawConfigParser.add_section(section)

   Add a section named *section* to the instance.  If a section by the given name
   already exists, :exc:`DuplicateSectionError` is raised.


.. method:: RawConfigParser.has_section(section)

   Indicates whether the named section is present in the configuration. The
   ``DEFAULT`` section is not acknowledged.


.. method:: RawConfigParser.options(section)

   Returns a list of options available in the specified *section*.


.. method:: RawConfigParser.has_option(section, option)

   If the given section exists, and contains the given option, return
   :const:`True`; otherwise return :const:`False`.

   .. versionadded:: 1.6


.. method:: RawConfigParser.read(filenames)

   Attempt to read and parse a list of filenames, returning a list of filenames
   which were successfully parsed.  If *filenames* is a string or Unicode string,
   it is treated as a single filename. If a file named in *filenames* cannot be
   opened, that file will be ignored.  This is designed so that you can specify a
   list of potential configuration file locations (for example, the current
   directory, the user's home directory, and some system-wide directory), and all
   existing configuration files in the list will be read.  If none of the named
   files exist, the :class:`ConfigParser` instance will contain an empty dataset.
   An application which requires initial values to be loaded from a file should
   load the required file or files using :meth:`readfp` before calling :meth:`read`
   for any optional files::

      import ConfigParser, os

      config = ConfigParser.ConfigParser()
      config.readfp(open('defaults.cfg'))
      config.read(['site.cfg', os.path.expanduser('~/.myapp.cfg')])

   .. versionchanged:: 2.4
      Returns list of successfully parsed filenames.


.. method:: RawConfigParser.readfp(fp[, filename])

   Read and parse configuration data from the file or file-like object in *fp*
   (only the :meth:`readline` method is used).  If *filename* is omitted and *fp*
   has a :attr:`name` attribute, that is used for *filename*; the default is
   ``<???>``.


.. method:: RawConfigParser.get(section, option)

   Get an *option* value for the named *section*.


.. method:: RawConfigParser.getint(section, option)

   A convenience method which coerces the *option* in the specified *section* to an
   integer.


.. method:: RawConfigParser.getfloat(section, option)

   A convenience method which coerces the *option* in the specified *section* to a
   floating point number.


.. method:: RawConfigParser.getboolean(section, option)

   A convenience method which coerces the *option* in the specified *section* to a
   Boolean value.  Note that the accepted values for the option are ``"1"``,
   ``"yes"``, ``"true"``, and ``"on"``, which cause this method to return ``True``,
   and ``"0"``, ``"no"``, ``"false"``, and ``"off"``, which cause it to return
   ``False``.  These string values are checked in a case-insensitive manner.  Any
   other value will cause it to raise :exc:`ValueError`.


.. method:: RawConfigParser.items(section)

   Return a list of ``(name, value)`` pairs for each option in the given *section*.


.. method:: RawConfigParser.set(section, option, value)

   If the given section exists, set the given option to the specified value;
   otherwise raise :exc:`NoSectionError`.  While it is possible to use
   :class:`RawConfigParser` (or :class:`ConfigParser` with *raw* parameters set to
   true) for *internal* storage of non-string values, full functionality (including
   interpolation and output to files) can only be achieved using string values.

   .. versionadded:: 1.6


.. method:: RawConfigParser.write(fileobject)

   Write a representation of the configuration to the specified file object.  This
   representation can be parsed by a future :meth:`read` call.

   .. versionadded:: 1.6


.. method:: RawConfigParser.remove_option(section, option)

   Remove the specified *option* from the specified *section*. If the section does
   not exist, raise :exc:`NoSectionError`.  If the option existed to be removed,
   return :const:`True`; otherwise return :const:`False`.

   .. versionadded:: 1.6


.. method:: RawConfigParser.remove_section(section)

   Remove the specified *section* from the configuration. If the section in fact
   existed, return ``True``. Otherwise return ``False``.


.. method:: RawConfigParser.optionxform(option)

   Transforms the option name *option* as found in an input file or as passed in by
   client code to the form that should be used in the internal structures.  The
   default implementation returns a lower-case version of *option*; subclasses may
   override this or client code can set an attribute of this name on instances to
   affect this behavior.  Setting this to :func:`str`, for example, would make
   option names case sensitive.


.. _configparser-objects:

ConfigParser Objects
--------------------

The :class:`ConfigParser` class extends some methods of the
:class:`RawConfigParser` interface, adding some optional arguments.


.. method:: ConfigParser.get(section, option[, raw[, vars]])

   Get an *option* value for the named *section*.  All the ``'%'`` interpolations
   are expanded in the return values, based on the defaults passed into the
   constructor, as well as the options *vars* provided, unless the *raw* argument
   is true.


.. method:: ConfigParser.items(section[, raw[, vars]])

   Return a list of ``(name, value)`` pairs for each option in the given *section*.
   Optional arguments have the same meaning as for the :meth:`get` method.

   .. versionadded:: 2.3


.. _safeconfigparser-objects:

SafeConfigParser Objects
------------------------

The :class:`SafeConfigParser` class implements the same extended interface as
:class:`ConfigParser`, with the following addition:


.. method:: SafeConfigParser.set(section, option, value)

   If the given section exists, set the given option to the specified value;
   otherwise raise :exc:`NoSectionError`.  *value* must be a string (:class:`str`
   or :class:`unicode`); if not, :exc:`TypeError` is raised.

   .. versionadded:: 2.4

