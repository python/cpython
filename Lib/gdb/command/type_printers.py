# Type printer commands.
# Copyright (C) 2010-2024 Free Software Foundation, Inc.

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

import copy

import gdb

"""GDB commands for working with type-printers."""


class InfoTypePrinter(gdb.Command):
    """GDB command to list all registered type-printers.

    Usage: info type-printers"""

    def __init__(self):
        super(InfoTypePrinter, self).__init__("info type-printers", gdb.COMMAND_DATA)

    def list_type_printers(self, type_printers):
        """Print a list of type printers."""
        # A potential enhancement is to provide an option to list printers in
        # "lookup order" (i.e. unsorted).
        sorted_type_printers = sorted(copy.copy(type_printers), key=lambda x: x.name)
        for printer in sorted_type_printers:
            if printer.enabled:
                enabled = ""
            else:
                enabled = " [disabled]"
            print("  %s%s" % (printer.name, enabled))

    def invoke(self, arg, from_tty):
        """GDB calls this to perform the command."""
        sep = ""
        for objfile in gdb.objfiles():
            if objfile.type_printers:
                print("%sType printers for %s:" % (sep, objfile.filename))
                self.list_type_printers(objfile.type_printers)
                sep = "\n"
        if gdb.current_progspace().type_printers:
            print("%sType printers for program space:" % sep)
            self.list_type_printers(gdb.current_progspace().type_printers)
            sep = "\n"
        if gdb.type_printers:
            print("%sGlobal type printers:" % sep)
            self.list_type_printers(gdb.type_printers)


class _EnableOrDisableCommand(gdb.Command):
    def __init__(self, setting, name):
        super(_EnableOrDisableCommand, self).__init__(name, gdb.COMMAND_DATA)
        self.setting = setting

    def set_some(self, name, printers):
        result = False
        for p in printers:
            if name == p.name:
                p.enabled = self.setting
                result = True
        return result

    def invoke(self, arg, from_tty):
        """GDB calls this to perform the command."""
        for name in arg.split():
            ok = False
            for objfile in gdb.objfiles():
                if self.set_some(name, objfile.type_printers):
                    ok = True
            if self.set_some(name, gdb.current_progspace().type_printers):
                ok = True
            if self.set_some(name, gdb.type_printers):
                ok = True
            if not ok:
                print("No type printer named '%s'" % name)

    def add_some(self, result, word, printers):
        for p in printers:
            if p.name.startswith(word):
                result.append(p.name)

    def complete(self, text, word):
        result = []
        for objfile in gdb.objfiles():
            self.add_some(result, word, objfile.type_printers)
        self.add_some(result, word, gdb.current_progspace().type_printers)
        self.add_some(result, word, gdb.type_printers)
        return result


class EnableTypePrinter(_EnableOrDisableCommand):
    """GDB command to enable the specified type printer.

    Usage: enable type-printer NAME

    NAME is the name of the type-printer."""

    def __init__(self):
        super(EnableTypePrinter, self).__init__(True, "enable type-printer")


class DisableTypePrinter(_EnableOrDisableCommand):
    """GDB command to disable the specified type-printer.

    Usage: disable type-printer NAME

    NAME is the name of the type-printer."""

    def __init__(self):
        super(DisableTypePrinter, self).__init__(False, "disable type-printer")


InfoTypePrinter()
EnableTypePrinter()
DisableTypePrinter()
