:mod:`!locale` --- Internationalization services
================================================

.. module:: locale
   :synopsis: Internationalization services.

.. moduleauthor:: Martin von Löwis <martin@v.loewis.de>
.. sectionauthor:: Martin von Löwis <martin@v.loewis.de>

**Source code:** :source:`Lib/locale.py`

--------------

The :mod:`locale` module opens access to the POSIX locale database and
functionality. The POSIX locale mechanism allows programmers to deal with
certain cultural issues in an application, without requiring the programmer to
know all the specifics of each country where the software is executed.

.. index:: pair: module; _locale

The :mod:`locale` module is implemented on top of the :mod:`!_locale` module,
which in turn uses an ANSI C locale implementation if available.

The :mod:`locale` module defines the following exception and functions:


.. exception:: Error

   Exception raised when the locale passed to :func:`setlocale` is not
   recognized.


.. function:: setlocale(category, locale=None)

   If *locale* is given and not ``None``, :func:`setlocale` modifies the locale
   setting for the *category*. The available categories are listed in the data
   description below. *locale* may be a string, or an iterable of two strings
   (language code and encoding). If it's an iterable, it's converted to a locale
   name using the locale aliasing engine. An empty string specifies the user's
   default settings. If the modification of the locale fails, the exception
   :exc:`Error` is raised. If successful, the new locale setting is returned.

   If *locale* is omitted or ``None``, the current setting for *category* is
   returned.

   :func:`setlocale` is not thread-safe on most systems. Applications typically
   start with a call of ::

      import locale
      locale.setlocale(locale.LC_ALL, '')

   This sets the locale for all categories to the user's default setting (typically
   specified in the :envvar:`LANG` environment variable).  If the locale is not
   changed thereafter, using multithreading should not cause problems.


.. function:: localeconv()

   Returns the database of the local conventions as a dictionary. This dictionary
   has the following strings as keys:

   .. tabularcolumns:: |l|l|L|

   +----------------------+-------------------------------------+--------------------------------+
   | Category             | Key                                 | Meaning                        |
   +======================+=====================================+================================+
   | :const:`LC_NUMERIC`  | ``'decimal_point'``                 | Decimal point character.       |
   +----------------------+-------------------------------------+--------------------------------+
   |                      | ``'grouping'``                      | Sequence of numbers specifying |
   |                      |                                     | which relative positions the   |
   |                      |                                     | ``'thousands_sep'`` is         |
   |                      |                                     | expected.  If the sequence is  |
   |                      |                                     | terminated with                |
   |                      |                                     | :const:`CHAR_MAX`, no further  |
   |                      |                                     | grouping is performed. If the  |
   |                      |                                     | sequence terminates with a     |
   |                      |                                     | ``0``,  the last group size is |
   |                      |                                     | repeatedly used.               |
   +----------------------+-------------------------------------+--------------------------------+
   |                      | ``'thousands_sep'``                 | Character used between groups. |
   +----------------------+-------------------------------------+--------------------------------+
   | :const:`LC_MONETARY` | ``'int_curr_symbol'``               | International currency symbol. |
   +----------------------+-------------------------------------+--------------------------------+
   |                      | ``'currency_symbol'``               | Local currency symbol.         |
   +----------------------+-------------------------------------+--------------------------------+
   |                      | ``'p_cs_precedes/n_cs_precedes'``   | Whether the currency symbol    |
   |                      |                                     | precedes the value (for        |
   |                      |                                     | positive resp. negative        |
   |                      |                                     | values).                       |
   +----------------------+-------------------------------------+--------------------------------+
   |                      | ``'p_sep_by_space/n_sep_by_space'`` | Whether the currency symbol is |
   |                      |                                     | separated from the value  by a |
   |                      |                                     | space (for positive resp.      |
   |                      |                                     | negative values).              |
   +----------------------+-------------------------------------+--------------------------------+
   |                      | ``'mon_decimal_point'``             | Decimal point used for         |
   |                      |                                     | monetary values.               |
   +----------------------+-------------------------------------+--------------------------------+
   |                      | ``'frac_digits'``                   | Number of fractional digits    |
   |                      |                                     | used in local formatting of    |
   |                      |                                     | monetary values.               |
   +----------------------+-------------------------------------+--------------------------------+
   |                      | ``'int_frac_digits'``               | Number of fractional digits    |
   |                      |                                     | used in international          |
   |                      |                                     | formatting of monetary values. |
   +----------------------+-------------------------------------+--------------------------------+
   |                      | ``'mon_thousands_sep'``             | Group separator used for       |
   |                      |                                     | monetary values.               |
   +----------------------+-------------------------------------+--------------------------------+
   |                      | ``'mon_grouping'``                  | Equivalent to ``'grouping'``,  |
   |                      |                                     | used for monetary values.      |
   +----------------------+-------------------------------------+--------------------------------+
   |                      | ``'positive_sign'``                 | Symbol used to annotate a      |
   |                      |                                     | positive monetary value.       |
   +----------------------+-------------------------------------+--------------------------------+
   |                      | ``'negative_sign'``                 | Symbol used to annotate a      |
   |                      |                                     | negative monetary value.       |
   +----------------------+-------------------------------------+--------------------------------+
   |                      | ``'p_sign_posn/n_sign_posn'``       | The position of the sign (for  |
   |                      |                                     | positive resp. negative        |
   |                      |                                     | values), see below.            |
   +----------------------+-------------------------------------+--------------------------------+

   All numeric values can be set to :const:`CHAR_MAX` to indicate that there is no
   value specified in this locale.

   The possible values for ``'p_sign_posn'`` and ``'n_sign_posn'`` are given below.

   +--------------+-----------------------------------------+
   | Value        | Explanation                             |
   +==============+=========================================+
   | ``0``        | Currency and value are surrounded by    |
   |              | parentheses.                            |
   +--------------+-----------------------------------------+
   | ``1``        | The sign should precede the value and   |
   |              | currency symbol.                        |
   +--------------+-----------------------------------------+
   | ``2``        | The sign should follow the value and    |
   |              | currency symbol.                        |
   +--------------+-----------------------------------------+
   | ``3``        | The sign should immediately precede the |
   |              | value.                                  |
   +--------------+-----------------------------------------+
   | ``4``        | The sign should immediately follow the  |
   |              | value.                                  |
   +--------------+-----------------------------------------+
   | ``CHAR_MAX`` | Nothing is specified in this locale.    |
   +--------------+-----------------------------------------+

   The function temporarily sets the ``LC_CTYPE`` locale to the ``LC_NUMERIC``
   locale or the ``LC_MONETARY`` locale if locales are different and numeric or
   monetary strings are non-ASCII. This temporary change affects other threads.

   .. versionchanged:: 3.7
      The function now temporarily sets the ``LC_CTYPE`` locale to the
      ``LC_NUMERIC`` locale in some cases.


