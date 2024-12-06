# Frame-filter commands.
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

"""GDB commands for working with frame-filters."""

import sys

import gdb
import gdb.frames


# GDB Commands.
class SetFilterPrefixCmd(gdb.Command):
    """Prefix command for 'set' frame-filter related operations."""

    def __init__(self):
        super(SetFilterPrefixCmd, self).__init__(
            "set frame-filter", gdb.COMMAND_OBSCURE, gdb.COMPLETE_NONE, True
        )


class ShowFilterPrefixCmd(gdb.Command):
    """Prefix command for 'show' frame-filter related operations."""

    def __init__(self):
        super(ShowFilterPrefixCmd, self).__init__(
            "show frame-filter", gdb.COMMAND_OBSCURE, gdb.COMPLETE_NONE, True
        )


class InfoFrameFilter(gdb.Command):
    """List all registered Python frame-filters.

    Usage: info frame-filters"""

    def __init__(self):
        super(InfoFrameFilter, self).__init__("info frame-filter", gdb.COMMAND_DATA)

    @staticmethod
    def enabled_string(state):
        """Return "Yes" if filter is enabled, otherwise "No"."""
        if state:
            return "Yes"
        else:
            return "No"

    def print_list(self, title, frame_filters, blank_line):
        sorted_frame_filters = sorted(
            frame_filters.items(),
            key=lambda i: gdb.frames.get_priority(i[1]),
            reverse=True,
        )

        if len(sorted_frame_filters) == 0:
            return 0

        print(title)
        print("  Priority  Enabled  Name")
        for frame_filter in sorted_frame_filters:
            name = frame_filter[0]
            try:
                priority = "{:<8}".format(str(gdb.frames.get_priority(frame_filter[1])))
                enabled = "{:<7}".format(
                    self.enabled_string(gdb.frames.get_enabled(frame_filter[1]))
                )
                print("  %s  %s  %s" % (priority, enabled, name))
            except Exception:
                e = sys.exc_info()[1]
                print("  Error printing filter '" + name + "': " + str(e))
        if blank_line:
            print("")
        return 1

    def invoke(self, arg, from_tty):
        any_printed = self.print_list("global frame-filters:", gdb.frame_filters, True)

        cp = gdb.current_progspace()
        any_printed += self.print_list(
            "progspace %s frame-filters:" % cp.filename, cp.frame_filters, True
        )

        for objfile in gdb.objfiles():
            any_printed += self.print_list(
                "objfile %s frame-filters:" % objfile.filename,
                objfile.frame_filters,
                False,
            )

        if any_printed == 0:
            print("No frame filters.")


# Internal enable/disable functions.


def _enable_parse_arg(cmd_name, arg):
    """Internal worker function to take an argument from
    enable/disable and return a tuple of arguments.

    Arguments:
        cmd_name: Name of the command invoking this function.
        args: The argument as a string.

    Returns:
        A tuple containing the dictionary, and the argument, or just
        the dictionary in the case of "all".
    """

    argv = gdb.string_to_argv(arg)
    argc = len(argv)
    if argc == 0:
        raise gdb.GdbError(cmd_name + " requires an argument")
    if argv[0] == "all":
        if argc > 1:
            raise gdb.GdbError(
                cmd_name + ": with 'all' " "you may not specify a filter."
            )
    elif argc != 2:
        raise gdb.GdbError(cmd_name + " takes exactly two arguments.")

    return argv


def _do_enable_frame_filter(command_tuple, flag):
    """Worker for enabling/disabling frame_filters.

    Arguments:
        command_type: A tuple with the first element being the
                      frame filter dictionary, and the second being
                      the frame filter name.
        flag: True for Enable, False for Disable.
    """

    list_op = command_tuple[0]
    op_list = gdb.frames.return_list(list_op)

    if list_op == "all":
        for item in op_list:
            gdb.frames.set_enabled(item, flag)
    else:
        frame_filter = command_tuple[1]
        try:
            ff = op_list[frame_filter]
        except KeyError:
            msg = "frame-filter '" + str(frame_filter) + "' not found."
            raise gdb.GdbError(msg)

        gdb.frames.set_enabled(ff, flag)


