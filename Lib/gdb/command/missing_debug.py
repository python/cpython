# Missing debug related commands.
#
# Copyright 2023-2024 Free Software Foundation, Inc.
#
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
    """Compile exp into a compiler regular expression object.

    Arguments:
        exp: The string to compile into a re.Pattern object.
        idstring: A string, what exp is a regexp for.

    Returns:
        A re.Pattern object representing exp.

    Raises:
        SyntaxError: If exp is an invalid regexp.
    """
    try:
        return re.compile(exp)
    except SyntaxError:
        raise SyntaxError("Invalid %s regexp: %s." % (idstring, exp))


def parse_missing_debug_command_args(arg):
    """Internal utility to parse missing debug handler command argv.

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
        validate_regexp(name_regexp, "handler"),
    )


class InfoMissingDebugHanders(gdb.Command):
    """GDB command to list missing debug handlers.

    Usage: info missing-debug-handlers [LOCUS-REGEXP [NAME-REGEXP]]

    LOCUS-REGEXP is a regular expression matching the location of the
    handler.  If it is omitted, all registered handlers from all
    loci are listed.  A locus can be 'global', 'progspace' to list
    the handlers from the current progspace, or a regular expression
    matching filenames of progspaces.

    NAME-REGEXP is a regular expression to filter missing debug
    handler names.  If this omitted for a specified locus, then all
    registered handlers in the locus are listed.
    """

    def __init__(self):
        super().__init__("info missing-debug-handlers", gdb.COMMAND_FILES)

    def list_handlers(self, title, handlers, name_re):
        """Lists the missing debug handlers whose name matches regexp.

        Arguments:
            title: The line to print before the list.
            handlers: The list of the missing debug handlers.
            name_re: handler name filter.
        """
        if not handlers:
            return
        print(title)
        for handler in handlers:
            if name_re.match(handler.name):
                print(
                    "  %s%s" % (handler.name, "" if handler.enabled else " [disabled]")
                )

    def invoke(self, arg, from_tty):
        locus_re, name_re = parse_missing_debug_command_args(arg)

        if locus_re.match("progspace") and locus_re.pattern != "":
            cp = gdb.current_progspace()
            self.list_handlers(
                "Progspace %s:" % cp.filename, cp.missing_debug_handlers, name_re
            )

        for progspace in gdb.progspaces():
            filename = progspace.filename or ""
            if locus_re.match(filename):
                if filename == "":
                    if progspace == gdb.current_progspace():
                        msg = "Current Progspace:"
                    else:
                        msg = "Progspace <no-file>:"
                else:
                    msg = "Progspace %s:" % filename
                self.list_handlers(
                    msg,
                    progspace.missing_debug_handlers,
                    name_re,
                )

        # Print global handlers last, as these are invoked last.
        if locus_re.match("global"):
            self.list_handlers("Global:", gdb.missing_debug_handlers, name_re)


def do_enable_handler1(handlers, name_re, flag):
    """Enable/disable missing debug handlers whose names match given regex.

    Arguments:
        handlers: The list of missing debug handlers.
        name_re: Handler name filter.
        flag: A boolean indicating if we should enable or disable.

    Returns:
        The number of handlers affected.
    """
    total = 0
    for handler in handlers:
        if name_re.match(handler.name) and handler.enabled != flag:
            handler.enabled = flag
            total += 1
    return total


def do_enable_handler(arg, flag):
    """Enable or disable missing debug handlers."""
    (locus_re, name_re) = parse_missing_debug_command_args(arg)
    total = 0
    if locus_re.match("global"):
        total += do_enable_handler1(gdb.missing_debug_handlers, name_re, flag)
    if locus_re.match("progspace") and locus_re.pattern != "":
        total += do_enable_handler1(
            gdb.current_progspace().missing_debug_handlers, name_re, flag
        )
    for progspace in gdb.progspaces():
        filename = progspace.filename or ""
        if locus_re.match(filename):
            total += do_enable_handler1(progspace.missing_debug_handlers, name_re, flag)
    print(
        "%d missing debug handler%s %s"
        % (total, "" if total == 1 else "s", "enabled" if flag else "disabled")
    )


class EnableMissingDebugHandler(gdb.Command):
    """GDB command to enable missing debug handlers.

    Usage: enable missing-debug-handler [LOCUS-REGEXP [NAME-REGEXP]]

    LOCUS-REGEXP is a regular expression specifying the handlers to
    enable.  It can be 'global', 'progspace' for the current
    progspace, or the filename for a file associated with a progspace.

    NAME_REGEXP is a regular expression to filter handler names.  If
    this omitted for a specified locus, then all registered handlers
    in the locus are affected.
    """

    def __init__(self):
        super().__init__("enable missing-debug-handler", gdb.COMMAND_FILES)

    def invoke(self, arg, from_tty):
        """GDB calls this to perform the command."""
        do_enable_handler(arg, True)


class DisableMissingDebugHandler(gdb.Command):
    """GDB command to disable missing debug handlers.

    Usage: disable missing-debug-handler [LOCUS-REGEXP [NAME-REGEXP]]

    LOCUS-REGEXP is a regular expression specifying the handlers to
    enable.  It can be 'global', 'progspace' for the current
    progspace, or the filename for a file associated with a progspace.

    NAME_REGEXP is a regular expression to filter handler names.  If
    this omitted for a specified locus, then all registered handlers
    in the locus are affected.
    """

    def __init__(self):
        super().__init__("disable missing-debug-handler", gdb.COMMAND_FILES)

    def invoke(self, arg, from_tty):
        """GDB calls this to perform the command."""
        do_enable_handler(arg, False)


def register_missing_debug_handler_commands():
    """Installs the missing debug handler commands."""
    InfoMissingDebugHanders()
    EnableMissingDebugHandler()
    DisableMissingDebugHandler()


register_missing_debug_handler_commands()