.. function:: nl_langinfo(option)

   Return some locale-specific information as a string.  This function is not
   available on all systems, and the set of possible options might also vary
   across platforms.  The possible argument values are numbers, for which
   symbolic constants are available in the locale module.

   The :func:`nl_langinfo` function accepts one of the following keys.  Most
   descriptions are taken from the corresponding description in the GNU C
   library.

   .. data:: CODESET

      Get a string with the name of the character encoding used in the
      selected locale.

   .. data:: D_T_FMT

      Get a string that can be used as a format string for :func:`time.strftime` to
      represent date and time in a locale-specific way.

   .. data:: D_FMT

      Get a string that can be used as a format string for :func:`time.strftime` to
      represent a date in a locale-specific way.

   .. data:: T_FMT

      Get a string that can be used as a format string for :func:`time.strftime` to
      represent a time in a locale-specific way.

   .. data:: T_FMT_AMPM

      Get a format string for :func:`time.strftime` to represent time in the am/pm
      format.

   .. data:: DAY_1
             DAY_2
             DAY_3
             DAY_4
             DAY_5
             DAY_6
             DAY_7

      Get the name of the n-th day of the week.

      .. note::

         This follows the US convention of :const:`DAY_1` being Sunday, not the
         international convention (ISO 8601) that Monday is the first day of the
         week.

   .. data:: ABDAY_1
             ABDAY_2
             ABDAY_3
             ABDAY_4
             ABDAY_5
             ABDAY_6
             ABDAY_7

      Get the abbreviated name of the n-th day of the week.

   .. data:: MON_1
             MON_2
             MON_3
             MON_4
             MON_5
             MON_6
             MON_7
             MON_8
             MON_9
             MON_10
             MON_11
             MON_12

      Get the name of the n-th month.

   .. data:: ABMON_1
             ABMON_2
             ABMON_3
             ABMON_4
             ABMON_5
             ABMON_6
             ABMON_7
             ABMON_8
             ABMON_9
             ABMON_10
             ABMON_11
             ABMON_12

      Get the abbreviated name of the n-th month.

   .. data:: RADIXCHAR

      Get the radix character (decimal dot, decimal comma, etc.).

   .. data:: THOUSEP

      Get the separator character for thousands (groups of three digits).

   .. data:: YESEXPR

      Get a regular expression that can be used with the regex function to
      recognize a positive response to a yes/no question.

   .. data:: NOEXPR

      Get a regular expression that can be used with the ``regex(3)`` function to
      recognize a negative response to a yes/no question.

      .. note::

         The regular expressions for :const:`YESEXPR` and
         :const:`NOEXPR` use syntax suitable for the
         ``regex`` function from the C library, which might
         differ from the syntax used in :mod:`re`.

   .. data:: CRNCYSTR

      Get the currency symbol, preceded by "-" if the symbol should appear before
      the value, "+" if the symbol should appear after the value, or "." if the
      symbol should replace the radix character.

   .. data:: ERA

      Get a string that represents the era used in the current locale.

      Most locales do not define this value.  An example of a locale which does
      define this value is the Japanese one.  In Japan, the traditional
      representation of dates includes the name of the era corresponding to the
      then-emperor's reign.

      Normally it should not be necessary to use this value directly. Specifying
      the ``E`` modifier in their format strings causes the :func:`time.strftime`
      function to use this information.  The format of the returned string is not
      specified, and therefore you should not assume knowledge of it on different
      systems.

   .. data:: ERA_D_T_FMT

      Get a format string for :func:`time.strftime` to represent date and time in a
      locale-specific era-based way.

   .. data:: ERA_D_FMT

      Get a format string for :func:`time.strftime` to represent a date in a
      locale-specific era-based way.

   .. data:: ERA_T_FMT

      Get a format string for :func:`time.strftime` to represent a time in a
      locale-specific era-based way.

   .. data:: ALT_DIGITS

      Get a representation of up to 100 values used to represent the values
      0 to 99.