def _complete_frame_filter_list(text, word, all_flag):
    """Worker for frame filter dictionary name completion.

    Arguments:
        text: The full text of the command line.
        word: The most recent word of the command line.
        all_flag: Whether to include the word "all" in completion.

    Returns:
        A list of suggested frame filter dictionary name completions
        from text/word analysis.  This list can be empty when there
        are no suggestions for completion.
    """
    if all_flag:
        filter_locations = ["all", "global", "progspace"]
    else:
        filter_locations = ["global", "progspace"]
    for objfile in gdb.objfiles():
        filter_locations.append(objfile.filename)

    # If the user just asked for completions with no completion
    # hints, just return all the frame filter dictionaries we know
    # about.
    if text == "":
        return filter_locations

    # Otherwise filter on what we know.
    flist = filter(lambda x, y=text: x.startswith(y), filter_locations)

    # If we only have one completion, complete it and return it.
    if len(flist) == 1:
        flist[0] = flist[0][len(text) - len(word) :]

    # Otherwise, return an empty list, or a list of frame filter
    # dictionaries that the previous filter operation returned.
    return flist


def _complete_frame_filter_name(word, printer_dict):
    """Worker for frame filter name completion.

    Arguments:

        word: The most recent word of the command line.

        printer_dict: The frame filter dictionary to search for frame
        filter name completions.

        Returns: A list of suggested frame filter name completions
        from word analysis of the frame filter dictionary.  This list
        can be empty when there are no suggestions for completion.
    """

    printer_keys = printer_dict.keys()
    if word == "":
        return printer_keys

    flist = filter(lambda x, y=word: x.startswith(y), printer_keys)
    return flist


class EnableFrameFilter(gdb.Command):
    """GDB command to enable the specified frame-filter.

    Usage: enable frame-filter DICTIONARY [NAME]

    DICTIONARY is the name of the frame filter dictionary on which to
    operate.  If dictionary is set to "all", perform operations on all
    dictionaries.  Named dictionaries are: "global" for the global
    frame filter dictionary, "progspace" for the program space's frame
    filter dictionary.  If either all, or the two named dictionaries
    are not specified, the dictionary name is assumed to be the name
    of an "objfile" -- a shared library or an executable.

    NAME matches the name of the frame-filter to operate on."""

    def __init__(self):
        super(EnableFrameFilter, self).__init__("enable frame-filter", gdb.COMMAND_DATA)

    def complete(self, text, word):
        """Completion function for both frame filter dictionary, and
        frame filter name."""
        if text.count(" ") == 0:
            return _complete_frame_filter_list(text, word, True)
        else:
            printer_list = gdb.frames.return_list(text.split()[0].rstrip())
            return _complete_frame_filter_name(word, printer_list)

    def invoke(self, arg, from_tty):
        command_tuple = _enable_parse_arg("enable frame-filter", arg)
        _do_enable_frame_filter(command_tuple, True)


class DisableFrameFilter(gdb.Command):
    """GDB command to disable the specified frame-filter.

    Usage: disable frame-filter DICTIONARY [NAME]

    DICTIONARY is the name of the frame filter dictionary on which to
    operate.  If dictionary is set to "all", perform operations on all
    dictionaries.  Named dictionaries are: "global" for the global
    frame filter dictionary, "progspace" for the program space's frame
    filter dictionary.  If either all, or the two named dictionaries
    are not specified, the dictionary name is assumed to be the name
    of an "objfile" -- a shared library or an executable.

    NAME matches the name of the frame-filter to operate on."""

    def __init__(self):
        super(DisableFrameFilter, self).__init__(
            "disable frame-filter", gdb.COMMAND_DATA
        )

    def complete(self, text, word):
        """Completion function for both frame filter dictionary, and
        frame filter name."""
        if text.count(" ") == 0:
            return _complete_frame_filter_list(text, word, True)
        else:
            printer_list = gdb.frames.return_list(text.split()[0].rstrip())
            return _complete_frame_filter_name(word, printer_list)

    def invoke(self, arg, from_tty):
        command_tuple = _enable_parse_arg("disable frame-filter", arg)
        _do_enable_frame_filter(command_tuple, False)


