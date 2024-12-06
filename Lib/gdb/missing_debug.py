# Copyright (C) 2023-2024 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""
MissingDebugHandler base class, and register_handler function.
"""

import sys

import gdb

if sys.version_info >= (3, 7):
    # Functions str.isascii() and str.isalnum are available starting Python
    # 3.7.
    def isascii(ch):
        return ch.isascii()

    def isalnum(ch):
        return ch.isalnum()

else:
    # Fall back to curses.ascii.isascii() and curses.ascii.isalnum() for
    # earlier versions.
    from curses.ascii import isalnum, isascii


def _validate_name(name):
    """Validate a missing debug handler name string.

    If name is valid as a missing debug handler name, then this
    function does nothing.  If name is not valid then an exception is
    raised.

    Arguments:
        name: A string, the name of a missing debug handler.

    Returns:
        Nothing.

    Raises:
        ValueError: If name is invalid as a missing debug handler
                    name.
    """
    for ch in name:
        if not isascii(ch) or not (isalnum(ch) or ch in "_-"):
            raise ValueError("invalid character '%s' in handler name: %s" % (ch, name))


class MissingDebugHandler(object):
    """Base class for missing debug handlers written in Python.

    A missing debug handler has a single method __call__ along with
    the read/write attribute enabled, and a read-only attribute name.

    Attributes:
        name: Read-only attribute, the name of this handler.
        enabled: When true this handler is enabled.
    """

    def __init__(self, name):
        """Constructor.

        Args:
            name: An identifying name for this handler.

        Raises:
            TypeError: name is not a string.
            ValueError: name contains invalid characters.
        """

        if not isinstance(name, str):
            raise TypeError("incorrect type for name: %s" % type(name))

        _validate_name(name)

        self._name = name
        self._enabled = True

    @property
    def name(self):
        return self._name

    @property
    def enabled(self):
        return self._enabled

    @enabled.setter
    def enabled(self, value):
        if not isinstance(value, bool):
            raise TypeError("incorrect type for enabled attribute: %s" % type(value))
        self._enabled = value

    def __call__(self, objfile):
        """GDB handle missing debug information for an objfile.

        Arguments:
            objfile: A gdb.Objfile for which GDB could not find any
                debug information.

        Returns:
            True: GDB should try again to locate the debug information
                for objfile, the handler may have installed the
                missing information.
            False: GDB should move on without the debug information
                for objfile.
            A string: GDB should load the file at the given path; it
                contains the debug information for objfile.
            None: This handler can't help with objfile.  GDB should
                try any other registered handlers.
        """
        raise NotImplementedError("MissingDebugHandler.__call__()")


def register_handler(locus, handler, replace=False):
    """Register handler in given locus.

    The handler is prepended to the locus's missing debug handlers
    list. The name of handler should be unique (or replace must be
    True).

    Arguments:
        locus: Either a progspace, or None (in which case the unwinder
               is registered globally).
        handler: An object of a gdb.MissingDebugHandler subclass.

        replace: If True, replaces existing handler with the same name
                 within locus.  Otherwise, raises RuntimeException if
                 unwinder with the same name already exists.

    Returns:
        Nothing.

    Raises:
        RuntimeError: The name of handler is not unique.
        TypeError: Bad locus type.
        AttributeError: Required attributes of handler are missing.
    """

    if locus is None:
        if gdb.parameter("verbose"):
            gdb.write("Registering global %s handler ...\n" % handler.name)
        locus = gdb
    elif isinstance(locus, gdb.Progspace):
        if gdb.parameter("verbose"):
            gdb.write(
                "Registering %s handler for %s ...\n" % (handler.name, locus.filename)
            )
    else:
        raise TypeError("locus should be gdb.Progspace or None")

    # Some sanity checks on HANDLER.  Calling getattr will raise an
    # exception if the attribute doesn't exist, which is what we want.
    # These checks are not exhaustive; we don't check the attributes
    # have the correct types, or the method has the correct signature,
    # but this should catch some basic mistakes.
    getattr(handler, "name")
    getattr(handler, "enabled")
    call_method = getattr(handler, "__call__")
    if not callable(call_method):
        raise AttributeError(
            "'%s' object's '__call__' attribute is not callable"
            % type(handler).__name__
        )

    i = 0
    for needle in locus.missing_debug_handlers:
        if needle.name == handler.name:
            if replace:
                del locus.missing_debug_handlers[i]
            else:
                raise RuntimeError("Handler %s already exists." % handler.name)
        i += 1
    locus.missing_debug_handlers.insert(0, handler)