.. function:: getdefaultlocale([envvars])

   Tries to determine the default locale settings and returns them as a tuple of
   the form ``(language code, encoding)``.

   According to POSIX, a program which has not called ``setlocale(LC_ALL, '')``
   runs using the portable ``'C'`` locale.  Calling ``setlocale(LC_ALL, '')`` lets
   it use the default locale as defined by the :envvar:`LANG` variable.  Since we
   do not want to interfere with the current locale setting we thus emulate the
   behavior in the way described above.

   To maintain compatibility with other platforms, not only the :envvar:`LANG`
   variable is tested, but a list of variables given as envvars parameter.  The
   first found to be defined will be used.  *envvars* defaults to the search
   path used in GNU gettext; it must always contain the variable name
   ``'LANG'``.  The GNU gettext search path contains ``'LC_ALL'``,
   ``'LC_CTYPE'``, ``'LANG'`` and ``'LANGUAGE'``, in that order.

   Except for the code ``'C'``, the language code corresponds to :rfc:`1766`.
   *language code* and *encoding* may be ``None`` if their values cannot be
   determined.

   .. deprecated-removed:: 3.11 3.15


.. function:: getlocale(category=LC_CTYPE)

   Returns the current setting for the given locale category as sequence containing
   *language code*, *encoding*. *category* may be one of the :const:`!LC_\*` values
   except :const:`LC_ALL`.  It defaults to :const:`LC_CTYPE`.

   Except for the code ``'C'``, the language code corresponds to :rfc:`1766`.
   *language code* and *encoding* may be ``None`` if their values cannot be
   determined.


.. function:: getpreferredencoding(do_setlocale=True)

   Return the :term:`locale encoding` used for text data, according to user
   preferences.  User preferences are expressed differently on different
   systems, and might not be available programmatically on some systems, so
   this function only returns a guess.

   On some systems, it is necessary to invoke :func:`setlocale` to obtain the
   user preferences, so this function is not thread-safe. If invoking setlocale
   is not necessary or desired, *do_setlocale* should be set to ``False``.

   On Android or if the :ref:`Python UTF-8 Mode <utf8-mode>` is enabled, always
   return ``'utf-8'``, the :term:`locale encoding` and the *do_setlocale*
   argument are ignored.

   The :ref:`Python preinitialization <c-preinit>` configures the LC_CTYPE
   locale. See also the :term:`filesystem encoding and error handler`.

   .. versionchanged:: 3.7
      The function now always returns ``"utf-8"`` on Android or if the
      :ref:`Python UTF-8 Mode <utf8-mode>` is enabled.


.. function:: getencoding()

   Get the current :term:`locale encoding`:

   * On Android and VxWorks, return ``"utf-8"``.
   * On Unix, return the encoding of the current :data:`LC_CTYPE` locale.
     Return ``"utf-8"`` if ``nl_langinfo(CODESET)`` returns an empty string:
     for example, if the current LC_CTYPE locale is not supported.
   * On Windows, return the ANSI code page.

   The :ref:`Python preinitialization <c-preinit>` configures the LC_CTYPE
   locale. See also the :term:`filesystem encoding and error handler`.

   This function is similar to
   :func:`getpreferredencoding(False) <getpreferredencoding>` except this
   function ignores the :ref:`Python UTF-8 Mode <utf8-mode>`.

   .. versionadded:: 3.11