class SetFrameFilterPriority(gdb.Command):
    """GDB command to set the priority of the specified frame-filter.

    Usage: set frame-filter priority DICTIONARY NAME PRIORITY

    DICTIONARY is the name of the frame filter dictionary on which to
    operate.  Named dictionaries are: "global" for the global frame
    filter dictionary, "progspace" for the program space's framefilter
    dictionary.  If either of these two are not specified, the
    dictionary name is assumed to be the name of an "objfile" -- a
    shared library or an executable.

    NAME matches the name of the frame filter to operate on.

    PRIORITY is the an integer to assign the new priority to the frame
    filter."""

    def __init__(self):
        super(SetFrameFilterPriority, self).__init__(
            "set frame-filter " "priority", gdb.COMMAND_DATA
        )

    def _parse_pri_arg(self, arg):
        """Internal worker to parse a priority from a tuple.

        Arguments:
            arg: Tuple which contains the arguments from the command.

        Returns:
            A tuple containing the dictionary, name and priority from
            the arguments.

        Raises:
            gdb.GdbError: An error parsing the arguments.
        """

        argv = gdb.string_to_argv(arg)
        argc = len(argv)
        if argc != 3:
            print("set frame-filter priority " "takes exactly three arguments.")
            return None

        return argv

    def _set_filter_priority(self, command_tuple):
        """Internal worker for setting priority of frame-filters, by
        parsing a tuple and calling _set_priority with the parsed
        tuple.

        Arguments:
            command_tuple: Tuple which contains the arguments from the
                           command.
        """

        list_op = command_tuple[0]
        frame_filter = command_tuple[1]

        # GDB returns arguments as a string, so convert priority to
        # a number.
        priority = int(command_tuple[2])

        op_list = gdb.frames.return_list(list_op)

        try:
            ff = op_list[frame_filter]
        except KeyError:
            msg = "frame-filter '" + str(frame_filter) + "' not found."
            raise gdb.GdbError(msg)

        gdb.frames.set_priority(ff, priority)

    def complete(self, text, word):
        """Completion function for both frame filter dictionary, and
        frame filter name."""
        if text.count(" ") == 0:
            return _complete_frame_filter_list(text, word, False)
        else:
            printer_list = gdb.frames.return_list(text.split()[0].rstrip())
            return _complete_frame_filter_name(word, printer_list)

    def invoke(self, arg, from_tty):
        command_tuple = self._parse_pri_arg(arg)
        if command_tuple is not None:
            self._set_filter_priority(command_tuple)


class ShowFrameFilterPriority(gdb.Command):
    """GDB command to show the priority of the specified frame-filter.

    Usage: show frame-filter priority DICTIONARY NAME

    DICTIONARY is the name of the frame filter dictionary on which to
    operate.  Named dictionaries are: "global" for the global frame
    filter dictionary, "progspace" for the program space's framefilter
    dictionary.  If either of these two are not specified, the
    dictionary name is assumed to be the name of an "objfile" -- a
    shared library or an executable.

    NAME matches the name of the frame-filter to operate on."""

    def __init__(self):
        super(ShowFrameFilterPriority, self).__init__(
            "show frame-filter " "priority", gdb.COMMAND_DATA
        )

    def _parse_pri_arg(self, arg):
        """Internal worker to parse a dictionary and name from a
        tuple.

        Arguments:
            arg: Tuple which contains the arguments from the command.

        Returns:
            A tuple containing the dictionary,  and frame filter name.

        Raises:
            gdb.GdbError: An error parsing the arguments.
        """

        argv = gdb.string_to_argv(arg)
        argc = len(argv)
        if argc != 2:
            print("show frame-filter priority " "takes exactly two arguments.")
            return None

        return argv

    def get_filter_priority(self, frame_filters, name):
        """Worker for retrieving the priority of frame_filters.

        Arguments:
            frame_filters: Name of frame filter dictionary.
            name: object to select printers.

        Returns:
            The priority of the frame filter.

        Raises:
            gdb.GdbError: A frame filter cannot be found.
        """

        op_list = gdb.frames.return_list(frame_filters)

        try:
            ff = op_list[name]
        except KeyError:
            msg = "frame-filter '" + str(name) + "' not found."
            raise gdb.GdbError(msg)

        return gdb.frames.get_priority(ff)

    def complete(self, text, word):
        """Completion function for both frame filter dictionary, and
        frame filter name."""

        if text.count(" ") == 0:
            return _complete_frame_filter_list(text, word, False)
        else:
            printer_list = gdb.frames.return_list(text.split()[0].rstrip())
            return _complete_frame_filter_name(word, printer_list)

    def invoke(self, arg, from_tty):
        command_tuple = self._parse_pri_arg(arg)
        if command_tuple is None:
            return
        filter_name = command_tuple[1]
        list_name = command_tuple[0]
        priority = self.get_filter_priority(list_name, filter_name)
        print(
            "Priority of filter '"
            + filter_name
            + "' in list '"
            + list_name
            + "' is: "
            + str(priority)
        )


# Register commands
SetFilterPrefixCmd()
ShowFilterPrefixCmd()
InfoFrameFilter()
EnableFrameFilter()
DisableFrameFilter()
SetFrameFilterPriority()
ShowFrameFilterPriority()
