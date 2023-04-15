:mod:`curses.ascii` --- Utilities for ASCII characters
======================================================

.. module:: curses.ascii
   :synopsis: Constants and set-membership functions for ASCII characters.

.. moduleauthor:: Eric S. Raymond <esr@thyrsus.com>
.. sectionauthor:: Eric S. Raymond <esr@thyrsus.com>

**Source code:** :source:`Lib/curses/ascii.py`

--------------

The :mod:`curses.ascii` module supplies name constants for ASCII characters and
functions to test membership in various ASCII character classes.  The constants
supplied are names for control characters as follows:

+--------------+----------------------------------------------+
| Name         | Meaning                                      |
+==============+==============================================+
| :const:`NUL` |                                              |
+--------------+----------------------------------------------+
| :const:`SOH` | Start of heading, console interrupt          |
+--------------+----------------------------------------------+
| :const:`STX` | Start of text                                |
+--------------+----------------------------------------------+
| :const:`ETX` | End of text                                  |
+--------------+----------------------------------------------+
| :const:`EOT` | End of transmission                          |
+--------------+----------------------------------------------+
| :const:`ENQ` | Enquiry, goes with :const:`ACK` flow control |
+--------------+----------------------------------------------+
| :const:`ACK` | Acknowledgement                              |
+--------------+----------------------------------------------+
| :const:`BEL` | Bell                                         |
+--------------+----------------------------------------------+
| :const:`BS`  | Backspace                                    |
+--------------+----------------------------------------------+
| :const:`TAB` | Tab                                          |
+--------------+----------------------------------------------+
| :const:`HT`  | Alias for :const:`TAB`: "Horizontal tab"     |
+--------------+----------------------------------------------+
| :const:`LF`  | Line feed                                    |
+--------------+----------------------------------------------+
| :const:`NL`  | Alias for :const:`LF`: "New line"            |
+--------------+----------------------------------------------+
| :const:`VT`  | Vertical tab                                 |
+--------------+----------------------------------------------+
| :const:`FF`  | Form feed                                    |
+--------------+----------------------------------------------+
| :const:`CR`  | Carriage return                              |
+--------------+----------------------------------------------+
| :const:`SO`  | Shift-out, begin alternate character set     |
+--------------+----------------------------------------------+
| :const:`SI`  | Shift-in, resume default character set       |
+--------------+----------------------------------------------+
| :const:`DLE` | Data-link escape                             |
+--------------+----------------------------------------------+
| :const:`DC1` | XON, for flow control                        |
+--------------+----------------------------------------------+
| :const:`DC2` | Device control 2, block-mode flow control    |
+--------------+----------------------------------------------+
| :const:`DC3` | XOFF, for flow control                       |
+--------------+----------------------------------------------+
| :const:`DC4` | Device control 4                             |
+--------------+----------------------------------------------+
| :const:`NAK` | Negative acknowledgement                     |
+--------------+----------------------------------------------+
| :const:`SYN` | Synchronous idle                             |
+--------------+----------------------------------------------+
| :const:`ETB` | End transmission block                       |
+--------------+----------------------------------------------+
| :const:`CAN` | Cancel                                       |
+--------------+----------------------------------------------+
| :const:`EM`  | End of medium                                |
+--------------+----------------------------------------------+
| :const:`SUB` | Substitute                                   |
+--------------+----------------------------------------------+
| :const:`ESC` | Escape                                       |
+--------------+----------------------------------------------+
| :const:`FS`  | File separator                               |
+--------------+----------------------------------------------+
| :const:`GS`  | Group separator                              |
+--------------+----------------------------------------------+
| :const:`RS`  | Record separator, block-mode terminator      |
+--------------+----------------------------------------------+
| :const:`US`  | Unit separator                               |
+--------------+----------------------------------------------+
| :const:`SP`  | Space                                        |
+--------------+----------------------------------------------+
| :const:`DEL` | Delete                                       |
+--------------+----------------------------------------------+

Note that many of these have little practical significance in modern usage.  The
mnemonics derive from teleprinter conventions that predate digital computers.

The module supplies the following functions, patterned on those in the standard
C library:


.. function:: isalnum(c)

   Checks for an ASCII alphanumeric character; it is equivalent to ``isalpha(c) or
   isdigit(c)``.


.. function:: isalpha(c)

   Checks for an ASCII alphabetic character; it is equivalent to ``isupper(c) or
   islower(c)``.


.. function:: isascii(c)

   Checks for a character value that fits in the 7-bit ASCII set.


.. function:: isblank(c)

   Checks for an ASCII whitespace character; space or horizontal tab.


.. function:: iscntrl(c)

   Checks for an ASCII control character (in the range 0x00 to 0x1f or 0x7f).


.. function:: isdigit(c)

   Checks for an ASCII decimal digit, ``'0'`` through ``'9'``.  This is equivalent
   to ``c in string.digits``.


.. function:: isgraph(c)

   Checks for ASCII any printable character except space.


.. function:: islower(c)

   Checks for an ASCII lower-case character.


.. function:: isprint(c)

   Checks for any ASCII printable character including space.


.. function:: ispunct(c)

   Checks for any printable ASCII character which is not a space or an alphanumeric
   character.


.. function:: isspace(c)

   Checks for ASCII white-space characters; space, line feed, carriage return, form
   feed, horizontal tab, vertical tab.


.. function:: isupper(c)

   Checks for an ASCII uppercase letter.


.. function:: isxdigit(c)

   Checks for an ASCII hexadecimal digit.  This is equivalent to ``c in
   string.hexdigits``.


.. function:: isctrl(c)

   Checks for an ASCII control character (ordinal values 0 to 31).


.. function:: ismeta(c)

   Checks for a non-ASCII character (ordinal values 0x80 and above).

These functions accept either integers or single-character strings; when the argument is a
string, it is first converted using the built-in function :func:`ord`.

Note that all these functions check ordinal bit values derived from the
character of the string you pass in; they do not actually know anything about
the host machine's character encoding.

The following two functions take either a single-character string or integer
byte value; they return a value of the same type.


.. function:: ascii(c)

   Return the ASCII value corresponding to the low 7 bits of *c*.


.. function:: ctrl(c)

   Return the control character corresponding to the given character (the character
   bit value is bitwise-anded with 0x1f).


.. function:: alt(c)

   Return the 8-bit character corresponding to the given ASCII character (the
   character bit value is bitwise-ored with 0x80).

The following function takes either a single-character string or integer value;
it returns a string.


.. index::
   single: ^ (caret); in curses module
   single: ! (exclamation); in curses module

.. function:: unctrl(c)

   Return a string representation of the ASCII character *c*.  If *c* is printable,
   this string is the character itself.  If the character is a control character
   (0x00--0x1f) the string consists of a caret (``'^'``) followed by the
   corresponding uppercase letter. If the character is an ASCII delete (0x7f) the
   string is ``'^?'``.  If the character has its meta bit (0x80) set, the meta bit
   is stripped, the preceding rules applied, and ``'!'`` prepended to the result.


.. data:: controlnames

   A 33-element string array that contains the ASCII mnemonics for the thirty-two
   ASCII control characters from 0 (NUL) to 0x1f (US), in order, plus the mnemonic
   ``SP`` for the space character.

