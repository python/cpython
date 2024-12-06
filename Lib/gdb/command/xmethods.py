# Xmethod commands.
# Copyright 2013-2024 Free Software Foundation, Inc.

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

"""GDB commands for working with xmethods."""


def validate_xm_regexp(part_name, regexp):
    try:
        return re.compile(regexp)
    except SyntaxError:
        raise SyntaxError("Invalid %s regexp: %s", part_name, regexp)


def parse_xm_command_args(arg):
    """Parses the arguments passed to a xmethod command.

    Arguments:
        arg: The argument string passed to a xmethod command.

    Returns:
        A 3-tuple: (<locus matching regular expression>,
                    <matcher matching regular expression>,
                    <name matching regular experession>)
    """
    argv = gdb.string_to_argv(arg)
    argc = len(argv)
    if argc > 2:
        raise SyntaxError("Too many arguments to command.")
    locus_regexp = ""
    matcher_name_regexp = ""
    xm_name_regexp = None
    if argc >= 1:
        locus_regexp = argv[0]
    if argc == 2:
        parts = argv[1].split(";", 1)
        matcher_name_regexp = parts[0]
        if len(parts) > 1:
            xm_name_regexp = parts[1]
    if xm_name_regexp:
        name_re = validate_xm_regexp("xmethod name", xm_name_regexp)
    else:
        name_re = None
    return (
        validate_xm_regexp("locus", locus_regexp),
        validate_xm_regexp("matcher name", matcher_name_regexp),
        name_re,
    )


def get_global_method_matchers(locus_re, matcher_re):
    """Returns a dict of matching globally registered xmethods.

    Arguments:
        locus_re: Even though only globally registered xmethods are
                  looked up, they will be looked up only if 'global' matches
                  LOCUS_RE.
        matcher_re: The regular expression matching the names of xmethods.

    Returns:
        A dict of matching globally registered xmethod matchers.  The only
        key in the dict will be 'global'.
    """
    locus_str = "global"
    xm_dict = {locus_str: []}
    if locus_re.match("global"):
        xm_dict[locus_str].extend([m for m in gdb.xmethods if matcher_re.match(m.name)])
    return xm_dict


def get_method_matchers_in_loci(loci, locus_re, matcher_re):
    """Returns a dict of matching registered xmethods in the LOCI.

    Arguments:
        loci: The list of loci to lookup matching xmethods in.
        locus_re: If a locus is an objfile, then xmethod matchers will be
                  looked up in it only if its filename matches the regular
                  expression LOCUS_RE.  If a locus is the current progspace,
                  then xmethod matchers will be looked up in it only if the
                  string "progspace" matches LOCUS_RE.
        matcher_re: The regular expression to match the xmethod matcher
                    names.

    Returns:
        A dict of matching xmethod matchers.  The keys of the dict are the
        filenames of the loci the xmethod matchers belong to.
    """
    xm_dict = {}
    for locus in loci:
        if isinstance(locus, gdb.Progspace):
            if not locus_re.match("progspace"):
                continue
            locus_type = "progspace"
        else:
            if not locus_re.match(locus.filename):
                continue
            locus_type = "objfile"
        locus_str = "%s %s" % (locus_type, locus.filename)
        xm_dict[locus_str] = [m for m in locus.xmethods if matcher_re.match(m.name)]
    return xm_dict


def print_xm_info(xm_dict, name_re):
    """Print a dictionary of xmethods."""

    def get_status_string(m):
        if not m.enabled:
            return " [disabled]"
        else:
            return ""

    if not xm_dict:
        return
    for locus_str in xm_dict:
        if not xm_dict[locus_str]:
            continue
        print("Xmethods in %s:" % locus_str)
        for matcher in xm_dict[locus_str]:
            print("  %s%s" % (matcher.name, get_status_string(matcher)))
            if not matcher.methods:
                continue
            for m in matcher.methods:
                if name_re is None or name_re.match(m.name):
                    print("    %s%s" % (m.name, get_status_string(m)))


def set_xm_status1(xm_dict, name_re, status):
    """Set the status (enabled/disabled) of a dictionary of xmethods."""
    for locus_str, matchers in xm_dict.items():
        for matcher in matchers:
            if not name_re:
                # If the name regex is missing, then set the status of the
                # matcher and move on.
                matcher.enabled = status
                continue
            if not matcher.methods:
                # The methods attribute could be None.  Move on.
                continue
            for m in matcher.methods:
                if name_re.match(m.name):
                    m.enabled = status