.. function:: normalize(localename)

   Returns a normalized locale code for the given locale name.  The returned locale
   code is formatted for use with :func:`setlocale`.  If normalization fails, the
   original name is returned unchanged.

   If the given encoding is not known, the function defaults to the default
   encoding for the locale code just like :func:`setlocale`.


.. function:: resetlocale(category=LC_ALL)

   Sets the locale for *category* to the default setting.

   The default setting is determined by calling :func:`getdefaultlocale`.
   *category* defaults to :const:`LC_ALL`.

   .. deprecated-removed:: 3.11 3.13


.. function:: strcoll(string1, string2)

   Compares two strings according to the current :const:`LC_COLLATE` setting. As
   any other compare function, returns a negative, or a positive value, or ``0``,
   depending on whether *string1* collates before or after *string2* or is equal to
   it.


.. function:: strxfrm(string)

   Transforms a string to one that can be used in locale-aware
   comparisons.  For example, ``strxfrm(s1) < strxfrm(s2)`` is
   equivalent to ``strcoll(s1, s2) < 0``.  This function can be used
   when the same string is compared repeatedly, e.g. when collating a
   sequence of strings.


.. function:: format_string(format, val, grouping=False, monetary=False)

   Formats a number *val* according to the current :const:`LC_NUMERIC` setting.
   The format follows the conventions of the ``%`` operator.  For floating point
   values, the decimal point is modified if appropriate.  If *grouping* is ``True``,
   also takes the grouping into account.

   If *monetary* is true, the conversion uses monetary thousands separator and
   grouping strings.

   Processes formatting specifiers as in ``format % val``, but takes the current
   locale settings into account.

   .. versionchanged:: 3.7
      The *monetary* keyword parameter was added.


.. function:: currency(val, symbol=True, grouping=False, international=False)

   Formats a number *val* according to the current :const:`LC_MONETARY` settings.

   The returned string includes the currency symbol if *symbol* is true, which is
   the default. If *grouping* is ``True`` (which is not the default), grouping is done
   with the value. If *international* is ``True`` (which is not the default), the
   international currency symbol is used.

   .. note::

     This function will not work with the 'C' locale, so you have to set a
     locale via :func:`setlocale` first.


.. function:: str(float)

   Formats a floating point number using the same format as the built-in function
   ``str(float)``, but takes the decimal point into account.


.. function:: delocalize(string)

    Converts a string into a normalized number string, following the
    :const:`LC_NUMERIC` settings.

    .. versionadded:: 3.5


.. function:: localize(string, grouping=False, monetary=False)

    Converts a normalized number string into a formatted string following the
    :const:`LC_NUMERIC` settings.

    .. versionadded:: 3.10


.. function:: atof(string, func=float)

   Converts a string to a number, following the :const:`LC_NUMERIC` settings,
   by calling *func* on the result of calling :func:`delocalize` on *string*.


.. function:: atoi(string)

   Converts a string to an integer, following the :const:`LC_NUMERIC` conventions.


.. data:: LC_CTYPE

   Locale category for the character type functions.  Most importantly, this
   category defines the text encoding, i.e. how bytes are interpreted as
   Unicode codepoints.  See :pep:`538` and :pep:`540` for how this variable
   might be automatically coerced to ``C.UTF-8`` to avoid issues created by
   invalid settings in containers or incompatible settings passed over remote
   SSH connections.

   Python doesn't internally use locale-dependent character transformation functions
   from ``ctype.h``. Instead, an internal ``pyctype.h`` provides locale-independent
   equivalents like :c:macro:`!Py_TOLOWER`.


.. data:: LC_COLLATE

   Locale category for sorting strings.  The functions :func:`strcoll` and
   :func:`strxfrm` of the :mod:`locale` module are affected.


.. data:: LC_TIME

   Locale category for the formatting of time.  The function :func:`time.strftime`
   follows these conventions.


.. data:: LC_MONETARY

   Locale category for formatting of monetary values.  The available options are
   available from the :func:`localeconv` function.


.. data:: LC_MESSAGES

   Locale category for message display. Python currently does not support
   application specific locale-aware messages.  Messages displayed by the operating
   system, like those returned by :func:`os.strerror` might be affected by this
   category.

   This value may not be available on operating systems not conforming to the
   POSIX standard, most notably Windows.


