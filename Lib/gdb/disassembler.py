# Copyright (C) 2021-2024 Free Software Foundation, Inc.

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

"""Disassembler related module."""

import _gdb.disassembler

# Re-export everything from the _gdb.disassembler module, which is
# defined within GDB's C++ code.  Note that two indicators are needed
# here to silence flake8.
from _gdb.disassembler import *  # noqa: F401,F403

import gdb

# Module global dictionary of gdb.disassembler.Disassembler objects.
# The keys of this dictionary are bfd architecture names, or the
# special value None.
#
# When a request to disassemble comes in we first lookup the bfd
# architecture name from the gdbarch, if that name exists in this
# dictionary then we use that Disassembler object.
#
# If there's no architecture specific disassembler then we look for
# the key None in this dictionary, and if that key exists, we use that
# disassembler.
#
# If none of the above checks found a suitable disassembler, then no
# disassembly is performed in Python.
_disassemblers_dict = {}


class Disassembler(object):
    """A base class from which all user implemented disassemblers must
    inherit."""

    def __init__(self, name):
        """Constructor.  Takes a name, which should be a string, which can be
        used to identify this disassembler in diagnostic messages."""
        self.name = name

    def __call__(self, info):
        """A default implementation of __call__.  All sub-classes must
        override this method.  Calling this default implementation will throw
        a NotImplementedError exception."""
        raise NotImplementedError("Disassembler.__call__")


def register_disassembler(disassembler, architecture=None):
    """Register a disassembler.  DISASSEMBLER is a sub-class of
    gdb.disassembler.Disassembler.  ARCHITECTURE is either None or a
    string, the name of an architecture known to GDB.

    DISASSEMBLER is registered as a disassembler for ARCHITECTURE, or
    all architectures when ARCHITECTURE is None.

    Returns the previous disassembler registered with this
    ARCHITECTURE value.
    """

    if not isinstance(disassembler, Disassembler) and disassembler is not None:
        raise TypeError("disassembler should sub-class gdb.disassembler.Disassembler")

    old = None
    if architecture in _disassemblers_dict:
        old = _disassemblers_dict[architecture]
        del _disassemblers_dict[architecture]
    if disassembler is not None:
        _disassemblers_dict[architecture] = disassembler

    # Call the private _set_enabled function within the
    # _gdb.disassembler module.  This function sets a global flag
    # within GDB's C++ code that enables or dissables the Python
    # disassembler functionality, this improves performance of the
    # disassembler by avoiding unneeded calls into Python when we know
    # that no disassemblers are registered.
    _gdb.disassembler._set_enabled(len(_disassemblers_dict) > 0)
    return old


def _print_insn(info):
    """This function is called by GDB when it wants to disassemble an
    instruction.  INFO describes the instruction to be
    disassembled."""

    def lookup_disassembler(arch):
        name = arch.name()
        if name is None:
            return None
        if name in _disassemblers_dict:
            return _disassemblers_dict[name]
        if None in _disassemblers_dict:
            return _disassemblers_dict[None]
        return None

    disassembler = lookup_disassembler(info.architecture)
    if disassembler is None:
        return None
    return disassembler(info)


class maint_info_py_disassemblers_cmd(gdb.Command):
    """
    List all registered Python disassemblers.

    List the name of all registered Python disassemblers, next to the
    name of the architecture for which the disassembler is registered.

    The global Python disassembler is listed next to the string
    'GLOBAL'.

    The disassembler that matches the architecture of the currently
    selected inferior will be marked, this is an indication of which
    disassembler will be invoked if any disassembly is performed in
    the current inferior.
    """

    def __init__(self):
        super().__init__("maintenance info python-disassemblers", gdb.COMMAND_USER)

    def invoke(self, args, from_tty):
        # If no disassemblers are registered, tell the user.
        if len(_disassemblers_dict) == 0:
            print("No Python disassemblers registered.")
            return

        # Figure out the longest architecture name, so we can
        # correctly format the table of results.
        longest_arch_name = 0
        for architecture in _disassemblers_dict:
            if architecture is not None:
                name = _disassemblers_dict[architecture].name
                if len(name) > longest_arch_name:
                    longest_arch_name = len(name)

        # Figure out the name of the current architecture.  There
        # should always be a current inferior, but if, somehow, there
        # isn't, then leave curr_arch as the empty string, which will
        # not then match agaisnt any architecture in the dictionary.
        curr_arch = ""
        if gdb.selected_inferior() is not None:
            curr_arch = gdb.selected_inferior().architecture().name()

        # Now print the dictionary of registered disassemblers out to
        # the user.
        match_tag = "\t(Matches current architecture)"
        fmt_len = max(longest_arch_name, len("Architecture"))
        format_string = "{:" + str(fmt_len) + "s} {:s}"
        print(format_string.format("Architecture", "Disassember Name"))
        for architecture in _disassemblers_dict:
            if architecture is not None:
                name = _disassemblers_dict[architecture].name
                if architecture == curr_arch:
                    name += match_tag
                    match_tag = ""
                print(format_string.format(architecture, name))
        if None in _disassemblers_dict:
            name = _disassemblers_dict[None].name + match_tag
            print(format_string.format("GLOBAL", name))


maint_info_py_disassemblers_cmd()
