"""Class based built-in exception hierarchy.

New with Python 1.5, all standard built-in exceptions are now class objects by
default.  This gives Python's exception handling mechanism a more
object-oriented feel.  Traditionally they were string objects.  Python will
fallback to string based exceptions if the interpreter is invoked with the -X
option, or if some failure occurs during class exception initialization (in
this case a warning will be printed).

Most existing code should continue to work with class based exceptions.  Some
tricky uses of IOError may break, but the most common uses should work.

Here is a rundown of the class hierarchy.  You can change this by editing this
file, but it isn't recommended because the old string based exceptions won't
be kept in sync.  The class names described here are expected to be found by
the bltinmodule.c file.  If you add classes here, you must modify
bltinmodule.c or the exceptions won't be available in the __builtin__ module,
nor will they be accessible from C.

The classes with a `*' are new since Python 1.5.  They are defined as tuples
containing the derived exceptions when string-based exceptions are used.  If
you define your own class based exceptions, they should be derived from
Exception.

Exception(*)
 |
 +-- SystemExit
 +-- StandardError(*)
      |
      +-- KeyboardInterrupt
      +-- ImportError
      +-- EnvironmentError(*)
      |    |
      |    +-- IOError
      |    +-- OSError(*)
      |
      +-- EOFError
      +-- RuntimeError
      |    |
      |    +-- NotImplementedError(*)
      |
      +-- NameError
      +-- AttributeError
      +-- SyntaxError
      +-- TypeError
      +-- AssertionError
      +-- LookupError(*)
      |    |
      |    +-- IndexError
      |    +-- KeyError
      |
      +-- ArithmeticError(*)
      |    |
      |    +-- OverflowError
      |    +-- ZeroDivisionError
      |    +-- FloatingPointError
      |
      +-- ValueError
      +-- SystemError
      +-- MemoryError
"""

class Exception:
    """Proposed base class for all exceptions."""
    def __init__(self, *args):
        self.args = args

    def __str__(self):
        if not self.args:
            return ''
        elif len(self.args) == 1:
            return str(self.args[0])
        else:
            return str(self.args)

    def __getitem__(self, i):
        return self.args[i]

class StandardError(Exception):
    """Base class for all standard Python exceptions."""
    pass

class SyntaxError(StandardError):
    """Invalid syntax."""
    filename = lineno = offset = text = None
    msg = ""
    def __init__(self, *args):
        self.args = args
        if len(self.args) >= 1:
            self.msg = self.args[0]
        if len(self.args) == 2:
            info = self.args[1]
            try:
                self.filename, self.lineno, self.offset, self.text = info
            except:
                pass
    def __str__(self):
        return str(self.msg)

class EnvironmentError(StandardError):
    """Base class for I/O related errors."""
    def __init__(self, *args):
        self.args = args
        self.errno = None
        self.strerror = None
        self.filename = None
        if len(args) == 3:
            # open() errors give third argument which is the filename.  BUT,
            # so common in-place unpacking doesn't break, e.g.:
            #
            # except IOError, (errno, strerror):
            #
            # we hack args so that it only contains two items.  This also
            # means we need our own __str__() which prints out the filename
            # when it was supplied.
            self.errno, self.strerror, self.filename = args
            self.args = args[0:2]
        if len(args) == 2:
            # common case: PyErr_SetFromErrno()
            self.errno, self.strerror = args

    def __str__(self):
        if self.filename is not None:
            return '[Errno %s] %s: %s' % (self.errno, self.strerror,
                                          repr(self.filename))
        elif self.errno and self.strerror:
            return '[Errno %s] %s' % (self.errno, self.strerror)
        else:
            return StandardError.__str__(self)

class IOError(EnvironmentError):
    """I/O operation failed."""
    pass

class OSError(EnvironmentError):
    """OS system call failed."""
    pass

class RuntimeError(StandardError):
    """Unspecified run-time error."""
    pass

class NotImplementedError(RuntimeError):
    """Method or function hasn't been implemented yet."""
    pass

class SystemError(StandardError):
    """Internal error in the Python interpreter.

    Please report this to the Python maintainer, along with the traceback,
    the Python version, and the hardware/OS platform and version."""
    pass

class EOFError(StandardError):
    """Read beyond end of file."""
    pass

class ImportError(StandardError):
    """Import can't find module, or can't find name in module."""
    pass

class TypeError(StandardError):
    """Inappropriate argument type."""
    pass

class ValueError(StandardError):
    """Inappropriate argument value (of correct type)."""
    pass

class KeyboardInterrupt(StandardError):
    """Program interrupted by user."""
    pass

class AssertionError(StandardError):
    """Assertion failed."""
    pass

class ArithmeticError(StandardError):
    """Base class for arithmetic errors."""
    pass

class OverflowError(ArithmeticError):
    """Result too large to be represented."""
    pass

class FloatingPointError(ArithmeticError):
    """Floating point operation failed."""
    pass

class ZeroDivisionError(ArithmeticError):
    """Second argument to a division or modulo operation was zero."""
    pass

class LookupError(StandardError):
    """Base class for lookup errors."""
    pass

class IndexError(LookupError):
    """Sequence index out of range."""
    pass

class KeyError(LookupError):
    """Mapping key not found."""
    pass

class AttributeError(StandardError):
    """Attribute not found."""
    pass

class NameError(StandardError):
    """Name not found locally or globally."""
    pass

class MemoryError(StandardError):
    """Out of memory."""
    pass

class SystemExit(Exception):
    """Request to exit from the interpreter."""
    def __init__(self, *args):
        self.args = args
        if len(args) == 0:
            self.code = None
        elif len(args) == 1:
            self.code = args[0]
        else:
            self.code = args
