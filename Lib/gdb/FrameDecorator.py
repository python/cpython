# Copyright (C) 2013-2024 Free Software Foundation, Inc.

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

import gdb


class _FrameDecoratorBase(object):
    """Base class of frame decorators."""

    # 'base' can refer to a gdb.Frame or another frame decorator.  In
    # the latter case, the child class will have called the super
    # method and _base will be an object conforming to the Frame Filter
    # class.
    def __init__(self, base):
        self._base = base

    @staticmethod
    def __is_limited_frame(frame):
        """Internal utility to determine if the frame is special or
        limited."""
        sal = frame.find_sal()

        if (
            not sal.symtab
            or not sal.symtab.filename
            or frame.type() == gdb.DUMMY_FRAME
            or frame.type() == gdb.SIGTRAMP_FRAME
        ):
            return True

        return False

    def elided(self):
        """Return any elided frames that this class might be
        wrapping, or None."""
        if hasattr(self._base, "elided"):
            return self._base.elided()

        return None

    def function(self):
        """Return the name of the frame's function or an address of
        the function of the frame.  First determine if this is a
        special frame.  If not, try to determine filename from GDB's
        frame internal function API.  Finally, if a name cannot be
        determined return the address.  If this function returns an
        address, GDB will attempt to determine the function name from
        its internal minimal symbols store (for example, for inferiors
        without debug-info)."""

        # Both gdb.Frame, and FrameDecorator have a method called
        # "function", so determine which object this is.
        if not isinstance(self._base, gdb.Frame):
            if hasattr(self._base, "function"):
                # If it is not a gdb.Frame, and there is already a
                # "function" method, use that.
                return self._base.function()

        frame = self.inferior_frame()

        if frame.type() == gdb.DUMMY_FRAME:
            return "<function called from gdb>"
        elif frame.type() == gdb.SIGTRAMP_FRAME:
            return "<signal handler called>"

        func = frame.name()
        if not isinstance(func, str):
            func = "???"
        return func

    def address(self):
        """Return the address of the frame's pc"""

        if hasattr(self._base, "address"):
            return self._base.address()

        frame = self.inferior_frame()
        return frame.pc()

    def frame_args(self):
        """Return an iterable of frame arguments for this frame, if
        any.  The iterable object contains objects conforming with the
        Symbol/Value interface.  If there are no frame arguments, or
        if this frame is deemed to be a special case, return None."""

        if hasattr(self._base, "frame_args"):
            return self._base.frame_args()

        frame = self.inferior_frame()
        if self.__is_limited_frame(frame):
            return None

        args = FrameVars(frame)
        return args.fetch_frame_args()

    def frame_locals(self):
        """Return an iterable of local variables for this frame, if
        any.  The iterable object contains objects conforming with the
        Symbol/Value interface.  If there are no frame locals, or if
        this frame is deemed to be a special case, return None."""

        if hasattr(self._base, "frame_locals"):
            return self._base.frame_locals()

        frame = self.inferior_frame()
        if self.__is_limited_frame(frame):
            return None

        args = FrameVars(frame)
        return args.fetch_frame_locals()

    def line(self):
        """Return line number information associated with the frame's
        pc.  If symbol table/line information does not exist, or if
        this frame is deemed to be a special case, return None"""

        if hasattr(self._base, "line"):
            return self._base.line()

        frame = self.inferior_frame()
        if self.__is_limited_frame(frame):
            return None

        sal = frame.find_sal()
        if sal:
            return sal.line
        else:
            return None

    def inferior_frame(self):
        """Return the gdb.Frame underpinning this frame decorator."""

        # If 'base' is a frame decorator, we want to call its inferior
        # frame method.  If '_base' is a gdb.Frame, just return that.
        if hasattr(self._base, "inferior_frame"):
            return self._base.inferior_frame()
        return self._base


