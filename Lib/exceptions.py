"""Class based built-in exception hierarchy.

This is a new feature whereby all the standard built-in exceptions,
traditionally string objects, are replaced with classes.  This gives
Python's exception handling mechanism a more object-oriented feel.

Most existing code should continue to work with class based
exceptions.  Some tricky uses of IOError may break, but the most
common uses should work.

To disable this feature, start the Python executable with the -X option.

Here is a rundown of the class hierarchy.  You can change this by
editing this file, but it isn't recommended.  The classes with a `*'
are new with this feature.  They are defined as tuples containing the
derived exceptions when string-based exceptions are used.

Exception(*)
 |
 +-- StandardError(*)
      |
      +-- SystemExit
      +-- KeyboardInterrupt
      +-- ImportError
      +-- IOError
      +-- EOFError
      +-- RuntimeError
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
    pass

class SyntaxError(StandardError):
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

class IOError(StandardError):
    def __init__(self, *args):
	self.args = args
        self.errno = None
        self.strerror = None
        if len(args) == 2:
            # common case: PyErr_SetFromErrno()
            self.errno = args[0]
            self.strerror = args[1]

class RuntimeError(StandardError):
    pass

class SystemError(StandardError):
    pass

class EOFError(StandardError):
    pass

class ImportError(StandardError):
    pass

class TypeError(StandardError):
    pass

class ValueError(StandardError):
    pass

class KeyboardInterrupt(StandardError):
    pass

class AssertionError(StandardError):
    pass

class ArithmeticError(StandardError):
    pass

class OverflowError(ArithmeticError):
    pass

class FloatingPointError(ArithmeticError):
    pass

class ZeroDivisionError(ArithmeticError):
    pass

class LookupError(StandardError):
    pass

class IndexError(LookupError):
    pass

class KeyError(LookupError):
    pass

class AttributeError(StandardError):
    pass

class NameError(StandardError):
    pass

class MemoryError(StandardError):
    pass

class SystemExit(Exception):
    def __init__(self, *args):
	self.args = args
        if len(args) == 0:
            self.code = None
        elif len(args) == 1:
            self.code = args[0]
        else:
            self.code = args
