# Copyright (C) 2015-2024 Free Software Foundation, Inc.

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

"""Unwinder class and register_unwinder function."""

import gdb


class Unwinder(object):
    """Base class (or a template) for frame unwinders written in Python.

    An unwinder has a single method __call__ and the attributes
    described below.

    Attributes:
        name: The name of the unwinder.
        enabled: A boolean indicating whether the unwinder is enabled.
    """

    def __init__(self, name):
        """Constructor.

        Args:
            name: An identifying name for the unwinder.
        """

        if not isinstance(name, str):
            raise TypeError("incorrect type for name: %s" % type(name))

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
        gdb.invalidate_cached_frames()

    def __call__(self, pending_frame):
        """GDB calls this method to unwind a frame.

        Arguments:
            pending_frame: gdb.PendingFrame instance.

        Returns:
            gdb.UnwindInfo instance.
        """
        raise NotImplementedError("Unwinder __call__.")


class FrameId(object):
    """A Frame-ID class for use when creating gdb.UnwindInfo objects.

    Attributes (all read-only):
        pc: Program counter value.
        sp: The stack-pointer value.
        special: An alternative stack-pointer value, can be None."""

    def __init__(self, sp, pc, special=None):
        self._sp = sp
        self._pc = pc
        self._special = special

    @property
    def sp(self):
        return self._sp

    @property
    def pc(self):
        return self._pc

    @property
    def special(self):
        return self._special


def register_unwinder(locus, unwinder, replace=False):
    """Register unwinder in given locus.

    The unwinder is prepended to the locus's unwinders list. Unwinder
    name should be unique.

    Arguments:
        locus: Either an objfile, progspace, or None (in which case
               the unwinder is registered globally).
        unwinder: An object of a gdb.Unwinder subclass
        replace: If True, replaces existing unwinder with the same name.
                 Otherwise, raises exception if unwinder with the same
                 name already exists.

    Returns:
        Nothing.

    Raises:
        RuntimeError: Unwinder name is not unique
        TypeError: Bad locus type
    """
    if locus is None:
        if gdb.parameter("verbose"):
            gdb.write("Registering global %s unwinder ...\n" % unwinder.name)
        locus = gdb
    elif isinstance(locus, gdb.Objfile) or isinstance(locus, gdb.Progspace):
        if gdb.parameter("verbose"):
            gdb.write(
                "Registering %s unwinder for %s ...\n" % (unwinder.name, locus.filename)
            )
    else:
        raise TypeError("locus should be gdb.Objfile or gdb.Progspace or None")

    i = 0
    for needle in locus.frame_unwinders:
        if needle.name == unwinder.name:
            if replace:
                del locus.frame_unwinders[i]
            else:
                raise RuntimeError("Unwinder %s already exists." % unwinder.name)
        i += 1
    locus.frame_unwinders.insert(0, unwinder)
    gdb.invalidate_cached_frames()
