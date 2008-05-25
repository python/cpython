
:mod:`shlex` --- Simple lexical analysis
========================================

.. module:: shlex
   :synopsis: Simple lexical analysis for Unix shell-like languages.
.. moduleauthor:: Eric S. Raymond <esr@snark.thyrsus.com>
.. moduleauthor:: Gustavo Niemeyer <niemeyer@conectiva.com>
.. sectionauthor:: Eric S. Raymond <esr@snark.thyrsus.com>
.. sectionauthor:: Gustavo Niemeyer <niemeyer@conectiva.com>


.. versionadded:: 1.5.2

The :class:`shlex` class makes it easy to write lexical analyzers for simple
syntaxes resembling that of the Unix shell.  This will often be useful for
writing minilanguages, (for example, in run control files for Python
applications) or for parsing quoted strings.

.. note::

   The :mod:`shlex` module currently does not support Unicode input.

The :mod:`shlex` module defines the following functions:


.. function:: split(s[, comments[, posix]])

   Split the string *s* using shell-like syntax. If *comments* is :const:`False`
   (the default), the parsing of comments in the given string will be disabled
   (setting the :attr:`commenters` member of the :class:`shlex` instance to the
   empty string).  This function operates in POSIX mode by default, but uses
   non-POSIX mode if the *posix* argument is false.

   .. versionadded:: 2.3

   .. versionchanged:: 2.6
      Added the *posix* parameter.

   .. note::

      Since the :func:`split` function instantiates a :class:`shlex` instance, passing
      ``None`` for *s* will read the string to split from standard input.

The :mod:`shlex` module defines the following class:


.. class:: shlex([instream[, infile[, posix]]])

   A :class:`shlex` instance or subclass instance is a lexical analyzer object.
   The initialization argument, if present, specifies where to read characters
   from. It must be a file-/stream-like object with :meth:`read` and
   :meth:`readline` methods, or a string (strings are accepted since Python 2.3).
   If no argument is given, input will be taken from ``sys.stdin``.  The second
   optional argument is a filename string, which sets the initial value of the
   :attr:`infile` member.  If the *instream* argument is omitted or equal to
   ``sys.stdin``, this second argument defaults to "stdin".  The *posix* argument
   was introduced in Python 2.3, and defines the operational mode.  When *posix* is
   not true (default), the :class:`shlex` instance will operate in compatibility
   mode.  When operating in POSIX mode, :class:`shlex` will try to be as close as
   possible to the POSIX shell parsing rules.


.. seealso::

   Module :mod:`ConfigParser`
      Parser for configuration files similar to the Windows :file:`.ini` files.


.. _shlex-objects:

shlex Objects
-------------

A :class:`shlex` instance has the following methods:


.. method:: shlex.get_token()

   Return a token.  If tokens have been stacked using :meth:`push_token`, pop a
   token off the stack.  Otherwise, read one from the input stream.  If reading
   encounters an immediate end-of-file, :attr:`self.eof` is returned (the empty
   string (``''``) in non-POSIX mode, and ``None`` in POSIX mode).


.. method:: shlex.push_token(str)

   Push the argument onto the token stack.


.. method:: shlex.read_token()

   Read a raw token.  Ignore the pushback stack, and do not interpret source
   requests.  (This is not ordinarily a useful entry point, and is documented here
   only for the sake of completeness.)


.. method:: shlex.sourcehook(filename)

   When :class:`shlex` detects a source request (see :attr:`source` below) this
   method is given the following token as argument, and expected to return a tuple
   consisting of a filename and an open file-like object.

   Normally, this method first strips any quotes off the argument.  If the result
   is an absolute pathname, or there was no previous source request in effect, or
   the previous source was a stream (such as ``sys.stdin``), the result is left
   alone.  Otherwise, if the result is a relative pathname, the directory part of
   the name of the file immediately before it on the source inclusion stack is
   prepended (this behavior is like the way the C preprocessor handles ``#include
   "file.h"``).

   The result of the manipulations is treated as a filename, and returned as the
   first component of the tuple, with :func:`open` called on it to yield the second
   component. (Note: this is the reverse of the order of arguments in instance
   initialization!)

   This hook is exposed so that you can use it to implement directory search paths,
   addition of file extensions, and other namespace hacks. There is no
   corresponding 'close' hook, but a shlex instance will call the :meth:`close`
   method of the sourced input stream when it returns EOF.

   For more explicit control of source stacking, use the :meth:`push_source` and
   :meth:`pop_source` methods.


.. method:: shlex.push_source(stream[, filename])

   Push an input source stream onto the input stack.  If the filename argument is
   specified it will later be available for use in error messages.  This is the
   same method used internally by the :meth:`sourcehook` method.

   .. versionadded:: 2.1


.. method:: shlex.pop_source()

   Pop the last-pushed input source from the input stack. This is the same method
   used internally when the lexer reaches EOF on a stacked input stream.

   .. versionadded:: 2.1