class FrameDecorator(_FrameDecoratorBase):
    """Basic implementation of a Frame Decorator

    This base frame decorator decorates a frame or another frame
    decorator, and provides convenience methods.  If this object is
    wrapping a frame decorator, defer to that wrapped object's method
    if it has one.  This allows for frame decorators that have
    sub-classed FrameDecorator object, but also wrap other frame
    decorators on the same frame to correctly execute.

    E.g

    If the result of frame filters running means we have one gdb.Frame
    wrapped by multiple frame decorators, all sub-classed from
    FrameDecorator, the resulting hierarchy will be:

    Decorator1
      -- (wraps) Decorator2
        -- (wraps) FrameDecorator
          -- (wraps) gdb.Frame

    In this case we have two frame decorators, both of which are
    sub-classed from FrameDecorator.  If Decorator1 just overrides the
    'function' method, then all of the other methods are carried out
    by the super-class FrameDecorator.  But Decorator2 may have
    overriden other methods, so FrameDecorator will look at the
    'base' parameter and defer to that class's methods.  And so on,
    down the chain."""

    def filename(self):
        """Return the filename associated with this frame, detecting
        and returning the appropriate library name is this is a shared
        library."""

        if hasattr(self._base, "filename"):
            return self._base.filename()

        frame = self.inferior_frame()
        sal = frame.find_sal()
        if not sal.symtab or not sal.symtab.filename:
            pc = frame.pc()
            return gdb.solib_name(pc)
        else:
            return sal.symtab.filename


class DAPFrameDecorator(_FrameDecoratorBase):
    """Like FrameDecorator, but has slightly different results
    for the "filename" method."""

    def filename(self):
        """Return the filename associated with this frame, detecting
        and returning the appropriate library name is this is a shared
        library."""

        if hasattr(self._base, "filename"):
            return self._base.filename()

        frame = self.inferior_frame()
        sal = frame.find_sal()
        if sal.symtab is not None:
            return sal.symtab.fullname()
        return None

    def frame_locals(self):
        """Return an iterable of local variables for this frame, if
        any.  The iterable object contains objects conforming with the
        Symbol/Value interface.  If there are no frame locals, or if
        this frame is deemed to be a special case, return None."""

        if hasattr(self._base, "frame_locals"):
            return self._base.frame_locals()

        frame = self.inferior_frame()
        args = FrameVars(frame)
        return args.fetch_frame_locals(True)


class SymValueWrapper(object):
    """A container class conforming to the Symbol/Value interface
    which holds frame locals or frame arguments."""

    # The FRAME argument is needed here because gdb.Symbol doesn't
    # carry the block with it, and so read_var can't find symbols from
    # outer (static link) frames.
    def __init__(self, frame, symbol):
        self.frame = frame
        self.sym = symbol

    def value(self):
        """Return the value associated with this symbol, or None"""
        if self.frame is None:
            return None
        return self.frame.read_var(self.sym)

    def symbol(self):
        """Return the symbol, or Python text, associated with this
        symbol, or None"""
        return self.sym


class FrameVars(object):
    """Utility class to fetch and store frame local variables, or
    frame arguments."""

    def __init__(self, frame):
        self.frame = frame

    def fetch_frame_locals(self, follow_link=False):
        """Public utility method to fetch frame local variables for
        the stored frame.  Frame arguments are not fetched.  If there
        are no frame local variables, return an empty list."""
        lvars = []

        frame = self.frame
        try:
            block = frame.block()
        except RuntimeError:
            block = None

        traversed_link = False
        while block is not None:
            if block.is_global or block.is_static:
                break
            for sym in block:
                # Exclude arguments from the innermost function, but
                # if we found and traversed a static link, just treat
                # all such variables as "local".
                if sym.is_argument:
                    if not traversed_link:
                        continue
                elif not sym.is_variable:
                    # We use an 'elif' here because is_variable
                    # returns False for arguments as well.  Anyway,
                    # don't include non-variables here.
                    continue
                lvars.append(SymValueWrapper(frame, sym))

            if block.function is not None:
                if not follow_link:
                    break
                # If the frame has a static link, follow it here.
                traversed_link = True
                frame = frame.static_link()
                if frame is None:
                    break
                try:
                    block = frame.block()
                except RuntimeError:
                    block = None
            else:
                block = block.superblock

        return lvars

    def fetch_frame_args(self):
        """Public utility method to fetch frame arguments for the
        stored frame.  Frame arguments are the only type fetched.  If
        there are no frame argument variables, return an empty list."""

        args = []

        try:
            block = self.frame.block()
        except RuntimeError:
            block = None

        while block is not None:
            if block.is_global or block.is_static:
                break
            for sym in block:
                if not sym.is_argument:
                    continue
                args.append(SymValueWrapper(None, sym))

            # Stop when the function itself is seen, to avoid showing
            # variables from outer functions in a nested function.
            # Note that we don't traverse the static link for
            # arguments, only for locals.
            if block.function is not None:
                break

            block = block.superblock

        return args
