:mod:`configparser` --- Configuration file parser
=================================================

.. module:: configparser
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

This module provides the classes :class:`RawConfigParser` and
:class:`SafeConfigParser`.  They implement a basic configuration file parser
language which provides a structure similar to what you would find in Microsoft
Windows INI files.  You can use this to write Python programs which can be
customized by end users easily.

.. note::

   This library does *not* interpret or write the value-type prefixes used in
   the Windows Registry extended version of INI syntax.

A configuration file consists of sections, each led by a ``[section]`` header,
followed by key/value entries separated by a specific string (``=`` or ``:`` by
default). By default, section names are case sensitive but keys are not. Leading
und trailing whitespace is removed from keys and from values.  Values can be
omitted, in which case the key/value delimiter may also be left out.  Values
can also span multiple lines, as long as they are indented deeper than the first
line of the value.  Depending on the parser's mode, blank lines may be treated
as parts of multiline values or ignored.

Configuration files may include comments, prefixed by specific characters (``#``
and ``;`` by default).  Comments may appear on their own in an otherwise empty
line, or may be entered in lines holding values or section names.  In the
latter case, they need to be preceded by a whitespace character to be recognized
as a comment.  (For backwards compatibility, by default only ``;`` starts an
inline comment, while ``#`` does not.)

On top of the core functionality, :class:`SafeConfigParser` supports
interpolation.  This means values can contain format strings which refer to
other values in the same section, or values in a special ``DEFAULT`` section.
Additional defaults can be provided on initialization.

For example::

   [Paths]
   home_dir: /Users
   my_dir: %(home_dir)s/lumberjack
   my_pictures: %(my_dir)s/Pictures

   [Multiline Values]
   chorus: I'm a lumberjack, and I'm okay
      I sleep all night and I work all day

   [No Values]
   key_without_value
   empty string value here =

   [You can use comments] ; after a useful line
   ; in an empty line
   after: a_value ; here's another comment
   inside: a         ;comment
           multiline ;comment
           value!    ;comment

      [Sections Can Be Indented]
         can_values_be_as_well = True
         does_that_mean_anything_special = False
         purpose = formatting for readability
         multiline_values = are
            handled just fine as
            long as they are indented
            deeper than the first line
            of a value
         # Did I mention we can indent comments, too?


In the example above, :class:`SafeConfigParser` would resolve ``%(home_dir)s``
to the value of ``home_dir`` (``/Users`` in this case).  ``%(my_dir)s`` in
effect would resolve to ``/Users/lumberjack``.  All interpolations are done on
demand so keys used in the chain of references do not have to be specified in
any specific order in the configuration file.

:class:`RawConfigParser` would simply return ``%(my_dir)s/Pictures`` as the
value of ``my_pictures`` and ``%(home_dir)s/lumberjack`` as the value of
``my_dir``.  Other features presented in the example are handled in the same
manner by both parsers.

Default values can be specified by passing them as a dictionary when
constructing the :class:`SafeConfigParser`.

Sections are normally stored in an :class:`collections.OrderedDict` which
maintains the order of all keys.  An alternative dictionary type can be passed
to the :meth:`__init__` method.  For example, if a dictionary type is passed
that sorts its keys, the sections will be sorted on write-back, as will be the
keys within each section.


.. class:: RawConfigParser(defaults=None, dict_type=collections.OrderedDict, allow_no_value=False, delimiters=('=', ':'), comment_prefixes=_COMPATIBLE, strict=False, empty_lines_in_values=True)

   The basic configuration object.  When *defaults* is given, it is initialized
   into the dictionary of intrinsic defaults.  When *dict_type* is given, it
   will be used to create the dictionary objects for the list of sections, for
   the options within a section, and for the default values.

   When *delimiters* is given, it will be used as the set of substrings that
   divide keys from values.  When *comment_prefixes* is given, it will be used
   as the set of substrings that prefix comments in a line, both for the whole
   line and inline comments.  For backwards compatibility, the default value for
   *comment_prefixes* is a special value that indicates that ``;`` and ``#`` can
   start whole line comments while only ``;`` can start inline comments.

   When *strict* is ``True`` (default: ``False``), the parser won't allow for
   any section or option duplicates while reading from a single source (file,
   string or dictionary), raising :exc:`DuplicateSectionError` or
   :exc:`DuplicateOptionError`. When *empty_lines_in_values* is ``False``
   (default: ``True``), each empty line marks the end of an option.  Otherwise,
   internal empty lines of a multiline option are kept as part of the value.
   When *allow_no_value* is ``True`` (default: ``False``), options without
   values are accepted; the value presented for these is ``None``.

   This class does not support the magical interpolation behavior.

   .. versionchanged:: 3.1
      The default *dict_type* is :class:`collections.OrderedDict`.

   .. versionchanged:: 3.2
      *allow_no_value*, *delimiters*, *comment_prefixes*, *strict* and
      *empty_lines_in_values* were added.


.. class:: SafeConfigParser(defaults=None, dict_type=collections.OrderedDict, allow_no_value=False, delimiters=('=', ':'), comment_prefixes=_COMPATIBLE, strict=False, empty_lines_in_values=True)

   Derived class of :class:`ConfigParser` that implements a sane variant of the
   magical interpolation feature.  This implementation is more predictable as it
   validates the interpolation syntax used within a configuration file.  This
   class also enables escaping the interpolation character (e.g. a key can have
   ``%`` as part of the value by specifying ``%%`` in the file).

   Applications that don't require interpolation should use
   :class:`RawConfigParser`, otherwise :class:`SafeConfigParser` is the best
   option.

   .. versionchanged:: 3.1
      The default *dict_type* is :class:`collections.OrderedDict`.

   .. versionchanged:: 3.2
      *allow_no_value*, *delimiters*, *comment_prefixes*, *strict* and
      *empty_lines_in_values* were added.


.. class:: ConfigParser(defaults=None, dict_type=collections.OrderedDict, allow_no_value=False, delimiters=('=', ':'), comment_prefixes=_COMPATIBLE, strict=False, empty_lines_in_values=True)

   Derived class of :class:`RawConfigParser` that implements the magical
   interpolation feature and adds optional arguments to the :meth:`get` and
   :meth:`items` methods.

   :class:`SafeConfigParser` is generally recommended over this class if you
   need interpolation.

   The values in *defaults* must be appropriate for the ``%()s`` string
   interpolation.  Note that *__name__* is an intrinsic default; its value is
   the section name, and will override any value provided in *defaults*.

   All option names used in interpolation will be passed through the
   :meth:`optionxform` method just like any other option name reference.  For
   example, using the default implementation of :meth:`optionxform` (which
   converts option names to lower case), the values ``foo %(bar)s`` and ``foo
   %(BAR)s`` are equivalent.

   .. versionchanged:: 3.1
      The default *dict_type* is :class:`collections.OrderedDict`.

   .. versionchanged:: 3.2
      *allow_no_value*, *delimiters*, *comment_prefixes*,
      *strict* and *empty_lines_in_values* were added.


.. exception:: Error

   Base class for all other configparser exceptions.


.. exception:: NoSectionError

   Exception raised when a specified section is not found.


.. exception:: DuplicateSectionError

   Exception raised if :meth:`add_section` is called with the name of a section
   that is already present or in strict parsers when a section if found more
   than once in a single input file, string or dictionary.

   .. versionadded:: 3.2
      Optional ``source`` and ``lineno`` attributes and arguments to
      :meth:`__init__` were added.


.. exception:: DuplicateOptionError

   Exception raised by strict parsers if a single option appears twice during
   reading from a single file, string or dictionary. This catches misspellings
   and case sensitivity-related errors, e.g. a dictionary may have two keys
   representing the same case-insensitive configuration key.


.. exception:: NoOptionError

   Exception raised when a specified option is not found in the specified
   section.


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


.. exception:: InterpolationSyntaxError

   Exception raised when the source text into which substitutions are made does not
   conform to the required syntax. Subclass of :exc:`InterpolationError`.


.. exception:: MissingSectionHeaderError

   Exception raised when attempting to parse a file which has no section headers.


.. exception:: ParsingError

   Exception raised when errors occur attempting to parse a file.

   .. versionchanged:: 3.2
      The ``filename`` attribute and :meth:`__init__` argument were renamed to
      ``source`` for consistency.

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
   already exists, :exc:`DuplicateSectionError` is raised. If the name
   ``DEFAULT`` (or any of it's case-insensitive variants) is passed,
   :exc:`ValueError` is raised.

.. method:: RawConfigParser.has_section(section)

   Indicates whether the named section is present in the configuration. The
   ``DEFAULT`` section is not acknowledged.


.. method:: RawConfigParser.options(section)

   Returns a list of options available in the specified *section*.


.. method:: RawConfigParser.has_option(section, option)

   If the given section exists, and contains the given option, return
   :const:`True`; otherwise return :const:`False`.


.. method:: RawConfigParser.read(filenames, encoding=None)

   Attempt to read and parse a list of filenames, returning a list of filenames
   which were successfully parsed.  If *filenames* is a string, it is treated as
   a single filename.  If a file named in *filenames* cannot be opened, that
   file will be ignored.  This is designed so that you can specify a list of
   potential configuration file locations (for example, the current directory,
   the user's home directory, and some system-wide directory), and all existing
   configuration files in the list will be read.  If none of the named files
   exist, the :class:`ConfigParser` instance will contain an empty dataset.  An
   application which requires initial values to be loaded from a file should
   load the required file or files using :meth:`read_file` before calling
   :meth:`read` for any optional files::

      import configparser, os

      config = configparser.ConfigParser()
      config.read_file(open('defaults.cfg'))
      config.read(['site.cfg', os.path.expanduser('~/.myapp.cfg')], encoding='cp1250')

   .. versionadded:: 3.2
      The *encoding* parameter.  Previously, all files were read using the
      default encoding for :func:`open`.


.. method:: RawConfigParser.read_file(f, source=None)

   Read and parse configuration data from the file or file-like object in *f*
   (only the :meth:`readline` method is used).  The file-like object must
   operate in text mode, i.e. return strings from :meth:`readline`.

   Optional argument *source* specifies the name of the file being read. It not
   given and *f* has a :attr:`name` attribute, that is used for *source*; the
   default is ``<???>``.

   .. versionadded:: 3.2
      Renamed from :meth:`readfp` (with the ``filename`` attribute renamed to
      ``source`` for consistency with other ``read_*`` methods).


.. method:: RawConfigParser.read_string(string, source='<string>')

   Parse configuration data from a given string.

   Optional argument *source* specifies a context-specific name of the string
   passed. If not given, ``<string>`` is used.

   .. versionadded:: 3.2


.. method:: RawConfigParser.read_dict(dictionary, source='<dict>')

   Load configuration from a dictionary. Keys are section names, values are
   dictionaries with keys and values that should be present in the section. If
   the used dictionary type preserves order, sections and their keys will be
   added in order. Values are automatically converted to strings.

   Optional argument *source* specifies a context-specific name of the
   dictionary passed.  If not given, ``<dict>`` is used.

   .. versionadded:: 3.2

.. method:: RawConfigParser.get(section, option, [vars, default])

   Get an *option* value for the named *section*. If *vars* is provided, it
   must be a dictionary.  The *option* is looked up in *vars* (if provided),
   *section*, and in *DEFAULTSECT* in that order. If the key is not found and
   *default* is provided, it is used as a fallback value. ``None`` can be
   provided as a *default* value.


.. method:: RawConfigParser.getint(section, option, [vars, default])

   A convenience method which coerces the *option* in the specified *section* to
   an integer. See :meth:`get` for explanation of *vars* and *default*.


.. method:: RawConfigParser.getfloat(section, option, [vars, default])

   A convenience method which coerces the *option* in the specified *section* to
   a floating point number.  See :meth:`get` for explanation of *vars* and
   *default*.


.. method:: RawConfigParser.getboolean(section, option, [vars, default])

   A convenience method which coerces the *option* in the specified *section*
   to a Boolean value.  Note that the accepted values for the option are
   ``"1"``, ``"yes"``, ``"true"``, and ``"on"``, which cause this method to
   return ``True``, and ``"0"``, ``"no"``, ``"false"``, and ``"off"``, which
   cause it to return ``False``.  These string values are checked in
   a case-insensitive manner.  Any other value will cause it to raise
   :exc:`ValueError`. See :meth:`get` for explanation of *vars* and *default*.


.. method:: RawConfigParser.items(section)

   Return a list of ``(name, value)`` pairs for each option in the given *section*.


.. method:: RawConfigParser.set(section, option, value)

   If the given section exists, set the given option to the specified value;
   otherwise raise :exc:`NoSectionError`.  While it is possible to use
   :class:`RawConfigParser` (or :class:`ConfigParser` with *raw* parameters set to
   true) for *internal* storage of non-string values, full functionality (including
   interpolation and output to files) can only be achieved using string values.


.. method:: RawConfigParser.write(fileobject, space_around_delimiters=True)

   Write a representation of the configuration to the specified :term:`file object`,
   which must be opened in text mode (accepting strings).  This representation
   can be parsed by a future :meth:`read` call. If ``space_around_delimiters``
   is ``True`` (the default), delimiters between keys and values are surrounded
   by spaces.


.. method:: RawConfigParser.remove_option(section, option)

   Remove the specified *option* from the specified *section*. If the section does
   not exist, raise :exc:`NoSectionError`.  If the option existed to be removed,
   return :const:`True`; otherwise return :const:`False`.


.. method:: RawConfigParser.remove_section(section)

   Remove the specified *section* from the configuration. If the section in fact
   existed, return ``True``. Otherwise return ``False``.


.. method:: RawConfigParser.optionxform(option)

   Transforms the option name *option* as found in an input file or as passed in
   by client code to the form that should be used in the internal structures.
   The default implementation returns a lower-case version of *option*;
   subclasses may override this or client code can set an attribute of this name
   on instances to affect this behavior.

   You don't necessarily need to subclass a ConfigParser to use this method, you
   can also re-set it on an instance, to a function that takes a string
   argument.  Setting it to ``str``, for example, would make option names case
   sensitive::

      cfgparser = ConfigParser()
      ...
      cfgparser.optionxform = str

   Note that when reading configuration files, whitespace around the
   option names are stripped before :meth:`optionxform` is called.


.. method:: RawConfigParser.readfp(fp, filename=None)

   .. deprecated:: 3.2
      Please use :meth:`read_file` instead.


.. _configparser-objects:

ConfigParser Objects
--------------------

The :class:`ConfigParser` class extends some methods of the
:class:`RawConfigParser` interface, adding some optional arguments. Whenever you
can, consider using :class:`SafeConfigParser` which adds validation and escaping
for the interpolation.


.. method:: ConfigParser.get(section, option, raw=False, [vars, default])

   Get an *option* value for the named *section*.  If *vars* is provided, it
   must be a dictionary.  The *option* is looked up in *vars* (if provided),
   *section*, and in *DEFAULTSECT* in that order. If the key is not found and
   *default* is provided, it is used as a fallback value. ``None`` can be
   provided as a *default* value.

   All the ``'%'`` interpolations are expanded in the return values, unless the
   *raw* argument is true.  Values for interpolation keys are looked up in the
   same manner as the option.


.. method:: ConfigParser.getint(section, option, raw=False, [vars, default])

   A convenience method which coerces the *option* in the specified *section* to
   an integer. See :meth:`get` for explanation of *raw*, *vars* and *default*.


.. method:: ConfigParser.getfloat(section, option, raw=False, [vars, default])

   A convenience method which coerces the *option* in the specified *section* to
   a floating point number. See :meth:`get` for explanation of *raw*, *vars*
   and *default*.


.. method:: ConfigParser.getboolean(section, option, raw=False, [vars, default])

   A convenience method which coerces the *option* in the specified *section*
   to a Boolean value.  Note that the accepted values for the option are
   ``"1"``, ``"yes"``, ``"true"``, and ``"on"``, which cause this method to
   return ``True``, and ``"0"``, ``"no"``, ``"false"``, and ``"off"``, which
   cause it to return ``False``.  These string values are checked in
   a case-insensitive manner.  Any other value will cause it to raise
   :exc:`ValueError`. See :meth:`get` for explanation of *raw*, *vars* and
   *default*.


.. method:: ConfigParser.items(section, raw=False, vars=None)

   Return a list of ``(name, value)`` pairs for each option in the given
   *section*.  Optional arguments have the same meaning as for the :meth:`get`
   method.


.. _safeconfigparser-objects:

SafeConfigParser Objects
------------------------

The :class:`SafeConfigParser` class implements the same extended interface as
:class:`ConfigParser`, with the following addition:


.. method:: SafeConfigParser.set(section, option, value)

   If the given section exists, set the given option to the specified value;
   otherwise raise :exc:`NoSectionError`.  *value* must be a string; if it is
   not, :exc:`TypeError` is raised.


Examples
--------

An example of writing to a configuration file::

   import configparser

   config = configparser.RawConfigParser()

   # When adding sections or items, add them in the reverse order of
   # how you want them to be displayed in the actual file.
   # In addition, please note that using RawConfigParser's and the raw
   # mode of ConfigParser's respective set functions, you can assign
   # non-string values to keys internally, but will receive an error
   # when attempting to write to a file or when you get it in non-raw
   # mode. SafeConfigParser does not allow such assignments to take place.
   config.add_section('Section1')
   config.set('Section1', 'int', '15')
   config.set('Section1', 'bool', 'true')
   config.set('Section1', 'float', '3.1415')
   config.set('Section1', 'baz', 'fun')
   config.set('Section1', 'bar', 'Python')
   config.set('Section1', 'foo', '%(bar)s is %(baz)s!')

   # Writing our configuration file to 'example.cfg'
   with open('example.cfg', 'w') as configfile:
       config.write(configfile)

An example of reading the configuration file again::

   import configparser

   config = configparser.RawConfigParser()
   config.read('example.cfg')

   # getfloat() raises an exception if the value is not a float
   # getint() and getboolean() also do this for their respective types
   float = config.getfloat('Section1', 'float')
   int = config.getint('Section1', 'int')
   print(float + int)

   # Notice that the next output does not interpolate '%(bar)s' or '%(baz)s'.
   # This is because we are using a RawConfigParser().
   if config.getboolean('Section1', 'bool'):
       print(config.get('Section1', 'foo'))

To get interpolation, you will need to use a :class:`ConfigParser` or
:class:`SafeConfigParser`::

   import configparser

   config = configparser.ConfigParser()
   config.read('example.cfg')

   # Set the third, optional argument of get to 1 if you wish to use raw mode.
   print(config.get('Section1', 'foo', 0)) # -> "Python is fun!"
   print(config.get('Section1', 'foo', 1)) # -> "%(bar)s is %(baz)s!"

   # The optional fourth argument is a dict with members that will take
   # precedence in interpolation.
   print(config.get('Section1', 'foo', 0, {'bar': 'Documentation',
                                           'baz': 'evil'}))

Defaults are available in all three types of ConfigParsers. They are used in
interpolation if an option used is not defined elsewhere. ::

   import configparser

   # New instance with 'bar' and 'baz' defaulting to 'Life' and 'hard' each
   config = configparser.SafeConfigParser({'bar': 'Life', 'baz': 'hard'})
   config.read('example.cfg')

   print(config.get('Section1', 'foo')) # -> "Python is fun!"
   config.remove_option('Section1', 'bar')
   config.remove_option('Section1', 'baz')
   print(config.get('Section1', 'foo')) # -> "Life is hard!"

The function ``opt_move`` below can be used to move options between sections::

   def opt_move(config, section1, section2, option):
       try:
           config.set(section2, option, config.get(section1, option, 1))
       except configparser.NoSectionError:
           # Create non-existent section
           config.add_section(section2)
           opt_move(config, section1, section2, option)
       else:
           config.remove_option(section1, option)

Some configuration files are known to include settings without values, but which
otherwise conform to the syntax supported by :mod:`configparser`.  The
*allow_no_value* parameter to the :meth:`__init__` method can be used to
indicate that such values should be accepted:

.. doctest::

   >>> import configparser
   >>> import io

   >>> sample_config = """
   ... [mysqld]
   ...   user = mysql
   ...   pid-file = /var/run/mysqld/mysqld.pid
   ...   skip-external-locking
   ...   old_passwords = 1
   ...   skip-bdb
   ...   skip-innodb # we don't need ACID today
   ... """
   >>> config = configparser.RawConfigParser(allow_no_value=True)
   >>> config.read_file(io.BytesIO(sample_config))

   >>> # Settings with values are treated as before:
   >>> config.get("mysqld", "user")
   'mysql'

   >>> # Settings without values provide None:
   >>> config.get("mysqld", "skip-bdb")

   >>> # Settings which aren't specified still raise an error:
   >>> config.get("mysqld", "does-not-exist")
   Traceback (most recent call last):
     ...
   configparser.NoOptionError: No option 'does-not-exist' in section: 'mysqld'
