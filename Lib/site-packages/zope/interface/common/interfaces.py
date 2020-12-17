##############################################################################
#
# Copyright (c) 2003 Zope Foundation and Contributors.
# All Rights Reserved.
#
# This software is subject to the provisions of the Zope Public License,
# Version 2.1 (ZPL).  A copy of the ZPL should accompany this distribution.
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY AND ALL EXPRESS OR IMPLIED
# WARRANTIES ARE DISCLAIMED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF TITLE, MERCHANTABILITY, AGAINST INFRINGEMENT, AND FITNESS
# FOR A PARTICULAR PURPOSE.
#
##############################################################################
"""Interfaces for standard python exceptions
"""
from zope.interface import Interface
from zope.interface import classImplements

class IException(Interface):
    "Interface for `Exception`"
classImplements(Exception, IException)


class IStandardError(IException):
    "Interface for `StandardError` (Python 2 only.)"
try:
    classImplements(StandardError, IStandardError)
except NameError:  #pragma NO COVER
    pass # StandardError does not exist in Python 3


class IWarning(IException):
    "Interface for `Warning`"
classImplements(Warning, IWarning)


class ISyntaxError(IStandardError):
    "Interface for `SyntaxError`"
classImplements(SyntaxError, ISyntaxError)


class ILookupError(IStandardError):
    "Interface for `LookupError`"
classImplements(LookupError, ILookupError)


class IValueError(IStandardError):
    "Interface for `ValueError`"
classImplements(ValueError, IValueError)


class IRuntimeError(IStandardError):
    "Interface for `RuntimeError`"
classImplements(RuntimeError, IRuntimeError)


class IArithmeticError(IStandardError):
    "Interface for `ArithmeticError`"
classImplements(ArithmeticError, IArithmeticError)


class IAssertionError(IStandardError):
    "Interface for `AssertionError`"
classImplements(AssertionError, IAssertionError)


class IAttributeError(IStandardError):
    "Interface for `AttributeError`"
classImplements(AttributeError, IAttributeError)


class IDeprecationWarning(IWarning):
    "Interface for `DeprecationWarning`"
classImplements(DeprecationWarning, IDeprecationWarning)


class IEOFError(IStandardError):
    "Interface for `EOFError`"
classImplements(EOFError, IEOFError)


class IEnvironmentError(IStandardError):
    "Interface for `EnvironmentError`"
classImplements(EnvironmentError, IEnvironmentError)


class IFloatingPointError(IArithmeticError):
    "Interface for `FloatingPointError`"
classImplements(FloatingPointError, IFloatingPointError)


class IIOError(IEnvironmentError):
    "Interface for `IOError`"
classImplements(IOError, IIOError)


class IImportError(IStandardError):
    "Interface for `ImportError`"
classImplements(ImportError, IImportError)


class IIndentationError(ISyntaxError):
    "Interface for `IndentationError`"
classImplements(IndentationError, IIndentationError)


class IIndexError(ILookupError):
    "Interface for `IndexError`"
classImplements(IndexError, IIndexError)


class IKeyError(ILookupError):
    "Interface for `KeyError`"
classImplements(KeyError, IKeyError)


class IKeyboardInterrupt(IStandardError):
    "Interface for `KeyboardInterrupt`"
classImplements(KeyboardInterrupt, IKeyboardInterrupt)


class IMemoryError(IStandardError):
    "Interface for `MemoryError`"
classImplements(MemoryError, IMemoryError)


class INameError(IStandardError):
    "Interface for `NameError`"
classImplements(NameError, INameError)


class INotImplementedError(IRuntimeError):
    "Interface for `NotImplementedError`"
classImplements(NotImplementedError, INotImplementedError)


class IOSError(IEnvironmentError):
    "Interface for `OSError`"
classImplements(OSError, IOSError)


class IOverflowError(IArithmeticError):
    "Interface for `ArithmeticError`"
classImplements(OverflowError, IOverflowError)


class IOverflowWarning(IWarning):
    """Deprecated, no standard class implements this.

    This was the interface for ``OverflowWarning`` prior to Python 2.5,
    but that class was removed for all versions after that.
    """


class IReferenceError(IStandardError):
    "Interface for `ReferenceError`"
classImplements(ReferenceError, IReferenceError)


class IRuntimeWarning(IWarning):
    "Interface for `RuntimeWarning`"
classImplements(RuntimeWarning, IRuntimeWarning)


class IStopIteration(IException):
    "Interface for `StopIteration`"
classImplements(StopIteration, IStopIteration)


class ISyntaxWarning(IWarning):
    "Interface for `SyntaxWarning`"
classImplements(SyntaxWarning, ISyntaxWarning)


class ISystemError(IStandardError):
    "Interface for `SystemError`"
classImplements(SystemError, ISystemError)


class ISystemExit(IException):
    "Interface for `SystemExit`"
classImplements(SystemExit, ISystemExit)


class ITabError(IIndentationError):
    "Interface for `TabError`"
classImplements(TabError, ITabError)


class ITypeError(IStandardError):
    "Interface for `TypeError`"
classImplements(TypeError, ITypeError)


class IUnboundLocalError(INameError):
    "Interface for `UnboundLocalError`"
classImplements(UnboundLocalError, IUnboundLocalError)


class IUnicodeError(IValueError):
    "Interface for `UnicodeError`"
classImplements(UnicodeError, IUnicodeError)


class IUserWarning(IWarning):
    "Interface for `UserWarning`"
classImplements(UserWarning, IUserWarning)


class IZeroDivisionError(IArithmeticError):
    "Interface for `ZeroDivisionError`"
classImplements(ZeroDivisionError, IZeroDivisionError)
