"""Generate the skeleton for cStringIO as an example of framer."""

from framer.bases import Module, Type
from framer.member import member

class cStringIO(Module):
    """A simple fast partial StringIO replacement.

    This module provides a simple useful replacement for the StringIO
    module that is written in C.  It does not provide the full
    generality of StringIO, but it provides enough for most
    applications and is especially useful in conjunction with the
    pickle module.

    Usage:

    from cStringIO import StringIO

    an_output_stream = StringIO()
    an_output_stream.write(some_stuff)
    ...
    value = an_output_stream.getvalue()

    an_input_stream = StringIO(a_string)
    spam = an_input_stream.readline()
    spam = an_input_stream.read(5)
    an_input_stream.seek(0)             # OK, start over
    spam = an_input_stream.read()       # and read it all
    """

    __file__ = "cStringIO.c"

    def StringIO(o):
        """Return a StringIO-like stream for reading or writing"""
    StringIO.pyarg = "|O"

    class InputType(Type):
        "Simple type for treating strings as input file streams"

        abbrev = "input"

        struct = """\
        typedef struct {
                PyObject_HEAD
                char *buf;
                int pos;
                int size;
                PyObject *pbuf;
        } InputObject;
        """

        def flush(self):
            """Does nothing"""

        def getvalue(self):
            """Get the string value.

            If use_pos is specified and is a true value, then the
            string returned will include only the text up to the
            current file position.
            """

        def isatty(self):
            """Always returns False"""

        def read(self, s):
            """Return s characters or the rest of the string."""
        read.pyarg = "|i"

        def readline(self):
            """Read one line."""

        def readlines(self, hint):
            """Read all lines."""
        readlines.pyarg = "|i"

        def reset(self):
            """Reset the file position to the beginning."""

        def tell(self):
            """Get the current position."""

        def truncate(self, pos):
            """Truncate the file at the current position."""
        truncate.pyarg = "|i"

        def seek(self, position, mode=0):
            """Set the current position.

            The optional mode argument can be 0 for absolute, 1 for relative,
            and 2 for relative to EOF.  The default is absolute.
            """
        seek.pyarg = "i|i"

        def close(self):
            pass

    class OutputType(InputType):
        "Simple type for output strings."

        abbrev = "output"

        struct = """\
        typedef struct {
                PyObject_HEAD
                char *buf;
                int pos;
                int size;
                int softspace;
        } OutputObject;
        """

        softspace = member()

        def close(self):
            """Explicitly release resources."""

        def write(self, s):
            """Write a string to the file."""
            # XXX Hack: writing None resets the buffer

        def writelines(self, lines):
            """Write each string in lines."""


cStringIO.gen()
