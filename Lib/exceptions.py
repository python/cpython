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

StandardError(*)
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
 +-- NumberError(*)
 |    |
 |    +-- OverflowError
 |    +-- ZeroDivisionError
 |    +-- FloatingPointError
 |
 +-- ValueError
 +-- SystemError
 +-- MemoryError
"""

class StandardError:
    def __init__(self, *args):
        if len(args) == 0:
            self.args = None
        elif len(args) == 1:
            # de-tuplify
            self.args = args[0]
        else:
            self.args = args

    def __str__(self):
        if self.args == None:
            return ''
	else:
	    return str(self.args)

    def __getitem__(self, i):
	if type(self.args) == type(()):
	    return self.args[i]
	elif i == 0:
	    return self.args
	else:
	    raise IndexError

class SyntaxError(StandardError):
    filename = lineno = offset = text = None
    def __init__(self, msg, info=None):
        self.msg = msg
	if info:
	    self.args = msg
	else:
	    self.args = (msg, info)
	if info:
	    self.filename, self.lineno, self.offset, self.text = info
    def __str__(self):
        return str(self.msg)


class IOError(StandardError):
    def __init__(self, *args):
        self.errno = None
        self.strerror = None
        if len(args) == 1:
            # de-tuplify
            self.args = args[0]
        elif len(args) == 2:
            # common case: PyErr_SetFromErrno()
            self.args = args
            self.errno = args[0]
            self.strerror = args[1]
        else:
            self.args = args


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

class NumberError(StandardError):
    pass

class OverflowError(NumberError):
    pass

class FloatingPointError(NumberError):
    pass

class ZeroDivisionError(NumberError):
    pass

class LookupError(StandardError):
    pass

class IndexError(LookupError):
    pass

class KeyError(LookupError):
    pass

# debate: should these two inherit from LookupError?
class AttributeError(StandardError):
    pass

class NameError(StandardError):
    pass

class SystemExit(StandardError):
    def __init__(self, *args):
        if len(args) == 0:
            self.args = None
        elif len(args) == 1:
            # de-tuplify
            self.args = args[0]
        else:
            self.args = args
        self.code = self.args


class MemoryError(StandardError):
    pass