.. data:: LC_NUMERIC

   Locale category for formatting numbers.  The functions :func:`format_string`,
   :func:`atoi`, :func:`atof` and :func:`.str` of the :mod:`locale` module are
   affected by that category.  All other numeric formatting operations are not
   affected.


.. data:: LC_ALL

   Combination of all locale settings.  If this flag is used when the locale is
   changed, setting the locale for all categories is attempted. If that fails for
   any category, no category is changed at all.  When the locale is retrieved using
   this flag, a string indicating the setting for all categories is returned. This
   string can be later used to restore the settings.


.. data:: CHAR_MAX

   This is a symbolic constant used for different values returned by
   :func:`localeconv`.


Example::

   >>> import locale
   >>> loc = locale.getlocale()  # get current locale
   # use German locale; name might vary with platform
   >>> locale.setlocale(locale.LC_ALL, 'de_DE')
   >>> locale.strcoll('f\xe4n', 'foo')  # compare a string containing an umlaut
   >>> locale.setlocale(locale.LC_ALL, '')   # use user's preferred locale
   >>> locale.setlocale(locale.LC_ALL, 'C')  # use default (C) locale
   >>> locale.setlocale(locale.LC_ALL, loc)  # restore saved locale


Background, details, hints, tips and caveats
--------------------------------------------

The C standard defines the locale as a program-wide property that may be
relatively expensive to change.  On top of that, some implementations are broken
in such a way that frequent locale changes may cause core dumps.  This makes the
locale somewhat painful to use correctly.

Initially, when a program is started, the locale is the ``C`` locale, no matter
what the user's preferred locale is.  There is one exception: the
:data:`LC_CTYPE` category is changed at startup to set the current locale
encoding to the user's preferred locale encoding. The program must explicitly
say that it wants the user's preferred locale settings for other categories by
calling ``setlocale(LC_ALL, '')``.

It is generally a bad idea to call :func:`setlocale` in some library routine,
since as a side effect it affects the entire program.  Saving and restoring it
is almost as bad: it is expensive and affects other threads that happen to run
before the settings have been restored.

If, when coding a module for general use, you need a locale independent version
of an operation that is affected by the locale (such as
certain formats used with :func:`time.strftime`), you will have to find a way to
do it without using the standard library routine.  Even better is convincing
yourself that using locale settings is okay.  Only as a last resort should you
document that your module is not compatible with non-\ ``C`` locale settings.

The only way to perform numeric operations according to the locale is to use the
special functions defined by this module: :func:`atof`, :func:`atoi`,
:func:`format_string`, :func:`.str`.

There is no way to perform case conversions and character classifications
according to the locale.  For (Unicode) text strings these are done according
to the character value only, while for byte strings, the conversions and
classifications are done according to the ASCII value of the byte, and bytes
whose high bit is set (i.e., non-ASCII bytes) are never converted or considered
part of a character class such as letter or whitespace.


.. _embedding-locale:

For extension writers and programs that embed Python
----------------------------------------------------

Extension modules should never call :func:`setlocale`, except to find out what
the current locale is.  But since the return value can only be used portably to
restore it, that is not very useful (except perhaps to find out whether or not
the locale is ``C``).

When Python code uses the :mod:`locale` module to change the locale, this also
affects the embedding application.  If the embedding application doesn't want
this to happen, it should remove the :mod:`!_locale` extension module (which does
all the work) from the table of built-in modules in the :file:`config.c` file,
and make sure that the :mod:`!_locale` module is not accessible as a shared
library.


.. _locale-gettext:

Access to message catalogs
--------------------------

.. function:: gettext(msg)
.. function:: dgettext(domain, msg)
.. function:: dcgettext(domain, msg, category)
.. function:: textdomain(domain)
.. function:: bindtextdomain(domain, dir)
.. function:: bind_textdomain_codeset(domain, codeset)

The locale module exposes the C library's gettext interface on systems that
provide this interface.  It consists of the functions :func:`gettext`,
:func:`dgettext`, :func:`dcgettext`, :func:`textdomain`, :func:`bindtextdomain`,
and :func:`bind_textdomain_codeset`.  These are similar to the same functions in
the :mod:`gettext` module, but use the C library's binary format for message
catalogs, and the C library's search algorithms for locating message catalogs.

Python applications should normally find no need to invoke these functions, and
should use :mod:`gettext` instead.  A known exception to this rule are
applications that link with additional C libraries which internally invoke
C functions ``gettext`` or ``dcgettext``.  For these applications, it may be
necessary to bind the text domain, so that the libraries can properly locate
their message catalogs.