def set_xm_status(arg, status):
    """Set the status (enabled/disabled) of xmethods matching ARG.
    This is a helper function for enable/disable commands.  ARG is the
    argument string passed to the commands.
    """
    locus_re, matcher_re, name_re = parse_xm_command_args(arg)
    set_xm_status1(get_global_method_matchers(locus_re, matcher_re), name_re, status)
    set_xm_status1(
        get_method_matchers_in_loci([gdb.current_progspace()], locus_re, matcher_re),
        name_re,
        status,
    )
    set_xm_status1(
        get_method_matchers_in_loci(gdb.objfiles(), locus_re, matcher_re),
        name_re,
        status,
    )


class InfoXMethod(gdb.Command):
    """GDB command to list registered xmethod matchers.

    Usage: info xmethod [LOCUS-REGEXP [NAME-REGEXP]]

    LOCUS-REGEXP is a regular expression matching the location of the
    xmethod matchers.  If it is omitted, all registered xmethod matchers
    from all loci are listed.  A locus could be 'global', a regular expression
    matching the current program space's filename, or a regular expression
    matching filenames of objfiles.  Locus could be 'progspace' to specify that
    only xmethods from the current progspace should be listed.

    NAME-REGEXP is a regular expression matching the names of xmethod
    matchers.  If this omitted for a specified locus, then all registered
    xmethods in the locus are listed.  To list only a certain xmethods
    managed by a single matcher, the name regexp can be specified as
    matcher-name-regexp;xmethod-name-regexp."""

    def __init__(self):
        super(InfoXMethod, self).__init__("info xmethod", gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        locus_re, matcher_re, name_re = parse_xm_command_args(arg)
        print_xm_info(get_global_method_matchers(locus_re, matcher_re), name_re)
        print_xm_info(
            get_method_matchers_in_loci(
                [gdb.current_progspace()], locus_re, matcher_re
            ),
            name_re,
        )
        print_xm_info(
            get_method_matchers_in_loci(gdb.objfiles(), locus_re, matcher_re), name_re
        )


class EnableXMethod(gdb.Command):
    """GDB command to enable a specified (group of) xmethod(s).

    Usage: enable xmethod [LOCUS-REGEXP [NAME-REGEXP]]

    LOCUS-REGEXP is a regular expression matching the location of the
    xmethod matchers.  If it is omitted, all registered xmethods matchers
    from all loci are enabled.  A locus could be 'global', a regular expression
    matching the current program space's filename, or a regular expression
    matching filenames of objfiles.  Locus could be 'progspace' to specify that
    only xmethods from the current progspace should be enabled.

    NAME-REGEXP is a regular expression matching the names of xmethods
    within a given locus.  If this omitted for a specified locus, then all
    registered xmethod matchers in the locus are enabled.  To enable only
    a certain xmethods managed by a single matcher, the name regexp can be
    specified as matcher-name-regexp;xmethod-name-regexp."""

    def __init__(self):
        super(EnableXMethod, self).__init__("enable xmethod", gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        set_xm_status(arg, True)


class DisableXMethod(gdb.Command):
    """GDB command to disable a specified (group of) xmethod(s).

    Usage: disable xmethod [LOCUS-REGEXP [NAME-REGEXP]]

    LOCUS-REGEXP is a regular expression matching the location of the
    xmethod matchers.  If it is omitted, all registered xmethod matchers
    from all loci are disabled.  A locus could be 'global', a regular
    expression matching the current program space's filename, or a regular
    expression filenames of objfiles. Locus could be 'progspace' to specify
    that only xmethods from the current progspace should be disabled.

    NAME-REGEXP is a regular expression matching the names of xmethods
    within a given locus.  If this omitted for a specified locus, then all
    registered xmethod matchers in the locus are disabled.  To disable
    only a certain xmethods managed by a single matcher, the name regexp
    can be specified as matcher-name-regexp;xmethod-name-regexp."""

    def __init__(self):
        super(DisableXMethod, self).__init__("disable xmethod", gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        set_xm_status(arg, False)


def register_xmethod_commands():
    """Installs the xmethod commands."""
    InfoXMethod()
    EnableXMethod()
    DisableXMethod()


register_xmethod_commands()