.. method:: shlex.error_leader([file[, line]])

   This method generates an error message leader in the format of a Unix C compiler
   error label; the format is ``'"%s", line %d: '``, where the ``%s`` is replaced
   with the name of the current source file and the ``%d`` with the current input
   line number (the optional arguments can be used to override these).

   This convenience is provided to encourage :mod:`shlex` users to generate error
   messages in the standard, parseable format understood by Emacs and other Unix
   tools.

Instances of :class:`shlex` subclasses have some public instance variables which
either control lexical analysis or can be used for debugging:


.. attribute:: shlex.commenters

   The string of characters that are recognized as comment beginners. All
   characters from the comment beginner to end of line are ignored. Includes just
   ``'#'`` by default.


.. attribute:: shlex.wordchars

   The string of characters that will accumulate into multi-character tokens.  By
   default, includes all ASCII alphanumerics and underscore.


.. attribute:: shlex.whitespace

   Characters that will be considered whitespace and skipped.  Whitespace bounds
   tokens.  By default, includes space, tab, linefeed and carriage-return.


.. attribute:: shlex.escape

   Characters that will be considered as escape. This will be only used in POSIX
   mode, and includes just ``'\'`` by default.

   .. versionadded:: 2.3


.. attribute:: shlex.quotes

   Characters that will be considered string quotes.  The token accumulates until
   the same quote is encountered again (thus, different quote types protect each
   other as in the shell.)  By default, includes ASCII single and double quotes.


.. attribute:: shlex.escapedquotes

   Characters in :attr:`quotes` that will interpret escape characters defined in
   :attr:`escape`.  This is only used in POSIX mode, and includes just ``'"'`` by
   default.

   .. versionadded:: 2.3


.. attribute:: shlex.whitespace_split

   If ``True``, tokens will only be split in whitespaces. This is useful, for
   example, for parsing command lines with :class:`shlex`, getting tokens in a
   similar way to shell arguments.

   .. versionadded:: 2.3


.. attribute:: shlex.infile

   The name of the current input file, as initially set at class instantiation time
   or stacked by later source requests.  It may be useful to examine this when
   constructing error messages.


.. attribute:: shlex.instream

   The input stream from which this :class:`shlex` instance is reading characters.


.. attribute:: shlex.source

   This member is ``None`` by default.  If you assign a string to it, that string
   will be recognized as a lexical-level inclusion request similar to the
   ``source`` keyword in various shells.  That is, the immediately following token
   will opened as a filename and input taken from that stream until EOF, at which
   point the :meth:`close` method of that stream will be called and the input
   source will again become the original input stream. Source requests may be
   stacked any number of levels deep.


.. attribute:: shlex.debug

   If this member is numeric and ``1`` or more, a :class:`shlex` instance will
   print verbose progress output on its behavior.  If you need to use this, you can
   read the module source code to learn the details.


.. attribute:: shlex.lineno

   Source line number (count of newlines seen so far plus one).


.. attribute:: shlex.token

   The token buffer.  It may be useful to examine this when catching exceptions.


.. attribute:: shlex.eof

   Token used to determine end of file. This will be set to the empty string
   (``''``), in non-POSIX mode, and to ``None`` in POSIX mode.

   .. versionadded:: 2.3


.. _shlex-parsing-rules:

Parsing Rules
-------------

When operating in non-POSIX mode, :class:`shlex` will try to obey to the
following rules.

* Quote characters are not recognized within words (``Do"Not"Separate`` is
  parsed as the single word ``Do"Not"Separate``);

* Escape characters are not recognized;

* Enclosing characters in quotes preserve the literal value of all characters
  within the quotes;

* Closing quotes separate words (``"Do"Separate`` is parsed as ``"Do"`` and
  ``Separate``);

* If :attr:`whitespace_split` is ``False``, any character not declared to be a
  word character, whitespace, or a quote will be returned as a single-character
  token. If it is ``True``, :class:`shlex` will only split words in whitespaces;

* EOF is signaled with an empty string (``''``);

* It's not possible to parse empty strings, even if quoted.

When operating in POSIX mode, :class:`shlex` will try to obey to the following
parsing rules.

* Quotes are stripped out, and do not separate words (``"Do"Not"Separate"`` is
  parsed as the single word ``DoNotSeparate``);

* Non-quoted escape characters (e.g. ``'\'``) preserve the literal value of the
  next character that follows;

* Enclosing characters in quotes which are not part of :attr:`escapedquotes`
  (e.g. ``"'"``) preserve the literal value of all characters within the quotes;

* Enclosing characters in quotes which are part of :attr:`escapedquotes` (e.g.
  ``'"'``) preserves the literal value of all characters within the quotes, with
  the exception of the characters mentioned in :attr:`escape`. The escape
  characters retain its special meaning only when followed by the quote in use, or
  the escape character itself. Otherwise the escape character will be considered a
  normal character.

* EOF is signaled with a :const:`None` value;

* Quoted empty strings (``''``) are allowed;

