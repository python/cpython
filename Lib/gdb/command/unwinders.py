# Unwinder commands.
# Copyright 2015-2024 Free Software Foundation, Inc.

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

import re

import gdb


def validate_regexp(exp, idstring):
    try:
        return re.compile(exp)
    except SyntaxError:
        raise SyntaxError("Invalid %s regexp: %s." % (idstring, exp))


def parse_unwinder_command_args(arg):
    """Internal utility to parse unwinder command argv.

    Arguments:
        arg: The arguments to the command. The format is:
             [locus-regexp [name-regexp]]

    Returns:
        A 2-tuple of compiled regular expressions.

    Raises:
        SyntaxError: an error processing ARG
    """

    argv = gdb.string_to_argv(arg)
    argc = len(argv)
    if argc > 2:
        raise SyntaxError("Too many arguments.")
    locus_regexp = ""
    name_regexp = ""
    if argc >= 1:
        locus_regexp = argv[0]
        if argc >= 2:
            name_regexp = argv[1]
    return (
        validate_regexp(locus_regexp, "locus"),
        validate_regexp(name_regexp, "unwinder"),
    )


class InfoUnwinder(gdb.Command):
    """GDB command to list unwinders.

    Usage: info unwinder [LOCUS-REGEXP [NAME-REGEXP]]

    LOCUS-REGEXP is a regular expression matching the location of the
    unwinder.  If it is omitted, all registered unwinders from all
    loci are listed.  A locus can be 'global', 'progspace' to list
    the unwinders from the current progspace, or a regular expression
    matching filenames of objfiles.

    NAME-REGEXP is a regular expression to filter unwinder names.  If
    this omitted for a specified locus, then all registered unwinders
    in the locus are listed."""

    def __init__(self):
        super(InfoUnwinder, self).__init__("info unwinder", gdb.COMMAND_STACK)

    def list_unwinders(self, title, unwinders, name_re):
        """Lists the unwinders whose name matches regexp.

        Arguments:
            title: The line to print before the list.
            unwinders: The list of the unwinders.
            name_re: unwinder name filter.
        """
        if not unwinders:
            return
        print(title)
        for unwinder in unwinders:
            if name_re.match(unwinder.name):
                print(
                    "  %s%s"
                    % (unwinder.name, "" if unwinder.enabled else " [disabled]")
                )

    def invoke(self, arg, from_tty):
        locus_re, name_re = parse_unwinder_command_args(arg)
        if locus_re.match("global"):
            self.list_unwinders("Global:", gdb.frame_unwinders, name_re)
        if locus_re.match("progspace"):
            cp = gdb.current_progspace()
            self.list_unwinders(
                "Progspace %s:" % cp.filename, cp.frame_unwinders, name_re
            )
        for objfile in gdb.objfiles():
            if locus_re.match(objfile.filename):
                self.list_unwinders(
                    "Objfile %s:" % objfile.filename, objfile.frame_unwinders, name_re
                )


def do_enable_unwinder1(unwinders, name_re, flag):
    """Enable/disable unwinders whose names match given regex.

    Arguments:
        unwinders: The list of unwinders.
        name_re: Unwinder name filter.
        flag: Enable/disable.

    Returns:
        The number of unwinders affected.
    """
    total = 0
    for unwinder in unwinders:
        if name_re.match(unwinder.name):
            unwinder.enabled = flag
            total += 1
    return total


def do_enable_unwinder(arg, flag):
    """Enable/disable unwinder(s)."""
    (locus_re, name_re) = parse_unwinder_command_args(arg)
    total = 0
    if locus_re.match("global"):
        total += do_enable_unwinder1(gdb.frame_unwinders, name_re, flag)
    if locus_re.match("progspace"):
        total += do_enable_unwinder1(
            gdb.current_progspace().frame_unwinders, name_re, flag
        )
    for objfile in gdb.objfiles():
        if locus_re.match(objfile.filename):
            total += do_enable_unwinder1(objfile.frame_unwinders, name_re, flag)
    if total > 0:
        gdb.invalidate_cached_frames()
    print(
        "%d unwinder%s %s"
        % (total, "" if total == 1 else "s", "enabled" if flag else "disabled")
    )


class EnableUnwinder(gdb.Command):
    """GDB command to enable unwinders.

    Usage: enable unwinder [LOCUS-REGEXP [NAME-REGEXP]]

    LOCUS-REGEXP is a regular expression specifying the unwinders to
    enable.  It can 'global', 'progspace', or the name of an objfile
    within that progspace.

    NAME_REGEXP is a regular expression to filter unwinder names.  If
    this omitted for a specified locus, then all registered unwinders
    in the locus are affected."""

    def __init__(self):
        super(EnableUnwinder, self).__init__("enable unwinder", gdb.COMMAND_STACK)

    def invoke(self, arg, from_tty):
        """GDB calls this to perform the command."""
        do_enable_unwinder(arg, True)


class DisableUnwinder(gdb.Command):
    """GDB command to disable the specified unwinder.

    Usage: disable unwinder [LOCUS-REGEXP [NAME-REGEXP]]

    LOCUS-REGEXP is a regular expression specifying the unwinders to
    disable.  It can 'global', 'progspace', or the name of an objfile
    within that progspace.

    NAME_REGEXP is a regular expression to filter unwinder names.  If
    this omitted for a specified locus, then all registered unwinders
    in the locus are affected."""

    def __init__(self):
        super(DisableUnwinder, self).__init__("disable unwinder", gdb.COMMAND_STACK)

    def invoke(self, arg, from_tty):
        """GDB calls this to perform the command."""
        do_enable_unwinder(arg, False)


def register_unwinder_commands():
    """Installs the unwinder commands."""
    InfoUnwinder()
    EnableUnwinder()
    DisableUnwinder()


register_unwinder_commands()
